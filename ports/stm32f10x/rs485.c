/**************************************************************************
 *
 * Copyright (C) 2011 Steve Karg <skarg@users.sourceforge.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Module Description:
 * Handle the configuration and operation of the RS485 bus.
 **************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/basic/sys/fifo.h"
/* port specific */
#include "hardware.h"
#include "led.h"
#include "rs485.h"

/* buffer for storing received bytes - size must be power of two */
static uint8_t Receive_Buffer_Data[NEXT_POWER_OF_2(DLMSTP_MPDU_MAX)];
static FIFO_BUFFER Receive_Buffer;
/* amount of silence on the wire */
static struct mstimer Silence_Timer;
/* baud rate */
static uint32_t Baud_Rate = 38400;
/* flag to track RTS status */
static volatile bool Transmitting;

/* statistics */
static volatile uint32_t RS485_Transmit_Bytes;
static volatile uint32_t RS485_Receive_Bytes;


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
void USART2_IRQHandler(void)
{
    uint8_t data_byte;

    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        /* Read one byte from the receive data register */
        data_byte = USART_ReceiveData(USART2);
        (void)FIFO_Put(&Receive_Buffer, data_byte);
        RS485_Receive_Bytes++;
    }
    if (USART_GetFlagStatus(USART2, USART_FLAG_ORE) != RESET) {
        /* note: enabling RXNE interrupt also enables the ORE interrupt! */
        /* dummy read to clear error state */
        data_byte = USART_ReceiveData(USART2);
        USART_ClearFlag(USART2, USART_FLAG_ORE);
    }
}

/**
 * @brief Control the DE and /RE pins on the RS-485 transceiver
 * @param enable - true to set DE and /RE high, false to set /DE and RE low
 */
void rs485_rts_enable(bool enable)
{
    if (enable) {
        led_tx_on_interval(10);
        GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_SET);
    } else {
        GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_RESET);
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

    if (!FIFO_Empty(&Receive_Buffer)) {
        if (data_register) {
            *data_register = FIFO_Get(&Receive_Buffer);
        }
        rs485_silence_reset();
        data_available = true;
        led_rx_on_interval(10);
    }

    return data_available;
}

/**
 * @brief Determines if a byte in the USART has been shifted from register
 * @return true if the USART register is empty
 */
bool rs485_byte_sent(void)
{
    return USART_GetFlagStatus(USART2, USART_FLAG_TXE);
}

/**
 * @brief Determines if the entire frame is sent from USART FIFO
 * @brief true if the USART FIFO is empty
 */
bool rs485_frame_sent(void)
{
    return USART_GetFlagStatus(USART2, USART_FLAG_TC);
}

/**
 * @brief Transmit one or more bytes on RS-485.
 * @param buffer - array of one or more bytes to transmit
 * @param nbytes - number of bytes to transmit
 * @return true if added to queue
 */
void rs485_bytes_send(uint8_t *buffer, /* data to send */
    uint16_t nbytes)
{ /* number of bytes of data */
    uint8_t tx_byte;

    while (nbytes) {
        rs485_rts_enable(true);
        /* Send the data byte */
        tx_byte = *buffer;
        /* Send one byte */
        USART_SendData(USART2, tx_byte);
        while (!rs485_byte_sent()) {
            /* do nothing - wait until Tx buffer is empty */
        }
        buffer++;
        nbytes--;
        RS485_Transmit_Bytes++;
    }
    /* was the frame sent? */
    while (!rs485_frame_sent()) {
        /* do nothing - wait until the entire frame in the
           Transmit Shift Register has been shifted out */
    }
    rs485_rts_enable(false);
    rs485_silence_reset();

    return;
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
    USART_Init(USART2, &USART_InitStructure);
}

/**
 * @brief Sets the baud rate to non-volatile storeage and configures USART
 * @return true if a value baud rate was saved
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
 * @brief Initialize the room network USART
 */
void rs485_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    GPIO_StructInit(&GPIO_InitStructure);
    /* Configure USARTx Rx as input floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    /* Configure USARTx Tx as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    /* Configure the Request To Send (RTS) aka Transmit Enable pin */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    /* Enable USARTx Clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    /*RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);*/
    /* Enable GPIO Clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    /* Enable the USART Pins Software Remapping for this pair
    of pins and peripheral functions:
    USART3 Full remap (TX/PD8, RX/PD9, CK/PD10, CTS/PD11, RTS/PD12) */
    /*GPIO_PinRemapConfig(GPIO_FullRemap_USART3, ENABLE);*/
    /* Configure the NVIC Preemption Priority Bits */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
    /* Enable the USARTx Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    /* enable the USART to generate interrupts */
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    rs485_baud_rate_set(Baud_Rate);

    USART_Cmd(USART2, ENABLE);

    FIFO_Init(&Receive_Buffer, &Receive_Buffer_Data[0],
        (unsigned)sizeof(Receive_Buffer_Data));
    rs485_silence_reset();
}
