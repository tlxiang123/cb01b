#ifndef __VALVE_H__
#define __VALVE_H__

#include <stdint.h>
#include "stm32l0xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VALVE_CLOSE = 0,
    VALVE_OPEN  = 1
} valve_state_t;


static void Pulse_60ms(uint32_t pins)
{
    uint32_t tick = HAL_GetTick();
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
	HAL_Delay(900);
	
    HAL_GPIO_WritePin(GPIOA, pins, GPIO_PIN_SET);
	HAL_Delay(80);
	
    HAL_GPIO_WritePin(GPIOA, pins, GPIO_PIN_RESET); 
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
}


#define  switch_on  Pulse_60ms(GPIO_PIN_3 | GPIO_PIN_6);
#define  switch_off  Pulse_60ms(GPIO_PIN_5 | GPIO_PIN_4);


/**
 * @brief  初始化（如无特殊硬件初始化可留空）
 */
void Valve_Init(void);

/**
 * @brief  开阀（硬件动作）
 */
void Valve_Open(void);

/**
 * @brief  关阀（硬件动作）
 */
void Valve_Close(void);

/**
 * @brief  获取当前阀门状态（软件维护）
 */
valve_state_t Valve_GetState(void);

/**
 * @brief  设置阀门状态（带去抖/重复调用保护）
 * @return 1=状态发生变化 0=状态没变
 */
uint8_t Valve_SetState(valve_state_t st);

#ifdef __cplusplus
}
#endif

#endif
