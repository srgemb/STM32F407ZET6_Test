/*
 * eeprom.h
 *
 *  Created on: 8 февр. 2021 г.
 *      Author: admin
 */

#ifndef EEPROM_H_
#define EEPROM_H_

typedef enum {
    EEPROM_READ_OK,         //данные EEPROM прочитаны успешно
    EEPROM_READ_ERROR,      //ошибка чтения EEPROM
    EEPROM_CRC_ERROR,       //ошибка контрольной суммы EEPROM
    EEPROM_SAVE_ERROR,      //ошибка записи в EEPROM
    EEPROM_SAVE_OK          //запись в EEPROM выполнена
} EepromStatus;

#pragma pack( push, 1 )

//****************************************************************************************************************
// Структура хранения параметров уст-ва
//****************************************************************************************************************
typedef struct {
    uint8_t  data[254];
    uint16_t crc;
 } EEPROM;

typedef struct {
    uint8_t  eth_mac[6];
    uint8_t  eth_dhcp;
    uint32_t eth_ip;                //192.168.1.77
    uint32_t eth_mask;              //255.255.255.0
    uint32_t eth_gate;              //192.168.1.1
    uint16_t eth_port1;             //49100
    uint16_t eth_port2;             //49101
    uint32_t uart1_speed;           //38400
    uint32_t modbus_speed;          //38400
    uint16_t can1_speed;            //125
    uint16_t can2_speed;            //125
 } CONFIG;

#pragma pack( pop )

 EepromStatus ConfigRead( void );
void ConfigDefault( void );
void ConfigGetStat( void );
char *ConfigError( EepromStatus status );

CONFIG *ConfigGet( void );
ErrorStatus ConfigSave( CONFIG *eth_conf );

#endif /* EEPROM_H_ */
