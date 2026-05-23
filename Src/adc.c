/**
  ******************************************************************************
  * File Name          : ADC.c
  * Description        : This file provides code for the configuration
  *                      of the ADC instances.
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

/* Includes ------------------------------------------------------------------*/
#include "adc.h"
#include "usart.h"

/* USER CODE BEGIN 0 */
#include "bluetooth.h"

/* USER CODE END 0 */

ADC_HandleTypeDef hadc;

/* ADC init function */
void MX_ADC_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.OversamplingMode = DISABLE;
  hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.SamplingTime = ADC_SAMPLETIME_160CYCLES_5;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerFrequencyMode = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_ADCEx_Calibration_Start(&hadc, ADC_SINGLE_ENDED);
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspInit 0 */

  /* USER CODE END ADC1_MspInit 0 */
    /* ADC1 clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**ADC GPIO Configuration
    PB0     ------> ADC_IN8
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN ADC1_MspInit 1 */

  /* USER CODE END ADC1_MspInit 1 */
  }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspDeInit 0 */

  /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();

    /**ADC GPIO Configuration
    PB0     ------> ADC_IN8
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0);

  /* USER CODE BEGIN ADC1_MspDeInit 1 */

  /* USER CODE END ADC1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/* USER CODE BEGIN 1 */

// 定义ADC相关参数
#define ADC_MAX_VALUE        4095    // 12位ADC最大值
#define BATTERY_MAX_VOLTAGE  9.6f    // 电池最大电压9V
#define BATTERY_MIN_VOLTAGE  7.0f    // 电池最低有效电压(关键：设为7V，低于此值判定为无电池/电量0%)
#define R1                   10.0f  // 分压电阻147K
#define R2                   3.9f   // 分压电阻47K
#define VOLTAGE_REF          3.3f    // MCU参考电压3.3V

/**
 * @brief  读取ADC采样值（增加无效值过滤）
 * @retval ADC原始采样值（无效时返回0）
 */
uint32_t ADC_Get_Raw_Value(void)
{
  uint32_t adc_value = 0;
  uint32_t adc_temp = 0;
  
  // 启动ADC转换
  if(HAL_ADC_Start(&hadc) != HAL_OK)
  {
    Error_Handler();
    return 0;
  }
   
  // 等待转换完成
  if(HAL_ADC_PollForConversion(&hadc, 10) == HAL_OK)
  {
    // 读取ADC值
    adc_temp = HAL_ADC_GetValue(&hadc);
    adc_value = adc_temp;
  }
  // 停止ADC
  HAL_ADC_Stop(&hadc);
  
  return adc_value;
}

/**
 * @brief  读取ADC采样值并计算实际电压
 * @retval 实际电池电压值(V)（无电池时返回0）
 */
float ADC_Get_Battery_Voltage(void)
{
  uint32_t adc_raw = 0;
  uint32_t adc_avg = 0;
  float adc_voltage = 0.0f;
  float battery_voltage = 0.0f;

  // 多次采样取平均值，提高精度
  uint8_t i = 0;
  while (i < 100) {
    adc_raw = ADC_Get_Raw_Value();
	if (adc_raw == 0) continue;
	adc_avg += adc_raw;
	//adc_raw = ADC_Get_Raw_Value();
    
	  //DBG_UART2("\r\nadc_avg = %d\r\n", adc_raw);
	i++;
  }
  adc_avg = adc_avg / 100;
  //DBG_UART2("[adc_avg] = %d", adc_avg);
  // 如果ADC值为0，直接返回0V（无电池）
  if(adc_avg == 0)
  {
//    DBG_UART2("[ADC] raw_avg=0 => battery=0.0V (no battery?)\r\n");
    return 0.0f;
  }
  
  // 计算分压前的实际电池电压
  // 分压比计算
	float voltage_divider_ratio = (R1 + R2) / R2;

// ADC 测量电压
	 adc_voltage = (float)adc_avg * VOLTAGE_REF / ADC_MAX_VALUE;
	//DBG_UART2("[adc_voltage] = %f", adc_voltage);
// 实际电池电压
	 battery_voltage = adc_voltage * voltage_divider_ratio;
	//DBG_UART2("[battery_voltage] = %f", battery_voltage);
  
  // 电压值限幅，防止异常
  if(battery_voltage > BATTERY_MAX_VOLTAGE)
  {
    battery_voltage = BATTERY_MAX_VOLTAGE;
  }
  // ===== 串口2调试输出 =====
//  DBG_UART2("[ADC] raw_sum=%lu raw_avg=%lu, pin=%.3fV, batt=%.3fV\r\n",\
            (unsigned long)adc_avg,\
            (unsigned long)adc_raw,\
            adc_voltage,\
            battery_voltage);

  return battery_voltage;
}


uint8_t GetBatteryPercent(float voltage)
{
    // 根据实测数据点进行分段映射
    if (voltage >= 9.6f) return 100;
    if (voltage >= 9.0f) return 90;
    if (voltage >= 8.5f) return 75;
    if (voltage >= 8.0f) return 55;
    if (voltage >= 7.8f) return 40;
    if (voltage >= 7.6f) return 30;
    if (voltage >= 7.4f) return 23;
    if (voltage >= 7.3f) return 19;
    if (voltage >= 7.2f) return 15;
    if (voltage >= 7.1f) return 10;
    if (voltage >= 7.0f) return 5;
    return 0;
}

#define BAT_CAL_K   1.055  // 约 1.0104
extern uint8_t precharge_on;


 uint8_t ADC_Get_Battery_Percentage(void)
{

  float battery_voltage = ADC_Get_Battery_Voltage();
   
  battery_voltage *= BAT_CAL_K;  //电压修正

  uint8_t battery_percent = GetBatteryPercent(battery_voltage);

  // 无电池/电压低于阈值，直接返回0%
  if(battery_voltage <= BATTERY_MIN_VOLTAGE)
  {
    battery_percent = 0;
  }
 
  // 上报电池百分比
  unsigned char ret = mcu_dp_value_update(DPID_BATTERY_PERCENTAGE, battery_percent);
  //DBG_UART2("[\%u]  [%3fV]\r\n",(unsigned)battery_percent,\
			battery_voltage);
	//DBG_UART2("batt = %d", battery_percent);
  return battery_percent;
}

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
