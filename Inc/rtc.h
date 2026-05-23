/**
  ******************************************************************************
  * File Name          : RTC.h
  * Description        : This file provides code for the configuration
  *                      of the RTC instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __rtc_H
#define __rtc_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "irrigation.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */
// 状态定义
typedef enum {
  CYCLE_IDLE = 0,      // 空闲状态（不在循环时间段内）
  CYCLE_ON,            // 阀门开启状态
  CYCLE_OFF,           // 阀门关闭状态
  CYCLE_FINISH         // 循环结束
} CycleState_TypeDef;


extern CycleState_TypeDef cycle_state;
extern RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_RTC_Init(void);


void GetRtcTime(void);
uint32_t Getnowsec(void);
// STM32 RTC值 (1-7) 转换为 week_hit 需要的索引 (0-6)
uint8_t stm32_weekday_to_index(uint8_t stm32_weekday);

uint8_t week_hit_t(uint8_t week_mask, uint8_t dayIndex0Sun);
void Set_Cycle_Blarm(void);


uint32_t MyRTC_GetUnixTimestamp(RTC_HandleTypeDef *hrtc);
/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ rtc_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
