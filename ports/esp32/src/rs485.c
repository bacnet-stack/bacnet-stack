/**
 * @file
 * @brief RS485 UART driver used by BACnet MS/TP on ESP32
 * @author Kato Gangstad
 */

#include "rs485.h"

#include <driver/gpio.h>
#include <driver/uart.h>

/* UART_SCLK_DEFAULT was introduced in ESP-IDF v5.0.
   Fall back to UART_SCLK_APB for older IDF/Arduino-ESP32 toolchains. */
#ifndef UART_SCLK_DEFAULT
#define UART_SCLK_DEFAULT UART_SCLK_APB
#endif

#include "bacnet/basic/sys/mstimer.h"

static bool Rs485_RTS_Enabled;
static uint32_t Rs485_Baud_Rate = RS485_BAUD_RATE;
static volatile uint32_t Rs485_Bytes_Tx;
static volatile uint32_t Rs485_Bytes_Rx;
static struct mstimer Silence_Timer;

/**
 * @brief Initialize the RS485 UART peripheral for BACnet MS/TP
 */
void rs485_init(void)
{
    uart_config_t cfg = { .baud_rate = (int)Rs485_Baud_Rate,
                          .data_bits = UART_DATA_8_BITS,
                          .parity = UART_PARITY_DISABLE,
                          .stop_bits = UART_STOP_BITS_1,
                          .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                          .source_clk = UART_SCLK_DEFAULT };

    (void)uart_driver_delete(RS485_UART_NUM);
    (void)uart_param_config(RS485_UART_NUM, &cfg);
    (void)uart_set_pin(
        RS485_UART_NUM, RS485_TX_PIN, RS485_RX_PIN, RS485_DIR_PIN,
        UART_PIN_NO_CHANGE);
    (void)uart_driver_install(RS485_UART_NUM, 1024, 0, 0, NULL, 0);
    (void)uart_set_mode(RS485_UART_NUM, UART_MODE_RS485_HALF_DUPLEX);

    Rs485_RTS_Enabled = false;
    Rs485_Bytes_Tx = 0;
    Rs485_Bytes_Rx = 0;
    rs485_silence_reset();
}

/**
 * @brief Update the cached RS485 transmit-enable state
 * @param enable true when transmit is active
 */
void rs485_rts_enable(bool enable)
{
    /* Hardware RS485 half-duplex mode drives DIR. Keep state for stack queries.
     */
    Rs485_RTS_Enabled = enable;
}

/**
 * @brief Check whether the RS485 transmit-enable state is active
 * @return true if transmit is active
 */
bool rs485_rts_enabled(void)
{
    return Rs485_RTS_Enabled;
}

/**
 * @brief Read one byte from the RS485 receive FIFO when available
 * @param data_register destination for the received byte, or NULL to probe only
 * @return true if at least one byte is available
 */
bool rs485_byte_available(uint8_t *data_register)
{
    size_t buffered = 0;
    (void)uart_get_buffered_data_len(RS485_UART_NUM, &buffered);
    if (buffered == 0U) {
        return false;
    }
    if (!data_register) {
        return true;
    }

    if (uart_read_bytes(RS485_UART_NUM, data_register, 1, 0) == 1) {
        Rs485_Bytes_Rx++;
        rs485_silence_reset();
        return true;
    }

    return false;
}

/**
 * @brief Report whether an RS485 receive error is pending
 * @return true if a receive error is detected
 */
bool rs485_receive_error(void)
{
    /* In Arduino-ESP32 builds, low-level UART error queues are not always
     * exposed. */
    return false;
}

/**
 * @brief Send a block of RS485 bytes
 * @param buffer transmit buffer
 * @param nbytes number of bytes to send
 */
void rs485_bytes_send(const uint8_t *buffer, uint16_t nbytes)
{
    if (!buffer || (nbytes == 0U)) {
        return;
    }

    rs485_rts_enable(true);
    (void)uart_write_bytes(RS485_UART_NUM, (const char *)buffer, nbytes);
    (void)uart_wait_tx_done(RS485_UART_NUM, pdMS_TO_TICKS(20));
    Rs485_Bytes_Tx += nbytes;
    rs485_rts_enable(false);
    rs485_silence_reset();
}

/**
 * @brief Get the configured RS485 baud rate
 * @return baud rate in bits per second
 */
uint32_t rs485_baud_rate(void)
{
    return Rs485_Baud_Rate;
}

/**
 * @brief Set the RS485 baud rate
 * @param baud baud rate in bits per second
 * @return true if the baud rate was applied
 */
bool rs485_baud_rate_set(uint32_t baud)
{
    if (baud == 0U) {
        return false;
    }

    Rs485_Baud_Rate = baud;
    return (uart_set_baudrate(RS485_UART_NUM, (int)Rs485_Baud_Rate) == ESP_OK);
}

/**
 * @brief Get the elapsed silent time on the RS485 bus
 * @return milliseconds since the last bus activity
 */
uint32_t rs485_silence_milliseconds(void)
{
    return mstimer_elapsed(&Silence_Timer);
}

/**
 * @brief Reset the RS485 silence timer
 */
void rs485_silence_reset(void)
{
    mstimer_set(&Silence_Timer, 0);
}

/**
 * @brief Get the number of transmitted RS485 bytes
 * @return transmitted byte count
 */
uint32_t rs485_bytes_transmitted(void)
{
    return Rs485_Bytes_Tx;
}

/**
 * @brief Get the number of received RS485 bytes
 * @return received byte count
 */
uint32_t rs485_bytes_received(void)
{
    return Rs485_Bytes_Rx;
}
