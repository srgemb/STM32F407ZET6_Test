/*
 * event.h
 *
 *  Created on: 30 янв. 2021 г.
 *      Author: admin
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "cmsis_os.h"
#include "main.h"

#include "event.h"
#include "host.h"
#include "rtc.h"
#include "sdcard.h"

//*************************************************************************************************
// Локальные переменные
//*************************************************************************************************
static osThreadId TaskEventHandle;
static osMessageQId QueueEventHandle;
static char * const key_name[] = {
    "S1",
    "S2",
    "S3",
    "S4"
};

//*************************************************************************************************
// Прототипы локальных функций
//*************************************************************************************************
static void EventWork( void const * argument );

//*************************************************************************************************
// Инициализация очередей и задачи обработки нажатия кнопок
//*************************************************************************************************
void EventInit( void ) {

    //создаем очередь обработки команд
    osMessageQDef(QueueEvent, 16, uint16_t);
    QueueEventHandle = osMessageCreate(osMessageQ(QueueEvent), NULL);
    //создаем задачу обработки команд
    osThreadDef( TaskEvent, EventWork, osPriorityNormal, 0, 512 );
    TaskEventHandle = osThreadCreate( osThread( TaskEvent ), NULL );
}

//*************************************************************************************************
// Обработка нажатий клавиш и передача в очередь
// uint16_t GPIO_Pin - ID из stm32f4xx_hal_gpio.h
//*************************************************************************************************
void HAL_GPIO_EXTI_Callback( uint16_t GPIO_Pin ) {

    if ( GPIO_Pin == GPIO_PIN_12 )
        osMessagePut( QueueEventHandle, EVENT_KEY_PRESSED1, 0 );
    if ( GPIO_Pin == GPIO_PIN_13 )
        osMessagePut( QueueEventHandle, EVENT_KEY_PRESSED2, 0 );
    if ( GPIO_Pin == GPIO_PIN_14 )
        osMessagePut( QueueEventHandle, EVENT_KEY_PRESSED3, 0 );
    if ( GPIO_Pin == GPIO_PIN_15 )
        osMessagePut( QueueEventHandle, EVENT_KEY_PRESSED4, 0 );
}

//*************************************************************************************************
// Обработка очереди сообщений
// Для работы с запись результата файл нужен стек 512, без записи достаточно стека 256
//*************************************************************************************************
static void EventWork( void const * argument ) {

    osEvent event;
    char *ptr, *key, str[40], name[] = "key.log";

    for ( ;; ) {
        event = osMessageGet( QueueEventHandle, osWaitForever );
        if ( event.status == osEventMessage ) {
            if ( event.value.signals == EVENT_KEY_PRESSED1 )
                HAL_GPIO_TogglePin( LED1_GPIO_Port, LED1_Pin );
            if ( event.value.signals == EVENT_KEY_PRESSED2 )
                HAL_GPIO_TogglePin( LED2_GPIO_Port, LED2_Pin );
            if ( event.value.signals == EVENT_KEY_PRESSED3 )
                HAL_GPIO_TogglePin( LED3_GPIO_Port, LED3_Pin );
            if ( event.value.signals == EVENT_KEY_PRESSED4 )
                HAL_GPIO_TogglePin( LED4_GPIO_Port, LED4_Pin );
            //формируем имя нажатой кнопки
            ptr = DateTimeStr( str, MASK_DATE | MASK_TIME );
            if ( event.value.signals == EVENT_KEY_PRESSED1 )
                key = key_name[0];
            if ( event.value.signals == EVENT_KEY_PRESSED2 )
                key = key_name[1];
            if ( event.value.signals == EVENT_KEY_PRESSED3 )
                key = key_name[2];
            if ( event.value.signals == EVENT_KEY_PRESSED4 )
                key = key_name[3];
            ptr += sprintf( ptr, "%s\r\n", key );
            FileAddStr( name, str );
           }
       }
}
