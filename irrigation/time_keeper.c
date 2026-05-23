#include "time_keeper.h"
#include "rtc.h"        // 你的 hrtc 在这里
#include "bluetooth.h"  // 里面有 bt_send_time_sync_req()
#include "usart.h"  // 里面有 bt_send_time_sync_req()
#include <time.h>   // 需要包含这个头文件来使用 struct tm 和 mktime

/* ===== 内部状态 ===== */
static volatile uint8_t g_time_valid = 0;
static volatile int16_t g_tz_min = 0;   // 时区分钟（可选）

/* ===== 工具：DayIndex(0=Sunday) -> HAL WeekDay(1=Mon..7=Sun) ===== */
static uint8_t dayindex_to_hal_wday(uint8_t dayIndex)
{
    // dayIndex: 0=Sunday, 1=Monday, ... 6=Saturday
    if (dayIndex == 0) return 7;         // Sunday -> 7
    if (dayIndex <= 6) return dayIndex;  // Monday..Saturday -> 1..6
    return 1; // fallback
}

/* ===== 内部：写 RTC（年月日时分秒） ===== */
static void rtc_set_ymdhms(uint16_t year, uint8_t month, uint8_t day,
                           uint8_t hour, uint8_t min, uint8_t sec,
                           uint8_t wday)
{
    RTC_TimeTypeDef t = {0};
    RTC_DateTypeDef d = {0};

    t.Hours   = hour;
    t.Minutes = min;
    t.Seconds = sec;

    // Cube/HAL：Year=0..99 代表 2000..2099（默认习惯）
    if (year >= 2000) d.Year = (uint8_t)(year - 2000);
    else if (year >= 1900) d.Year = (uint8_t)(year - 1900);
    else d.Year = 0;

    d.Month   = month;
    d.Date    = day;
    d.WeekDay = (wday >= 1 && wday <= 7) ? wday : 1;

    HAL_RTC_SetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &d, RTC_FORMAT_BIN);
}

/* ===== 内部：读 RTC ===== */
static void rtc_get_ymdhms(tk_datetime_t *dt)
{
    RTC_TimeTypeDef t = {0};
    RTC_DateTypeDef d = {0};

    HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN); // 必须紧跟 GetTime

    dt->year  = 2000 + d.Year;
    dt->month = d.Month;
    dt->day   = d.Date;
    dt->wday  = d.WeekDay;

    dt->hour  = t.Hours;
    dt->min   = t.Minutes;
    dt->sec   = t.Seconds;
}

/* ===== 对外 API ===== */
void TimeKeeper_Init(void)
{
    // 你也可以在这里读备份寄存器判断是否已同步过
    g_time_valid = 0;
    g_tz_min = 0;
}

uint8_t TimeKeeper_IsValid(void)
{
    return g_time_valid;
}

void TimeKeeper_Invalidate(void)
{
    g_time_valid = 0;
}

void TimeKeeper_Get(tk_datetime_t *dt)
{
    if (!dt) return;
    rtc_get_ymdhms(dt);
}

void TimeKeeper_RequestSync(uint8_t sync_time_type)
{
    // 推荐：sync_time_type=0x02（年月日时分秒）
    bt_send_time_sync_req(sync_time_type);
}

/* ===== 供 bt_time_sync_result() 调用：自定义年月日格式 ===== */
void TimeKeeper_OnTimeSyncCustom(const bt_time_struct_data_t *bt_time,
                                 unsigned short time_zone_100)
{
    if (!bt_time) {
        g_time_valid = 0;
        return;
    }

    // 保存时区（小时*100 -> 分钟）
    g_tz_min = (int16_t)(time_zone_100 * 60 / 100);

    uint16_t year  = bt_time->nYear;
    uint8_t  month = bt_time->nMonth;
    uint8_t  day   = bt_time->nDay;
    uint8_t  hour  = bt_time->nHour;
    uint8_t  min   = bt_time->nMin;
    uint8_t  sec   = bt_time->nSec;
    uint8_t  wday  = dayindex_to_hal_wday(bt_time->DayIndex);

    // 简单合法性保护（可按你需要增强）
    if (month < 1 || month > 12 || day < 1 || day > 31 ||
        hour > 23 || min > 59 || sec > 59) {
        g_time_valid = 0;
        return;
    }

    rtc_set_ymdhms(year, month, day, hour, min, sec, wday);
    g_time_valid = 1;
}

/* ===== 可选：Unix ms 格式支持（你现在可以先不用） ===== */
static uint8_t is_leap(uint16_t y)
{
    return ((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0);
}

static void unix_to_ymdhms(uint32_t unix_s,
                           uint16_t *year, uint8_t *month, uint8_t *day,
                           uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    uint32_t s = unix_s;

    *sec  = s % 60; s /= 60;
    *min  = s % 60; s /= 60;
    *hour = s % 24; s /= 24;

    uint32_t days = s;
    uint16_t y = 1970;

    while (1) {
        uint16_t diy = is_leap(y) ? 366 : 365;
        if (days >= diy) { days -= diy; y++; }
        else break;
        if (y > 2099) break;
    }

    static const uint8_t mdays_norm[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    uint8_t m = 1;
    for (int i = 0; i < 12; i++) {
        uint8_t dim = mdays_norm[i];
        if (i == 1 && is_leap(y)) dim = 29;
        if (days >= dim) { days -= dim; m++; }
        else break;
    }

    *year  = y;
    *month = m;
    *day   = (uint8_t)days + 1;
}

void TimeKeeper_OnTimeSyncUnixMs(unsigned long time_stamp_ms,
                                 unsigned short time_zone_100)
{
    // 保存时区（小时*100 -> 分钟）
    g_tz_min = (int16_t)(time_zone_100 * 60 / 100);

    uint32_t unix_s = (uint32_t)(time_stamp_ms / 1000UL);

    // 默认按 UTC + 时区 -> 本地时间；如果你发现偏差，就去掉这一句
    unix_s += (int32_t)g_tz_min * 60;

    uint16_t y; uint8_t mo, da, hh, mm, ss;
    unix_to_ymdhms(unix_s, &y, &mo, &da, &hh, &mm, &ss);

    rtc_set_ymdhms(y, mo, da, hh, mm, ss, 1);
    g_time_valid = 1;
}


void TimeKeeper_GetTime(irr_time_t *t)
{
    if (!t) return;

    tk_datetime_t dt;
    TimeKeeper_Get(&dt);

    t->nYear    = (uint16_t)dt.year;
    t->nMonth   = (uint8_t)dt.month;
    t->nDay     = (uint8_t)dt.day;
    t->nHour    = (uint8_t)dt.hour;
    t->nMin     = (uint8_t)dt.min;
    t->nSec     = (uint8_t)dt.sec;

    // 你 dt.wday 是 HAL 格式：1=Mon..7=Sun
    // irrigation 里 DayIndex 要 0=Sun..6=Sat
    if (dt.wday == 7) t->DayIndex = 0;        // Sun
    else if (dt.wday >= 1 && dt.wday <= 6) t->DayIndex = dt.wday; // Mon..Sat -> 1..6
    else t->DayIndex = 1;
}


static uint8_t _tk_is_leap_year(uint16_t y)
{
    return ((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0);
}

uint32_t TimeKeeper_ToEpochSec(const irr_time_t *t)
{
    if (!t) return 0;
    if (!TimeKeeper_IsValid()) return 0;

    static const uint8_t mdays_norm[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

    uint16_t year  = t->nYear;
    uint8_t  month = t->nMonth;
    uint8_t  day   = t->nDay;

    // 基本保护
    if (year < 1970) year = 1970;
    if (month < 1) month = 1;
    if (month > 12) month = 12;
    if (day < 1) day = 1;
    if (day > 31) day = 31;

    uint32_t days = 0;

    for (uint16_t y = 1970; y < year; y++)
        days += _tk_is_leap_year(y) ? 366u : 365u;

    for (uint8_t m = 1; m < month; m++)
    {
        uint16_t dm = mdays_norm[m-1];
        if (m == 2 && _tk_is_leap_year(year)) dm = 29;
        days += (uint32_t)dm;
    }

    days += (uint32_t)(day - 1);

    return days * 86400u
         + (uint32_t)t->nHour * 3600u
         + (uint32_t)t->nMin  * 60u
         + (uint32_t)t->nSec;
}


void Debug_PrintTimeKeeper(void)
{
  tk_datetime_t dt;
  TimeKeeper_Get(&dt);

//  DBG_UART2("[TK] %04u-%02u-%02u %02u:%02u:%02u valid=%u\r\n",
//         dt.year, dt.month, dt.day,
//         dt.hour, dt.min, dt.sec,
//         TimeKeeper_IsValid());
}


void TimeKeeper_MarkNeedSync(void) { s_need_sync = 1; }
uint8_t TimeKeeper_NeedSync(void)  { return s_need_sync; }
void TimeKeeper_ClearNeedSync(void){ s_need_sync = 0; }



/**
  * @brief  将 RTC 读取的时间转换为 Unix 时间戳
  * @param  hrtc: RTC 句柄指针
  * @retval Unix 时间戳 (uint32_t)
  */
uint32_t MyRTC_GetUnixTimestamp(RTC_HandleTypeDef *hrtc)
{
    struct tm stm_time;             // 定义tm结构体，用于mktime函数
    RTC_DateTypeDef sDate;          // 定义HAL库的日期结构体
    RTC_TimeTypeDef sTime;          // 定义HAL库的时间结构体

    // 1. 从 RTC 硬件中读取当前的日期和时间
    HAL_RTC_GetTime(hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(hrtc, &sDate, RTC_FORMAT_BIN); // 注意：GetDate必须在GetTime之后调用

    // 2. 将HAL库的数据填入标准库的 tm 结构体
    stm_time.tm_year = sDate.Year + 100;  // tm_year是从1900年开始的，所以2023年应该是123。这里Year从RTC读出来是23，因此+100 = 123
    stm_time.tm_mon  = sDate.Month - 1;   // tm_mon 的范围是 0~11，所以需要减1
    stm_time.tm_mday = sDate.Date;
    stm_time.tm_hour = sTime.Hours;
    stm_time.tm_min  = sTime.Minutes;
    stm_time.tm_sec  = sTime.Seconds;

    // 3. 使用 mktime 进行转换
    // mktime 会将 tm 结构体转换为自1970年1月1日以来的秒数，并自动处理时区为UTC
    return (uint32_t)mktime(&stm_time);
}

