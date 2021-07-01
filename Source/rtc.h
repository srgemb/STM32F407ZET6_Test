/*
 * rtc.h
 *
 *  Created on: 23 мая 2021 г.
 *      Author: admin
 */

#ifndef RTC_H_
#define RTC_H_

typedef enum {
    MASK_DATE = 0x01,
    MASK_TIME = 0x02
} DataTimeMask;

ErrorStatus TimeSet( char *time );
ErrorStatus DateSet( char *date );
void DateTime( DataTimeMask mask );
char *DateTimeStr( char *str, DataTimeMask mask );
uint8_t DayOfWeek( int day, int month, int year );

#endif /* RTC_H_ */
