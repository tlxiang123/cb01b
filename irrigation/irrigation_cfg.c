#include "irrigation.h"
#include "usart.h"


// --------- 全局配置（如果你希望多条定时，就把结构改成数组即可） ---------
static uint16_t         s_once_duration_min = 0;
static uint8_t          s_once_start_cmd = 0;

static irr_timer_cfg_t  s_timer_cfg = {0};
irr_cycle_cfg_t  S_cycle_cfg = {0};

static irr_weather_cfg_t s_weather_cfg = {0};

// --------- 提供给 irrigation.c 读取（用内部 getter） ---------
uint16_t _IrrCfg_GetOnceDurationMin(void) { return s_once_duration_min; }
uint8_t  _IrrCfg_PopOnceStartCmd(void)    { uint8_t v = s_once_start_cmd; s_once_start_cmd = 0; return v; }


void _IrrCfg_GetTimer(irr_timer_cfg_t *out)   { *out = s_timer_cfg; }
//void _IrrCfg_GetCycle(irr_cycle_cfg_t *out)   { *out = s_cycle_cfg; }
void _IrrCfg_GetWeather(irr_weather_cfg_t *o) { *o = s_weather_cfg; }

void _IrrCfg_SetWeatherBlockUntil(uint32_t ts) { s_weather_cfg.block_until_ts = ts; }


// --------- 对外：给 protocol.c 调用 ---------
void Irrigation_SetOnceDurationMin(uint16_t min)
{
    if (min > 1440) min = 1440;
    if (min < 1) min = 1;
    s_once_duration_min = min;
}

void Irrigation_StartOnce(void)
{
    s_once_start_cmd = 1;
}

void Irrigation_SetTimerCfg(const irr_timer_cfg_t *cfg)
{
    if (!cfg) return;
    s_timer_cfg = *cfg;
    if (s_timer_cfg.duration_min > 1440) s_timer_cfg.duration_min = 1440;
}
#if 0
void Irrigation_SetCycleCfg(const irr_cycle_cfg_t *cfg)
{
    if (!cfg) return;
    s_cycle_cfg = *cfg;
    if (s_cycle_cfg.water_min > 1440) s_cycle_cfg.water_min = 1440;
    if (s_cycle_cfg.soak_min > 1440)  s_cycle_cfg.soak_min  = 1440;
//	DBG_UART2("\r\n Irrigation_SetCycleCfg set ok\r\n");
	
}
#endif
void Irrigation_SetWeatherEnable(uint8_t en)
{
    s_weather_cfg.enable = en ? 1 : 0;
}

void Irrigation_SetWeatherDelaySel(uint8_t sel)
{
    // 你UI：24/48/72/96/120/144 (6档)，这里用 0..5 表示
    if (sel > 5) sel = 5;
    s_weather_cfg.delay_sel = sel;
}

void Irrigation_SetWeatherRainSnow(uint8_t is_rain_snow)
{
    s_weather_cfg.is_rain_snow = is_rain_snow ? 1 : 0;
}

// irrigation.c 用到的内部符号声明（不放头文件，避免外部误用）
extern uint16_t _IrrCfg_GetOnceDurationMin(void);
extern uint8_t  _IrrCfg_PopOnceStartCmd(void);

