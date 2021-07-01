/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SD_CD_Pin GPIO_PIN_13
#define SD_CD_GPIO_Port GPIOC
#define EEPROM_SDA_Pin GPIO_PIN_0
#define EEPROM_SDA_GPIO_Port GPIOF
#define EEPROM_SCL_Pin GPIO_PIN_1
#define EEPROM_SCL_GPIO_Port GPIOF
#define CPU_LED_Pin GPIO_PIN_4
#define CPU_LED_GPIO_Port GPIOF
#define TIM7_CHECK_Pin GPIO_PIN_5
#define TIM7_CHECK_GPIO_Port GPIOF
#define CHECK1_Pin GPIO_PIN_6
#define CHECK1_GPIO_Port GPIOF
#define CHECK2_Pin GPIO_PIN_7
#define CHECK2_GPIO_Port GPIOF
#define LED1_Pin GPIO_PIN_8
#define LED1_GPIO_Port GPIOE
#define LED2_Pin GPIO_PIN_9
#define LED2_GPIO_Port GPIOE
#define LED3_Pin GPIO_PIN_10
#define LED3_GPIO_Port GPIOE
#define LED4_Pin GPIO_PIN_11
#define LED4_GPIO_Port GPIOE
#define KEY1_Pin GPIO_PIN_12
#define KEY1_GPIO_Port GPIOE
#define KEY1_EXTI_IRQn EXTI15_10_IRQn
#define KEY2_Pin GPIO_PIN_13
#define KEY2_GPIO_Port GPIOE
#define KEY2_EXTI_IRQn EXTI15_10_IRQn
#define KEY3_Pin GPIO_PIN_14
#define KEY3_GPIO_Port GPIOE
#define KEY3_EXTI_IRQn EXTI15_10_IRQn
#define KEY4_Pin GPIO_PIN_15
#define KEY4_GPIO_Port GPIOE
#define KEY4_EXTI_IRQn EXTI15_10_IRQn
#define ESP8266_RST_Pin GPIO_PIN_12
#define ESP8266_RST_GPIO_Port GPIOD
#define ESP8266_CH_PD_Pin GPIO_PIN_13
#define ESP8266_CH_PD_GPIO_Port GPIOD
#define ESP8266_GPIO2_Pin GPIO_PIN_14
#define ESP8266_GPIO2_GPIO_Port GPIOD
#define ESP8266_GPIO0_Pin GPIO_PIN_15
#define ESP8266_GPIO0_GPIO_Port GPIOD
#define RS485_DIR2_Pin GPIO_PIN_2
#define RS485_DIR2_GPIO_Port GPIOG
#define RS485_DIR1_Pin GPIO_PIN_4
#define RS485_DIR1_GPIO_Port GPIOG
#define NRF_CSN_Pin GPIO_PIN_7
#define NRF_CSN_GPIO_Port GPIOG
#define NRF_IRQ_Pin GPIO_PIN_8
#define NRF_IRQ_GPIO_Port GPIOG
#define FLASH_CE_Pin GPIO_PIN_10
#define FLASH_CE_GPIO_Port GPIOG
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
