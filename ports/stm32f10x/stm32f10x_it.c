/**
 ******************************************************************************
 * @file    I2C/EEPROM/stm32f10x_it.c
 * @author  MCD Application Team
 * @version V3.4.0
 * @date    10/15/2010
 * @brief   Main Interrupt Service Routines.
 *          This file provides template for all exceptions handler and
 *          peripherals interrupt service routine.
 ******************************************************************************
 * @copy
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
 * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"

/** @addtogroup STM32F10x_StdPeriph_Examples
 * @{
 */

/** @addtogroup I2C_EEPROM
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief  This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void)
{
}

#ifndef NDEBUG
// From Joseph Yiu, minor edits by FVH
// hard fault handler in C,
// with stack frame location as input parameter
// called from HardFault_Handler in file xxx.s
void hard_fault_handler_c(unsigned int *hardfault_args)
{
    unsigned int stacked_r0;
    unsigned int stacked_r1;
    unsigned int stacked_r2;
    unsigned int stacked_r3;
    unsigned int stacked_r12;
    unsigned int stacked_lr;
    unsigned int stacked_pc;
    unsigned int stacked_psr;

    stacked_r0 = ((unsigned long)hardfault_args[0]);
    stacked_r1 = ((unsigned long)hardfault_args[1]);
    stacked_r2 = ((unsigned long)hardfault_args[2]);
    stacked_r3 = ((unsigned long)hardfault_args[3]);

    stacked_r12 = ((unsigned long)hardfault_args[4]);
    stacked_lr = ((unsigned long)hardfault_args[5]);
    stacked_pc = ((unsigned long)hardfault_args[6]);
    stacked_psr = ((unsigned long)hardfault_args[7]);

#if 0
  printf ("\n\n[Hard fault handler - all numbers in hex]\n");
  printf ("R0 = %x\n", stacked_r0);
  printf ("R1 = %x\n", stacked_r1);
  printf ("R2 = %x\n", stacked_r2);
  printf ("R3 = %x\n", stacked_r3);
  printf ("R12 = %x\n", stacked_r12);
  printf ("LR [R14] = %x  subroutine call return address\n", stacked_lr);
  printf ("PC [R15] = %x  program counter\n", stacked_pc);
  printf ("PSR = %x\n", stacked_psr);
  printf ("BFAR = %x\n", (*((volatile unsigned long *)(0xE000ED38))));
  printf ("CFSR = %x\n", (*((volatile unsigned long *)(0xE000ED28))));
  printf ("HFSR = %x\n", (*((volatile unsigned long *)(0xE000ED2C))));
  printf ("DFSR = %x\n", (*((volatile unsigned long *)(0xE000ED30))));
  printf ("AFSR = %x\n", (*((volatile unsigned long *)(0xE000ED3C))));
  printf ("SCB_SHCSR = %x\n", SCB->SHCSR);
#else
    (void)stacked_r0;
    (void)stacked_r1;
    (void)stacked_r2;
    (void)stacked_r3;

    (void)stacked_r12;
    (void)stacked_lr;
    (void)stacked_pc;
    (void)stacked_psr;
#endif

    while (1)
        ;
}
#endif

/**
 * @brief  This function handles Hard Fault exception.
 * @param  None
 * @retval None
 */
void HardFault_Handler(void)
{
#ifndef NDEBUG
    __ASM("TST LR, #4");
    __ASM("ITE EQ \n"
          "MRSEQ R0, MSP \n"
          "MRSNE R0, PSP");
    __ASM("B hard_fault_handler_c");
#endif
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1) { }
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1) { }
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1) { }
}

/**
 * @brief  This function handles SVCall exception.
 * @param  None
 * @retval None
 */
void SVC_Handler(void)
{
}

/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void)
{
}

/**
 * @brief  This function handles PendSV_Handler exception.
 * @param  None
 * @retval None
 */
void PendSV_Handler(void)
{
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

void EXTI15_10_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void EXTI2_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void ETH_WKUP_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void WWDG_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void PVD_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TAMPER_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void RTC_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void FLASH_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void RCC_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void EXTI0_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void EXTI1_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void EXTI3_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void EXTI4_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void DMA1_Channel1_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void DMA1_Channel2_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void DMA1_Channel3_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void DMA1_Channel4_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void DMA1_Channel5_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

#if 0
/* used by i2c driver */
void DMA1_Channel6_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}
#endif

#if 0
/* used by i2c driver */
void DMA1_Channel7_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}
#endif

void ADC1_2_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void USB_HP_CAN_TX_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void USB_LP_CAN_RX0_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void CAN_RX1_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void CAN_SCE_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void EXTI9_5_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM1_BRK_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM1_UP_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM1_TRG_COM_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM1_CC_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM2_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM3_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM4_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void I2C1_EV_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void I2C1_ER_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void I2C2_EV_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void I2C2_ER_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void SPI1_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void SPI2_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void USART1_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

#if 0
/* used directly in irb.c module */
void USART2_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}
#endif

void USART3_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void RTCAlarm_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void USBWakeUp_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM8_BRK_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM8_UP_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM8_TRG_COM_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM8_CC_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void ADC3_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void FSMC_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void SDIO_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM5_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void SPI3_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void UART4_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void UART5_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM6_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM7_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void DMA2_Channel1_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void DMA2_Channel2_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void DMA2_Channel3_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void DMA2_Channel4_5_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void USB_HP_CAN1_TX_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void CAN1_RX1_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void CAN1_SCE_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM1_BRK_TIM9_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM1_UP_TIM10_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM8_BRK_TIM12_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM8_UP_TIM13_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

void TIM8_TRG_COM_TIM14_IRQHandler(void)
{
#ifndef NDEBUG
    /* Go to infinite loop when interrupt occurs */
    while (1) {
        /* do nothing */
    }
#endif
}

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
