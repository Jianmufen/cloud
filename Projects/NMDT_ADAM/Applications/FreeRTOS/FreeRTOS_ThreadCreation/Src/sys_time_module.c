/* Includes ------------------------------------------------------------------*/
#include "sys_time_module.h"
#include "cmsis_os.h"

#include "usart_module.h"
#include "storage_module.h"
#include "iwdg.h"
#include "pcf8563.h"
#include "time_related.h"
#include "adc.h"
#include "usart.h"
//#include	"LCM_DRIVE.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define systimeSTACK_SIZE   384//configMINIMAL_STACK_SIZE  /*����ʱ����������ļ�ϵͳ��������Ҫ��ջ�ϴ�  384����ܶ���������󻹲����Ļ���������Ϊ512������ջ��С���������ô�����ʱ���ᵼ��ϵͳ����*/
#define systimePRIORITY     osPriorityHigh

#define dataSTACK_SIZE   (512)  /*����ʱ����������ļ�ϵͳ��������Ҫ��ջ�ϴ�  384����ܶ���������󻹲����Ļ���������Ϊ512������ջ��С���������ô�����ʱ���ᵼ��ϵͳ����*/
#define dataPRIORITY     osPriorityNormal

/* STM32CPU�ڲ���׼��ѹADC Parameter */
#define REF_ADC1_VOLTAGE          (3.3)    /* extern reference voltage �ⲿ�ο���ѹ1.25V*/
#define MAX_ADC1_VALUE             (4095)   /* STM32�Դ���ADCֻ��12λ�ķֱ���  2^12=4096 ��AD�����ת�����Ϊ4096*/
#define VOLTAGE_R1									(100)		/*�����ѹ�ķ�ѹ����1����λK��*/
#define VOLTAGE_R2									(20)		/*�����ѹ�ķ�ѹ����1����λK��*/
/* RTC Time*/
static RTC_TimeTypeDef sys_time;
static RTC_DateTypeDef sys_date;

/*ȫ�ֽṹ�����*/
struct code qc;
static struct state_cloud state;
static struct tm tm_data;
/*ȫ���ַ�����=172*/
static char data_buf[172] = {0};

/* os relative */
static osThreadId SysTimeThreadHandle;
static osThreadId DataThreadHandle;
//static osThreadId Tim6ThreadHandle;
static osSemaphoreId semaphore;
static osSemaphoreId semaphore_data;
//static osSemaphoreId semaphore_tim6;
static osMutexId mutex;
/* Private function prototypes -----------------------------------------------*/
static void SysTime_Thread(void const *argument);
static void Data_Thread(void const *argument);

/**
  * @brief  Init System Time Module. 
  * @retval 0:success;-1:failed
  */
int32_t init_sys_time_module(void)
{
			/* Init RTC Internal */
			MX_RTC_Init();
	
			IWDG_Init();
			
			ADC_Init();
			/* Init extern RTC PCF8563 */
			if(IIC_Init() != HAL_OK)
			{
						printf("init pcf8563 failed!\r\n");
			}
			else
			{
						printf("pcf8563 init ok\r\n");
						/* synchronize internal RTC with pcf8563 */
						sync_time();
			}
		 
			/* Define used semaphore */
			osSemaphoreDef(SEM);
			/* Create the semaphore used by the two threads */
			semaphore=osSemaphoreCreate(osSemaphore(SEM), 1);
			if(semaphore == NULL)
			{
						printf("Create Semaphore failed!\r\n");
						return -1;
			}
			
			/* Define used semaphore ����data������ź���*/
			osSemaphoreDef(SEM_DATA);
			/* Create the semaphore used by the two threads */
			semaphore_data=osSemaphoreCreate(osSemaphore(SEM_DATA), 1);
			if(semaphore_data == NULL)
			{
				printf("Create semaphore_data failed!\r\n");
				return -1;
			}
			
			
			/* Create the mutex */
			osMutexDef(Mutex);
			mutex=osMutexCreate(osMutex(Mutex));
			if(mutex == NULL)
			{
						printf("Create Mutex failed!\r\n");
						return -1;
			}
			
			/* Create a thread to update system date and time */
			osThreadDef(SysTime, SysTime_Thread, systimePRIORITY, 0, systimeSTACK_SIZE);
			SysTimeThreadHandle=osThreadCreate(osThread(SysTime), NULL);
			if(SysTimeThreadHandle == NULL)
			{
						printf("Create System Time Thread failed!\r\n");
						return -1;
			}
			
			/* Create a thread to update system date and time */
			osThreadDef(Data, Data_Thread, dataPRIORITY, 0, dataSTACK_SIZE);
			DataThreadHandle = osThreadCreate(osThread(Data), NULL);
			if(DataThreadHandle == NULL)
			{
						printf("Create DataThreadHandle failed!\r\n");
						return -1;
			}
			
			return 0;
}

/**
  * @brief  get System Date and Time. 
  * @retval 0:success;-1:failed
  */
int32_t get_sys_time(RTC_DateTypeDef *sDate,RTC_TimeTypeDef *sTime)
{
  /* Wait until a Mutex becomes available */
  if(osMutexWait(mutex,500)==osOK)
  {
    if(sDate)
    {
      *sDate=sys_date;
    }
    if(sTime)
    {
      *sTime=sys_time;
    }
    
    /* Release mutex */
    osMutexRelease(mutex);
    
    return 0;
  }
  else
  {
    /* Time */
    if(sTime)
    {
      sTime->Seconds=0;
      sTime->Minutes=0;
      sTime->Hours=0;
    }
    /* Date */
    if(sDate)
    {
      sDate->Date=1;
      sDate->WeekDay=RTC_WEEKDAY_SUNDAY;
      sDate->Month=(uint8_t)RTC_Bcd2ToByte(RTC_MONTH_JANUARY);
      sDate->Year=0;
    }
    
    return -1;
  }
}

int32_t get_sys_time_tm(struct tm *DateTime)
{
  /* Wait until a Mutex becomes available */
  if(osMutexWait(mutex,500)==osOK)
  {
    if(DateTime)
    {
      DateTime->tm_year=sys_date.Year+2000;
      DateTime->tm_mon=sys_date.Month;
      DateTime->tm_mday=sys_date.Date;
      DateTime->tm_hour=sys_time.Hours;
      DateTime->tm_min=sys_time.Minutes;
      DateTime->tm_sec=sys_time.Seconds;
    }
    
    /* Release mutex */
    osMutexRelease(mutex);
    
    return 0;
  }
  else
  {
    if(DateTime)
    {
      DateTime->tm_year=2000;
      DateTime->tm_mon=0;
      DateTime->tm_mday=0;
      DateTime->tm_hour=0;
      DateTime->tm_min=0;
      DateTime->tm_sec=0;
    }
    
    return -1;
  }
}

int32_t set_sys_time(RTC_DateTypeDef *sDate,RTC_TimeTypeDef *sTime)
{
  int32_t res=0;
  
  /* Wait until a Mutex becomes available */
  if(osMutexWait(mutex,500)==osOK)
  {
    if(sDate)
    {
      sys_date=*sDate;
    }
    if(sTime)
    {
      sys_time=*sTime;
    }
    
    /* check param */
    if(IS_RTC_YEAR(sys_date.Year) && IS_RTC_MONTH(sys_date.Month) && IS_RTC_DATE(sys_date.Date) &&
       IS_RTC_HOUR24(sys_time.Hours) && IS_RTC_MINUTES(sys_time.Minutes) && IS_RTC_SECONDS(sys_time.Seconds))
    {
    
      if((HAL_RTC_SetDate(&hrtc,&sys_date,FORMAT_BIN)==HAL_OK)&&  /* internal RTC */
         (HAL_RTC_SetTime(&hrtc,&sys_time,FORMAT_BIN)==HAL_OK)&&
         (PCF8563_Set_Time(sDate->Year, sDate->Month, sDate->Date, sTime->Hours, sTime->Minutes, sTime->Seconds) == HAL_OK))      /* PCF8563 */
      {
					res=0;
      }
      else
      {
					res=-1;
      }
    }
    else
    {
      res=-1;
    }
    
    /* Release mutex */
    osMutexRelease(mutex);
    
    return res;
  }
  else
  {
    return -1;
  }
}

/**
  * @brief  System sys_time update
  * @param  thread not used
  * @retval None
  */
static void SysTime_Thread(void const *argument)
{
			int ret = 0;
			int i = 0;
			unsigned int seconds_power = 0;
	
			while(1)
			{
						/* Try to obtain the semaphore */
						if(osSemaphoreWait(semaphore, osWaitForever) == osOK)
						{
									/* Wait until a Mutex becomes available */
									if(osMutexWait(mutex,500) == osOK)
									{
												HAL_RTC_GetTime(&hrtc, &sys_time, FORMAT_BIN);
												HAL_RTC_GetDate(&hrtc, &sys_date, FORMAT_BIN);
												
												/*������Ϣ*/
												if(info.debug)
												{
																printf("time:20%02d-%02d-%02d %02d:%02d:%02d\r\n", sys_date.Year,	sys_date.Month, sys_date.Date,\
																								sys_time.Hours, sys_time.Minutes, sys_time.Seconds);
															
													
//																PCF8563_Read_Time();
//																printf("PCF8563_Time:\"20%d-%d-%d %d:%d:%d\"\r\n", 
//																PCF_DataStruct_Time.RTC_Year,PCF_DataStruct_Time.RTC_Month,PCF_DataStruct_Time.RTC_Day,
//																PCF_DataStruct_Time.RTC_Hour,PCF_DataStruct_Time.RTC_Minute,PCF_DataStruct_Time.RTC_Second );
													
//															//				//����ϵͳ��������ռ�ĶѴ�С
//															i = xPortGetFreeHeapSize();
//															printf("i = %d\r\n", i);
//															
//															i = xPortGetMinimumEverFreeHeapSize();
//															printf("i = %d\r\n", i);
												}
												
												
												
												
												/*�Ƹ����ϵ�90��֮���������ò�����*/ 
												if(seconds_power == 90)
												{
														seconds_power = 91;
														if(info.debug)
														{
																printf("seconds_power=%02d\r\n", seconds_power);
																printf("baud=%d\r\n", info.baud);
														}
														if(info.baud == 2)
														{
																/*�����Ƹ��ǵĲ�����*/
																HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*��ʼ��PA1Ϊ�ߵ�ƽ��485ֻ�ܷ��ͣ�����3ֻ��������*/
																HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=2\r\n", 16, 0xFF);
																HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*��ʼ��PA1Ϊ�͵�ƽ��485ֻ�ܽ��գ�����3ֻ��������*/
														}
														else if(info.baud == 3)
														{
																/*�����Ƹ��ǵĲ�����*/
																HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*��ʼ��PA1Ϊ�ߵ�ƽ��485ֻ�ܷ��ͣ�����3ֻ��������*/
																HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=3\r\n", 16, 0xFF);
																HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*��ʼ��PA1Ϊ�͵�ƽ��485ֻ�ܽ��գ�����3ֻ��������*/
														}
														else if(info.baud == 4)
														{
																/*�����Ƹ��ǵĲ�����*/
																HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*��ʼ��PA1Ϊ�ߵ�ƽ��485ֻ�ܷ��ͣ�����3ֻ��������*/
																HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=4\r\n", 16, 0xFF);
																HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*��ʼ��PA1Ϊ�͵�ƽ��485ֻ�ܽ��գ�����3ֻ��������*/
														}
														else if(info.baud == 5)
														{
																/*�����Ƹ��ǵĲ�����*/
																HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*��ʼ��PA1Ϊ�ߵ�ƽ��485ֻ�ܷ��ͣ�����3ֻ��������*/
																HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=5\r\n", 16, 0xFF);
																HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*��ʼ��PA1Ϊ�͵�ƽ��485ֻ�ܽ��գ�����3ֻ��������*/
														}
														else if(info.baud == 6)
														{
																/*�����Ƹ��ǵĲ�����*/
																HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*��ʼ��PA1Ϊ�ߵ�ƽ��485ֻ�ܷ��ͣ�����3ֻ��������*/
																HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=6\r\n", 16, 0xFF);
																HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*��ʼ��PA1Ϊ�͵�ƽ��485ֻ�ܽ��գ�����3ֻ��������*/
														}
														else if(info.baud == 7)
														{
																/*�����Ƹ��ǵĲ�����*/
																HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*��ʼ��PA1Ϊ�ߵ�ƽ��485ֻ�ܷ��ͣ�����3ֻ��������*/
																HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=7\r\n", 16, 0xFF);
																HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*��ʼ��PA1Ϊ�͵�ƽ��485ֻ�ܽ��գ�����3ֻ��������*/
														}
														else
														{
																/*�����Ƹ��ǵĲ�����*/
																HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*��ʼ��PA1Ϊ�ߵ�ƽ��485ֻ�ܷ��ͣ�����3ֻ��������*/
																HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=3\r\n", 16, 0xFF);
																HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*��ʼ��PA1Ϊ�͵�ƽ��485ֻ�ܽ��գ�����3ֻ��������*/
														}
												}
												else if(seconds_power < 90)
												{
														seconds_power++;
														if(info.debug)
														{
																printf("seconds_power=%02d\r\n", seconds_power);
														}
												}
												
												/*������Ҫ���ݺ���*/
												if(info.usart_down_flag == 1)														
												{
															ret = start_storage();
															if(0 == ret)
															{
																		info.usart_down_flag = 0;
															}
															else
															{
																		printf("<F>\r\n");
															}
												}
												
												if((sys_time.Seconds == 0) && (!info.mode))														/*ÿһ���Ӵ�����յ������ݣ����洢*/
												{
															tm_data.tm_year = sys_date.Year;
															tm_data.tm_mon	= sys_date.Month;
															tm_data.tm_mday	=	sys_date.Date;
															tm_data.tm_hour	=	sys_time.Hours;
															tm_data.tm_min	=	sys_time.Minutes;
															tm_data.tm_sec	=	0;
													
															voltage_measure();					/*������·��������ѹ*/
															/* Release the semaphore every 1 minute */
														 if(semaphore_data != NULL)
															{
																		/*������Ϣ*/
																		if(info.debug)
																		{
																					printf("release semaphore_data\r\n");
																		}
																		osSemaphoreRelease(semaphore_data);
															}
												}
												
												/*ÿСʱ��һ��ʱ��*/
												if((sys_time.Minutes == 25) &&(sys_time.Seconds == 30))
												{
															sync_time();
												}
												
												/* Release mutex */
												osMutexRelease(mutex);
										
									}
									else
									{
												printf("û�еȵ������ź���\r\n");
									}
						}
						
						if(hiwdg.Instance)
						{
									HAL_IWDG_Refresh(&hiwdg);  /* refresh the IWDG */
//									printf("ι����\r\n");
						}
			}
}


/*���Ӽ�Сʱ���ݴ���������*/
static void Data_Thread(void const *argument)
{
			uint8_t 			data_numbers 	= 0;
			FRESULT  			res						=	FR_OK;
			unsigned int  byteswritten1 = 0;
			char 					path_file[50]	=	{0};
			unsigned int  offset_save 	= 0;
			static FIL 					file; 
			char *p = NULL;
			while(osSemaphoreWait(semaphore_data, 1)	==	osOK);			/*���ĵ��մ����Ĵ����ź�������Դ���ź����մ��������ͷž��ǿ���ʹ�õ���Դ*/
			
			while(1)
			{
						/* Try to obtain the semaphore */
						if(osSemaphoreWait(semaphore_data, osWaitForever) == osOK)		/*osWaitForever��ʾһֱ�ȴ�semaphore_data����ź���*/
						{
									HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);     /*����PB7������SD���źŵ�*/
									info.state_numbers = 0;					/*��ʼֵΪ��*/
							
									if(info.data_flag)							/*�ȴ����ź��������ڵȴ��ź����ڼ���յ����Ƹ��ǵ�����*/
									{
												qc.cloud_1 = QC_OK;				/*ֻҪ���յ��Ƹ��ǵ����ݣ��ʿ��붼ΪQC_OK����*/
												qc.cloud_2 = QC_OK;
												qc.cloud_3 = QC_OK;
												qc.cloud_4 = QC_OK;
												qc.cloud_5 = QC_OK;
												qc.cloud_6 = QC_OK;
										
												if(cloud_d.cloud_system_state)					/*ϵͳ״̬*/
												{
															state.y_ALA = 2;					/*�Ƹߴ������Ĺ���״̬*/
															info.state_numbers++;			/*״̬������1*/
															
															qc.cloud_1 = QC_IC;				/*���յ������ݴ��ɣ�������*/
															qc.cloud_2 = QC_IC;
															qc.cloud_3 = QC_IC;
															qc.cloud_4 = QC_IC;
															qc.cloud_5 = QC_IC;
															qc.cloud_6 = QC_IC;
												}
												else
												{
															state.y_ALA = 0;
												}
												
//												if(cloud_d.cloud_environment_temperature > 5000)
//												{
//															state.wA = 3;							/*�����¶�ƫ��*/
//															info.state_numbers++;			/*״̬������1*/
//												}
//												else if(cloud_d.cloud_environment_temperature < -4500)
//												{
//															state.wA = 4;							/*�����¶�ƫ��*/
//															info.state_numbers++;			/*״̬������1*/
//												}
//												else
//												{
//															state.wA = 0;							/*�����¶�����*/
//												}
												
												if(cloud_d.cloud_sensor_temperature > 5000)
												{
															state.wB = 3;							/*̽�����¶�ƫ��*/
															info.state_numbers++;			/*״̬������1*/
												}
												else if(cloud_d.cloud_sensor_temperature < -4500)
												{
															state.wB = 4;							/*̽�����¶�ƫ��*/
															info.state_numbers++;			/*״̬������1*/
												}
												else
												{
															state.wB = 0;							/*̽�����¶�����*/
												}
										
												if(cloud_d.cloud_inside_temperature > 5000)
												{
															state.wC = 3;							/*ǻ���¶�ƫ��*/
															info.state_numbers++;			/*״̬������1*/
												}
												else if(cloud_d.cloud_inside_temperature < -4500)
												{
															state.wC = 4;							/*ǻ���¶�ƫ��*/
															info.state_numbers++;			/*״̬������1*/
												}
												else
												{
															state.wC = 0;							/*ǻ���¶�����*/
												}
												
												if(cloud_d.cloud_window_state > 90)
												{
															state.sA = 0;							/*������Ⱦ״̬����*/
												}
												else if(cloud_d.cloud_window_state > 80)
												{
															state.sA = 6;							/*������Ⱦ״̬��΢*/
															info.state_numbers++;			/*״̬������1*/
												}
												else if(cloud_d.cloud_window_state > 60)
												{
															state.sA = 7;							/*������Ⱦ״̬һ��*/
															info.state_numbers++;			/*״̬������1*/
												}
												else
												{
															state.sA = 8;							/*������Ⱦ״̬�ض�*/
															info.state_numbers++;			/*״̬������1*/
												}
												
												if(cloud_d.cloud_receiver_state > 90)
												{
															state.y = 0;							/*������״̬����*/
												}
												else if(cloud_d.cloud_receiver_state > 70)
												{
															state.y = 1;							/*������״̬�쳣*/
															info.state_numbers++;			/*״̬������1*/
												}
												else
												{
															state.y = 2;							/*������״̬����*/
															info.state_numbers++;			/*״̬������1*/
												}
												
												if(info.voltage > 15)
												{
															state.xB = 3;							/*AC-DC ��ѹ״̬ƫ��*/
															info.state_numbers++;			/*״̬������1*/
												}
												else if((info.voltage <= 15) || ((info.voltage >= 9)))
												{
															state.xB = 0;							/*AC-DC ��ѹ״̬����*/
												}
												else if(info.voltage < 9)
												{
															state.xB = 4;							/*AC-DC ��ѹ״̬ƫ��*/
															info.state_numbers++;			/*״̬������1*/
												}
												else	if(info.voltage < 4)
												{
															state.xB = 5;							/*AC-DC ��ѹ״̬����*/
															info.state_numbers++;			/*״̬������1*/
												}
												
												if(info.state_numbers)
												{
															state.z = 1;							/*�Լ�״̬�쳣*/
															info.state_numbers += 1;	/*״̬������1*/
												}
												else
												{
															state.z = 0;							/*�Լ�״̬����*/
															info.state_numbers = 1;		/*״̬����Ϊ1*/
												}
												
												/*������Ϣ*/
												if(info.debug)
												{
															printf("state_numbers=%d\r\n", info.state_numbers);
															printf("y_ALA=%d wA=%d wB=%d wC=%d sA=%d y=%d xE=%d z=%d\r\n", state.y_ALA, state.wA, state.wB, \
																													state.wC, state.sA, state.y, state.xE, state.z);
												}
									}
									else														/*�ȴ����ź��������ڵȴ��ź����ڼ�δ���յ����Ƹ��ǵ�����*/
									{
												qc.cloud_1 = QC_DE;				/*ֻҪδ���յ��Ƹ��ǵ����ݣ��ʿ��붼ΪQC_DEȱ��*/
												qc.cloud_2 = QC_DE;
												qc.cloud_3 = QC_DE;
												qc.cloud_4 = QC_DE;
												qc.cloud_5 = QC_DE;
												qc.cloud_6 = QC_DE;
										
												cloud_d.cloud_one									 = -2;		/*ֻҪδ���յ��Ƹ��ǵ����ݣ�����������ȫ��Ϊ-2*/
												cloud_d.cloud_two 								 = -2;
												cloud_d.cloud_three 							 = -2;
												cloud_d.cloud_four 								 = -2;
												cloud_d.cloud_five 								 = -2;
												cloud_d.cloud_vertical_visibility  = -2;
										
												state.z 						= 1;
												state.y_ALA  				= 2;
												info.state_numbers 	=	2;
									}
									
									/*�洢���ݵ�SD��*/	
									if(sys_time.Hours >= 20)
									{
												if((sys_time.Hours == 20) && (sys_time.Minutes == 0))												/*20:00�����ݴ洢��ÿһ���ļ������һ��*/
												{
															offset_save = 170 * 60 *24 ;
												}
												else
												{
															offset_save = ((sys_time.Hours - 20) * 60 + sys_time.Minutes) * 170;				/*ÿһ���������ļ������ƫ����*/
												}
												
									}
									else
									{
												offset_save = ((sys_time.Hours + 4) * 60 + sys_time.Minutes) * 170;					/*ÿһ���������ļ������ƫ����*/
									}
									
									memset(path_file, 0, sizeof(path_file));
									
									if(sys_time.Hours < 20)
									{
												snprintf(path_file, sizeof(path_file), "/DATA/%02u/X%s_cloud_value20%02d%02d%02d_01.txt", \
																sys_date.Month, save_data + 2, sys_date.Year, sys_date.Month, sys_date.Date);
									}
									else
									{
												if((sys_time.Hours == 20) && (sys_time.Minutes == 0))												/*20:00�����ݴ洢��ÿһ���ļ������һ��*/
												{
															snprintf(path_file, sizeof(path_file), "/DATA/%02u/X%s_cloud_value20%02d%02d%02d_01.txt", \
																sys_date.Month, save_data + 2, sys_date.Year, sys_date.Month, sys_date.Date);
												}
												else
												{
															AddaDay(&sys_date.Year, &sys_date.Month, &sys_date.Date, &sys_time.Hours, &sys_time.Minutes, &sys_time.Seconds);
															snprintf(path_file, sizeof(path_file), "/DATA/%02u/X%s_cloud_value20%02d%02d%02d_01.txt", \
																							sys_date.Month, save_data + 2, sys_date.Year, sys_date.Month, sys_date.Date);
												}
												
									}
									
									/*������Ϣ*/
									if(info.debug)
									{
												printf("path_file=%s\r\n", path_file);
												printf("offset_save=%d\r\n", offset_save);
									}
									
									/*ÿһ�������ļ��ĵ�һ��*/
									res = f_open(&file, (const char *)path_file, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
									if(res == FR_OK)
										{
													memset(data_buf, 0, sizeof(data_buf));
													res = f_read(&file, data_buf, 6, &byteswritten1);	
													if(res == FR_OK)
													{
																if(strncmp(data_buf, save_data + 2, 6) == 0)
																{
																			/*������Ϣ*/
																			if(info.debug)
																			{
																						printf("��һ���Ѿ�����\r\n");
																			}
																			f_close(&file);
																}
																else
																{
																			memset(data_buf, 0, sizeof(data_buf));
																			data_numbers = snprintf(data_buf, 171, "%s-------20%02d-%02d-%02d-%s-%s-00000-00000------------------------------------------------------------------------------------------------------------------\r\n",\
																			save_data + 2, sys_date.Year, sys_date.Month, sys_date.Date, save_data + 9, save_data + 19);
																			/*������Ϣ*/
																			if(info.debug)
																			{
																						printf("��һ�е�����=%d\r\n", data_numbers);
																						printf("��һ�е�����=%s\r\n", data_buf);
																			}
																			res = f_write(&file,  data_buf, 170, &byteswritten1);
																			f_close(&file);
																}
													}
													else
													{
																f_close(&file);
													}
										}
									else
										{
													/*������Ϣ*/
													if(info.debug)
													{
																printf("��һ�е�f_open=%d\r\n", res);
													}
										}
										
									/*ÿһ��Сʱ�ļ��ĵ�һ��*/
									path_file[4] = 'H';												/*Сʱ���ݵ��ļ����ͷ������ݵľ����������ַ���һ��*/
									path_file[37] = '2';											/*Сʱ���ݵ��ļ����ͷ������ݵľ����������ַ���һ��*/
									/*������Ϣ*/
									if(info.debug)
									{
												printf("Сʱpath_file=%s\r\n", path_file);
												printf("Сʱoffset_save=%d\r\n", offset_save);
									}
									
									res = f_open(&file, (const char *)path_file, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
									if(res == FR_OK)
										{
													memset(data_buf, 0, sizeof(data_buf));
													res = f_read(&file, data_buf, 6, &byteswritten1);	
													if(res == FR_OK)
													{
																if(strncmp(data_buf, save_data + 2, 6) == 0)
																{
																			/*������Ϣ*/
																			if(info.debug)
																			{
																						printf("Сʱ�ļ��ĵ�һ���Ѿ�����\r\n");
																			}
																			f_close(&file);
																}
																else
																{
																			memset(data_buf, 0, sizeof(data_buf));
																			data_numbers = snprintf(data_buf, 171, "%s-------20%02d-%02d-%02d-%s-%s-00000-00000------------------------------------------------------------------------------------------------------------------\r\n",\
																			save_data + 2, sys_date.Year, sys_date.Month, sys_date.Date, save_data + 8, save_data + 18);
																			/*������Ϣ*/
																			if(info.debug)
																			{
																						printf("Сʱ�ļ���һ�е�����=%d\r\n", data_numbers);
																						printf("Сʱ�ļ���һ�е�����=%s\r\n", data_buf);
																			}
																			res = f_write(&file,  data_buf, 170, &byteswritten1);
																			f_close(&file);
																}
													}
													else
													{
																f_close(&file);
													}
										}
									else
										{
													/*������Ϣ*/
													if(info.debug)
													{
																printf("Сʱ��һ�е�f_open=%d\r\n", res);
													}
										}
						
									/*����������*/
									path_file[4] = 'A';												/*Сʱ���ݵ��ļ����ͷ������ݵľ����������ַ���һ��*/
									path_file[37] = '1';											/*Сʱ���ݵ��ļ����ͷ������ݵľ����������ַ���һ��*/
									/*������Ϣ*/
									if(info.debug)
									{
												printf("����path_file=%s\r\n", path_file);
												printf("����offset_save=%d\r\n", offset_save);
									}
									memset(data_buf, 0, sizeof(data_buf));
									data_numbers = fill_data(&tm_data, data_buf);				
									info.data_flag = 0;																	/*�������ݱ�־*/
									if(info.set_comway)																	/*���ֻ���=1�����������ͷ�������*/
									{
												HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);     /*����PB6�������źų���*/
												HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);     /*����PA0��485ֻ�ܷ������ݣ�����2ֻ��������*/
												p = strstr(data_buf, "ED");
												*(p+2) = '\r';
												*(p+3) = '\n';
												*(p+4) = '\0';
												printf("%s", data_buf);
												HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);     /*����PA0��485ֻ�ܽ������ݣ�����2ֻ��������*/
												p = strstr(data_buf, "ED");
												*(p+2) = 0x20;
												*(p+3) = 0x20;
												*(p+4) = 0x20;
									}
									/*������Ϣ*/
									if(info.debug)
									{
												printf("data_buf=%d:%s", data_numbers, data_buf);
									}
									
									res = f_open(&file, (const char *)path_file, FA_OPEN_ALWAYS | FA_WRITE);
									if(res == FR_OK)
									{
											if(info.debug)
											 {
														printf("���ļ��ɹ�\r\n");
											 }
											 res = f_lseek(&file, offset_save);
											 if(res != FR_OK)
												{
														f_close(&file);	
														if(info.debug)
														{
																	printf("�ļ�ѡַ����:%d\r\n",res);
														}
														if(BSP_SD_Init() != MSD_OK)
														 {
																	if(info.debug)
																		{
																				printf("sd init failed!\r\n");
																		}													      
														 }
														else
														 {
																	if(info.debug)
																		{
																				printf("sd1 init ok!\r\n");
																		}																		      
														 }
												}
												else
												 {
																if(info.debug)
																{
																			printf("�ļ�ѡַ�ɹ�:%d\r\n",res);
																}		
																res = f_write(&file, (uint8_t *)data_buf, 170, &byteswritten1);
																if(res != FR_OK)
																{
																		f_close(&file);	
																		if(info.debug)
																			{
																					printf("д����ʧ�ܣ�%d\r\n",res);
																			}																
																		if(BSP_SD_Init()!=MSD_OK)
																			{
																					if(info.debug)
																						{
																								printf("sd2 init failed!\r\n");
																						}																	
																			}
																		else
																			{
																					if(info.debug)
																						{
																								printf("sd3 init ok!\r\n");
																						}																		
																			}
																	}	
																 else
																	{
																				if(info.debug)
																					{
																							printf("д���ݳɹ�:%s",data_buf);
																					}
																					
																					res = f_close(&file);	
																				 if(res == FR_OK)
																					{
																								if(info.debug)
																									{
																											printf("�ر��ļ��ɹ�:%d\r\n",res);
																									}																
																					}
																	}	
													}							
										}
									else 
									{
												if(info.debug)
												{
														printf("f_open=%d\r\n", res);
												}			
												if(BSP_SD_Init()!=MSD_OK)
													{
																if(info.debug)
																	{
																			printf("sd4 init failed!\r\n");
																	}					
																
													}
												else
													{
														if(info.debug)
															{
																printf("sd5 init ok!\r\n");
															}				
													}
									}
									
									if(sys_time.Minutes	==	0)											/*��ʱ���ݼ�Сʱ����*/
									{
												data_buf[37] = '1';												/*Сʱ���ݵ����ݺͷ������ݵ����ݾ��������ַ���һ��*/
												data_buf[38] = '6';
												data_buf[39] = '0';
										
												path_file[4] = 'H';												/*Сʱ���ݵ��ļ����ͷ������ݵľ����������ַ���һ��*/
												path_file[38] = '2';											/*Сʱ���ݵ��ļ����ͷ������ݵľ����������ַ���һ��*/
											
												if(sys_time.Hours >= 20)
												{
															offset_save = ((sys_time.Hours - 20) + 1) * 170;				/*ÿһ���������ļ������ƫ����*/
												}
												else
												{
															offset_save = ((sys_time.Hours + 4 ) + 1) * 170;					/*ÿһ���������ļ������ƫ����*/
												}
												res = f_open(&file, (const char *)path_file, FA_OPEN_ALWAYS | FA_WRITE);
												if(res == FR_OK)
												{
														if(info.debug)
														 {
																	printf("���ļ��ɹ�\r\n");
														 }
														 res = f_lseek(&file, offset_save);
														 if(res != FR_OK)
															{
																	f_close(&file);	
																	if(info.debug)
																	{
																				printf("�ļ�ѡַ����:%d\r\n",res);
																	}
																	if(BSP_SD_Init() != MSD_OK)
																	 {
																				if(info.debug)
																					{
																							printf("sd init failed!\r\n");
																					}													      
																	 }
																	else
																	 {
																				if(info.debug)
																					{
																							printf("sd1 init ok!\r\n");
																					}																		      
																	 }
															}
															else
															 {
																			if(info.debug)
																			{
																						printf("�ļ�ѡַ�ɹ�:%d\r\n",res);
																			}		
																			res = f_write(&file, (uint8_t *)data_buf, 170, &byteswritten1);
																			if(res != FR_OK)
																			{
																					f_close(&file);	
																					if(info.debug)
																						{
																								printf("д����ʧ�ܣ�%d\r\n",res);
																						}																
																					if(BSP_SD_Init()!=MSD_OK)
																						{
																								if(info.debug)
																									{
																											printf("sd2 init failed!\r\n");
																									}																	
																						}
																					else
																						{
																								if(info.debug)
																									{
																											printf("sd3 init ok!\r\n");
																									}																		
																						}
																				}	
																			 else
																				{
																							if(info.debug)
																								{
																										printf("д���ݳɹ�:%s",data_buf);
																								}				
																							
																				}	
																			 res = f_close(&file);	
																			 if(res == FR_OK)
																				{
																							if(info.debug)
																								{
																										printf("�ر��ļ��ɹ�:%d\r\n",res);
																								}																
																				}
																}							
													}
												else 
												{
															if(BSP_SD_Init()!=MSD_OK)
																{
																			if(info.debug)
																				{
																						printf("sd4 init failed!\r\n");
																				}					
																}
															else
																{
																	if(info.debug)
																		{
																			printf("sd5 init ok!\r\n");
																		}				
																}
												}
									}
									HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);     /*����PB7������SD���źŵ�*/
									HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);     /*����PB6�������źų���*/
						}
			}
}

/*��·���ѹ����*/
static void voltage_measure(void)
{
			unsigned int voltage_ad = 0;											/*������ADֵ*/
			float voltage_m = 0;																/*�����ĵ�ѹֵ*/
			float voltage_c = 0;																/*��������ĵ�ѹֵ*/
	
			 /* use STM32 ADC Channel 1 to measure VDD */
			/**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time. 
				*/
			ADC_ChannelConfTypeDef sConfig={0};
			
			/* channel : ADC_CHANNEL_1 */
			sConfig.Channel=ADC_CHANNEL_21;
			sConfig.Rank=ADC_REGULAR_RANK_1;
			sConfig.SamplingTime=ADC_SAMPLETIME_96CYCLES;
			HAL_ADC_ConfigChannel(&hadc,&sConfig);
			
			HAL_ADC_Start(&hadc);  /* Start ADC Conversion ��ʼ������·���ѹ*/
			
			/* Wait for regular group conversion to be completed. �ȵ�������ת�����*/
			if(HAL_ADC_PollForConversion(&hadc, 1000)==HAL_OK)
			{
				/*��ȡ��·��AD����ֵ*/
				voltage_ad = HAL_ADC_GetValue(&hadc);
			}
			
			
			/*��·���ѹ*/
			voltage_m    = (((float)voltage_ad) / ((float)MAX_ADC1_VALUE)) * REF_ADC1_VOLTAGE;
			
			voltage_c    = (voltage_m) *  (VOLTAGE_R1 + VOLTAGE_R2) / VOLTAGE_R2;
			
			info.voltage = (uint32_t) (voltage_c  + 0.5);
			
			/*������Ϣ*/
			if(info.debug)
			{
						printf("ADֵ       =%d\r\n", voltage_ad);
						printf("�����ĵ�ѹֵ=%f\r\n", voltage_m);
						printf("����ĵ�ѹֵ=%f\r\n", voltage_c);
						printf("��Ҫ�ĵ�ѹֵ=%d\r\n", info.voltage);
			}
}

/**
  * @brief  System sys_time update
  * @param  thread not used
  * @retval None
  */

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
			/* Release the semaphore every 1 second */
			 if(semaphore != NULL)
				{
//							printf("����A�жϲ�����\r\n");
							osSemaphoreRelease(semaphore);
//							printf("�ɹ��ͷ���ʱ���ź���\r\n");
				}
}


/*�������*/
uint8_t fill_data(struct tm *time, char *p)
{
		uint8_t   count    = 0;
		uint8_t   i				 = 0;
	
		count  =  snprintf(p, 4, "BG,");
		count +=  snprintf(p + count, 5, "001,");	/*�汾��*/
		count +=  snprintf(p + count, 8, "%.6s,", save_data + 2);		/*��վ��*/
		count +=  snprintf(p + count, 8, "%06ld,", info.lat);	/*γ��*/
		count +=  snprintf(p + count, 9, "%07ld,", info.lon);	/*����*/
		count +=  snprintf(p + count, 7, "%05d,", info.height);	/*����*/
		count +=  snprintf(p + count,	4, "01,");		/*��������*/
		count +=  snprintf(p + count,	6, "YCCL,");	/*�豸��ʶλ*/
		count +=  snprintf(p + count,	5, "000,");		/*�豸ID*/
		count +=  snprintf(p + count,	16, "20%02d%02d%02d%02d%02d%02d,",\
											time->tm_year, time->tm_mon, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec);
//		if(time->tm_min == 0)
//		{
//					count +=  snprintf(p + count,	5, "160,");
//		}
//		else
//		{
//					count +=  snprintf(p + count,	5, "001,");
//		}
		count +=  snprintf(p + count,	5, "001,");		/*֡��ʶ*/
		count +=  snprintf(p + count,	5, "006,");		/*�۲�Ҫ�ر���*/
		count +=  snprintf(p + count,	4, "%02d,", info.state_numbers);	/*�豸״̬������*/
		count +=  snprintf(p + count,	6, "AL0A,");
		if(cloud_d.cloud_one == -1)
		{
					count +=  snprintf(p + count,	6, "-----");
		}
		else if((cloud_d.cloud_one == -2) || (cloud_d.cloud_one >= 100000))
		{
					count +=  snprintf(p + count,	6, "/////");
		}
		else
		{
					count +=  snprintf(p + count,	6, "%05d", cloud_d.cloud_one);
		}
		
		count +=  snprintf(p + count,	7, ",AL1A,");
		if(cloud_d.cloud_two == -1)
		{
					count +=  snprintf(p + count,	6, "-----");
		}
		else if((cloud_d.cloud_two == -2) || (cloud_d.cloud_two >= 100000))
		{
					count +=  snprintf(p + count,	6, "/////");
		}
		else
		{
					count +=  snprintf(p + count,	6, "%05d", cloud_d.cloud_two);
		}
		
		count +=  snprintf(p + count,	7, ",AL2A,");
		if(cloud_d.cloud_three == -1)
		{
					count +=  snprintf(p + count,	6, "-----");
		}
		else if((cloud_d.cloud_three == -2) || (cloud_d.cloud_three >= 100000))
		{
					count +=  snprintf(p + count,	6, "/////");
		}
		else
		{
					count +=  snprintf(p + count,	6, "%05d", cloud_d.cloud_three);
		}
		
		count +=  snprintf(p + count,	7, ",AL3A,");
		if(cloud_d.cloud_four == -1)
		{
					count +=  snprintf(p + count,	6, "-----");
		}
		else if((cloud_d.cloud_four == -2) || (cloud_d.cloud_four >= 100000))
		{
					count +=  snprintf(p + count,	6, "/////");
		}
		else
		{
					count +=  snprintf(p + count,	6, "%05d", cloud_d.cloud_four);
		}
		
		count +=  snprintf(p + count,	7, ",AL4A,");
		if(cloud_d.cloud_five == -1)
		{
					count +=  snprintf(p + count,	6, "-----");
		}
		else if((cloud_d.cloud_five == -2) || (cloud_d.cloud_five >= 100000))
		{
					count +=  snprintf(p + count,	6, "/////");
		}
		else
		{
					count +=  snprintf(p + count,	6, "%05d", cloud_d.cloud_five);
		}
		
		count +=  snprintf(p + count,	6, ",ALE,");
		if(cloud_d.cloud_vertical_visibility == -1)
		{
					count +=  snprintf(p + count,	6, "-----");
		}
		else if((cloud_d.cloud_vertical_visibility == -2) || (cloud_d.cloud_vertical_visibility >= 100000))
		{
					count +=  snprintf(p + count,	6, "/////");
		}
		else
		{
					count +=  snprintf(p + count,	6, "%05d",  cloud_d.cloud_vertical_visibility);
		}
		
		count +=  snprintf(p + count,	9,  ",%d%d%d%d%d%d,", qc.cloud_1, qc.cloud_2, qc.cloud_3, qc.cloud_4, qc.cloud_5, qc.cloud_6);
		
		count +=  snprintf(p + count,	5, "z,%d,", state.z);
		
		if(0 != state.y_ALA)
		{
					count +=  snprintf(p + count,	9, "y_ALA,%d,", state.y_ALA);
		}
		
//		if(0 != state.wA)
//		{
//					count +=  snprintf(p + count,	6, "wA,%d,", state.wA);
//		}
		
		if(0 != state.wB)
		{
					count +=  snprintf(p + count,	6, "wB,%d,", state.wB);
		}
		
		if(0 != state.wC)
		{
					count +=  snprintf(p + count,	6, "wC,%d,", state.wC);
		}
		
		if(0 != state.sA)
		{
					count +=  snprintf(p + count,	6, "sA,%d,", state.sA);
		}
		
		if(0 != state.y)
		{
					count +=  snprintf(p + count,	5, "y,%d,", state.y);
		}
		
		if(0 != state.xB)
		{
					count +=  snprintf(p + count,	6, "xB,%d,", state.xB);
		}
		
		
		
		for(i = 0; i < count; i++)
		{
					info.check_code += p[i];
		}
		info.check_code = info.check_code % 10000;
		count +=  snprintf(p + count,	6, "%04d,", info.check_code);
		count +=  snprintf(p + count,	3, "ED");
		
		for(i = 0; i < (170-count); i++)
		{
					p[count + i] = ' ';
		}
		p[168] = '\r';
		p[169] = '\n';
		p[170] = '\0';
		
		info.check_code = 0;				/*�������ֵ*/
		
		return count;
}

