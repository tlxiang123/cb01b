#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

// 小端转大端（读取时使用）
#define TIMER_ACTION_RUN       0x01   // 开阀定时器
#define TIMER_ACTION_OFF       0x02   // 关阀定时器

#define LE_TO_BE16(x)  ((((x) >> 8) & 0xFF) | (((x) & 0xFF) << 8))

// 大端转小端（写入时使用）
#define BE_TO_LE16(x)  ((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF))

// 定时器数据结构（13字节）
typedef struct {
    uint8_t  reserved0;           // byte0: 保留，bit0=0
    uint8_t  timer_id;            // byte1: 定时器编号（1-8，0=空）
    uint16_t start_time;          // byte2-3: 开始时间（分钟）
    uint16_t irrigation_time;     // byte4-5: 灌溉时间（分钟）
    uint8_t  week_mask;           // byte6: 星期掩码
    uint8_t  valve_percent;       // byte7: 阀门百分比
    uint8_t  reserved[5];         // byte8-12: 保留
} TimerConfig_t;


// ==================== 周期灌溉专用结构体 ====================
typedef struct {
    uint8_t  reserved0;           // byte0: 保留
    uint8_t  timer_id;            // byte1: 定时器ID (10或11)
    uint8_t  enable;              // byte2: 开关
    uint8_t  week_mask;           // byte3: 星期掩码
    uint16_t start_time;          // byte4-5: 开始时间
    uint16_t end_time;            // byte6-7: 结束时间
    uint16_t water_time;          // byte8-9: 浇水时间
    uint16_t soak_time;           // byte10-11: 浸泡时间
    uint8_t  valve_percent;       // byte12: 阀门百分比
} CycleTimerConfig_t;

typedef struct {
    uint8_t type;           // 0=普通定时器, 1=周期灌溉
    uint8_t timer_id;       // 定时器ID
    uint16_t start_time;    // 开始时间（分钟）
    uint8_t week_mask;      // 星期掩码
    uint8_t action;         // 动作类型
    uint8_t valve_percent;  // 阀门百分比
	uint16_t end_time;		// 结束时间
	uint8_t target_weekday; // 实际执行的目标星期（1-7, 1=周一）
	uint16_t year;
	uint8_t month;
	uint8_t day;
} NearestTimer_t;

// 在 timer_manager.h 中添加定义
typedef enum {
    CYCLE_STATUS_IDLE = 0,      // 空闲（不在时间段内）
    CYCLE_STATUS_WATERING = 1,  // 浇水阶段（开阀）
    CYCLE_STATUS_SOAKING = 2    // 浸泡阶段（关阀）
} CycleTimerStatus_t;


extern uint16_t cycle_tmp_start_time;


// 初始化定时器管理（上电时调用一次）
void Timer_Init(void);

// 函数1：存储服务器下发的定时器（13字节数据）
bool Timer_Store(const uint8_t* data);

// 函数2：获取整个定时器列表
uint16_t Timer_GetList(uint8_t* buffer);


// 辅助函数：获取定时器数量
int Timer_GetCount(void);

bool Timer_Delete(uint8_t timer_id);

TimerConfig_t* Timer_GetNearest(void);
int Timer_SetAlarmByNearest(void); //设置到最近日期的定时器
uint8_t Timer_NeedRun(TimerConfig_t *timer);	//判断时间是否匹配
void SaveSlot(int slot_index);
int Timer_FindSlotByID(uint8_t timer_id);
uint8_t Timer_Modify(uint8_t timer_id, const uint8_t* data);
// ==================== 新增辅助函数 ====================
uint8_t Timer_GetAction(const TimerConfig_t* timer);
void Timer_SetAction(TimerConfig_t* timer, uint8_t action);

uint16_t Cycle_GetList(uint8_t* buffer);
void Cycle_Store(const uint8_t* data);
void Cycle_Delete(uint8_t server_timer_id);
void Cycle_Init(void);
void Cycle_Delete_All(void);
uint8_t Cycle_ModifyFromServer(const uint8_t* data);
NearestTimer_t Timer_GetNearestAll(void);
CycleTimerConfig_t* CycleTimer_GetNearest(void);
uint8_t CycleTimer_NeedRun(CycleTimerConfig_t *cycle_timer);
void OnWaterComplete(uint8_t timer_id);
bool Timer_DeleteAll(void);
int DeleteTimer(int timer_id);


#endif
