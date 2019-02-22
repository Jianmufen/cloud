/* Includes ------------------------------------------------------------------*/
#include "display_module.h"
#include "cmsis_os.h"

#include "button.h"
#include "time.h"
#include "adc.h"
#include "ds18b20.h"
#include "sys_time_module.h"
#include "storage_module.h"
#include "usart_module.h"


/* Private typedef -----------------------------------------------------------*/
/** 
  * @brief  Button Callback Function Definition
  */
typedef void (*ButtonCallbackFunc)(void *,int16_t);
/** 
  * @brief  LCD Display Function Definition
  */
typedef void (*DisplayFunc)(void *);

/** 
  * @brief  LCD Display Screen Structure
  */
typedef struct DisplayScreen
{
  int16_t selection;         /* current selection */
  int16_t screen_number;     /* current screen number */
	int16_t screen_leftright;
  
  ButtonCallbackFunc button_func;      /* button callback function */
  DisplayFunc        display_func;     /* display callback function */
} DisplayScreenTypeDef;
/** 
  * @brief  LCD Display Menu Structure
  */
typedef struct DisplayMenu
{
  DisplayScreenTypeDef Screen;
  
  struct DisplayMenu *prev;    /* previous menu */
  struct DisplayMenu *next;    /* next menu */
} DisplayMenuTypeDef;



/* Private define ------------------------------------------------------------*/
#define displaySTACK_SIZE   configMINIMAL_STACK_SIZE
#define displayPRIORITY     osPriorityNormal
#define QUEUE_SIZE ((uint32_t)1)

/* display */
#define MAX_DISPLAY_ON_TIME              (60)   /* display on time , unit:second */
//#define DATA_SCREEN_NUMBER               (2)    /* data screen number */
#define TIME_SCREEN_SELECTION_NUMBER     (6)    /* time screen selection number */
//#define PRESURE_SCREEN_SELECTION_NUMBER  (4)    /*��ѹ�����������Ĺ������*/
/* Private variables ---------------------------------------------------------*/

/* RTC Time*/
static RTC_TimeTypeDef Time;
static RTC_DateTypeDef Date;
/* Set Time */
static RTC_TimeTypeDef setTime;
static RTC_DateTypeDef setDate;

/* display */
static uint8_t disp_buf[128];
static uint8_t disp_on=true;
static int16_t disp_on_count=MAX_DISPLAY_ON_TIME;  /* display on time counter */
static float   voltage;      //��ص�ѹ
//static int16_t temperature;  //�����¶�

//static uint8_t nian,yue,ri,shi,fen,miao;//����ʱ�����
//static int16_t disp_on_count=MAX_DISPLAY_ON_TIME;  /* ��ʾ������ʱ��display on time counter */
//static	uint16_t rainzhi,rainzhi1,rainzhi2,rainzhi3,rainzhi4,rainzhi5,rainzhi6;
/* Display Menus and Screens -------------------------------------------------*/
/* Menus */
static DisplayMenuTypeDef *CurrentDisplay;  /* Current Display Menu */
static DisplayMenuTypeDef MainMenu;  /* Main Menu */
static DisplayMenuTypeDef ModeMenu;  /* Mode Menu */
static DisplayMenuTypeDef FrequencyMenu;  /* frequency   Menu */
static DisplayMenuTypeDef TimeMenu;  /* Time Set Menu */
static DisplayMenuTypeDef Normal_ModeMenu;  /* ���� Menu */
static DisplayMenuTypeDef Channel_ModeMenu;  /*  ͨ��ģʽ Menu */

/* os relative��ʾ���� */
static osThreadId DisplayThreadHandle;
/*static osSemaphoreId semaphore;*/
static osMessageQId ButtonQueue;  /* ��������button queue */
/* Private function prototypes -----------------------------------------------*/
static void Display_Thread(void const *argument);
static void init_display_menus(void);

static void ModeScreen_ButtonHandler(void *Menu,int16_t button);
static void ModeScreen_Display(void *Menu);//ģʽ�� 
static void TimeScreen_ButtonHandler(void *Menu,int16_t button);
static void TimeScreen_Display(void *Menu);//ʱ��������
static void FrequencyScreen_ButtonHandler(void *Menu,int16_t button);
static void FrequencyScreen_Display(void *Menu);//��������
static void MainScreen_ButtonHandler(void *Menu,int16_t button);
static void MainScreen_Display(void *Menu);//���˵���
static void Normal_ModeScreen_ButtonHandler(void *Menu,int16_t button);
static void Normal_ModeScreen_Display(void *Menu);//ƽ����
static void Channel_ModeScreen_ButtonHandler(void *Menu,int16_t button);
static void Channel_ModeScreen_Display(void *Menu);//PAMS��

__STATIC_INLINE void short_delay(void);
__STATIC_INLINE void turn_off_display(void);
__STATIC_INLINE void turn_on_display(void);

/**��ʼ����ʾ����
  * @brief  Init Display Module. 
  * @retval 0:success;-1:failed
  */
int32_t init_display_module(void)
{
	Button_Init();
	
  /* init display menus */
  init_display_menus();
 
  /* Create the queue used by the button interrupt to pass the button value.����һ�����䰴��ֵ�ö��� */
  osMessageQDef(button_queue,QUEUE_SIZE,uint16_t);
  ButtonQueue=osMessageCreate(osMessageQ(button_queue),NULL);
  if(ButtonQueue == NULL)
  {
    printf("Create Button Queue failed!\r\n");
    return -1;
  }
  
  /* Create a thread to update system date and time������ʾ���� */
  osThreadDef(Display, Display_Thread, displayPRIORITY, 0, displaySTACK_SIZE);
  DisplayThreadHandle=osThreadCreate(osThread(Display), NULL);
  if(DisplayThreadHandle == NULL)
  {
    printf("Create Display Thread failed!\r\n");
    return -1;
  }
  
  
  return 0;
}


/*���ɼ�����·���¶Ⱥ͵�ص�ѹ*/
void Voltage_Temperature(void)
{
	uint32_t value=0;
	
	ADC_ChannelConfTypeDef sConfig={0};
  
  /* channel : ADC_CHANNEL_1 */
  sConfig.Channel=ADC_CHANNEL_1;
  sConfig.Rank=ADC_REGULAR_RANK_1;
  sConfig.SamplingTime=ADC_SAMPLETIME_96CYCLES;
  HAL_ADC_ConfigChannel(&hadc,&sConfig);
  
  HAL_ADC_Start(&hadc);  /* Start ADC Conversion */
  
  /* Wait for regular group conversion to be completed. */
  if(HAL_ADC_PollForConversion(&hadc,1000)==HAL_OK)
  {
    value = HAL_ADC_GetValue(&hadc);
  }
	voltage=((float)value/4095)*3.3;
	printf("��ѹ��%f\r\n",voltage);
	
	//temperature = Temp_Get();
	//printf("�¶ȣ�%d\r\n",temperature);
}
/**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin: Specifies the port pin connected to corresponding EXTI line.
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* Disable Interrupt */
  HAL_NVIC_DisableIRQ((IRQn_Type)(BUTTONs_EXTI_IRQn));
  
  /* eliminate key jitter */
  short_delay();
  if(HAL_GPIO_ReadPin(BUTTONs_GPIO_PORT,GPIO_Pin)==GPIO_PIN_RESET)
  {
    /* Put the Button Value to the Message Queue */
    if(GPIO_Pin==BUTTON_ESCAPE_PIN)  /* ESCAPE button */
    {
      if(ButtonQueue)
      {
        osMessagePut(ButtonQueue, BUTTON_ESCAPE, 100);
      }
    }
    
    if(GPIO_Pin==BUTTON_ENTER_PIN)  /* ENTER button */
    {
      if(ButtonQueue)
      {
        osMessagePut(ButtonQueue, BUTTON_ENTER, 100);
      }
    }
    
    if(GPIO_Pin==BUTTON_LEFT_PIN)  /* LEFT button */
    {
      if(ButtonQueue)
      {
        osMessagePut(ButtonQueue, BUTTON_LEFT, 100);
      }
    }
    
    if(GPIO_Pin==BUTTON_RIGHT_PIN)  /* RIGHT button */
    {
      if(ButtonQueue)
      {
        osMessagePut(ButtonQueue, BUTTON_RIGHT, 100);
      }
    }
  
  }
  
  /* Enable Interrupt */
  HAL_NVIC_EnableIRQ((IRQn_Type)(BUTTONs_EXTI_IRQn));
}

/*��ʾ����*/
static void Display_Thread(void const *argument)
{
  osEvent event;
  int16_t button_value=0;
  struct tm datetime={0};
  
  (void)get_sys_time_tm(&datetime);
  
  /*osDelay(3000);*/
  LCM_DispFill(0);  /* clear screen */
  

  while(1)
  {
    /* Get the message from the queue */
    event = osMessageGet(ButtonQueue, 1000);

    if (event.status == osEventMessage)
    {
      
      /* get button value */
      button_value=event.value.v;
      
      /* button handler */
      if(disp_on && CurrentDisplay->Screen.button_func)
      {
        (*CurrentDisplay->Screen.button_func)(CurrentDisplay,button_value);
      }

      
      /* turn on display */
      turn_on_display();
		}
		/* get data&time */
    (void)get_sys_time(&Date,&Time);
		
		/* display on */
    if(disp_on==true)
    {
      
      /* display function */
      if(CurrentDisplay->Screen.display_func)
      {
        (*CurrentDisplay->Screen.display_func)(CurrentDisplay);
      }
      
      
      
      
      disp_on_count--;
      if(disp_on_count<MAX_DISPLAY_ON_TIME*2/3)
      {
        /* turn off backlight */
        LCD_BL_OFF();
      }
      if(disp_on_count<1)
      {
        /* turn off display */
        turn_off_display();
      }
    
    }
		
	}
}

/*��ʾ�˵�*/
static void init_display_menus(void)
{
  /* Mode Menu */
  ModeMenu.prev=&MainMenu;
  ModeMenu.next=&Normal_ModeMenu;
  ModeMenu.Screen.screen_number=0;
  ModeMenu.Screen.selection=0;
  ModeMenu.Screen.button_func=ModeScreen_ButtonHandler;
  ModeMenu.Screen.display_func=ModeScreen_Display;
	
	/* Normal_Mode Menu */
  Normal_ModeMenu.prev=&ModeMenu;
  Normal_ModeMenu.next=NULL;
  Normal_ModeMenu.Screen.screen_number=2;
  Normal_ModeMenu.Screen.selection=0;
  Normal_ModeMenu.Screen.button_func=Normal_ModeScreen_ButtonHandler;
  Normal_ModeMenu.Screen.display_func=Normal_ModeScreen_Display;

  /* Channel_Mode Menu */
  Channel_ModeMenu.prev=&ModeMenu;
  Channel_ModeMenu.next=NULL;
  Channel_ModeMenu.Screen.screen_number=0;
  Channel_ModeMenu.Screen.selection=1;
  Channel_ModeMenu.Screen.button_func=Channel_ModeScreen_ButtonHandler;
  Channel_ModeMenu.Screen.display_func=Channel_ModeScreen_Display;
	
  /* Time Menu */
  TimeMenu.prev=&MainMenu;
  TimeMenu.next=NULL;
  TimeMenu.Screen.screen_number=0;
  TimeMenu.Screen.selection=0;
  TimeMenu.Screen.button_func=TimeScreen_ButtonHandler;
  TimeMenu.Screen.display_func=TimeScreen_Display;
  
  /* Frequency Menu */
  FrequencyMenu.prev=&MainMenu;
  FrequencyMenu.next=NULL;
  FrequencyMenu.Screen.screen_number=0;
  FrequencyMenu.Screen.selection=0;
  FrequencyMenu.Screen.button_func=FrequencyScreen_ButtonHandler;
  FrequencyMenu.Screen.display_func=FrequencyScreen_Display;
  
  /* Main Menu */
  MainMenu.prev=NULL;
  MainMenu.next=&ModeMenu;
  MainMenu.Screen.screen_number=0;
  MainMenu.Screen.selection=0;
  MainMenu.Screen.button_func=MainScreen_ButtonHandler;
  MainMenu.Screen.display_func=MainScreen_Display;
	
  /* Current Menu */
  /*CurrentDisplay=&DataMenu;   //just use Data Menu for now*/
  CurrentDisplay=&MainMenu;  /* display main menu */
}

/*ģʽ�˵�����*/
static void ModeScreen_ButtonHandler(void *Menu,int16_t button)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  
  switch(button)
  {
  case BUTTON_ESCAPE:
    if(menu->prev)
    {
      CurrentDisplay=menu->prev;
      CurrentDisplay->Screen.screen_number=0;
      CurrentDisplay->Screen.selection=0;
      LCM_DispFill(0);  /* clear screen */
    }
    break;
  case BUTTON_ENTER:
		if(menu->next)
     {
       CurrentDisplay=menu->next;
       CurrentDisplay->Screen.screen_number=0;
       CurrentDisplay->Screen.selection=0;
			 CurrentDisplay->Screen.screen_leftright=0;
       LCM_DispFill(0);  /* clear screen */
			 
			 if(menu->Screen.selection == 0)
			 {
				 //����ģʽ
				Model_Flag  = false;
				HAL_UART_Transmit(&huart2,(uint8_t *)"(N)",strlen("(N)"),0xFF);//֪ͨ��λ����������ģʽ
			 }
			 else if(menu->Screen.selection == 1)
			 {
				  //ͨ��ģʽ
				 Model_Flag  = true;
				 HAL_UART_Transmit(&huart2,(uint8_t *)"(C)",strlen("(C)"),0xFF);//֪ͨ��λ������ͨ��ģʽ
			 }
      }
    break;
  case BUTTON_LEFT:
    menu->Screen.selection--;
    LCM_DispFill(0);  /* clear screen */
    break;
  case BUTTON_RIGHT:
    menu->Screen.selection++;
    LCM_DispFill(0);  /* clear screen */
    break;
  default:
    break;
  }
  
  if(menu->Screen.selection < 0)
  {
    menu->Screen.selection=1;
  }
  else if(menu->Screen.selection > 1)
  {
    menu->Screen.selection=0;
  }
  
}

/*ģʽ����ʾ*/
static void ModeScreen_Display(void *Menu)
{
	DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  uint8_t highlight=false;
	
	/*����ģʽ*/
	highlight=(menu->Screen.selection==0);
	LCM_DispCHSStr(0,0,4,HZ_ZCMS,highlight); 
  
	/*ͨ��ģʽ*/
	highlight=(menu->Screen.selection==1);
	LCM_DispCHSStr(1,0,4,HZ_TDMS,highlight);   
	
	/* Determine the next Menu */
  switch(menu->Screen.selection)
  {
  case 0:   /* Normal Menu */
    menu->next=&Normal_ModeMenu;
    break;
  case 1:   /* Channel Menu */
    menu->next=&Channel_ModeMenu;
    break;
  default:
    //menu->next=&PamsMenu;
    break;
  }
	
}

/*����ģʽ�˵�����*/
static void Normal_ModeScreen_ButtonHandler(void *Menu,int16_t button)
{
	DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  
  switch(button)
  {
  case BUTTON_ESCAPE:
    if(menu->prev)
    {
      CurrentDisplay=menu->prev;
      CurrentDisplay->Screen.screen_number=0;
      CurrentDisplay->Screen.selection=0;
      LCM_DispFill(0);  /* clear screen */
    }
    break;
  case BUTTON_ENTER:
    break;
  case BUTTON_LEFT:
    menu->Screen.screen_number--;
    LCM_DispFill(0);  /* clear screen */
    break;
  case BUTTON_RIGHT:
    menu->Screen.screen_number++;
    LCM_DispFill(0);  /* clear screen */
    break;
  default:
    break;
  }
  
  if(menu->Screen.screen_number<0)
  {
    menu->Screen.screen_number=1;
  }
  else if(menu->Screen.screen_number>1)
  {
    menu->Screen.screen_number=0;
  }
}

/*����ģʽ����ʾ*/
static void Normal_ModeScreen_Display(void *Menu)
{
	DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
	
	if(menu->Screen.screen_number==0)
	{
		/*˲ʱ����*/
		LCM_DispCHSStr(0,0,4,HZ_SSFX,false);
		snprintf((char *)disp_buf,sizeof(disp_buf),":  %03d",wind_direction_1S);
    LCM_DispStr8_16(0,8,(char *)disp_buf,false);
		LCM_DispCHSStr(0,7,1,HZ_Du,false);
		if(wind_speed_1S < 0.05)
		{
			LCM_DispStr8_16(0,8,":  ///",false);
		}
		
		/*˲ʱ����*/
		LCM_DispCHSStr(1,0,4,HZ_SSFS,false);
		snprintf((char *)disp_buf,sizeof(disp_buf),":%4.1fm/s",wind_speed_1S);
    LCM_DispStr8_16(1,8,(char *)disp_buf,false);
		
		/*һ�ַ���*/
		LCM_DispCHSStr(2,0,4,HZ_YFFX,false);
		snprintf((char *)disp_buf,sizeof(disp_buf),":  %03d",wind_direction_1M);
    LCM_DispStr8_16(2,8,(char *)disp_buf,false);
		LCM_DispCHSStr(2,7,1,HZ_Du,false);
		if(wind_speed_1M < 0.05)
		{
			LCM_DispStr8_16(2,8,":  ///",false);
		}
		
		/*һ�ַ���*/
		LCM_DispCHSStr(3,0,4,HZ_YFFS,false);
		snprintf((char *)disp_buf,sizeof(disp_buf),":%4.1fm/s",wind_speed_1M);
    LCM_DispStr8_16(3,8,(char *)disp_buf,false);
	}
	else if(menu->Screen.screen_number==1)
	{
		/*���ַ���*/
		LCM_DispCHSStr(0,0,4,HZ_LFFX,false);
		snprintf((char *)disp_buf,sizeof(disp_buf),":  %03d",wind_direction_2M);
    LCM_DispStr8_16(0,8,(char *)disp_buf,false);
		LCM_DispCHSStr(0,7,1,HZ_Du,false);
		if(wind_speed_2M < 0.05)
		{
			LCM_DispStr8_16(0,8,":  ///",false);
		}
		
		/*���ַ���*/
		LCM_DispCHSStr(1,0,4,HZ_LFFS,false);
		snprintf((char *)disp_buf,sizeof(disp_buf),":%4.1fm/s",wind_speed_2M);
    LCM_DispStr8_16(1,8,(char *)disp_buf,false);
		
		/*ʮ�ַ���*/
		LCM_DispCHSStr(2,0,4,HZ_SFFX,false);
		snprintf((char *)disp_buf,sizeof(disp_buf),":  %03d",wind_direction_10M);
    LCM_DispStr8_16(2,8,(char *)disp_buf,false);
		LCM_DispCHSStr(2,7,1,HZ_Du,false);
		if(wind_speed_10M < 0.05)
		{
			LCM_DispStr8_16(2,8,":  ///",false);
		}
		
		/*ʮ�ַ���*/
		LCM_DispCHSStr(3,0,4,HZ_SFFS,false);
		snprintf((char *)disp_buf,sizeof(disp_buf),":%4.1fm/s",wind_speed_10M);
    LCM_DispStr8_16(3,8,(char *)disp_buf,false);
		
	}
}

/*ͨ��ģʽ����*/
static void Channel_ModeScreen_ButtonHandler(void *Menu,int16_t button)
{
	DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  
  switch(button)
  {
  case BUTTON_ESCAPE:
    if(menu->prev)
    {
      CurrentDisplay=menu->prev;
      CurrentDisplay->Screen.screen_number=0;
      CurrentDisplay->Screen.selection=1;
      LCM_DispFill(0);  /* clear screen */
			
			/*�˳�ͨ��ģʽ������������ģʽ*/
			Model_Flag  = false;
			HAL_UART_Transmit(&huart2,(uint8_t *)"(C)",strlen("(C)"),0xFF);//֪ͨ��λ����������ģʽ
    }
    break;
  case BUTTON_ENTER:
		 /* get system time */
    get_sys_time(&setDate,&setTime);
    break;
  case BUTTON_LEFT:
    break;
  case BUTTON_RIGHT:
    break;
  default:
    break;
  }
}

/*ͨ��ģʽ����ʾ*/
static void Channel_ModeScreen_Display(void *Menu)
{
	//DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
	
	/* Date */
  snprintf((char *)disp_buf,sizeof(disp_buf),"   20%02u-%02u-%02u   ",
           Date.Year,Date.Month,Date.Date);
  LCM_DispStr8_16(0,0,(char *)disp_buf,false);
  /* Time */
  snprintf((char *)disp_buf,sizeof(disp_buf),"    %02u:%02u:%02u",
           Time.Hours,Time.Minutes,Time.Seconds);
  LCM_DispStr8_16(1,0,(char *)disp_buf,false);
	
	if(Speed_Flag == false)
	{
		/*˲ʱ����*/
		LCM_DispCHSStr(2,0,4,HZ_SSFX,false);
		snprintf((char *)disp_buf,sizeof(disp_buf),":  %03d",wind_direction);
		LCM_DispStr8_16(2,8,(char *)disp_buf,false);
		LCM_DispCHSStr(2,7,1,HZ_Du,false);
	
		/*˲ʱ����*/
		LCM_DispCHSStr(3,0,4,HZ_SSFS,false);
		snprintf((char *)disp_buf,sizeof(disp_buf),":%4.1fm/s",wind_speed);
		LCM_DispStr8_16(3,8,(char *)disp_buf,false);
	}
	else if(Speed_Flag == true)
	{
		/*˲ʱ����*/
		LCM_DispCHSStr(2,0,4,HZ_SSFX,false);
		LCM_DispStr8_16(2,8,":  ///",false);
		LCM_DispCHSStr(2,7,1,HZ_Du,false);
		
		/*˲ʱ����*/
		LCM_DispCHSStr(3,0,4,HZ_SSFS,false);
		LCM_DispStr8_16(3,8,": 0.0m/s",false);
	}
	
}

/*ʱ��������*/
static void TimeScreen_ButtonHandler(void *Menu,int16_t button)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  int16_t selected_timevalue=0,min_value=0,max_value=0;
  uint8_t *selected=NULL;
  
  switch(menu->Screen.selection)
  {
  case 0:  /* year */
    selected=&setDate.Year;
    selected_timevalue=setDate.Year;
    min_value=0;
    max_value=99;
    break;
  case 1:  /* month */
    selected=&setDate.Month;
    selected_timevalue=setDate.Month;
    min_value=1;
    max_value=12;
    break;
  case 2:  /* day */
    selected=&setDate.Date;
    selected_timevalue=setDate.Date;
    min_value=1;
    max_value=31;
    break;
  case 3:  /* hour */
    selected=&setTime.Hours;
    selected_timevalue=setTime.Hours;
    min_value=0;
    max_value=23/*59*/;   /* 16.3.23 hour:0-23 */
    break;
  case 4:  /* minute */
    selected=&setTime.Minutes;
    selected_timevalue=setTime.Minutes;
    min_value=0;
    max_value=59;
    break;
  case 5:  /* second */
    selected=&setTime.Seconds;
    selected_timevalue=setTime.Seconds;
    min_value=0;
    max_value=59;
    break;
  default:
    menu->Screen.selection=5;
    selected=&setTime.Seconds;
    selected_timevalue=setTime.Seconds;
    min_value=0;
    max_value=59;
    break;
  }
  
  switch(button)
  {
  case BUTTON_ESCAPE:
    /* set data&time */
    /* fill unused value */
    setTime.DayLightSaving=RTC_DAYLIGHTSAVING_NONE;
    setTime.StoreOperation=RTC_STOREOPERATION_RESET;
    setTime.SubSeconds=0;
    setTime.TimeFormat=RTC_HOURFORMAT12_AM;
    if(set_sys_time(&setDate,&setTime)<0)
    {
      LCM_DispStr8_16(3,0,"set failed!",false);
    }
    else
    {
      LCM_DispStr8_16(3,0,"set ok!",false);
    }
    osDelay(500);
    
    if(menu->prev)
    {
      CurrentDisplay=menu->prev;
      CurrentDisplay->Screen.screen_number=0;
      CurrentDisplay->Screen.selection=2;
      LCM_DispFill(0);  /* clear screen */
    }
    break;
  case BUTTON_ENTER:
    menu->Screen.selection++;
    break;
  case BUTTON_LEFT:
    selected_timevalue--;
    break;
  case BUTTON_RIGHT:
    selected_timevalue++;
    break;
  default:
    break;
  }
  
  if(menu->Screen.selection<0)
  {
    menu->Screen.selection=TIME_SCREEN_SELECTION_NUMBER-1;
  }
  else if(menu->Screen.selection>TIME_SCREEN_SELECTION_NUMBER-1)
  {
    menu->Screen.selection=0;
  }
  
  if(selected_timevalue<min_value)
  {
    selected_timevalue=max_value;
  }
  else if(selected_timevalue>max_value)
  {
    selected_timevalue=min_value;
  }
  /* set selected value */
  *selected=selected_timevalue;
}

/*ʱ������ʾ*/
static void TimeScreen_Display(void *Menu)
{
  DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  uint8_t highlight=false;
  
  /* year */
  LCM_DispStr8_16(1,0,"   20",false);
  snprintf(disp_buf,sizeof(disp_buf),"%02u",setDate.Year);
  highlight=(menu->Screen.selection==0);
  LCM_DispStr8_16(1,5,disp_buf,highlight);
  LCM_DispStr8_16(1,7,"-",false);
  /* month */
  snprintf(disp_buf,sizeof(disp_buf),"%02u",setDate.Month);
  highlight=(menu->Screen.selection==1);
  LCM_DispStr8_16(1,8,disp_buf,highlight);
  LCM_DispStr8_16(1,10,"-",false);
  /* day */
  snprintf(disp_buf,sizeof(disp_buf),"%02u",setDate.Date);
  highlight=(menu->Screen.selection==2);
  LCM_DispStr8_16(1,11,disp_buf,highlight);
  
  /* hour */
  snprintf(disp_buf,sizeof(disp_buf),"%02u",setTime.Hours);
  highlight=(menu->Screen.selection==3);
  LCM_DispStr8_16(2,4,disp_buf,highlight);
  LCM_DispStr8_16(2,6,":",false);
  /* minute */
  snprintf(disp_buf,sizeof(disp_buf),"%02u",setTime.Minutes);
  highlight=(menu->Screen.selection==4);
  LCM_DispStr8_16(2,7,disp_buf,highlight);
  LCM_DispStr8_16(2,9,":",false);
  /* second */
  snprintf(disp_buf,sizeof(disp_buf),"%02u",setTime.Seconds);
  highlight=(menu->Screen.selection==5);
  LCM_DispStr8_16(2,10,disp_buf,highlight);
}

/*Ƶ��������*/
static void FrequencyScreen_ButtonHandler(void *Menu,int16_t button)
{
	DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
	uint8_t i=0;
  
  switch(button)
  {
  case BUTTON_ESCAPE:
    if(menu->prev)
    {
      CurrentDisplay=menu->prev;
      CurrentDisplay->Screen.screen_number=0;
      CurrentDisplay->Screen.selection=1;
      LCM_DispFill(0);  /* clear screen */
    }
    break;
  case BUTTON_ENTER:
		if(menu->Screen.selection == 0)
		{
			__HAL_USART_DISABLE_IT(&huart1,USART_IT_RXNE);     /*ʧ�ܽ����ж�*/
			for(i=0;i<50;i++)
			{
				HAL_UART_Transmit(&huart1, (uint8_t *)"*\r\n", strlen("*\r\n"), 0xFF);
			}
			osDelay(1000);
			//for(i=0xffff;i>0;i--);
			HAL_UART_Transmit(&huart1,(uint8_t *)"P3\r\n",strlen("P3\r\n"),0xFF);//�������Ƶ��1��1�����ݵ�����
			osDelay(1000);
			//for(i=0xffff;i>0;i--);
			__HAL_USART_ENABLE_IT(&huart1,USART_IT_RXNE);     /*ʹ�ܴ���1�����ж�*/
			HAL_UART_Transmit(&huart1,(uint8_t *)"Q\r\n",strlen("Q\r\n"),0xFF);//�������Ƶ��1��1�����ݵ�����
			osDelay(1000);
			/*���óɹ����ı�־*/
			if(P_Flag == true)
			{
				LCM_DispStr8_16(0,14,"OK",false);
				P_Flag = false;
			}
			else if(P_Flag == true)
			{
				LCM_DispStr8_16(0,14,"NO",false);
				P_Flag = false;
			}
		}
		else if(menu->Screen.selection == 1)
		{
			__HAL_USART_DISABLE_IT(&huart1,USART_IT_RXNE);     /*ʧ�ܽ����ж�*/
			for(i=0;i<50;i++)
			{
				HAL_UART_Transmit(&huart1, (uint8_t *)"*\r\n", strlen("*\r\n"), 0xFF);
			}
			osDelay(1000);
			//for(i=0xffff;i>0;i--);
			HAL_UART_Transmit(&huart1,(uint8_t *)"P2\r\n",strlen("P2\r\n"),0xFF);//�������Ƶ��1��1�����ݵ�����
			osDelay(1000);
			//for(i=0xffff;i>0;i--);
			__HAL_USART_ENABLE_IT(&huart1,USART_IT_RXNE);     /*ʹ�ܴ���1�����ж�*/
			HAL_UART_Transmit(&huart1,(uint8_t *)"Q\r\n",strlen("Q\r\n"),0xFF);//�������Ƶ��1��1�����ݵ�����
			osDelay(1000);
			/*���óɹ����ı�־*/
			if(P_Flag == true)
			{
				LCM_DispStr8_16(1,14,"OK",false);
				P_Flag = false;
			}
			else if(P_Flag == true)
			{
				LCM_DispStr8_16(1,14,"NO",false);
				P_Flag = false;
			}
		}
		else if(menu->Screen.selection == 2)
		{
			__HAL_USART_DISABLE_IT(&huart1,USART_IT_RXNE);     /*ʧ�ܽ����ж�*/
			for(i=0;i<50;i++)
			{
				HAL_UART_Transmit(&huart1, (uint8_t *)"*\r\n", strlen("*\r\n"), 0xFF);
			}
			osDelay(1000);
			//for(i=0xffff;i>0;i--);
			HAL_UART_Transmit(&huart1,(uint8_t *)"P1\r\n",strlen("P1\r\n"),0xFF);//�������Ƶ��1��1�����ݵ�����
			osDelay(1000);
			//for(i=0xffff;i>0;i--);
			__HAL_USART_ENABLE_IT(&huart1,USART_IT_RXNE);     /*ʹ�ܴ���1�����ж�*/
			HAL_UART_Transmit(&huart1,(uint8_t *)"Q\r\n",strlen("Q\r\n"),0xFF);//�������Ƶ��1��1�����ݵ�����
			osDelay(1000);
			/*���óɹ����ı�־*/
			if(P_Flag == true)
			{
				LCM_DispStr8_16(2,14,"OK",false);
				P_Flag = false;
			}
			else if(P_Flag == true)
			{
				LCM_DispStr8_16(2,14,"NO",false);
				P_Flag = false;
			}
		}
		else if(menu->Screen.selection == 3)
		{
			__HAL_USART_DISABLE_IT(&huart1,USART_IT_RXNE);     /*ʧ�ܽ����ж�*/
			for(i=0;i<50;i++)
			{
				HAL_UART_Transmit(&huart1, (uint8_t *)"*\r\n", strlen("*\r\n"), 0xFF);
			}
			osDelay(1000);
			//for(i=0xffff;i>0;i--);
			HAL_UART_Transmit(&huart1,(uint8_t *)"P21\r\n",strlen("P21\r\n"),0xFF);//�������Ƶ��1��1�����ݵ�����
			osDelay(1000);
			//for(i=0xffff;i>0;i--);
			__HAL_USART_ENABLE_IT(&huart1,USART_IT_RXNE);     /*ʹ�ܴ���1�����ж�*/
			HAL_UART_Transmit(&huart1,(uint8_t *)"Q\r\n",strlen("Q\r\n"),0xFF);//�������Ƶ��1��1�����ݵ�����
			osDelay(1000);
			/*���óɹ����ı�־*/
			if(P_Flag == true)
			{
				LCM_DispStr8_16(0,14,"OK",false);
				P_Flag = false;
			}
			else if(P_Flag == true)
			{
				LCM_DispStr8_16(0,14,"NO",false);
				P_Flag = false;
			}
		}
		else if(menu->Screen.selection == 4)
		{
			__HAL_USART_DISABLE_IT(&huart1,USART_IT_RXNE);     /*ʧ�ܽ����ж�*/
			for(i=0;i<50;i++)
			{
				HAL_UART_Transmit(&huart1, (uint8_t *)"*\r\n", strlen("*\r\n"), 0xFF);
			}
			osDelay(1000);
			//for(i=0xffff;i>0;i--);
			HAL_UART_Transmit(&huart1,(uint8_t *)"P20\r\n",strlen("P20\r\n"),0xFF);//�������Ƶ��1��1�����ݵ�����
			osDelay(1000);
			//for(i=0xffff;i>0;i--);
			__HAL_USART_ENABLE_IT(&huart1,USART_IT_RXNE);     /*ʹ�ܴ���1�����ж�*/
			HAL_UART_Transmit(&huart1,(uint8_t *)"Q\r\n",strlen("Q\r\n"),0xFF);//�������Ƶ��1��1�����ݵ�����
			osDelay(1000);
			
			/*���óɹ����ı�־*/
			if(P_Flag == true)
			{
				LCM_DispStr8_16(1,14,"OK",false);
				P_Flag = false;
			}
			else if(P_Flag == true)
			{
				LCM_DispStr8_16(1,14,"NO",false);
				P_Flag = false;
			}
		}
    break;
  case BUTTON_LEFT:
    menu->Screen.selection--;
    LCM_DispFill(0);  /* clear screen */
    break;
  case BUTTON_RIGHT:
    menu->Screen.selection++;
    LCM_DispFill(0);  /* clear screen */
    break;
  default:
    break;
  }
  
  if(menu->Screen.selection<0)
  {
    menu->Screen.selection = 4;
  }
  else if(menu->Screen.selection>4)
  {
    menu->Screen.selection = 0;
  }
}

/*Ƶ������ʾ*/
static void FrequencyScreen_Display(void *Menu)
{
	DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
	uint8_t highlight=false;
	
	if(menu->Screen.selection<3)
	{
		menu->Screen.screen_number=0;
	}
	else if(menu->Screen.selection>2)
	{
		menu->Screen.screen_number=1;
	}
	
	if(menu->Screen.screen_number==0)
	{
		/*һ����������*/  
		highlight=(menu->Screen.selection==0);
    LCM_DispCHSStr(0,0,6,HZ_YMSTSJ,highlight);
		
		/*һ���������*/  
		highlight=(menu->Screen.selection==1);
    LCM_DispCHSStr(1,0,6,HZ_YMLTSJ,highlight);
    
		
		/*һ��һ������*/  
		highlight=(menu->Screen.selection==2);
    LCM_DispCHSStr(2,0,6,HZ_YMYTSJ,highlight);
    
		
		
	}
	else if(menu->Screen.screen_number==1)
	{
		/*2��1������*/  
		highlight=(menu->Screen.selection==3);
    LCM_DispCHSStr(0,0,6,HZ_LMYTSJ,highlight);
   
		
		/*4��1������*/  
		highlight=(menu->Screen.selection==4);
    LCM_DispCHSStr(1,0,6,HZ_SMYTSJ,highlight);
    
	}
}

/*���˵�����*/
static void MainScreen_ButtonHandler(void *Menu,int16_t button)
{
	DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  
  switch(button)
  {
  case BUTTON_ESCAPE:
    break;
  case BUTTON_ENTER:   /* enter next menu */
    if(menu->next)
    {
      CurrentDisplay=menu->next;
      CurrentDisplay->Screen.screen_number=0;
      CurrentDisplay->Screen.selection=0;
      LCM_DispFill(0);  /* clear screen */
  
      /* get system time */
      get_sys_time(&setDate,&setTime);
    }
    break;
  case BUTTON_LEFT:
    menu->Screen.selection--;
    break;
  case BUTTON_RIGHT:
    menu->Screen.selection++;
    break;
  default:
    break;
  }
  
  
  if(menu->Screen.selection<0)
  {
    menu->Screen.selection=2;
  }
  else if(menu->Screen.selection>2)
  {
    menu->Screen.selection=0;
  }
}

/*���˵�����ʾ*/
static void MainScreen_Display(void *Menu)
{
	DisplayMenuTypeDef *menu=(DisplayMenuTypeDef *)Menu;
  uint8_t highlight=false;
  
  /* Date */
  snprintf((char *)disp_buf,sizeof(disp_buf),"   20%02u-%02u-%02u   ",
           Date.Year,Date.Month,Date.Date);
  LCM_DispStr8_16(0,0,(char *)disp_buf,false);
  /* Time */
  snprintf((char *)disp_buf,sizeof(disp_buf),"    %02u:%02u:%02u",
           Time.Hours,Time.Minutes,Time.Seconds);
  LCM_DispStr8_16(1,0,(char *)disp_buf,false);
	
	  /* ʱ�� */
  highlight=(menu->Screen.selection==2);
  LCM_DispCHSStr(2,3,2,HZ_ShiJian,highlight);
  
  /* ģʽ */
  highlight=(menu->Screen.selection==0);
  LCM_DispCHSStr(3,1,2,HZ_MoShi,highlight);
  
  /* Ƶ�� */
  highlight=(menu->Screen.selection==1);
  LCM_DispCHSStr(3,5,2,HZ_Pinlv,highlight);
  
  /* Determine the next Menu */
  switch(menu->Screen.selection)
  {
  case 0:   /* Data Menu */
    menu->next=&ModeMenu;
    break;
  case 1:   /* Communication Menu */
    menu->next=&FrequencyMenu;
    break;
  case 2:   /* Time Menu */
    menu->next=&TimeMenu;
    break;
  default:
    menu->next=&ModeMenu;
    break;
  }
}

/*�̵���ʱ*/
__STATIC_INLINE void short_delay(void)
{
  volatile uint32_t i=3000;
  
	for(i=0;i<3000;i++);
//  while(i--)
//  {
//    asm("nop");
//  }
}

/**
  * @brief  turn off display
  * @param  None
  * @retval None
  */
__STATIC_INLINE void turn_off_display(void)
{
  /* turn off display */
  disp_on=false;
  LCD_BL_OFF();  /* backlight off */
  LCM_DispFill(0);  /* clear screen */
  
  CurrentDisplay=&MainMenu;  /* display main menu */
  CurrentDisplay->Screen.screen_number=0;
  CurrentDisplay->Screen.selection=0;
}

/**
  * @brief  turn on display
  * @param  None
  * @retval None
  */
__STATIC_INLINE void turn_on_display(void)
{
  /* turn on display */
  disp_on=true;  /* display on */
  LCD_BL_ON();   /* backlight on */
  disp_on_count=MAX_DISPLAY_ON_TIME;
}
