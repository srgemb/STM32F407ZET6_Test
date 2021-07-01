/*
 * hooks.h
 *
 *  Created on: 8 ���. 2020 �.
 *      Author: admin
 */

#ifndef HOOKS_H_
#define HOOKS_H_

void vApplicationMallocFailedHook( void );
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName);

#endif /* HOOKS_H_ */
