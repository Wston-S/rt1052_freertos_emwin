/**
******************************************************************
* @file    main.c
* @author  fire
* @version V1.0
* @date    2018-xx-xx
* @brief   ELCDIF—液晶显示（显示英文）
******************************************************************
* @attention
*
* 实验平台:野火  i.MXRT1052开发板 
* 论坛    :http://www.firebbs.cn
* 淘宝    :http://firestm32.taobao.com
*
******************************************************************
*/
#include <stdio.h>

#include "fsl_debug_console.h"
#include "fsl_elcdif.h"


#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "./systick/bsp_systick.h"

#include "./delay/core_delay.h"   
#include "./led/bsp_led.h" 
#include "./lcd/bsp_lcd.h" 
#include "./touch/bsp_touch_gtxx.h"
#include "./touch/bsp_i2c_touch.h"
#include "./touch/palette.h"

/* FreeRTOS头文件 */
#include "FreeRTOS.h"
#include "task.h"

/*******************************************************************
* Prototypes
*******************************************************************/

/*
**********************************************************************************************************
											函数声明
**********************************************************************************************************
*/
static void vTaskGUI(void *pvParameters);
static void vTaskTouch(void *pvParameters);
static void AppTaskCreate (void);
static void BSP_Init(void);/* 用于初始化板载相关资源 */


/*
**********************************************************************************************************
											变量声明
**********************************************************************************************************
*/
static TaskHandle_t xHandleTaskStart = NULL;



/*******************************************************************
* Code
*******************************************************************/


/**
* @brief  主函数
* @param  无
* @retval 无
*/
int main(void)
{
	BSP_Init();
  
	/* 创建任务 */
	AppTaskCreate();
  
	 /* 启动调度，开始执行任务 */
    vTaskStartScheduler();

	/* 
	  如果系统正常启动是不会运行到这里的，运行到这里极有可能是用于定时器任务或者空闲任务的
	  heap空间不足造成创建失败，此要加大FreeRTOSConfig.h文件中定义的heap大小：
	  #define configTOTAL_HEAP_SIZE	      ( ( size_t ) ( 17 * 1024 ) )
	*/
	while(1); 
}

				
/*
*********************************************************************************************************
*	函 数 名: AppTaskCreate
*	功能说明: 创建应用任务
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void AppTaskCreate (void)
{
	xTaskCreate(  vTaskGUI,             /* 任务函数  */
                  "vTaskGUI",           /* 任务名    */
                  1024,                 /* 任务栈大小，单位word，也就是4字节 */
                  NULL,                 /* 任务参数  */
                  1,                    /* 任务优先级,越小优先级越低,空闲任务的优先级是0*/
                  NULL );               /* 任务句柄  */
	
	xTaskCreate( vTaskTouch,     		/* 任务函数  */
                 "vTaskTouch",   		/* 任务名    */
                 512,            		/* 任务栈大小，单位word，也就是4字节 */
                 NULL,           		/* 任务参数  */
                 5,              		/* 任务优先级*/
                 &xHandleTaskStart );   /* 任务句柄  */
}
/*
*********************************************************************************************************
*	函 数 名: vTaskTouch
*	功能说明: 最高优先级任务。主要实现触摸检测。
*	形    参: pvParameters 是在创建该任务时传递的形参
*	返 回 值: 无
*   优 先 级: 5  
*********************************************************************************************************
*/
static void vTaskTouch(void *pvParameters)
{	
    while(1)
    {
		GTP_TouchProcess();
		vTaskDelay(20);
	}
}

/*
*********************************************************************************************************
*	函 数 名: vTaskGUI
*	功能说明: emWin任务
*	形    参: pvParameters 是在创建该任务时传递的形参
*	返 回 值: 无
*   优 先 级: 1   (数值越小优先级越低，这个跟uCOS相反)
*********************************************************************************************************
*/
static void vTaskGUI(void *pvParameters)
{
	while (1) 
	{
		MainTask();
	}
}

/***********************************************************************
  * @ 函数名  ： BSP_Init
  * @ 功能说明： 板级外设初始化，所有板子上的初始化均可放在这个函数里面
  * @ 参数    ：   
  * @ 返回值  ： 无
  *********************************************************************/
static void BSP_Init(void)
{
#if (!CPU_TS_INIT_IN_DELAY_FUNCTION)       
    //使用时间戳延时函数前必须先使能计数器 
    CPU_TS_TmrInit(); 
#endif 
	/* 初始化内存保护单元 */
	BOARD_ConfigMPU();
	/* 初始化开发板引脚 */
	BOARD_InitPins();
	/* 初始化开发板时钟 */
	BOARD_BootClockRUN();
	/* 初始化调试串口 */
	BOARD_InitDebugConsole();
	/* 打印系统时钟 */
	PRINTF("\r\n");
	PRINTF("CPU:             %d Hz\r\n", CLOCK_GetFreq(kCLOCK_CpuClk));
	PRINTF("AHB:             %d Hz\r\n", CLOCK_GetFreq(kCLOCK_AhbClk));
	PRINTF("SEMC:            %d Hz\r\n", CLOCK_GetFreq(kCLOCK_SemcClk));
	PRINTF("SYSPLL:          %d Hz\r\n", CLOCK_GetFreq(kCLOCK_SysPllClk));
	PRINTF("SYSPLLPFD0:      %d Hz\r\n", CLOCK_GetFreq(kCLOCK_SysPllPfd0Clk));
	PRINTF("SYSPLLPFD1:      %d Hz\r\n", CLOCK_GetFreq(kCLOCK_SysPllPfd1Clk));
	PRINTF("SYSPLLPFD2:      %d Hz\r\n", CLOCK_GetFreq(kCLOCK_SysPllPfd2Clk));
	PRINTF("SYSPLLPFD3:      %d Hz\r\n", CLOCK_GetFreq(kCLOCK_SysPllPfd3Clk));	
  
	/* 初始化LED */
	LED_GPIO_Config();
	/* 初始化systick计算帧率 */
	SysTick_Init();
  
	/* 触摸初始化 也可以在emwin配置文件中实现 */  
	GTP_Init_Panel();
}



/****************************END OF FILE**********************/



