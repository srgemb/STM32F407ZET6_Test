/*
 * tcp.c
 *
 *  Created on: 20 мая 2021 г.
 *      Author: admin
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"

#include "cmsis_os.h"

#include "eeprom.h"
#include "uart.h"
#include "lwip_error.h"

//*************************************************************************************************
// Локальные константы
//*************************************************************************************************
#define TCP_TASK_STACK_SIZE         384

#define KEY_CR                      0x0D        //код '\r'
#define KEY_LF                      0x0A        //код '\n'

static const char msg_err_task[]   = "TaskEcho error.\r\n";
static const char msg_conn_open[]  = "Netconn open.\r\n";
static const char msg_conn_close[] = "Netconn close.\r\n";
static const char msg_err_connew[] = "Can not bind TCP netconn.\r\n";
static const char msg_err_create[] = "Can not create TCP netconn.\r\n";
static const char msg_err_memory[] = "Could not allocate required memory.\r\n";

//*************************************************************************************************
// Локальные переменные
//*************************************************************************************************
static CONFIG *config;
static uint8_t num_task = 1;
static uint16_t echo_cnt = 0;
static osThreadId TaskEchoHandle, TaskServerHandle;
static char name_task[configMAX_TASK_NAME_LEN];

//****************************************************************************************************************
// Прототипы локальных функций
//****************************************************************************************************************
static void TCPServer( void const *arg );
static void TCPThread( void const *arg );
static void TaskEcho( void const *arg );
static char *ClrCrLf( char *str );

//*************************************************************************************************
// Инициализация TCP server
//*************************************************************************************************
void TCPServerInit( void ) {

    config = ConfigGet();
    osThreadDef( TaskTCPServer, TCPServer, osPriorityNormal, 0, TCP_TASK_STACK_SIZE );
    TaskServerHandle = osThreadCreate( osThread( TaskTCPServer ), NULL );
    if ( TaskServerHandle == NULL )
        UARTSendStr( (char *)msg_err_task );
    else UARTSendStr( "TaskTCPServer ... OK.\r\n" );
 }

//*************************************************************************************************
// Инициализация TCP echo server
//*************************************************************************************************
void TCPEchoInit( void ) {

    osThreadDef( TaskTCPEcho, TaskEcho, osPriorityNormal, 0, TCP_TASK_STACK_SIZE );
    TaskEchoHandle = osThreadCreate( osThread( TaskTCPEcho ), NULL );
    if ( TaskEchoHandle == NULL )
        UARTSendStr( (char *)msg_err_task );
    else UARTSendStr( "TaskTCPEcho ... OK.\r\n" );
 }

//*************************************************************************************************
// Основной поток отслеживает новые подключения для порта №1.
// На каждое подключение создается новая задача.
//*************************************************************************************************
static void TCPServer( void const *arg ) {

    uint16_t port;
    ip_addr_t addr;
    uint8_t step = 0;
    char str[80], str_addr[20];
    BaseType_t xReturned;
    err_t err, accept_err;
    struct netconn *tcp_conn, *new_conn;

    for ( ;; ) {
        switch( step ) {
            case 0: {
                step++;
                //создание нового соединения
                tcp_conn = netconn_new( NETCONN_TCP );
                if ( tcp_conn == NULL ) {
                    UARTSendStr( (char *)msg_err_connew );
                    vTaskSuspend( NULL );
                   }
                break;
              }
            case 1: {
                //установите локальный IP-адрес/порт для соединения
                err = netconn_bind( tcp_conn, NULL, config->eth_port1 );
                if ( err == ERR_OK )
                    step++;
                break;
              }
            case 2: {
                step++;
                //слушаем соединение
                netconn_listen( tcp_conn );
                break;
               }
            case 3: {
                //Ждем подключения клиента и создаем новую задачу для обработки соединения
                accept_err = netconn_accept( tcp_conn, &new_conn );
                if ( accept_err == ERR_OK ) {
                   netconn_getaddr( new_conn, &addr, &port, 0 );
                   ip4addr_ntoa_r( &addr, str_addr, sizeof( str_addr ) );
                   sprintf( str, "%sConnect from: %s, port: %u\r\n", (char *)msg_conn_open, str_addr, config->eth_port1 );
                   UARTSendStr( str );
                   //формируем имя задачи
                   sprintf( name_task, "TCPThread_%03d", num_task );
                   xReturned = sys_thread_new( name_task, TCPThread, new_conn, TCP_TASK_STACK_SIZE, TCPIP_THREAD_PRIO );
                   if ( xReturned == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY )
                       UARTSendStr( (char *)msg_err_memory );
                   num_task++;
                  }
                break;
               }
            default: break;
        }
    }
}

//*************************************************************************************************
// Поток для каждого TCP соединения
//*************************************************************************************************
static void TCPThread( void const *arg ) {

    void *data;
    char *str_n, *recv_data;
    err_t recv_err;
    char str_addr[20];
    struct netconn *conn;
    struct netbuf *inbuf;
    ip4_addr_t clent_addr;
    uint16_t len, port;

    conn = (struct netconn*)arg;
    netconn_getaddr( conn, &clent_addr, &port, 0 ); //посмотрим, кто к нам подключился
    while ( 1 ) {
        recv_data = pvPortMalloc( 80 );
        str_n = pvPortMalloc( 160 );
        while ( ( recv_err = netconn_recv( conn, &inbuf ) ) == ERR_OK ) {
            do {
                netbuf_data( inbuf, &data, &len );
                memset( recv_data, 0x00, 80 );
                memset( str_n, 0x00, 160 );
                ClrCrLf( (char *)data );
                strncpy( recv_data, (char *)data, len );
                ip4addr_ntoa_r( &clent_addr, str_addr, sizeof( str_addr ) );
                sprintf( str_n, "Receive from: %s, Port: %u, Data: \"%s\" [%u]\r\n", str_addr, port, recv_data, len );
                UARTSendStr( str_n );
                netconn_write( conn, str_n, strlen( str_n ), NETCONN_COPY );
              } while ( netbuf_next( inbuf ) >= 0 );
            netbuf_delete( inbuf );
           }
        ip4addr_ntoa_r( &clent_addr, str_addr, sizeof( str_addr ) );
        sprintf( str_n, "Netconn from %s, port: %u close. (%s)\r\n", str_addr, port, GetLWIPErrorMsg( recv_err ) );
        UARTSendStr( str_n );
        netconn_close( conn );
        netconn_delete( conn );
        vPortFree( recv_data );
        vPortFree( str_n );
        vTaskDelete( NULL );
       }
 }

//*************************************************************************************************
// Задача TCP echo server
//*************************************************************************************************
void TaskEcho( void const *arg ) {

    struct netconn *conn, *newconn;
    err_t err, accept_err;
    struct netbuf *buf;
    void *data;
    char str[80], str_addr[20];
    ip_addr_t addr;
    u16_t len, port;
    err_t recv_err;
    LWIP_UNUSED_ARG( arg );

    conn = netconn_new( NETCONN_TCP );
    if ( conn != NULL ) {
        err = netconn_bind( conn, NULL, 7 );
        if ( err == ERR_OK ) {
            netconn_listen( conn );
            while ( 1 ) {
                accept_err = netconn_accept( conn, &newconn );
                if ( accept_err == ERR_OK ) {
                    echo_cnt = 0;
                    err = netconn_getaddr( newconn, &addr, &port, 0 );
                    if ( err == ERR_OK ) {
                        ip4addr_ntoa_r( &addr, str_addr, sizeof( str_addr ) );
                        sprintf( str, "%sEcho connect from: %s, port: %u\r\n", (char *)msg_conn_open, str_addr, port );
                        UARTSendStr( str );
                       }
                    while ( ( recv_err = netconn_recv( newconn, &buf ) ) == ERR_OK ) {
                        do {
                            echo_cnt++;
                            netbuf_data( buf, &data, &len );
                            netconn_write( newconn, data, len, NETCONN_COPY );
                          } while ( netbuf_next( buf ) >= 0 );
                        netbuf_delete( buf );
                       }
                    sprintf( str, "Echo count of echo requests = %u\r\n%s", echo_cnt, msg_conn_close );
                    UARTSendStr( str );
                    netconn_close( newconn );
                    netconn_delete( newconn );
                   }
              }
            sprintf( str, "Netconnect bind error: %d", err );
            UARTSendStr( str );
            vTaskSuspend( NULL );
           }
        else {
            netconn_delete( newconn );
            UARTSendStr( (char *)msg_err_create );
            vTaskSuspend( NULL );
           }
      }
    else {
        UARTSendStr( (char *)msg_err_connew );
        vTaskSuspend( NULL );
       }
 }

//*************************************************************************************************
// Удаляет в исходной строке коды CR и LF '\r\n'
// char *str - указатеель на исходную строку
// result    - указатель на исходную строку с удаленными CR и LF '\r\n'
//*************************************************************************************************
static char *ClrCrLf( char *str ) {

    char *temp;

    temp = str;
    while ( *temp ) {
        if ( *temp == KEY_CR || *temp == KEY_LF )
            *temp = '\0';
        temp++;
       }
    return str;
 }

