/**
  ******************************************************************************
  * File Name          : sys_time_module.h
  * Description        : This file provides a module updating system data&time. 
  * 
  ******************************************************************************
  *
  * COPYRIGHT(c) 2015 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SYS_TIME_MODULE_H
#define __SYS_TIME_MODULE_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"
#include "main.h"
#include "rtc.h"
#include "pcf8563.h"
#include "time.h"

#include "stm32_adafruit_sd.h"
//#include "data_eeprom.h"
/* FatFs includes component */
#include "ff_gen_drv.h"
#include "sd_diskio.h"

///*完整数据结构体*/
//struct data
//{
//			char flat_start[3];							/*BG,								3		起始标识*/
//			char station_number[6];					/*12345,						6		区站号,五位数字*/
//			char type[3];										/*01,								3		服务类型*/
//			char device_flag[5];						/*YCCL,							5		设备标识位*/
//			char device_id[4];							/*000,							4		设备ID*/
//			char data_time[15];							/*20180527123212,		15	观测时间*/
//			char frame[4];									/*001,							4		帧标识*/
//			char element_numbers[4];				/*006,							4		观测要素变量数*/
//			char state_numbers[3];					/*01,								4		设备状态变量数*/
//			char cloud_one_height[11];			/*AL0A,12345,				11	第一层云高*/
//			char cloud_two_height[11];			/*AL1A,12345,				11	第二层云高*/
//			char cloud_thr_height[11];			/*AL2A,12345,				11	第三层云高*/
//			char cloud_fou_height[11];			/*AL3A,12345,				11	第四层云高*/
//			char cloud_fiv_height[11];			/*AL4A,12345,				11	第五层云高*/
//			char cloud_vv[10];							/*ALE,12345,				10	垂直能见度*/
//			char control_code[7];						/*000000,						7		质量控制位*/
//			char self_check_state[4];				/*z,0,							4		设备自检状态*/
//			char check_code[4];							/*1234,							5		校验码*/
//			char flat_end[3];								/*BG\0							3		结束标识*/
//};


/*状态变量结构体*/
struct state_cloud
{
			char z;								/*设备自检状态*/
			char y;								/*传感器工作状态*/
			char y_ALA;						/*云高传感器的工作状态*/
			char xA;							/*外接电源*/
			char xB;							/*设备/主采主板电压状态*/
			char xD;							/*蓄电池电压状态*/
			char xE;							/*AC-DC 电压状态*/
			char wA;							/*设备/主采主板环境温度状态*/
			char wB;							/*探测器温度状态*/
			char wC;							/*腔体温度状态*/
			char vA;							/*设备加热*/
			char uA;							/*设备通风状态*/
			char tC;							/*RS232/485/422 状态*/
			char sA;							/*窗口污染情况*/
			char rA;							/*发射器能量*/
};


/*质控码结构体*/
struct code
{
			unsigned int cloud_1;					/*第一层云高质控码*/
			unsigned int cloud_2;					/*第二层云高质控码*/
			unsigned int cloud_3;					/*第三层云高质控码*/
			unsigned int cloud_4;					/*第四层云高质控码*/
			unsigned int cloud_5;					/*第五层云高质控码*/
			unsigned int cloud_6;					/*垂直能见度质控码*/
};
extern struct code qc;

enum 
{
		QC_OK  = 0,			/*“正确”：数据没有超过给定界限。*/
		QC_IC  = 1,			/*存疑*/
		QC_UB  = 2,			/*错误*/
		QC_ER  = 3,			/*订正数据*/
		QC_UC  = 4,			/*“修改数据*/
		QC_AD  = 5,			/*预留*/
		QC_RE  = 6,			/*预留，暂不用。*/
		QC_RS  = 7,			/*预留，暂不用。*/
		QC_DE  = 8,			/*“缺失”：缺测数据。*/
		QC_NO  = 9,			/*未做质量控制*/
};

int32_t init_sys_time_module(void);
int32_t get_sys_time(RTC_DateTypeDef *sDate,RTC_TimeTypeDef *sTime);
int32_t get_sys_time_tm(struct tm *DateTime);
int32_t set_sys_time(RTC_DateTypeDef *sDate,RTC_TimeTypeDef *sTime);
uint8_t fill_data(struct tm *time, char *p);
static void voltage_measure(void);
#ifdef __cplusplus
}
#endif
#endif /*__SYS_TIME_MODULE_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
