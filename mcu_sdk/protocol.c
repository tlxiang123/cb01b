/****************************************Copyright (c)*************************
**                               Copyright (C) 2014-2020, Tuya Inc., All Rights Reserved
**
**                                 http://www.tuya.com
**
**--------------File Information-------------------------------------------------------

** file name :protocol.c
** description: send / report data processing functions
* * instructions for use

                  *******important, be sure to see it!********

* * 1. Users implement the function of sending / reporting data in this file
* * 2. The ID/TYPE and data processing functions of DP need to be implemented according to the actual definition.
* * 3. There is a # err prompt inside the function that requires the user to implement the code after starting some macro definitions. Delete the # err after completing the function.
**
**--------------Revision record---------------------------------------------------
** version: v1.0
** date : may 3, 2017 
description: Initial version
**

**version::v2.0
** date: March 23, 2020
** description: 
1. Added module unbinding interface support, command code 0x09.
2.Add rf RF test interface support, command code 0x0e.
3.Add record-based data reporting interface support,command code 0xe0.
4. Added access to real-time time API support,command code 0xe1.
5. Added support for modifying sleep mode state bluetooth broadcast interval,command code 0xe2.
6. Added support for turning off system clock,command code 0xe4.
7. Increase low power consumption to enable support,commadn code 0xe5.
8. Add dynamic password authentication interface support,command code 0xe6.
9. Added support for disconnecting Bluetooth connection,command code 0xe7.
10. Added support for querying MCU version number,command code 0xe8.
11. Added support for MCU to actively send version Numbers,command code 0xe9.
12. Add OTA upgrade request support,command code 0xea.
13. Add OTA update file information support,command 0xeb.
14. Add OTA upgrade file migration request support,command code 0xec.
15. Add OTA upgrade data support,command code 0xed.
16. Add OTA upgrade end support,command code 0xee.
17. Added support for MCU to acquire module version information,commadn code 0xa0.
18. Added support for resuming factory Settings notifications,command code 0xa1.
19. Add MCU OTA demo code.
20. Optimized bt_uart_service.
**
**-----------------------------------------------------------------------------
******************************************************************************/


#include "bluetooth.h"
#include "usart.h"
#include "time_keeper.h"
#include "valve.h"
#include "irrigation_debug.h"
#include "rtc.h"
#include "timer_manager.h"


/******************************************************************************
								Transplant notes:
1.MCU call bt_uart_service() in the main loop.
2: After the program is initialized normally, it is recommended not to close the serial port interrupt. If the interrupt must be turned off, the interrupt time must be short. Turning off the interrupt will cause the serial port data packet to be lost.
3: Do not call the report function in the interrupt/timer interrupt
******************************************************************************/


/******************************************************************************
							  Step 1: initialization
1: Include the header file "Bt.h" in files that need to be used with BT-related files
2: Call the BT_protocol_init () function of the McU_api.c file during MCU initialization
3: Fill the MCU serial byte transmission function into the protocol. C file uart_transmit_output function, and delete #error
4: Call the Uart_receive_input function in the file mcu_api. c from the MCU uart port receipt function, and pass in the received bytes as parameters
5: After the MCU enters the while loop, it calls the bT_uart_service () function in the mcu_api. c file
******************************************************************************/

/******************************************************************************
                        1:Comparison Table of sequence types of dp data points
		  * * this is automatically generated code. If there are any changes on the development platform, please download MCU_SDK** again.
******************************************************************************/
const DOWNLOAD_CMD_S download_cmd[] =
{
  {DPID_SWITCH, DP_TYPE_BOOL},
  {DPID_PERCENT_CONTROL, DP_TYPE_VALUE},
  {DPID_PERCENT_STATE, DP_TYPE_VALUE},
  {DPID_FAULT, DP_TYPE_BITMAP},
  {DPID_WATER_ONCE, DP_TYPE_VALUE},
  {DPID_WATER_TOTAL, DP_TYPE_VALUE},
  {DPID_BATTERY_PERCENTAGE, DP_TYPE_VALUE},
  {DPID_BATTERY_STATE, DP_TYPE_ENUM},
  {DPID_USE_TIME, DP_TYPE_VALUE},
  {DPID_WEATHER_DELAY, DP_TYPE_ENUM},
  {DPID_COUNTDOWN, DP_TYPE_VALUE},
  {DPID_WORK_STATE, DP_TYPE_ENUM},
  {DPID_SMART_WEATHER, DP_TYPE_ENUM},
  {DPID_WEATHER_SWITCH, DP_TYPE_BOOL},
  {DPID_USE_TIME_ONE, DP_TYPE_VALUE},
  {DPID_CYCLE_TIMING, DP_TYPE_RAW},
  {DPID_TIMER, DP_TYPE_RAW},
  {DPID_SENSOR_HUMIDITY, DP_TYPE_VALUE},
  {DPID_MAXHUM_SET, DP_TYPE_VALUE},
  {DPID_SWITCH_ENABLED, DP_TYPE_BOOL},
};




/******************************************************************************
						   2: uart port single byte sending function
Please fill the MCU serial port sending function into this function, and pass the received data into the serial port sending function as parameters
******************************************************************************/

/*****************************************************************************
Function name: uart_transmit_output
Function description: send data processing
Input parameters: value: uart port receives byte data
Return parameter: none
Instructions for use: please fill the MCU serial port sending function into this function, and pass the received data into the serial port sending function as parameters
*****************************************************************************/
void uart_transmit_output(unsigned char val)
{
 //#error "Please fill the MCU serial port sending function into this function and delete the line"
	UART1_SendByte(val);
/*
  //Example:
  extern void Uart_PutChar(unsigned char value);
  Uart_PutChar(value);	                                //uart port sending function
*/  
}
/******************************************************************************
						   Step 2: implement specific user functions
1:APP dispatch data processing
2: data reporting processing
******************************************************************************/

/******************************************************************************
							1: all data reporting and processing
The current function handles all data reporting (including dispatch / escalation and escalation only)
  Users are required to implement according to the actual situation:
  1: it is necessary to implement data points that can be sent / reported.
  2: only data points need to be reported.
This function must be called within MCU.
Users can also call this function to report all data.
******************************************************************************/

// automatically generate data reporting function.

/*****************************************************************************
Function name: all_data_update
Function description: upload all dp information of the system to achieve data synchronization between APP and muc
Input parameters: none
Return parameter: none
Instructions for use: this function needs to be called internally in SDK
		   MCU must implement the data reporting function within this function, including reporting only and downloadable hairstyle data.
*****************************************************************************/
void all_data_update(void)
{
  //#error "Please process all DP data here and delete the row when the processing is complete"
  // This code is automatically generated for the platform. Please modify each function according to the actual data
//    mcu_dp_bool_update(DPID_SWITCH,当前阀门开�?); //BOOL型数�?上报;
//    mcu_dp_value_update(DPID_PERCENT_CONTROL,当前出水量调�?); //VALUE型数�?上报;
//    mcu_dp_value_update(DPID_PERCENT_STATE,当前出水量状�?); //VALUE型数�?上报;
//    mcu_dp_fault_update(DPID_FAULT,当前故障上报); //故障型数�?上报;
//    mcu_dp_value_update(DPID_WATER_ONCE,当前单�?�水�?); //VALUE型数�?上报;
//    mcu_dp_value_update(DPID_WATER_TOTAL,当前当日耗水总量); //VALUE型数�?上报;
//    mcu_dp_value_update(DPID_BATTERY_PERCENTAGE,当前电池电量); //VALUE型数�?上报;
//    mcu_dp_enum_update(DPID_BATTERY_STATE,当前电池状�?); //枚举型数�?上报;
//    mcu_dp_value_update(DPID_USE_TIME,当前�?计使用时�?); //VALUE型数�?上报;
//    mcu_dp_enum_update(DPID_WEATHER_DELAY,当前天气延时); //枚举型数�?上报;
//    mcu_dp_value_update(DPID_COUNTDOWN,当前灌溉时长); //VALUE型数�?上报;
//    mcu_dp_enum_update(DPID_WORK_STATE,当前工作状�?); //枚举型数�?上报;
//    mcu_dp_enum_update(DPID_SMART_WEATHER,当前智能天气); //枚举型数�?上报;
//    mcu_dp_bool_update(DPID_WEATHER_SWITCH,当前智能天气开�?); //BOOL型数�?上报;
//    mcu_dp_value_update(DPID_USE_TIME_ONE,当前单�?�使用时�?); //VALUE型数�?上报;
//    mcu_dp_raw_update(DPID_CYCLE_TIMING,当前周期灌溉指针,当前周期灌溉数据长度); //RAW型数�?上报;
//    mcu_dp_raw_update(DPID_TIMER,当前�?通定时指�?,当前�?通定时数�?长度); //RAW型数�?上报;
//    mcu_dp_value_update(DPID_SENSOR_HUMIDITY,当前土壤湿度); //VALUE型数�?上报;
//    mcu_dp_value_update(DPID_MAXHUM_SET,当前湿度上限设置); //VALUE型数�?上报;
//    mcu_dp_bool_update(DPID_SWITCH_ENABLED,当前传感器�?�置); //BOOL型数�?上报;



}


/******************************************************************************
                                WARNING!!!    
							2. Report and process all data
Automatic code template function, the specific user to realize the data processing
******************************************************************************/

/*****************************************************************************
函数名称 : dp_download_switch_handle
功能描述 : 针�?�DPID_SWITCH的�?�理函数
输入参数 : value:数据源数�?
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : �?下发�?上报类型,需要在处理完数�?后上报�?�理结果至app
*****************************************************************************/
//static unsigned char dp_download_switch_handle(const unsigned char value[], unsigned short length)
//{
//    unsigned char ret;
//    unsigned char switch_1;

//    DBG_UART2("[DP] SWITCH downlink: len=%u", (unsigned)length);

//    switch_1 = mcu_get_dp_download_bool(value, length);
//    DBG_UART2("[DP] SWITCH parsed: %u (%s)", switch_1, switch_1 ? "ON" : "OFF");

//    if(switch_1 == 0) {
//        DBG_UART2("[DP] SWITCH action: valve OFF");
//        // bool off
//        switch_off;   // 你原来的动作
//    } else {
//        DBG_UART2("[DP] SWITCH action: valve ON");
//        // bool on
//        switch_on;    // 你原来的动作
//    }

//    ret = mcu_dp_bool_update(DPID_SWITCH, switch_1);
//    DBG_UART2("[DP] SWITCH report: %s (ret=%u)", ok_fail_str(ret), (unsigned)ret);

//    return (ret == SUCCESS) ? SUCCESS : ERROR;
//}

static unsigned char dp_download_switch_handle(const unsigned char value[], unsigned short length)
{
    unsigned char ret;
    unsigned char switch_1;
	


    switch_1 = mcu_get_dp_download_bool(value, length);

	irr_mode_t s_rt = Irrigation_GetMode();

    if (switch_1 == 0) {
        Irrigation_StopAll();                 // �? 关键：停止倒�?�时/定时/周期
    } else {
        Irrigation_OnManualCmd(1);            // �? 关键：交�? Irrigation 接�??
    }
	

    ret = mcu_dp_bool_update(DPID_SWITCH, switch_1);


    return (ret == SUCCESS) ? SUCCESS : ERROR;
}

/*****************************************************************************
函数名称 : dp_download_percent_control_handle
功能描述 : 针�?�DPID_PERCENT_CONTROL的�?�理函数
输入参数 : value:数据源数�?
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : �?下发�?上报类型,需要在处理完数�?后上报�?�理结果至app
*****************************************************************************/
static unsigned char dp_download_percent_control_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为VALUE
    unsigned char ret;
    unsigned long percent_control;
    
    percent_control = mcu_get_dp_download_value(value,length);
    /*
    //VALUE type data processing
    
    */
    
    //There should be a report after processing the DP
    ret = mcu_dp_value_update(DPID_PERCENT_CONTROL,percent_control);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_weather_delay_handle
功能描述 : 针�?�DPID_WEATHER_DELAY的�?�理函数
输入参数 : value:数据源数�?
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : �?下发�?上报类型,需要在处理完数�?后上报�?�理结果至app
*****************************************************************************/
//static unsigned char dp_download_weather_delay_handle(const unsigned char value[], unsigned short length)
//{
//    //示例:当前DP类型为ENUM
//    unsigned char ret;
//    unsigned char weather_delay;
//    
//    weather_delay = mcu_get_dp_download_enum(value,length);
//    switch(weather_delay) {
//        case 0:
//        break;
//        
//        case 1:
//        break;
//        
//        case 2:
//        break;
//        
//        case 3:
//        break;
//        
//        case 4:
//        break;
//        
//        case 5:
//        break;
//        
//        case 6:
//        break;
//        
//        case 7:
//        break;
//        
//        default:
//    
//        break;
//    }
//    
//    //There should be a report after processing the DP
//    ret = mcu_dp_enum_update(DPID_WEATHER_DELAY, weather_delay);
//    if(ret == SUCCESS)
//        return SUCCESS;
//    else
//        return ERROR;
//}
// 全局变量

static unsigned char dp_download_weather_delay_handle(const unsigned char value[], unsigned short length)
{
    unsigned char ret;
    unsigned char weather_delay;

//    DBG_UART2("[DP] WEATHER_DELAY downlink: len=%u", (unsigned)length);

    weather_delay = mcu_get_dp_download_enum(value, length);
//    DBG_UART2("[DP] WEATHER_DELAY parsed: %u", weather_delay);

    // 这里建�??你把 0..7 映射成含义打印出来（按你实际定义�?
    switch(weather_delay) {
        case 0: DBG_UART2("[DP] WEATHER_DELAY meaning: sel=0"); break;
        case 1: DBG_UART2("[DP] WEATHER_DELAY meaning: sel=1"); break;
        case 2: DBG_UART2("[DP] WEATHER_DELAY meaning: sel=2"); break;
        case 3: DBG_UART2("[DP] WEATHER_DELAY meaning: sel=3"); break;
        case 4: DBG_UART2("[DP] WEATHER_DELAY meaning: sel=4"); break;
        case 5: DBG_UART2("[DP] WEATHER_DELAY meaning: sel=5"); break;
        case 6: DBG_UART2("[DP] WEATHER_DELAY meaning: sel=6"); break;
        case 7: DBG_UART2("[DP] WEATHER_DELAY meaning: sel=7"); break;
        default:
            DBG_UART2("[DP] WEATHER_DELAY meaning: INVALID(%u)", weather_delay);
            break;
    }
	valve_takeover(IRR_SRC_NONE, 0, 0);
	SetValveDelay(weather_delay);

	//Irrigation_SetWeatherDelaySel(weather_delay);
//		DBG_UART2("[DP] WEATHER_DELAY apply: delay_sel=%u", weather_delay);
		
    ret = mcu_dp_enum_update(DPID_WEATHER_DELAY, weather_delay);
//    DBG_UART2("[DP] WEATHER_DELAY report: %s (ret=%u)", ok_fail_str(ret), (unsigned)ret);

    return (ret == SUCCESS) ? SUCCESS : ERROR;
}

/*****************************************************************************
函数名称 : dp_download_countdown_handle
功能描述 : 针�?�DPID_COUNTDOWN的�?�理函数
输入参数 : value:数据源数�?
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : �?下发�?上报类型,需要在处理完数�?后上报�?�理结果至app
*****************************************************************************/
//static unsigned char dp_download_countdown_handle(const unsigned char value[], unsigned short length)
//{
//    //示例:当前DP类型为VALUE
//    unsigned char ret;
//    unsigned long countdown;
//    
//    countdown = mcu_get_dp_download_value(value,length);
//    /*
//    //VALUE type data processing
//    
//    */
//    
//    //There should be a report after processing the DP
//    ret = mcu_dp_value_update(DPID_COUNTDOWN,countdown);
//    if(ret == SUCCESS)
//        return SUCCESS;
//    else
//        return ERROR;
//}

//static unsigned char dp_download_countdown_handle(const unsigned char value[], unsigned short length)
//{
//    unsigned char ret;
//    unsigned long countdown;

//    DBG_UART2("[DP] COUNTDOWN downlink: len=%u", (unsigned)length);

//    countdown = mcu_get_dp_download_value(value, length);
//    DBG_UART2("[DP] COUNTDOWN parsed: %lu", countdown);

//    // 这里强烈建�??打印单位（Tuya DP 常�?�是秒，也有人用分钟�?
//    // 你最好按�?�? DP 定义来写�?
//    // DBG_UART2("[DP] COUNTDOWN unit: seconds (assumed)");

//    ret = mcu_dp_value_update(DPID_COUNTDOWN, countdown);
//    DBG_UART2("[DP] COUNTDOWN report: %s (ret=%u)", ok_fail_str(ret), (unsigned)ret);

//    return (ret == SUCCESS) ? SUCCESS : ERROR;
//}
static unsigned char dp_download_countdown_handle(const unsigned char value[], unsigned short length)
{
    unsigned char ret;
    unsigned long countdown;

//    DBG_UART2("[DP] COUNTDOWN downlink: len=%u", (unsigned)length);

    countdown = mcu_get_dp_download_value(value, length);
//    DBG_UART2("[DP] COUNTDOWN parsed: %lu", countdown);

    // === apply to irrigation ===
    // 假�?? DP 单位=秒（你按实际 DP 定义改）
    uint32_t min = (countdown + 59) / 60;     // 向上取整，避�? 1�? => 0分钟
    if (min < 1) min = 1;
    if (min > 1440) min = 1440;

    Irrigation_SetOnceDurationMin((uint16_t)min);
    Irrigation_StartOnce();

//    DBG_UART2("[DP] COUNTDOWN apply: once_duration_min=%lu, start_once=1", (unsigned long)min);

    ret = mcu_dp_value_update(DPID_COUNTDOWN, countdown);
//    DBG_UART2("[DP] COUNTDOWN report: %s (ret=%u)", ok_fail_str(ret), (unsigned)ret);

    return (ret == SUCCESS) ? SUCCESS : ERROR;
}

/*****************************************************************************
函数名称 : dp_download_smart_weather_handle
功能描述 : 针�?�DPID_SMART_WEATHER的�?�理函数
输入参数 : value:数据源数�?
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : �?下发�?上报类型,需要在处理完数�?后上报�?�理结果至app
*****************************************************************************/
static unsigned char dp_download_smart_weather_handle(const unsigned char value[], unsigned short length)
{
    unsigned char ret;
    unsigned char smart_weather;

//    DBG_UART2("[DP] SMART_WEATHER downlink: len=%u", (unsigned)length);

    smart_weather = mcu_get_dp_download_enum(value, length);
//    DBG_UART2("[DP] SMART_WEATHER parsed: %u", smart_weather);

    switch(smart_weather) {
//        case 0: DBG_UART2("[DP] SMART_WEATHER meaning: OFF"); break;
//        case 1: DBG_UART2("[DP] SMART_WEATHER meaning: ON");  break;
//        default: DBG_UART2("[DP] SMART_WEATHER meaning: INVALID(%u)", smart_weather); break;
    }

		
		Irrigation_SetWeatherRainSnow(smart_weather ? 1 : 0);
//		DBG_UART2("[DP] SMART_WEATHER apply: is_rain_snow=%u", smart_weather ? 1 : 0);

    ret = mcu_dp_enum_update(DPID_SMART_WEATHER, smart_weather);
//    DBG_UART2("[DP] SMART_WEATHER report: %s (ret=%u)", ok_fail_str(ret), (unsigned)ret);

    return (ret == SUCCESS) ? SUCCESS : ERROR;
}

/*****************************************************************************
函数名称 : dp_download_weather_switch_handle
功能描述 : 针�?�DPID_WEATHER_SWITCH的�?�理函数
输入参数 : value:数据源数�?
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : �?下发�?上报类型,需要在处理完数�?后上报�?�理结果至app
*****************************************************************************/
static unsigned char dp_download_weather_switch_handle(const unsigned char value[], unsigned short length)
{
    unsigned char ret;
    unsigned char weather_switch;

//    DBG_UART2("[DP] WEATHER_SWITCH downlink: len=%u", (unsigned)length);

    weather_switch = mcu_get_dp_download_bool(value, length);
//    DBG_UART2("[DP] WEATHER_SWITCH parsed: %u (%s)", weather_switch, weather_switch ? "ON" : "OFF");

    if(weather_switch == 0) {
//        DBG_UART2("[DP] WEATHER_SWITCH action: OFF");
        // TODO: 这里写你关闭智能天气的动�?
    } else {
//        DBG_UART2("[DP] WEATHER_SWITCH action: ON");
        // TODO: 这里写你开�?智能天气的动�?
    }

		Irrigation_SetWeatherEnable(weather_switch);
//		DBG_UART2("[DP] WEATHER_SWITCH apply: weather_enable=%u", weather_switch);
		
		
    ret = mcu_dp_bool_update(DPID_WEATHER_SWITCH, weather_switch);
//    DBG_UART2("[DP] WEATHER_SWITCH report: %s (ret=%u)", ok_fail_str(ret), (unsigned)ret);

    return (ret == SUCCESS) ? SUCCESS : ERROR;
}

/*****************************************************************************
函数名称 : dp_download_cycle_timing_handle
功能描述 : 针�?�DPID_CYCLE_TIMING的�?�理函数
输入参数 : value:数据源数�?
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : �?下发�?上报类型,需要在处理完数�?后上报�?�理结果至app
*****************************************************************************/
static void cycle_weekmask_print(uint8_t week_mask)
{
//    DBG_UART2("[DP][CYCLE] week_mask=0x%02X", week_mask);
 //   DBG_UART2("[DP][CYCLE] week_mask bin=%c%c%c%c%c%c%c%c",\
//              (week_mask & 0x80) ? '1':'0',\
//              (week_mask & 0x40) ? '1':'0',\
//              (week_mask & 0x20) ? '1':'0',\
//              (week_mask & 0x10) ? '1':'0',\
//              (week_mask & 0x08) ? '1':'0',\
//              (week_mask & 0x04) ? '1':'0',\
//              (week_mask & 0x02) ? '1':'0',\
//              (week_mask & 0x01) ? '1':'0');

//    if (week_mask == 0x00) {
//        DBG_UART2("[DP][CYCLE] week: (none selected)");
//        return;
//    }

//    // �?打印你已经用样本�?认过的，不乱猜其它天
//    if (week_mask & 0x01) DBG_UART2("[DP][CYCLE] week_hit: 周日 (0x01)");
//    if (week_mask & 0x04) DBG_UART2("[DP][CYCLE] week_hit: 周一 (0x04)");
//    if (week_mask & 0x20) DBG_UART2("[DP][CYCLE] week_hit: 周五 (0x20)");
//    if ((week_mask & 0x7F) == 0x7F) DBG_UART2("[DP][CYCLE] week_hit: 周日~周六全�? (0x7F)");
}

static void cycle_print_parsed(uint16_t start_min, uint16_t end_min,
                               uint16_t water_min, uint16_t soak_min,
                               uint8_t percent, uint8_t week_mask)
{
//    uint16_t sh = (uint16_t)(start_min / 60u);
//    uint16_t sm = (uint16_t)(start_min % 60u);
//    uint16_t eh = (uint16_t)(end_min   / 60u);
//    uint16_t em = (uint16_t)(end_min   % 60u);

//    DBG_UART2("[DP][CYCLE] parsed window: %02u:%02u -> %02u:%02u (start_min=%u end_min=%u)",
//              (unsigned)sh,(unsigned)sm,(unsigned)eh,(unsigned)em,
//              (unsigned)start_min,(unsigned)end_min);

//    if (start_min <= end_min) DBG_UART2("[DP][CYCLE] window type: same-day");
//    else                      DBG_UART2("[DP][CYCLE] window type: cross-midnight");

//    DBG_UART2("[DP][CYCLE] parsed water_min=%u soak_min=%u percent=%u%%",
//              (unsigned)water_min, (unsigned)soak_min, (unsigned)percent);

    cycle_weekmask_print(week_mask);
}
uint8_t last_date = 0; //一天执行一�?
static unsigned char dp_download_cycle_timing_handle(const unsigned char value[], unsigned short length)
{
    unsigned char count = 0;;
    unsigned char ret;
	uint8_t tmp_val[32] = {0};

	
	switch (value[1]) {
		case 0x02:	//�?�?
			Cycle_ModifyFromServer(value);
			break;
		case 0x03:	//新�??
			Cycle_Store(value);
			break;
		case 0x04:	//删除
            //valve_takeover(IRR_SRC_NONE, 0, 0);
			Cycle_Delete(value[2]);
			break;
		default:
			break;
	}
	
    // 下发后必须上报，否则 APP 认为失败
	my_memset(tmp_val, 0, sizeof(tmp_val));
	count = Cycle_GetList(tmp_val);
    ret = mcu_dp_raw_update(DPID_CYCLE_TIMING, tmp_val, count);
    //DBG_UART2("[DP] CYCLE_TIMING report: %s (ret=%u)", ok_fail_str(ret), (unsigned)ret);
	Timer_SetAlarmByNearest();
    return (ret == SUCCESS) ? SUCCESS : ERROR;
}

/*****************************************************************************
函数名称 : dp_download_timer_handle
功能描述 : 针�?�DPID_TIMER的�?�理函数
输入参数 : value:数据源数�?
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : �?下发�?上报类型,需要在处理完数�?后上报�?�理结果至app
*****************************************************************************/
static void timer_weekmask_print(uint8_t week_mask)
{
    // �?前你已确认：周一=0x04、周�?=0x20；周�?=0x01（你样本里出现很多）
//    DBG_UART2("[DP][TIMER] week_mask=0x%02X", week_mask);
//    DBG_UART2("[DP][TIMER] week_mask bin=%c%c%c%c%c%c%c%c",
//              (week_mask & 0x80) ? '1':'0',
//              (week_mask & 0x40) ? '1':'0',
//              (week_mask & 0x20) ? '1':'0',
//              (week_mask & 0x10) ? '1':'0',
//              (week_mask & 0x08) ? '1':'0',
//              (week_mask & 0x04) ? '1':'0',
//              (week_mask & 0x02) ? '1':'0',
//              (week_mask & 0x01) ? '1':'0');

//    if (week_mask & 0x01) DBG_UART2("[DP][TIMER] week_hit: 周日 (0x01)");
//    if (week_mask & 0x04) DBG_UART2("[DP][TIMER] week_hit: 周一 (0x04)");
//    if (week_mask & 0x20) DBG_UART2("[DP][TIMER] week_hit: 周五 (0x20)");
//    if ((week_mask & 0x7F) == 0x7F) DBG_UART2("[DP][TIMER] week_hit: 周日~周六全�? (0x7F)");
}
void DeleteTimerByIndex(uint8_t index)
{
    //DBG_UART2("[TIMER] Deleting timer index=%u", index);
	//Irrigation_OnlyDeleteTimer();
	Timer_Delete(index);
}

static unsigned char dp_download_timer_handle(const unsigned char value[], unsigned short length)
{
    unsigned char ret;
//    DBG_UART2("[DP] TIMER downlink: RAW len=%u", (unsigned)length);
    //dump_raw_hex_uart2(value, length);
		
		// ===== 新�?�：处理删除指令 =====
    if (length == 3 && value[1] == 0x04) {
        uint8_t timer_index = value[2];
//        DBG_UART2("[DP][TIMER] DELETE command: index=%u", timer_index);
        //valve_takeover(IRR_SRC_NONE, 0, 0);
        // 执�?�删除操�?
        DeleteTimer(timer_index);  // 您需要实现这�?函数
        //return SUCCESS;
    }
	else if (length == 14)
    {
		ret = Timer_Store(value); //存储一�?定时�?
    }
    else if (length == 15)
    {
		uint8_t tmp_val[32] = {0};
		my_memcpy(tmp_val, value, 13);
		my_memcpy(&tmp_val[1], &value[2], 12);
		ret = Timer_Modify(value[2], tmp_val); //存储一�?定时�?
    }
    else
    {
		
    }
	uint8_t trans_buf[8 * 13] = {0};
	uint8_t count = 0;
	count = Timer_GetList(trans_buf);
	trans_buf[count] = 1;
	mcu_dp_raw_update(DPID_TIMER, trans_buf, count + 1);
	Timer_SetAlarmByNearest();
    return (ret == SUCCESS) ? SUCCESS : ERROR;
}


/*****************************************************************************
函数名称 : dp_download_maxhum_set_handle
功能描述 : 针�?�DPID_MAXHUM_SET的�?�理函数
输入参数 : value:数据源数�?
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : �?下发�?上报类型,需要在处理完数�?后上报�?�理结果至app
*****************************************************************************/
static unsigned char dp_download_maxhum_set_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为VALUE
    unsigned char ret;
    unsigned long maxhum_set;
    
    maxhum_set = mcu_get_dp_download_value(value,length);
    /*
    //VALUE type data processing
    
    */
    
    //There should be a report after processing the DP
    ret = mcu_dp_value_update(DPID_MAXHUM_SET,maxhum_set);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}
/*****************************************************************************
函数名称 : dp_download_switch_enabled_handle
功能描述 : 针�?�DPID_SWITCH_ENABLED的�?�理函数
输入参数 : value:数据源数�?
        : length:数据长度
返回参数 : 成功返回:SUCCESS/失败返回:ERROR
使用说明 : �?下发�?上报类型,需要在处理完数�?后上报�?�理结果至app
*****************************************************************************/
extern uint8_t rain_on;
static unsigned char dp_download_switch_enabled_handle(const unsigned char value[], unsigned short length)
{
    //示例:当前DP类型为BOOL
    unsigned char ret;
    //0:off/1:on
    unsigned char switch_enabled;
    
    switch_enabled = mcu_get_dp_download_bool(value,length);
    if(switch_enabled == 0) {
        DBG_UART2("2222");
		rain_on = 0;
    }else {
		rain_on = 1;
         DBG_UART2("1111");
    }
  
    //There should be a report after processing the DP
    ret = mcu_dp_bool_update(DPID_SWITCH_ENABLED,switch_enabled);
    if(ret == SUCCESS)
        return SUCCESS;
    else
        return ERROR;
}


/******************************************************************************
                                WARNING!!!                     
This code is SDK internal call, please implement the function internal data according to the actual DP data
******************************************************************************/

/******************************************************************************
                                WARNING!!!                     
The following function users do not modify!!
******************************************************************************/

/*****************************************************************************
Function name: DP_download_handle
Function description: DP release processing function
Input parameter: DPID :DP serial number
           value:Dp data buffer address
           length:dp data length
Return parameters:SUCCESS/ERRO
Instructions: This function cannot be modified by the user
*****************************************************************************/
unsigned char dp_download_handle(unsigned char dpid,const unsigned char value[], unsigned short length)
{
  /*********************************
  The current function handles the call to issue/report data
  The specific function needs to implement the data processing
  The processing results need to be fed back to the APP side for completion, otherwise the APP will think that the delivery fails
  ***********************************/
  unsigned char ret;
  
  switch(dpid)
  {
        case DPID_SWITCH:
            //阀门开关�?�理函数
            ret = dp_download_switch_handle(value,length);
        break;
        case DPID_PERCENT_CONTROL:
            //出水量调节�?�理函数
            ret = dp_download_percent_control_handle(value,length);
        break;
        case DPID_WEATHER_DELAY:
            //天气延时处理函数
            ret = dp_download_weather_delay_handle(value,length);
        break;
        case DPID_COUNTDOWN:
            //灌溉时长处理函数
            ret = dp_download_countdown_handle(value,length);
        break;
        case DPID_SMART_WEATHER:
            //智能天气处理函数
            ret = dp_download_smart_weather_handle(value,length);
        break;
        case DPID_WEATHER_SWITCH:
            //智能天气开关�?�理函数
            ret = dp_download_weather_switch_handle(value,length);
        break;
        case DPID_CYCLE_TIMING:
            //周期灌溉处理函数
            ret = dp_download_cycle_timing_handle(value,length);
        break;
        case DPID_TIMER:
            //�?通定时�?�理函数
            ret = dp_download_timer_handle(value,length);
        break;
        case DPID_MAXHUM_SET:
            //湿度上限设置处理函数
            ret = dp_download_maxhum_set_handle(value,length);
        break;
        case DPID_SWITCH_ENABLED:
            //传感器�?�置处理函数
            ret = dp_download_switch_enabled_handle(value,length);
        break;


  default:
    break;
  }
  return ret;
}
/*****************************************************************************
Function name: get_download_cmd_total
Function description: Gets the sum of all DP commands
Input parameters: None
Return parameter: issue the sum of commands
Instructions: This function cannot be modified by the user
*****************************************************************************/
unsigned char get_download_cmd_total(void)
{
  return(sizeof(download_cmd) / sizeof(download_cmd[0]));
}

//////////////////////////////////The current version of MCU SDK has a new support protocol interface over the previous version////////////////////
#ifdef TUYA_BCI_UART_COMMON_UNBOUND_REQ 
/*****************************************************************************
Function name: bt_unbound_req
Function description: Send the unbind request to the module, and the module will unbind the Bluetooth connection after receiving the instruction
Input parameters: None
Return parameter: none
Instructions: The MCU calls for active untying
*****************************************************************************/
void bt_unbound_req(void)
{
	bt_uart_write_frame(TUYA_BCI_UART_COMMON_UNBOUND_REQ,0);
}
#endif

#ifdef TUYA_BCI_UART_COMMON_RF_TEST 
/*****************************************************************************
Function name: bt_rf_test_req
Function description: transmit frequency test request to the module
Input parameters: None
Return parameter: none
Instructions for use:
*****************************************************************************/
void bt_rf_test_req(void)
{
	bt_uart_write_frame(TUYA_BCI_UART_COMMON_RF_TEST,0);
}
/*****************************************************************************
Function name: bt_rf_test_result
Function description: Bluetooth RF test feedback
Input parameter: Result: Bluetooth RF test result;0: failure /1: success
		   Rssi: Successful test indicates that the Bluetooth signal strength/test failure value is meaningless
Return parameter: none
Instructions: The MCU needs to improve the function itself
*****************************************************************************/
void bt_rf_test_result(unsigned char result,signed char rssi)
{
  //#error "Please improve the function by yourself and delete the line after completion"
  if(result == 0)
  {
	// The test failed
  }
  else
  {
	// The test was successful
	// RSSI is the signal strength, which is generally greater than -70dbM and within the normal range of Bluetooth signals
  }
  
}
#endif

#ifdef TUYA_BCI_UART_COMMON_SEND_STORAGE_TYPE 
/*****************************************************************************
Function name: bt_send_recordable_DP_data
Function description: report the recorded data
Input parameters: Type-1: Bluetooth module built-in time report -2: original data only report, no time -3: MCU built-in time report
		Dpid: former datapoint serial number
		Dptype: Corresponds to a datapoint specific data type on the open platform
		value:
		len:
Return parameter: none
Instructions: the MCU needs to improve the function itself
	It is recommended to use the cache queue. All data to be sent to the module should be put into the MCU cache queue, and the next data should be reported after one has been reported successfully. The recorded data should ensure that each data has been reported successfully
*****************************************************************************/
void bt_send_recordable_dp_data(unsigned char snedType,unsigned char dpid,unsigned char dpType, unsigned char val[],unsigned short len)
{
	//#error "Please improve the function by yourself and delete the line after completion"
	if(snedType==0x01)//Format 1, Bluetooth module self-report time
	{

	}
	else if(snedType==0x02)//Format 2, report only the original data, no time (Note: Telink docking platform does not support this format)
	{

	}
	else if(snedType==0x03)//Format 3, MCU own time report
	{

	}
}
/*****************************************************************************
Function name: bt_send_recordable_dp_data_result
Function description: report the recorded data
Input parameter :result: 0 storage success, 1 storage failure
Return parameter: none
Instructions: the MCU needs to improve the function itself
*****************************************************************************/
void bt_send_recordable_dp_data_result(unsigned char result)
{
	//#error "Please improve the function by yourself and delete the line after completion"
}
#ifdef TUYA_BCI_UART_COMMON_SEND_TIME_SYNC_TYPE 
/*****************************************************************************
Function name: bt_send_time_sync_req
Function description: Send time synchronization request to module
Input parameter: sync_time_type
0x00- Gets 7 bytes of time time type +2
Byte time zone information
0x01- Gets 13 bytes of MS level Unix time
+ 2-byte time zone information
0x02-get 7 byte time type + 2 Byte time zone information

Return parameter: none
*****************************************************************************/
void bt_send_time_sync_req(unsigned char sync_time_type)
{
	unsigned short length = 0;
  
  	length = set_bt_uart_byte(length,sync_time_type);
  	
	bt_uart_write_frame(TUYA_BCI_UART_COMMON_SEND_TIME_SYNC_TYPE,length);
}
/*****************************************************************************
Function name: bt_time_sync_result
Function description: send the result of time synchronization to the module
Input parameters: result synchronization result 0 successful, other failed
		sync_time_type :time format
		Bt_time: Custom time (valid if time format 0 or 1)
		Time_zone_100: time zone
		Time_stamp_ms: timestamp (valid if it is in time format 1)
Return parameter: none
Instructions: MCU needs to improve the function on its own.
*****************************************************************************/
//void bt_time_sync_result(unsigned char result,unsigned char sync_time_type,bt_time_struct_data_t bt_time,unsigned short time_zone_100,unsigned long time_stamp_ms)
//{
//	//#error "Please improve the function by yourself and delete the line after completion"
//	if(result == 0x00)
//	{
//		// synchronization time is successful
//		if(sync_time_type==0x00||sync_time_type==0x02)
//		{
//			// populate the data of custom time format in bt_time into the mcu clock system
//			//time_zone_100
//		}
//		else if(sync_time_type==0x01)
//		{
//			// populate the timestamp in time_stamp_ms into the mcu clock system
//			//time_zone_100
//		}
//	}
//	else
//	{
//		// synchronization time failed
//	}
//}

volatile uint8_t g_time_ok = 0;   // 1=已经拿到�?信时�?

static uint8_t is_time_reasonable_custom(const bt_time_struct_data_t *t)
{
    if (t->nYear < 2020 || t->nYear > 2050) return 0;
    if (t->nMonth < 1 || t->nMonth > 12) return 0;
    if (t->nDay   < 1 || t->nDay   > 31) return 0;
    if (t->nHour  > 23) return 0;
    if (t->nMin   > 59) return 0;
    if (t->nSec   > 59) return 0;
    return 1;
}

void bt_time_sync_result(unsigned char result,
                         unsigned char sync_time_type,
                         bt_time_struct_data_t bt_time,
                         unsigned short time_zone_100,
                         unsigned long time_stamp_ms)
{
    (void)time_stamp_ms; // 0x02不用�?

    // �?要失败：时间无效 + 继续请求
    if (result != 0x00) {
        g_time_ok = 0;
        TimeKeeper_Invalidate();
        TimeKeeper_MarkNeedSync();
        //DBG_UART2("[TS] sync failed: result=%u type=0x%02X", (unsigned)result, (unsigned)sync_time_type);
        return;
    }

    // 你当前用的就�? 0x02：强制只接受 0x02（也�?以兼�?0x00，但建�??先锁�?0x02排查�?
    if (sync_time_type != 0x02) {
        // 不清 need_sync！�?�主�?�?继续请求0x02
        g_time_ok = 0;
        TimeKeeper_MarkNeedSync();
//        DBG_UART2("[TS] ignore unexpected type=0x%02X (expect 0x02)", (unsigned)sync_time_type);
        return;
    }

    // 合理性过滤：挡住 2070 这�?�脏�?/默�?��?
    if (!is_time_reasonable_custom(&bt_time)) {
        g_time_ok = 0;
//        DBG_UART2("[TS] unreasonable time ignore: %u-%u-%u %u:%u:%u",\\
                  bt_time.nYear, bt_time.nMonth, bt_time.nDay,\
                  bt_time.nHour, bt_time.nMin, bt_time.nSec);
        TimeKeeper_Invalidate();
        TimeKeeper_MarkNeedSync();
        return;
    }

    // 写入TimeKeeper
    TimeKeeper_OnTimeSyncCustom(&bt_time, time_zone_100);

    // 写入后再二�?�确认（防�?�内部没置valid、或解析异常�?
    if (TimeKeeper_IsValid()) {
        g_time_ok = 1;
        TimeKeeper_ClearNeedSync();
		
//        DBG_UART2("[TS] time ok (type=0x02) tz=%u", (unsigned)time_zone_100);
    } else {
        g_time_ok = 0;
        TimeKeeper_MarkNeedSync();
//        DBG_UART2("[TS] write done but TimeKeeper invalid -> keep requesting");
    }
}



#endif
#endif

#ifdef TUYA_BCI_UART_COMMON_MODIFY_ADV_INTERVAL
/*****************************************************************************
Function name: bt_modify_adv_interval_req
Function description: send a request to the module to modify the broadcast interval of the module at low power consumption
Input parameter: value * 100ms equals the broadcast interval, value (0-20 to be modified)
Return parameter: none
Instructions for use:
*****************************************************************************/
void bt_modify_adv_interval_req(unsigned char val)
{
	unsigned short length = 0;
  	length = set_bt_uart_byte(length,val);
  	
	bt_uart_write_frame(TUYA_BCI_UART_COMMON_MODIFY_ADV_INTERVAL,length);
}
/*****************************************************************************
Function name: bt_modify_adv_interval_result
Function description:Processing the result of modifying the broadcast interval
Input parameters: result synchronization result 0 successful, other failed

Return parameter: none
Instructions: MCU needs to improve the function on its own.
*****************************************************************************/
void bt_modify_adv_interval_result(unsigned char result)
{
	//#error "Please improve the function by yourself and delete the line after completion"
	if(result == 0x00)
	{
		//success

	}
	else
	{
		//failed
	}
}
#endif

#ifdef TUYA_BCI_UART_COMMON_TURNOFF_SYSTEM_TIME
/*****************************************************************************
Function name: bt_close_timer_req
Function description: send a request to the module to turn off the system clock (currently available on telink platform only)
Input parameters: value 0 off, 1 on
Return parameter: none
Instructions for use:
*****************************************************************************/
void bt_close_timer_req(unsigned char val)
{
	unsigned short length = 0;
  	length = set_bt_uart_byte(length,val);
  	
	bt_uart_write_frame(TUYA_BCI_UART_COMMON_TURNOFF_SYSTEM_TIME,length);
}
/*****************************************************************************
Function name: bt_close_timer_result
Function description: processing result
Input parameters: 0 successful, other failed

Return parameter: none
Instructions: MCU needs to improve the function on its own.
*****************************************************************************/
void bt_close_timer_result(unsigned char result)
{
	//#error "Please improve the function by yourself and delete the line after completion"
	if(result == 0x00)
	{
		//success

	}
	else
	{
		//failed
	}
}
#endif

#ifdef TUYA_BCI_UART_COMMON_ENANBLE_LOWER_POWER
/*****************************************************************************
Function name: bt_enable_lowpoer_req
Function description: send a request to enable low power consumption to the module (currently only applicable to telink platform)
Input parameters: value 0 off, 1 on
Return parameter: none
*****************************************************************************/
void bt_enable_lowpoer_req(unsigned char val)
{
	unsigned short length = 0;
  
  	length = set_bt_uart_byte(length,val);
  	
	bt_uart_write_frame(TUYA_BCI_UART_COMMON_ENANBLE_LOWER_POWER,length);
}
/*****************************************************************************
Function name: bt_enable_lowpoer_result
Function description: processing result
Input parameters: 0 successful, other failed

Return parameter: none
Instructions: MCU needs to improve the function on its own.
*****************************************************************************/

void bt_enable_lowpoer_result(unsigned char result)
{
	//#error "Please improve the function by yourself and delete the line after completion"
	if(result == 0x00)
	{
		//success

	}
	else
	{
		//failed
	}
}
#endif

#ifdef TUYA_BCI_UART_COMMON_SEND_ONE_TIME_PASSWORD_TOKEN
/*****************************************************************************
Function name: bt_send_one_time_password_token
Function description: dynamic password check
Input parameters dynamic password entered on the: value cmcu side, len 8
Return parameter: none
Instructions for use: it is used to lock the universal serial port docking dynamic password function.
*****************************************************************************/
unsigned char bt_send_one_time_password_token(unsigned char val[],unsigned char len)
{
	unsigned short length = 0;
 	if(len!=8)return 0;
 	
  	length = set_bt_uart_buffer(length,val,8);
  	
	bt_uart_write_frame(TUYA_BCI_UART_COMMON_SEND_ONE_TIME_PASSWORD_TOKEN,length);
	return 0;
}
/*****************************************************************************
Function name:bt_send_one_time_password_token_result
Function description: get the result of one-time dynamic password matching from the module
Input parameters:0x00 password check passed, 0x01 password check failed
Return parameter: none
Instructions: MCU needs to improve the function on its own.
*****************************************************************************/
void bt_send_one_time_password_token_result(unsigned char result)
{
	//#error "Please improve the function by yourself and delete the line after completion"
	if(result == 0x00)
	{
		// password verification passed

	}
	else
	{
		//Password check failed.
	}
}
#endif

#ifdef TUYA_BCI_UART_COMMON_ACTIVE_DISCONNECT
/*****************************************************************************
Function name: bt_disconnect_req
Function description: send a request to disconnect the Bluetooth connection to the module
Input parameters: value 0 off, 1 on
Return parameter: none
Instructions for use:
*****************************************************************************/
void bt_disconnect_req(void)
{
	bt_uart_write_frame(TUYA_BCI_UART_COMMON_ACTIVE_DISCONNECT,0);
}
/*****************************************************************************
Function name: bt_disconnect_result
Function description: receive the result that the module is disconnected from Bluetooth
Input parameters: result result 0 successful, other failed

Return parameter: none
Instructions: MCU needs to improve the function on its own.
*****************************************************************************/
extern uint8_t is_disc;
void bt_disconnect_result(unsigned char result)
{
	//#error "Please improve the function by yourself and delete the line after completion"
	if(result == 0x00)
	{
		//success
		is_disc = 1;
	}
	else
	{
		is_disc = 0;
		//failed
	}
}
#endif

#ifdef TUYA_BCI_UART_COMMON_MCU_SEND_VERSION
/*****************************************************************************
Function name: bt_send_mcu_ver
Function description: send the MCU version number to the module actively, mainly in order that the module can obtain the MCU version information in a more timely manner.

Return parameter: none
Instructions: MCU can be called once after initialization of serial port.
*****************************************************************************/
void bt_send_mcu_ver(void)
{
	unsigned short length = 0;
	unsigned char version_buf[6] = {0};
	unsigned int mcu_fw_version = MCU_APP_VER_NUM;
	unsigned int mcu_hd_version = MCU_HARD_VER_NUM;
	
	version_buf[0] = (mcu_fw_version>>16)&0xff;
	version_buf[1] = (mcu_fw_version>>8)&0xff;
	version_buf[2] = (mcu_fw_version>>0)&0xff;
	version_buf[3] = (mcu_hd_version>>16)&0xff;
	version_buf[4] = (mcu_hd_version>>8)&0xff;
	version_buf[5] = (mcu_hd_version>>0)&0xff;

	length = set_bt_uart_buffer(length,version_buf,6);

	bt_uart_write_frame(TUYA_BCI_UART_COMMON_MCU_SEND_VERSION,length);
}
#endif
#ifdef TUYA_BCI_UART_COMMON_FACTOR_RESET_NOTIFY
/*****************************************************************************
Function name: bt_factor_reset_notify
Function description: notification sent to mcu after the module resumes factory settings

Return parameter: none
Instructions for use: MCU can complete the operation of restoring factory settings with MCU here.
*****************************************************************************/
void bt_factor_reset_notify(void)
{
	//#error "Please improve the function by yourself and delete the line after completion"
}
#endif

