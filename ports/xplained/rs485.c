/**
 * @file
 * @brief RS-485 Interface
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2013
 * @copyright SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "board.h"
#include "usart.h"
#include "ioport.h"
#include "sysclk.h"
#include "bacnet/basic/sys/fifo.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstpdef.h"
#include "led.h"
/* me! */
#include "rs485.h"

#ifdef CONF_BOARD_ENABLE_RS485_XPLAINED
#define RS485_RE IOPORT_CREATE_PIN(PORTC, 1)
#define RS485_DE IOPORT_CREATE_PIN(PORTC, 0)
#define RS485_TXD IOPORT_CREATE_PIN(PORTC, 3)
#define RS485_RXD IOPORT_CREATE_PIN(PORTC, 2)
#define RS485_USART USARTC0
#define RS485_TXC_vect USARTC0_TXC_vect
#define RS485_RXC_vect USARTC0_RXC_vect
#else
#define RS485_RE IOPORT_CREATE_PIN(PORTE, 0)
#define RS485_DE IOPORT_CREATE_PIN(PORTE, 0)
#define RS485_TXD IOPORT_CREATE_PIN(PORTE, 3)
#define RS485_RXD IOPORT_CREATE_PIN(PORTE, 2)
#define RS485_USART USARTE0
#define RS485_TXC_vect USARTE0_TXC_vect
#define RS485_RXC_vect USARTE0_RXC_vect
#endif

/* buffer for storing received bytes - size must be power of two */
/* BACnet MAX_APDU for MS/TP is 480 bytes */
static uint8_t Receive_Queue_Data[512];
static FIFO_BUFFER Receive_Queue;
/* buffer for storing bytes to transmit - size must be power of two */
/* BACnet MAX_APDU for MS/TP is 480 bytes */
static uint8_t Transmit_Queue_Data[512];
static FIFO_BUFFER Transmit_Queue;
/* baud rate of the UART interface */
static uint32_t Baud_Rate;
/* timer for measuring line silence */
static struct mstimer Silence_Timer;
/* flag to track transmit status */
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
 *  enable the transmit-enable line on the RS-485 transceiver
 *
 * @param enable - true to enable RTS, false to disable RTS
 */
void rs485_rts_enable(bool enable)
{
    if (enable) {
        /* Turn Tx enable on */
        ioport_set_value(RS485_RE, 1);
        ioport_set_value(RS485_DE, 1);
        led_on(LED_RS485_TX);
        Transmitting = true;
    } else {
        /* Turn Tx enable off */
        ioport_set_value(RS485_RE, 0);
        ioport_set_value(RS485_DE, 0);
        led_off_delay(LED_RS485_TX, 10);
        Transmitting = false;
    }
}

/**
 *  Determine the status of the transmit-enable line on the RS-485 transceiver
 *
 * @return true if RTS is enabled, false if RTS is disabled
 */
bool rs485_rts_enabled(void)
{
    return Transmitting;
}

/**
 * Checks for data on the receive UART, and handles errors
 *
 * @param data register to store the byte, if available (can be NULL)
 *
 * @return true if a byte is available
 */
bool rs485_byte_available(uint8_t *data_register)
{
    bool data_available = false; /* return value */

    if (FIFO_Empty(&Receive_Queue)) {
        led_off_delay(LED_RS485_RX, 2);
    } else {
        led_on(LED_RS485_RX);
        if (data_register) {
            *data_register = FIFO_Get(&Receive_Queue);
            RS485_Receive_Bytes++;
        }
        data_available = true;
    }

    return data_available;
}

/**
 * returns an error indication if errors are enabled
 *
 * @return returns true if error is detected and errors are enabled
 */
bool rs485_receive_error(void)
{
    return false;
}

/**
 *  Transmit one or more bytes on RS-485. Can be called while transmitting to
 * add additional bytes to transmit queue.
 *
 * @param buffer - array of one or more bytes to transmit
 * @param nbytes - number of bytes to transmit
 */
void rs485_bytes_send(uint8_t *buffer, uint16_t nbytes)
{
    bool status = false;
    bool start_required = false;
    uint8_t ch = 0;

    if (buffer && (nbytes > 0)) {
        if (FIFO_Empty(&Transmit_Queue)) {
            start_required = true;
        }
        status = FIFO_Add(&Transmit_Queue, buffer, nbytes);
        if (start_required && status) {
            rs485_rts_enable(true);
            rs485_silence_reset();
            ch = FIFO_Get(&Transmit_Queue);
            usart_clear_tx_complete(&RS485_USART);
            usart_set_tx_interrupt_level(&RS485_USART, USART_INT_LVL_LO);
            usart_putchar(&RS485_USART, ch);
            RS485_Transmit_Bytes++;
        }
    }
}

/**
 * RS485 RX interrupt
 */
ISR(RS485_RXC_vect)
{
    unsigned char ch;

    ch = usart_getchar(&RS485_USART);
    FIFO_Put(&Receive_Queue, ch);
    usart_clear_rx_complete(&RS485_USART);
}

/**
 * RS485 TX interrupt
 */
ISR(RS485_TXC_vect)
{
    uint8_t ch;

    if (FIFO_Empty(&Transmit_Queue)) {
        /* end of packet */
        usart_set_tx_interrupt_level(&RS485_USART, USART_INT_LVL_OFF);
        rs485_rts_enable(false);
    } else {
        rs485_rts_enable(true);
        ch = FIFO_Get(&Transmit_Queue);
        usart_putchar(&RS485_USART, ch);
    }
}

/**
 * Return the RS-485 baud rate
 *
 * @return baud - RS-485 baud rate in bits per second (bps)
 */
uint32_t rs485_baud_rate(void)
{
    return Baud_Rate;
}

/**
 * @brief Set the baud in kili-baud
 * @param baud_k baud rate in approximate kilobaud
*/
bool rs485_kbaud_rate_set(uint8_t baud_k)
{
    uint32_t baud = 38400;

    if (baud_k == 255) {
        baud = 38400;
    } else if (baud_k >= 115) {
        baud = 115200;
    } else if (baud_k >= 76) {
        baud = 76800;
    } else if (baud_k >= 57) {
        baud = 57600;
    } else if (baud_k >= 38) {
        baud = 38400;
    } else if (baud_k >= 19) {
        baud = 19200;
    } else if (baud_k >= 9) {
        baud = 9600;
    }

    return rs485_baud_rate_set(baud);
}

/**
* Converts baud in bps to kili-baud
*
* @param baud - baud rate in bps
* @return: baud rate in approximate kilo-baud
*/
uint8_t rs485_kbaud_rate(void)
{
    return Baud_Rate/1000;
}

/**
 * Initialize the RS-485 baud rate
 *
 * @param baud - RS-485 baud rate in bits per second (bps)
 *
 * @return true if set and valid
 */
bool rs485_baud_rate_set(uint32_t baud)
{
    bool valid = true;
    unsigned long frequency;

    switch (baud) {
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 76800:
        case 115200:
            frequency = sysclk_get_peripheral_bus_hz(&RS485_USART);
            valid = usart_set_baudrate(&RS485_USART, baud, frequency);
            if (valid) {
                Baud_Rate = baud;
            }
            break;
        default:
            valid = false;
            break;
    }

    return valid;
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
 *  Initialize the RS-485 UART interface, receive interrupts enabled
 */
void rs485_init(void)
{
    usart_rs232_options_t option;

    /* initialize the Rx and Tx byte queues */
    FIFO_Init(&Receive_Queue, &Receive_Queue_Data[0],
        (unsigned)sizeof(Receive_Queue_Data));
    FIFO_Init(&Transmit_Queue, &Transmit_Queue_Data[0],
        (unsigned)sizeof(Transmit_Queue_Data));
    /* initialize the silence timer */
    rs485_silence_reset();
    /* configure the TX pin */
    ioport_configure_pin(RS485_TXD, IOPORT_DIR_OUTPUT | IOPORT_INIT_HIGH);
    /* configure the RX pin */
    ioport_configure_pin(RS485_RXD, IOPORT_DIR_INPUT);
    /* configure the RTS pins */
    ioport_configure_pin(RS485_RE, IOPORT_DIR_OUTPUT | IOPORT_INIT_LOW);
    ioport_configure_pin(RS485_DE, IOPORT_DIR_OUTPUT | IOPORT_INIT_LOW);
    option.baudrate = Baud_Rate;
    option.charlength = USART_CHSIZE_8BIT_gc;
    option.paritytype = USART_PMODE_DISABLED_gc;
    option.stopbits = false;
    usart_init_rs232(&RS485_USART, &option);
    usart_set_rx_interrupt_level(&RS485_USART, USART_INT_LVL_HI);
}
