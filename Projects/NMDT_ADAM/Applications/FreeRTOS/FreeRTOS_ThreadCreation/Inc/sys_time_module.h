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

///*�������ݽṹ��*/
//struct data
//{
//			char flat_start[3];							/*BG,								3		��ʼ��ʶ*/
//			char station_number[6];					/*12345,						6		��վ��,��λ����*/
//			char type[3];										/*01,								3		��������*/
//			char device_flag[5];						/*YCCL,							5		�豸��ʶλ*/
//			char device_id[4];							/*000,							4		�豸ID*/
//			char data_time[15];							/*20180527123212,		15	�۲�ʱ��*/
//			char frame[4];									/*001,							4		֡��ʶ*/
//			char element_numbers[4];				/*006,							4		�۲�Ҫ�ر�����*/
//			char state_numbers[3];					/*01,								4		�豸״̬������*/
//			char cloud_one_height[11];			/*AL0A,12345,				11	��һ���Ƹ�*/
//			char cloud_two_height[11];			/*AL1A,12345,				11	�ڶ����Ƹ�*/
//			char cloud_thr_height[11];			/*AL2A,12345,				11	�������Ƹ�*/
//			char cloud_fou_height[11];			/*AL3A,12345,				11	���Ĳ��Ƹ�*/
//			char cloud_fiv_height[11];			/*AL4A,12345,				11	������Ƹ�*/
//			char cloud_vv[10];							/*ALE,12345,				10	��ֱ�ܼ���*/
//			char control_code[7];						/*000000,						7		��������λ*/
//			char self_check_state[4];				/*z,0,							4		�豸�Լ�״̬*/
//			char check_code[4];							/*1234,							5		У����*/
//			char flat_end[3];								/*BG\0							3		������ʶ*/
//};


/*״̬�����ṹ��*/
struct state_cloud
{
			char z;								/*�豸�Լ�״̬*/
			char y;								/*����������״̬*/
			char y_ALA;						/*�Ƹߴ������Ĺ���״̬*/
			char xA;							/*��ӵ�Դ*/
			char xB;							/*�豸/���������ѹ״̬*/
			char xD;							/*���ص�ѹ״̬*/
			char xE;							/*AC-DC ��ѹ״̬*/
			char wA;							/*�豸/�������廷���¶�״̬*/
			char wB;							/*̽�����¶�״̬*/
			char wC;							/*ǻ���¶�״̬*/
			char vA;							/*�豸����*/
			char uA;							/*�豸ͨ��״̬*/
			char tC;							/*RS232/485/422 ״̬*/
			char sA;							/*������Ⱦ���*/
			char rA;							/*����������*/
};


/*�ʿ���ṹ��*/
struct code
{
			unsigned int cloud_1;					/*��һ���Ƹ��ʿ���*/
			unsigned int cloud_2;					/*�ڶ����Ƹ��ʿ���*/
			unsigned int cloud_3;					/*�������Ƹ��ʿ���*/
			unsigned int cloud_4;					/*���Ĳ��Ƹ��ʿ���*/
			unsigned int cloud_5;					/*������Ƹ��ʿ���*/
			unsigned int cloud_6;					/*��ֱ�ܼ����ʿ���*/
};
extern struct code qc;

enum 
{
		QC_OK  = 0,			/*����ȷ��������û�г����������ޡ�*/
		QC_IC  = 1,			/*����*/
		QC_UB  = 2,			/*����*/
		QC_ER  = 3,			/*��������*/
		QC_UC  = 4,			/*���޸�����*/
		QC_AD  = 5,			/*Ԥ��*/
		QC_RE  = 6,			/*Ԥ�����ݲ��á�*/
		QC_RS  = 7,			/*Ԥ�����ݲ��á�*/
		QC_DE  = 8,			/*��ȱʧ����ȱ�����ݡ�*/
		QC_NO  = 9,			/*δ����������*/
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
