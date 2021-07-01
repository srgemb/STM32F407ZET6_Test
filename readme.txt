
---------------------------------------------------------------------------------------------------------------
Тестирования компонентов: EEPROM, UART, CAN, RTC, SDCard, Ethernet отладочной платы на базе STM32F407ZET6.
---------------------------------------------------------------------------------------------------------------

1. EEPROM - хранение параметров конфигурации.

2. UART1 - RS-232 (115200,8N1) - прием команд, выдача результата выполения команды. 
   Перечень доступных команд:
   date                    - Display/set date.
   time [hh:mm[:ss]]       - Display/set time.
   dtime [dd.mm.yy]        - Display date and time.
   stat                    - List, task statuses, time statistics.
   mac [xx xx xx xx xx xx] - Displaying / setting the MAC address.
   ip [[ip][mask][gate]]   - Displaying the current configuration and setting parameters.
   config [hex]            - Display of configuration parameters.
   uart speed              - Setting the speed (9200,14400,19200,28800,38400,56000,57600,115200) baud.
   modbus speed            - Setting the speed (9200,14400,19200,28800,38400,56000,57600,115200) baud.
   mdebug                  - Enabling / disabling the output of debug information exchange via MODBUS.
   can {1/2} speed         - Setting the speed (10,20,50,125,250,500) kbit/s.
   version                 - Displays the version number and date.
   reset                   - Displays the version number and date.
   test                    - Sending a test command by MODBUS.
   ?                       - Help.

3. UART2 - RS-485 (19200,8N1) - обмен по протоколу MODBUS.
   Выполняет чтение/запись регистров модуля WP8028ADAM.
   Интервал выполнения цикла - 300 ms.

4. CAN1/2 (125 kbit/s) - Прием данных. Циклическая, поочередная отправка фреймов по обоим интерфейсам.
   Размер данных - 8 байт, данные циклически меняются от 0x00 до 0xFF. Интервал передачи - 300 ms.

5. SDCard - монтирование файловой системы, тестовая запись в файл.

6. Обработка нажатий кнопок S1-S4, логирование нажатий в файле. Управление светодиодными индикаторами.

7. TCP/IP (LwIP) - Сервер для портов 7 (echo), 49100 - прием текстовых данных, вывод принятых данных в UART1
   и обратно клиенту. Поддержка нескольких соединений с отдельными потоками.

***************************************************************************************************************
Вывод информации контроллером UART1

Read configuration: OK.
TaskTCPServer ... OK.
TaskTCPEcho ... OK.
Date 28.06.21 Time 20:57:53 

>mdebug
IDX=0 DEV=0x04 FUNC=0x03 The slave is not responding
IDX=1 DEV=0x10 FUNC=0x02 RES=0 LEN=1 DATA=0x00 
IDX=2 DEV=0x10 FUNC=0x0F RES=0 LEN=2 DATA=0x00 0x08 
IDX=3 DEV=0x10 FUNC=0x01 RES=0 LEN=1 DATA=0x55 
IDX=0 DEV=0x04 FUNC=0x03 The slave is not responding
IDX=1 DEV=0x10 FUNC=0x02 RES=0 LEN=1 DATA=0x00 
IDX=2 DEV=0x10 FUNC=0x0F RES=0 LEN=2 DATA=0x00 0x08 
IDX=3 DEV=0x10 FUNC=0x01 RES=0 LEN=1 DATA=0xAA 
IDX=0 DEV=0x04 FUNC=0x03 The slave is not responding
IDX=1 DEV=0x10 FUNC=0x02 RES=0 LEN=1 DATA=0x00 
IDX=2 DEV=0x10 FUNC=0x0F RES=0 LEN=2 DATA=0x00 0x08 
IDX=3 DEV=0x10 FUNC=0x01 RES=0 LEN=1 DATA=0xFF 
IDX=0 DEV=0x04 FUNC=0x03 The slave is not responding
IDX=1 DEV=0x10 FUNC=0x02 RES=0 LEN=1 DATA=0x00 
IDX=2 DEV=0x10 FUNC=0x0F RES=0 LEN=2 DATA=0x00 0x08 
IDX=3 DEV=0x10 FUNC=0x01 RES=0 LEN=1 DATA=0x00 
IDX=0 DEV=0x04 FUNC=0x03 The slave is not responding
IDX=1 DEV=0x10 FUNC=0x02 RES=0 LEN=1 DATA=0x00 
IDX=2 DEV=0x10 FUNC=0x0F RES=0 LEN=2 DATA=0x00 0x08 
IDX=3 DEV=0x10 FUNC=0x01 RES=0 LEN=1 DATA=0x01 

>date
Date 28.06.21 

>time
Time 21:02:00 

>
Date 28.06.21 Time 21:07:47

>stat
Free Heap Size = 22664 bytes
Task name     State Priority   Stack   Num
----------------------------------------------------
TaskUART       	X      3        246     1
IDLE           	R      0        104     6
LinkThr        	B      2        222     10
TaskLed        	B      3        72      5
tcpip_thread   	B      3        882     8
TaskTest       	B      3        240     12
TaskTCPServer  	B      3        263     13
TaskTCPEcho    	B      3        239     14
TaskEvent      	B      3        104     2
EthIf          	B      6        226     9
TaskCAN1       	B      3        178     3
TaskModBus     	B      3        448     11
Tmr Svc        	B      2        186     7
TaskCAN2       	B      3        198     4
----------------------------------------------------
B : Blocked, R : Ready, D : Deleted, S : Suspended

Task name	Abs time	% Time
----------------------------------------------------
TaskUART       	23       <1%
IDLE           	3324432	 97%
TaskLed        	67281     1%
tcpip_thread   	346      <1%
TaskTest       	2889     <1%
LinkThr        	834      <1%
TaskTCPEcho    	5        <1%
TaskEvent      	967      <1%
EthIf          	55       <1%
TaskCAN1       	279      <1%
TaskModBus     	2037     <1%
Tmr Svc        	1122     <1%
TaskCAN2       	1        <1%
TaskTCPServer  	6        <1%
----------------------------------------------------

>config
DHCP   = OFF
MAC    = 40:16:7F:7E:D5:58
IP     = 192.168.1.77
MASK   = 255.255.255.0
GATE   = 192.168.1.1
PORT1  = 49100
PORT2  = 49101
CAN1   = 125
CAN2   = 125
UART   = 115200
MODBUS = 19200

>version
Jun 28 2021 20:57:54

>config hex
0x0000: 40 16 7F 7E D5 58 00 C0  A8 01 4D FF FF FF 00 C0  @..~.X....M.....
0x0010: A8 01 01 CC BF CD BF 00  C2 01 00 00 4B 00 00 7D  ............K..}
0x0020: 00 7D 00 00 00 00 00 00  00 00 00 00 00 00 00 00  .}..............
0x0030: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
0x0040: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
0x0050: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
0x0060: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
0x0070: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
0x0080: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
0x0090: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
0x00A0: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
0x00B0: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
0x00C0: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
0x00D0: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
0x00E0: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................
0x00F0: 00 00 00 00 00 00 00 00  00 00 00 00 00 00 F2 C5  ................

Netconn open.
Connect from: 192.168.1.33, port: 49100
Receive from: 192.168.1.33, Port: 62775, Data: "0123456789" [12]
Netconn from 192.168.1.33, port: 53709 close. (Connection reset.)

Netconn open.
Echo connect from: 192.168.1.33, port: 53732
Echo count of echo requests = 15
Netconn close.

