/*
 * event.h
 *
 *  Created on: 30 янв. 2021 г.
 *      Author: admin
 */

#ifndef EVENT_H_
#define EVENT_H_

#define EVENT_KEY_PRESSED1          0x0001          //нажатие S1
#define EVENT_KEY_PRESSED2          0x0002          //нажатие S2
#define EVENT_KEY_PRESSED3          0x0004          //нажатие S3
#define EVENT_KEY_PRESSED4          0x0008          //нажатие S4

#define EVENT_CAN1_SEND             0x0010          //отправка сообщения по CAN1
#define EVENT_CAN2_SEND             0x0020          //отправка сообщения по CAN2

#define EVENT_CAN1_UPD              0x0100          //обновление данных в отправляемом пакете для CAN1
#define EVENT_CAN2_UPD              0x0200          //обновление данных в отправляемом пакете для CAN2

#define EVENT_CAN1_RECV             0x0040          //сообщение по CAN1 принято
#define EVENT_CAN2_RECV             0x0080          //сообщение по CAN2 принято

#define EVENT_UART_RECV             0x0001          //по UART1 принята строка завершенная CR

#define EVENT_MODBUS_SEND           0x1000          //передача пакета
#define EVENT_MODBUS_RECV           0x2000          //весь пакет от уст-ва получен
#define EVENT_MODBUS_CHECK          0x4000          //предварительная проверка примаемого пакета данных
#define EVENT_MODBUS_TIMEOUT        0x8000          //вышло время ожидания ответа

void EventInit( void );

#endif /* EVENT_H_ */
