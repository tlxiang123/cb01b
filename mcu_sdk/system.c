

#define SYSTEM_GLOBAL

#include "bluetooth.h"
#include <string.h>
#include "usart.h"

//
extern const DOWNLOAD_CMD_S download_cmd[];
extern uint32_t last_activity_time;

/*****************************************************************************
Function name: set_bt_uart_byte
Function description: Writes 1 byte to BT_UART
Input parameters: dest: the actual address of the buffer area;
           byte:Write byte value
Return parameter: the total length after writing
*****************************************************************************/
unsigned short set_bt_uart_byte(unsigned short dest, unsigned char byte)
{
  unsigned char *obj = (unsigned char *)bt_uart_tx_buf + DATA_START + dest;
  
  *obj = byte;
  dest += 1;
  
  return dest;
}
/*****************************************************************************
Function name: set_bt_uart_buffer
Function description: Writes a buffer to BT_UART
Input parameter: dest: destination address
           src:source address
           len:Data length
Return parameter: none
*****************************************************************************/
unsigned short set_bt_uart_buffer(unsigned short dest, unsigned char *src, unsigned short len)
{
  unsigned char *obj = (unsigned char *)bt_uart_tx_buf + DATA_START + dest;
  
  my_memcpy(obj,src,len);
  
  dest += len;
  return dest;
}
/*****************************************************************************
Function name: bt_uart_write_data
Function description: Writes continuous data to BT UART
Input parameters: in:  buffer pointer
		   len: data length
Return parameter: none
*****************************************************************************/
void dbg_uart_write(unsigned char *in, unsigned short len) {
	
	DBG_UART2("Ty  rx data:  ");
	while (len --) {
		DBG_UART2("%02X ", *in);
		in++;
	}
	DBG_UART2("\r\n");
}

static void bt_uart_write_data(unsigned char *in, unsigned short len)
{
  if((NULL == in) || (0 == len))
  {
    return;
  }
  //dbg_uart_write(in, len);
  while(len --)
  {
    uart_transmit_output(*in);
    in ++;
  }
}
/*****************************************************************************
Function name:get_check_sum
Function description: calculate checksum
Input parameter: pack: data source pointer
           pack_len:data length
Return parameter: checksum
*****************************************************************************/
unsigned char get_check_sum(unsigned char *pack, unsigned short pack_len)
{
  unsigned short i;
  unsigned char check_sum = 0;
  
  for(i = 0; i < pack_len; i ++)
  {
    check_sum += *pack ++;
  }
  
  return check_sum;
}
/*****************************************************************************
Function name: bt_uart_write_frame
Function description: Send a frame of data to the bt uart port
Input parameter: fr_type:frame type
           len:data length
Return parameter: none
*****************************************************************************/
void bt_uart_write_frame(unsigned char fr_type, unsigned short len)
{
  unsigned char check_sum = 0;
  
  bt_uart_tx_buf[HEAD_FIRST] = 0x55;
  bt_uart_tx_buf[HEAD_SECOND] = 0xaa;
  bt_uart_tx_buf[PROTOCOL_VERSION] = 0x00;
  bt_uart_tx_buf[FRAME_TYPE] = fr_type;
  bt_uart_tx_buf[LENGTH_HIGH] = len >> 8;
  bt_uart_tx_buf[LENGTH_LOW] = len & 0xff;
  
  len += PROTOCOL_HEAD;
  check_sum = get_check_sum((unsigned char *)bt_uart_tx_buf, len - 1);
  bt_uart_tx_buf[len - 1] = check_sum;
  
  bt_uart_write_data((unsigned char *)bt_uart_tx_buf, len);
}

/*****************************************************************************
Function name: bt_uart_write_factory_test_frame
Function description: Send a frame of data to the bt uart port
Input parameter:
			sub_cmd : Production test subcommand
			p_data:data
           len:data length
Return parameter: none
*****************************************************************************/

void bt_uart_write_factory_test_frame(unsigned short sub_cmd,unsigned char *p_data,unsigned short len)
{
	unsigned char protocol_buf[256];
    unsigned char i = 0;
    unsigned short data_len = len +3;
    protocol_buf[i++] = 0x66;
    protocol_buf[i++] = 0xAA;
    protocol_buf[i++] = 0x00;
    protocol_buf[i++] = 0xF0;
    protocol_buf[i++] = (data_len>>8)&0xff;
    protocol_buf[i++] = data_len&0xff;
    protocol_buf[i++] = 0x03;
    protocol_buf[i++] = (sub_cmd>>8)&0xff;
    protocol_buf[i++] = sub_cmd&0xff;

	
    memcpy(protocol_buf+i,p_data,len);
    i += len;
    
    protocol_buf[i++] = get_check_sum(protocol_buf,i);
    
	bt_uart_write_data(protocol_buf,i);

}



/*****************************************************************************
Function name: heat_beat_check
Function description: Heartbeat packet detection
Input parameters: none
Return parameter: none
*****************************************************************************/
static void heat_beat_check(void)
{
  unsigned char length = 0;
  static unsigned char mcu_reset_state = FALSE;
  
  if(FALSE == mcu_reset_state)
  {
    length = set_bt_uart_byte(length,FALSE);
    mcu_reset_state = TRUE;
  }
  else
  {
    length = set_bt_uart_byte(length,TRUE);
  }
  
  bt_uart_write_frame(HEAT_BEAT_CMD, length);
}
/*****************************************************************************
Function name: product_info_update
Function description: upload product information
Input parameters: none
Return parameter: none
*****************************************************************************/
static void product_info_update(void)
{
  unsigned char length = 0;
  
  length = set_bt_uart_buffer(length,(unsigned char *)PRODUCT_KEY,my_strlen((unsigned char *)PRODUCT_KEY));
  length = set_bt_uart_buffer(length,(unsigned char *)MCU_VER,my_strlen((unsigned char *)MCU_VER));
  
  bt_uart_write_frame(PRODUCT_INFO_CMD, length);
}
/*****************************************************************************
Function name: get_mcu_bt_mode
Function description: query the working mode of mcu and bt
Input parameters: none
Return parameter: none
*****************************************************************************/
static void get_mcu_bt_mode(void)
{
  unsigned char length = 0;
  
#ifdef BT_CONTROL_SELF_MODE                                   //Module self-processing
  length = set_bt_uart_byte(length, BT_STATE_KEY);
  length = set_bt_uart_byte(length, BT_RESERT_KEY);
#else                                                           
  //No need to process data
#endif
  
  bt_uart_write_frame(WORK_MODE_CMD, length);
}
/*****************************************************************************
Function name: get_update_dpid_index
Function description: Get the serial number of DPID in the array
Input parameters: dpid:dpid
Return parameter: index: DP serial number
*****************************************************************************/
static unsigned char get_dowmload_dpid_index(unsigned char dpid)
{
  unsigned char index;
  unsigned char total = get_download_cmd_total();
  
  for(index = 0; index < total; index ++)
  {
    if(download_cmd[index].dp_id == dpid)
    {
      break;
    }
  }
  
  return index;
}
/*****************************************************************************
Function name: data_point_handle
Function description: send data processing
Input parameter: 
	value: the pointer of the data source issued
Return parameter: 
	ret: return data processing result
*****************************************************************************/
/**
 * @brief 处理数据点下�?
 *
 * 从接收的数据�?解析数据点ID、类型和长度，并调用数据点�?�理函数
 *
 * @param value 数据点数�?缓冲�?
 * @return unsigned char 处理结果 TRUE-成功 FALSE-失败
 */
static unsigned char data_point_handle(const unsigned char value[])
{
  unsigned char dp_id,index;
  unsigned char dp_type;
  unsigned char ret;
  unsigned short dp_len;
  
  dp_id = value[0];
  dp_type = value[1];
  dp_len = value[2] * 0x100;
  dp_len += value[3];
  
  index = get_dowmload_dpid_index(dp_id);

  if(dp_type != download_cmd[index].dp_type || dp_len >BT_UART_RECV_BUF_LMT)
  {
    //Error message
    return FALSE;
  }
  else
  {
    ret = dp_download_handle(dp_id,value + 4,dp_len);
  }
  
  return ret;
}
/*****************************************************************************
Function name: data_handle
Function description: data frame processing
Input parameter: 
	Offset: Data start bit
Return parameter: none
*****************************************************************************/
void data_handle(unsigned short offset)
{
#ifdef SUPPORT_MCU_FIRM_UPDATE
  unsigned char *firmware_addr;
  static unsigned long firm_length;                                             //MCU upgrades file length
  static unsigned char firm_update_flag;                                        //MCU upgrade flag
  unsigned long dp_len;
#else
  unsigned short dp_len;
#endif
  unsigned char length = 0;
  unsigned char is_sync_sleep = 1;
  unsigned char ret;
  unsigned short i,total_len;
  unsigned char cmd_type = bt_uart_rx_buf[offset + FRAME_TYPE];


  signed char bt_rssi;

#ifdef TUYA_BCI_UART_COMMON_SEND_TIME_SYNC_TYPE 
  bt_time_struct_data_t bt_time;
  unsigned short time_zone_100;
  char current_timems_string[14] = "000000000000";
  unsigned long time_stamp_ms;
#endif

  unsigned char version_buf[6] = {0};
  unsigned int mcu_fw_version = MCU_APP_VER_NUM;
  unsigned int mcu_hd_version = MCU_HARD_VER_NUM;


  switch(cmd_type)
  {
  case HEAT_BEAT_CMD:                                   //Heartbeat package
    is_sync_sleep = 0;
    heat_beat_check();
    break;
    
  case PRODUCT_INFO_CMD:                                //product information
    product_info_update();
    break;
    
  case WORK_MODE_CMD:                                   //Query module working mode set by MCU
    get_mcu_bt_mode();
    break;
    
#ifndef BT_CONTROL_SELF_MODE
  case BT_STATE_CMD:                                  //bt work state
    bt_work_state = bt_uart_rx_buf[offset + DATA_START];
    if(bt_work_state==0x01||bt_work_state==0x00)
    {
    	#ifdef SUPPORT_MCU_FIRM_UPDATE
    	mcu_ota_init_disconnect();
    	#endif
    }
    bt_uart_write_frame(BT_STATE_CMD,0);
    break;

  case BT_RESET_CMD:                                  //Reset BT (BT returns success)
    reset_bt_flag = RESET_BT_SUCCESS;
	
    break;
#endif
    
  case DATA_QUERT_CMD:                                  //dp data handled
    total_len = bt_uart_rx_buf[offset + LENGTH_HIGH] * 0x100;
    total_len += bt_uart_rx_buf[offset + LENGTH_LOW];
    
    for(i = 0;i < total_len;)
    {
      dp_len = bt_uart_rx_buf[offset + DATA_START + i + 2] * 0x100;
      dp_len += bt_uart_rx_buf[offset + DATA_START + i + 3];
      
      ret = data_point_handle((unsigned char *)bt_uart_rx_buf + offset + DATA_START + i);
      
      if(SUCCESS == ret)
      {
        //Success tips
      }
      else
      {
        //Error message
      }
      
      i += (dp_len + 4);
    }
    break;
  case STATE_UPLOAD_CMD:
    //is_sync_sleep = 0;
    break;
  case STATE_QUERY_CMD:                                 //Status query
    all_data_update();                               
    break;
    
#ifdef TUYA_BCI_UART_COMMON_RF_TEST 
	case TUYA_BCI_UART_COMMON_RF_TEST:
		if(my_memcmp((unsigned char *)bt_uart_rx_buf + offset + DATA_START+7,"true",4)==0)
		{
			bt_rssi = (bt_uart_rx_buf[offset + DATA_START+21]-'0')*10 + (bt_uart_rx_buf[offset + DATA_START+22]-'0');
			bt_rssi = -bt_rssi;
			bt_rf_test_result(1,bt_rssi);
		}
		else
		{
			bt_rf_test_result(0,0);
		}
		break;
#endif

#ifdef TUYA_BCI_UART_COMMON_SEND_STORAGE_TYPE
	case TUYA_BCI_UART_COMMON_SEND_STORAGE_TYPE:
		bt_send_recordable_dp_data_result(bt_uart_rx_buf[offset + DATA_START]);
		break;
#endif

#ifdef TUYA_BCI_UART_COMMON_SEND_TIME_SYNC_TYPE
	case TUYA_BCI_UART_COMMON_SEND_TIME_SYNC_TYPE:
    //is_sync_sleep = 1;
		ret = bt_uart_rx_buf[offset + DATA_START];
    //DBG_UART2("-----------------get time success-------------------ret = %d", ret);
		if(ret==0)//Get time succeeded
		{
			if(bt_uart_rx_buf[offset + DATA_START+1]==0x00)//Time format 0 :Get 7 bytes of time and time type + 2 bytes of time zone information
			{
				bt_time.nYear = bt_uart_rx_buf[offset + DATA_START+2] + 2018;
				bt_time.nMonth = bt_uart_rx_buf[offset + DATA_START+3];
				bt_time.nDay = bt_uart_rx_buf[offset + DATA_START+4];
				bt_time.nHour = bt_uart_rx_buf[offset + DATA_START+5];
				bt_time.nMin = bt_uart_rx_buf[offset + DATA_START+6];
				bt_time.nSec = bt_uart_rx_buf[offset + DATA_START+7];
				bt_time.DayIndex = bt_uart_rx_buf[offset + DATA_START+8];
				time_zone_100 = ((unsigned short)bt_uart_rx_buf[offset + DATA_START+9]<<8)+bt_uart_rx_buf[offset + DATA_START+10];
			}
			else if(bt_uart_rx_buf[offset + DATA_START+1]==0x01)//Time format 1: Get 13 bytes of ms-level unix time + 2 bytes of time zone information
			{
				my_memcpy(current_timems_string,&bt_uart_rx_buf[offset + DATA_START+2],13);
				time_stamp_ms = my_atol(current_timems_string,10);
				time_zone_100 = ((unsigned short)bt_uart_rx_buf[offset + DATA_START+15]<8)+bt_uart_rx_buf[offset + DATA_START+16];
			}
			else if(bt_uart_rx_buf[offset + DATA_START+1]==0x02)//Time format 2: Get 7 bytes of time and time type + 2 bytes of time zone information
			{
				bt_time.nYear = bt_uart_rx_buf[offset + DATA_START+2] + 2000;
				bt_time.nMonth = bt_uart_rx_buf[offset + DATA_START+3];
				bt_time.nDay = bt_uart_rx_buf[offset + DATA_START+4];
				bt_time.nHour = bt_uart_rx_buf[offset + DATA_START+5];
				bt_time.nMin = bt_uart_rx_buf[offset + DATA_START+6];
				bt_time.nSec = bt_uart_rx_buf[offset + DATA_START+7];
				bt_time.DayIndex = bt_uart_rx_buf[offset + DATA_START+8];
				time_zone_100 = ((unsigned short)bt_uart_rx_buf[offset + DATA_START+9]<<8)+bt_uart_rx_buf[offset + DATA_START+10];
			}
			bt_time_sync_result(0,bt_uart_rx_buf[offset + DATA_START+1],bt_time,time_zone_100,time_stamp_ms);
		}
		else//Failed to get time
		{
			bt_time_sync_result(1,bt_uart_rx_buf[offset + DATA_START+1],bt_time,time_zone_100,time_stamp_ms);
		}
		break;
#endif

#ifdef TUYA_BCI_UART_COMMON_MODIFY_ADV_INTERVAL
	case TUYA_BCI_UART_COMMON_MODIFY_ADV_INTERVAL:
		bt_modify_adv_interval_result(bt_uart_rx_buf[offset + DATA_START]);
		break;
#endif
#ifdef TUYA_BCI_UART_COMMON_TURNOFF_SYSTEM_TIME
	case TUYA_BCI_UART_COMMON_TURNOFF_SYSTEM_TIME:
	  bt_close_timer_result(bt_uart_rx_buf[offset + DATA_START]);
	  break;
#endif
#ifdef TUYA_BCI_UART_COMMON_ENANBLE_LOWER_POWER
	case TUYA_BCI_UART_COMMON_ENANBLE_LOWER_POWER:
		bt_enable_lowpoer_result(bt_uart_rx_buf[offset + DATA_START]);
		break;
#endif
#ifdef TUYA_BCI_UART_COMMON_SEND_ONE_TIME_PASSWORD_TOKEN
	case TUYA_BCI_UART_COMMON_SEND_ONE_TIME_PASSWORD_TOKEN:
	  bt_send_one_time_password_token_result(bt_uart_rx_buf[offset + DATA_START]);
	  break;
#endif
#ifdef TUYA_BCI_UART_COMMON_ACTIVE_DISCONNECT
	case TUYA_BCI_UART_COMMON_ACTIVE_DISCONNECT:
		bt_disconnect_result(bt_uart_rx_buf[offset + DATA_START]);
		break;
#endif
#ifdef TUYA_BCI_UART_COMMON_QUERY_MCU_VERSION
	case TUYA_BCI_UART_COMMON_QUERY_MCU_VERSION:  
	  version_buf[0] = (mcu_fw_version>>16)&0xff;
	  version_buf[1] = (mcu_fw_version>>8)&0xff;
	  version_buf[2] = (mcu_fw_version>>0)&0xff;
	  version_buf[3] = (mcu_hd_version>>16)&0xff;
	  version_buf[4] = (mcu_hd_version>>8)&0xff;
	  version_buf[5] = (mcu_hd_version>>0)&0xff;

	  length = set_bt_uart_buffer(length,version_buf,6);
	  bt_uart_write_frame(TUYA_BCI_UART_COMMON_QUERY_MCU_VERSION,length);
	  break;
#endif
#ifdef TUYA_BCI_UART_COMMON_FACTOR_RESET_NOTIFY
	case TUYA_BCI_UART_COMMON_FACTOR_RESET_NOTIFY:	
		bt_factor_reset_notify();
		break;
#endif
#ifdef SUPPORT_MCU_FIRM_UPDATE
	  case TUYA_BCI_UART_COMMON_MCU_OTA_REQUEST:
	  case TUYA_BCI_UART_COMMON_MCU_OTA_FILE_INFO:
	  case TUYA_BCI_UART_COMMON_MCU_OTA_FILE_OFFSET:
	  case TUYA_BCI_UART_COMMON_MCU_OTA_DATA:
	  case TUYA_BCI_UART_COMMON_MCU_OTA_END:
		total_len = bt_uart_rx_buf[offset + LENGTH_HIGH] * 0x100;
		total_len += bt_uart_rx_buf[offset + LENGTH_LOW];
		mcu_ota_proc(cmd_type,&bt_uart_rx_buf[offset + DATA_START],total_len);
	  	break;
#endif
  default:
    break;
  }

  if (is_sync_sleep == 1) {
    last_activity_time = HAL_GetTick();
  }
}
/*****************************************************************************
Function name:get_queue_total_data
Function description: read data in the queue
Input parameters: none
Return parameter: none
*****************************************************************************/
unsigned char get_queue_total_data(void)
{
  if(queue_in != queue_out)
    return 1;
  else
    return 0;
}
/*****************************************************************************
Function name:Queue_Read_Byte
Function description: Read 1 byte data of queue
Input parameters: none
Return parameter: none
*****************************************************************************/
unsigned char Queue_Read_Byte(void)
{
  unsigned char value =0;
  
  if(queue_out != queue_in)
  {
    //Data is not empty
    if(queue_out >= (unsigned char *)(bt_queue_buf + sizeof(bt_queue_buf)))
    {
      //Data has reached the end
      queue_out = (unsigned char *)(bt_queue_buf);
    }
    
    value = *queue_out ++;   
  }
  
  return value;
}

