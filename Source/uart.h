/*
 * uart.h
 *
 *  Created on: 27 янв. 2021 г.
 *      Author: admin
 */

#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include <stdbool.h>

void UARTInit( void );
void UARTSendStr( char *str );
void UARTRecvComplt( void );
void UARTSendComplt( void );

#endif /* UART_H_ */


