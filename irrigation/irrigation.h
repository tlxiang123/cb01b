#ifndef __IRRIGATION_H__
#define __IRRIGATION_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ====== 时间结构：尽量对齐你 bt_time_struct_data_t ======

typedef struct
{
    uint16_t nYear;
    uint8_t  nMonth;
    uint8_t  nDay;
    uint8_t  nHour;
    uint8_t  nMin;
    uint8_t  nSec;
    uint8_t  DayIndex; /* 0=Sunday */
} irr_time_t;

// ====== 业务优先级源 ======
typedef enum {
    IRR_SRC_NONE = 0,
    IRR_SRC_MANUAL,
    IRR_SRC_ONCE,        // 倒计时/单次灌溉
    IRR_SRC_TIMER,       // 普通定时
    IRR_SRC_CYCLE,        // 周期灌溉（窗口内水/浸泡循环）
	IRR_SEC_CYCLE_OFF
} irr_src_t;

typedef enum {
	TIMER_RUN = 0x62,
	TIMER_OFF
} timer_sta_t;

// ====== 普通定时配置 ======
typedef struct {
    uint8_t  enable;
    uint8_t  week_mask;       // bit0=Sun ... bit6=Sat
    uint8_t  hour;            // 0..23
    uint8_t  min;             // 0..59
    uint16_t duration_min;    // 1..1440
	uint8_t timerstatus;
} irr_timer_cfg_t;



// ====== 周期定时配置 ======
typedef struct {
	uint16_t water_min;       // 开阀分钟
    uint16_t soak_min;        // 关阀浸泡分钟
    uint8_t  enable;
    uint8_t  week_mask;       // bit0=Sun ... bit6=Sat
    uint8_t  start_hour, start_min;
    uint8_t  end_hour,   end_min;
} irr_cycle_cfg_t;

extern irr_cycle_cfg_t  S_cycle_cfg;


// ====== 天气延时配置 ======
typedef struct {	
    uint8_t  enable;          // 天气延时开关
    uint8_t  delay_sel;       // 0..6 => {24,48,72,96,120,144}小时，你在 cfg.c 做映射
    uint8_t  is_rain_snow;    // 当前是否雨雪（由DP或算法赋值）
    uint32_t block_until_ts;  // 阻断截止（绝对秒）
} irr_weather_cfg_t;

// ====== 运行态（只读） ======
typedef struct {
    uint8_t   valve_open;
    irr_src_t running_src;
    uint32_t  remain_sec;     // 当前阶段剩余秒
} irr_runtime_t;


// ---------------- 对外接口 ----------------

void Irrigation_Init(void);

/**
 * @brief 每1秒调用一次（建议用 HAL_GetTick() 控制）
 */
void Irrigation_Tick_1s(void);

/**
 * @brief 手动命令（按键/APP开关）最高优先级
 * @param on 1=开阀 0=关阀
 */
void Irrigation_OnManualCmd(uint8_t on);

/**
 * @brief 倒计时/单次灌溉：设置时长（分钟）
 */
void Irrigation_SetOnceDurationMin(uint16_t min);

/**
 * @brief 倒计时/单次灌溉：点击“开始”
 */
void Irrigation_StartOnce(void);

/**
 * @brief 设置普通定时配置（解析RAW后填入）
 */
void Irrigation_SetTimerCfg(const irr_timer_cfg_t *cfg);

/**
 * @brief 设置周期定时配置（解析RAW后填入）
 */
void Irrigation_SetCycleCfg(const irr_cycle_cfg_t *cfg);

/**
 * @brief 设置天气延时开关与选项
 */
void Irrigation_SetWeatherEnable(uint8_t en);
void Irrigation_SetWeatherDelaySel(uint8_t sel);
void Irrigation_SetWeatherRainSnow(uint8_t is_rain_snow);

void valve_takeover(irr_src_t src, uint8_t on, uint32_t remain_sec);

uint32_t RTC_GetSeconds(void);
void SetValveDelay(uint8_t enum_value);
/**
 * @brief 获取运行态（调试用）
 */
irr_runtime_t Irrigation_GetRuntime(void);


void Irrigation_StopAll(void);

typedef enum {
    IRR_MODE_NONE = 0,
    IRR_MODE_MANUAL,
    IRR_MODE_ONCE,
    IRR_MODE_TIMER,
    IRR_MODE_CYCLE,
    IRR_MODE_WEATHER_DELAY,
} irr_mode_t;

typedef enum {
    IRR_CYCLE_WATER = 0,
    IRR_CYCLE_SOAK,
} irr_cycle_phase_t;

/* ===== 对外查询接口 ===== */
irr_mode_t Irrigation_GetMode(void);
uint32_t   Irrigation_GetRemainSec(void);
irr_cycle_phase_t Irrigation_GetCyclePhase(void);

// irrigation.h 中添加
void Irrigation_SyncToApp(void);
void Irrigation_ReportIdleStatus(void);
void Irrigation_DeleteAllTimers(void);


void     _IrrCfg_GetTimer(irr_timer_cfg_t *out);
void     _IrrCfg_GetCycle(irr_cycle_cfg_t *out);
void     _IrrCfg_GetWeather(irr_weather_cfg_t *o);
void     _IrrCfg_SetWeatherBlockUntil(uint32_t ts);

#ifdef __cplusplus
}
#endif

#endif
