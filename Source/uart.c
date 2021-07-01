/*
 * uart.c
 *
 *  Created on: 27 янв. 2021 г.
 *      Author: admin
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "main.h"
#include "cmsis_os.h"
#include "stm32f4xx.h"
#include "event.h"
#include "host.h"
#include "can.h"

#define USE_UART_QUEUE                      //использовать буфер передачи

#define QUEUE_SIZE          1024            //размер буфера очереди
#define QUEUE_MAX_INDEX     QUEUE_SIZE - 1  //максимальное значение индекса

typedef struct {
    uint16_t iwrite;                        //индекс первого свободного элемента для записи в очередь (0...N)
    uint16_t iread;                         //индекс первого элемента очереди для чтения (0...N)
    uint16_t length;                        //размер данных в очереди (в байтах)
    uint16_t add;                           //кол-во байт добавленных в очередь во время вывода предыдущего блока
    uint8_t buffer[QUEUE_SIZE];             //данные очереди
 } QUEUE_UART;

//*************************************************************************************************
// Внешние переменные
//*************************************************************************************************
extern UART_HandleTypeDef huart1;

//*************************************************************************************************
// Локальные переменные
//*************************************************************************************************
static QUEUE_UART queue_uart;
static char recv, buffer[80];
static uint8_t recv_ind = 0;
static osThreadId TaskUARTHandle;
static osMessageQId UARTQueueHandle;
static osSemaphoreId QueueSemaphoreHandle;

//*************************************************************************************************
// Прототипы локальных функций
//*************************************************************************************************
#ifdef USE_UART_QUEUE
static void QueueClear( void );
static uint16_t QueueGetFree( void );
#endif
static void StartTaskUART( void const * argument );

//*************************************************************************************************
// Инициализация очереди, задачи для UART1
//*************************************************************************************************
void UARTInit( void ) {

    QueueClear();
    osSemaphoreDef( QueueSemaphore );
    QueueSemaphoreHandle = osSemaphoreCreate( osSemaphore( QueueSemaphore ), 1 );
    //создаем очереди обработки сообщений
    osMessageQDef( UARTQueue, 4, uint16_t );
    UARTQueueHandle = osMessageCreate( osMessageQ( UARTQueue ), NULL );
    //создаем задачу обработки команд
    osThreadDef( TaskUART, StartTaskUART, osPriorityNormal, 0, 384 );
    TaskUARTHandle = osThreadCreate( osThread( TaskUART ), NULL );
    //инициализация приема по UART
    memset( buffer, 0x00, sizeof( buffer ) );
    HAL_UART_Receive_IT( &huart1, (uint8_t *)&recv, sizeof( recv ) );
}

//*************************************************************************************************
// Задача обработки очереди сообщений приема данных по UART1.
//*************************************************************************************************
static void StartTaskUART( void const * argument ) {

    osEvent event;

    for ( ;; ) {
        event = osMessageGet( UARTQueueHandle, osWaitForever );
        if ( event.status == osEventMessage ) {
            if ( event.value.signals == EVENT_UART_RECV ) {
                ExecCommand( buffer ); //выполнение команды
                //команда выполнена, запускаем прием по UART1
                recv_ind = 0;
                memset( buffer, 0x00, sizeof( buffer ) );
                HAL_UART_Receive_IT( &huart1, (uint8_t *)&recv, sizeof( recv ) );
              }
           }
       }
 }

//*************************************************************************************************
// Добавляем строку в кольцевой буфер и запускаем передачу в UART
// char *str - указатель на строку для добавления
//*************************************************************************************************
void UARTSendStr( char *str ) {

    #ifdef USE_UART_QUEUE
    uint16_t ind_str = 0, buff_free, last_len;
    #endif

    if ( str == NULL || !strlen( str ) )
        return;
    #ifndef USE_UART_QUEUE
    HAL_UART_Transmit( &huart1, (uint8_t *)str, strlen( str ), HAL_MAX_DELAY );
    return;
    #else
    last_len = strlen( str );
    while ( true ) {
        buff_free = QueueGetFree();
        if ( !buff_free ) {
            //ждем пока освободится семафор очереди
            osSemaphoreWait( QueueSemaphoreHandle, osWaitForever );
            continue;
           }
        if ( last_len > buff_free ) {
            //вся строка не помещается в очередь, записывать будем по частям
            last_len -= buff_free;
            if ( !queue_uart.length )
                queue_uart.length = buff_free;
            memcpy( (char *)queue_uart.buffer + queue_uart.iwrite, str + ind_str, queue_uart.length );
            queue_uart.iwrite += buff_free;
            QueueGetFree(); //проверка места, установка семафора
            if ( huart1.gState == HAL_UART_STATE_BUSY_TX )
                queue_uart.add += buff_free;
            else HAL_UART_Transmit_IT( &huart1, &queue_uart.buffer[queue_uart.iread], queue_uart.length );
            ind_str += buff_free;
            continue;
           }
        else {
            //строка вся помещается в буфер
            if ( !queue_uart.length )
                queue_uart.length = last_len;
            memcpy( (char *)queue_uart.buffer + queue_uart.iwrite, str + ind_str, queue_uart.length );
            queue_uart.iwrite += last_len;
            if ( huart1.gState == HAL_UART_STATE_BUSY_TX )
                queue_uart.add += last_len;
            else HAL_UART_Transmit_IT( &huart1, &queue_uart.buffer[queue_uart.iread], queue_uart.length );
            break;
           }
       }
    #endif
 }

//*************************************************************************************************
// Функция вызывается при приеме байта по UART
//*************************************************************************************************
void UARTRecvComplt( void ) {

    if ( recv_ind >= sizeof( buffer ) ) {
        recv_ind = 0;
        memset( buffer, 0x00, sizeof( buffer ) );
       }
    buffer[recv_ind++] = recv;
    //проверим последний принятый байт, если CR - обработка команды
    if ( buffer[recv_ind - 1] == '\r' ) {
        buffer[recv_ind - 1] = '\0'; //уберем код CR
        osMessagePut( UARTQueueHandle, EVENT_UART_RECV, 0 );
        return; //не выполняем запуск приема по UART1
       }
    HAL_UART_Receive_IT( &huart1, (uint8_t *)&recv, sizeof( recv ) );
}

//*************************************************************************************************
// Функция вызывается при завершении передачи из UART1
//*************************************************************************************************
void UARTSendComplt( void ) {

    queue_uart.iread += queue_uart.length;
    if ( !queue_uart.add && queue_uart.iwrite == queue_uart.iread ) {
        //все передано
        QueueClear();
        osSemaphoreRelease( QueueSemaphoreHandle );
       }
    else {
        //передача следующего фрагмента из очереди
        queue_uart.length = queue_uart.iwrite - queue_uart.iread;
        HAL_UART_Transmit_IT( &huart1, &queue_uart.buffer[queue_uart.iread], queue_uart.length );
        if ( queue_uart.add )
            queue_uart.add -= queue_uart.length;
       }
 }

//*************************************************************************************************
// Сброс параметров очереди сообщений
//*************************************************************************************************
static void QueueClear( void ) {

    queue_uart.add = 0;
    queue_uart.iwrite = 0;
    queue_uart.iread = 0;
    queue_uart.length = 0;
    memset( queue_uart.buffer, 0x00, sizeof( queue_uart.buffer ) );
}

//****************************************************************************************************************
// Возвращает размер свободного места в очереди, при отсутствии места установливается семафора
// return - размер в байтах
//****************************************************************************************************************
static uint16_t QueueGetFree( void ) {

    uint16_t sfree;

    sfree = QUEUE_MAX_INDEX - queue_uart.iwrite;
    if ( !sfree )
        osSemaphoreWait( QueueSemaphoreHandle, 0 ); //буфер занят, зарядим семафор
    return sfree;
 }
