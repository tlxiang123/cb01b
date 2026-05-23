#include "timer_manager.h"
#include "stm32l0xx_hal.h"
#include <string.h>
#include "usart.h"
#include "rtc.h"
#include "adc.h"


// ==================== ??????? ====================
#define EEPROM_BASE_ADDR        0x08080000UL
#define EEPROM_MAGIC_ADDR       (EEPROM_BASE_ADDR + 0x0000)
#define EEPROM_TIMER_START_ADDR (EEPROM_BASE_ADDR + 0x0010)

#define TIMER_SLOT_SIZE         13
#define MAX_TIMER_SLOTS         8
#define MAGIC_NUMBER            0xAA

// ???????????????????????????
#define CYCLE_SLOT_1            (MAX_TIMER_SLOTS - 2)  // ????8
#define CYCLE_SLOT_2            (MAX_TIMER_SLOTS - 1)  // ????9
#define MAX_CYCLE_TIMERS        2

// ????????????????????????????????隆陋?
#define EEPROM_CYCLE_START_ADDR (EEPROM_TIMER_START_ADDR + MAX_TIMER_SLOTS * TIMER_SLOT_SIZE)



// ????隆矛??????????
static TimerConfig_t g_timers[MAX_TIMER_SLOTS];
static CycleTimerConfig_t g_cycle_timers[MAX_CYCLE_TIMERS];

// ==================== ?????????? ====================
static void PrintHex(const char* title, const uint8_t* data, uint32_t len)
{
    //DBG_UART2("%s (%d bytes): ", title, len);
    for (uint32_t i = 0; i < len; i++) {
        //DBG_UART2("%02X ", data[i]);
    }
    //DBG_UART2("\n");
}

static void PrintTimer(const char* title, const TimerConfig_t* timer, int slot)
{
//    DBG_UART2("%s [????%d]:\n", title, slot);
//    DBG_UART2("  reserved0      = 0x%02X\n", timer->reserved0);
//    DBG_UART2("  timer_id       = %d\n", timer->timer_id);
//    DBG_UART2("  start_time     = %d (%02d:%02d)\n", \
//           timer->start_time, timer->start_time / 60, timer->start_time % 60);
//    DBG_UART2("  irrigation_time= %d ????\n", timer->irrigation_time);
//    DBG_UART2("  week_mask      = 0x%02X\n", timer->week_mask);
//    DBG_UART2("  valve_percent  = %d\n", timer->valve_percent);
//    DBG_UART2("  reserved       = %02X %02X %02X %02X %02X\n",\
//           timer->reserved[0], timer->reserved[1], timer->reserved[2],\
//           timer->reserved[3], timer->reserved[4]);
}

// ==================== EEPROM ??隆矛? ====================
static void EEPROM_Read(uint32_t addr, uint8_t* data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        data[i] = *(__IO uint8_t*)(addr + i);
    }
    PrintHex("EEPROM ???", data, len);
}

static HAL_StatusTypeDef EEPROM_Write(uint32_t addr, uint8_t* data, uint32_t len)
{
    HAL_StatusTypeDef status = HAL_OK;
    PrintHex("EEPROM 隆矛???", data, len);
    
    HAL_FLASHEx_DATAEEPROM_Unlock();
    
    for (uint32_t i = 0; i < len; i++) {
        status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE, 
                                                 addr + i, 
                                                 data[i]);
        if (status != HAL_OK) {
            ////DBG_UART2("EEPROM 隆矛?????? at addr 0x%08lX\n", addr + i);
            break;
        }
    }
    
    HAL_FLASHEx_DATAEEPROM_Lock();
    return status;
}

static int FindFreeSlot(void)
{
    for (int i = 0; i < MAX_TIMER_SLOTS; i++) {
        if (g_timers[i].timer_id == 0 || g_timers[i].timer_id == 0xFF) {
//            DBG_UART2("slot free: %d\n", i);
            return i;
        }
    }
    DBG_UART2("slot full!\n");
    return -1;
}


void SaveSlot(int slot_index)
{
    if (slot_index < 0 || slot_index >= MAX_TIMER_SLOTS) return;
    
    //DBG_UART2("\n=== ??????? %d ===\n", slot_index);
    PrintTimer("?????", &g_timers[slot_index], slot_index);
    
    uint32_t addr = EEPROM_TIMER_START_ADDR + slot_index * TIMER_SLOT_SIZE;
    EEPROM_Write(addr, (uint8_t*)&g_timers[slot_index], TIMER_SLOT_SIZE);
    
    //DBG_UART2("???? %d ???????\n\n", slot_index);
}

// ==================== ???????????? ====================
uint8_t Timer_GetAction(const TimerConfig_t* timer)
{
    return timer->reserved[4];
}

void Timer_SetAction(TimerConfig_t* timer, uint8_t action)
{
    timer->reserved[4] = action;
}

static uint16_t CalculateOffTime(uint16_t start_minutes, uint16_t irrig_minutes)
{
    uint16_t off_minutes = start_minutes + irrig_minutes;
    if (off_minutes >= 1440) {
        off_minutes -= 1440;
    }
    return off_minutes;
}


static void CompactAfterDelete(int deleted_slot)
{
    for (int i = deleted_slot; i < MAX_TIMER_SLOTS - 1; i++) {
        if (g_timers[i + 1].timer_id != 0) {
			
            memcpy(&g_timers[i], &g_timers[i + 1], TIMER_SLOT_SIZE);
            
            g_timers[i].timer_id = i + 1;
           
            SaveSlot(i);
        } else {
          
            memset(&g_timers[i], 0, TIMER_SLOT_SIZE);
            SaveSlot(i);
            break;
        }
    }
    
 
    if (g_timers[MAX_TIMER_SLOTS - 1].timer_id != 0) {
        memset(&g_timers[MAX_TIMER_SLOTS - 1], 0, TIMER_SLOT_SIZE);
        SaveSlot(MAX_TIMER_SLOTS - 1);
    }
}

void Timer_Init(void)
{
	
    uint8_t magic;
    Cycle_Init();
    //DBG_UART2("\n========== Timer_Init ??? ==========\n");
    
    // ??????
    EEPROM_Read(EEPROM_MAGIC_ADDR, &magic, 1);
    //DBG_UART2("???: 0x%02X\n", magic);
    
    if (magic != MAGIC_NUMBER) {
        //DBG_UART2("EEPROM ???????????隆矛?????...\n");
        
        // ???????????????隆矛?????
        memset(g_timers, 0, sizeof(g_timers));
        
        // 隆矛??????
        uint8_t init_data[4] = {MAGIC_NUMBER, 0, 0, 0};
        EEPROM_Write(EEPROM_MAGIC_ADDR, init_data, 4);
        
        // ????????????
        for (int i = 0; i < MAX_TIMER_SLOTS; i++) {
            SaveSlot(i);
        }
        
        //DBG_UART2("EEPROM ????????\n");
    } else {
        //DBG_UART2("EEPROM ????????????????...\n");
        
        // ???????????????隆矛?????
        for (int i = 0; i < MAX_TIMER_SLOTS; i++) {
            uint32_t addr = EEPROM_TIMER_START_ADDR + i * TIMER_SLOT_SIZE;
            EEPROM_Read(addr, (uint8_t*)&g_timers[i], TIMER_SLOT_SIZE);
            
            // ??????? EEPROM ?? 0xFF ???? 0????????隆矛??
            if (g_timers[i].timer_id == 0xFF) {
                //DBG_UART2("???? %d: ??? 0xFF?????? 0\n", i);
                g_timers[i].timer_id = 0;
                // ????????????EEPROM?隆矛??????
                SaveSlot(i);
            }
            
            // ????????????
            if (g_timers[i].timer_id != 0) {
                PrintTimer("????", &g_timers[i], i);
            }
        }
    }
    
    //DBG_UART2("========== Timer_Init ??? ==========\n\n");
}
/**
 * @brief ??????隆猫???????EEPROM
 * @param index 0=????隆猫, 1=????隆猫
 */

void SaveCycleToEEPROM(uint8_t index)
{
    uint32_t addr = EEPROM_CYCLE_START_ADDR + index * sizeof(CycleTimerConfig_t);
    
    HAL_FLASHEx_DATAEEPROM_Unlock();
    
    uint8_t* data = (uint8_t*)&g_cycle_timers[index];
    for (int i = 0; i < sizeof(CycleTimerConfig_t); i++) {
        HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_BYTE, addr + i, data[i]);
    }
    
    HAL_FLASHEx_DATAEEPROM_Lock();
    
    //DBG_UART2("??????%d??????EEPROM\n", index + 1);
}

/**
 * @brief ?????????????
 * @param data ????????隆猫??????13????
 */
/**
 * @brief ?????????????
 * @param server_timer_id ????????隆猫??? timer_id??0??1??
 */
/**
 * @brief ??????????????EEPROM?????
 */
void Cycle_Init(void)
{
    for (int i = 0; i < MAX_CYCLE_TIMERS; i++) {
        uint32_t addr = EEPROM_CYCLE_START_ADDR + i * sizeof(CycleTimerConfig_t);
        EEPROM_Read(addr, (uint8_t*)&g_cycle_timers[i], sizeof(CycleTimerConfig_t));
        
        if (g_cycle_timers[i].timer_id == 0xFF) {
            memset(&g_cycle_timers[i], 0, sizeof(CycleTimerConfig_t));
        }
        
        if (g_cycle_timers[i].timer_id != 0) {
            //DBG_UART2("??????%d????: ID=%d, enable=%d, \
			week_mask=0x%02X, start=%02d:%02d\n",\
                   i + 1,\
                   g_cycle_timers[i].timer_id,\
                   g_cycle_timers[i].enable,\
                   g_cycle_timers[i].week_mask,\
                   g_cycle_timers[i].start_time / 60,\
                   g_cycle_timers[i].start_time % 60);
        }
    }
}

void Cycle_Store(const uint8_t* data)
{
    CycleTimerConfig_t cycle_timer;
    int slot_index = -1;
    uint8_t valid_count = 0;
    
    // ?????隆矛????????????????????隆陋?
    for (int i = 0; i < MAX_CYCLE_TIMERS; i++) {
        if (g_cycle_timers[i].timer_id != 0 && g_cycle_timers[i].week_mask != 0) {
            valid_count++;
        }
    }
    
    // ?????2???????????????
    if (valid_count >= 2) {
        //DBG_UART2("????????????2??????????????????\n");
        return;
    }
    
    // ?????????
    //DBG_UART2("Cycle_Store raw data: ");
    for (int i = 0; i < 13; i++) {
        //DBG_UART2("%02X ", data[i]);
    }
    //DBG_UART2("\n");
    
    // ??????????????
    cycle_timer.reserved0 = data[0];                    // byte0: ????
    cycle_timer.enable = data[2];                       // byte2: ????
    cycle_timer.week_mask = data[3];                    // byte3: ????????
    cycle_timer.start_time = (uint16_t)data[4] << 8 | data[5];   // byte4-5: ??????
    cycle_timer.end_time = (uint16_t)data[6] << 8 | data[7];     // byte6-7: ???????
    cycle_timer.water_time = (uint16_t)data[8] << 8 | data[9];   // byte8-9: ??????
    cycle_timer.soak_time = (uint16_t)data[10] << 8 | data[11];  // byte10-11: ???????
    cycle_timer.valve_percent = data[12];               // byte12: ???????
    
    // ??????????
    //DBG_UART2("????: enable=%d, week_mask=0x%02X, \
	start=%02d:%02d, end=%02d:%02d, water=%d, soak=%d, valve=%d%%\n",\
           cycle_timer.enable,\
           cycle_timer.week_mask,\
           cycle_timer.start_time / 60,\
           cycle_timer.start_time % 60,\
           cycle_timer.end_time / 60,\
           cycle_timer.end_time % 60,\
           cycle_timer.water_time,\
           cycle_timer.soak_time,\
           cycle_timer.valve_percent);
    
    // ???????????????????0????隆矛????隆矛篓陇?????0???隆矛篓陇?????1
    if (g_cycle_timers[0].timer_id == 0 || g_cycle_timers[0].week_mask == 0) {
        slot_index = 0;  // ????隆猫
        cycle_timer.timer_id = 1;
        //DBG_UART2("?????????隆猫??????\n");
    } else if (g_cycle_timers[1].timer_id == 0 || g_cycle_timers[1].week_mask == 0) {
        slot_index = 1;  // ????隆猫
        cycle_timer.timer_id = 2;
        //DBG_UART2("?????????隆猫??????\n");
    } else {
        // ???隆猫???隆矛????????
        //DBG_UART2("???隆猫????????????\n");
        return;
    }
    
    // week_mask == 0 ????
    if (cycle_timer.week_mask == 0) {
        memset(&g_cycle_timers[slot_index], 0, sizeof(CycleTimerConfig_t));
        //DBG_UART2("??????%d week_mask=0???????\n", slot_index + 1);
        SaveCycleToEEPROM(slot_index);
        return;
    }
    
    // ????????
    memcpy(&g_cycle_timers[slot_index], &cycle_timer, sizeof(CycleTimerConfig_t));
    
    //DBG_UART2("??????%d??????: ID=%d, ????=%d, \
	??????=0x%02X, ???=%02d:%02d, ????=%02d:%02d, ???=%d????, \
	????=%d????, ????=%d%%\n",\
           slot_index + 1,\
           cycle_timer.timer_id,\
           cycle_timer.enable,\
           cycle_timer.week_mask,\
           cycle_timer.start_time / 60,\
           cycle_timer.start_time % 60,\
           cycle_timer.end_time / 60,\
           cycle_timer.end_time % 60,\
           cycle_timer.water_time,\
           cycle_timer.soak_time,\
           cycle_timer.valve_percent);
    
    // ?????EEPROM
    SaveCycleToEEPROM(slot_index);
}
/**
 * @brief ?????????????????????????
 * @param buffer ?????????
 * @return ???????????1?隆猫=13??2?隆猫=25??0?隆猫=13?0??
 */uint16_t Cycle_GetList(uint8_t* buffer)
{
    uint16_t offset = 0;
    uint8_t valid_count = 0;
    
    // ???????隆矛?????
    for (int i = 0; i < MAX_CYCLE_TIMERS; i++) {
        if (g_cycle_timers[i].timer_id != 0 && g_cycle_timers[i].week_mask != 0) {
            valid_count++;
        }
    }
    
    //DBG_UART2("????????隆矛?????: %d\n", valid_count);
    
    if (valid_count == 0) {
        // ??隆矛???????????13????0
        memset(buffer, 0, 15);
        return 15;
    }
    
    // ????隆猫???????隆矛???13????
    if (g_cycle_timers[0].timer_id != 0 && g_cycle_timers[0].week_mask != 0) {
        buffer[offset + 0] = g_cycle_timers[0].reserved0;
        buffer[offset + 1] = g_cycle_timers[0].timer_id;
        buffer[offset + 2] = g_cycle_timers[0].enable;
        buffer[offset + 3] = g_cycle_timers[0].week_mask;
        buffer[offset + 4] = (g_cycle_timers[0].start_time >> 8) & 0xFF;
        buffer[offset + 5] = g_cycle_timers[0].start_time & 0xFF;
        buffer[offset + 6] = (g_cycle_timers[0].end_time >> 8) & 0xFF;
        buffer[offset + 7] = g_cycle_timers[0].end_time & 0xFF;
        buffer[offset + 8] = (g_cycle_timers[0].water_time >> 8) & 0xFF;
        buffer[offset + 9] = g_cycle_timers[0].water_time & 0xFF;
        buffer[offset + 10] = (g_cycle_timers[0].soak_time >> 8) & 0xFF;
        buffer[offset + 11] = g_cycle_timers[0].soak_time & 0xFF;
        buffer[offset + 12] = g_cycle_timers[0].valve_percent;
        offset += 13;
        
        //DBG_UART2("????隆猫???: timer_id=%d, ????=%d, ??????=0x%02X\n",\
               g_cycle_timers[0].timer_id,\
               g_cycle_timers[0].enable,\
               g_cycle_timers[0].week_mask);
    }
    // ????隆猫???????隆矛????????隆猫??隆矛???????13????????
    else if (g_cycle_timers[1].timer_id != 0 && g_cycle_timers[1].week_mask != 0) {
        buffer[offset + 0] = g_cycle_timers[1].reserved0;
        buffer[offset + 1] = g_cycle_timers[1].timer_id;
        buffer[offset + 2] = g_cycle_timers[1].enable;
        buffer[offset + 3] = g_cycle_timers[1].week_mask;
        buffer[offset + 4] = (g_cycle_timers[1].start_time >> 8) & 0xFF;
        buffer[offset + 5] = g_cycle_timers[1].start_time & 0xFF;
        buffer[offset + 6] = (g_cycle_timers[1].end_time >> 8) & 0xFF;
        buffer[offset + 7] = g_cycle_timers[1].end_time & 0xFF;
        buffer[offset + 8] = (g_cycle_timers[1].water_time >> 8) & 0xFF;
        buffer[offset + 9] = g_cycle_timers[1].water_time & 0xFF;
        buffer[offset + 10] = (g_cycle_timers[1].soak_time >> 8) & 0xFF;
        buffer[offset + 11] = g_cycle_timers[1].soak_time & 0xFF;
        buffer[offset + 12] = g_cycle_timers[1].valve_percent;
        offset += 13;
        
        //DBG_UART2("????隆猫???(????): timer_id=%d, ????=%d, ??????=0x%02X\n",\
               g_cycle_timers[1].timer_id,\
               g_cycle_timers[1].enable,\
               g_cycle_timers[1].week_mask);
    }
    
    // ???隆猫???????????隆猫??12????????? reserved0??
    if (g_cycle_timers[0].timer_id != 0 && g_cycle_timers[0].week_mask != 0 &&
        g_cycle_timers[1].timer_id != 0 && g_cycle_timers[1].week_mask != 0) {
        
        // ????隆猫????????13????offset=13
        // ????隆猫??12?????? timer_id ???????? reserved0??
        buffer[offset + 0] = g_cycle_timers[1].timer_id;
        buffer[offset + 1] = g_cycle_timers[1].enable;
        buffer[offset + 2] = g_cycle_timers[1].week_mask;
        buffer[offset + 3] = (g_cycle_timers[1].start_time >> 8) & 0xFF;
        buffer[offset + 4] = g_cycle_timers[1].start_time & 0xFF;
        buffer[offset + 5] = (g_cycle_timers[1].end_time >> 8) & 0xFF;
        buffer[offset + 6] = g_cycle_timers[1].end_time & 0xFF;
        buffer[offset + 7] = (g_cycle_timers[1].water_time >> 8) & 0xFF;
        buffer[offset + 8] = g_cycle_timers[1].water_time & 0xFF;
        buffer[offset + 9] = (g_cycle_timers[1].soak_time >> 8) & 0xFF;
        buffer[offset + 10] = g_cycle_timers[1].soak_time & 0xFF;
        buffer[offset + 11] = g_cycle_timers[1].valve_percent;
        offset += 12;
        
        //DBG_UART2("????隆猫???(???隆猫): timer_id=%d, ????=%d, ??????=0x%02X\n",\
               g_cycle_timers[1].timer_id,\
               g_cycle_timers[1].enable,\
               g_cycle_timers[1].week_mask);
    }
    
    //DBG_UART2("?????????: ??%d?隆猫, ?????=%d\n", valid_count, offset);
    
    return offset;
}

void Cycle_Delete_All(void)
{
    // 清空所有周期定时器
    for (uint8_t i = 0; i < MAX_CYCLE_TIMERS; i++) {
        memset(&g_cycle_timers[i], 0, sizeof(CycleTimerConfig_t));
        SaveCycleToEEPROM(i);
    }
		
}

void Cycle_Delete(uint8_t timer_id)
{
    int index;
    
    // ???? timer_id ?????????1->0, 2->1??
    if (timer_id == 1) {
        index = 0;
    } else if (timer_id == 2) {
        index = 1;
    } else {
        //DBG_UART2("???????????????隆矛??? timer_id=%d\n", timer_id);
        return;
    }
    
    // ????????????
    if (g_cycle_timers[index].timer_id == 0) {
        //DBG_UART2("??????%d?????????????\n", index + 1);
        return;
    }
    
    // ???
    memset(&g_cycle_timers[index], 0, sizeof(CycleTimerConfig_t));
    
    
    SaveCycleToEEPROM(index);
    
    //DBG_UART2("??????%d?????\n", index + 1);
}
uint8_t Cycle_ModifyFromServer(const uint8_t* data)
{
    // ??????????? Cycle_Store ??????篓潞?????????????
    uint8_t switch_state = data[3];       // byte3: ??????
    uint8_t week_mask = data[4];          // byte4: ????????
    uint16_t start_time = (uint16_t)data[5] << 8 | data[6];   // byte5-6: ??????
    uint16_t end_time = (uint16_t)data[7] << 8 | data[8];     // byte7-8: ???????
    uint16_t water_time = (uint16_t)data[9] << 8 | data[10];  // byte9-10: ??????
    uint16_t soak_time = (uint16_t)data[11] << 8 | data[12];  // byte11-12: ???????
    uint8_t valve_percent = data[13];     // byte13: ???????
    
    //DBG_UART2("????????????: switch=%d, week_mask=0x%02X, \
	start=%02d:%02d, end=%02d:%02d, water=%d, soak=%d, valve=%d%%\n",\
           switch_state, week_mask,\
           start_time / 60, start_time % 60,\
           end_time / 60, end_time % 60,\
           water_time, soak_time, valve_percent);
    
    // ????????????????????? week_mask ?? start_time ?????
    int index = -1;
    for (int i = 0; i < MAX_CYCLE_TIMERS; i++) {
        if (g_cycle_timers[i].week_mask == week_mask && 
            g_cycle_timers[i].start_time == start_time &&
            g_cycle_timers[i].timer_id != 0) {
            index = i;
            break;
        }
    }
    
    // ???????????????????
    if (index == -1) {
        for (int i = 0; i < MAX_CYCLE_TIMERS; i++) {
            if (g_cycle_timers[i].timer_id != 0) {
                index = i;
                break;
            }
        }
    }
    
    // ????????????????????
    if (index == -1) {
        //DBG_UART2("??????????????????????????\n");
        return false;
    }
    
    // ??????????????
    CycleTimerConfig_t cycle_timer;
    memset(&cycle_timer, 0, sizeof(CycleTimerConfig_t));
    cycle_timer.reserved0 = data[0];                    // byte0: ????
    cycle_timer.timer_id = (index == 0) ? 1 : 2;        // ID??1???
    cycle_timer.enable = switch_state;
    cycle_timer.week_mask = week_mask;
    cycle_timer.start_time = start_time;
    cycle_timer.end_time = end_time;
    cycle_timer.water_time = water_time;
    cycle_timer.soak_time = soak_time;
    cycle_timer.valve_percent = valve_percent;
    
    // week_mask == 0 ????
    if (week_mask == 0) {
        memset(&g_cycle_timers[index], 0, sizeof(CycleTimerConfig_t));
        //DBG_UART2("??????%d week_mask=0???????\n", index + 1);
        SaveCycleToEEPROM(index);
        return true;
    }
    
    // ????????
    memcpy(&g_cycle_timers[index], &cycle_timer, sizeof(CycleTimerConfig_t));
    
    SaveCycleToEEPROM(index);
    
    return true;
}


bool Timer_Store(const uint8_t* data)
{
    TimerConfig_t new_timer;
    int slot_index;
    
    
    new_timer.reserved0 = data[0];
    new_timer.timer_id = 0;
    new_timer.start_time = (uint16_t)data[2] << 8 | data[3];      // ????????
    new_timer.irrigation_time = (uint16_t)data[4] << 8 | data[5]; // ????????
    new_timer.week_mask = data[6];
    new_timer.valve_percent = data[7];
    for (int i = 0; i < 5; i++) {
        new_timer.reserved[i] = data[8 + i];
    }
    
    // week_mask == 0 ???????????
    if (new_timer.week_mask == 0) {
        //DBG_UART2("week_mask=0????????????\n");
        return false;
    }
    
    slot_index = FindFreeSlot();
    if (slot_index < 0) {
		
        return false;
    }
    
    new_timer.timer_id = slot_index + 1;
    
    memcpy(&g_timers[slot_index], &new_timer, TIMER_SLOT_SIZE);
    SaveSlot(slot_index);
	
	//DBG_UART2("??????: ID=%d, ???=%02d:%02d, ??????=0x%02X\n",\
           new_timer.timer_id,\
           new_timer.start_time / 60,\
           new_timer.start_time % 60,\
           new_timer.week_mask);
	
    return true;
}

// ?????????????????ID??
uint8_t Timer_Modify(uint8_t timer_id, const uint8_t* data)
{
    int slot = Timer_FindSlotByID(timer_id);
    if (slot < 0) {
        //DBG_UART2("???????????? timer_id=%d\n", timer_id);
        return false;
    }
    
    // ??????????
    TimerConfig_t new_timer;
    new_timer.reserved0 = data[0];
    new_timer.timer_id = timer_id;  // ?????ID
    new_timer.start_time = (uint16_t)data[2] << 8 | data[3];
    new_timer.irrigation_time = (uint16_t)data[4] << 8 | data[5];
    new_timer.week_mask = data[6];
    new_timer.valve_percent = data[7];
    for (int i = 0; i < 5; i++) {
        new_timer.reserved[i] = data[8 + i];
    }
    
    // week_mask == 0 ???????????
    if (new_timer.week_mask == 0) {
        //DBG_UART2("week_mask=0????????????\n");
        return false;
    }
	
    // ????????EEPROM
    memcpy(&g_timers[slot], &new_timer, TIMER_SLOT_SIZE);
    SaveSlot(slot);
    
    //DBG_UART2("???????timer_id=%d, ????=%d\n", timer_id, slot);
    
    return true;
}
uint16_t Timer_GetList(uint8_t* buffer)
{
    uint16_t offset = 0;
    
    for (int i = 0; i < MAX_TIMER_SLOTS; i++) {
        if (g_timers[i].timer_id != 0) {
            buffer[offset + 0] = g_timers[i].reserved0;
            buffer[offset + 1] = g_timers[i].timer_id;
            buffer[offset + 2] = (g_timers[i].start_time >> 8) & 0xFF;  // ?????
            buffer[offset + 3] = g_timers[i].start_time & 0xFF;          // ?????
            buffer[offset + 4] = (g_timers[i].irrigation_time >> 8) & 0xFF;
            buffer[offset + 5] = g_timers[i].irrigation_time & 0xFF;
            buffer[offset + 6] = g_timers[i].week_mask;
            buffer[offset + 7] = g_timers[i].valve_percent;
            for (int j = 0; j < 5; j++) {
                buffer[offset + 8 + j] = g_timers[i].reserved[j];
            }
            offset += TIMER_SLOT_SIZE;
        }
    }
    
    return offset;
}


int Timer_FindSlotByID(uint8_t timer_id)
{
    for (int i = 0; i < MAX_TIMER_SLOTS; i++) {
        if (g_timers[i].timer_id == timer_id) {
            //DBG_UART2("???????? ID=%d ????? %d\n", timer_id, i);
            return i;
        }
    }
    //DBG_UART2("?????????? ID=%d\n", timer_id);
    return -1;
}

bool Timer_DeleteAll(void)
{
    //DBG_UART2("\n========== 删除所有普通定时器 ==========\n");
    
    // 清空内存中的所有定时器
    memset(g_timers, 0, sizeof(g_timers));
    
    // 保存到EEPROM
    for (int i = 0; i < MAX_TIMER_SLOTS; i++) {
        SaveSlot(i);
    }
    
	Cycle_Delete_All();
    //DBG_UART2("已删除所有普通定时器\n");
    //DBG_UART2("========================================\n\n");
    
    return true;
}

bool Timer_Delete(uint8_t timer_id)
{
    int slot = Timer_FindSlotByID(timer_id);
    if (slot < 0) {
        return false;
    }
	
    CompactAfterDelete(slot);
    
    return true;
}

int DeleteTimer(int timer_id)
{
    for (int i = 0; i < MAX_TIMER_SLOTS; i++) {
        if (g_timers[i].timer_id == timer_id) {
            // 直接清空这个槽位，不移动其他数据
            memset(&g_timers[i], 0, TIMER_SLOT_SIZE);
            SaveSlot(i);
            return 0;  // 成功
        }
    }
    return -1;  // 未找到
}


int Timer_GetCount(void)
{
    int count = 0;
    for (int i = 0; i < MAX_TIMER_SLOTS; i++) {
        if (g_timers[i].timer_id != 0) {
            count++;
        }
    }
    return count;
}

// ???????????隆矛??????
static bool is_leap_year(uint16_t year)
{
    return (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
}

// ????????????????????????????????????2000-01-01 00:00?????
static uint32_t date_to_minutes(uint16_t year, uint8_t month, uint8_t day, uint16_t minutes)
{
    uint32_t total_days = 0;
    
    // ???????????
    for (uint16_t y = 2000; y < year; y++) {
        total_days += is_leap_year(y) ? 366 : 365;
    }
    
    // ?????隆猫??????
    uint8_t month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (is_leap_year(year)) {
        month_days[1] = 29;
    }
    for (uint8_t m = 1; m < month; m++) {
        total_days += month_days[m-1];
    }
    
    // ???????
    total_days += (day - 1);
    
    // ????????????????????
    return total_days * 1440 + minutes;
}

void get_current_datetime(uint16_t *year, uint8_t *month, uint8_t *day, uint16_t *minutes)
{
    RTC_DateTypeDef rtc_date;
    RTC_TimeTypeDef rtc_time;
    
    // ???RTC??????????STM32 HAL???????
    HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
    
    *year = 2000 + rtc_date.Year;  // RTC???????????????
    *month = rtc_date.Month;
    *day = rtc_date.Date;
    *minutes = rtc_time.Hours * 60 + rtc_time.Minutes;
}

// ??????????????? 1=???, 2=???, 3=????, 4=????, 5=????, 6=????, 7=????
uint8_t calculate_weekday(uint16_t year, uint8_t month, uint8_t day)
{
    if (month < 3) {
        month += 12;
        year -= 1;
    }
    
    uint16_t weekday = (day + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400) % 7;
    // weekday: 0=???, 1=???, 2=????, 3=????, 4=????, 5=????, 6=????
    
    return weekday + 1;  // ???? 1=??? ... 7=????
}

// ??????????????????????
const char* get_weekday_string(uint8_t weekday)
{
    static const char* weekdays[] = {"", "?", "??", "??", "??", "??", "??", "??"};
    return weekdays[weekday];
}
uint8_t Timer_NeedRun(TimerConfig_t *timer)
{
    if (timer == NULL || timer->timer_id == 0) {
        return 0;
    }
    
    if (timer->week_mask == 0) {
        return 0;
    }
    
    // 获取当前时间
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    
    uint16_t current_year = 2000 + sDate.Year;
    uint8_t current_month = sDate.Month;
    uint8_t current_day = sDate.Date;
    uint16_t current_minutes = sTime.Hours * 60 + sTime.Minutes;
    uint8_t current_weekday = calculate_weekday(current_year, current_month, current_day);
    
    if (current_year == 2000) {
        return 0;
    }
    
    uint16_t timer_minutes = timer->start_time;
    uint8_t week_mask = timer->week_mask;
    
    uint8_t current_bit;
    if (current_weekday == 7) {
        current_bit = 0;
    } else {
        current_bit = current_weekday;
    }
    
   if ((week_mask & (1 << current_bit)) && (timer_minutes == current_minutes)) {
//    DBG_UART2(">>> 定时器ID:%d 触发执行！\n", timer->timer_id);
    return 1;
} else {
//    // 打印设置的星期和时间
//    DBG_UART2("定时器ID:%d 未触发 - 设定时间:%02d:%02d 设定星期:", 
//              timer->timer_id, timer_minutes / 60, timer_minutes % 60);
//    
//    // 打印设置的星期
//    if (week_mask & (1<<0)) DBG_UART2("日");
//    if (week_mask & (1<<1)) DBG_UART2("一");
//    if (week_mask & (1<<2)) DBG_UART2("二");
//    if (week_mask & (1<<3)) DBG_UART2("三");
//    if (week_mask & (1<<4)) DBG_UART2("四");
//    if (week_mask & (1<<5)) DBG_UART2("五");
//    if (week_mask & (1<<6)) DBG_UART2("六");
//    
//    DBG_UART2("\n");
}
    
    return 0;
}

TimerConfig_t* Timer_GetNearest(void)
{
//    DBG_UART2("\n========== Timer_GetNearest ???????????? ==========\n");
    
    TimerConfig_t* nearest = NULL;
    uint32_t min_diff = 0xFFFFFFFF;
    uint32_t diff;
    uint16_t current_minutes;
    uint8_t current_weekday;
    
    uint16_t current_year;
    uint8_t current_month;
    uint8_t current_day;
    
    get_current_datetime(&current_year, &current_month, &current_day, &current_minutes);
    current_weekday = calculate_weekday(current_year, current_month, current_day);
    
//    DBG_UART2("??????: %04d-%02d-%02d %02d:%02d\n",
//              current_year, current_month, current_day,
//              current_minutes / 60, current_minutes % 60);
//    DBG_UART2("???????: %d (%s)\n", current_weekday, get_weekday_string(current_weekday));
//    DBG_UART2("?????????: %d\n\n", current_minutes);
    
    int timer_count = 0;
    for (int i = 0; i < MAX_TIMER_SLOTS; i++) {
        if (g_timers[i].timer_id == 0) {
            continue;
        }
        
        // ??? reserved[0] ??????? 1
        if (g_timers[i].reserved[0] != 1) {
            //DBG_UART2("????%d: timer_id=%d, reserved[0]=%d (???????????)\n", \
                      i, g_timers[i].timer_id, g_timers[i].reserved[0]);
            continue;
        }
        
        if (g_timers[i].week_mask == 0) {
            //DBG_UART2("????%d: timer_id=%d, week_mask=0 (????)\n", \
                      i, g_timers[i].timer_id);
            continue;
        }
        
        timer_count++;
        // ????????????
        uint16_t timer_minutes = g_timers[i].start_time;
        uint8_t week_mask = g_timers[i].week_mask;
        
        //DBG_UART2("????%d: timer_id=%d, ???=%02d:%02d, ??????=0x%02X\n",\
                  i, g_timers[i].timer_id,\
                  timer_minutes / 60, timer_minutes % 60,\
                  week_mask);
        
        uint32_t best_diff = 0xFFFFFFFF;
        
        for (int offset_days = 0; offset_days < 7; offset_days++) {
            uint8_t target_rtc_weekday = current_weekday + offset_days;
            if (target_rtc_weekday > 7) {
                target_rtc_weekday -= 7;
            }
            
            uint8_t target_bit;
            if (target_rtc_weekday == 7) {
                target_bit = 0;
            } else {
                target_bit = target_rtc_weekday;
            }
            
            if (week_mask & (1 << target_bit)) {
                if (offset_days == 0) {
                    if (timer_minutes >= current_minutes) {
                        diff = timer_minutes - current_minutes;
                        //DBG_UART2("  ???? %d?????????: diff=%d????\n", \
                                  offset_days, diff);
                    } else {
                        diff = 7 * 1440 + timer_minutes - current_minutes;
                        //DBG_UART2("  ???????????????????: diff=%d????\n", diff);
                    }
                } else {
                    diff = offset_days * 1440 + timer_minutes - current_minutes;
                    //DBG_UART2("  %d?????????: diff=%d????\n", offset_days, diff);
                }
                
                if (diff < best_diff) {
                    best_diff = diff;
                }
                break;
            }
        }
        
        if (best_diff != 0xFFFFFFFF) {
           // DBG_UART2("  ?????: %d???? (%d?? %d隆矛?? %d????)\n",\
                      best_diff, best_diff / 1440, \
                      (best_diff % 1440) / 60, best_diff % 60);
        }
        
        if (best_diff != 0xFFFFFFFF && best_diff < min_diff) {
            min_diff = best_diff;
            nearest = &g_timers[i];
           // DBG_UART2("  *** ???????????: ID=%d, ???=%d???? ***\n", \
                      g_timers[i].timer_id, min_diff);
        }
      //  DBG_UART2("\n");
    }
    
    if (nearest != NULL) {
//        DBG_UART2("========== ??????????? ==========\n");
//        DBG_UART2("?????ID: %d\n", nearest->timer_id);
//        DBG_UART2("??????: %02d:%02d\n", 
//                  nearest->start_time / 60, nearest->start_time % 60);
//        DBG_UART2("??????: %d????\n", nearest->irrigation_time);
//        DBG_UART2("??????: 0x%02X\n", nearest->week_mask);
//        DBG_UART2("???????: %d%%\n", nearest->valve_percent);
//        DBG_UART2("???????: %d???? (%d?? %d隆矛?? %d????)\n",
//                  min_diff, min_diff / 1440,
//                  (min_diff % 1440) / 60, min_diff % 60);
//        DBG_UART2("====================================\n\n");
    } else {
//        DBG_UART2("========== ???????隆矛??????? ==========\n\n");
    }
    
    return nearest;
}

// ?? timer_manager.c ?????????????

uint16_t cycle_tmp_start_time = 0;  // ????????????
uint8_t cycle_phase[MAX_CYCLE_TIMERS] = {0};  // 0=????, 1=??????, 2=??????
// ????????????
void OnWaterComplete(uint8_t timer_id)
{
    uint8_t idx = (timer_id == 1) ? 0 : 1;
	uint16_t minute;
    CycleTimerConfig_t *cycle_timer = &g_cycle_timers[idx];
    

	get_current_datetime(0,0,0, &minute);
	if (minute >= cycle_timer->end_time) {
		cycle_timer->start_time = 
		cycle_phase[idx] = 2;
        // ???
        cycle_timer->start_time = cycle_tmp_start_time;
		//DBG_UART2("cycle_timer->start_time = %d:%dX \r\n", cycle_timer->start_time / 60,  cycle_timer->start_time % 60);
        valve_takeover(IRR_SRC_NONE, 0, cycle_timer->soak_time * 60);
		Timer_SetAlarmByNearest();
		return;
	}
    if (cycle_phase[idx] == 1) {
        if (minute + cycle_timer->water_time > cycle_timer->end_time) {
            // 不够时间完成灌溉，提前结束
            DBG_UART2("不够时间完成灌溉，提前结束,当前浸泡 \n");
            cycle_phase[idx] = 0;
            cycle_timer->start_time = cycle_tmp_start_time;
            valve_takeover(IRR_SRC_NONE, 0, 0);
            Timer_SetAlarmByNearest();
            return;
        }
        cycle_phase[idx] = 2;
        
        // ???
        //DBG_UART2("????????????=%d????\n", cycle_timer->soak_time);
        valve_takeover(IRR_SRC_CYCLE, 0, cycle_timer->soak_time * 60);
        
        // ?????????????????????????? + ???????
        
        cycle_timer->start_time = cycle_timer->start_time + cycle_timer->soak_time;
        if (cycle_timer->start_time >= 1440) {
            cycle_timer->start_time -= 1440;
        }
        //DBG_UART2("?????????: %02d:%02d -> %02d:%02d\n",\
                  old_start_time / 60, old_start_time % 60,\
                  cycle_timer->start_time / 60, cycle_timer->start_time % 60);
        
    } else if (cycle_phase[idx] == 2) {
        if (minute + cycle_timer->water_time > cycle_timer->end_time) {
            DBG_UART2("不够时间完成灌溉，提前结束,当前灌水 \n");
            cycle_phase[idx] = 0;
            cycle_timer->start_time = cycle_tmp_start_time;
            valve_takeover(IRR_SRC_NONE, 0, 0);
            Timer_SetAlarmByNearest();
            return;
        }
        cycle_phase[idx] = 1;
			 // ????
		
		valve_takeover(IRR_SRC_CYCLE, 1, cycle_timer->water_time * 60);
		
		// ????????????????????????? + ??????
		
		cycle_timer->start_time = cycle_timer->start_time + cycle_timer->water_time;
		if (cycle_timer->start_time >= 1440) {
			cycle_timer->start_time -= 1440;
		}
		//DBG_UART2("?????????: %02d:%02d -> %02d:%02d\n",\
				  old_start_time / 60, old_start_time % 60,\
				  cycle_timer->start_time / 60, cycle_timer->start_time % 60);
		
    } else {
        //DBG_UART2("????????????? %d\n", cycle_phase[idx]);
    }

    Timer_SetAlarmByNearest();
    //DBG_UART2("========================================\n\n");
}


int Timer_SetAlarmByNearest(void)
{
    NearestTimer_t nearest = Timer_GetNearestAll();
    
    if (nearest.timer_id == 0) {
        return -1;
    }
    
    uint16_t timer_minutes = nearest.start_time;
    uint8_t week_mask = nearest.week_mask;
    uint8_t action = nearest.action;
    uint8_t type = nearest.type;
    
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    
    uint16_t current_year = 2000 + sDate.Year;
    uint8_t current_month = sDate.Month;
    uint8_t current_day = sDate.Date;
    uint16_t current_minutes = sTime.Hours * 60 + sTime.Minutes;
    uint8_t current_weekday = calculate_weekday(current_year, current_month, current_day);
    
    uint8_t target_stm32_weekday = nearest.target_weekday;
    
    RTC_AlarmTypeDef sAlarm = {0};
    
    sAlarm.AlarmTime.Hours = timer_minutes / 60;
    sAlarm.AlarmTime.Minutes = timer_minutes % 60;
    sAlarm.AlarmTime.Seconds = 0;
    
    sAlarm.AlarmMask = RTC_ALARMMASK_SECONDS;
    sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY;
    sAlarm.AlarmDateWeekDay = target_stm32_weekday;
    sAlarm.Alarm = RTC_ALARM_A;
    
    if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK) {
        return -1;
    }

    //static uint16_t count = 0;
    DBG_UART2("下次定时器=[星期 %d], %02d:%02d \r\n", target_stm32_weekday, \
	timer_minutes / 60, timer_minutes % 60);
	
    return 0;
}


NearestTimer_t Timer_GetNearestAll(void)
{
    NearestTimer_t result;
    result.type = 0;
    result.timer_id = 0;
    result.start_time = 0;
    result.week_mask = 0;
    result.action = 0;
    result.valve_percent = 0;
    result.end_time = 0;
    result.target_weekday = 0;
	result.year = 0;
	result.month = 0;
	result.day = 0;
    
    uint32_t min_diff = 0xFFFFFFFF;
    uint32_t diff;
    uint16_t current_minutes;
    uint8_t current_weekday;
    
    // 获取当前时间
    uint16_t current_year;
    uint8_t current_month;
    uint8_t current_day;
    
    get_current_datetime(&current_year, &current_month, &current_day, &current_minutes);
    current_weekday = calculate_weekday(current_year, current_month, current_day);

    //DBG_UART2("\n========== Timer_GetNearestAll ==========\n");
    //DBG_UART2("当前时间: %04d-%02d-%02d %02d:%02d (星期%d)\n",\
    //          current_year, current_month, current_day,\
    //          current_minutes / 60, current_minutes % 60, current_weekday);

    // ========== 1. 普通定时器 ==========
    for (int i = 0; i < MAX_TIMER_SLOTS; i++) {
        if (g_timers[i].timer_id == 0) continue;
        if (g_timers[i].reserved[0] != 1) continue;
        if (g_timers[i].week_mask == 0) continue;
        
        uint16_t timer_minutes = g_timers[i].start_time;
        uint8_t week_mask = g_timers[i].week_mask;

        //DBG_UART2("\n[普通定时器 %d] ID=%d, 开始=%02d:%02d, week_mask=0x%02X\n",
        //          i, g_timers[i].timer_id, timer_minutes/60, timer_minutes%60, week_mask);

        // 找到最近的有效日期
        for (int offset_days = 0; offset_days < 7; offset_days++) {
            uint8_t target_weekday = current_weekday + offset_days;
            if (target_weekday > 7) target_weekday -= 7;
            
            uint8_t target_bit;
            if (target_weekday == 7) {
                target_bit = 0;
            } else {
                target_bit = target_weekday;
            }
            
            if (week_mask & (1 << target_bit)) {
                
                if (offset_days == 0) {
                    if (timer_minutes > current_minutes) {
                        // 今天，时间未过
                        diff = timer_minutes - current_minutes;
                    } else {
                        // 今天已过，跳过，继续找明天
                        continue;
                    }
                } else {
                    // 未来几天
                    diff = offset_days * 1440 + timer_minutes - current_minutes;
                }
                
                if (diff < min_diff) {
                    min_diff = diff;
                    result.type = 0;
                    result.timer_id = g_timers[i].timer_id;
                    result.start_time = timer_minutes;
                    result.week_mask = week_mask;
                    result.action = Timer_GetAction(&g_timers[i]);
                    result.valve_percent = g_timers[i].valve_percent;
                    result.target_weekday = target_weekday;  // 直接用循环计算出的值
                }
                break;  // 找到这个定时器的最近有效时间
            }
        }
    }
    
    // ========== 2. 周期定时器 ==========
    for (int i = 0; i < MAX_CYCLE_TIMERS; i++) {
        if (g_cycle_timers[i].timer_id == 0) continue;
        if (g_cycle_timers[i].enable != 1) continue;
        if (g_cycle_timers[i].week_mask == 0) continue;
        
        uint16_t timer_minutes = g_cycle_timers[i].start_time;
        uint16_t end_time = g_cycle_timers[i].end_time;
        uint8_t week_mask = g_cycle_timers[i].week_mask;

        uint16_t water_time = g_cycle_timers[i].water_time;
        uint16_t soak_time = g_cycle_timers[i].soak_time;
        uint16_t cycle_duration = water_time + soak_time;

        // 找到最近的有效日期
        bool found = false;
        for (int offset_days = 0; offset_days < 7; offset_days++) {
            uint8_t target_weekday = current_weekday + offset_days;
            if (target_weekday > 7) target_weekday -= 7;
            
            uint8_t target_bit;
            if (target_weekday == 7) {
                target_bit = 0;
            } else {
                target_bit = target_weekday;
            }
            
            if (week_mask & (1 << target_bit)) {
                //DBG_UART2("  检查星期%d (offset_days=%d)\n", target_weekday, offset_days);

                if (offset_days == 0) {
                    // 当天处理逻辑
                    if (current_minutes >= end_time) {
                        // 已过结束时间，跳过今天，继续找明天
                        //DBG_UART2("    当前时间(%02d:%02d) >= 结束时间(%02d:%02d), 跳过今天\n",
                        //          current_minutes/60, current_minutes%60, end_time/60, end_time%60);
					    //valve_takeover(IRR_SRC_NONE, 0, 0);
						//DBG_UART2("ID = %d", g_cycle_timers[i].timer_id);
                        continue;  // 继续循环，找offset_days=1
                    } else if (current_minutes < timer_minutes) {
                        // 还没到开始时间，使用今天的开始时间
                        diff = timer_minutes - current_minutes;
                        //DBG_UART2("    当前时间 < 开始时间, diff=%d分钟\n", diff);
                        found = true;
                    } else {
                        // 在运行区间内
                        uint16_t elapsed = current_minutes - timer_minutes;
                    
                        // 判断总的周期超时
                        if (timer_minutes + cycle_duration > end_time) {
                            // 跨天周期，需要重新计算
                            //DBG_UART2("  跨天周期超时\n");
                            continue;
                        } else {
                            //DBG_UART2("  在运行区间内, elapsed=%d分钟\n", elapsed);
                        }
                        
                        if (elapsed > 1) {
                            continue;
                        } else {
                            diff = 0;
                            found = true;
                        }
                    }
            } else {
                // 未来几天
                diff = offset_days * 1440 + timer_minutes - current_minutes;
                //DBG_UART2("    %d天后, diff=%d分钟\n", offset_days, diff);
                found = true;
            }

            if (found) {
                if (diff < min_diff) {
                    min_diff = diff;
                    result.type = 1;
                    result.timer_id = g_cycle_timers[i].timer_id;
                    result.start_time = timer_minutes;
                    result.week_mask = week_mask;
                    result.action = TIMER_ACTION_RUN;
                    result.valve_percent = g_cycle_timers[i].valve_percent;
                    result.end_time = end_time;
                    result.target_weekday = target_weekday;
                    //DBG_UART2("    *** 更新最近定时器，diff=%d分钟 ***\n", diff);
                }
                break;  // 找到最近的就退出
            }
            }
        }
    }

    //DBG_UART2("\n========== 最终结果 ==========\n");
    if (min_diff == 0xFFFFFFFF) {
        //DBG_UART2("没有找到任何定时器！\n");
    } else {
        //DBG_UART2("type=%d, ID=%d, 星期%d, 时间=%02d:%02d, diff=%d分钟\n",
        //          result.type, result.timer_id, result.target_weekday,
        //          result.start_time / 60, result.start_time % 60, min_diff);
    }
    
    return result;
}

CycleTimerConfig_t* CycleTimer_GetNearest(void)
{
    CycleTimerConfig_t* nearest = NULL;
    uint32_t min_diff = 0xFFFFFFFF;
    uint32_t diff;
    uint16_t current_minutes;
    uint8_t current_weekday;
    
    uint16_t current_year;
    uint8_t current_month;
    uint8_t current_day;
    
    //DBG_UART2("\n========== CycleTimer_GetNearest ????????????? ==========\n");
    
    get_current_datetime(&current_year, &current_month, &current_day, &current_minutes);
    current_weekday = calculate_weekday(current_year, current_month, current_day);
    
    //DBG_UART2("??????: %04d-%02d-%02d %02d:%02d\n",
    //          current_year, current_month, current_day,
    //          current_minutes / 60, current_minutes % 60);
    //DBG_UART2("???????: %d\n", current_weekday);
    //DBG_UART2("?????????: %d\n\n", current_minutes);
    
    for (int i = 0; i < MAX_CYCLE_TIMERS; i++) {
        if (g_cycle_timers[i].timer_id == 0) {
            //DBG_UART2("????%d: timer_id=0??????\n", i);
            continue;
        }
        if (g_cycle_timers[i].enable != 1) {
            //DBG_UART2("????%d: timer_id=%d, enable=%d??????\n", 
            //          i, g_cycle_timers[i].timer_id, g_cycle_timers[i].enable);
            continue;
        }
        if (g_cycle_timers[i].week_mask == 0) {
            //DBG_UART2("????%d: timer_id=%d, week_mask=0??????\n", 
            //          i, g_cycle_timers[i].timer_id);
            continue;
        }
        
        uint16_t timer_minutes = g_cycle_timers[i].start_time;
        uint8_t week_mask = g_cycle_timers[i].week_mask;
        
        //DBG_UART2("????%d: timer_id=%d, ??????=%02d:%02d, ??????=0x%02X\n",
        //          i, g_cycle_timers[i].timer_id,
        //          timer_minutes / 60, timer_minutes % 60,
        //          week_mask);
        
        uint32_t best_diff = 0xFFFFFFFF;
        
        for (int offset_days = 0; offset_days < 7; offset_days++) {
            uint8_t target_rtc_weekday = current_weekday + offset_days;
            if (target_rtc_weekday > 7) {
                target_rtc_weekday -= 7;
            }
            
            uint8_t target_bit;
            if (target_rtc_weekday == 7) {
                target_bit = 0;
            } else {
                target_bit = target_rtc_weekday;
            }
            
            if (week_mask & (1 << target_bit)) {
                if (offset_days == 0) {
                    if (timer_minutes >= current_minutes) {
                        diff = timer_minutes - current_minutes;
                        //DBG_UART2("  ??????????: diff=%d????\n", diff);
                    } else {
                        diff = 7 * 1440 + timer_minutes - current_minutes;
                        //DBG_UART2("  ???????????????????: diff=%d????\n", diff);
                    }
                } else {
                    diff = offset_days * 1440 + timer_minutes - current_minutes;
                    //DBG_UART2("  %d?????????: diff=%d????\n", offset_days, diff);
                }
                
                if (diff < best_diff) {
                    best_diff = diff;
                }
                break;
            }
        }
        
        if (best_diff != 0xFFFFFFFF) {
            //DBG_UART2("  ?????: %d???? (%d?? %d隆矛?? %d????)\n",
            //          best_diff, best_diff / 1440, 
            //          (best_diff % 1440) / 60, best_diff % 60);
        }
        
        if (best_diff != 0xFFFFFFFF && best_diff < min_diff) {
            min_diff = best_diff;
            nearest = &g_cycle_timers[i];
            //DBG_UART2("  *** ??????: ID=%d, ???=%d???? ***\n", 
            //          g_cycle_timers[i].timer_id, min_diff);
        }
        //DBG_UART2("\n");
    }
    
    if (nearest != NULL) {
        //DBG_UART2("========== ???????????? ==========\n");
        //DBG_UART2("?????ID: %d\n", nearest->timer_id);
        //DBG_UART2("??????: %02d:%02d\n", nearest->start_time / 60, nearest->start_time % 60);
        //DBG_UART2("???????: %02d:%02d\n", nearest->end_time / 60, nearest->end_time % 60);
        //DBG_UART2("??????: %d????\n", nearest->water_time);
        //DBG_UART2("???????: %d????\n", nearest->soak_time);
        //DBG_UART2("??????: 0x%02X\n", nearest->week_mask);
        //DBG_UART2("???????: %d%%\n", nearest->valve_percent);
        //DBG_UART2("???????: %d???? (%d?? %d隆矛?? %d????)\n",
        //          min_diff, min_diff / 1440,
        //          (min_diff % 1440) / 60, min_diff % 60);
        //DBG_UART2("======================================\n\n");
    } else {
        //DBG_UART2("========== ???????隆矛????????? ==========\n\n");
    }
    
    return nearest;
}

/**
 * @brief ?????????????????????隆矛???????????????
 * @param cycle_timer ?????????????
 * @return 1-???????, 0-?????????
 */

uint8_t CycleTimer_NeedRun(CycleTimerConfig_t *cycle_timer)
{
    //DBG_UART2("\n========== CycleTimer_NeedRun ??? ==========\n");
    
    if (cycle_timer == NULL || cycle_timer->timer_id == 0) {
        //DBG_UART2("???: ???????隆矛? (NULL??ID=0)\n");
        return 0;
    }
    
    //DBG_UART2("?????ID: %d\n", cycle_timer->timer_id);
    //DBG_UART2("??????: %02d:%02d\n", cycle_timer->start_time / 60, cycle_timer->start_time % 60);
    //DBG_UART2("??????: 0x%02X\n", cycle_timer->week_mask);
    //DBG_UART2("??????: %d\n", cycle_timer->enable);
    
    if (cycle_timer->enable != 1) {
        //DBG_UART2("???: ???????????\n");
        return 0;
    }
    
    if (cycle_timer->week_mask == 0) {
        //DBG_UART2("???: ???????0\n");
        return 0;
    }
    
    // ?????????
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    
    uint16_t current_year = 2000 + sDate.Year;
    uint8_t current_month = sDate.Month;
    uint8_t current_day = sDate.Date;
    uint16_t current_minutes = sTime.Hours * 60 + sTime.Minutes;
    uint8_t current_weekday = calculate_weekday(current_year, current_month, current_day);
    
    //DBG_UART2("??????: %04d-%02d-%02d %02d:%02d\n",
    //          current_year, current_month, current_day,
    //          sTime.Hours, sTime.Minutes);
    //DBG_UART2("???????: %d\n", current_weekday);
    //DBG_UART2("?????????: %d\n", current_minutes);
    
    if (current_year == 2000) {
        //DBG_UART2("???: RTC?????隆矛?(2000??)\n");
        return 0;
    }
    
    uint8_t current_bit = (current_weekday == 7) ? 0 : current_weekday;
    //DBG_UART2("?????: bit%d\n", current_bit);
    //DBG_UART2("????????: 0x%02X & (1<<%d) = 0x%02X\n", 
    //          cycle_timer->week_mask, current_bit, 
    //          cycle_timer->week_mask & (1 << current_bit));
    //DBG_UART2("?????: %d == %d ? %s\n", 
    //          cycle_timer->start_time, current_minutes,
    //          (cycle_timer->start_time == current_minutes) ? "???" : "?????");
    
    if ((cycle_timer->week_mask & (1 << current_bit)) && 
        (cycle_timer->start_time == current_minutes)) {
        //DBG_UART2("???: ??????????????\n");
        //DBG_UART2("========================================\n\n");
        return 1;
    }
    
    //DBG_UART2("???: ?????????\n");
    //DBG_UART2("========================================\n\n");
    return 0;
}

