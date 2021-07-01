/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
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

#include "fatfs.h"

uint8_t retSD;    /* Return value for SD */
char SDPath[4];   /* SD logical drive path */
FATFS SDFatFS;    /* File system object for SD logical drive */
FIL SDFile;       /* File object for SD */

/* USER CODE BEGIN Variables */

//FRESULT mount;
extern RTC_HandleTypeDef hrtc;

/* USER CODE END Variables */

void MX_FATFS_Init(void)
{
  /*## FatFS: Link the SD driver ###########################*/
  retSD = FATFS_LinkDriver(&SD_Driver, SDPath);

  /* USER CODE BEGIN Init */
  /* additional user code for init */
  //mount = f_mount( &SDFatFS, (const TCHAR *)&SDPath, 1 );
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
    //Функция вернет время, упакованное в величину типа DWORD. Битовые поля этого значения следующие:
    //bit31:25 Год (Year), начиная с 1980 (0..127)
    //bit24:21 Месяц (Month, 1..12)
    //bit20:16 День месяца (Day in month 1..31)
    //bit15:11 Час (Hour, 0..23)
    //bit10:5  Минута (Minute, 0..59)
    //bit4:0   Количество секунд (Second), поделенное пополам (0..29)

    uint32_t result;
    RTC_TimeTypeDef stime;
    RTC_DateTypeDef sdate;

    HAL_RTC_GetDate( &hrtc, &sdate, RTC_FORMAT_BIN );
    HAL_RTC_GetTime( &hrtc, &stime, RTC_FORMAT_BIN );

    result  = ( (uint32_t)sdate.Year ) << 25;
    result |= ( (uint32_t)sdate.Month ) << 21;
    result |= ( (uint32_t)sdate.Date ) << 16;
    result |= ( (uint32_t)stime.Hours ) << 11;
    result |= ( (uint32_t)stime.Minutes ) << 5;
    result |= ( (uint32_t)stime.Seconds / 2 );

    return result;
  /* USER CODE END get_fattime */
}

/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
