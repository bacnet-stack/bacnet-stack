/**************************************************************************
 *
 * Copyright (C) 2025 Testimony Adams <adamstestimony@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/basic/sys/fifo.h"
#include "rs485.h"
#include "mstimer_init.h" 
#include <stdio.h>

// --- Static State Variables ---
static bool Rs485_RTS_Enabled = false;
static uint32_t Rs485_Baud_Rate = RS485_BAUD_RATE;
static volatile uint32_t Rs485_Bytes_Tx = 0;
static volatile uint32_t Rs485_Bytes_Rx = 0;

/* amount of silence on the wire */
static struct mstimer Silence_Timer;

/**
 * @brief Initialize the RS-485 UART and GPIO pins.
 */
void rs485_init(void)
{
    // 1. Initialize GPIO pins
    gpio_set_function(RS485_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RS485_RX_PIN, GPIO_FUNC_UART);

    // Initialize DE/RE pin (RTS) as an output
    gpio_init(RS485_DE_PIN);
    gpio_set_dir(RS485_DE_PIN, GPIO_OUT);
    rs485_rts_enable(false); // Set to receive (DE/RE low)

    // 2. Initialize UART
    uart_init(RS485_UART_ID, Rs485_Baud_Rate);
    uart_set_format(RS485_UART_ID, DATA_BIT, STOP_BIT, UART_PARITY_NONE);
    uart_set_hw_flow(RS485_UART_ID, false, false);
    uart_set_fifo_enabled(RS485_UART_ID, false);
    
    // 3. Reset silence timer
    rs485_silence_reset();
    
    // 4. Flush any junk data in the RX buffer from startup
    while (uart_is_readable(RS485_UART_ID)) {
        uart_getc(RS485_UART_ID);
    }

}

/**
 * @brief Enables or disables the Request To Send (RTS) / Driver Enable (DE/RE) pin.
 * @param enable True to enable transmitter (TX), False to enable receiver (RX).
 */
void rs485_rts_enable(bool enable)
{
    // DE=1: Driver Enable (TX mode)
    // DE=0: Receiver Enable (RX mode)
    gpio_put(RS485_DE_PIN, enable ? 1 : 0);
    Rs485_RTS_Enabled = enable;
}

/**
 * @brief Returns the current state of the RTS/DE/RE pin.
 * @return True if transmitter is enabled, False if receiver is enabled.
 */
bool rs485_rts_enabled(void)
{
    return Rs485_RTS_Enabled;
}

/**
 * @brief Attempts to read a single byte from the UART.
 * @param data_register Pointer to store the received byte.
 * @return True if a byte was received, False otherwise.
 */
bool rs485_byte_available(uint8_t *data_register)
{
    if (!uart_is_readable(RS485_UART_ID)) {
        return false;
    }
    if (!data_register) {
        // only checking availability — DO NOT CONSUME
        // Edit to consume if needed
        return true;
    }

    *data_register = uart_getc(RS485_UART_ID);
    Rs485_Bytes_Rx++;
    rs485_silence_reset();
    return true;
}


/**
 * @brief Checks the UART hardware for a receive error (Framing, Parity, Overrun).
 * @return True if a receive error is present, False otherwise.
 */
bool rs485_receive_error(void)
{
    // The RSR (Receive Status Register) holds error flags.
    // It is part of the raw hardware registers, accessed via uart_get_hw().
    uart_hw_t *uart_hw = uart_get_hw(RS485_UART_ID);
    
    // RSR bits: 0: FE (Framing Error), 1: PE (Parity Error), 2: BE (Break Error), 3: OE (Overrun Error)
    uint32_t rsr = uart_hw->rsr;
    
    // Clearing the RSR register is crucial after reading errors.
    // Writing anything to it clears all error flags.
    uart_hw->rsr = 0; 
    
    // Check if any error bit is set
    return (rsr != 0);
}

/**
 * @brief Sends a buffer of bytes over the UART.
 * @param buffer Pointer to the data buffer.
 * @param nbytes Number of bytes to send.
 */
void rs485_bytes_send(const uint8_t *buffer, uint16_t nbytes)
{
    // The BACnet stack will call rs485_rts_enable(true) before this function,
    // and wait T_prop before sending.
    // printf("TX %u bytes: ", nbytes);
    // for (uint16_t i = 0; i < nbytes; i++) {
    //     printf("%02X ", buffer[i]);
    // }
    // printf("\r\n");
    rs485_rts_enable(true);
    // TO BE IMPROVED (Implement custom_sleep)
    sleep_us(200);
    uart_write_blocking(RS485_UART_ID, buffer, nbytes);
    uart_tx_wait_blocking(RS485_UART_ID);
    sleep_us(200);
    
    // Update count and silence timer.
    Rs485_Bytes_Tx += nbytes;
    rs485_rts_enable(false);
    rs485_silence_reset();
    
    // The BACnet stack handles the post-transmit delay (T_prop + T_frame_gap) 
    // and calls rs485_rts_enable(false) to turn off the driver.
}

/**
 * @brief Returns the currently configured baud rate.
 * @return The current baud rate.
 */
uint32_t rs485_baud_rate(void)
{
    return Rs485_Baud_Rate;
}

/**
 * @brief Sets a new baud rate for the UART.
 * @param baud The new baud rate to set.
 * @return True if the baud rate was successfully set, False otherwise.
 */
bool rs485_baud_rate_set(uint32_t baud)
{
    if (baud == 0) {
        return false;
    }
    
    // uart_set_baudrate returns the actual configured rate, which may be slightly off.
    // We only care that the function executed successfully and we store the requested rate.
    switch (baud) {
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 76800:
        case 115200:
            Rs485_Baud_Rate = baud;
            uart_set_baudrate(RS485_UART_ID, Rs485_Baud_Rate);
            break;
        default:
            return false;
            break;
    }
    
    return true;
}

/**
 * @brief Measures the duration of silence on the bus since the last byte (Tx or Rx).
 * @return The duration of silence in milliseconds.
 */
uint32_t rs485_silence_milliseconds(void)
{
    return mstimer_elapsed(&Silence_Timer);
}

/**
 * @brief Resets the silence timer to the current time.
 */
void rs485_silence_reset(void)
{
    mstimer_set(&Silence_Timer, 0);
}

/**
 * @brief Gets the total number of bytes transmitted.
 * @return The byte count.
 */
uint32_t rs485_bytes_transmitted(void)
{
    return Rs485_Bytes_Tx;
}

/**
 * @brief Gets the total number of bytes received.
 * @return The byte count.
 */
uint32_t rs485_bytes_received(void)
{
    return Rs485_Bytes_Rx;
}