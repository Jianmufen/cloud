/* Includes ------------------------------------------------------------------*/
#include "usart_module.h"
#include "cmsis_os.h"

#include "sys_time_module.h"
#include "storage_module.h"
#include "usart.h"
#include "gpio.h"
#include "pcf8563.h"
#include "string.h"
#include "stdlib.h"
#include "eeprom.h"
#include "time_related.h"
    //
static FIL file1;          /* File object structure (FIL) */
/* Private define ------------------------------------------------------------*/
#define UART_RX_BUF_SIZE  (512) 	


#define usart1PRIORITY     osPriorityNormal
#define usart1STACK_SIZE   (256)

#define usart2PRIORITY     osPriorityNormal
#define usart2STACK_SIZE   (512)

#define usart3PRIORITY     osPriorityNormal
#define usart3STACK_SIZE   (256)

/*串口设置参数的全局变量*/
struct usart_info info = {0};
struct cloud cloud_d = {0};
/*记忆存储的全局变量*/
char save_data[36];

/*串口下载读取数据的字符数组*/
char data_down[172] = {0};

/* RTC Time通过串口设置时间*/
static RTC_TimeTypeDef Usart_Time;
static RTC_DateTypeDef Usart_Date;

static char rx1_buffer[UART_RX_BUF_SIZE]={0};  /* USART1 receiving buffer */
static char rx2_buffer[UART_RX_BUF_SIZE]={0};  /*USART2 receiving buffer */
static char rx3_buffer[UART_RX_BUF_SIZE]={0};  /*USART3 receiving buffer */
static uint32_t rx1_count=0;     /* receiving counter */
static uint32_t rx2_count=0;
static uint32_t rx3_count=0;
static uint8_t cr1_begin=false;        /* '\r'  received */ 
static uint8_t cr2_begin=false;        /* '\r'  received */ 
static uint8_t cr3_begin=false;        /* '\r'  received */
static uint8_t rx1_cplt=false;   /* received a frame of data ending with '\r'and'\n' */
static uint8_t rx2_cplt=false;   /* received a frame of data ending with '\r'and'\n' */
static uint8_t rx3_cplt=false;   /* received a frame of data ending with '\r'and'\n' */



/* os relative */
static osThreadId    Usart1ThreadHandle;
static osThreadId    Usart2ThreadHandle;
static osThreadId    Usart3ThreadHandle;
static osSemaphoreId semaphore_usart1;
static osSemaphoreId semaphore_usart2;
static osSemaphoreId semaphore_usart3;
static void Usart1_Thread(void const *argument);
static void Usart2_Thread(void const *argument);
static void Usart3_Thread(void const *argument);


/**
  * @brief  Init Storage Module. 
  * @retval 0:success;-1:failed
  */
int32_t init_usart_module(void)
{
			/*初始化一些重要的变量*/
			info.time.down_numbers = 0;
			info.debug = 0;

			if(data_eeprom_read(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
			{
					if(save_data[0] == '1')					/*若向eeprom里面写过东西*/
					{
								info.baud = save_data[34] - 48;
								
								if(info.baud == 0)
								{
										info.baud = 3;
										/*串口初始化*/
										USART1_UART_Init(9600);
										USART2_UART_Init(9600);
										USART3_UART_Init(9600);
										printf("Hello World\r\n");
								}
								else if(info.baud == 1)
								{
										info.baud = 3;
										/*串口初始化*/
										USART1_UART_Init(9600);
										USART2_UART_Init(9600);
										USART3_UART_Init(9600);
										printf("Hello World\r\n");
								}
								else if(info.baud == 2)
								{
										/*串口初始化*/
										USART1_UART_Init(4800);
										USART2_UART_Init(4800);
										USART3_UART_Init(4800);
										printf("Hello World\r\n");
								}
								else if(info.baud == 3)
								{
										/*串口初始化*/
										USART1_UART_Init(9600);
										USART2_UART_Init(9600);
										USART3_UART_Init(9600);
										printf("Hello World\r\n");
								}
								else if(info.baud == 4)
								{
										/*串口初始化*/
										USART1_UART_Init(19200);
										USART2_UART_Init(19200);
										USART3_UART_Init(19200);
										printf("Hello World\r\n");
								}
								else if(info.baud == 5)
								{
										/*串口初始化*/
										USART1_UART_Init(38400);
										USART2_UART_Init(38400);
										USART3_UART_Init(38400);
										printf("Hello World\r\n");
								}
								else if(info.baud == 6)
								{
										/*串口初始化*/
										USART1_UART_Init(57600);
										USART2_UART_Init(57600);
										USART3_UART_Init(57600);
										printf("Hello World\r\n");
								}
								else if(info.baud == 7)
								{
										/*串口初始化*/
										USART1_UART_Init(115200);
										USART2_UART_Init(115200);
										USART3_UART_Init(115200);
										printf("Hello World\r\n");
								}
								else
								{
										/*串口初始化*/
										USART1_UART_Init(9600);
										USART2_UART_Init(9600);
										USART3_UART_Init(9600);
										printf("Hello World\r\n");
								}
								printf("save_data[34]=%s\r\n", save_data + 34);		/*波特率*/
						
								printf("save_data[0]=%s\r\n", save_data);					/*记忆存储标识*/
								printf("save_data[2]=%s\r\n", save_data + 2);			/*台站号*/
								printf("save_data[9]=%s\r\n", save_data + 9);			/*经度*/
								printf("save_data[19]=%s\r\n", save_data + 19);		/*纬度*/
								printf("save_data[28]=%s\r\n", save_data + 28);		/*海拔*/
								
								info.lon = (save_data[9] - 48) * 1000000 + (save_data[10] - 48) * 100000 + (save_data[11] - 48) * 10000 +\
													 (save_data[13] - 48) * 1000 + (save_data[14] - 48) * 100 + (save_data[16] - 48) * 10 + (save_data[17] - 48);
								info.lat = (save_data[19] - 48) * 100000 + (save_data[20] - 48) * 10000 + (save_data[22] - 48) * 1000 +\
													 (save_data[23] - 48) * 100 + (save_data[25] - 48) * 10 + (save_data[26] - 48);
								info.height = atoi(save_data + 28);
								info.height /= 10;
								
								printf("lon=%07ld\r\n", info.lon);
								printf("lat=%06ld\r\n", info.lat);
								printf("height=%f\r\n", info.height);
								printf("baud=%d\r\n", info.baud);
					}
					else
					{
								snprintf(save_data,	sizeof(save_data), "1 A00001 000.00.00 00.00.00 00000 3 ");
								save_data[1] = '\0';
								save_data[8] = '\0';
								save_data[18] = '\0';
								save_data[27] = '\0';
								save_data[33] = '\0';
								save_data[35] = '\0';
						
								/*串口初始化*/
								USART1_UART_Init(9600);
								USART2_UART_Init(9600);
								USART3_UART_Init(9600);
								printf("Hello World\r\n");
								
								info.lon = (save_data[9] - 48) * 1000000 + (save_data[10] - 48) * 100000 + (save_data[11] - 48) * 10000 +\
													 (save_data[13] - 48) * 1000 + (save_data[14] - 48) * 100 + (save_data[16] - 48) * 10 + (save_data[17] - 48);
								info.lat = (save_data[19] - 48) * 100000 + (save_data[20] - 48) * 10000 + (save_data[22] - 48) * 1000 +\
													 (save_data[23] - 48) * 100 + (save_data[25] - 48) * 10 + (save_data[26] - 48);
								info.height = 0;
								info.baud = save_data[34] - 48;
						
								printf("lon=%07ld\r\n", info.lon);
								printf("lat=%06ld\r\n", info.lat);
								printf("height=%f\r\n", info.height);
								printf("baud=%d\r\n", info.baud);
								
								if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
								{
											printf("first write save data success\r\n");
											printf("save_data[0]=%s\r\n", save_data);					/*记忆存储标识*/
											printf("save_data[2]=%s\r\n", save_data + 2);			/*台站号*/
											printf("save_data[9]=%s\r\n", save_data + 9);			/*经度*/
											printf("save_data[19]=%s\r\n", save_data + 19);		/*纬度*/
											printf("save_data[28]=%s\r\n", save_data + 28);		/*海拔*/
											printf("save_data[34]=%s\r\n", save_data + 34);		/*波特率*/
								}
								else
								{
											printf("first write save data failed\r\n");
								}
					}
			}
			else
			{
						printf("data_eeprom_read1\r\n");
						strcpy(save_data, "1 A00001 000.00.00 00.00.00 00000 3 ");		/*给变量赋初值*/
						save_data[1] = '\0';
						save_data[8] = '\0';
						save_data[18] = '\0';
						save_data[27] = '\0';
						save_data[33] = '\0';
						save_data[35] = '\0';
				
						/*串口初始化*/
						USART1_UART_Init(9600);
						USART2_UART_Init(9600);
						USART3_UART_Init(9600);
						printf("Hello World\r\n");
				
						info.lon = (save_data[9] - 48) * 1000000 + (save_data[10] - 48) * 100000 + (save_data[11] - 48) * 10000 +\
											 (save_data[13] - 48) * 1000 + (save_data[14] - 48) * 100 + (save_data[16] - 48) * 10 + (save_data[17] - 48);
						info.lat = (save_data[19] - 48) * 100000 + (save_data[20] - 48) * 10000 + (save_data[22] - 48) * 1000 +\
											 (save_data[23] - 48) * 100 + (save_data[25] - 48) * 10 + (save_data[26] - 48);
						info.height = 0;
						info.baud = 3;
						printf("lon=%07ld\r\n", info.lon);
						printf("lat=%06ld\r\n", info.lat);
						printf("height=%f\r\n", info.height);
						printf("baud=%d\r\n", info.baud);
			}
			/* Define used semaphore 创建串口1的信号量*/
			osSemaphoreDef(SEM_USART1);
			/* Create the semaphore used by the two threads */
			semaphore_usart1=osSemaphoreCreate(osSemaphore(SEM_USART1), 1);
			if(semaphore_usart1 == NULL)
			{
				printf("Create Semaphore_USART1 failed!\r\n");
				return -1;
			}
			
			/* Define used semaphore 创建串口2的信号量*/
			osSemaphoreDef(SEM_USART2);
			/* Create the semaphore used by the two threads */
			semaphore_usart2=osSemaphoreCreate(osSemaphore(SEM_USART2), 1);
			if(semaphore_usart2 == NULL)
			{
				printf("Create Semaphore_USART2 failed!\r\n");
				return -1;
			}
			
			/* Define used semaphore 创建串口3的信号量*/
			osSemaphoreDef(SEM_USART3);
			/* Create the semaphore used by the two threads */
			semaphore_usart3=osSemaphoreCreate(osSemaphore(SEM_USART3), 1);
			if(semaphore_usart3 == NULL)
			{
				printf("Create Semaphore_USART3 failed!\r\n");
				return -1;
			}
		 
			
			/* Create a thread to read historical data创建串口1处理存储数据任务 */
			osThreadDef(Usart1, Usart1_Thread, usart1PRIORITY, 0, usart1STACK_SIZE);
			Usart1ThreadHandle=osThreadCreate(osThread(Usart1), NULL);
			if(Usart1ThreadHandle == NULL)
			{
				printf("Create Usart1 Thread failed!\r\n");
				return -1;
			}
			
			/* Create a thread to read historical data创建串口2处理存储数据任务 */
			osThreadDef(Usart2, Usart2_Thread, usart2PRIORITY, 0, usart2STACK_SIZE);
			Usart2ThreadHandle=osThreadCreate(osThread(Usart2), NULL);
			if(Usart2ThreadHandle == NULL)
			{
				printf("Create Usart2 Thread failed!\r\n");
				return -1;
			}
			
			
			/* Create a thread to read historical data创建串口3处理存储数据任务 */
			osThreadDef(Usart3, Usart3_Thread, usart3PRIORITY, 0, usart3STACK_SIZE);
			Usart3ThreadHandle=osThreadCreate(osThread(Usart3), NULL);
			if(Usart3ThreadHandle == NULL)
			{
				printf("Create Usart3 Thread failed!\r\n");
				return -1;
			}
			
			return 0;
}




/*串口1处理数据任务*/
static void Usart1_Thread(void const *argument)
{
		unsigned int start_seconds = 0, end_seconds = 0;
		char ret = 0;
	
		while(osSemaphoreWait(semaphore_usart1, 1)	==	osOK);	 /*消耗掉刚创建的串口信号量的资源，信号量刚创建不用释放就是可以使用的资源*/
		while(1)
		{
			if(osSemaphoreWait(semaphore_usart1, osWaitForever) == osOK)
			{
					/*调试信息*/
					if(info.debug)
					{
								printf("rx1_count=%d\r\n", rx1_count);
								printf("rx1_buffer=%s\r\n", rx1_buffer);
					}
					
					get_sys_time(&Usart_Date, &Usart_Time);																	/*得到串口时间*/
					
					
					if((strncmp(rx1_buffer,"QZ", 2) == 0) && (rx1_count == 2))							/*查询设备区站号*/	
					{
								printf("<%s>\r\n", save_data +2);		/*<57494>↙*/
					}
					else if((strncmp(rx1_buffer,"QZ", 2) == 0) && (rx1_count == 9))				/*设置设备区站号*/	
					{
								save_data[2] = rx1_buffer[3];
								save_data[3] = rx1_buffer[4];
								save_data[4] = rx1_buffer[5];
								save_data[5] = rx1_buffer[6];
								save_data[6] = rx1_buffer[7];
								save_data[7] = rx1_buffer[8];
								save_data[8] = '\0';
						
								/*将新设置的参数保存到CPU的内部EEPROM里*/
								if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 28) == HAL_OK)
								{
											printf("<T>\r\n");
								}
								else
								{
											printf("<F>\r\n");
								}
						
								/*调试信息*/
								if(info.debug)
								{
											printf("save_data[2]=%s\r\n", save_data + 2);
								}
					}
					else if((strncmp(rx1_buffer,"LONG", 4) == 0) && (rx1_count == 4))				/*查询经度*/	
					{
								printf("<%s>\r\n", save_data + 9);
					}
					else if((strncmp(rx1_buffer,"LONG", 4) == 0) && (rx1_count == 14))				/*设置经度*/	
					{
								save_data[9] = rx1_buffer[5];
								save_data[10] = rx1_buffer[6];
								save_data[11] = rx1_buffer[7];
								save_data[12] = rx1_buffer[8];
								save_data[13] = rx1_buffer[9];
								save_data[14] = rx1_buffer[10];
								save_data[15] = rx1_buffer[11];
								save_data[16] = rx1_buffer[12];
								save_data[17] = rx1_buffer[13];
								save_data[18] = '\0';
						
								info.lon = (save_data[9] - 48) * 1000000 + (save_data[10] - 48) * 100000 + (save_data[11] - 48) * 10000 +\
													 (save_data[13] - 48) * 1000 + (save_data[14] - 48) * 100 + (save_data[16] - 48) * 10 + (save_data[17] - 48);
								info.lat = (save_data[19] - 48) * 100000 + (save_data[20] - 48) * 10000 + (save_data[22] - 48) * 1000 +\
													 (save_data[23] - 48) * 100 + (save_data[25] - 48) * 10 + (save_data[26] - 48);
								if(info.debug)
								{
											printf("lon=%07ld\r\n", info.lon);
											printf("lat=%06ld\r\n", info.lat);
								}
								
								/*将新设置的参数保存到CPU的内部EEPROM里*/
								if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
								{
											printf("<T>\r\n");
								}
								else
								{
											printf("<F>\r\n");
								}
								
					}
					else if((strncmp(rx1_buffer,"LAT", 3) == 0) && (rx1_count == 3))				/*查询纬度*/	
					{
								printf("<%s>\r\n", save_data + 19);
					}
					else if((strncmp(rx1_buffer,"LAT", 3) == 0) && (rx1_count == 12))				/*设置纬度*/	
					{
								save_data[19] = rx1_buffer[4];
								save_data[20] = rx1_buffer[5];
								save_data[21] = rx1_buffer[6];
								save_data[22] = rx1_buffer[7];
								save_data[23] = rx1_buffer[8];
								save_data[24] = rx1_buffer[9];
								save_data[25] = rx1_buffer[10];
								save_data[26] = rx1_buffer[11];
								save_data[27] = '\0';
						
								info.lon = (save_data[9] - 48) * 1000000 + (save_data[10] - 48) * 100000 + (save_data[11] - 48) * 10000 +\
													 (save_data[13] - 48) * 1000 + (save_data[14] - 48) * 100 + (save_data[16] - 48) * 10 + (save_data[17] - 48);
								info.lat = (save_data[19] - 48) * 100000 + (save_data[20] - 48) * 10000 + (save_data[22] - 48) * 1000 +\
													 (save_data[23] - 48) * 100 + (save_data[25] - 48) * 10 + (save_data[26] - 48);
								if(info.debug)
								{
											printf("lon=%07ld\r\n", info.lon);
											printf("lat=%06ld\r\n", info.lat);
								}
								
								/*将新设置的参数保存到CPU的内部EEPROM里*/
								if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
								{
											printf("<T>\r\n");
								}
								else
								{
											printf("<F>\r\n");
								}
					}
					else if((strncmp(rx1_buffer,"ALT", 3) == 0) && (rx1_count == 3))				/*查询海拔高度*/	
					{
								printf("<%.1f>\r\n", info.height);
					}
					else if((strncmp(rx1_buffer,"ALT", 3) == 0) && (rx1_count >= 5))				/*设置海拔高度*/	
					{
							info.height = atof(rx1_buffer + 4);
							if(info.height > 9999.9)
							{
									printf("<F>\r\n");
							}
							
							snprintf(save_data + 28, 6, "%05d", (int)(info.height * 10));
							
							if(info.debug)
							{
									printf("height=%s\r\n", save_data + 28);
							}
							
							/*将新设置的参数保存到CPU的内部EEPROM里*/
							if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
							{
										printf("<T>\r\n");
							}
							else
							{
										printf("<F>\r\n");
							}
					}
					else if((strncmp(rx1_buffer,"DATE", 4) == 0) && (rx1_count == 4))				/*查询日期*/	
					{
								get_sys_time(&Usart_Date, &Usart_Time);														/*读取系统时间*/
								printf("<20%02d-%02d-%02d>\r\n", Usart_Date.Year, Usart_Date.Month, Usart_Date.Date);
					}
					else if((strncmp(rx1_buffer,"DATE", 4) == 0) && (rx1_count == 15))				/*设置日期*/	
					{
								Usart_Date.Year  = (rx1_buffer[7] - 48) * 10 + (rx1_buffer[8] - 48);
								Usart_Date.Month = (rx1_buffer[10] - 48) * 10 + (rx1_buffer[11] - 48);
								Usart_Date.Date	 = (rx1_buffer[13] - 48) * 10 + (rx1_buffer[14] - 48);
								if(set_sys_time(&Usart_Date, &Usart_Time))
								{
											printf("<F>\r\n");
								}
								else
								{
											printf("<T>\r\n");
								}
					}
					else if((strncmp(rx1_buffer,"TIME", 4) == 0) && (rx1_count == 4))				/*查询时间*/	
					{
								get_sys_time(&Usart_Date, &Usart_Time);														/*读取系统时间*/
								printf("<%02d-%02d-%02d>\r\n", Usart_Time.Hours, Usart_Time.Minutes, Usart_Time.Seconds);
					}
					else if((strncmp(rx1_buffer,"TIME", 4) == 0) && (rx1_count == 13))				/*设置时间*/	
					{
								Usart_Time.Hours   = (rx1_buffer[5] - 48) * 10 + (rx1_buffer[6] - 48);
								Usart_Time.Minutes = (rx1_buffer[8] - 48) * 10 + (rx1_buffer[9] - 48);
								Usart_Time.Seconds = (rx1_buffer[11] - 48) * 10 + (rx1_buffer[12] - 48);
								if(set_sys_time(&Usart_Date, &Usart_Time))
								{
											printf("<F>\r\n");
								}
								else
								{
											printf("<T>\r\n");
								}
					}
					else if((strncmp(rx1_buffer,"DOWN", 4) == 0) && (rx1_count == 44))				/*下载分钟数据:DOWN,2012-07-21,20:00:00,2012-07-24,20:00:00↙*/	
					{
								if(0 == info.time.down_numbers)
								{
											info.time.year_start   = (rx1_buffer[7] - 48) *10 + (rx1_buffer[8] - 48);				/*下载起始时间*/
											info.time.month_start  = (rx1_buffer[10] - 48) *10 + (rx1_buffer[11] - 48);
											info.time.day_start 	 = (rx1_buffer[13] - 48) *10 + (rx1_buffer[14] - 48);
											info.time.hour_start 	 = (rx1_buffer[16] - 48) *10 + (rx1_buffer[17] - 48);
											info.time.minute_start = (rx1_buffer[19] - 48) *10 + (rx1_buffer[20] - 48);
											info.time.second_start = (rx1_buffer[22] - 48) *10 + (rx1_buffer[23] - 48);
									
											info.time.year_end     = (rx1_buffer[27] - 48) *10 + (rx1_buffer[28] - 48);			/*下载结束时间*/
											info.time.month_end  	 = (rx1_buffer[30] - 48) *10 + (rx1_buffer[31] - 48);
											info.time.day_end 	   = (rx1_buffer[33] - 48) *10 + (rx1_buffer[34] - 48);
											info.time.hour_end 	   = (rx1_buffer[36] - 48) *10 + (rx1_buffer[37] - 48);
											info.time.minute_end   = (rx1_buffer[39] - 48) *10 + (rx1_buffer[40] - 48);
											info.time.second_end   = (rx1_buffer[42] - 48) *10 + (rx1_buffer[43] - 48);
											
											start_seconds = l_mktime(info.time.year_start, info.time.month_start,  info.time.day_start, \
																							 info.time.hour_start, info.time.minute_start, info.time.second_start);
											end_seconds   = l_mktime(info.time.year_end,   info.time.month_end,    info.time.day_end, \
																							 info.time.hour_end,   info.time.minute_end,   info.time.second_end);
											
											if(end_seconds > start_seconds)
											{
														info.time.flag = 1;
														info.usart_down_flag = 1;
											}
											else
											{
														info.time.flag = 0;
														info.usart_down_flag = 0;
											}
											info.time.down_numbers = (end_seconds - start_seconds) / 60 + 1;					/*下载分钟数据的条数*/
											
											/*调试信息*/
											if(info.debug)
											{
														printf("start_time:20%02d-%02d-%02d %02d:%02d:%02d\r\n", info.time.year_start, info.time.month_start,\
																				info.time.day_start, info.time.hour_start, info.time.minute_start, info.time.second_start);
														printf("end_time  :20%02d-%02d-%02d %02d:%02d:%02d\r\n", info.time.year_end, info.time.month_end,\
																				info.time.day_end, info.time.hour_end, info.time.minute_end, info.time.second_end);
														printf("down_numbers=%d\r\n", info.time.down_numbers);
														printf("flag=%d\r\n", info.time.flag);
											}
								}
								else
								{
											/*调试信息*/
											if(info.debug)
											{
														printf("down_numbers=%d\r\n", info.time.down_numbers);
											}
											printf("downing!!!!\r\n");
								}
					}
					else if((strncmp(rx1_buffer,"DOWN", 4) == 0) && (rx1_count == 48) && (rx1_buffer[45] == '1') && (rx1_buffer[46] == '6') && (rx1_buffer[47] == '0'))				/*下载小时数据:DOWN,2012-07-21,20:00:00,2012-07-24,20:00:00,160↙*/	
					{
								if(0 == info.time.down_numbers)
								{
										info.time.year_start   = (rx1_buffer[7] - 48) *10 + (rx1_buffer[8] - 48);				/*下载起始时间*/
										info.time.month_start  = (rx1_buffer[10] - 48) *10 + (rx1_buffer[11] - 48);
										info.time.day_start 	 = (rx1_buffer[13] - 48) *10 + (rx1_buffer[14] - 48);
										info.time.hour_start 	 = (rx1_buffer[16] - 48) *10 + (rx1_buffer[17] - 48);
										info.time.minute_start = (rx1_buffer[19] - 48) *10 + (rx1_buffer[20] - 48);
										info.time.second_start = (rx1_buffer[22] - 48) *10 + (rx1_buffer[23] - 48);
								
										info.time.year_end     = (rx1_buffer[27] - 48) *10 + (rx1_buffer[28] - 48);			/*下载结束时间*/
										info.time.month_end  	 = (rx1_buffer[30] - 48) *10 + (rx1_buffer[31] - 48);
										info.time.day_end 	   = (rx1_buffer[33] - 48) *10 + (rx1_buffer[34] - 48);
										info.time.hour_end 	   = (rx1_buffer[36] - 48) *10 + (rx1_buffer[37] - 48);
										info.time.minute_end   = (rx1_buffer[39] - 48) *10 + (rx1_buffer[40] - 48);
										info.time.second_end   = (rx1_buffer[42] - 48) *10 + (rx1_buffer[43] - 48);
										
										start_seconds = l_mktime(info.time.year_start, info.time.month_start,  info.time.day_start, \
																						 info.time.hour_start, info.time.minute_start, info.time.second_start);
										end_seconds   = l_mktime(info.time.year_end,   info.time.month_end,    info.time.day_end, \
																						 info.time.hour_end,   info.time.minute_end,   info.time.second_end);
										
										if(end_seconds > start_seconds)
										{
													info.time.flag = 2;
													info.usart_down_flag = 1;
										}
										else
										{
													info.time.flag = 0;
													info.usart_down_flag = 0;
										}
										info.time.down_numbers = (start_seconds - end_seconds) / 3600 + 1;					/*下载小时数据的条数*/
										
										/*调试信息*/
										if(info.debug)
										{
													printf("start_time:20%02d-%02d-%02d %02d:%02d:%02d\r\n", info.time.year_start, info.time.month_start,\
																			info.time.day_start, info.time.hour_start, info.time.minute_start, info.time.second_start);
													printf("end_time  :20%02d-%02d-%02d %02d:%02d:%02d\r\n", info.time.year_end, info.time.month_end,\
																			info.time.day_end, info.time.hour_end, info.time.minute_end, info.time.second_end);
													printf("down_numbers=%d\r\n", info.time.down_numbers);
													printf("flag=%d\r\n", info.time.flag);
													printf("usart_down_flag=%d\r\n", info.usart_down_flag);
										}	
								}
								else
								{
											/*调试信息*/
											if(info.debug)
											{
														printf("down_numbers=%d\r\n", info.time.down_numbers);
											}
											printf("downing!!!!\r\n");
								}
					}
					else if((strncmp(rx1_buffer,"READDATA", 8) == 0) && (rx1_count == 8))		/*读取实时分钟数据*/	
					{
								memset(data_down, 0, sizeof(data_down));
								ret = read_file(Usart_Date.Year, Usart_Date.Month, Usart_Date.Date, Usart_Time.Hours, Usart_Time.Minutes, data_down);
								if(ret)
								{
											printf("%s", data_down);
								}
								else
								{
											printf("<F>\r\n");
								}
					}
					else if((strncmp(rx1_buffer,"READDATA,160", 12) == 0) && (rx1_count == 12))		/*读取实时整点数据*/	
					{
								memset(data_down, 0, sizeof(data_down));
								ret = read_file(Usart_Date.Year, Usart_Date.Month, Usart_Date.Date, Usart_Time.Hours, 'H', data_down);
								if(ret)
								{
											printf("%s", data_down);
								}
								else
								{
											printf("<F>\r\n");
								}
					}
					else if((strncmp(rx1_buffer,"SETCOMWAY", 9) == 0) && (rx1_count == 9))		/*握手机制设置*/	
					{
								info.set_comway = rx1_buffer[10] - 48;
								printf("<T>\r\n");
								/*调试信息*/
								if(info.debug)
								{
											printf("set_comway=%d\r\n", info.set_comway);
								}
					}
					else if((strncmp(rx1_buffer,"POWER", 5) == 0) && (rx1_count == 5))		/*读取发射功率*/	
					{
								printf("<%03d>\r\n", info.power);
					}
					else if((strncmp(rx1_buffer,"LENS", 4) == 0) && (rx1_count == 4))		/*读取镜头污染状态*/	
					{
								printf("<%03d>\r\n", cloud_d.cloud_window_state);
					}
					else if((strncmp(rx1_buffer,"VOLTAGE", 7) == 0) && (rx1_count == 7))		/*读取采集器电压*/	
					{
								printf("<%02d>\r\n", info.voltage);
					}
					else if((strncmp(rx1_buffer,"ANGLE", 5) == 0) && (rx1_count == 5))		/*读取发射器倾角*/	
					{
								printf("<%02d>\r\n", info.angle);
					}
					else if((strncmp(rx1_buffer,"TEMP", 4) == 0) && (rx1_count == 4))		/*读取发射器温度*/	
					{
								printf("<%02d>\r\n", cloud_d.cloud_sensor_temperature);
					}
					else if((strncmp(rx1_buffer,"STA", 3) == 0) && (rx1_count == 3))		/*读取设备状态值*/	
					{
								
					}
					else if((strncmp(rx1_buffer,"profI", 5) == 0) && (rx1_count == 5))	/*读取当前时刻的消光系数*/	
					{
								
					}
					else if((strncmp(rx1_buffer,"profE", 5) == 0) && (rx1_count == 5))	/*读取当前时刻的后向散射廓线*/	
					{
								
					}
					else if((strncmp(rx1_buffer,"RESTART", 7) == 0) && (rx1_count == 7))	/*重启主采集器*/	
					{
								/*软件重启主采集器 */
								//HAL_UART_Transmit(&huart1,"主采集器重启中\r\n",(10),0xFF);
								HAL_NVIC_SystemReset();
					}
					else if((strncmp(rx1_buffer,"DEBUG", 5) == 0) && (rx1_count == 5))	/*显示或不显示调试信息*/	
					{
								if(info.debug)
								{
											info.debug = 0;
								}
								else
								{
											info.debug = 1;
								}
					}
					else if((strncmp(rx1_buffer,"VERSION", 7) == 0) && (rx1_count == 7))	/*显示版本号*/	
					{
								printf("VERSION:V1.0.1\r\n");
					}
					else if((strncmp(rx1_buffer, "MODE", 4) == 0) && (rx1_count == 6))	/*切换工作模式*/	
					{
								info.mode = rx1_buffer[5] - 48;
								if(info.debug)
								{
											printf("mode=%d\r\n", info.mode);
								}
					}
					else if((strncmp(rx1_buffer, "SETCOM", 6) == 0) && (rx1_count == 6))	/*查询波特率*/	
					{
								if(info.baud == 0)
								{
										printf("<1200 8 N 1>\r\n");
								}
								else if(info.baud == 1)
								{
										printf("<2400 8 N 1>\r\n");
								}
								else if(info.baud == 2)
								{
										printf("<4800 8 N 1>\r\n");
								}
								else if(info.baud == 3)
								{
										printf("<9600 8 N 1>\r\n");
								}
								else if(info.baud == 4)
								{
										printf("<19200 8 N 1>\r\n");
								}
								else if(info.baud == 5)
								{
										printf("<38400 8 N 1>\r\n");
								}
								else if(info.baud == 6)
								{
										printf("<57600 8 N 1>\r\n");
								}
								else if(info.baud == 7)
								{
										printf("<115200 8 N 1>\r\n");
								}
								else
								{
										info.baud = 3;
										printf("<9600 8 N 1>\r\n");
									
										/*串口初始化*/
										USART1_UART_Init(9600);
										USART2_UART_Init(9600);
										USART3_UART_Init(9600);
								}
					}
					else if((strncmp(rx1_buffer, "SETCOM", 6) == 0) && (rx1_count >= 17))	/*设置波特率*/	
					{
								if(strstr(rx1_buffer, "1200") != 0)
								{
											printf("<F>\r\n");			/*云高仪传感器的波特率不能使用1200和2400的波特率*/
								}
								else if(strstr(rx1_buffer, "2400") != 0)
								{
											printf("<F>\r\n");			/*云高仪传感器的波特率不能使用1200和2400的波特率*/
								}
								else if(strstr(rx1_buffer, "4800") != 0)
								{
											//printf("<T>\r\n");
											info.baud = 2;
											
											/*设置云高仪的波特率*/
											HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*初始化PA1为高电平，485只能发送，串口3只发送数据*/
											HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=2\r\n", 16, 0xFF);
											HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*初始化PA1为低电平，485只能接收，串口3只接收数据*/
									
											save_data[34] = info.baud + 48;
											/*将新设置的参数保存到CPU的内部EEPROM里*/
											if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
											{
														printf("<T>\r\n");
											}
											else
											{
														printf("<F>\r\n");
											}
											
											/*串口初始化*/
											USART1_UART_Init(4800);
											USART2_UART_Init(4800);
											USART3_UART_Init(4800);
								}
								else if(strstr(rx1_buffer, "9600") != 0)
								{
											//printf("<T>\r\n");
											info.baud = 3;
											
											/*设置云高仪的波特率*/
											HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*初始化PA1为高电平，485只能发送，串口3只发送数据*/
											HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=3\r\n", 16, 0xFF);
											HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*初始化PA1为低电平，485只能接收，串口3只接收数据*/
											
											save_data[34] = info.baud + 48;
											/*将新设置的参数保存到CPU的内部EEPROM里*/
											if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
											{
														printf("<T>\r\n");
											}
											else
											{
														printf("<F>\r\n");
											}
											
											/*串口初始化*/
											USART1_UART_Init(9600);
											USART2_UART_Init(9600);
											USART3_UART_Init(9600);
								}
								else if(strstr(rx1_buffer, "19200") != 0)
								{
											//printf("<T>\r\n");
											info.baud = 4;
									
											/*设置云高仪的波特率*/
											HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*初始化PA1为高电平，485只能发送，串口3只发送数据*/
											HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=4\r\n", 16, 0xFF);
											HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*初始化PA1为低电平，485只能接收，串口3只接收数据*/
									
											save_data[34] = info.baud + 48;
											/*将新设置的参数保存到CPU的内部EEPROM里*/
											if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
											{
														printf("<T>\r\n");
											}
											else
											{
														printf("<F>\r\n");
											}
											
											/*串口初始化*/
											USART1_UART_Init(19200);
											USART2_UART_Init(19200);
											USART3_UART_Init(19200);
								}
								else if(strstr(rx1_buffer, "38400") != 0)
								{
											//printf("<T>\r\n");
											info.baud = 5;
											
											/*设置云高仪的波特率*/
											HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*初始化PA1为高电平，485只能发送，串口3只发送数据*/
											HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=5\r\n", 16, 0xFF);
											HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*初始化PA1为低电平，485只能接收，串口3只接收数据*/
									
											save_data[34] = info.baud + 48;
											/*将新设置的参数保存到CPU的内部EEPROM里*/
											if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
											{
														printf("<T>\r\n");
											}
											else
											{
														printf("<F>\r\n");
											}
											
											/*串口初始化*/
											USART1_UART_Init(38400);
											USART2_UART_Init(38400);
											USART3_UART_Init(38400);
								}
								else if(strstr(rx1_buffer, "57600") != 0)
								{
											//printf("<T>\r\n");
											info.baud = 6;
											
											/*设置云高仪的波特率*/
											HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*初始化PA1为高电平，485只能发送，串口3只发送数据*/
											HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=6\r\n", 16, 0xFF);
											HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*初始化PA1为低电平，485只能接收，串口3只接收数据*/
									
											save_data[34] = info.baud + 48;
											/*将新设置的参数保存到CPU的内部EEPROM里*/
											if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
											{
														printf("<T>\r\n");
											}
											else
											{
														printf("<F>\r\n");
											}
											
											/*串口初始化*/
											USART1_UART_Init(57600);
											USART2_UART_Init(57600);
											USART3_UART_Init(57600);
								}
								else if(strstr(rx1_buffer, "115200") != 0)
								{
											//printf("<T>\r\n");
											info.baud = 7;
											
											/*设置云高仪的波特率*/
											HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*初始化PA1为高电平，485只能发送，串口3只发送数据*/
											HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=7\r\n", 16, 0xFF);
											HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*初始化PA1为低电平，485只能接收，串口3只接收数据*/
									
											save_data[34] = info.baud + 48;
											/*将新设置的参数保存到CPU的内部EEPROM里*/
											if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
											{
														printf("<T>\r\n");
											}
											else
											{
														printf("<F>\r\n");
											}
											
											/*串口初始化*/
											USART1_UART_Init(115200);
											USART2_UART_Init(115200);
											USART3_UART_Init(115200);
								}
								else
								{
										printf("<F>\r\n");
								}
					}
					else
					{
								printf("BADCOMMAND\r\n");
					}
				
					rx1_count=0;
					rx1_cplt=false;                                              
					memset(rx1_buffer,0,sizeof(rx1_buffer));  
			}
			else 
			{
					printf("BADCOMMAND\r\n");
					
					/*调试信息*/
					if(info.debug)
					{
								printf("rx1_buffer=%s\r\n", rx1_buffer);
					}
			}
		}
}




/*串口2处理数据任务*/
static void Usart2_Thread(void const *argument)
{
			unsigned int start_seconds = 0, end_seconds = 0;
			char ret = 0;
	
			if(init_sys_time_module()<0)
			{
						printf("init sys_time module failed!\r\n");
			}
			else
			{
						printf("init sys_time module ok!\r\n");
			}
			
			
			if(init_storage_module()<0)
			{
						printf("init storage module failed!\r\n");
			}
			else
			{
						printf("init storage module ok!\r\n");
			}
	
			while(osSemaphoreWait(semaphore_usart2, 1)	==	osOK);
			while(1)
			{
						if(osSemaphoreWait(semaphore_usart2,osWaitForever)==osOK)
						{
									HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);     /*拉高PA0，485只能发送数据，串口2只发送数据*/
									/*调试信息*/
									if(info.debug)
									{
												printf("rx2_count=%d\r\n", rx2_count);
												printf("rx2_buffer=%s\r\n", rx2_buffer);
									}
									
									get_sys_time(&Usart_Date, &Usart_Time);																	/*得到串口时间*/
									
									
									if((strncmp(rx2_buffer,"QZ", 2) == 0) && (rx2_count == 2))							/*查询设备区站号*/	
									{
												printf("<%s>\r\n", save_data +2);		/*<57494>↙*/
									}
									else if((strncmp(rx2_buffer,"QZ", 2) == 0) && (rx2_count == 9))				/*设置设备区站号*/	
									{
												save_data[2] = rx2_buffer[3];
												save_data[3] = rx2_buffer[4];
												save_data[4] = rx2_buffer[5];
												save_data[5] = rx2_buffer[6];
												save_data[6] = rx2_buffer[7];
												save_data[7] = rx2_buffer[8];
												save_data[8] = '\0';
										
												/*将新设置的参数保存到CPU的内部EEPROM里*/
												if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 28) == HAL_OK)
												{
															printf("<T>\r\n");
												}
												else
												{
															printf("<F>\r\n");
												}
										
												/*调试信息*/
												if(info.debug)
												{
															printf("save_data[2]=%s\r\n", save_data + 2);
												}
									}
									else if((strncmp(rx2_buffer,"LONG", 4) == 0) && (rx2_count == 4))				/*查询经度*/	
									{
												printf("<%s>\r\n", save_data + 9);
									}
									else if((strncmp(rx2_buffer,"LONG", 4) == 0) && (rx2_count == 14))				/*设置经度*/	
									{
												save_data[9] = rx2_buffer[5];
												save_data[10] = rx2_buffer[6];
												save_data[11] = rx2_buffer[7];
												save_data[12] = rx2_buffer[8];
												save_data[13] = rx2_buffer[9];
												save_data[14] = rx2_buffer[10];
												save_data[15] = rx2_buffer[11];
												save_data[16] = rx2_buffer[12];
												save_data[17] = rx2_buffer[13];
												save_data[18] = '\0';
												
												/*将新设置的参数保存到CPU的内部EEPROM里*/
												if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 28) == HAL_OK)
												{
															printf("<T>\r\n");
												}
												else
												{
															printf("<F>\r\n");
												}
												
									}
									else if((strncmp(rx2_buffer,"LAT", 3) == 0) && (rx2_count == 3))				/*查询纬度*/	
									{
												printf("<%s>\r\n", save_data + 19);
									}
									else if((strncmp(rx2_buffer,"LAT", 3) == 0) && (rx2_count == 12))				/*设置纬度*/	
									{
												save_data[19] = rx2_buffer[4];
												save_data[20] = rx2_buffer[5];
												save_data[21] = rx2_buffer[6];
												save_data[22] = rx2_buffer[7];
												save_data[23] = rx2_buffer[8];
												save_data[24] = rx2_buffer[9];
												save_data[25] = rx2_buffer[10];
												save_data[26] = rx2_buffer[11];
												save_data[27] = '\0';
										
												/*将新设置的参数保存到CPU的内部EEPROM里*/
												if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 28) == HAL_OK)
												{
															printf("<T>\r\n");
												}
												else
												{
															printf("<F>\r\n");
												}
									}
									else if((strncmp(rx2_buffer,"DATE", 4) == 0) && (rx2_count == 4))				/*查询日期*/	
									{
												get_sys_time(&Usart_Date, &Usart_Time);														/*读取系统时间*/
												printf("<20%02d-%02d-%02d>\r\n", Usart_Date.Year, Usart_Date.Month, Usart_Date.Date);
									}
									else if((strncmp(rx2_buffer,"DATE", 4) == 0) && (rx2_count == 15))				/*设置日期*/	
									{
												Usart_Date.Year  = (rx2_buffer[7] - 48) * 10 + (rx2_buffer[8] - 48);
												Usart_Date.Month = (rx2_buffer[10] - 48) * 10 + (rx2_buffer[11] - 48);
												Usart_Date.Date	 = (rx2_buffer[13] - 48) * 10 + (rx2_buffer[14] - 48);
												if(set_sys_time(&Usart_Date, &Usart_Time))
												{
															printf("<F>\r\n");
												}
												else
												{
															printf("<T>\r\n");
												}
									}
									else if((strncmp(rx2_buffer,"TIME", 4) == 0) && (rx2_count == 4))				/*查询时间*/	
									{
												get_sys_time(&Usart_Date, &Usart_Time);														/*读取系统时间*/
												printf("<%02d-%02d-%02d>\r\n", Usart_Time.Hours, Usart_Time.Minutes, Usart_Time.Seconds);
									}
									else if((strncmp(rx2_buffer,"TIME", 4) == 0) && (rx2_count == 13))				/*设置时间*/	
									{
												Usart_Time.Hours   = (rx2_buffer[5] - 48) * 10 + (rx2_buffer[6] - 48);
												Usart_Time.Minutes = (rx2_buffer[8] - 48) * 10 + (rx2_buffer[9] - 48);
												Usart_Time.Seconds = (rx2_buffer[11] - 48) * 10 + (rx2_buffer[12] - 48);
												if(set_sys_time(&Usart_Date, &Usart_Time))
												{
															printf("<F>\r\n");
												}
												else
												{
															printf("<T>\r\n");
												}
									}
									else if((strncmp(rx2_buffer,"DOWN", 4) == 0) && (rx2_count == 44))				/*下载分钟数据:DOWN,2012-07-21,20:00:00,2012-07-24,20:00:00↙*/	
									{
												if(0 == info.time.down_numbers)
												{
															info.time.year_start   = (rx2_buffer[7] - 48) *10 + (rx2_buffer[8] - 48);				/*下载起始时间*/
															info.time.month_start  = (rx2_buffer[10] - 48) *10 + (rx2_buffer[11] - 48);
															info.time.day_start 	 = (rx2_buffer[13] - 48) *10 + (rx2_buffer[14] - 48);
															info.time.hour_start 	 = (rx2_buffer[16] - 48) *10 + (rx2_buffer[17] - 48);
															info.time.minute_start = (rx2_buffer[19] - 48) *10 + (rx2_buffer[20] - 48);
															info.time.second_start = (rx2_buffer[22] - 48) *10 + (rx2_buffer[23] - 48);
													
															info.time.year_end     = (rx2_buffer[27] - 48) *10 + (rx2_buffer[28] - 48);			/*下载结束时间*/
															info.time.month_end  	 = (rx2_buffer[30] - 48) *10 + (rx2_buffer[31] - 48);
															info.time.day_end 	   = (rx2_buffer[33] - 48) *10 + (rx2_buffer[34] - 48);
															info.time.hour_end 	   = (rx2_buffer[36] - 48) *10 + (rx2_buffer[37] - 48);
															info.time.minute_end   = (rx2_buffer[39] - 48) *10 + (rx2_buffer[40] - 48);
															info.time.second_end   = (rx2_buffer[42] - 48) *10 + (rx2_buffer[43] - 48);
															
															start_seconds = l_mktime(info.time.year_start, info.time.month_start,  info.time.day_start, \
																											 info.time.hour_start, info.time.minute_start, info.time.second_start);
															end_seconds   = l_mktime(info.time.year_end,   info.time.month_end,    info.time.day_end, \
																											 info.time.hour_end,   info.time.minute_end,   info.time.second_end);
															
															if(end_seconds > start_seconds)
															{
																		info.time.flag = 1;
																		info.usart_down_flag = 1;
															}
															else
															{
																		info.time.flag = 0;
																		info.usart_down_flag = 0;
															}
															info.time.down_numbers = (end_seconds - start_seconds) / 60 + 1;					/*下载分钟数据的条数*/
															
															/*调试信息*/
															if(info.debug)
															{
																		printf("start_time:20%02d-%02d-%02d %02d:%02d:%02d\r\n", info.time.year_start, info.time.month_start,\
																								info.time.day_start, info.time.hour_start, info.time.minute_start, info.time.second_start);
																		printf("end_time  :20%02d-%02d-%02d %02d:%02d:%02d\r\n", info.time.year_end, info.time.month_end,\
																								info.time.day_end, info.time.hour_end, info.time.minute_end, info.time.second_end);
																		printf("down_numbers=%d\r\n", info.time.down_numbers);
																		printf("flag=%d %d\r\n", info.time.flag, info.usart_down_flag);
															}
												}
												else
												{
															/*调试信息*/
															if(info.debug)
															{
																		printf("down_numbers=%d\r\n", info.time.down_numbers);
															}
															printf("downing!!!!\r\n");
												}
									}
									else if((strncmp(rx2_buffer,"DOWN", 4) == 0) && (rx2_count == 48) && (rx2_buffer[45] == '1') && (rx2_buffer[46] == '6') && (rx2_buffer[47] == '0'))				/*下载小时数据:DOWN,2012-07-21,20:00:00,2012-07-24,20:00:00,160↙*/	
									{
												if(0 == info.time.down_numbers)
												{
														info.time.year_start   = (rx2_buffer[7] - 48) *10 + (rx2_buffer[8] - 48);				/*下载起始时间*/
														info.time.month_start  = (rx2_buffer[10] - 48) *10 + (rx2_buffer[11] - 48);
														info.time.day_start 	 = (rx2_buffer[13] - 48) *10 + (rx2_buffer[14] - 48);
														info.time.hour_start 	 = (rx2_buffer[16] - 48) *10 + (rx2_buffer[17] - 48);
														info.time.minute_start = (rx2_buffer[19] - 48) *10 + (rx2_buffer[20] - 48);
														info.time.second_start = (rx2_buffer[22] - 48) *10 + (rx2_buffer[23] - 48);
												
														info.time.year_end     = (rx2_buffer[27] - 48) *10 + (rx2_buffer[28] - 48);			/*下载结束时间*/
														info.time.month_end  	 = (rx2_buffer[30] - 48) *10 + (rx2_buffer[31] - 48);
														info.time.day_end 	   = (rx2_buffer[33] - 48) *10 + (rx2_buffer[34] - 48);
														info.time.hour_end 	   = (rx2_buffer[36] - 48) *10 + (rx2_buffer[37] - 48);
														info.time.minute_end   = (rx2_buffer[39] - 48) *10 + (rx2_buffer[40] - 48);
														info.time.second_end   = (rx2_buffer[42] - 48) *10 + (rx2_buffer[43] - 48);
														
														start_seconds = l_mktime(info.time.year_start, info.time.month_start,  info.time.day_start, \
																										 info.time.hour_start, info.time.minute_start, info.time.second_start);
														end_seconds   = l_mktime(info.time.year_end,   info.time.month_end,    info.time.day_end, \
																										 info.time.hour_end,   info.time.minute_end,   info.time.second_end);
														
														if(end_seconds > start_seconds)
														{
																	info.time.flag = 2;
																	info.usart_down_flag = 1;
														}
														else
														{
																	info.time.flag = 0;
																	info.usart_down_flag = 1;
														}
														info.time.down_numbers = (end_seconds - start_seconds) / 3600 + 1;					/*下载小时数据的条数*/
														
														/*调试信息*/
														if(info.debug)
														{
																	printf("start_time:20%02d-%02d-%02d %02d:%02d:%02d\r\n", info.time.year_start, info.time.month_start,\
																							info.time.day_start, info.time.hour_start, info.time.minute_start, info.time.second_start);
																	printf("end_time  :20%02d-%02d-%02d %02d:%02d:%02d\r\n", info.time.year_end, info.time.month_end,\
																							info.time.day_end, info.time.hour_end, info.time.minute_end, info.time.second_end);
																	printf("down_numbers=%d\r\n", info.time.down_numbers);
																	printf("flag=%d\r\n", info.time.flag);
																	printf("usart_down_flag=%d\r\n", info.usart_down_flag);
														}	
												}
												else
												{
															/*调试信息*/
															if(info.debug)
															{
																		printf("down_numbers=%d\r\n", info.time.down_numbers);
															}
															printf("downing!!!!\r\n");
												}
									}
									else if((strncmp(rx2_buffer,"READDATA", 8) == 0) && (rx2_count == 8))		/*读取实时分钟数据*/	
									{
												memset(data_down, 0, sizeof(data_down));
												ret = read_file(Usart_Date.Year, Usart_Date.Month, Usart_Date.Date, Usart_Time.Hours, Usart_Time.Minutes, data_down);
												if(ret)
												{
															printf("%s", data_down);
												}
												else
												{
															printf("<F>\r\n");
												}
									}
									else if((strncmp(rx2_buffer,"READDATA,160", 12) == 0) && (rx2_count == 12))		/*读取实时整点数据*/	
									{
												memset(data_down, 0, sizeof(data_down));
												ret = read_file(Usart_Date.Year, Usart_Date.Month, Usart_Date.Date, Usart_Time.Hours, 'H', data_down);
												if(ret)
												{
															printf("%s", data_down);
												}
												else
												{
															printf("<F>\r\n");
												}
									}
									else if((strncmp(rx2_buffer,"SETCOMWAY", 9) == 0) && (rx2_count == 11))		/*握手机制设置*/	
									{
												info.set_comway = rx2_buffer[10] - 48;
												printf("<T>\r\n");
												/*调试信息*/
												if(info.debug)
												{
															printf("set_comway=%d\r\n", info.set_comway);
												}
									}
									else if((strncmp(rx2_buffer,"POWER", 5) == 0) && (rx2_count == 5))		/*读取发射功率*/	
									{
												printf("<%03d>\r\n", info.power);
									}
									else if((strncmp(rx2_buffer,"LENS", 4) == 0) && (rx2_count == 4))		/*读取镜头污染状态*/	
									{
												printf("<%03d>\r\n", cloud_d.cloud_window_state);
									}
									else if((strncmp(rx2_buffer,"VOLTAGE", 7) == 0) && (rx2_count == 7))		/*读取采集器电压*/	
									{
												printf("<%02d>\r\n", info.voltage);
									}
									else if((strncmp(rx2_buffer,"ANGLE", 5) == 0) && (rx2_count == 5))		/*读取发射器倾角*/	
									{
												printf("<%02d>\r\n", info.angle);
									}
									else if((strncmp(rx2_buffer,"TEMP", 4) == 0) && (rx2_count == 4))		/*读取发射器温度*/	
									{
												printf("<%02d>\r\n", cloud_d.cloud_sensor_temperature);
									}
									else if((strncmp(rx2_buffer,"STA", 3) == 0) && (rx2_count == 3))		/*读取设备状态值*/	
									{
												
									}
									else if((strncmp(rx2_buffer,"profI", 5) == 0) && (rx2_count == 5))	/*读取当前时刻的消光系数*/	
									{
												
									}
									else if((strncmp(rx2_buffer,"profE", 5) == 0) && (rx2_count == 5))	/*读取当前时刻的后向散射廓线*/	
									{
												
									}
									else if((strncmp(rx2_buffer,"RESTART", 7) == 0) && (rx2_count == 7))	/*重启主采集器*/	
									{
												/*软件重启主采集器 */
												//HAL_UART_Transmit(&huart1,"主采集器重启中\r\n",(10),0xFF);
												HAL_NVIC_SystemReset();
									}
									else if((strncmp(rx2_buffer,"DEBUG", 5) == 0) && (rx2_count == 5))	/*显示或不显示调试信息*/	
									{
												if(info.debug)
												{
															info.debug = 0;
												}
												else
												{
															info.debug = 1;
												}
									}
									else if((strncmp(rx2_buffer,"VERSION", 7) == 0) && (rx2_count == 7))	/*显示版本号*/	
									{
												printf("VERSION:V1.0.1\r\n");
									}
									else if((strncmp(rx2_buffer, "MODE", 4) == 0) && (rx2_count == 6))	/*切换工作模式*/	
									{
												info.mode = rx2_buffer[5] - 48;
												if(info.debug)
												{
															printf("mode=%d\r\n", info.mode);
												}
									}
									else if((strncmp(rx2_buffer, "SETCOM", 6) == 0) && (rx2_count == 6))	/*查询波特率*/	
									{
												if(info.baud == 0)
												{
														printf("<1200 8 N 1>\r\n");
												}
												else if(info.baud == 1)
												{
														printf("<2400 8 N 1>\r\n");
												}
												else if(info.baud == 2)
												{
														printf("<4800 8 N 1>\r\n");
												}
												else if(info.baud == 3)
												{
														printf("<9600 8 N 1>\r\n");
												}
												else if(info.baud == 4)
												{
														printf("<19200 8 N 1>\r\n");
												}
												else if(info.baud == 5)
												{
														printf("<38400 8 N 1>\r\n");
												}
												else if(info.baud == 6)
												{
														printf("<57600 8 N 1>\r\n");
												}
												else if(info.baud == 7)
												{
														printf("<115200 8 N 1>\r\n");
												}
												else
												{
														info.baud = 3;
														printf("<9600 8 N 1>\r\n");
													
														/*串口初始化*/
														USART1_UART_Init(9600);
														USART2_UART_Init(9600);
														USART3_UART_Init(9600);
												}
									}
									else if((strncmp(rx2_buffer, "SETCOM", 6) == 0) && (rx2_count >= 17))	/*设置波特率*/	
									{
												if(strstr(rx2_buffer, "1200") != 0)
												{
															printf("<F>\r\n");			/*云高仪传感器的波特率不能使用1200和2400的波特率*/
												}
												else if(strstr(rx2_buffer, "2400") != 0)
												{
															printf("<F>\r\n");			/*云高仪传感器的波特率不能使用1200和2400的波特率*/
												}
												else if(strstr(rx2_buffer, "4800") != 0)
												{
															//printf("<T>\r\n");
															info.baud = 2;
															
															/*设置云高仪的波特率*/
															HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*初始化PA1为高电平，485只能发送，串口3只发送数据*/
															HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=2\r\n", 16, 0xFF);
															HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*初始化PA1为低电平，485只能接收，串口3只接收数据*/
													
															save_data[34] = info.baud + 48;
															/*将新设置的参数保存到CPU的内部EEPROM里*/
															if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
															{
																		printf("<T>\r\n");
															}
															else
															{
																		printf("<F>\r\n");
															}
															
															/*串口初始化*/
															USART1_UART_Init(4800);
															USART2_UART_Init(4800);
															USART3_UART_Init(4800);
												}
												else if(strstr(rx2_buffer, "9600") != 0)
												{
															//printf("<T>\r\n");
															info.baud = 3;
															
															/*设置云高仪的波特率*/
															HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*初始化PA1为高电平，485只能发送，串口3只发送数据*/
															HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=3\r\n", 16, 0xFF);
															HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*初始化PA1为低电平，485只能接收，串口3只接收数据*/
															
															save_data[34] = info.baud + 48;
															/*将新设置的参数保存到CPU的内部EEPROM里*/
															if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
															{
																		printf("<T>\r\n");
															}
															else
															{
																		printf("<F>\r\n");
															}
															
															/*串口初始化*/
															USART1_UART_Init(9600);
															USART2_UART_Init(9600);
															USART3_UART_Init(9600);
												}
												else if(strstr(rx2_buffer, "19200") != 0)
												{
															//printf("<T>\r\n");
															info.baud = 4;
															
															/*设置云高仪的波特率*/
															HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*初始化PA1为高电平，485只能发送，串口3只发送数据*/
															HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=4\r\n", 16, 0xFF);
															HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*初始化PA1为低电平，485只能接收，串口3只接收数据*/
													
															save_data[34] = info.baud + 48;
															/*将新设置的参数保存到CPU的内部EEPROM里*/
															if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
															{
																		printf("<T>\r\n");
															}
															else
															{
																		printf("<F>\r\n");
															}
															
															/*串口初始化*/
															USART1_UART_Init(19200);
															USART2_UART_Init(19200);
															USART3_UART_Init(19200);
												}
												else if(strstr(rx2_buffer, "38400") != 0)
												{
															//printf("<T>\r\n");
															info.baud = 5;
													
															/*设置云高仪的波特率*/
															HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*初始化PA1为高电平，485只能发送，串口3只发送数据*/
															HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=5\r\n", 16, 0xFF);
															HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*初始化PA1为低电平，485只能接收，串口3只接收数据*/
													
															save_data[34] = info.baud + 48;
															/*将新设置的参数保存到CPU的内部EEPROM里*/
															if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
															{
																		printf("<T>\r\n");
															}
															else
															{
																		printf("<F>\r\n");
															}
															
															/*串口初始化*/
															USART1_UART_Init(38400);
															USART2_UART_Init(38400);
															USART3_UART_Init(38400);
												}
												else if(strstr(rx2_buffer, "57600") != 0)
												{
															//printf("<T>\r\n");
															info.baud = 6;
															
															/*设置云高仪的波特率*/
															HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*初始化PA1为高电平，485只能发送，串口3只发送数据*/
															HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=6\r\n", 16, 0xFF);
															HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*初始化PA1为低电平，485只能接收，串口3只接收数据*/
													
															save_data[34] = info.baud + 48;
															/*将新设置的参数保存到CPU的内部EEPROM里*/
															if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
															{
																		printf("<T>\r\n");
															}
															else
															{
																		printf("<F>\r\n");
															}
															
															/*串口初始化*/
															USART1_UART_Init(57600);
															USART2_UART_Init(57600);
															USART3_UART_Init(57600);
												}
												else if(strstr(rx2_buffer, "115200") != 0)
												{
															//printf("<T>\r\n");
															info.baud = 7;
															
															/*设置云高仪的波特率*/
															HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);     /*初始化PA1为高电平，485只能发送，串口3只发送数据*/
															HAL_UART_Transmit(&huart3, (uint8_t *)"set 16:Baud=7\r\n", 16, 0xFF);
															HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);     /*初始化PA1为低电平，485只能接收，串口3只接收数据*/
													
															save_data[34] = info.baud + 48;
															/*将新设置的参数保存到CPU的内部EEPROM里*/
															if(data_eeprom_write(EEPROM_ADDR, (uint8_t *)&save_data, 36) == HAL_OK)
															{
																		printf("<T>\r\n");
															}
															else
															{
																		printf("<F>\r\n");
															}
															
															/*串口初始化*/
															USART1_UART_Init(115200);
															USART2_UART_Init(115200);
															USART3_UART_Init(115200);
												}
												else
												{
														printf("<F>\r\n");
												}
									}
									else
									{
												printf("BADCOMMAND\r\n");
									}
									
									rx2_count=0;
									rx2_cplt=false;                                              /* clear the flag of receive */
									memset(rx2_buffer,0,sizeof(rx2_buffer));                      /* clear the register of receive */
									
									HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);     /*拉低PA0，485只能接收数据，串口2只接收数据*/
						}
			 }
}







/*串口3处理数据任务*/
static void Usart3_Thread(void const *argument)
{
			while(osSemaphoreWait(semaphore_usart3, 1)	==	osOK);
			while(1)
			{
				if(osSemaphoreWait(semaphore_usart3, osWaitForever)==osOK)
				{
						if((strstr(rx3_buffer, "X1TA") != 0) && (rx3_count > 240))
						{
									HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);     /*拉低PB5，点亮*/
									if(info.mode)						/*只当中转站，将接收到的数据直接发送出去就行啦，不需要进行任何其他无理操作*/
									{
												HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);     /*拉高PB6，点亮信号出灯*/
												rx3_buffer[283] = '\r';
												rx3_buffer[284] = '\n';
												rx3_buffer[285] = 0x04;
												rx3_buffer[286] = '\0';
												printf("%s", rx3_buffer);
									}
									else
									{
												info.data_flag	=	1;																																															/*接收到数据*/
												/*调试信息*/
												if(info.debug)
												{
															printf("rx3_count=%d\r\n", rx3_count);
															printf("rx3_buffer=%s\r\n", rx3_buffer + 8);
															
												}
												
												cloud_d.cloud_output_interval = (rx3_buffer[8] - 48) * 100 + (rx3_buffer[9] - 48) * 10 + (rx3_buffer[10] - 48);		/*输出间隔秒*/
										
												cloud_d.cloud_layer_numbers   = (rx3_buffer[30] - 48);																														/*云层数量*/
												if(cloud_d.cloud_layer_numbers != 5)
												{
															info.data_flag	=	0;
												}
										
												if(rx3_buffer[32] == 'N')																																													/*第一层云*/
												{
															cloud_d.cloud_one = -1;
												}
												else
												{
															cloud_d.cloud_one		= (rx3_buffer[32] - 48)*10000 + (rx3_buffer[33] - 48)*1000 + \
																										(rx3_buffer[34] - 48)*100 + (rx3_buffer[35] - 48)*10 + (rx3_buffer[36] - 48);
												}
												
												if(rx3_buffer[38] == 'N')																																														/*第二层云*/
												{
															cloud_d.cloud_two = -1;
												}
												else
												{
															cloud_d.cloud_two		= (rx3_buffer[38] - 48)*10000 + (rx3_buffer[39] - 48)*1000 + \
																										(rx3_buffer[40] - 48)*100 + (rx3_buffer[41] - 48)*10 + (rx3_buffer[42] - 48);					
												}
												
												if(rx3_buffer[44] == 'N')																																														/*第三层云*/
												{
															cloud_d.cloud_three = -1;
												}
												else
												{
															cloud_d.cloud_three	= (rx3_buffer[44] - 48)*10000 + (rx3_buffer[45] - 48)*1000 + \
																										(rx3_buffer[46] - 48)*100 + (rx3_buffer[47] - 48)*10 + (rx3_buffer[48] - 48);					
												}
												
												if(rx3_buffer[50] == 'N')																																														/*第四层云*/
												{
															cloud_d.cloud_four = -1;
												}
												else
												{
															cloud_d.cloud_four = (rx3_buffer[50] - 48)*10000 + (rx3_buffer[51] - 48)*1000 + \
																									 (rx3_buffer[52] - 48)*100 + (rx3_buffer[53] - 48)*10 + (rx3_buffer[54] - 48);					
												}
										
												if(rx3_buffer[56] == 'N')																																														/*第五层云*/
												{
															cloud_d.cloud_five = -1;
												}
												else
												{
															cloud_d.cloud_five = (rx3_buffer[56] - 48)*10000 + (rx3_buffer[57] - 48)*1000 + \
																									 (rx3_buffer[58] - 48)*100 + (rx3_buffer[59] - 48)*10 + (rx3_buffer[60] - 48);					
												}
											
												if(rx3_buffer[92] == 'N')																																													/*垂直能见度*/
												{
															cloud_d.cloud_vertical_visibility = -1;
												}
												else
												{
															cloud_d.cloud_vertical_visibility = (rx3_buffer[92] - 48)*10000 + (rx3_buffer[93] - 48)*1000 + \
																																	(rx3_buffer[94] - 48)*100 + (rx3_buffer[95] - 48)*10 + (rx3_buffer[96] - 48);
												}
										
												/*数据单位m*/
												cloud_d.cloud_unit_1 = rx3_buffer[109];
												cloud_d.cloud_unit_2 = rx3_buffer[110];
												
												if((rx3_buffer[208] == 'O') &&(rx3_buffer[209] == 'K'))																															/*系统状态*/
												{
															cloud_d.cloud_system_state = 0;
												}
												else
												{
															cloud_d.cloud_system_state = 1;																																												
												}
												
																									
												sscanf(rx3_buffer + 211, "%04d", &cloud_d.cloud_environment_temperature);																							/*外部温度*/
										
												sscanf(rx3_buffer + 216, "%04d", &cloud_d.cloud_sensor_temperature);																									/*传感器温度*/
												
												sscanf(rx3_buffer + 221, "%04d", &cloud_d.cloud_inside_temperature);																									/*内部温度*/
												
												sscanf(rx3_buffer + 236, "%06d", &cloud_d.cloud_laser_time);																													/*激光工作时间*/
												
												sscanf(rx3_buffer + 243, "%03d", &cloud_d.cloud_window_state);																												/*窗口状态*/
												
												sscanf(rx3_buffer + 247, "%05d", &cloud_d.cloud_laser_rate);																													/*激光重复率*/
												
												sscanf(rx3_buffer + 253, "%03d", &cloud_d.cloud_receiver_state);																											/*接收器状态*/
												
												sscanf(rx3_buffer + 257, "%03d", &cloud_d.cloud_light_state);																													/*光源状态*/
												
												/*调试信息*/
												if(info.debug)
												{
															printf("系统状态=%d\r\n", cloud_d.cloud_system_state);
															printf("单位=%c%c\r\n", cloud_d.cloud_unit_1, cloud_d.cloud_unit_2);
															printf("输出间隔时间=%dS\r\n", cloud_d.cloud_output_interval);
															printf("云层=%d\r\n", cloud_d.cloud_layer_numbers);
															printf("云高=%d	%d	%d	%d	%d	%d\r\n", cloud_d.cloud_one, cloud_d.cloud_two, cloud_d.cloud_three, cloud_d.cloud_four, cloud_d.cloud_five, cloud_d.cloud_vertical_visibility);
															printf("温度=%d	%d	%d\r\n", cloud_d.cloud_environment_temperature, cloud_d.cloud_sensor_temperature,\
																																															cloud_d.cloud_inside_temperature);
															printf("状态=%d	%d	%d	%d	%d\r\n", cloud_d.cloud_laser_time, cloud_d.cloud_window_state, cloud_d.cloud_laser_rate,\
																																															cloud_d.cloud_receiver_state, cloud_d.cloud_light_state);
												}
									}
									HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);     /*拉高PB5，点灭信号入灯*/
									HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);     /*拉高PB6，点灭信号出灯*/
						}
					
						rx3_count = 0;
						rx3_cplt = false;                                              /* clear the flag of receive */
						memset(rx3_buffer, 0, sizeof(rx3_buffer));                      /* clear the register of receive */
				}
			}
}




/**串口1中断函数*/
void USART1_IRQHandler(void)
{
  UART_HandleTypeDef *huart=&huart1;
  uint32_t tmp_flag = 0, tmp_it_source = 0;

  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_PE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_PE);  
  /* UART parity error interrupt occurred ------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  { 
    huart->ErrorCode |= HAL_UART_ERROR_PE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_FE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_ERR);
  /* UART frame error interrupt occurred -------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_FE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_NE);
  /* UART noise error interrupt occurred -------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_NE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_ORE);
  /* UART Over-Run interrupt occurred ----------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_ORE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_RXNE);
  /* UART in mode Receiver ---------------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_Receive_IT(huart);*/
    uint8_t data = 0, r0 = 0, r1 = 0;
  
    data = huart->Instance->DR;  /* the byte just received  */
    
		
//		if(!rx1_cplt)
//    {
//      if(data=='<')
//      {
//        cr1_begin=true;
//        rx1_count=0;
//        rx1_buffer[rx1_count]=data;
//        rx1_count++; 
//      }
//     
//      else if(cr1_begin==true)
//      {
//        rx1_buffer[rx1_count]=data;
//        rx1_count++; 
//        if(rx1_count>UART_RX_BUF_SIZE-1)  /* rx buffer full */
//        {
//          /* Set transmission flag: trasfer complete*/
//          rx1_cplt=true;
//        }
//        
//        if(data=='>')
//        {
//          rx1_cplt=true;
//          cr1_begin=false;
//        }
//      }
//      else
//      {
//        rx1_count=0;
//      }
//    }

		
		
//    if(!rx1_cplt)
//    {
//      if(cr1_begin==true)  /* received '\r' */
//      {
//        cr1_begin=false;
//        if(data=='\n')  /* received '\r' and '\n' */
//        {
//          /* Set transmission flag: trasfer complete*/
//          rx1_cplt=true;
//        }
//        else
//        {
//          rx1_count=0;
//        }
//      }
//      else
//      {
//        if(data=='\r')  /* get '\r' */
//        {
//          cr1_begin=true;
//        }
//        else  /* continue saving data */
//        {
//          rx1_buffer[rx1_count]=data;
//          rx1_count++;
//          if(rx1_count>UART_RX_BUF_SIZE-1)  /* rx buffer full */
//          {
//            /* Set transmission flag: trasfer complete*/
//            rx1_cplt=true;
//          } 
//        }
//       }
//      }



//		if(!rx1_cplt)
//    {
//      if(data == 'B')
//      {
//					rx1_count = 0;
//					r0 = rx1_count;
//					rx1_buffer[rx1_count] = data;
//					rx1_count++; 
//      }
//			
//			
//			else if(data == 'G')
//      {
//					r1 = rx1_count;
//					if((0 == r0) && (1 == r1))
//					{
//							cr1_begin = true;
//							rx1_buffer[rx1_count] = data;
//							rx1_count++; 
//					}
//					else
//					{
//							cr1_begin = false;
//							rx1_count = 0;
//					}
//      }
//     
//      else if(cr1_begin == true)
//      {
//					rx1_buffer[rx1_count] = data;
//					rx1_count++; 
//					if(rx1_count > UART_RX_BUF_SIZE - 1)  /* rx buffer full */
//					{
//						/* Set transmission flag: trasfer complete*/
//						rx1_cplt = true;
//					}
//					
//					if(data == 'E')
//					{
//						rx1_cplt = true;
//						cr1_begin = false;
//					}
//					
//					if(data == 'D')
//					{
//						rx1_cplt = true;
//						cr1_begin = false;
//					}
//      }
//      else
//      {
//					rx1_count = 0;
//      }
//    }
    
		
		if(!rx1_cplt)
    {
      if(cr1_begin==true)  /* received '\r' */
      {
        cr1_begin=false;
        if(data=='\n')  /* received '\r' and '\n' */
        {
          /* Set transmission flag: trasfer complete*/
          rx1_cplt=true;
        }
        else
        {
          rx1_count=0;
        }
      }
      else
      {
        if(data=='\r')  /* get '\r' */
        {
          cr1_begin=true;
        }
        else  /* continue saving data */
        {
          rx1_buffer[rx1_count]=data;
          rx1_count++;
          if(rx1_count>UART_RX_BUF_SIZE-1)  /* rx buffer full */
          {
            /* Set transmission flag: trasfer complete*/
            rx1_cplt = true;
          } 
        }
       }
      }
		
  	 /* received a data frame 数据接收完成就释放互斥量*/
    if(rx1_cplt == true)
    {
//					printf("串口1接收到了数据\r\n");
					if(semaphore_usart1!=NULL)
					{
									 /* Release mutex */
//									printf("semaphore_usart1\r\n");
									osSemaphoreRelease(semaphore_usart1);
//									printf("osSemaphoreReleaseosSemaphoreReleasesemaphore_usart1\r\n");
					}
    }

   
    }
  
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_TXE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TXE);
  /* UART in mode Transmitter ------------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_Transmit_IT(huart);*/
  }

  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_TC);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TC);
  /* UART in mode Transmitter end --------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_EndTransmit_IT(huart);*/
  }  

  if(huart->ErrorCode != HAL_UART_ERROR_NONE)
  {
    /* Clear all the error flag at once */
    __HAL_UART_CLEAR_PEFLAG(huart);
    
    /* Set the UART state ready to be able to start again the process */
    huart->State = HAL_UART_STATE_READY;
    
    HAL_UART_ErrorCallback(huart);
  } 
}





/**串口2中断函数*/
void USART2_IRQHandler(void)
{
  UART_HandleTypeDef *huart=&huart2;
  uint32_t tmp_flag = 0, tmp_it_source = 0;

  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_PE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_PE);  
  /* UART parity error interrupt occurred ------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  { 
    huart->ErrorCode |= HAL_UART_ERROR_PE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_FE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_ERR);
  /* UART frame error interrupt occurred -------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_FE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_NE);
  /* UART noise error interrupt occurred -------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_NE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_ORE);
  /* UART Over-Run interrupt occurred ----------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_ORE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_RXNE);
  /* UART in mode Receiver ---------------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_Receive_IT(huart);*/
    uint8_t data=0;
  
    data=huart->Instance->DR;  /* the byte just received  */
    
    
  
     
//			if(!rx2_cplt)
//    {
//      if(data=='<')
//      {
//        cr2_begin=true;
//        rx2_count=0;
//        rx2_buffer[rx2_count]=data;
//        rx2_count++; 
//      }
//     
//      else if(cr2_begin==true)
//      {
//        rx2_buffer[rx2_count]=data;
//        rx2_count++; 
//        if(rx2_count>UART_RX_BUF_SIZE-1)  /* rx buffer full */
//        {
//          /* Set transmission flag: trasfer complete*/
//          rx2_cplt=true;
//        }
//        
//        if(data=='>')
//        {
//          rx2_cplt=true;
//          cr2_begin=false;
//        }
//      }
//      else
//      {
//        rx2_count=0;
//      }

//    }
		
		if(!rx2_cplt)
    {
      if(cr2_begin==true)  /* received '\r' */
      {
        cr2_begin=false;
        if(data=='\n')  /* received '\r' and '\n' */
        {
          /* Set transmission flag: trasfer complete*/
          rx2_cplt=true;
        }
        else
        {
          rx2_count=0;
        }
      }
      else
      {
        if(data=='\r')  /* get '\r' */
        {
          cr2_begin=true;
        }
        else  /* continue saving data */
        {
          rx2_buffer[rx2_count]=data;
          rx2_count++;
          if(rx2_count>UART_RX_BUF_SIZE-1)  /* rx buffer full */
          {
            /* Set transmission flag: trasfer complete*/
            rx2_cplt=true;
          } 
        }
       }
      }
		
		 /* received a data frame 数据接收完成就释放信号量*/
    if(rx2_cplt==true)
    {
//			printf("串口2接收到数据\r\n");
      if(semaphore_usart2 != NULL)
      {
        /* Release the semaphore */
//				printf("信号量2创建成功\r\n");
        osSemaphoreRelease(semaphore_usart2);
//				printf("释放串口2的信号\r\n");
      }
    }
		
		
    }
  
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_TXE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TXE);
  /* UART in mode Transmitter ------------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_Transmit_IT(huart);*/
  }

  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_TC);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TC);
  /* UART in mode Transmitter end --------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_EndTransmit_IT(huart);*/
  }  

  if(huart->ErrorCode != HAL_UART_ERROR_NONE)
  {
    /* Clear all the error flag at once */
    __HAL_UART_CLEAR_PEFLAG(huart);
    
    /* Set the UART state ready to be able to start again the process */
    huart->State = HAL_UART_STATE_READY;
    
    HAL_UART_ErrorCallback(huart);
  } 
}




/**串口3中断函数*/
void USART3_IRQHandler(void)
{
  UART_HandleTypeDef *huart=&huart3;
  uint32_t tmp_flag = 0, tmp_it_source = 0;

  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_PE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_PE);  
  /* UART parity error interrupt occurred ------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  { 
    huart->ErrorCode |= HAL_UART_ERROR_PE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_FE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_ERR);
  /* UART frame error interrupt occurred -------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_FE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_NE);
  /* UART noise error interrupt occurred -------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_NE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_ORE);
  /* UART Over-Run interrupt occurred ----------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    huart->ErrorCode |= HAL_UART_ERROR_ORE;
  }
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_RXNE);
  /* UART in mode Receiver ---------------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_Receive_IT(huart);*/
    uint8_t data=0;
  
    data=huart->Instance->DR;  /* the byte just received  */
    
    
  
     
			if(!rx3_cplt)
			{
					if(data == 0x02)
					{
							cr3_begin=true;
							rx3_count=0;
							rx3_buffer[rx3_count]=data;
							rx3_count++; 
					}
					else if(cr3_begin==true)
					{
							rx3_buffer[rx3_count]=data;
							rx3_count++; 
							if(rx3_count>UART_RX_BUF_SIZE-1)  /* rx buffer full */
							{
									/* Set transmission flag: trasfer complete*/
									rx3_cplt=true;
							}
							
							if(data == 0x04)
							{
									rx3_cplt=true;
									cr3_begin=false;
							}
					}
					else
					{
							rx3_count=0;
					}
			}
		
//		if(!rx3_cplt)
//    {
//					if(cr3_begin==true)  /* received '\r' */
//					{
//								cr3_begin=false;
//								if(data=='\n')  /* received '\r' and '\n' */
//								{
//											/* Set transmission flag: trasfer complete*/
//											rx3_cplt=true;
//								}
//								else
//								{
//											rx3_count=0;
//								}
//					}
//					else
//					{
//								if(data=='\r')  /* get '\r' */
//								{
//											cr3_begin=true;
//								}
//								else  /* continue saving data */
//								{
//											rx3_buffer[rx3_count]=data;
//											rx3_count++;
//											if(rx3_count>UART_RX_BUF_SIZE-1)  /* rx buffer full */
//											{
//														/* Set transmission flag: trasfer complete*/
//														rx3_cplt=true;
//											} 
//								}
//					 }
//      }
		
		 /* received a data frame 数据接收完成就释放信号量*/
    if(rx3_cplt==true)
    {
//			printf("串口3接收到数据\r\n");
      if(semaphore_usart3!=NULL)
      {
						/* Release the semaphore */
//						printf("信号量3创建成功\r\n");
						osSemaphoreRelease(semaphore_usart3);
//						printf("释放串口3的信号\r\n");
      }
    }
		
		
    }
  
  
  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_TXE);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TXE);
  /* UART in mode Transmitter ------------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_Transmit_IT(huart);*/
  }

  tmp_flag = __HAL_UART_GET_FLAG(huart, UART_FLAG_TC);
  tmp_it_source = __HAL_UART_GET_IT_SOURCE(huart, UART_IT_TC);
  /* UART in mode Transmitter end --------------------------------------------*/
  if((tmp_flag != RESET) && (tmp_it_source != RESET))
  {
    /*UART_EndTransmit_IT(huart);*/
  }  

  if(huart->ErrorCode != HAL_UART_ERROR_NONE)
  {
    /* Clear all the error flag at once */
    __HAL_UART_CLEAR_PEFLAG(huart);
    
    /* Set the UART state ready to be able to start again the process */
    huart->State = HAL_UART_STATE_READY;
    
    HAL_UART_ErrorCallback(huart);
  } 
}




