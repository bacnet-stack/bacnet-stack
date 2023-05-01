/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2023
 * @brief RS-485 Interface
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <string.h>
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/fifo.h"
#include "bacnet/datalink/mstpdef.h"
#include "bacnet/datalink/dlmstp.h"
#include "rs485.h"

/* buffer for storing received bytes - size must be power of two */
static uint8_t Receive_Buffer_Data[NEXT_POWER_OF_2(MAX_MPDU)];
static FIFO_BUFFER Receive_Queue;
/* buffer for storing data to be transmitted - size must be power of two */
static uint8_t Transmit_Buffer_Data[NEXT_POWER_OF_2(MAX_MPDU)];
static FIFO_BUFFER Transmit_Queue;

/* baud rate of the UART interface */
static uint32_t RS485_Baud_Rate = 9600;
/* flag to track RTS status */
static volatile bool RS485_Transmitting;
/* silence timer */
static struct mstimer Silence_Timer;
/* statistics */
static volatile uint32_t RS485_Transmit_Bytes;
static volatile uint32_t RS485_Receive_Bytes;

/**
 * \brief SERCOM UART interrupt handler
 */
static void rs485_interrupt_handler(uint8_t dummy)
{
    bool register_empty_interrupt = true;
    bool transmit_complete_interrupt = false;
    bool receive_complete_interrupt = false;

    if (register_empty_interrupt) {
        /* transmitting, register is ready for more */
        if (FIFO_Count(&Transmit_Queue)) {
            if (RS485_Transmitting) {
                data = FIFO_Get(&Transmit_Queue);
                RS485_Transmit_Bytes++;
                rs485_silence_reset();
            }
            /* set the 'data' into the register */
            if (FIFO_Empty(&Transmit_Queue)) {
                /* Disable the Data Register Empty Interrupt */
                /* Enable Transmission Complete interrupt */
            }
        } else {
            /* Disable the Data Register Empty Interrupt */
            /* Enable Transmission Complete interrupt */
	    }
    }
    if (transmit_complete_interrupt) {
        /* register was already empty and is now complete */
        rs485_rts_enable(false);
    }
    if (receive_complete_interrupt) {
        /* get 'data' from the register */
        if (!RS485_Transmitting) {
            (void)FIFO_Put(&Receive_Queue, data);
        }
        /* get any `error_code` from UART register */
        if (error_code) {
            /* clear error codes */
        }
        rs485_silence_reset();
        RS485_Receive_Bytes++;
    }
}

/**
 * @brief Control the DE and /RE pins on the RS-485 transceiver
 * @param enable - true to set DE and /RE high, false to set /DE and RE low
 * @note  DE and /RE are controlled by pin is PA23
 */
void rs485_rts_enable(bool enable)
{
    if (enable) {
        /* DE, /RE */
        RS485_Transmitting = true;
    } else {
        /* /DE, RE */
        RS485_Transmitting = false;
        rs485_silence_reset();
    }
}

/**
 * @brief Determine the status of the transmit-enable line on the RS-485
 * transceiver
 * @return true if RTS is enabled, false if RTS is disabled
 */
bool rs485_rts_enabled(void)
{
    return RS485_Transmitting;
}

/**
 * @brief Checks for data on the receive UART, and handles errors
 * @param data register to store the byte, if available (can be NULL)
 * @return true if a byte is available
 */
bool rs485_byte_available(uint8_t *data_register)
{
    bool data_available = false; /* return value */

    if (!FIFO_Empty(&Receive_Queue)) {
        if (data_register) {
            *data_register = FIFO_Get(&Receive_Queue);
        }
        data_available = true;
    }

    return data_available;
}

/**
 * @brief returns an error indication if errors are enabled
 * @return returns true if error is detected and errors are enabled
 */
bool rs485_receive_error(void)
{
    return false;
}

/**
 * @brief Transmit one or more bytes on RS-485.
 * @param buffer - array of one or more bytes to transmit
 * @param nbytes - number of bytes to transmit
 */
bool rs485_bytes_send(uint8_t *buffer, uint16_t nbytes)
{
    if (buffer && nbytes) {
        FIFO_Flush(&Transmit_Queue);
        FIFO_Add(&Transmit_Queue, buffer, nbytes);
        rs485_rts_enable(true);
    }

    return true;
}

/**
 * Return the RS-485 baud rate
 *
 * @return baud - RS-485 baud rate in bits per second (bps)
 */
uint32_t rs485_baud_rate(void)
{
    return RS485_Baud_Rate;
}

/**
 * Initialize the RS-485 baud rate
 *
 * @param baudrate - RS-485 baud rate in bits per second (bps)
 *
 * @return true if set and valid
 */
bool rs485_baud_rate_set(uint32_t baudrate)
{
    bool valid = true;

    switch (baudrate) {
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 76800:
        case 115200:
            RS485_Baud_Rate = baudrate;
            break;
        default:
            valid = false;
            break;
    }

    return valid;
}

/**
 * @brief Return the RS-485 silence time in milliseconds
 * @return silence time in milliseconds
 */
uint32_t rs485_silence_milliseconds(
    void)
{
    return mstimer_elapsed(&Silence_Timer);
}

/**
 * @brief Reset the RS-485 silence time to zero
 */
void rs485_silence_reset(
    void)
{
    mstimer_restart(&Silence_Timer);
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
 * Initialize the USART SERCOM module clock
 */
static void rs485_clock_init(void)
{
}

/**
 * Initialize the RTS pin, configured to receive enable
 */
static void rs485_pin_init(void)
{
}

/**
 * Initialize the USART module for RS485
 */
static void rs485_usart_init(void)
{
}

/* SERCOM UART initialization */
void rs485_init(void)
{
    FIFO_Init(&Receive_Queue, &Receive_Buffer_Data[0],
        (unsigned)sizeof(Receive_Buffer_Data));
    FIFO_Init(&Transmit_Queue, &Transmit_Buffer_Data[0],
        (unsigned)sizeof(Transmit_Buffer_Data));
    rs485_silence_reset();
    rs485_clock_init();
    rs485_pin_init();
    rs485_usart_init();
}
