/*
 * uart_cb.c
 *
 *  Created on: 2 июн. 2021 г.
 *      Author: admin
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "main.h"
#include "cmsis_os.h"
#include "stm32f4xx.h"
#include "uart.h"
#include "modbus.h"

//*************************************************************************************************
// Внешние переменные
//*************************************************************************************************
extern UART_HandleTypeDef huart1, huart2, huart3;

//*************************************************************************************************
// CallBack функция, вызывается при завершении передачи из UART (stm32f4xx_it.c)
//*************************************************************************************************
void HAL_UART_TxCpltCallback( UART_HandleTypeDef *huart ) {

    if ( huart == &huart1 )
        UARTSendComplt();
    if ( huart == &huart2 )
        RS485SendComplt();
 }

//*************************************************************************************************
// CallBack функция приеме байта по UART (stm32f4xx_it.c)
//*************************************************************************************************
void HAL_UART_RxCpltCallback( UART_HandleTypeDef *huart ) {

    if ( huart == &huart1 )
        UARTRecvComplt();
    if ( huart == &huart2 )
        RS485RecvComplt();
}

