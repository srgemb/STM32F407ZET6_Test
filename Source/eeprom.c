
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cmsis_os.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal_i2c.h"

#include "main.h"

#include "crc16.h"
#include "eeprom.h"
#include "uart.h"
#include "hex.h"

//*************************************************************************************************
// Локальные константы
//*************************************************************************************************
#define EEPROM_DEV_ADDR             0xA0        //адрес AT24C02B
#define EEPROM_SIZE                 256         //объем памяти AT24C02B
#define EEPROM_PAGE_SIZE            8           //объем одной страницы AT24C02B
#define EEPROM_TIME_OUT             40000

//*************************************************************************************************
// Внешние переменные
//*************************************************************************************************
extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart1;

//*************************************************************************************************
// Локальные переменные
//*************************************************************************************************
static CONFIG *config;
static EepromStatus load_stat = 0;
static uint8_t data_eeprom[EEPROM_SIZE] = {0,};
static char * const error_desc[] = {
    "Read configuration: OK.\r\n",
    "Read configuration: error, default parameters.\r\n",
    "Read configuration: checksum error, default parameters.\r\n",
    "Save configuration: error writing to EEPROM.\r\n",
    "Save configuration: OK.\r\n",
};

//*************************************************************************************************
// Прототипы локальных функций
//*************************************************************************************************
static ErrorStatus EepromSave( void );

//*************************************************************************************************
// Читаем конфигурацию из внешней EEPROM.
// При ошибках чтения или нарушения КС - устанавливаются параметры по умолчанию.
// return - результат чтения см. EepromStatus
//*************************************************************************************************
EepromStatus ConfigRead( void ) {

    uint16_t crc;
    EEPROM *eeprom;

    eeprom = (EEPROM *)&data_eeprom;
    if ( HAL_I2C_Mem_Read( &hi2c2, EEPROM_DEV_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, (uint8_t *)&data_eeprom, sizeof( data_eeprom ), EEPROM_TIME_OUT ) != HAL_OK ) {
        //данные не прочитаны, установка параметров ETH по умолчанию
        ConfigDefault();
        load_stat = EEPROM_READ_ERROR;
        return EEPROM_READ_ERROR;
       }
    else {
        //данные прочитаны
        eeprom = (EEPROM *)&data_eeprom;
        crc = CalcCRC16( (uint8_t *)eeprom, sizeof( EEPROM ) - 2 );
        if ( eeprom->crc != crc ) {
            ConfigDefault();
            load_stat = EEPROM_CRC_ERROR;
            return EEPROM_CRC_ERROR; //контрольная сумма не совпала
           }
        load_stat = EEPROM_READ_OK;
        return EEPROM_READ_OK;
       }
 }

//*************************************************************************************************
// Вывод результата загрузки параметров из внешней EEPROM
//*************************************************************************************************
void ConfigGetStat( void ) {

    char *ptr;

    ptr = ConfigError( load_stat );
    if ( osKernelRunning() )
        UARTSendStr( ptr ); //OS запущена, вывод через кольцевой буфер
    else HAL_UART_Transmit_IT( &huart1, (uint8_t *)ptr, strlen( ptr ) );
 }

//*************************************************************************************************
// Установка параметров конфигурации по умолчанию.
// Сохранение при этом не выполняется.
// Сохранение выполнять только после изменени параметров.
//*************************************************************************************************
void ConfigDefault( void ) {

    EEPROM *eeprom;

    memset( data_eeprom, 0x00, sizeof( data_eeprom ) );
    //установка параметров ETH по умолчанию
    config = (CONFIG *)&data_eeprom;
    config->eth_mac[0] = 0x40;
    config->eth_mac[1] = 0x16;
    config->eth_mac[2] = 0x7F;
    config->eth_mac[3] = 0x7E;
    config->eth_mac[4] = 0xD5;
    config->eth_mac[5] = 0x58;
    config->eth_dhcp = 0;
    config->eth_ip = 0x4D01A8C0;     //192.168.1.77
    config->eth_mask = 0x00FFFFFF;   //255.255.255.0
    config->eth_gate = 0x0101A8C0;   //192.168.1.1
    config->eth_port1 = 49100;
    config->eth_port2 = 49101;
    config->uart1_speed = 115200;
    config->modbus_speed = 19200;
    config->can1_speed = 125;
    config->can2_speed = 125;
    //контрольная сумма с учетом установленных параметров
    eeprom = (EEPROM *)&data_eeprom;
    eeprom->crc = CalcCRC16( (uint8_t *)eeprom, sizeof( EEPROM ) - 2 );
 }

//*************************************************************************************************
// Возвращает указатель на структуру CONFIG
//*************************************************************************************************
CONFIG *ConfigGet( void ) {

    return (CONFIG *)&data_eeprom;
 }

//*************************************************************************************************
// Запись конфигурации во внешний EEPROM
// ETH_CONFIG *new_conf - указатель на структуру с параметрами ETH
// если new_conf = NULL - сохраним данные непосредственно из data_eeprom
// return = SUCCESS     - данные записаны
//        = ERROR       - ошибка записи параметров
//*************************************************************************************************
ErrorStatus ConfigSave( CONFIG *new_conf ) {

    char *ptr;
    uint8_t stat;
    EEPROM *eeprom;

    if ( new_conf != NULL ) {
        //копируем данные из новой структуры
        config = (CONFIG *)&data_eeprom;
        memcpy( config, new_conf, sizeof( CONFIG ) );
       }
    eeprom = (EEPROM *)&data_eeprom;
    eeprom->crc = CalcCRC16( (uint8_t *)eeprom, sizeof( EEPROM ) - 2 );
    if ( ( stat = EepromSave() ) == ERROR )
        ptr = ConfigError( EEPROM_SAVE_ERROR );
    else ptr = ConfigError( EEPROM_SAVE_OK );
    if ( osKernelRunning() )
        UARTSendStr( ptr ); //OS запущена, вывод через кольцевой буфер
    else HAL_UART_Transmit_IT( &huart1, (uint8_t *)ptr, strlen( ptr ) );
    return stat;
 }

//*************************************************************************************************
// Запись конфигурации во внешний EEPROM.
// Запись выполняется постранично, размер страницы: EEPROM_PAGE_SIZE
// Интервал записи между страницами 7 ms.
// return = SUCCESS - данные записаны
//        = ERROR   - ошибка записи параметров
//*************************************************************************************************
static ErrorStatus EepromSave( void ) {

    uint8_t page, offset;

    for ( offset = 0, page = 0; page < ( EEPROM_SIZE / EEPROM_PAGE_SIZE ); page++, offset += EEPROM_PAGE_SIZE ) {
        HAL_Delay( 7 );
        if ( HAL_I2C_Mem_Write( &hi2c2, EEPROM_DEV_ADDR, offset, I2C_MEMADD_SIZE_8BIT, ( data_eeprom + offset ), EEPROM_PAGE_SIZE, EEPROM_TIME_OUT ) != HAL_OK )
            return ERROR;
       }
    return SUCCESS;
}

//*************************************************************************************************
// Возвращает расшифровку ошибки чтения/записи параметров в/из EEPROM
// EepromStatus status - код ошибки
// return              - указатель на расшифровку ошибки
//*************************************************************************************************
char *ConfigError( EepromStatus status ) {

    if ( status > EEPROM_SAVE_OK )
        return NULL;
    return error_desc[status];
}
