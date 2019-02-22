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

/*����洢��ַ*/
#define EEPROM_ADDR							  	(FLASH_EEPROM_BASE + 4)							//����洢д��־�ʹ洢��EEPROM��ͷ��ַ
	 
/*�Ƹ�����չ�������ʱ��*/
struct cloud_time
{
		unsigned int year;
		unsigned int month;
		unsigned int day;
		unsigned int hour;
		unsigned int minute;
		unsigned int second;
};
	 
/*�Ƹ�����չ�������ݽṹ��*/
struct cloud
{
		unsigned int cloud_output_interval;						/*������(��λ��)*/
		struct 			 cloud_time time;									/*�������ʱ��*/
		int cloud_layer_numbers;											/*�Ʋ�����*/
		int cloud_one;																/*��һ����*/
		int cloud_two;																/*�ڶ�����*/
		int cloud_three;															/*��������*/
		int cloud_four;																/*���Ĳ���*/
		int cloud_five;																/*�������*/
//		unsigned int cloud_one_depth;									/*��һ���ƴ�͸���*/
//		unsigned int cloud_two_depth;									/*�ڶ����ƴ�͸���*/
//		unsigned int cloud_three_depth;								/*�������ƴ�͸���*/
		int cloud_vertical_visibility;								/*��ֱ�ܼ���*/
//		unsigned int cloud_excursion;									/*�Ƹ�ƫ����/����(m)*/
			char cloud_unit_1;														/*������λ,*/
			char cloud_unit_2;														/*������λ,*/
//		unsigned int cloud_device_state;							/*�豸״̬*/
//		unsigned int cloud_rs485_addr;								/*485���ߵ�ַ*/
//		char				 cloud_name[10];									/*�豸����CHMyynnnn��*/
//		unsigned int cloud_one_deviation;							/*��һ���Ʋ�ı�׼ƫ��*/
//		unsigned int cloud_two_deviation;							/*��һ���Ʋ�ı�׼ƫ��*/
//		unsigned int cloud_three_deviation;						/*��һ���Ʋ�ı�׼ƫ��*/
//		unsigned int cloud_one_depth_deviation;				/*��һ���Ʋ㼤������͸��ȵı�׼ƫ��*/
//		unsigned int cloud_two_depth_deviation;				/*��һ���Ʋ㼤������͸��ȵı�׼ƫ��*/
//		unsigned int cloud_three_depth_deviation;			/*��һ���Ʋ㼤������͸��ȵı�׼ƫ��*/
//		unsigned int cloud_vertical_visibility_deviation;				/*��ֱ�ܼ��ȵı�׼ƫ��*/
		unsigned int cloud_system_state;							/*ϵͳ״̬��0��OK;1��ERROR*/
		int cloud_environment_temperature;						/*�ⲿ�¶ȣ��������¶�*/
		int cloud_sensor_temperature;									/*�������¶�*/
		int cloud_inside_temperature;									/*�ڲ��¶�*/
		unsigned int cloud_laser_time;								/*���⹤��ʱ��(��λСʱ)*/
		unsigned int cloud_window_state;							/*����״̬���������Ⱦ״̬*/
		unsigned int cloud_laser_rate;								/*�����ظ���*/
		unsigned int cloud_receiver_state;						/*������״̬*/
		unsigned int cloud_light_state;								/*��Դ״̬*/
//		unsigned int cloud_airsol_one;								/*���ܽ���һ��*/
//		unsigned int cloud_airsol_two;								/*���ܽ��ڶ���*/
//		char 				 cloud_airsol_one_index;					/*���ܽ���һ��ļ������ָ����1�ֽ�*/
//		char 				 cloud_airsol_two_index;					/*���ܽ��ڶ���ļ������ָ����1�ֽ�*/
//		char 				 cloud_bottom;										/*�Ʋ���ף�1�ֽ�*/
//		char 				 cloud_height;										/*�Ʋ��ܸߣ�1�ֽ�*/
//		short 			 cloud_check;											/*У����*/
};
extern struct cloud cloud_d;

/*��������ʱ��ṹ��*/
struct down_time
{
			char flag;													/*1�������ط������ݣ�2��������Сʱ���ݣ�0���������κ�����*/
			unsigned int down_numbers;					/*������������*/
		
			int year_start;										/*��ʼʱ��*/
			int month_start;
			int day_start;
			int hour_start;
			int minute_start;
			int second_start;
	
			int year_end;										/*��ֹʱ��*/
			int month_end;
			int day_end;
			int hour_end;
			int minute_end;
			int second_end;
};

/*ͨ������1������2���õĲ����ṹ��*/
struct usart_info
{
			int usart_down_flag;								/*1�����������ݣ�0������Ҫ��������*/
			unsigned int data_flag;											/*���յ��Ƹ��Ƿ��͵����ݱ�־��1Ϊ���յ���0Ϊδ���յ�����*/
			struct down_time time;											/*��������*/
			unsigned int set_comway;										/*���ֻ��ƣ�1 Ϊ�������ͷ�ʽ�� 0 Ϊ������ȡ��ʽ*/
			unsigned int power;													/*���书��*/
			unsigned int voltage;												/*�ɼ�����ѹ*/
			int angle;																	/*���������*/
			unsigned int state_numbers;									/*�豸״̬������*/
			unsigned int check_code;										/*У����*/
			unsigned int debug;													/*������Ϣ��ʶ��*/
			unsigned int mode;													/*0��������ģʽ��1����ֻ���ͱ��Ĳ�����*/
			unsigned long lon;													/*����*/
			unsigned long lat;													/*γ��*/
			float height;																/*���θ߶�*/
			unsigned int baud;													/*������*/
//			 int temp_upper_limit;											/*�¶�����*/
//			 int temp_lower_limit;											/*�¶�����*/
//			unsigned int voltage_upper_limit;						/*�����ѹ����*/
//			unsigned int voltage_lower_limit;						/*�����ѹ����*/
};
extern struct usart_info info;

/*����洢��������*/
extern char save_data[36];		/*save_data[] = "1 A00001 123.32.33 23.45.23 12345 4";  ����Ŀո�ȫ���ǽ�����'\0'*/
/*�������ض�ȡ���ݵ��ַ�����*/
extern char data_down[172];

int32_t init_usart_module(void);
	 
#ifdef __cplusplus
}
#endif
#endif /*__STORAGE_MODULE_H */
