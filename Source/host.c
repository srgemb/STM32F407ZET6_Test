/*
 * host.c
 *
 *  Created on: 13 мар. 2021 г.
 *      Author: admin
 */

#include <stdio.h>
#include <string.h>

#include "main.h"
#include "cmsis_os.h"
#include "lwip.h"

#include "host.h"
#include "event.h"
#include "eeprom.h"
#include "modbus.h"
#include "uart.h"
#include "rtc.h"
#include "hex.h"

//****************************************************************************************************************
// Внешние переменные
//****************************************************************************************************************
extern struct netif gnetif;

//****************************************************************************************************************
// Локальные константы
//****************************************************************************************************************
//индексы для получения параметров команды
#define IND_PAR_CMND            0           //команда
#define IND_PARAM1              1           //параметр 1
#define IND_PARAM2              2           //параметр 2
#define IND_PARAM3              3           //параметр 3
#define IND_PARAM4              4           //параметр 4
#define IND_PARAM5              5           //параметр 5
#define IND_PARAM6              6           //параметр 6
#define IND_PARAM7              7           //параметр 7
#define IND_PARAM8              8           //параметр 8

#define CMD_COUNT           	( sizeof( cmd )/sizeof( cmd[0] ) )

//структура хранения и выполнения команд
typedef struct {
    char    name_cmd[20];                              //имя команды
    void    (*func)( uint8_t cnt_par, char *param );   //указатель на функцию выполнения
} SCMD;

//****************************************************************************************************************
// Прототипы локальных функций
//****************************************************************************************************************
static uint8_t ParseCommand( char *src, char *par );
static char *GetParam( char *param, uint8_t index );
static int8_t CheckBaudRate( uint32_t *list, uint8_t size, uint32_t value );
static char *GetMacToStr( char *mac_adr, char *buff, uint8_t len_buff );

static void CmndIP( uint8_t cnt_par, char *param );
static void CmndDate( uint8_t cnt_par, char *param );
static void CmndTime( uint8_t cnt_par, char *param );
static void CmndDateTime( uint8_t cnt_par, char *param );
static void CmndMAC( uint8_t cnt_par, char *param );
static void CmndHelp( uint8_t cnt_par, char *param );
static void CmndStat( uint8_t cnt_par, char *param );
static void CmndConfig( uint8_t cnt_par, char *param );
static void CmndPort( uint8_t cnt_par, char *param );
static void CmndUart( uint8_t cnt_par, char *param );
static void CmndCan( uint8_t cnt_par, char *param );
static void CmndMbus( uint8_t cnt_par, char *param );
static void CmndMbusTest( uint8_t cnt_par, char *param );
static void CmndVersion( uint8_t cnt_par, char *param );
static void CmndReset( uint8_t cnt_par, char *param );

//****************************************************************************************************************
// Локальные переменные
//****************************************************************************************************************
static char stat[4096];
static char param_cons[MAX_CNT_PARAM][MAX_LEN_PARAM];

static const char prompt[]     = ">\r\n";
static const char no_command[] = "Enter the command.\r\n";
static const char err_ip[]     = "Invalid IP address.\r\n";
static const char err_mask[]   = "Invalid mask.\r\n";
static const char err_param[]  = "Invalid parameters.\r\n";
static const char err_gate[]   = "Invalid GATE address.\r\n";
static const char str_delim[]  = "----------------------------------------------------\r\n";

static uint32_t baud_rate_can[]  = {10,20,50,125,250,500};
static uint32_t baud_rate_uart[] = {9600,14400,19200,28800,38400,56000,57600,115200};

static const char *help = {
    "date                    - Display/set date.\r\n"
    "time [hh:mm[:ss]]       - Display/set time.\r\n"
    "dtime [dd.mm.yy]        - Display date and time.\r\n"
    "stat                    - List, task statuses, time statistics.\r\n"
    "mac [xx xx xx xx xx xx] - Displaying / setting the MAC address.\r\n"
    "ip [[ip][mask][gate]]   - Displaying the current configuration and setting parameters.\r\n"
    "config [hex]            - Display of configuration parameters.\r\n"
    "uart speed              - Setting the speed (9200,14400,19200,28800,38400,56000,57600,115200) baud.\r\n"
    "modbus speed            - Setting the speed (9200,14400,19200,28800,38400,56000,57600,115200) baud.\r\n"
    "mdebug                  - Enabling / disabling the output of debug information exchange via MODBUS.\r\n"
    "can {1/2} speed         - Setting the speed (10,20,50,125,250,500) kbit/s.\r\n"
    "version                 - Displays the version number and date.\r\n"
    "reset                   - Displays the version number and date.\r\n"
    "test                    - Sending a test command by MODBUS.\r\n"
    "?                       - Help. \r\n"
  };

//Перечень доступных команд
//-------------------------
//имя           функция
//команды       вызова
//-------------------------
static const SCMD cmd[] = {
    { "time",       CmndTime },
    { "date",       CmndDate },
    { "dtime",      CmndDateTime },
    { "ip",         CmndIP },
    { "mac",        CmndMAC },
    { "stat",       CmndStat },
    { "config",     CmndConfig },
    { "uart",       CmndUart },
    { "port",       CmndPort },
    { "modbus",     CmndUart },
    { "mdebug",     CmndMbus },
    { "can",        CmndCan },
    { "version",    CmndVersion },
    { "reset",      CmndReset },
    { "test",       CmndMbusTest },
    { "?",          CmndHelp }
 };

//****************************************************************************************************************
// Обработка команд полученных по UART
// char *buff - указатель на буфер с командой
//****************************************************************************************************************
void ExecCommand( char *buff ) {

    uint8_t i, cnt_par;

    //разбор параметров команды
    cnt_par = ParseCommand( buff, *param_cons );
    //проверка и выполнение команды
    for ( i = 0; i < CMD_COUNT; i++ ) {
        if ( strcasecmp( (const char *)&cmd[i].name_cmd, param_cons[IND_PAR_CMND] ) )
            continue;
        cmd[i].func( cnt_par, *param_cons ); //выполнение команды
        UARTSendStr( (char *)prompt );
        break;
       }
    if ( i == CMD_COUNT ) {
        UARTSendStr( (char *)no_command );
        UARTSendStr( (char *)prompt );
       }
 }

//****************************************************************************************************************
// Вывод списка задач и их статус
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
void CmndStat( uint8_t cnt_par, char *param ) {

    char *ptr;
    uint16_t prev_len;

    ptr = stat;
	//место в куче
	ptr += sprintf( ptr, "Free Heap Size = %u bytes\r\n", xPortGetFreeHeapSize() );
    //список задач и их статус
	ptr += sprintf( ptr, "Task name     State Priority   Stack   Num\r\n" );
	ptr += sprintf( ptr, "%s", str_delim );
	prev_len = strlen( stat );
    osThreadList( (uint8_t *)ptr );
    ptr += strlen( stat ) - prev_len;
    ptr += sprintf( ptr, "%s", str_delim );
    ptr += sprintf( ptr, "B : Blocked, R : Ready, D : Deleted, S : Suspended\r\n\r\n" );
    //список задач и использованный стек и время
    ptr += sprintf( ptr, "Task name\tAbs time\t%% Time\r\n" );
    ptr += sprintf( ptr, "%s", str_delim );
    prev_len = strlen( stat );
	vTaskGetRunTimeStats( ptr );
    ptr += strlen( stat ) - prev_len;
    ptr += sprintf( ptr, "%s", str_delim );
    UARTSendStr( stat );
 }

//****************************************************************************************************************
// Вывод и установка конфигурации ETH
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
static void CmndIP( uint8_t cnt_par, char *param ) {

    CONFIG *config;
    uint32_t ip = 0UL, mask = 0UL, gate = 0UL;

    config = ConfigGet();
    memset( stat, 0x00, sizeof( stat ) );
    if ( cnt_par == 1 ) {
        //только просмотр параметров ETH
        CmndConfig( 0, NULL );
        return;
      }
    if ( cnt_par > 1 ) {
        if ( strlen( GetParam( param, IND_PARAM1 ) ) ) {
            //установка IP адреса ETH
            ip = ipaddr_addr( GetParam( param, IND_PARAM1 ) );
            if ( ip == IPADDR_NONE ) {
                UARTSendStr( (char *)err_ip );
                UARTSendStr( (char *)prompt );
                return;
               }
            gnetif.ip_addr.addr = ip;
            config->eth_ip = ip;
          }
        if ( strlen( GetParam( param, IND_PARAM2 ) ) ) {
            //установка маски ETH
            mask = ipaddr_addr( GetParam( param, IND_PARAM2 ) );
            if ( !ip4_addr_netmask_valid( mask ) ) {
                UARTSendStr( (char *)err_mask );
                UARTSendStr( (char *)prompt );
                return;
               }
            gnetif.netmask.addr = mask;
            config->eth_mask = mask;
          }
        if ( strlen( GetParam( param, IND_PARAM3 ) ) ) {
            //установка IP шлюза ETH
            gate = ipaddr_addr( GetParam( param, IND_PARAM3 ) );
            if ( gate == IPADDR_NONE ) {
                UARTSendStr( (char *)err_gate );
                UARTSendStr( (char *)prompt );
                return;
               }
            gnetif.gw.addr = gate;
            config->eth_gate = gate;
          }
        ConfigSave( NULL );
        ethernetif_update_config( &gnetif );
        CmndConfig( 0, NULL );
      }
 }

//****************************************************************************************************************
// Вывод и установка значения MAC адреса
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
static void CmndMAC( uint8_t cnt_par, char *param ) {

    int i, val;
    char addr[20];
    CONFIG *config;

    config = ConfigGet();
    if ( cnt_par != 1 && cnt_par != 7 ) {
        UARTSendStr( (char *)err_param );
        UARTSendStr( (char *)prompt );
        return;
       }
    if ( cnt_par == 1 ) {
        //только просмотр адреса
        sprintf( stat, "MAC = %s\r\n", GetMacToStr( (char *)&gnetif.hwaddr, addr, sizeof( addr ) ) );
        UARTSendStr( stat );
      }
    if ( cnt_par == 7 ) {
        for ( i = 0; i < 6; i++ ) {
            sscanf( GetParam( param, IND_PARAM1 + i ), "%x", &val );
            config->eth_mac[i] = (uint8_t)val;
           }
        ConfigSave( NULL );
       }
 }

//****************************************************************************************************************
// Вывод текущей скорости портов UART, CAN, кофигурации ETH
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
static void CmndConfig( uint8_t cnt_par, char *param ) {

    char *ptr, addr[20];
    CONFIG *config;

    ptr = stat;
    config = ConfigGet();
    memset( stat, 0x00, sizeof( stat ) );
    if ( cnt_par == 2 && !strcasecmp( GetParam( param, IND_PARAM1 ), "hex" ) ) {
        DataHexDump( (uint8_t *)config, ptr, sizeof( stat ) );
        return;
       }
    else {
        ptr += sprintf( ptr, "DHCP   = %s\r\n", config->eth_dhcp ? "ON" : "OFF" );
        ptr += sprintf( ptr, "MAC    = %s\r\n", GetMacToStr( (char *)&gnetif.hwaddr, addr, sizeof( addr ) ) );
        ptr += sprintf( ptr, "IP     = %s\r\n", ip4addr_ntoa_r( &gnetif.ip_addr, addr, sizeof( addr ) ) );
        ptr += sprintf( ptr, "MASK   = %s\r\n", ip4addr_ntoa_r( &gnetif.netmask, addr, sizeof( addr ) ) );
        ptr += sprintf( ptr, "GATE   = %s\r\n", ip4addr_ntoa_r( &gnetif.gw, addr, sizeof( addr ) ) );
        ptr += sprintf( ptr, "PORT1  = %u\r\n", config->eth_port1 );
        ptr += sprintf( ptr, "PORT2  = %u\r\n", config->eth_port2 );
        ptr += sprintf( ptr, "CAN1   = %u\r\n", config->can1_speed );
        ptr += sprintf( ptr, "CAN2   = %u\r\n", config->can2_speed );
        ptr += sprintf( ptr, "UART   = %u\r\n", (uint)config->uart1_speed );
        ptr += sprintf( ptr, "MODBUS = %u\r\n", (uint)config->modbus_speed );
        UARTSendStr( stat );
       }
 }

//****************************************************************************************************************
// Вывод номера и даты версии
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
static void CmndVersion( uint8_t cnt_par, char *param ) {

    sprintf( stat, "%s %s\r\n", __DATE__, __TIME__ );
    UARTSendStr( stat );
}

//****************************************************************************************************************
// Разбор параметров команды, параметры размещаются в массиве param[][]
// char *src    - строка с параметрами
// char *par    - указатель на массив параметров param[][]
// return       - количество параметров, в т.ч. команда
// Если параметров указано больше MAX_CNT_PARAM, лишниние параметры игнорируется
//****************************************************************************************************************
static uint8_t ParseCommand( char *src, char *par ) {

    uint8_t i = 0;
	char *str, *token, *saveptr;

	//обнулим предыдущие параметры
	memset( par, 0x00, sizeof( param_cons ) );
    //разбор параметров
    str = src;
    for ( i = 0; ; i++ ) {
		token = strtok_r( str, " ", &saveptr );
		if ( token == NULL )
			break;
		str = saveptr;
		strncpy( par + i * MAX_LEN_PARAM, token, strlen( token ) );
		}
    return i;
 }

//****************************************************************************************************************
// Возвращает указатель на значение параметра по индексу
// char *param   - указатель на массив параметров param[index][]
// uint8_t index - индекс параметра
//****************************************************************************************************************
static char *GetParam( char *param, uint8_t index ) {

    if ( index >= MAX_CNT_PARAM )
        return NULL;
    return param + index * MAX_LEN_PARAM;
 }

//****************************************************************************************************************
// Вывод/установка текущего значения дата/время
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
static void CmndDate( uint8_t cnt_par, char *param ) {

    if ( cnt_par == 2 ) {
        if ( DateSet( GetParam( param, IND_PARAM1 ) ) == ERROR ) {
            UARTSendStr( (char *)err_param );
            return;
           }
       }
    DateTime( MASK_DATE ); //вывод даты
}

//****************************************************************************************************************
// Вывод/установка текущего значения времени
// Установка времени, входной формат HH:MI:SS
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
static void CmndTime( uint8_t cnt_par, char *param ) {

    if ( cnt_par == 2 ) {
        if ( TimeSet( GetParam( param, IND_PARAM1 ) ) == ERROR ) {
            UARTSendStr( (char *)err_param );
            return;
           }
       }
    DateTime( MASK_TIME );  //вывод времени
 }

//****************************************************************************************************************
// Вывод текущего значения дата/время
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
static void CmndDateTime( uint8_t cnt_par, char *param ) {

    DateTime( MASK_DATE | MASK_TIME );
}

//****************************************************************************************************************
// Установка скорости обмена для портов UART1/2 (RS232/RS485)
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
static void CmndUart( uint8_t cnt_par, char *param ) {

    uint32_t val;
    CONFIG *config;

    config = ConfigGet();
    if ( cnt_par == 2 && !strcasecmp( GetParam( param, IND_PAR_CMND ), "uart" ) ) {
        val = atoi( GetParam( param, IND_PARAM1 ) );
        if ( CheckBaudRate( baud_rate_uart, sizeof( baud_rate_uart ), val ) < 0 ) {
            UARTSendStr( (char *)err_param );
            return;
        }
        config->uart1_speed = val;
        ConfigSave( NULL );
        return;
       }
    if ( cnt_par == 2 && !strcasecmp( GetParam( param, IND_PAR_CMND ), "modbus" ) ) {
        val = atoi( GetParam( param, IND_PARAM1 ) );
        if ( CheckBaudRate( baud_rate_uart, sizeof( baud_rate_uart ), val ) < 0 ) {
            UARTSendStr( (char *)err_param );
            return;
        }
        config->modbus_speed = val;
        ConfigSave( NULL );
        return;
       }
    UARTSendStr( (char *)err_param );
}

//****************************************************************************************************************
// Установка скорости обмена для интерфейса CAN
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
static void CmndCan( uint8_t cnt_par, char *param ) {

    uint32_t val;
    uint8_t intf;
    CONFIG *config;

    config = ConfigGet();
    if ( cnt_par == 3 && !strcasecmp( GetParam( param, IND_PAR_CMND ), "can" ) ) {
        intf = atoi( GetParam( param, IND_PARAM1 ) );
        if ( intf != 1 && intf != 2 ) {
            UARTSendStr( (char *)err_param );
            return;
           }
        val = atoi( GetParam( param, IND_PARAM2 ) );
        if ( CheckBaudRate( baud_rate_can, sizeof( baud_rate_can ), val ) < 0 ) {
            UARTSendStr( (char *)err_param );
            return;
        }
        if ( intf == 1 )
            config->can1_speed = val;
        else config->can2_speed = val;
        ConfigSave( NULL );
        return;
       }
    UARTSendStr( (char *)err_param );
}

//****************************************************************************************************************
// Установка номеров портов для TCP/IP серверов
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
static void CmndPort( uint8_t cnt_par, char *param ) {

    uint8_t intf;
    CONFIG *config;

    config = ConfigGet();
    if ( cnt_par == 3 && !strcasecmp( GetParam( param, IND_PAR_CMND ), "port" ) ) {
        intf = atoi( GetParam( param, IND_PARAM1 ) );
        if ( intf != 1 && intf != 2 ) {
            UARTSendStr( (char *)err_param );
            return;
           }
        if ( intf == 1 )
            config->eth_port1 = atoi( GetParam( param, IND_PARAM2 ) );
        else config->eth_port2 = atoi( GetParam( param, IND_PARAM2 ) );
        ConfigSave( NULL );
        return;
       }
    UARTSendStr( (char *)err_param );
}

//****************************************************************************************************************
// Включение/выключение отладочной информации обмена по MODBUS
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
static void CmndMbus( uint8_t cnt_par, char *param ) {

    ModBusDebug();
}

//****************************************************************************************************************
// Перезапуск контроллера
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
static void CmndReset( uint8_t cnt_par, char *param ) {

    NVIC_SystemReset();
}

//****************************************************************************************************************
// Тестовая функция отправки команды по MODBUS
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
static void CmndMbusTest( uint8_t cnt_par, char *param ) {

    TestExec();
}

//****************************************************************************************************************
// Формирует двоичный MAC адрес в формат: XX:XX:XX:XX:XX:XX
// char *mac_adr    - указатель на буфер в котором размещен MAC адрес
// char *buff       - адрес буфера для размещения строки XX:XX:XX:XX:XX:XX
// uint8_t len_buff - размер буфера
// return == NULL   - размер буфера недостаточен для размещения результата
//        != NULL   - адрес буфера с результатом
//****************************************************************************************************************
static char *GetMacToStr( char *mac_adr, char *buff, uint8_t len_buff ) {

    uint8_t ind, offset;

    if ( len_buff < 19 )
        return NULL;
    memset( buff, 0x00, len_buff );
    for ( ind = 0, offset = 0; ind < 6; ind++, offset += 3 ) {
        if ( ind == 5 )
            sprintf( buff + offset, "%02X", *( mac_adr + ind ) );
        else sprintf( buff + offset, "%02X:", *( mac_adr + ind ) );
       }
    return buff;
 }

//****************************************************************************************************************
// Вывод подсказки по доступным командам
// uint8_t cnt_par - кол-во параметров
// char *param     - указатель на список параметров
//****************************************************************************************************************
static void CmndHelp( uint8_t cnt_par, char *param ) {

    UARTSendStr( (char *)help );
 }

//****************************************************************************************************************
// Вывод приглашения
//****************************************************************************************************************
void HostPrompt( void ) {

    UARTSendStr( (char *)prompt );
}

//****************************************************************************************************************
// Проверка значения VALUE на допустимое значение из списка LIST
// uint32_t *list - указатель на массив допустимых параметров
// uint8_t size   - полный размер массива допустимых параметров в байтах
// uint32_t value - проверяемое значение
// return = -1    - не допустимое значение
//        >= 0    - индекс значения допустимого значения
//****************************************************************************************************************
static int8_t CheckBaudRate( uint32_t *list, uint8_t size, uint32_t value ) {

    uint8_t ind, max;

    max = size / sizeof( uint32_t );
    for ( ind = 0; ind < max; ind++ ) {
        if ( value == list[ind] )
            return ind;
       }
    return -1;
}
