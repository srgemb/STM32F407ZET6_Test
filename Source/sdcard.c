/*
 * sdcard.c
 *
 *  Created on: 1 июн. 2021 г.
 *      Author: admin1
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "fatfs.h"
#include "cmsis_os.h"

#include "main.h"
#include "can.h"
#include "rtc.h"
#include "uart.h"
#include "event.h"
#include "host.h"
#include "sdcard.h"

//*************************************************************************************************
// Локальные переменные
//*************************************************************************************************
static FRESULT mount;
static char str[90];
static const char sd_mount[] = "SDCard mount: %s\r\n";

static char * const sd_error_desc[] = {
    "Succeeded",                                                            //(0) FR_OK
    "A hard error occurred in the low level disk I/O layer",                //(1) FR_DISK_ERR
    "Assertion failed",                                                     //(2) FR_INT_ERR
    "The physical drive cannot work",                                       //(3) FR_NOT_READY
    "Could not find the file",                                              //(4) FR_NO_FILE
    "Could not find the path",                                              //(5) FR_NO_PATH
    "The path name format is invalid",                                      //(6) FR_INVALID_NAME
    "Access denied due to prohibited access or directory full",             //(7) FR_DENIED
    "Access denied due to prohibited access",                               //(8) FR_EXIST
    "The file/directory object is invalid",                                 //(9) FR_INVALID_OBJECT
    "The physical drive is write protected",                                //(10)FR_WRITE_PROTECTED
    "The logical drive number is invalid",                                  //(11)FR_INVALID_DRIVE
    "The volume has no work area",                                          //(12)FR_NOT_ENABLED
    "There is no valid FAT volume",                                         //(13)FR_NO_FILESYSTEM
    "The f_mkfs() aborted due to any problem",                              //(14)FR_MKFS_ABORTED
    "Could not get a grant to access the volume within defined period",     //(15)FR_TIMEOUT
    "The operation is rejected according to the file sharing policy",       //(16)FR_LOCKED
    "LFN working buffer could not be allocated",                            //(17)FR_NOT_ENOUGH_CORE
    "Number of open files > _FS_LOCK",                                      //(18)FR_TOO_MANY_OPEN_FILES
    "Given parameter is invalid"                                            //(19)FR_INVALID_PARAMETER
};

//*************************************************************************************************
// Прототипы локальных функций
//*************************************************************************************************


//*************************************************************************************************
// Монтирование файловой системы
// return ErrorStatus - результат выполенения
//*************************************************************************************************
ErrorStatus SDCardMount( void ) {

    mount = f_mount( &SDFatFS, (const TCHAR *)&SDPath, 1 );
    sprintf( str, sd_mount, sd_error_desc[mount] );
    //UARTSendStr( str );
    if ( mount != FR_OK )
        return ERROR;
    else return SUCCESS;
}

//*************************************************************************************************
// Тестовая запись файла
//*************************************************************************************************
void FileTest( void ) {

    FIL test;
    uint32_t bw;
    FRESULT res;
    char out[90], *ptr, text[] = "Welcome STM32F407ZET6\tПривет STM32F407ZET6\t";

    if ( mount != FR_OK )
        return;
    res = f_open( &test, "test.txt", FA_OPEN_ALWAYS | FA_WRITE );
    f_lseek( &test, f_size( &test ) );
    if ( res != FR_OK ) {
        HAL_GPIO_WritePin( LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET );
        return;
       }
    ptr = out;
    ptr += sprintf( ptr, "%s", text );
    ptr = DateTimeStr( ptr, MASK_DATE | MASK_TIME );
    sprintf( ptr, "\r\n" );
    res = f_write( &test, out, strlen( out ), (void *)&bw );
    if ( ( bw == 0 ) || ( res != FR_OK ) ) {
        HAL_GPIO_WritePin( LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET );
        return;
       }
    f_close( &test );
}

//*************************************************************************************************
// Добавление строки в текстовый файл
// char *name         - указатель на имя файл, в т.ч. с указанием пути
// char *str          - строка добавляемая в файл
// return ErrorStatus - результат выполенения
//*************************************************************************************************
ErrorStatus FileAddStr( char *name, char *str ) {

    FIL file;
    FRESULT res;
    uint32_t bw;

    if ( mount != FR_OK )
        return ERROR;
    res = f_open( &file, name, FA_OPEN_ALWAYS | FA_WRITE );
    f_lseek( &file, f_size( &file ) );
    if ( res != FR_OK )
        return ERROR;
    res = f_write( &file, str, strlen( str ), (void *)&bw );
    if ( ( bw == 0 ) || ( res != FR_OK ) )
        return ERROR;
    f_close( &file );
    return SUCCESS;
}
