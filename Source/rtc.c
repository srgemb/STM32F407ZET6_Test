/*
 * rtc.c
 *
 *  Created on: 23 мая 2021 г.
 *      Author: admin
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>

#include "main.h"
#include "uart.h"
#include "rtc.h"

//****************************************************************************************************************
// Внешние переменные
//****************************************************************************************************************
extern RTC_HandleTypeDef hrtc;

//****************************************************************************************************************
// Установка текущего значения времени
// Установка времени, входной формат HH:MI:SS
// char *time       - строка с временем в формате HH:MI:SS
//                    допустимые значения: HH:MI:SS 00-23:00-59:00-59
// return = SUCCESS - параметры указаны верно, время изменено
//          ERROR   - ошибка в формате или неправильное значение
//****************************************************************************************************************
ErrorStatus TimeSet( char *time ) {

    RTC_TimeTypeDef stime;
    uint8_t idx, chk = 0;
    char *mask = NULL, mask1[] = "NN:NN:NN", mask2[] = "NN:NN";

    memset( &stime, 0, sizeof( stime ) );
    if ( strlen( time ) == strlen( mask1 ) )
        mask = mask1;
    if ( strlen( time ) == strlen( mask2 ) )
        mask = mask2;
    if ( mask == NULL )
        return ERROR;
    //проверка формата
    for ( idx = 0; idx < strlen( mask ); idx++ ) {
        if ( mask[idx] == 'N' && isdigit( (int)*(time+idx) ) )
            chk++;
        if ( mask[idx] == ':' && ispunct( (int)*(time+idx) ) )
            chk++;
       }
    if ( chk == strlen( mask1 ) ) {
        //полный формат "NN:NN:NN"
        //проверка значений
        stime.Hours = atoi( time );
        stime.Minutes = atoi( time + 3 );
        stime.Seconds = atoi( time + 6 );
        if ( stime.Hours > 23 || stime.Minutes > 59 || stime.Seconds > 59 )
            return ERROR;
        if ( HAL_RTC_SetTime( &hrtc, &stime, RTC_FORMAT_BIN ) == HAL_OK )
            return SUCCESS;
        else return ERROR;
       }
    if ( chk == strlen( mask2 ) ) {
        //сокращенный формат "NN:NN"
        stime.Hours = atoi( time );
        stime.Minutes = atoi( time + 3 );
        stime.Seconds = 0;
        if ( stime.Hours > 23 || stime.Minutes > 59 )
            return ERROR;
        if ( HAL_RTC_SetTime( &hrtc, &stime, RTC_FORMAT_BIN ) == HAL_OK )
            return SUCCESS;
        else return ERROR;
       }
    return ERROR;
 }

//****************************************************************************************************************
// Установка текущего значения даты
// Установка даты, входной формат DD.MM.YY
// char *time       - строка с датой в формате DD.MM.YY
//                    допустимые значения: DD.MM.YY 01-31:01-31:00-99
// return = SUCCESS - параметры указаны верно, дата изменена
//          ERROR   - ошибка в формате или неправильное значение
//****************************************************************************************************************
ErrorStatus DateSet( char *date ) {

    RTC_DateTypeDef sdate;
    uint8_t idx, chk = 0;
    char mask[] = "NN.NN.NN";

    //проверка формата
    for ( idx = 0; idx < strlen( mask ); idx++ ) {
        if ( mask[idx] == 'N' && isdigit( (int)*(date+idx) ) )
            chk++;
        if ( mask[idx] == '.' && ispunct( (int)*(date+idx) ) )
            chk++;
       }
    if ( chk != strlen( mask ) )
        return ERROR; //не соответствие формату
    //проверка значений
    sdate.Date = atoi( date );
    sdate.Month = atoi( date + 3 );
    sdate.Year = atoi( date + 6 );
    sdate.WeekDay = DayOfWeek( sdate.Date, sdate.Month, sdate.Year );
    if ( sdate.Date < 1 || sdate.Date > 31 || sdate.Month < 1 || sdate.Month > 12 || sdate.Year > 99 )
        return ERROR; //недопустимые значения
    //проверка прошла
    if ( HAL_RTC_SetDate( &hrtc, &sdate, RTC_FORMAT_BIN ) == HAL_OK )
        return SUCCESS;
    else return ERROR;
}

//****************************************************************************************************************
// Вывод текущего значения дата/время
// DataTimeMask mask - маска вывода
//****************************************************************************************************************
void DateTime( DataTimeMask mask ) {

    char str[20];
    RTC_TimeTypeDef stime;
    RTC_DateTypeDef sdate;

    if ( mask & MASK_DATE ) {
        HAL_RTC_GetDate( &hrtc, &sdate, RTC_FORMAT_BIN );
        sprintf( str, "Date %02d.%02d.%02d ", sdate.Date, sdate.Month, sdate.Year );
        UARTSendStr( str );
    }
    if ( mask & MASK_TIME ) {
        HAL_RTC_GetTime( &hrtc, &stime, RTC_FORMAT_BIN );
        sprintf( str, "Time %02d:%02d:%02d ", stime.Hours, stime.Minutes, stime.Seconds );
        UARTSendStr( str );
    }
    if ( mask )
        UARTSendStr( "\r\n" );
}

//****************************************************************************************************************
// Вывод текущего значения дата/время в буфер
// char *buff        - указатель на буфер для размещения результата
// DataTimeMask mask - маска вывода
// return            - значение указателя на следующий свободный байт в буфере (на '\0')
//****************************************************************************************************************
char *DateTimeStr( char *buff, DataTimeMask mask ) {

    char *ptr = buff;
    RTC_TimeTypeDef stime;
    RTC_DateTypeDef sdate;

    if ( mask & MASK_DATE ) {
        HAL_RTC_GetDate( &hrtc, &sdate, RTC_FORMAT_BIN );
        ptr += sprintf( ptr, "Date %02d.%02d.%02d ", sdate.Date, sdate.Month, sdate.Year );
    }
    if ( mask & MASK_TIME ) {
        HAL_RTC_GetTime( &hrtc, &stime, RTC_FORMAT_BIN );
        ptr += sprintf( ptr, "Time %02d:%02d:%02d ", stime.Hours, stime.Minutes, stime.Seconds );
    }
    return ptr;
}

//*********************************************************************************************
// Расчет дня недели по дате
// Все деления целочисленные (остаток отбрасывается).
// Результат: ( 0 — воскресенье )-> 7, 1 — понедельник и т.д.
//*********************************************************************************************
uint8_t DayOfWeek( int day, int month, int year ) {

    int a, y, m, w;

    a = ( 14 - month ) / 12;
    y = ( 2000 + year ) - a;
    m = month + 12 * a - 2;
    w = (7000 + (day + y + y / 4 - y / 100 + y / 400 + (31 * m) / 12)) % 7;
    if ( w )
        return w;
    else return 7;
}

