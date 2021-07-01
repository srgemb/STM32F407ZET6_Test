
//*************************************************************************************************
//
// Управление протоколом MODBUS
//
//*************************************************************************************************

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "main.h"
#include "cmsis_os.h"

#include "crc16.h"
#include "modbus.h"
#include "uart.h"
#include "event.h"
#include "host.h"
#include "modbus_def.h"

//*************************************************************************************************
// Внешние переменные
//*************************************************************************************************
extern UART_HandleTypeDef huart2;

//*************************************************************************************************
// Локальные константы
//*************************************************************************************************
#define RS485_MODE_SEND         1
#define RS485_MODE_RECV         0
#define TIMEOUT_ANSWER          200         //время ожидания ответа от уст-ва (ms)

#define MB_ANSWER_DEV           0           //индекс ID уст-ва ответа
#define MB_ANSWER_FUNC          1           //индекс кода функции ответа
#define MB_ANSWER_COUNT         2           //индекс кол-ва байт данных ответа (без CRC)
#define MB_ANSWER_ERROR         2           //индекс кода ошибки
#define MB_ANSWER_DATA1         3           //индекс начала данных в пакете
#define MB_ANSWER_DATA2         4           //индекс начала данных в пакете

#define SIZE_CRC                2           //кол-во байт для хранения КС
#define SIZE_PACK_ERROR         5           //кол-во байт ответа с ошибкой
#define SIZE_PACK_0F10          8           //кол-во байт ответа функций 0F,10

#define CNT_RECV_CHECK          3           //кол-во принятых байт для предварительной проверки пакета

#define IND_BYTE_CMD01_04       2           //позиция в пакете кол-ва байт данных для функций 01,02,03,04

#define CALC_BYTE( bits )        ( bits/8 + bits%8 )

//расшифровка ошибок протокола MODBUS
static char * const modbus_error_descr[] = {
    "OK",                                           //MBUS_ANSWER_OK
    "Function code not supported",                  //MBUS_ERROR_FUNC
    "Data address not available",                   //MBUS_ERROR_ADDR
    "Invalid value in data field",                  //MBUS_ERROR_DATA
    "Unrecoverable error",                          //MBUS_ERROR_DEV
    "It takes time to process the request",         //MBUS_ERROR_ACKWAIT
    "Busy processing command",                      //MBUS_ERROR_BUSY
    "The slave cannot execute the function",        //MBUS_ERROR_NOACK
    "Memory Parity Error",                          //MBUS_ERROR_MEMCRC
    "Errors in function call parameters",           //MBUS_ERROR_PARAM
    "Received packet checksum error",               //MBUS_ANSWER_CRC
    "The slave is not responding"                   //MBUS_ANSWER_TIMEOUT
   };

#pragma pack( push, 1 )                 //выравнивание структуры по границе 1 байта

//Структура регистров для запроса чтения (01,02,03,04)
typedef struct {
    uint8_t  dev_addr;                  //Адрес устройства
    uint8_t  function;                  //Функциональный код
    uint16_t addr_reg;                  //Адрес первого регистра HI/LO байт
    uint16_t cnt_reg;                   //Количество регистров HI/LO байт
    uint16_t crc;                       //Контрольная сумма CRC
 } MBUS_REQ_REG;

 //Структура регистров для записи (05,06) дискретная/16-битная
 typedef struct {
    uint8_t  dev_addr;                  //Адрес устройства
    uint8_t  function;                  //Функциональный код
    uint16_t addr_reg;                  //Адрес регистра HI/LO байт
    uint16_t reg_val;                   //Значение HI/LO байт
    uint16_t crc;                       //Контрольная сумма CRC
   } MBUS_WRT_REG;

//Структура для записи значений в несколько регистров (0F,10)
typedef struct {
    uint8_t  dev_addr;                  //Адрес устройства
    uint8_t  function;                  //Функциональный код
    uint16_t addr_reg;                  //Адрес первого регистра HI/LO байт
    uint16_t cnt_reg;                   //Количество регистров HI/LO байт
    uint8_t  cnt_byte;                  //Количество байт данных регистров
    //далее идут данные и КС
 } MBUS_WRT_REGS;

#pragma pack( pop )

//*************************************************************************************************
// Локальные переменные
//*************************************************************************************************
char str[256];
bool debug = false;
static osTimerId TimerHandle;
osMessageQId ModBusQueueHandle;
static osThreadId TaskModBusHandle, TaskTest;
static osMutexId ModBusMutexHandle;
static osSemaphoreId AnswerSemaphoreHandle;

static uint8_t recv_byte, recv_ind, pack_len;
static uint8_t send_buff[256], recv_buff[256];

//*************************************************************************************************
// Локальные прототипы функций
//*************************************************************************************************
static uint8_t CreateFrame( MBUS_REQUEST *reqst );
static uint8_t AnswerData( uint8_t *data, uint8_t pack_len );
static int AnswerDescr( ModBusError error, char *ptr );
static void BufferClear( void );
static void ModBusRecv( uint8_t byte );
static void CallbackTimer( void const * argument );
static void StartTaskModBus( void const * argument );
void RS485SendData( uint8_t *data, uint8_t data_len );
static void StartTaskTest( void const * argument );

//*************************************************************************************************
// Инициализация модуля обмена по протоколу MODBUS
//*************************************************************************************************
void ModBusInit( void ) {

    BufferClear();
    //таймер ожидания ответа
    osTimerDef( TimerModBus, CallbackTimer );
    TimerHandle = osTimerCreate( osTimer( TimerModBus ), osTimerOnce, NULL );
    //мьютех блокировки порта RS485
    osMutexDef( ModBusMutex );
    ModBusMutexHandle = osMutexCreate( osMutex( ModBusMutex ) );
    //семафор ожидания ответа
    osSemaphoreDef( AnswerSemaphore );
    AnswerSemaphoreHandle = osSemaphoreCreate( osSemaphore( AnswerSemaphore ), 1 );
    //очередь сообщений
    osMessageQDef( ModBusQueue, 8, uint16_t );
    ModBusQueueHandle = osMessageCreate( osMessageQ( ModBusQueue ), NULL );
    //задача управления обменом по MODBUS
    osThreadDef( TaskModBus, StartTaskModBus, osPriorityNormal, 0, 512 );
    TaskModBusHandle = osThreadCreate( osThread( TaskModBus ), NULL );
    //задача тестового обмена данными с уст-ми
    osThreadDef( TaskTest, StartTaskTest, osPriorityNormal, 0, 384 );
    TaskTest = osThreadCreate( osThread( TaskTest ), NULL );
}

//*************************************************************************************************
// Задача управления обменом по протоколу MODBUS
//*************************************************************************************************
static void StartTaskModBus( void const * argument ) {

    osEvent event;
    uint8_t len_pack, func;

    for ( ;; ) {
        event = osMessageGet( ModBusQueueHandle, osWaitForever );
        if ( event.status == osEventMessage ) {
            //отправка сообщения
            if ( event.value.signals & EVENT_MODBUS_SEND ) {
                //передача сообщения
                BufferClear();
                len_pack = event.value.signals & ~EVENT_MODBUS_SEND;
                RS485SendData( send_buff, len_pack );
               }
            //проверка заголовка пакета
            if ( event.value.signals == EVENT_MODBUS_CHECK ) {
               if ( recv_buff[MB_ANSWER_FUNC] & FUNC_ANSWER_ERROR )
                   pack_len = SIZE_PACK_ERROR; //размер пакета с ошибкой
               else {
                   //расчет размера пакета
                   func = recv_buff[MB_ANSWER_FUNC];
                   if ( func == FUNC_RD_COIL_STAT || func == FUNC_RD_DISC_INP || func == FUNC_RD_HOLD_REG || func == FUNC_RD_INP_REG )
                       pack_len = recv_ind + recv_buff[IND_BYTE_CMD01_04] + SIZE_CRC;
                   if ( func == FUNC_WR_SING_COIL || func == FUNC_WR_SING_REG )
                       pack_len = sizeof( MBUS_WRT_REG );
                   if ( func == FUNC_WR_MULT_COIL || func == FUNC_WR_MULT_REG )
                       pack_len = SIZE_PACK_0F10;
                  }
              }
            //весь пакет получен
            if ( event.value.signals == EVENT_MODBUS_RECV )
                osSemaphoreRelease( AnswerSemaphoreHandle );
            //вышло время ожидания ответа
            if ( event.value.signals == EVENT_MODBUS_TIMEOUT ) {
                BufferClear();
                osSemaphoreRelease( AnswerSemaphoreHandle );
               }
          }
      }
}

//*************************************************************************************************
// Запрос данных по MODBUS
// MBUS_REQUEST *reqst  - указатель на структуры с параметрами запроса
// return = ModBusError - результат выполнения запроса
//*************************************************************************************************
ModBusError ModBusRequest( MBUS_REQUEST *reqst ) {

    uint16_t len_pack, len_data = 0, data_ind;
    ModBusError status = MBUS_ERROR_PARAM;

    if ( reqst->ptr_data == NULL || reqst->ptr_lendata == NULL || !reqst->function )
        return MBUS_ERROR_PARAM;
    //устанавливаем блокировку/ожидаем снятие блокировки
    osMutexWait( ModBusMutexHandle, osWaitForever );
    //формируем пакет для передачи
    len_pack = CreateFrame( reqst );
    if ( len_pack ) {
        //установим семафор ожидания ответа уст-ва
        osSemaphoreWait( AnswerSemaphoreHandle, 0 );
        //отправка запроса
        osMessagePut( ModBusQueueHandle, EVENT_MODBUS_SEND | len_pack, osWaitForever );
        //если семафор установлен, ждем ответа от устройства
        osSemaphoreWait( AnswerSemaphoreHandle, osWaitForever );
        status = AnswerData( recv_buff, recv_ind );
        if ( status == MBUS_ANSWER_OK ) {
            //только чтение регистров (01,02,03,04)
            if ( reqst->function == FUNC_RD_COIL_STAT || reqst->function == FUNC_RD_DISC_INP || reqst->function == FUNC_RD_HOLD_REG || reqst->function == FUNC_RD_INP_REG ) {
                data_ind = MB_ANSWER_DATA1;             //смещение начала данных
                len_data = recv_buff[MB_ANSWER_COUNT];  //размер данных
               }
            //запись одного регистра (05,06) 8/16-ми битная адресация, битов/регистров (0F,10)
            if ( reqst->function == FUNC_WR_SING_COIL || reqst->function == FUNC_WR_SING_REG || reqst->function == FUNC_WR_MULT_COIL || reqst->function == FUNC_WR_MULT_REG ) {
                /*if ( *reqst->ptr_lendata == sizeof( uint8_t ) )
                    *reqst->ptr_lendata = sizeof( uint16_t );*/
                data_ind = MB_ANSWER_DATA2;             //смещение начала данных
                len_data = 2;                           //размер данных
               }
            //скопируем принятые данные ответа в MBUS_REQUEST->ptr_data
            if ( len_data <= *reqst->ptr_lendata ) {
                //размер принятых данных меньше размера выделенной памяти, копируем все
                *reqst->ptr_lendata = len_data;
                memcpy( reqst->ptr_data, recv_buff + data_ind, len_data );
               }
            //размер принятых данных больше размера выделенной памяти, копируем только часть
            else memcpy( reqst->ptr_data, recv_buff + data_ind, *reqst->ptr_lendata );
           }
        else *reqst->ptr_lendata = 0;
       }
    //снимаем блокировку
    osMutexRelease( ModBusMutexHandle );
    return status;
 }

//*************************************************************************************************
// Формирование пакета протокола MODBUS
// MBUS_REQUEST *reqst - указатель на структуры с параметрами запроса
// return              - размер пакета в байтах для передачи
//*************************************************************************************************
static uint8_t CreateFrame( MBUS_REQUEST *reqst ) {

    uint16_t *src, *dst, value;
    uint8_t idx, data_len;
    MBUS_REQ_REG mbus_req;
    MBUS_WRT_REG mbus_reg;
    MBUS_WRT_REGS mbus_regs;

    BufferClear();
    //только чтение регистров (01,02,03,04)
    if ( reqst->function == FUNC_RD_COIL_STAT || reqst->function == FUNC_RD_DISC_INP || reqst->function == FUNC_RD_HOLD_REG || reqst->function == FUNC_RD_INP_REG || \
         reqst->function == FUNC_RD_EXCP_STAT || reqst->function == FUNC_RD_DIAGNOSTIC || reqst->function == FUNC_RD_EVENT_CNT || reqst->function == FUNC_WR_EVENT_LOG ||
         reqst->function == FUNC_RD_SLAVE_ID ) {
        mbus_req.dev_addr = reqst->dev_addr;
        mbus_req.function = reqst->function;
        //поменяем байты местами для переменных uint16_t, т.к. сначала передаем старший байт
        //в текущей модели LITTLE-ENDIAN младший байт хранится первым
        mbus_req.addr_reg = __REVSH( reqst->addr_reg );
        mbus_req.cnt_reg = __REVSH( reqst->cnt_reg );
        //расчет контрольной суммы данных по уже переставленным байтам
        //Контрольная сумма передается в фрейме младшим байтом вперед, 
        //т.е. в формате LSB|MSB, т.е. байты местами не меняем !!!
        mbus_req.crc = CalcCRC16( (uint8_t *)&mbus_req, sizeof( mbus_req ) - sizeof( uint16_t ) );
        memset( send_buff, 0x00, sizeof( send_buff ) );
        memcpy( send_buff, &mbus_req, sizeof( MBUS_REQ_REG ) );
        return sizeof( MBUS_REQ_REG );
       }
    //запись одного регистра (05, 06) 8/16-ми битная адресация
    if ( reqst->function == FUNC_WR_SING_COIL || reqst->function == FUNC_WR_SING_REG ) {
        mbus_reg.dev_addr = reqst->dev_addr;
        mbus_reg.function = reqst->function;
        //поменяем байты местами для переменных uint16_t, т.к. сначала передаем старший байт
        //в текущей модели LITTLE-ENDIAN младший байт хранится первым
        mbus_reg.addr_reg = __REVSH( reqst->addr_reg );
        mbus_reg.reg_val = __REVSH( *( (uint16_t *)reqst->ptr_data ) );
        //расчет контрольной суммы
        mbus_reg.crc = CalcCRC16( (uint8_t *)&mbus_reg, sizeof( mbus_reg ) - 2 );
        memset( send_buff, 0x00, sizeof( send_buff ) );
        memcpy( send_buff, &mbus_reg, sizeof( MBUS_WRT_REG ) );
        return sizeof( MBUS_WRT_REG );
       }
    //запись нескольких битов/регистров (0F,10)
    if ( reqst->function == FUNC_WR_MULT_COIL || reqst->function == FUNC_WR_MULT_REG ) {
        mbus_regs.dev_addr = reqst->dev_addr;
        mbus_regs.function = reqst->function;
        //поменяем байты местами для переменных uint16_t, т.к. сначала передаем старший байт
        //в текущей модели LITTLE-ENDIAN младший байт хранится первым
        mbus_regs.addr_reg = __REVSH( reqst->addr_reg );
        mbus_regs.cnt_reg = __REVSH( reqst->cnt_reg );
        data_len = sizeof( MBUS_WRT_REGS );
        if ( reqst->function == FUNC_WR_MULT_COIL )
            mbus_regs.cnt_byte = CALC_BYTE( reqst->cnt_reg );
        if ( reqst->function == FUNC_WR_MULT_REG )
            mbus_regs.cnt_byte = reqst->cnt_reg * sizeof( uint16_t );
        memset( send_buff, 0x00, sizeof( send_buff ) );
        //копируем в буфер заголовок пакета
        memcpy( send_buff, &mbus_regs, data_len );
        //копируем в буфер данные регистров
        if ( mbus_regs.function == FUNC_WR_MULT_COIL ) {
            //копируем как uint8_t
            memcpy( send_buff + data_len, (uint8_t *)reqst->ptr_data, mbus_regs.cnt_byte );
            data_len += mbus_regs.cnt_byte;
           }
        if ( mbus_regs.function == FUNC_WR_MULT_REG ) {
            //копируем как uint16_t
            src = (uint16_t *)reqst->ptr_data;
            dst = (uint16_t *)( send_buff + data_len );
            for ( idx = 0; idx < reqst->cnt_reg; idx++, src++, dst++ )
                *dst = __REVSH( *src );
            data_len += idx * sizeof( uint16_t );
           }
        //расчет, сохранение КС
        value = CalcCRC16( send_buff, data_len );
        memcpy( send_buff + data_len, (uint8_t *)&value, sizeof( uint16_t ) );
        return data_len + sizeof( uint16_t );
       }
    return 0;
 }
 
//*************************************************************************************************
// Обработка ответа, проверка всего пакета с ответом
// uint8_t *data        - адрес буфера с принятыми данными
// uint8_t len          - размер пакета (байт)
// return = ModBusError - результат проверки принятого пакета
//*************************************************************************************************
static ModBusError AnswerData( uint8_t *data, uint8_t pack_len ) {

    uint8_t func, len_data, idx, cnt_word;
    uint16_t crc_calc, crc_data, *ptr_uint16;

    if ( data == NULL || !pack_len )
        return MBUS_ANSWER_TIMEOUT;
    //проверим КС всего пакета
    crc_calc = CalcCRC16( data, pack_len - sizeof( uint16_t ) );
    crc_data = *((uint16_t*)( data + pack_len - sizeof( uint16_t ) ));
    if ( crc_calc != crc_data )
        return MBUS_ANSWER_CRC; //КС не совпали
    //проверим флаг ошибки в пакете
    if ( data[MB_ANSWER_FUNC] & FUNC_ANSWER_ERROR )
        return *( data + MB_ANSWER_ERROR );
    func = *( data + MB_ANSWER_FUNC );
    len_data = *( data + MB_ANSWER_COUNT );
    if ( func == FUNC_RD_HOLD_REG || func == FUNC_RD_INP_REG || func == FUNC_WR_SING_REG || func == FUNC_WR_MULT_REG  ) {
        //для функций с 16-битной адресацией меняем байты местами
        cnt_word = len_data / sizeof( uint16_t );
        ptr_uint16 = (uint16_t *)( data + MB_ANSWER_DATA1 );
        for ( idx = 0; idx < cnt_word; idx++, ptr_uint16++ )
            *ptr_uint16 = __REVSH( *ptr_uint16 );
       }
    return MBUS_ANSWER_OK;
 }

//*************************************************************************************************
// Вывод расшифровки результата выполнения запроса по протоколу MODBUS
// ModBusError error - код ошибки обработки принятого ответа от уст-ва
// char *ptr         - адрес размещения расшифровки кода ошибки
// return            - количеству символов, добавленных в массив.
//*************************************************************************************************
static int AnswerDescr( ModBusError error, char *ptr ) {

    return sprintf( ptr, "%s\r\n", modbus_error_descr[error] );
 }

//*************************************************************************************************
// Размещение принятого байта в приемном буфере и передача сообщения в задачу управления
// uint8_t byte - принятый байт
//*************************************************************************************************
static void ModBusRecv( uint8_t byte ) {

    if ( recv_ind >= sizeof( recv_buff ) )
        BufferClear();
    recv_buff[recv_ind++] = byte;
    if ( recv_ind == CNT_RECV_CHECK )
        osMessagePut( ModBusQueueHandle, EVENT_MODBUS_CHECK, 0 ); //предварительная проверка пакета
    if ( recv_ind == pack_len )
        osMessagePut( ModBusQueueHandle, EVENT_MODBUS_RECV, 0 );  //пакет получен полностью
}

//*************************************************************************************************
// Обнуляем приемный буфер и индексные переменные
//*************************************************************************************************
static void BufferClear( void ) {

    recv_ind = 0;
    pack_len = 0;
    memset( recv_buff, 0x00, sizeof( recv_buff ) );
}

//*************************************************************************************************
// Функция обратного вызова таймера ожидания ответа
//*************************************************************************************************
void CallbackTimer( void const * argument ) {

    osMessagePut( ModBusQueueHandle, EVENT_MODBUS_TIMEOUT, 0 );
}

//*************************************************************************************************
// Передача пакета данных
// uint8_t *data    - указатель на блок с данными для передачи
// uint8_t data_len - кол-во передаваемых байт
//*************************************************************************************************
void RS485SendData( uint8_t *data, uint8_t data_len ) {

    HAL_GPIO_WritePin( RS485_DIR1_GPIO_Port, RS485_DIR1_Pin, RS485_MODE_SEND );
    HAL_UART_Transmit_IT( &huart2, data, data_len );
}

//*************************************************************************************************
// Функция обратного вызова при приеме байта по UART2
//*************************************************************************************************
void RS485RecvComplt( void ) {

    ModBusRecv( recv_byte );
    HAL_UART_Receive_IT( &huart2, (uint8_t *)&recv_byte, sizeof( recv_byte ) );
}

//*************************************************************************************************
// Функция вызывается при завершении передачи пакета по UART2
// Переход в режим приема данных
//*************************************************************************************************
void RS485SendComplt( void ) {

    //переход в режим приема
    HAL_GPIO_WritePin( RS485_DIR1_GPIO_Port, RS485_DIR1_Pin, RS485_MODE_RECV );
    //запускаем прием информации
    HAL_UART_Receive_IT( &huart2, (uint8_t *)&recv_byte, sizeof( recv_byte ) );
    //таймер ожидания ответа
    osTimerStart( TimerHandle, TIMEOUT_ANSWER );
}

//*************************************************************************************************
// Тестовая задача
// Выполняется чтение состояния 8-ми входов, 8-ми выходов и
// циклическое управление 8-мю выходами внешнего модуля WP8028ADAM
//*************************************************************************************************
static void StartTaskTest( void const * argument ) {

    char *ptr;
    uint16_t *ptr16;
    ModBusError error;
    uint8_t i, len, cnt16, idxcmd, data[32];
    uint8_t idxwr = 0, wrt[] = { 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x55, 0xAA, 0xFF };
    MBUS_REQUEST reqst[] = {
        { 4,  FUNC_RD_HOLD_REG,  0x0000, 8, &data, &len }, //чтение состояния
        { 16, FUNC_RD_DISC_INP,  0x0000, 8, &data, &len }, //чтение входов
        { 16, FUNC_WR_MULT_COIL, 0x0000, 8, &data, &len }, //управление выходами
        { 16, FUNC_RD_COIL_STAT, 0x0000, 8, &data, &len }  //чтение выходов
    };

    for ( ;; ) {
        for ( idxcmd = 0; idxcmd < ( sizeof( reqst ) / sizeof( MBUS_REQUEST ) ); idxcmd++ ) {
            len = sizeof( data );
            if ( idxwr >= sizeof( wrt ) )
                idxwr = 0;
            if ( reqst[idxcmd].function == FUNC_WR_MULT_COIL )
                data[0] = wrt[idxwr++];
            if ( idxwr >= sizeof( wrt ) )
                idxwr = 0;
            ptr = str;
            ptr += sprintf( ptr, "IDX=%u DEV=0x%02X FUNC=0x%02X ", idxcmd, reqst[idxcmd].dev_addr, reqst[idxcmd].function );
            error = ModBusRequest( &reqst[idxcmd] );
            if ( error == MBUS_ANSWER_OK ) {
                ptr += sprintf( ptr, "RES=%u LEN=%u DATA=", error, *reqst[idxcmd].ptr_lendata );
                if ( reqst[idxcmd].function == FUNC_RD_HOLD_REG || reqst[idxcmd].function == FUNC_RD_INP_REG ) {
                    //вывод как uint16_t
                    ptr16 = (uint16_t *)reqst[idxcmd].ptr_data;
                    cnt16 = *reqst[idxcmd].ptr_lendata / sizeof( uint16_t );
                    for ( i = 0; i < cnt16; i++, ptr16++ )
                        ptr += sprintf( ptr, "0x%04X ", *ptr16 );
                   }
                else {
                    //вывод как uint8_t
                    for ( i = 0; i < *reqst[idxcmd].ptr_lendata; i++ )
                        ptr += sprintf( ptr, "0x%02X ", *( ((uint8_t *)reqst[idxcmd].ptr_data ) + i ) );
                   }
                ptr += sprintf( ptr, "\r\n" );
                if ( debug == true )
                    UARTSendStr( str );
               }
            else {
                AnswerDescr( error, ptr );
                if ( debug == true )
                    UARTSendStr( str );
               }
        }
        osDelay( 300 );
      }
}

//*************************************************************************************************
// Включение/выключение вывода отладочной информации обмена по MODBUS
//*************************************************************************************************
void ModBusDebug( void  ) {

    debug = !debug;
}

//*************************************************************************************************
// Тестовая функция
// Для ус-ва: 0x04 выполняется запись в регистр: 0x0000 значение: 0x0002
//*************************************************************************************************
void TestExec( void  ) {

    char *ptr;
    uint16_t *ptr16;
    ModBusError error;
    uint8_t i, len, data[10];
    MBUS_REQUEST reqst = { 4, FUNC_WR_MULT_REG, 0x0000, 1, &data, &len };

    ptr16 = (uint16_t *)data;
    *ptr16 = 0x0002;
    len = sizeof( data );
    ptr = str;
    ptr += sprintf( ptr, "DEV=0x%02X FUNC=0x%02X ", reqst.dev_addr, reqst.function );
    error = ModBusRequest( &reqst );
    if ( error == MBUS_ANSWER_OK ) {
        ptr += sprintf( ptr, "RES=%u LEN=%u DATA=", error, *reqst.ptr_lendata );
        for ( i = 0; i < *reqst.ptr_lendata; i++ )
            ptr += sprintf( ptr, "0x%02X ", *( ((uint8_t *)reqst.ptr_data ) + i ) );
        ptr += sprintf( ptr, "\r\n" );
        UARTSendStr( str );
       }
    else {
        AnswerDescr( error, ptr );
        UARTSendStr( str );
       }
}

