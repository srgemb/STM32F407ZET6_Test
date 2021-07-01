/*
 * hooks.c
 *
 *  Created on: 8 мар. 2020 г.
 *      Author: admin
 */

#include <stdio.h>
#include <string.h>

#include "main.h"
#include "cmsis_os.h"
#include "hooks.h"
#include "uart.h"

//*************************************************************************************************
// Внешние переменные
//*************************************************************************************************
extern UART_HandleTypeDef huart1;

//*************************************************************************************************
// Локальные переменные
//*************************************************************************************************
static char str[80];
static const char malloc_ovr[] = "Error: MallocFailed.\r\n";
static const char stack_ovr[]  = "Error: StackOverflow, task name: %s\r\n";

//*************************************************************************************************
// Обработка ошибки переполнения стека
//*************************************************************************************************
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName) {

    taskDISABLE_INTERRUPTS(); //блокировка прерываний
    sprintf( str, stack_ovr, (char *)pcTaskName );
    HAL_UART_Transmit( &huart1, (uint8_t *)str, strlen( str ), HAL_MAX_DELAY );
    for ( ;; );
}

//*************************************************************************************************
// Обработка ошибки выделения памяти
//*************************************************************************************************
void vApplicationMallocFailedHook( void ) {

    taskDISABLE_INTERRUPTS();
    sprintf( str, "%s", malloc_ovr );
    HAL_UART_Transmit( &huart1, (uint8_t *)str, strlen( str ), HAL_MAX_DELAY );
    for ( ;; );
 }
