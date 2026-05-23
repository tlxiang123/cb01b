/**
  ******************************************************************************
  * File Name          : USART.h
  * Description        : This file provides code for the configuration
  *                      of the USART instances.
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
#ifndef __usart_H
#define __usart_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "stdio.h"
/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
void UART1_SendByte(unsigned char data);

/************************** 4. ??1 ????????u8??? **************************/
void UART1_SendString(unsigned char *pbuf);


void UART2_SendString(const char *str);


#define DBG_UART2(...)               \
do {                                \
  char __buf[128] = {0};                  \
  snprintf(__buf, sizeof(__buf), __VA_ARGS__); \
  UART2_SendString(__buf);          \
} while(0)


static const char* ok_fail_str(unsigned char ret)
{
    return (ret == SUCCESS) ? "SUCCESS" : "ERROR";
}

static void dump_raw_hex_uart2(const unsigned char *buf, unsigned short len)
{
    unsigned short i;
    DBG_UART2("\r\n raw len = %d \r\n", (unsigned)len);
    for(i = 0; i < len; i++)
    {
        DBG_UART2("%02X ",buf[i]);
    }
}
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ usart_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
