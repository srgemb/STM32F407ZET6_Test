/*
 * can.h
 *
 *  Created on: 8 февр. 2021 г.
 *      Author: admin
 */

#ifndef CAN_H_
#define CAN_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CAN_SPEED,                      //Speed
    CAN_PRESCALER,                  //Prescaler
    CAN_TIMESEG1,                   //Seg 1 (Prop_Seg+Phase_Seg1)
    CAN_TIMESEG2                    //Seg 2
} CAN_PARAM;

#pragma pack( push, 1 )

typedef struct {
    uint8_t can_numb;
    uint32_t msg_id;
    uint8_t data_len;
    uint8_t data[8];
} CAN_DATA;

#pragma pack( pop )

void CANInit( void );
void CANCommand( uint16_t cmnd );
uint32_t CANGetParam( uint16_t speed, CAN_PARAM id_param );

#endif /* CAN_H_ */
