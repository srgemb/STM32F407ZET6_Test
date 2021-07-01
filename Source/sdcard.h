/*
 * sdcard.h
 *
 *  Created on: 1 июн. 2021 г.
 *      Author: admin1
 */

#ifndef SDCARD_H_
#define SDCARD_H_

ErrorStatus SDCardMount( void );
ErrorStatus FileAddStr( char *name, char *str );

void FileTest( void );

#endif /* SDCARD_H_ */
