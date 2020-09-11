#include	"fsl_pxp.h"
#include	"pxp_drv_em.h"
#include 	"Freertos.h"
#include 	"semphr.h"
#include 	"task.h"
/*=========================================================================================*/
extern void GUI_SemPostISR(SemaphoreHandle_t *hsem);
extern uint8_t GUI_SemWait(SemaphoreHandle_t *hsem,uint32_t  time);
extern SemaphoreHandle_t*	GUI_SemCreate(int init,int max);
/*=========================================================================================*/

void MEM_Flush(uint32_t addr,uint32_t size)
{
  if(addr>=0x81C00000 || addr < 0x81000000)
    return;
//  else
//  if(size%4!=0)
//    GUI_DEBUG("1");
  SCB_CleanDCache_by_Addr((uint32_t*)addr,size);
//  SCB_CleanDCache();
}

/*=========================================================================================*/

static volatile int PXP_Rdy=false;
//static GUI_SEM *sem_pxp=NULL;
static SemaphoreHandle_t *sem_pxp=NULL;
static int init=0;

void PXP_IRQHandler(void)
{
	uint32_t ulReturn;
	/* 进入临界段，临界段可以嵌套 */
	ulReturn = taskENTER_CRITICAL_FROM_ISR(); 

	PXP_ClearStatusFlags(PXP,kPXP_CompleteFlag);
	NVIC_ClearPendingIRQ(PXP_IRQn);

//	uint32_t size =PXP->OUT_PITCH*(PXP->OUT_LRC&0xFFFF);
//	SCB_InvalidateDCache_by_Addr((uint32_t *)PXP->OUT_BUF,size);

	PXP_Rdy=true;
	GUI_SemPostISR(sem_pxp);

	taskEXIT_CRITICAL_FROM_ISR( ulReturn ); 
}

void PXP_WaitRdy(void)
{
	while(PXP_Rdy==false)
	{
		GUI_SemWait(sem_pxp,200);
	}

}

void PXP_WaitDone(uint32_t *addr,uint32_t size)
{
	while(PXP_Rdy==false)
	{
		GUI_SemWait(sem_pxp,200);
	}
	PXP->CTRL_CLR =PXP_CTRL_ENABLE_MASK;

	SCB_InvalidateDCache_by_Addr(addr,size);

}

void PXP_Execu(void)
{
	PXP_Rdy =false;

	//PXP_Start(PXP);
	PXP->CTRL_SET =PXP_CTRL_ENABLE_MASK;
}

void PXP_DrvInit(void)
{
	if(init==0)
	{
		init=1;

		sem_pxp =GUI_SemCreate(0,1);
		PXP_Rdy =true;

		PXP_Init(PXP);
		PXP_Reset(PXP);
		PXP_SetProcessBlockSize(PXP,kPXP_BlockSize16);
		PXP_EnableOverWrittenAlpha(PXP,false);
		PXP_EnableAlphaSurfaceOverlayColorKey(PXP,false);
		PXP_EnableLcdHandShake(PXP,false);
		PXP_EnableContinousRun(PXP,false);
     
		PXP_EnableInterrupts(PXP,kPXP_CompleteInterruptEnable);
		NVIC_EnableIRQ(PXP_IRQn);
		NVIC_ClearPendingIRQ(PXP_IRQn);
		NVIC_SetPriority(PXP_IRQn, 6U);
		/* Disable AS. */
		PXP_SetAlphaSurfacePosition(PXP, 0, 0, 0, 0);
		PXP_SetOverwrittenAlphaValue(PXP,255);
		PXP_EnableOverWrittenAlpha(PXP,false);

		/* Disable CSC1, it is enabled by default. */
		PXP_EnableCsc1(PXP, false);
	}
}

/*=========================================================================================*/
