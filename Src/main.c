
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "rtc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bluetooth.h"
#include "time_keeper.h"
#include "valve.h"
#include "irrigation_debug.h"
#include "timer_manager.h"


/* USER CODE END Includes */

/* ========================= 参数配置区 ========================= */
#define TIME_BEFORE_SLEEP_MS     8000u   // 无串口活动多久进入STOP
#define RTC_WAKEUP_TIME_S        60u     // RTC兜底唤醒时间(秒)

#define KEY_DEBOUNCE_MS          50u
#define KEY_LONGPRESS_MS         3000u
#define BOTH_HOLD_MS             2000u
#define PRECHARGE_MS             2500u
#define BLUE_MS                  5000u

/* ========================= 调试输出 ========================= */
#define DEBUG_ENABLE
#ifdef DEBUG_ENABLE
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

void init_stack_check(void);
void check_stack_overflow(void);
/* ========================= 全局/静态变量（按模块分组） ========================= */
/* USER CODE BEGIN PV */

/* ---- 低功耗相关 ---- */
static uint8_t  is_in_stop_mode = 0;      // 1=STOP中/准备STOP（用于屏蔽UART回调喂SDK）
uint32_t last_activity_time = 0;   // 最近一次UART活动时间


/* ---- UART接收 & 同步帧头 ---- */
static unsigned char val;                 // USART1单字节接收缓冲
static volatile uint8_t sync_mode = 0;    // 1=丢弃并找帧头 0x55 0xAA
static volatile uint8_t last_byte = 0;
static const uint8_t FRAME_H1 = 0x55;
static const uint8_t FRAME_H2 = 0xAA;

/* ---- Key/预充电/蓝灯 ---- */
typedef enum {
  ACT_NONE = 0,
  ACT_OPEN,
  ACT_CLOSE
} action_t;

static action_t pending_act = ACT_NONE;
static uint8_t  switch_state = 0;     // 0=关阀 1=开阀

static uint8_t  pb3_last = 0, pb12_last = 0;
static uint32_t pb3_down_tick = 0, pb12_down_tick = 0;
static uint32_t pb3_last_change = 0, pb12_last_change = 0;
static uint8_t  pb3_long_fired = 0, pb12_long_fired = 0;

static uint8_t  both_fired = 0;
static uint32_t both_down_tick = 0;

uint8_t  precharge_on = 0;
static uint32_t precharge_tick = 0;

static uint8_t  blue_on = 0;
static uint32_t blue_tick = 0;

uint8_t is_disc = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
/* ---- 低功耗相关 ---- */
static void SystemPower_Config(void);
static void RTC_Time_Config(uint32_t stoptime);
static void UART1_RX_PA11_EXTI_Wakeup_Config(void);

/* ---- STOP进入/退出流程（新增封装，不改逻辑） ---- */
static void EnterStopSequence(void);
static void ExitStopSequence(void);

/* ---- Key任务 ---- */
static void Key_Task(void);
/* USER CODE END PFP */


/* ========================= 低功耗配置 ========================= */
/**
  * @brief  低功耗配置：开启超低功耗/快速唤醒，并将GPIO设为最低漏电状态。
  * @note   所有GPIO先设Analog，PA10/PA11例外设EXTI唤醒；进入STOP前清EXTI pending。
  */
	
/**
  * @brief  低功耗配置：开启超低功耗/快速唤醒，并将GPIO设为最低漏电状态。
  * @note   保留LED引脚为输出模式，其他设为Analog
  */
static void SystemPower_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  HAL_PWREx_EnableUltraLowPower();
  HAL_PWREx_EnableFastWakeUp();

  __HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_STOP_WAKEUPCLOCK_HSI);

	HAL_ADC_DeInit(&hadc);
	HAL_PWREx_EnableUltraLowPower();
	HAL_DBGMCU_DisableDBGStopMode();
	
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
    
      /* 5) 关指示灯 */
  GPIO_InitStructure.Pin   = GPIO_PIN_0 | GPIO_PIN_1| GPIO_PIN_2| GPIO_PIN_3| GPIO_PIN_4| GPIO_PIN_5| GPIO_PIN_6| GPIO_PIN_7;
  GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull  = GPIO_PULLUP;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1| GPIO_PIN_2| GPIO_PIN_3| GPIO_PIN_4| GPIO_PIN_5| GPIO_PIN_6| GPIO_PIN_7, GPIO_PIN_RESET);	//关灯
  
  
  GPIO_InitStructure.Pin   = GPIO_PIN_13 ;
  GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull  = GPIO_PULLUP;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET); //Bt lowpower
  HAL_Delay(100);

  /* 1) 先将所有GPIO设为Analog降低漏电 */
  GPIO_InitStructure.Pin  = GPIO_PIN_All;
  GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
  HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);


  UART1_RX_PA11_EXTI_Wakeup_Config();
  
  /* 4) 关闭GPIO时钟进一步省电 */
  // 注意：不能关闭有LED的GPIOA时钟，否则LED无法工作
  //__HAL_RCC_GPIOA_CLK_DISABLE();  // 注释掉这一行！
  __HAL_RCC_GPIOB_CLK_DISABLE();
  __HAL_RCC_GPIOC_CLK_DISABLE();
  __HAL_RCC_GPIOD_CLK_DISABLE();
  __HAL_RCC_GPIOH_CLK_DISABLE();
  //__HAL_RCC_USART1_CLK_DISABLE();
  __HAL_RCC_USART2_CLK_DISABLE();
}

/**
  * @brief  配置PA10/PA11为外部中断唤醒：PA10(串口起始位下降沿)、PA11(按键下降沿)。
  */
static void UART1_RX_PA11_EXTI_Wakeup_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  //KEY
  GPIO_InitStruct.Pin   =  GPIO_PIN_11;
  GPIO_InitStruct.Mode  = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  //KEY
  GPIO_InitStruct.Pin   =  GPIO_PIN_14;
  GPIO_InitStruct.Mode  = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  //KEY
  GPIO_InitStruct.Pin   =  GPIO_PIN_3;
  GPIO_InitStruct.Mode  = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_NVIC_SetPriority(EXTI2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);
  
  
  //Tuya_to_mcu
  GPIO_InitStruct.Pin   =  GPIO_PIN_12;
  GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11 | GPIO_PIN_12);
  HAL_NVIC_ClearPendingIRQ(EXTI4_15_IRQn);

  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}


/**
  * @brief  配置RTC唤醒定时器（兜底唤醒），stoptime单位为秒。
  */
static void RTC_Time_Config(uint32_t stoptime)
{
//  uint32_t i;

//  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

//  i = stoptime * 2396;  // 与RTC时钟源频率相关，存在误差
//  HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, i, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
}


/* ========================= 中断回调 ========================= */
/**
  * @brief  EXTI回调：PA10串口唤醒/PA11按键唤醒，并退出STOP标志。
  * @note   回调末尾关闭EXTI4_15_IRQn，抑制抖动/噪声导致的中断风暴。
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	
  if (GPIO_Pin == GPIO_PIN_12)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_12);

  }
  //KEY
  if (GPIO_Pin == GPIO_PIN_11)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11);

  }
  //KEY
  if (GPIO_Pin == GPIO_PIN_3)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);

  }
  //KEY
  if (GPIO_Pin == GPIO_PIN_14)
  {
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_14);
  }
  HAL_NVIC_DisableIRQ(EXTI2_3_IRQn);
  HAL_NVIC_DisableIRQ(EXTI4_15_IRQn);
  last_activity_time = HAL_GetTick();
}


/**
  * @brief  USART1接收中断：将字节喂给SDK，并在唤醒后用sync_mode重新对齐0x55AA帧头。
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1 && !is_in_stop_mode)
  {
    if (sync_mode)
    {
      if (last_byte == FRAME_H1 && val == FRAME_H2)
      {
        uart_receive_input(FRAME_H1);
        uart_receive_input(FRAME_H2);
        sync_mode = 0;
      }
      last_byte = val;
    }
    else
    {
      uart_receive_input(val);
    }
    HAL_UART_Receive_IT(&huart1, &val, 1);
  }
}


/**
  * @brief  RTC唤醒回调：作为兜底唤醒来源，清除低功耗标志并清PWR唤醒标志。
  */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
}


/* ========================= Key/预充电/蓝灯 ========================= */
/* 你原来的 KEY_PB3_PRESSED / KEY_PB12_PRESSED 宏继续用 */
static inline uint8_t read_pb3(void)  { return KEY_PB3_PRESSED() ? 1u : 0u; }
static inline uint8_t read_pb12(void) { return KEY_PB12_PRESSED() ? 1u : 0u; }
static inline uint8_t read_pa11(void) { return KEY_PA11_PRESSED() ? 1u : 0u; }

static void start_blue_5s(void)
{
  blue_on = 1;
  blue_tick = HAL_GetTick();
}

static void stop_blue_if_timeout(uint32_t now)
{
  if (blue_on && (now - blue_tick >= BLUE_MS))
  {
    blue_on = 0;
  }
}

static void do_action(action_t act)
{

  if (act == ACT_OPEN)
  {
	
    // 统一走 Irrigation：手动开（最高优先级、不带定时）

    Irrigation_OnManualCmd(1);

    // switch_state 只是你 main.c 的显示状态，如果还想保留就同步一下
	  switch_state = 1;
//	  DBG_UART2("\r\nPA7 SET333\r\n");
  }
  else if (act == ACT_CLOSE)
  {
    // 统一走 Irrigation：手动关（终止所有模式 + 倒计时清零 + 上报关状态）
    Irrigation_StopAll();
    switch_state = 0;
  }
}

static void service_precharge(uint32_t now)
{
  if (!precharge_on) return;

  if (now - precharge_tick >= PRECHARGE_MS)
  {
    precharge_on = 0;
    if (pending_act != ACT_NONE)
    {
      do_action(pending_act);
      pending_act = ACT_NONE;
    }
  }
}

/**
  * @brief  按键任务：消抖、长按判定、预充电PA7、单双键动作调度（非阻塞）。
  */
void Key_Check(void); //PA11长按3秒初始化
static void Key_Task(void)
{
  uint32_t now = HAL_GetTick();

  stop_blue_if_timeout(now);
  service_precharge(now);

  uint8_t pb3  = read_pb3();
  uint8_t pb12 = read_pb12();
  uint8_t pa11 = read_pa11();

	if (pb3 || pb12 || pa11) last_activity_time = now;
  /* PB3 消抖 */
  if (pb3 != pb3_last)
  {
    if (now - pb3_last_change >= KEY_DEBOUNCE_MS)
    {
      pb3_last_change = now;
      pb3_last = pb3;

      if (pb3)
      {
        pb3_down_tick = now;
        pb3_long_fired = 0;
        // 按下时点亮LED
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);

        if (!precharge_on)
        {
          precharge_on = 1;
          precharge_tick = now;
        }
      }
      else
      {
        // 松开时熄灭LED
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        pb3_long_fired = 0;
      }
    }
  }

  /* PB12 消抖 */
  if (pb12 != pb12_last)
  {
    if (now - pb12_last_change >= KEY_DEBOUNCE_MS)
    {
      pb12_last_change = now;
      pb12_last = pb12;

      if (pb12)
      {
        pb12_down_tick = now;
        pb12_long_fired = 0;
        // 按下时点亮LED
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);

        if (!precharge_on)
        {
          precharge_on = 1;
          precharge_tick = now;
        }
      }
      else
      {
        // 松开时熄灭LED
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
        pb12_long_fired = 0;
      }
    }
  }

  /* 双键同时按住优先 */
  if (pb3_last && pb12_last)
  {
    // 双键按下时，保持两个LED都亮
    if (!both_fired)
    {
      if (both_down_tick == 0) both_down_tick = now;

      if (now - both_down_tick >= BOTH_HOLD_MS)
      {
        both_fired = 1;

        //Irrigation_StopAll();   // 双键优先：先关阀并终止所有模式
        //bt_disconnect_req();
		//DBG_UART2("unbound!");
		
	    Irrigation_StopAll();
        DBG_UART2("do unbound");
        bt_unbound_req();
        start_blue_5s();
        pending_act = ACT_NONE;
        pb3_long_fired = 1;
        pb12_long_fired = 1;
      }
    }
    return;
  }
  else
  {
    both_down_tick = 0;
    both_fired = 0;
  }

  /* 单键长按 */
  if (pb3_last && !pb3_long_fired && !pb12_last)
  {
    // 长按期间保持LED亮
    //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    
    if (now - pb3_down_tick >= KEY_LONGPRESS_MS)
    {
		
      pb3_long_fired = 1;
      if (!precharge_on) do_action(ACT_OPEN);
      else pending_act = ACT_OPEN;
    }
  }
  
  if (pb12_last && !pb12_long_fired && !pb3_last)
  {
    // 长按期间保持LED亮
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
    
    if (now - pb12_down_tick >= KEY_LONGPRESS_MS)
    {
      pb12_long_fired = 1;
      if (!precharge_on) do_action(ACT_CLOSE);
      else pending_act = ACT_CLOSE;
    }
  }
  Key_Check();
}

static uint32_t key_press_time = 0;
static uint8_t key_pressed = 0;

void Key_Check(void)
{
    uint8_t key_level = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11);
    
    if (key_level == 0) { // 按键按下
        if (!key_pressed) {
            key_pressed = 1;
            key_press_time = HAL_GetTick();
        } else {
            if ((HAL_GetTick() - key_press_time) >= 4000) {
                // 长按3秒触发
                DBG_UART2("长按3秒触发！\n");
                // 执行你的操作
                
                // 可选：防止重复触发，直到按键释放
                while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11) == 0) {
                    HAL_Delay(10);
					          Irrigation_DeleteAllTimers();
                }
                key_pressed = 0;
            }
        }
    } else {
        key_pressed = 0; // 按键释放，重置状态
    }
}

/* ========================= STOP 进入/退出封装（不改你逻辑，只封装） ========================= */
/**
  * @brief  进入STOP前的准备流程：RTC兜底 + 关闭USART1 + 配置EXTI + 清标志 + 关LED + 低功耗GPIO配置
  */
static void EnterStopSequence(void)
{
//  DBG_UART2("\r\nsuspend\r\n");

  /* 1) RTC兜底 */
  //RTC_Time_Config(RTC_WAKEUP_TIME_S);

  /* 2) 关闭USART1，避免PA10与EXTI冲突 */

  /* 6) 低功耗GPIO配置 */
  DBG_UART2("Entering STOP mode...");
  Timer_SetAlarmByNearest();
  SystemPower_Config();
}

static void ExitStopSequence(void)
{
  /* 1) 恢复系统时钟 */
  SystemClock_Config();

  /* 2) 恢复GPIO/USART时钟并回到工程配置 */
  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_USART2_CLK_ENABLE();
  __HAL_RCC_ADC1_CLK_ENABLE();
	
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  
  sync_mode = 1;
  last_byte = 0;
	
 
  /* 3) 恢复PA9/PA10为USART1复用 */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_USART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
  HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
  HAL_UART_Receive_IT(&huart1, &val, 1);
  
  is_in_stop_mode = 0;
	#if 1
  MX_USART2_UART_Init();
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF4_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  #endif
  /* 4) 重新初始化USART1并启动接收 */
  
  
  /* 5) 关闭EXTI，避免抖动风暴；下次准备STOP时再开 */
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_3 | GPIO_PIN_14);
  HAL_NVIC_DisableIRQ(EXTI2_3_IRQn);
  HAL_NVIC_DisableIRQ(EXTI4_15_IRQn);
	
  /* 6) 重置活动计时 */
  last_activity_time = HAL_GetTick();
  
  is_disc = 0;
  DBG_UART2("\r\nresume\r\n");
  print_rtc();
  
	ADC_Get_Battery_Percentage();
}


extern volatile uint8_t g_time_ok;

static uint32_t s_last_sync_req_ms = 0;
static uint8_t  s_timer_sync_done = 0;    // 是否已经执行过定时删除

void print_rtc(void)
{
			// 设置RTC时间为当前年月日周不变，但是小时和分钟为23:58
            RTC_TimeTypeDef sTime = {0};
            RTC_DateTypeDef sDate = {0};

            HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
            HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
			
            DBG_UART2("-----RTC time-------  \n %04d-%02d-%02d %02d:%02d:%02d weekday=[%d]",
                      2000 + sDate.Year, sDate.Month, sDate.Date,\
			sTime.Hours, sTime.Minutes, sTime.Seconds, sDate.WeekDay);
}

static void TimeSync_Service(void)
{
    uint32_t now = HAL_GetTick();

    // 有效且可信，才停止请求
    if (TimeKeeper_IsValid() && !TimeKeeper_NeedSync() && g_time_ok) 
    {
        // ===== 时间同步成功后，执行一次定时删除 =====
        if (!s_timer_sync_done) {
            
            //Irrigation_ReportIdleStatus();
            DBG_UART2("first sync done, delete all timers");
			      print_rtc();
            s_timer_sync_done = 1;
			      mcu_dp_bool_update(DPID_SWITCH_ENABLED,1);
        }
        return;
    }

   // if (now - s_last_sync_req_ms >= 2000 + TIME_BEFORE_SLEEP_MS) {
	if (now - s_last_sync_req_ms >= 3000) {
        s_last_sync_req_ms = now;
        TimeKeeper_RequestSync(0x02);
        DBG_UART2("[TS] request sync type=0x02");
    }
}

static uint8_t  s_time_req_sent = 0;
static uint32_t s_boot_tick = 0;
static uint32_t s_last_time_print_tick = 0;

static uint32_t last_ms = 0;


static uint32_t s_last_time_req_ms = 0;
static uint8_t  s_time_req_started = 0;

#define TIME_REQ_RETRY_MS   3000u   // 每3秒重试一次
#define TIME_REQ_FIRST_DELAY_MS 500u // 上电后至少等500ms




// ---- 上电PA2闪烁3秒（非阻塞）----
static uint8_t  s_boot_blink_en = 0;
static uint32_t s_boot_blink_start_ms = 0;
static uint32_t s_boot_blink_last_toggle_ms = 0;

// 你想要的闪烁周期（比如 200ms 翻转一次）
#define BOOT_BLINK_TOTAL_MS     3000u
#define BOOT_BLINK_TOGGLE_MS    200u


static void BootBlink_Start(void)
{
    s_boot_blink_en = 1;
    s_boot_blink_start_ms = HAL_GetTick();
    s_boot_blink_last_toggle_ms = s_boot_blink_start_ms;

    // 先确保从灭开始（可选）
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_RESET);
}

static void BootBlink_Service(void)
{
    if (!s_boot_blink_en) return;

    uint32_t now = HAL_GetTick();
	
    // 到 3 秒，停止并熄灭
    if ((now - s_boot_blink_start_ms) >= BOOT_BLINK_TOTAL_MS)
    {
        s_boot_blink_en = 0;
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_RESET);
        return;
    }
	
    // 定时翻转
    if ((now - s_boot_blink_last_toggle_ms) >= BOOT_BLINK_TOGGLE_MS)
    {
        s_boot_blink_last_toggle_ms = now;

        // 翻转 PA2
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_1);
    }
}


void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc){
	last_activity_time = HAL_GetTick();

}

void HAL_RTC_AlarmBEventCallback(RTC_HandleTypeDef *hrtc)
{
    //DBG_UART2("AlarmB");
	last_activity_time = HAL_GetTick();
}

// 串口输出
void uart_print_reset_info(void)
{
    DBG_UART2("\r\n========== RESET INFO ==========\r\n");
    
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)) {
        DBG_UART2("Reset: Power-On Reset (首次上电)\r\n");
    }
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST)) {
        DBG_UART2("Reset: Pin Reset (NRST引脚复位)\r\n");
    }
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
        DBG_UART2("Reset: IWDG Reset (看门狗超时)\r\n");
    }
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) {
        DBG_UART2("Reset: WWDG Reset (窗口看门狗超时)\r\n");
    }
    
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

void IWDG_Feed(void)
{
    IWDG->KR = 0xAAAA;  // 重装载
}

int main(void)
{
  
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
	
  MX_RTC_Init();
  MX_ADC_Init();
  TimeKeeper_Init();
  Irrigation_Init();
  bt_protocol_init();
  s_timer_sync_done = 0;
	
  Timer_Init(); //服务器下发定时器初始化
  is_in_stop_mode = 0;
  last_activity_time = HAL_GetTick();

  s_boot_tick = HAL_GetTick();

  HAL_UART_Receive_IT(&huart1, &val, 1);
	
	// 上电开始闪烁
  BootBlink_Start();
  bt_enable_lowpoer_req(1);
	
  init_stack_check();//检测堆栈溢出
  DBG_UART2("Start");
  uart_print_reset_info();
  ADC_Get_Battery_Percentage();
  
  
  while (1) {
		IWDG_Feed();
		BootBlink_Service();
		check_stack_overflow();
		bt_uart_service();
		Key_Task();
		TimeSync_Service();
		if (HAL_GetTick() - last_ms >= 1000)
		{
			last_ms += 1000;          // 用 += 避免抖动累积
			Irrigation_Tick_1s();
		}
		if (HAL_GetTick() - last_activity_time >= TIME_BEFORE_SLEEP_MS)
		{
			EnterStopSequence();
			HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
			ExitStopSequence();
		}
	}
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* ? 关键：允许访问备份域（LSE/RTC 属于备份域） */
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();

  /* ? 关键：打开 LSE（32.768k） */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE
                                  | RCC_OSCILLATORTYPE_HSE;   // 你系统还在用HSE+PLL
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;

  RCC_OscInitStruct.LSEState = RCC_LSE_ON;   // ? 新增
  // RCC_OscInitStruct.LSIState = RCC_LSI_OFF; // 可选：不用LSI就关掉省电；也可保留做fallback

  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_8;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* 你的 SYSCLK/HCLK/PCLK 配置保持不变 */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  /* ? 关键：RTC 外设时钟源改成 LSE */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_RTC;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}


/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
// 栈溢出检测函数
void check_stack_overflow(void) {
    extern uint32_t __initial_sp;
    extern uint32_t Stack_Mem;
    
    // 检查栈底是否被破坏
    uint32_t *stack_bottom = (uint32_t*)&Stack_Mem;
    if (*stack_bottom != 0xAAAAAAAA) {
        // 栈溢出！进入错误处理
		DBG_UART2("-----stack_mem_error!!!\r\n");
        while(1) {
            // 可以点亮 LED 或保存错误信息
          HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);
          HAL_Delay(50);
        }
    }
}

// 在 main() 开始时填充栈
void init_stack_check(void) {
    extern uint32_t __initial_sp;
    extern uint32_t Stack_Mem;
    
    uint32_t *stack_start = (uint32_t*)&Stack_Mem;
    uint32_t *stack_end = (uint32_t*)&__initial_sp;
    
    // 用特定模式填充整个栈
    for(uint32_t *p = stack_start; p < stack_end; p++) {
        *p = 0xAAAAAAAA;
    }
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
