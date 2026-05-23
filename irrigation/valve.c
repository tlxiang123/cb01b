#include "valve.h"
#include "main.h"   // GPIO_PIN_x / 你的 Pulse_60ms 声明通常也在这里或别处


// ====== 你项目的硬件动作映射（按你原来的逻辑） ======
#ifndef VALVE_OPEN_PULSE_PINS
#define VALVE_OPEN_PULSE_PINS   (GPIO_PIN_3 | GPIO_PIN_6)
#endif

#ifndef VALVE_CLOSE_PULSE_PINS
#define VALVE_CLOSE_PULSE_PINS  (GPIO_PIN_5 | GPIO_PIN_4)
#endif

// 你工程已有的函数（如果编译报错，说明你函数名不一致，改成你实际的）
static volatile valve_state_t s_valve_state = VALVE_CLOSE;

void Valve_Init(void)
{
    // 如需初始化IO、电源等可在此加入
    s_valve_state = VALVE_CLOSE;
}

void Valve_Open(void)
{
	DBG_UART2("Open");
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    Pulse_60ms(VALVE_OPEN_PULSE_PINS);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    s_valve_state = VALVE_OPEN;
}

void Valve_Close(void)
{
	DBG_UART2("Close");
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
    Pulse_60ms(VALVE_CLOSE_PULSE_PINS);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    s_valve_state = VALVE_CLOSE;
}

valve_state_t Valve_GetState(void)
{
    return s_valve_state;
}

uint8_t Valve_SetState(valve_state_t st)
{
    if (s_valve_state == st) return 0;
    if (st == VALVE_OPEN) Valve_Open();
    else Valve_Close();
    
    return 1;
}
