/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_MODULE_H
#define __USART_MODULE_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"
#include "main.h"
#include "time.h"

/*记忆存储地址*/
#define EEPROM_ADDR							  	(FLASH_EEPROM_BASE + 4)							//记忆存储写标志就存储在EEPROM的头地址
	 
/*云高仪扩展报文输出时间*/
struct cloud_time
{
		unsigned int year;
		unsigned int month;
		unsigned int day;
		unsigned int hour;
		unsigned int minute;
		unsigned int second;
};
	 
/*云高仪扩展报文数据结构体*/
struct cloud
{
		unsigned int cloud_output_interval;						/*输出间隔(单位秒)*/
		struct 			 cloud_time time;									/*输出报文时间*/
		int cloud_layer_numbers;											/*云层数量*/
		int cloud_one;																/*第一层云*/
		int cloud_two;																/*第二层云*/
		int cloud_three;															/*第三层云*/
		int cloud_four;																/*第四层云*/
		int cloud_five;																/*第五层云*/
//		unsigned int cloud_one_depth;									/*第一层云穿透深度*/
//		unsigned int cloud_two_depth;									/*第二层云穿透深度*/
//		unsigned int cloud_three_depth;								/*第三层云穿透深度*/
		int cloud_vertical_visibility;								/*垂直能见度*/
//		unsigned int cloud_excursion;									/*云高偏移量/海拔(m)*/
			char cloud_unit_1;														/*测量单位,*/
			char cloud_unit_2;														/*测量单位,*/
//		unsigned int cloud_device_state;							/*设备状态*/
//		unsigned int cloud_rs485_addr;								/*485总线地址*/
//		char				 cloud_name[10];									/*设备名“CHMyynnnn”*/
//		unsigned int cloud_one_deviation;							/*第一层云层的标准偏差*/
//		unsigned int cloud_two_deviation;							/*第一层云层的标准偏差*/
//		unsigned int cloud_three_deviation;						/*第一层云层的标准偏差*/
//		unsigned int cloud_one_depth_deviation;				/*第一层云层激光柱穿透深度的标准偏差*/
//		unsigned int cloud_two_depth_deviation;				/*第一层云层激光柱穿透深度的标准偏差*/
//		unsigned int cloud_three_depth_deviation;			/*第一层云层激光柱穿透深度的标准偏差*/
//		unsigned int cloud_vertical_visibility_deviation;				/*垂直能见度的标准偏差*/
		unsigned int cloud_system_state;							/*系统状态，0：OK;1：ERROR*/
		int cloud_environment_temperature;						/*外部温度，即环境温度*/
		int cloud_sensor_temperature;									/*传感器温度*/
		int cloud_inside_temperature;									/*内部温度*/
		unsigned int cloud_laser_time;								/*激光工作时间(单位小时)*/
		unsigned int cloud_window_state;							/*窗口状态，镜面的污染状态*/
		unsigned int cloud_laser_rate;								/*激光重复率*/
		unsigned int cloud_receiver_state;						/*接收器状态*/
		unsigned int cloud_light_state;								/*光源状态*/
//		unsigned int cloud_airsol_one;								/*气溶胶第一层*/
//		unsigned int cloud_airsol_two;								/*气溶胶第二层*/
//		char 				 cloud_airsol_one_index;					/*气溶胶第一层的检测质量指数，1字节*/
//		char 				 cloud_airsol_two_index;					/*气溶胶第二层的检测质量指数，1字节*/
//		char 				 cloud_bottom;										/*云层基底，1字节*/
//		char 				 cloud_height;										/*云层总高，1字节*/
//		short 			 cloud_check;											/*校验码*/
};
extern struct cloud cloud_d;

/*下载数据时间结构体*/
struct down_time
{
			char flag;													/*1代表下载分钟数据，2代表下载小时数据，0代表不下载任何数据*/
			unsigned int down_numbers;					/*下载数据条数*/
		
			int year_start;										/*起始时间*/
			int month_start;
			int day_start;
			int hour_start;
			int minute_start;
			int second_start;
	
			int year_end;										/*终止时间*/
			int month_end;
			int day_end;
			int hour_end;
			int minute_end;
			int second_end;
};

/*通过串口1、串口2设置的参数结构体*/
struct usart_info
{
			int usart_down_flag;								/*1代表下载数据，0代表不需要下载数据*/
			unsigned int data_flag;											/*接收到云高仪发送的数据标志，1为接收到，0为未接收到数据*/
			struct down_time time;											/*下载数据*/
			unsigned int set_comway;										/*握手机制，1 为主动发送方式， 0 为被动读取方式*/
			unsigned int power;													/*发射功率*/
			unsigned int voltage;												/*采集器电压*/
			int angle;																	/*发射器倾角*/
			unsigned int state_numbers;									/*设备状态变量数*/
			unsigned int check_code;										/*校验码*/
			unsigned int debug;													/*调试信息标识符*/
			unsigned int mode;													/*0代表正常模式，1代表只发送报文不处理*/
			unsigned long lon;													/*经度*/
			unsigned long lat;													/*纬度*/
			float height;																/*海拔高度*/
			unsigned int baud;													/*波特率*/
//			 int temp_upper_limit;											/*温度上限*/
//			 int temp_lower_limit;											/*温度下限*/
//			unsigned int voltage_upper_limit;						/*输入电压上限*/
//			unsigned int voltage_lower_limit;						/*输入电压下限*/
};
extern struct usart_info info;

/*记忆存储内容数组*/
extern char save_data[36];		/*save_data[] = "1 A00001 123.32.33 23.45.23 12345 4";  里面的空格全部是结束符'\0'*/
/*串口下载读取数据的字符数组*/
extern char data_down[172];

int32_t init_usart_module(void);
	 
#ifdef __cplusplus
}
#endif
#endif /*__STORAGE_MODULE_H */
