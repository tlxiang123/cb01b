#include "irrigation.h"
#include "valve.h"
#include "time_keeper.h"
#include <string.h>
#include <stdio.h>
#include "protocol.h"
#include "usart.h"
#include "rtc.h"
#include "mcu_api.h"
#include "adc.h"
#include "timer_manager.h"






// ---- irrigation_cfg.c 的内部 getter ----
extern uint16_t _IrrCfg_GetOnceDurationMin(void);
extern uint8_t  _IrrCfg_PopOnceStartCmd(void);
extern void     _IrrCfg_GetTimer(irr_timer_cfg_t *out);
//extern void     _IrrCfg_GetCycle(irr_cycle_cfg_t *out);
extern void     _IrrCfg_GetWeather(irr_weather_cfg_t *o);
extern void     _IrrCfg_SetWeatherBlockUntil(uint32_t ts);

static uint32_t Starttimesec = 0;
static uint32_t Endtimesec = 0;
// ---- 内部运行态 ----
static irr_runtime_t s_rt = {0};

// ---- 手动命令 latch（来自按键/APP开关）----
static uint8_t s_manual_has_cmd = 0;
static uint8_t s_manual_on = 0;
uint8_t rain_on = 1;
// ---- 防重复触发（普通定时/周期定时每天只触发一次“启动”）----
static uint32_t s_last_timer_fire_daykey = 0;
static uint32_t s_last_cycle_enter_daykey = 0;

// ---- 周期灌溉相位 ----
typedef enum { CYC_PHASE_WATER = 0, CYC_PHASE_SOAK = 1 } cyc_phase_t;
static cyc_phase_t s_cyc_phase = CYC_PHASE_WATER;
void GetCurrentSta(irr_runtime_t *out) {*out = s_rt;}
 
// ------------------ 小工具 ------------------
static uint8_t week_hit(uint8_t week_mask, uint8_t dayIndex0Sun)
{
    if (dayIndex0Sun > 6) return 0;
    return (week_mask & (1u << dayIndex0Sun)) ? 1 : 0;
}

static uint16_t hm_to_min(uint8_t h, uint8_t m)
{
    return (uint16_t)h * 60u + (uint16_t)m;
}

// daykey：用于“同一天只触发一次”
// 你如果有更标准的日期编码也行，这里用 yyyymmdd 压成 uint32
static uint32_t make_daykey(const irr_time_t *t)
{
    return (uint32_t)t->nYear * 10000u + (uint32_t)t->nMonth * 100u + (uint32_t)t->nDay;
}


static uint8_t is_weather_blocking(const irr_time_t *now)
{
    irr_weather_cfg_t w;
    _IrrCfg_GetWeather(&w);

    if (!w.enable) return 0;
    if (!w.is_rain_snow) return 0;

    uint32_t now_ts = TimeKeeper_ToEpochSec(now);
    if (now_ts < w.block_until_ts) return 1;

    return 0;
}

static uint32_t weather_delay_hours_from_sel(uint8_t sel)
{
    // sel 0..5 => 24/48/72/96/120/144
    static const uint32_t table[6] = {24, 48, 72, 96, 120, 144};
    if (sel > 5) sel = 5;
    return table[sel];
}

// ------------------ 对外接口 ------------------
void Irrigation_Init(void)
{
    Valve_Init();
    s_rt.valve_open = (Valve_GetState() == VALVE_OPEN) ? 1 : 0;
    s_rt.running_src = s_rt.valve_open ? IRR_SRC_MANUAL : IRR_SRC_NONE;
    s_rt.remain_sec = 0;

    s_manual_has_cmd = 0;
    s_manual_on = 0;
}

void Irrigation_OnManualCmd(uint8_t on)
{
    s_manual_has_cmd = 1;
    s_manual_on = on ? 1 : 0;
}

irr_runtime_t Irrigation_GetRuntime(void)
{
    return s_rt;
}

extern unsigned char mcu_dp_value_update(unsigned char dpid, unsigned long value);

//static void report_countdown_if_needed(void)
//{
//    static uint32_t last_report_sec = 0xFFFFFFFF;
//    static uint8_t  div = 0;

//    // 每5秒上报一次（你也可以改成 1 秒）
//    if (++div < 5) return;
//    div = 0;

//    // 只在 ONCE/TIMER/CYCLE 运行时上报，空闲就上报0一次（可选）
//    uint32_t cur = 0;
//    if (s_rt.running_src == IRR_SRC_ONCE || s_rt.running_src == IRR_SRC_TIMER || s_rt.running_src == IRR_SRC_CYCLE) {
//        cur = s_rt.remain_sec;
//    } else {
//        cur = 0;
//    }

//    if (cur == last_report_sec) return;
//    last_report_sec = cur;

//
//		
//    unsigned char ret = mcu_dp_value_update(DPID_COUNTDOWN, (unsigned long)cur);
//    DBG_UART2("[DP] COUNTDOWN auto-report: %lu sec (ret=%u)", (unsigned long)cur, (unsigned)ret);
//}


static uint32_t ceil_to_minute_sec(uint32_t sec)
{
    if (sec == 0) return 0;
    // 向上取整到整分钟（单位：秒）
    return ((sec + 59u) / 60u) * 60u;
}

static void report_countdown_if_needed(void)
{
    static uint32_t last_report_val = 0xFFFFFFFF;
    static uint8_t  div = 0;

    // 每5秒尝试一次（你也可以改成1秒）
    if (++div < 5) return;
    div = 0;

    // 当前剩余秒
    uint32_t cur = 0;
    if (s_rt.running_src == IRR_SRC_ONCE ||
        s_rt.running_src == IRR_SRC_TIMER ||
        s_rt.running_src == IRR_SRC_CYCLE)
    {
        cur = s_rt.remain_sec;
    }

    // ? 面板按分钟显示且向下取整的话：这里把“上报秒数”向上取整到分钟边界
    // 这样 160s -> 上报180s，面板 floor(180/60)=3
    uint32_t disp = ceil_to_minute_sec(cur);

    if (disp == last_report_val) return;
    last_report_val = disp;

    unsigned char ret = mcu_dp_value_update(DPID_COUNTDOWN, (unsigned long)disp);
//    DBG_UART2("[DP] COUNTDOWN auto-report: cur=%lu sec, send=%lu sec (ret=%u)",\
              (unsigned long)cur, (unsigned long)disp, (unsigned)ret);
}



// ---- 雨量阻断标志（1=正在雨量阻断）----
static uint8_t s_rain_blocking = 0;


// 0=自动 1=手动 2=空闲
static uint8_t calc_work_state_enum(const irr_time_t *now_or_null)
{
    // 手动优先级最高：只要当前被手动接管，就认为是手动
    if (s_rt.running_src == IRR_SRC_MANUAL || s_rt.running_src == IRR_SRC_ONCE) {
        return 1; // 手动
    }
		
		// 雨量阻断：显示为空闲（因为阀门关闭）
    if (s_rain_blocking) {
        return 2; // 空闲
    }

    // 自动运行中：once/timer/cycle 任意一种在跑，都算自动
    if (
        s_rt.running_src == IRR_SRC_TIMER ||
        s_rt.running_src == IRR_SRC_CYCLE)
    {
        return 0; // 自动
    }

    // 自动阻断态：雨量阻断 / 天气延时阻断 -> 也归为“自动”
    // 注意：is_weather_blocking 需要 now，不建议传 NULL
    if (now_or_null != NULL) {
        if (is_weather_blocking(now_or_null)) {
            return 0; // 自动（天气延时）
        }
    }

    // TimeKeeper 无效时：你逻辑里仍允许手动，其它都不跑
    // 但这里我们只看“当前运行态”，如果不在手动且没在跑自动，那就空闲
    return 2; // 空闲
}

static void report_work_state_if_changed(const irr_time_t *now_or_null)
{
    static int8_t last = -1;

    uint8_t ws = calc_work_state_enum(now_or_null);
    if ((int8_t)ws == last) return;
    last = (int8_t)ws;

    unsigned char ret = mcu_dp_enum_update(DPID_WORK_STATE, ws);
//    DBG_UART2("[DP] WORK_STATE auto-report: %u (0:auto 1:manual 2:idle) ret=%u",\
              (unsigned)ws, (unsigned)ret);
}

static void report_irrigation_record(irr_src_t src, uint32_t actual_sec)
{
    unsigned char ret;
    
    // 直接上报实际时长（秒）
    ret = mcu_dp_value_update(DPID_USE_TIME_ONE, (unsigned long)actual_sec);
    
    if (actual_sec == 0) {
//        DBG_UART2("[DP] IRRIGATION RECORD: end (0 sec) ret=%u", (unsigned)ret);
    } else {
        const char* src_name = "UNKNOWN";
        switch(src) {
            case IRR_SRC_MANUAL: src_name = "MANUAL"; break;
            case IRR_SRC_ONCE:   src_name = "ONCE"; break;
            case IRR_SRC_TIMER:  src_name = "TIMER"; break;
            case IRR_SRC_CYCLE:  src_name = "CYCLE"; break;
            default: break;
        }
        
//        DBG_UART2("[DP] IRRIGATION RECORD: %s, actual=%lu sec ret=%u", \
                  src_name, (unsigned long)actual_sec, (unsigned)ret);
    }
}

// ===== 新增：记录本次灌溉的实际开始时间和已灌溉时长 =====
static uint32_t s_irrigation_start_tick = 0;    // 开始时的系统tick
static uint32_t s_irrigation_actual_sec = 0;    // 已经灌溉的秒数
static uint8_t irr_timer_status = 0;
void valve_takeover(irr_src_t src, uint8_t on, uint32_t remain_sec)
{
    // 实际开关阀
	static uint32_t start_time = 0, end_time = 0;
	static int8_t  isopenflag = -1;
    Valve_SetState(on ? VALVE_OPEN : VALVE_CLOSE);
	
    s_rt.valve_open  = on ? 1 : 0;
    s_rt.running_src = src;
    s_rt.remain_sec  = remain_sec;
    
	if (isopenflag == on) {
		return;
	} else {
		if (on) {
			start_time = MyRTC_GetUnixTimestamp(&hrtc);
		} else {
			end_time =	MyRTC_GetUnixTimestamp(&hrtc);
			end_time -= start_time;
		}
		isopenflag = on;
	}
	
	unsigned char ret = mcu_dp_bool_update(DPID_SWITCH, on ? 1 : 0);
    //DBG_UART2("[DP] SWITCH auto-report: %s", on ?  "开": "关" );
	if (IRR_SRC_TIMER == src || src == IRR_SRC_CYCLE)
		report_irrigation_record(src, remain_sec);
	else
		report_irrigation_record(src, end_time);
	
	
	if (!on) {
		//如果是在定时器内手动关阀， 则需要正确设置下一次最近日期
		if (!irr_timer_status) return;
		else if(irr_timer_status == 1) {
			irr_timer_status = 0;
			TimerConfig_t *timer = Timer_GetNearest();
			
			uint16_t next_start = timer->start_time - timer->irrigation_time;
			if (next_start >= 1440) {
				next_start -= 1440;
			} else if (next_start < 0) {
				next_start += 1440;
			}
			
			// 重新设置为开阀定时器
			timer->start_time = next_start;
			
			// 重新设置闹钟（下一个开阀时间）
			Timer_SetAlarmByNearest();
		} else if (irr_timer_status == 2) {
			
		}
	}
}

static void report_countdown_force_zero(void)
{
    // 只要你 DP 的单位是秒，这里直接 0
    unsigned char ret = mcu_dp_value_update(DPID_COUNTDOWN, 0);

//    DBG_UART2("[DP] COUNTDOWN force-report: 0 (ret=%u)", (unsigned)ret);
}


void Irrigation_StopAll(void)
{
		// ===== 修改：上报实际灌溉时长 =====
    if (s_rt.valve_open) {
        // 计算实际时长
        uint32_t actual_sec = s_irrigation_actual_sec;
        if (s_irrigation_start_tick != 0) {
            uint32_t now_tick = HAL_GetTick();
            uint32_t elapsed_ms = now_tick - s_irrigation_start_tick;
            actual_sec = (elapsed_ms + 500) / 1000;
            if (s_irrigation_actual_sec > actual_sec) {
                actual_sec = s_irrigation_actual_sec;
            }
        }
        report_irrigation_record(IRR_SRC_NONE, actual_sec);
    }
    
    // 关阀 + 立即清除运行态
    valve_takeover(IRR_SRC_NONE, 0, 0);

    // 清掉手动命令闩锁，避免下一秒又被手动命令接管
    s_manual_has_cmd = 0;
    s_manual_on = 0;
	
		// 重置触发标志
    s_last_timer_fire_daykey = 0;
    s_last_cycle_enter_daykey = 0;
	
	report_countdown_force_zero(); 
}

//雨量传感器检测
static uint8_t rain_raw(void)
{
    // PB8: 正常=0(接GND), 下雨=1(断开, 上拉)
    return (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8) == GPIO_PIN_SET) ? 1 : 0;
}

static uint8_t rain_confirmed(void)
{
    if (rain_raw()) {
		uint8_t i;
		for (i = 0; i < 10; i++) {
			if (rain_raw()) {
				HAL_Delay(10);
				continue;
			} else {
				break;
			}
		}
        if (i >= 9) return 1;
    }

    return 0;
}
extern uint8_t last_date; //一天执行一次

extern uint8_t cycle_phase[2];
// ------------------ 核心：每秒调度一次 ------------------

uint32_t g_target_time = 0;      // 目标开阀时间（绝对秒数）
uint8_t g_delay_enum = 0;        // 当前延迟枚举值

// 获取当前绝对时间（秒）- 根据你的RTC实现



// 函数1：设置延迟
// enum_value: 0-7, 1=24小时, 2=48小时, ... 7=168小时

void SetValveDelay(uint8_t enum_value)
{
    if (enum_value > 7) {
        enum_value = 7;
    }
    
    if (enum_value == 0) {
   
        g_target_time = 0;
    } else {
        // 计算目标时间 = 当前时间 + 枚举值 * 24小时
        g_target_time = RTC_GetSeconds() + (uint32_t)enum_value * 24 * 3600;
    }
}

// 函数2：检查延迟时间是否到了
// 返回值: 1=时间到了（已开阀）, 0=还没到

uint8_t CheckValveDelay(void)
{
    uint32_t now = RTC_GetSeconds();
    
    if (now >= g_target_time) {
       
        g_target_time = 0;
        return 1;
    }
    
    // 计算剩余时间
    uint32_t remaining = g_target_time - now;
    uint32_t hours = remaining / 3600;
    uint32_t minutes = (remaining % 3600) / 60;
    uint32_t seconds = remaining % 60;
    
    DBG_UART2("Remaining: %02d:%02d:%02d\n", hours, minutes, seconds);
    
    return 0;
}

uint32_t RTC_GetSeconds(void) 
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    
    // 1. 获取 RTC 的当前时间和日期
    // 注意：必须先调用 HAL_RTC_GetTime，再调用 HAL_RTC_GetDate
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    
    // 2. 将日期转换为从 2000 年开始计算的总天数
    //    Month 和 Year 需要是整数形式 (HAL_RTC_GetDate 已返回整数)
    uint32_t days = 0;
    
    // 累加年份 (从 2000 年到当前年份的前一年)
    for (uint16_t y = 2000; y < (2000 + sDate.Year); y++) {
        // 判断闰年: 能被4整除且不能被100整除，或者能被400整除
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
            days += 366;
        } else {
            days += 365;
        }
    }
    
    // 累加月份 (从 1 月到当前月份的前一月)
    uint8_t month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    for (uint8_t m = 1; m < sDate.Month; m++) {
        days += month_days[m - 1];
        // 如果是闰年的2月，加一天
        if (m == 2 && ((2000 + sDate.Year) % 4 == 0 && (2000 + sDate.Year) % 100 != 0)) {
            days += 1;
        }
    }
    
    // 累加日期
    days += (sDate.Date - 1);
    
    // 3. 将天、时、分、秒全部转换为总秒数
    uint32_t seconds = days * 86400;
    seconds += sTime.Hours * 3600;
    seconds += sTime.Minutes * 60;
    seconds += sTime.Seconds;
    
    return seconds;
}



void Irrigation_Tick_1s(void)
{
	report_countdown_if_needed();
	 // ===== 新增：更新实际灌溉时长 =====
    if (s_rt.valve_open) {
        s_irrigation_actual_sec++;  // 每秒增加1
    }
	 
    if (rain_confirmed() && rain_on)
    {
        s_rain_blocking = 1;

        // 下雨：立即关阀 + 清一切状态 + 禁止启动
        if (s_rt.valve_open || s_rt.running_src != IRR_SRC_NONE || s_rt.remain_sec != 0)
        {
                valve_takeover(IRR_SRC_NONE, 0, 0);
                report_countdown_force_zero();
        }
        s_manual_has_cmd = 0;
        s_manual_on = 0;

        // 雨量阻断下，WORK_STATE 应该显示空闲(2)
        // valve_takeover() 里如果你已经加了 report_work_state_if_changed(NULL) 也可以不写这句
        report_work_state_if_changed(NULL);
        return;
    }
	
	irr_time_t now;
    TimeKeeper_GetTime(&now);
	
    // 3) 最高优先级：手动命令（按键/APP开关）立即生效，并抢占一切
    if (s_manual_has_cmd)
    {
        s_manual_has_cmd = 0;
        valve_takeover(IRR_SRC_MANUAL, s_manual_on, 0);
        return;
    }
	
	if (!CheckValveDelay()) //雨雪天延迟
	{
		s_manual_has_cmd = 0;
		s_manual_on = 0;
		return;
	}
    
	if(s_rt.running_src == IRR_SRC_NONE)
	{
		//时间距离最近的普通定时器
		TimerConfig_t *timer = Timer_GetNearest();
		
		if (timer != NULL) {
			
			if (Timer_NeedRun(timer)) {
				
				if(now.nSec <= 1) {
					// 执行开阀
					irr_timer_status = 1; //开阀过后置位，如果中途手动关阀
					valve_takeover(IRR_SRC_TIMER, 1, timer->irrigation_time * 60);
					
					// 记录开阀开始时间
					Starttimesec = Getnowsec();

					uint16_t off_minutes = timer->start_time + timer->irrigation_time; 
					if (off_minutes >= 1440) {
						off_minutes -= 1440;
					}
					timer->start_time = off_minutes;
					
					DBG_UART2("普通定时器开阀: ID=%d, 灌溉时间=%d分钟\n", 
						   timer->timer_id, timer->start_time);
					// 重新设置闹钟（关阀时间）
					Timer_SetAlarmByNearest();
				}
			}
		}
		CycleTimerConfig_t *cycle_timer = CycleTimer_GetNearest();
		if (cycle_timer != NULL && CycleTimer_NeedRun(cycle_timer))
		{
			if(now.nSec <= 1) {
				uint8_t idx = (cycle_timer->timer_id == 1) ? 0 : 1;
				irr_timer_status = 2;
				valve_takeover(IRR_SRC_CYCLE, 1, cycle_timer->water_time * 60);
				cycle_phase[idx] = 1; //第一次进入周期灌溉
				// 修改开始时间为关阀时间（当前时间 + 浇水时间）
				cycle_tmp_start_time = cycle_timer->start_time;
				cycle_timer->start_time = cycle_timer->start_time + cycle_timer->water_time;
				if (cycle_timer->start_time >= 1440) {
					cycle_timer->start_time -= 1440;
				}
				Timer_SetAlarmByNearest();

				DBG_UART2("周期定时器灌溉时间=%d分钟\n",
						   cycle_timer->water_time);
			}
		}
	} else if(s_rt.running_src == IRR_SRC_TIMER) {
		TimerConfig_t *timer = Timer_GetNearest();
		if (timer != NULL) {

			uint32_t elapsed_seconds = Getnowsec() - Starttimesec;
			uint32_t duration_seconds = timer->irrigation_time * 60;
			
			if (elapsed_seconds >= duration_seconds) {
				DBG_UART2("定时器关阀: ID=%d, 灌溉完成\n", timer->timer_id);
				
				// 执行关阀
				irr_timer_status = 0;
				valve_takeover(IRR_SRC_NONE, 0, elapsed_seconds);
				
				int16_t next_start = timer->start_time - timer->irrigation_time;
				if (next_start >= 1440) {
					next_start -= 1440;
				} else if (next_start < 0) {
					next_start += 1440;
				}
				
				// 重新设置为开阀定时器
				timer->start_time = next_start;
				
				// 重新设置闹钟（下一个开阀时间）
				Timer_SetAlarmByNearest();
			}
		}
	} else if (s_rt.running_src == IRR_SRC_CYCLE) {
		CycleTimerConfig_t *cycle_timer = CycleTimer_GetNearest();
		if (cycle_timer->enable && now.nSec == 0)
		{
			OnWaterComplete(cycle_timer->timer_id); //周期相位
		}
    }
	
	
	#if 0
    if (_IrrCfg_PopOnceStartCmd())
    {
        uint16_t min = _IrrCfg_GetOnceDurationMin();
        if (min >= 1 && min <= 1440)
        {
            valve_takeover(IRR_SRC_ONCE, 1, (uint32_t)min * 60u);
            return;
        }
    }
	#endif
}

irr_mode_t Irrigation_GetMode(void)
{
    if (!TimeKeeper_IsValid()) return IRR_MODE_NONE;

    if (is_weather_blocking(NULL /*不方便就去掉*/)) {
        return IRR_MODE_WEATHER_DELAY;
    }

    switch (s_rt.running_src) {
        case IRR_SRC_NONE:   return IRR_MODE_NONE;
        case IRR_SRC_MANUAL: return IRR_MODE_MANUAL;
        case IRR_SRC_ONCE:   return IRR_MODE_ONCE;
        case IRR_SRC_TIMER:  return IRR_MODE_TIMER;
        case IRR_SRC_CYCLE:  return IRR_MODE_CYCLE;
        default:             return IRR_MODE_NONE;
    }
}

uint32_t Irrigation_GetRemainSec(void)
{
    return s_rt.remain_sec;
}

irr_cycle_phase_t Irrigation_GetCyclePhase(void)
{
    return (s_cyc_phase == CYC_PHASE_WATER) ? IRR_CYCLE_WATER : IRR_CYCLE_SOAK;
}

// ===== 发送清除定时指令到APP =====
void Irrigation_ClearAppTimers(void)
{
//    DBG_UART2("[SYNC] Sending clear timer commands to APP...");
    
    // 1. 清除普通定时 (DP9) - 发送一个"禁用"的定时配置
    uint8_t timer_data[15] = {0};
    timer_data[0] = 0x00;  // 帧头 - 使用0x00表示清除
    timer_data[1] = 0x00;  // 帧头
    timer_data[2] = 0x00;  // start_min high
    timer_data[3] = 0x00;  // start_min low
    timer_data[4] = 0x00;  // duration high
    timer_data[5] = 0x00;  // duration low
    timer_data[6] = 0x00;  // week_mask = 0 (无重复)
    timer_data[7] = 0x00;  // percent = 0
    timer_data[8] = 0x00;  // flag = 0 (DISABLE)
    timer_data[9] = 0x00;  // year high
    timer_data[10] = 0x00; // year low
    timer_data[11] = 0x00; // month
    timer_data[12] = 0x00; // day
    timer_data[13] = 0x00; // tail
    timer_data[14] = 0x00; // start_date_day
	
    unsigned char ret = mcu_dp_raw_update(DPID_TIMER, timer_data, 15);
//    DBG_UART2("[SYNC] CLEAR TIMER: sent disable timer (ret=%u)", (unsigned)ret);
    
    // 2. 清除周期定时 (DP10) - 发送一个"禁用"的周期配置
    uint8_t cycle_data[15] = {0};
    cycle_data[0] = 0x00;  // 帧头
    cycle_data[1] = 0x00;  // 帧头
    cycle_data[2] = 0x00;  // enable = 0 (DISABLE)
    cycle_data[3] = 0x00;  // start_hour
    cycle_data[4] = 0x00;  // start_min
    cycle_data[5] = 0x00;  // end_hour
    cycle_data[6] = 0x00;  // end_min
    cycle_data[7] = 0x00;  // water_min
    cycle_data[8] = 0x00;  // soak_min
    cycle_data[9] = 0x00;  // week_mask
    cycle_data[10] = 0x00; // water_percent
    cycle_data[11] = 0x00; // soak_percent
    cycle_data[12] = 0x00; // start_date_year
    cycle_data[13] = 0x00; // start_date_month
    cycle_data[14] = 0x00; // start_date_day
    
    ret = mcu_dp_raw_update(DPID_CYCLE_TIMING, cycle_data, 15);
//    DBG_UART2("[SYNC] CLEAR CYCLE: sent disable cycle (ret=%u)", (unsigned)ret);
    
//    DBG_UART2("[SYNC] ====== Clear commands sent to APP ======");
}

void Irrigation_DeleteAllTimers(void)
{

	Irrigation_ClearAppTimers();
	
	Timer_DeleteAll();
}


// ===== 上报设备空闲状态到APP =====
void Irrigation_ReportIdleStatus(void)
{
  //  DBG_UART2("[SYNC] ====== Reporting idle status to APP ======");
    
    // 1. 上报开关状态为关 (DP1)
    unsigned char ret = mcu_dp_bool_update(DPID_SWITCH, 0);
   // DBG_UART2("[SYNC] SWITCH: OFF (ret=%u)", (unsigned)ret);
    
    // 2. 上报倒计时为0 (DP4)
    ret = mcu_dp_value_update(DPID_COUNTDOWN, 0);
   // DBG_UART2("[SYNC] COUNTDOWN: 0 sec (ret=%u)", (unsigned)ret);
    
    // 3. 上报工作状态为空闲 (DP5) - 2表示空闲
    ret = mcu_dp_enum_update(DPID_WORK_STATE, 2);  // 0:自动 1:手动 2:空闲
  //  DBG_UART2("[SYNC] WORK_STATE: IDLE (2) (ret=%u)", (unsigned)ret);
    
    // 4. 清除所有定时器显示
    Irrigation_DeleteAllTimers();
    
//    DBG_UART2("[SYNC] ====== Idle status reported ======");
}