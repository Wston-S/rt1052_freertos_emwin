/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "GUI.h"
#include "WM.h"
#include "GUIDRV_Lin.h" // TODO - choose appropriate LCD driver here - see emWin documentation
#include "emwin_support.h"

#include "fsl_debug_console.h"
#include "fsl_gpio.h"

/**kuver include **/
#include "./lcd/bsp_lcd.h" 
#include "./touch/bsp_touch_gtxx.h"
#include "fsl_debug_console.h"
/* FreeRTOS include files */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#define LCD_BITS_PER_PIXEL 16
#define LCD_BYTES_PER_PIXEL (LCD_BITS_PER_PIXEL / 8)
#define NUM_BUFFERS 2

uint32_t VRAM_SIZE=0;
static volatile int32_t s_LCDpendingBuffer = -1;


/*********************************************************************
*
* Global data
*/
static xSemaphoreHandle xQueueMutex;
static xSemaphoreHandle xSemaTxDone;

//�����Ƕ�̬�ڴ棬��ͬ���Դ�
#define GUI_MEMORY_ADDR (uint32_t)0x81400000   //�˶��ڴ�Ҫ����cache����Ȼˢ�º�������

#ifndef GUI_MEMORY_ADDR   
static uint32_t s_gui_memory[(GUI_NUMBYTES + 3) / 4]; /* needs to be word aligned */
#define GUI_MEMORY_ADDR ((uint32_t)s_gui_memory)
#endif

//kuver 
#define  GUI_BLOCKSIZE 			0x80
#define  VRAM_ADDR				((uint32_t)s_psBufferLcd)


/*******************************************************************************
 * Implementation of communication with the touch controller
 ******************************************************************************/
void LCDIF_IRQHandler(void)
{
    uint32_t intStatus;

    intStatus = ELCDIF_GetInterruptStatus(LCDIF);

    ELCDIF_ClearInterruptStatus(LCDIF, intStatus);

   if (intStatus & kELCDIF_CurFrameDone)
    {
		if (s_LCDpendingBuffer >= 0)
        {
            /* Send a confirmation that the given buffer is visible */
            GUI_MULTIBUF_Confirm(s_LCDpendingBuffer);
            s_LCDpendingBuffer = -1;
        }
	}
	/* ���²�����Ϊ ARM �Ŀ���838869��ӵ�, 
       �ô���Ӱ�� Cortex-M4, Cortex-M4F�ںˣ�       
       �����洢�����ص��쳣�����·��ز������ܻ�ָ�������ж�
        CM7����Ӱ�죬�˴������ô���
    */  
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}

/*******************************************************************************
 * Application implemented functions required by emWin library
 ******************************************************************************/

void LCD_X_Config(void)
{
	VRAM_SIZE=LCD_WIDTH*LCD_HEIGHT*LCD_BYTES_PER_PIXEL;
	
#if (NUM_BUFFERS > 1)
    GUI_MULTIBUF_Config(NUM_BUFFERS);  //˫����
#endif	
    
    //Initialize the gui device with specified display driver and color conversion
	GUI_DEVICE_CreateAndLink(GUIDRV_LIN_16, GUICC_M565, 0, 0);//��Ҫ����������Բο�UM03001_emWin�� LCD_X_Config()
    
    LCD_SetSizeEx(0, LCD_WIDTH,LCD_HEIGHT );
    LCD_SetVSizeEx(0, LCD_WIDTH, LCD_HEIGHT);		
	LCD_SetVRAMAddrEx(0, (void *)VRAM_ADDR);  //Set VRAM address
	
#if defined(PALETTE)
    LCD_SetLUTEx(0, PALETTE);
#endif
}

int LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void *p)
{
	uint32_t addr;
    int result = 0;
	LCD_X_SHOWBUFFER_INFO *pData;
    switch (Cmd)
    {
        case LCD_X_INITCONTROLLER:
		{
			Bsp_LCD_Init(LCD_INTERRUPT_ENABLE);   
			break;
		}    
		case LCD_X_SHOWBUFFER:
        {
            pData = (LCD_X_SHOWBUFFER_INFO *)p;
            /* Calculate address of the given buffer */
            addr = (uint32_t)VRAM_ADDR + VRAM_SIZE * pData->Index;
            /* Make the given buffer visible */
            ELCDIF_SetNextBufferAddr(LCDIF, addr);
            //
            // Remember buffer index to be used by ISR
            //
            s_LCDpendingBuffer = pData->Index;
            while (s_LCDpendingBuffer >= 0)
                ;
            return 0;
        }
        default:
            result = -1;
            break;
    }

    return result;
}

void GUI_X_Config(void)
{	
    /* Assign work memory area to emWin */
    GUI_ALLOC_AssignMemory((void *)GUI_MEMORY_ADDR, GUI_NUMBYTES);
	
	/*K*/
	GUI_ALLOC_SetAvBlockSize(GUI_BLOCKSIZE);

    /* Select default font */
   GUI_SetDefaultFont(GUI_FONT_8X16_ASCII);
	
}

void GUI_X_Init(void)
{
}

/* Dummy RTOS stub required by emWin */
void GUI_X_InitOS(void)
{
	/* Create Mutex lock */
	xQueueMutex = xSemaphoreCreateMutex();
	configASSERT (xQueueMutex != NULL);
  
	/* Queue Semaphore */ 
	vSemaphoreCreateBinary( xSemaTxDone );
	configASSERT ( xSemaTxDone != NULL );
}

/* Dummy RTOS stub required by emWin */
void GUI_X_Lock(void)
{
	if(xQueueMutex == NULL)
	{
		GUI_X_InitOS();
	}
  
	xSemaphoreTake( xQueueMutex, portMAX_DELAY );
}

/* Dummy RTOS stub required by emWin */
void GUI_X_Unlock(void)
{
	xSemaphoreGive( xQueueMutex ); 
}

/* Dummy RTOS stub required by emWin */
U32 GUI_X_GetTaskId(void)
{
	return ((U32) xTaskGetCurrentTaskHandle());
}

void GUI_X_ExecIdle(void)
{
}

GUI_TIMER_TIME GUI_X_GetTime(void)
{
	return ((int) xTaskGetTickCount());
}

void GUI_X_Delay(int Period)
{
	vTaskDelay(Period);
}

