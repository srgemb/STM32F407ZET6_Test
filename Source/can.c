/*
 * can.c
 *
 *  Created on: 8 февраля. 2021 г.
 *      Author: admin
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "cmsis_os.h"

#include "main.h"
#include "can.h"
#include "uart.h"
#include "event.h"
#include "host.h"

#define CAN1_DEVICE_ID      1
#define CAN2_DEVICE_ID      2

//*************************************************************************************************
// Внешние переменные
//*************************************************************************************************
extern CAN_HandleTypeDef hcan1, hcan2;

//*************************************************************************************************
// Локальные переменные
//*************************************************************************************************
static char str[90];
static uint8_t data1 = 0, data2 = 255;
static uint8_t can_data1[8] = { 0, };
static uint8_t can_data2[8] = { 0, };

//APB1 = 40 MHz, Sample-Point at: 87.5%
static uint32_t can_cfg[][4] = {
    //Speed, Prescaler, TimeSeg1,       TimeSeg2
    { 10,    250,       CAN_BS1_13TQ,   CAN_BS2_2TQ },      //10 kbit/s
    { 20,    125,       CAN_BS1_13TQ,   CAN_BS2_2TQ },      //20 kbit/s
    { 50,    50,        CAN_BS1_13TQ,   CAN_BS2_2TQ },      //50 kbit/s
    { 125,   20,        CAN_BS1_13TQ,   CAN_BS2_2TQ },      //125 kbit/s
    { 250,   10,        CAN_BS1_13TQ,   CAN_BS2_2TQ },      //250 kbit/s
    { 500,   5,         CAN_BS1_13TQ,   CAN_BS2_2TQ }       //500 kbit/s
 };

static osMailQId CANMailHandle;
static osThreadId TaskCAN1Handle, TaskCAN2Handle;
static osMessageQId CANQueue1Handle;

static char * const can_error[] = {
    "Protocol Error Warning",
    "Error Passive",
    "Bus-off error",
    "Stuff error",
    "Form error",
    "Acknowledgment error",
    "Bit recessive error",
    "Bit dominant error",
    "CRC error",
    "Rx FIFO0 overrun error",
    "Rx FIFO1 overrun error",
    "TxMailbox 0 transmit failure due to arbitration lost",
    "TxMailbox 1 transmit failure due to transmit error",
    "TxMailbox 0 transmit failure due to arbitration lost",
    "TxMailbox 1 transmit failure due to transmit error",
    "TxMailbox 0 transmit failure due to arbitration lost",
    "TxMailbox 1 transmit failure due to transmit error",
    "Timeout error",
    "Peripheral not initialized",
    "Peripheral not ready",
    "Peripheral not started",
    "Parameter error"
 };

//*************************************************************************************************
// Прототипы локальных функций
//*************************************************************************************************
static void StartTaskCAN1( void const * argument );
static void StartTaskCAN2( void const * argument );

//*************************************************************************************************
// Инициализация фильтров, очередей, задач CAN интерфейса
//*************************************************************************************************
void CANInit( void ) {
    
    CAN_FilterTypeDef canFilterConfig1, canFilterConfig2;

    //создаем очередь передачи пакета
    osMessageQDef( CANQueue1, 8, uint16_t );
    CANQueue1Handle = osMessageCreate( osMessageQ( CANQueue1 ), NULL );

    //создаем задачу передачи пакета
    osThreadDef( TaskCAN1, StartTaskCAN1, osPriorityNormal, 0, 256 );
    TaskCAN1Handle = osThreadCreate( osThread( TaskCAN1 ), NULL );

    //создаем задачу приема пакета
    osThreadDef( TaskCAN2, StartTaskCAN2, osPriorityNormal, 0, 256 );
    TaskCAN2Handle = osThreadCreate( osThread( TaskCAN2 ), NULL );

    //создаем очередь данных
    osMailQDef( CANMail, 8, CAN_DATA );
    CANMailHandle = osMailCreate( osMailQ( CANMail ), TaskCAN2Handle );
    //параметры фильтрации для CAN1
    canFilterConfig1.FilterBank = 0;
    canFilterConfig1.FilterMode = CAN_FILTERMODE_IDMASK;
    canFilterConfig1.FilterScale = CAN_FILTERSCALE_32BIT;
    canFilterConfig1.FilterIdHigh = 0x0000;
    canFilterConfig1.FilterIdLow = 0x0000;
    canFilterConfig1.FilterMaskIdHigh = 0x0000;
    canFilterConfig1.FilterMaskIdLow = 0x0000;
    canFilterConfig1.FilterFIFOAssignment = CAN_RX_FIFO0;
    canFilterConfig1.FilterActivation = ENABLE;
    canFilterConfig1.SlaveStartFilterBank = 14;
    //
    HAL_CAN_ConfigFilter( &hcan1, &canFilterConfig1 );
    HAL_CAN_Start( &hcan1 );
    HAL_CAN_ActivateNotification( &hcan1, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_ERROR );
    //параметры фильтрации для CAN2
    canFilterConfig2.FilterBank = 14;
    canFilterConfig2.FilterMode = CAN_FILTERMODE_IDMASK;
    canFilterConfig2.FilterScale = CAN_FILTERSCALE_32BIT;
    canFilterConfig2.FilterIdHigh = 0x0000;
    canFilterConfig2.FilterIdLow = 0x0000;
    canFilterConfig2.FilterMaskIdHigh = 0x0000;
    canFilterConfig2.FilterMaskIdLow = 0x0000;
    canFilterConfig2.FilterFIFOAssignment = CAN_RX_FIFO1;
    canFilterConfig2.FilterActivation = ENABLE;
    canFilterConfig2.SlaveStartFilterBank = 14;
    //
    HAL_CAN_ConfigFilter( &hcan2, &canFilterConfig2 );
    HAL_CAN_Start( &hcan2 );
    HAL_CAN_ActivateNotification( &hcan2, CAN_IT_RX_FIFO1_MSG_PENDING | CAN_IT_ERROR );
}

//*************************************************************************************************
// Выполнение команд для CAN интерфейсов
// uint16_t cmnd - маска команды, см. EVENT_CAN*
//*************************************************************************************************
void CANCommand( uint16_t cmnd ) {

    osMessagePut( CANQueue1Handle, cmnd, 0 );
}

//*************************************************************************************************
// Задача обработки сообщений CANQueue1
//*************************************************************************************************
static void StartTaskCAN1( void const * argument ) {

    uint8_t i;
    osEvent event;
    uint32_t mailBoxNum1 = 0, mailBoxNum2 = 0;
    CAN_TxHeaderTypeDef msgHeader1, msgHeader2;

    for ( ;; ) {
        event = osMessageGet( CANQueue1Handle, osWaitForever );
        if ( event.status == osEventMessage ) {
            //изменение данных
            if ( event.value.signals & EVENT_CAN1_UPD ) {
                //изменяем передаваеммые данные для CAN1
                for ( i = 0; i < 8; i++ )
                    can_data1[i] = data1++;
               }
            if ( event.value.signals & EVENT_CAN2_UPD ) {
                //изменяем передаваеммые данные для CAN2
                for ( i = 0; i < 8; i++ )
                    can_data2[i] = data2--;
               }
            //передача данных
            if ( event.value.signals & EVENT_CAN1_SEND ) {
                if ( HAL_CAN_GetTxMailboxesFreeLevel( &hcan1 ) != 0 ) {
                    //отправка данных CAN1
                    msgHeader1.StdId = CAN1_DEVICE_ID;
                    msgHeader1.DLC = 8;				//размер блока данных
                    msgHeader1.TransmitGlobalTime = DISABLE;
                    msgHeader1.RTR = CAN_RTR_DATA;	//фрейм данных
                    msgHeader1.IDE = CAN_ID_STD;	//идентификатор 11 бит
                    HAL_CAN_AddTxMessage( &hcan1, &msgHeader1, can_data1, &mailBoxNum1 );
                  }
               }
            if ( event.value.signals & EVENT_CAN2_SEND ) {
                if ( HAL_CAN_GetTxMailboxesFreeLevel( &hcan2 ) != 0 ) {
                    //отправка данных CAN2
                    msgHeader2.StdId = CAN2_DEVICE_ID;
                    msgHeader2.DLC = 8;				//размер блока данных
                    msgHeader2.TransmitGlobalTime = DISABLE;
                    msgHeader2.RTR = CAN_RTR_DATA;	//фрейм данных
                    msgHeader2.IDE = CAN_ID_STD;	//идентификатор 11 бит
                    HAL_CAN_AddTxMessage( &hcan2, &msgHeader2, can_data2, &mailBoxNum2 );
                  }
               }
           }
      }
}

//*************************************************************************************************
// Задача обработки принятых сообщений через CANMail
//*************************************************************************************************
static void StartTaskCAN2( void const * argument ) {

    char *ptr;
    uint8_t i;
    osEvent event;
    CAN_DATA *can_ptr;

    for ( ;; ) {
        event = osMailGet( CANMailHandle, osWaitForever );
        if ( event.status == osEventMail ) {
            //обработка принятого пакета
            can_ptr = (CAN_DATA *)event.value.p;
            ptr = str;
            ptr += sprintf( ptr, "CAN%u ID: 0x%03X DATA: ", can_ptr->can_numb, (uint16_t)can_ptr->msg_id );
            for ( i = 0; i < can_ptr->data_len; i++ )
                ptr += sprintf( ptr, "0x%02X ", can_ptr->data[i] );
            osMailFree( CANMailHandle, (CAN_DATA *)can_ptr );
            ptr += sprintf( ptr, "\r\n" );
            //по состоянию пина определяем время реакции от
            //момента получения пакета по шине CAN до этого места
            //HAL_GPIO_TogglePin( LED1_GPIO_Port, LED1_Pin );
            UARTSendStr( str );
           }
      }
}

//*************************************************************************************************
// Обработка прерывания - данные приняты по CAN1
//*************************************************************************************************
void HAL_CAN_RxFifo0MsgPendingCallback( CAN_HandleTypeDef *hcan ) {

    CAN_DATA *can_data;
    uint8_t msg_data[8];
    CAN_RxHeaderTypeDef msgHeader;

    //проверим очередь сообщений FIFO0, прочитаем сообщение
    if ( HAL_CAN_GetRxMessage( hcan, CAN_RX_FIFO0, &msgHeader, msg_data ) == HAL_OK ) {
        can_data = (CAN_DATA *)osMailAlloc( CANMailHandle, osWaitForever );
        can_data->can_numb = CAN1_DEVICE_ID;
        if ( msgHeader.IDE == CAN_ID_EXT )
            can_data->msg_id = msgHeader.ExtId;
        else can_data->msg_id = msgHeader.StdId;
        can_data->data_len = msgHeader.DLC;
        memcpy( can_data->data, msg_data, sizeof( msg_data ) );
        osMailPut( CANMailHandle, (CAN_DATA *)can_data );
        //CANExecCmnd( &can_data ); //передача данных по назначению
    }
 }

//*************************************************************************************************
// Обработка прерывания - данные приняты по CAN2
//*************************************************************************************************
void HAL_CAN_RxFifo1MsgPendingCallback( CAN_HandleTypeDef *hcan ) {

    CAN_DATA *can_data;
    uint8_t msg_data[8];
    CAN_RxHeaderTypeDef msgHeader;

    //проверим очередь сообщений FIFO1, прочитаем сообщение
    if ( HAL_CAN_GetRxMessage( hcan, CAN_RX_FIFO1, &msgHeader, msg_data ) == HAL_OK ) {
        can_data = (CAN_DATA *)osMailAlloc( CANMailHandle, osWaitForever );
        can_data->can_numb = CAN2_DEVICE_ID;
        if ( msgHeader.IDE == CAN_ID_EXT )
            can_data->msg_id = msgHeader.ExtId;
        else can_data->msg_id = msgHeader.StdId;
        can_data->data_len = msgHeader.DLC;
        memcpy( can_data->data, msg_data, sizeof( msg_data ) );
        osMailPut( CANMailHandle, (CAN_DATA *)can_data );
        //CANExecCmnd( &can_data ); //передача данных по назначению
    }
 }

//*************************************************************************************************
// Обработка ошибок CAN интерфейса
//*************************************************************************************************
void HAL_CAN_ErrorCallback( CAN_HandleTypeDef *hcan ) {

    char *ptr;
    uint8_t index, can = 0;
    uint32_t mask = 0x00000001;

    if ( !hcan->ErrorCode )
        return;
    if ( hcan == &hcan1 )
        can = 1;
    if ( hcan == &hcan2 )
        can = 2;
    ptr = str;
    ptr += sprintf( ptr, "CAN%u Error Code: 0x%08X ", can, (unsigned int)hcan->ErrorCode );
    for ( index = 0; index < 6; index++, mask <<= 1 ) {
        if ( hcan->ErrorCode & mask )
            ptr += sprintf( ptr, "%s ", can_error[index] ); 
       }
    ptr += sprintf( ptr, "\r\n" );
    UARTSendStr( str );
}

//*************************************************************************************************
// Возвращает значения параметров Prescaler, TimeSeg1, TimeSeg2 для CAN интерфейса
// uint8_t speed    - значение скорости (10,20,50,125,250,500)
// uint8_t id_param - ID параметра см. CAN_PARAM
// return           - значение параметра для MX_CAN_Init
//*************************************************************************************************
uint32_t CANGetParam( uint16_t speed, CAN_PARAM id_param ) {

    uint8_t idx, dims;

    dims = sizeof( can_cfg ) / sizeof( can_cfg[0] );
    for ( idx = 0; idx < dims; idx++ ) {
        if ( can_cfg[idx][CAN_SPEED] == speed )
            return can_cfg[idx][id_param];
       }
    //по умолчанию скорость 125 kbit/s
    return can_cfg[3][id_param];
}

