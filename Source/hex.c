/*
 * hex.c
 *
 *  Created on: 18 мая 2021 г.
 *      Author: admin
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

#include "stm32f4xx.h"

#include "uart.h"

//*************************************************************************************************
// Внешние переменные
//*************************************************************************************************
extern UART_HandleTypeDef huart1;

//*************************************************************************************************
// Вывод дампа данных 16*16 в HEX формате из FLASH памяти
// 0x00000000: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
// uint32_t *addr    - адрес FLASH памяти
// char *buff        - буфер для размещения результирующей строки
// uint16_t buff_len - размер буфера для размещения результирующей строки
//                     минимальный размер буфера 90 байт - вывод в UART по одной строке
//                     при размере буфера > 1300 байт - вывод в UART не выполняется
//                     вывод должен осуществляться из функции вызвавшей FlashHexDump()
//*************************************************************************************************
void FlashHexDump( uint32_t *addr, char *buff, uint16_t buff_len ) {

    char *ptr;
    uint8_t idx, line, *byte;
    uint32_t data[4], *dst_addr, *src_addr;

    if ( buff_len < 90 )
        return;
    ptr = buff;
    src_addr = addr;
    for ( line = 0; line < 16; line++ ) {
        //вывод адреса строки
        ptr += sprintf( ptr, "0x%08X: ", (unsigned int)src_addr );
        //читаем значение только как WORD !!! по 4 байта
        dst_addr = data;
        for ( idx = 0; idx < ( sizeof( data ) / sizeof( uint32_t ) ); idx++, src_addr++, dst_addr++ )
            *dst_addr = *src_addr;
        //вывод данных
        byte = (uint8_t *)&data;
        for ( idx = 0; idx < 16; idx++, byte++ ) {
            //выводим коды
            if ( ( idx & 0x07 ) == 7 )
                ptr += sprintf( ptr, "%02X  ", *byte );
            else ptr += sprintf( ptr, "%02X ", *byte );
           }
        //выводим символы
        byte = (uint8_t *)&data;
        for ( idx = 0; idx < 16; idx++, byte++ ) {
            if ( *byte >= 32 && *byte < 255 )
                ptr += sprintf( ptr, "%c", *byte );
            else ptr += sprintf( ptr, "." );
           }
        ptr += sprintf( ptr, "\r\n" );
        if ( line == 15 )
            ptr += sprintf( ptr, "\r\n" ); //отделим блок 16*16
        if ( buff_len < 1300 ) {
            //размер буфера не большой, вывод по одной строке
            UARTSendStr( buff );
            ptr = buff;
           }
      }
    //размер буфера позволяет вывести сразу весь блок 16*16
    if ( buff_len >= 1300 )
        UARTSendStr( buff );
 }

//*************************************************************************************************
// Вывод дампа данных 16*16 в HEX формате
// 0x0000: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
// uint8_t *addr    - адрес памяти
// char *buff,
// uint16_t buff_len
//*************************************************************************************************
void DataHexDump( uint8_t *addr, char *buff, uint16_t buff_len ) {

    char *ptr, ch;
    uint16_t idx, line, offset;

    if ( buff_len < 90 )
        return;
    ptr = buff;
    for ( line = 0, offset = 0; line < 16; line++, offset += 16 ) {
        //вывод адреса строки
        ptr += sprintf( ptr, "0x%04X: ", offset );
        //вывод данных
        for ( idx = 0; idx < 16; idx++ ) {
            ch = *( addr + offset + idx );
            //выводим коды
            if ( ( idx & 0x07 ) == 7 )
                ptr += sprintf( ptr, "%02X  ", ch );
            else ptr += sprintf( ptr, "%02X ", ch );
           }
        //выводим символы
        for ( idx = 0; idx < 16; idx++ ) {
            ch = *( addr + offset + idx );
            if ( ch >= 32 && ch < 127 )
                ptr += sprintf( ptr, "%c", ch );
            else ptr += sprintf( ptr, "." );
           }
        ptr += sprintf( ptr, "\r\n" );
//        if ( line == 15 )
//            ptr += sprintf( ptr, "\r\n" ); //отделим блок 16*16
        if ( buff_len < 1300 ) {
            //размер буфера не большой, вывод по одной строке
            UARTSendStr( buff );
            ptr = buff;
           }
      }
    //размер буфера позволяет вывести сразу весь блок 16*16
    if ( buff_len >= 1300 )
        UARTSendStr( buff );
 }

