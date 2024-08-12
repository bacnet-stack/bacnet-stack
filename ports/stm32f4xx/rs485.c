/**
 * @file
 * @brief RS-485 Interface
 *
 * Copyright (C) 2020 Steve Karg <skarg@users.sourceforge.net>
 *
 * @page License
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/fifo.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstpdef.h"
/* port specific */
#include "stm32f4xx.h"
#include "rs485.h"

#ifndef RS485_LINKSPRITE_ENABLED
    #ifndef RS485_DFR0259_ENABLED
        #define RS485_DFR0259_ENABLED 1
    #endif
#endif

#if defined(RS485_DFR0259_ENABLED) || defined(RS485_LINKSPRITE_ENABLED)
    /* DFR0259 RS485 Shield - TXD=PG14, RXD=PG9, USART6 */
    #define RS485_USARTx      USART6
    #define RS485_NVIC_IRQ    USART6_IRQn
    #define RS485_USARTx_ISR  USART6_IRQHandler
    #define RS485_USARTx_RCC  RCC_APB2Periph_USART6
    #define RS485_GPIO_RCC    RCC_AHB1Periph_GPIOG
    #define RS485_GPIO_PINS   GPIO_Pin_9 | GPIO_Pin_14
    #define RS485_GPIO        GPIOG
    /* alternate function (AF) */
    #define RS485_AF_PINSOURCE_RX GPIO_PinSource9
    #define RS485_AF_PINSOURCE_TX GPIO_PinSource14
    #define RS485_AF_FUNCTION     GPIO_AF_USART6
#endif
#if defined(RS485_DFR0259_ENABLED)
    /* DFR0259 RS485 Shield - CE=PF15 */
    #define RS485_RTS_RCC     RCC_AHB1Periph_GPIOF
    #define RS485_RTS_PIN     GPIO_Pin_15
    #define RS485_RTS_GPIO    GPIOF
#endif
#if defined(RS485_LINKSPRITE_ENABLED)
    /* LINKSPRITE RS485 Shield - CE=PD15 */
    #define RS485_RTS_RCC     RCC_AHB1Periph_GPIOD
    #define RS485_RTS_PIN     GPIO_Pin_15
    #define RS485_RTS_GPIO    GPIOD
#endif

/* buffer for storing received bytes - size must be power of two */
/* BACnet DLMSTP_MPDU_MAX for MS/TP is 1501 bytes */
static uint8_t Receive_Queue_Data[NEXT_POWER_OF_2(DLMSTP_MPDU_MAX)];
static FIFO_BUFFER Receive_Queue;

/* buffer for storing bytes to transmit */
/* BACnet DLMSTP_MPDU_MAX for MS/TP is 1501 bytes */
static uint8_t Transmit_Queue_Data[NEXT_POWER_OF_2(DLMSTP_MPDU_MAX)];
static FIFO_BUFFER Transmit_Queue;

/* baud rate of the UART interface */
static uint32_t Baud_Rate = 38400;
/* flag to track RTS status */
static volatile bool Transmitting;

/* statistics */
static volatile uint32_t RS485_Transmit_Bytes;
static volatile uint32_t RS485_Receive_Bytes;

/* amount of silence on the wire */
static struct mstimer Silence_Timer;

/**
 * @brief Reset the silence on the wire timer.
 */
void rs485_silence_reset(void)
{
    mstimer_set(&Silence_Timer, 0);
}

/**
 * @brief Return the RS-485 silence time in milliseconds
 * @return silence time in milliseconds
 */
uint32_t rs485_silence_milliseconds(void)
{
    return mstimer_elapsed(&Silence_Timer);
}

/**
 * @brief Determines if an error occured while receiving
 * @return true an error occurred
 */
bool rs485_receive_error(void)
{
    return false;
}

/**
 * @brief USARTx interrupt handler sub-routine
 */
void RS485_USARTx_ISR(void)
{
    uint8_t data_byte;

    if (USART_GetITStatus(RS485_USARTx, USART_IT_RXNE) != RESET) {
        /* Read one byte from the receive data register */
        data_byte = USART_ReceiveData(RS485_USARTx);
        if (!Transmitting) {
            FIFO_Put(&Receive_Queue, data_byte);
            RS485_Receive_Bytes++;
        }
        USART_ClearITPendingBit(RS485_USARTx, USART_IT_RXNE);
    }
    if (USART_GetITStatus(RS485_USARTx, USART_IT_TXE) != RESET) {
        if (FIFO_Count(&Transmit_Queue)) {
            USART_SendData(RS485_USARTx, FIFO_Get(&Transmit_Queue));
            RS485_Transmit_Bytes += 1;
            rs485_silence_reset();
        } else {
            /* disable the USART to generate interrupts on TX empty */
            USART_ITConfig(RS485_USARTx, USART_IT_TXE, DISABLE);
            /* enable the USART to generate interrupts on TX complete */
            USART_ITConfig(RS485_USARTx, USART_IT_TC, ENABLE);
        }
        USART_ClearITPendingBit(RS485_USARTx, USART_IT_TXE);
    }
    if (USART_GetITStatus(RS485_USARTx, USART_IT_TC) != RESET) {
        rs485_rts_enable(false);
        /* disable the USART to generate interrupts on TX complete */
        USART_ITConfig(RS485_USARTx, USART_IT_TC, DISABLE);
        /* enable the USART to generate interrupts on RX not empty */
        USART_ITConfig(RS485_USARTx, USART_IT_RXNE, ENABLE);
        USART_ClearITPendingBit(RS485_USARTx, USART_IT_TC);
    }
    /* check for errors and clear them */
    if (USART_GetFlagStatus(RS485_USARTx, USART_FLAG_ORE) == SET) {
        /* note: enabling RXNE interrupt also enables the ORE interrupt! */
        /* dummy read to clear error state */
        data_byte = USART_ReceiveData(RS485_USARTx);
        USART_ClearFlag(RS485_USARTx, USART_FLAG_ORE);
    }
    if (USART_GetFlagStatus(RS485_USARTx, USART_FLAG_NE) == SET) {
        USART_ClearFlag(RS485_USARTx, USART_FLAG_NE);
    }
    if (USART_GetFlagStatus(RS485_USARTx, USART_FLAG_FE) == SET) {
        USART_ClearFlag(RS485_USARTx, USART_FLAG_FE);
    }
    if (USART_GetFlagStatus(RS485_USARTx, USART_FLAG_PE) == SET) {
        USART_ClearFlag(RS485_USARTx, USART_FLAG_PE);
    }
}

/**
 * @brief Control the DE and /RE pins on the RS-485 transceiver
 * @param enable - true to set DE and /RE high, false to set /DE and RE low
 */
void rs485_rts_enable(bool enable)
{
    Transmitting = enable;
    if (Transmitting) {
        GPIO_WriteBit(RS485_RTS_GPIO, RS485_RTS_PIN, Bit_SET);
    } else {
        GPIO_WriteBit(RS485_RTS_GPIO, RS485_RTS_PIN, Bit_RESET);
    }
}

/**
 * @brief Determine the status of the transmit-enable line on the RS-485
 *  transceiver
 * @return true if RTS is enabled, false if RTS is disabled
 */
bool rs485_rts_enabled(void)
{
    return Transmitting;
}

/**
 * @brief Return true if a byte is available
 * @param data_register - byte in this parameter if there is one available
 * @return true if a byte is available, with the byte in the parameter
 */
bool rs485_byte_available(uint8_t *data_register)
{
    bool data_available = false; /* return value */

    if (!FIFO_Empty(&Receive_Queue)) {
        if (data_register) {
            *data_register = FIFO_Get(&Receive_Queue);
        }
        rs485_silence_reset();
        data_available = true;
    }

    return data_available;
}

/**
 * @brief Transmit one or more bytes on RS-485.
 * @param buffer - array of one or more bytes to transmit
 * @param nbytes - number of bytes to transmit
 * @return true if added to queue
 */
void rs485_bytes_send(uint8_t *buffer, uint16_t nbytes)
{
    if (buffer && (nbytes > 0)) {
        if (FIFO_Add(&Transmit_Queue, buffer, nbytes)) {
            rs485_silence_reset();
            rs485_rts_enable(true);
            /* disable the USART to generate interrupts on RX not empty */
            USART_ITConfig(RS485_USARTx, USART_IT_RXNE, DISABLE);
            /* enable the USART to generate interrupts on TX empty */
            USART_ITConfig(RS485_USARTx, USART_IT_TXE, ENABLE);
            /* TXE interrupt will load the first byte */
        }
    }
}

/**
 * @brief Configures the baud rate of the USART
 */
static void rs485_baud_rate_configure(void)
{
    USART_InitTypeDef USART_InitStructure;

    USART_InitStructure.USART_BaudRate = Baud_Rate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl =
        USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    /* Configure USARTx */
    USART_Init(RS485_USARTx, &USART_InitStructure);
}

/**
 * @brief Initialize the RS-485 baud rate
 * @param baudrate - RS-485 baud rate in bits per second (bps)
 * @return true if set and valid
 */
bool rs485_baud_rate_set(uint32_t baud)
{
    bool valid = true;

    switch (baud) {
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 76800:
        case 115200:
            Baud_Rate = baud;
            rs485_baud_rate_configure();
            break;
        default:
            valid = false;
            break;
    }

    return valid;
}

/**
 * @brief Return the RS-485 baud rate
 * @return baud - RS-485 baud rate in bits per second (bps)
 */
uint32_t rs485_baud_rate(void)
{
    return Baud_Rate;
}

/**
 * @brief Return the RS-485 statistics for transmit bytes
 * @return number of bytes transmitted
 */
uint32_t rs485_bytes_transmitted(void)
{
    return RS485_Transmit_Bytes;
}

/**
 * @brief Return the RS-485 statistics for receive bytes
 * @return number of bytes received
 */
uint32_t rs485_bytes_received(void)
{
    return RS485_Receive_Bytes;
}

/**
 * @brief Initialize the USART for RS485
 */
void rs485_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* initialize the Rx and Tx byte queues */
    FIFO_Init(&Receive_Queue, &Receive_Queue_Data[0],
        (unsigned)sizeof(Receive_Queue_Data));
    FIFO_Init(&Transmit_Queue, &Transmit_Queue_Data[0],
        (unsigned)sizeof(Transmit_Queue_Data));

    /* Enable GPIOx clock */
    RCC_AHB1PeriphClockCmd(RS485_GPIO_RCC, ENABLE);
    /* Enable USARTx Clock */
    RCC_APB2PeriphClockCmd(RS485_USARTx_RCC, ENABLE);

    /* Configure USARTx Rx and Tx pins for Alternate Function (AF) */
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = RS485_GPIO_PINS;
    /* GPIO_Speed_2MHz/GPIO_Speed_25MHz/GPIO_Speed_50MHz/GPIO_Speed_100MHz */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    /* GPIO_Mode_IN/GPIO_Mode_OUT/GPIO_Mode_AF/GPIO_Mode_AN */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    /* GPIO_OType_PP/GPIO_OType_OD */
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    /* GPIO_PuPd_NOPULL, GPIO_PuPd_UP, GPIO_PuPd_DOWN */
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(RS485_GPIO, &GPIO_InitStructure);
    /* alternate function (AF) */
    GPIO_PinAFConfig(RS485_GPIO, RS485_AF_PINSOURCE_RX, RS485_AF_FUNCTION);
    GPIO_PinAFConfig(RS485_GPIO, RS485_AF_PINSOURCE_TX, RS485_AF_FUNCTION);

    /* DFR0259 RS485 Shield - CE=PF15 */
    /* Enable GPIOx clock */
    RCC_AHB1PeriphClockCmd(RS485_RTS_RCC, ENABLE);
    /* Configure the Request To Send (RTS) aka Transmit Enable pin */
    GPIO_InitStructure.GPIO_Pin = RS485_RTS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(RS485_RTS_GPIO, &GPIO_InitStructure);

    /* Configure the NVIC Preemption Priority Bits */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
    /* Enable the USARTx Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = RS485_NVIC_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    /* enable the USART to generate interrupts on RX */
    USART_ITConfig(RS485_USARTx, USART_IT_RXNE, ENABLE);

    rs485_baud_rate_set(Baud_Rate);

    USART_Cmd(RS485_USARTx, ENABLE);

    rs485_silence_reset();
}
