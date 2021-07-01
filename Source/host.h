/*
 * host.h
 *
 *  Created on: 13 мар. 2021 г.
 *      Author: admin
 */

#ifndef HOST_H_
#define HOST_H_

#define MAX_CNT_PARAM           10          //максимальное кол-во параметров, включая команду
#define MAX_LEN_PARAM           20          //максимальный размер (длинна) параметра в коммандной строке

void ExecCommand( char *buff );
void HostPrompt( void );

#endif /* HOST_H_ */
