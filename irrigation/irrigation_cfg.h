#ifndef __TIME_KEEPER_H__
#define __TIME_KEEPER_H__

#include <stdint.h>
#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 统一的日期时间结构 */
typedef struct {
    uint16_t year;   // 2000-2099
    uint8_t  month;  // 1-12
    uint8_t  day;    // 1-31
    uint8_t  hour;   // 0-23
    uint8_t  min;    // 0-59
    uint8_t  sec;    // 0-59
    uint8_t  wday;   // 1=Mon..7=Sun (HAL RTC weekday convention)
} tk_datetime_t;

/* 初始化：可选做一些标志位初始化 */
void TimeKeeper_Init(void);

/* 时间是否有效（是否已成功同步过） */
uint8_t TimeKeeper_IsValid(void);

/* 读取 RTC 当前时间 */
void TimeKeeper_Get(tk_datetime_t *dt);

/* 可选：清除有效标志（比如断电/重置后你想强制重新同步） */
void TimeKeeper_Invalidate(void);

/* 请求模块下发当前时间（推荐 sync_time_type=0x02） */
void TimeKeeper_RequestSync(uint8_t sync_time_type);

/* ===== 给涂鸦 SDK 回调使用的接口 =====
 * 在 bt_time_sync_result() 里调用它们
 */
//typedef struct
//{
//    unsigned short nYear;
//    unsigned char  nMonth;
//    unsigned char  nDay;
//    unsigned char  nHour;
//    unsigned char  nMin;
//    unsigned char  nSec;
//    unsigned char  DayIndex; /* 0 = Sunday */
//} bt_time_struct_data_t;

/* 用年月日时分秒格式写入 RTC（sync_time_type=0x00/0x02） */
void TimeKeeper_OnTimeSyncCustom(const bt_time_struct_data_t *bt_time,
                                 unsigned short time_zone_100);

/* 若你未来要支持 0x01（Unix ms），可以用这个接口 */
void TimeKeeper_OnTimeSyncUnixMs(unsigned long time_stamp_ms,
                                 unsigned short time_zone_100);


void Debug_PrintTimeKeeper(void);

#ifdef __cplusplus
}
#endif

#endif /* __TIME_KEEPER_H__ */
