/*
 * hex.h
 *
 *  Created on: 18 мая 2021 г.
 *      Author: admin
 */

#ifndef HEX_H_
#define HEX_H_

void FlashHexDump( uint32_t *addr, char *buff, uint16_t buff_len );
void DataHexDump( uint8_t *addr, char *buff, uint16_t buff_len );

#endif /* HEX_H_ */
