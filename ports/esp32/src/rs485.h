/**
 * @file
 * @brief RS485 UART driver declarations used by BACnet MS/TP on ESP32
 * @author Kato Gangstad
 */

#ifndef M5STAMPLC_RS485_H
#define M5STAMPLC_RS485_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RS485_UART_NUM UART_NUM_1
#define RS485_BAUD_RATE 38400
#define RS485_TX_PIN 0
#define RS485_RX_PIN 39
#define RS485_DIR_PIN 46

/**
 * @brief Initialize the RS485 UART peripheral
 */
void rs485_init(void);

/**
 * @brief Update the cached RS485 transmit-enable state
 * @param enable true when transmit is active
 */
void rs485_rts_enable(bool enable);

/**
 * @brief Check whether the RS485 transmit-enable state is active
 * @return true if transmit is active
 */
bool rs485_rts_enabled(void);

/**
 * @brief Read one byte from the RS485 receive FIFO when available
 * @param data_register destination for the received byte, or NULL to probe only
 * @return true if a byte is available
 */
bool rs485_byte_available(uint8_t *data_register);

/**
 * @brief Report whether an RS485 receive error is pending
 * @return true if a receive error is detected
 */
bool rs485_receive_error(void);

/**
 * @brief Send a block of RS485 bytes
 * @param buffer transmit buffer
 * @param nbytes number of bytes to send
 */
void rs485_bytes_send(const uint8_t *buffer, uint16_t nbytes);

/**
 * @brief Get the configured RS485 baud rate
 * @return baud rate in bits per second
 */
uint32_t rs485_baud_rate(void);

/**
 * @brief Set the RS485 baud rate
 * @param baud baud rate in bits per second
 * @return true if the baud rate was applied
 */
bool rs485_baud_rate_set(uint32_t baud);

/**
 * @brief Get the elapsed silent time on the RS485 bus
 * @return milliseconds since the last bus activity
 */
uint32_t rs485_silence_milliseconds(void);

/**
 * @brief Reset the RS485 silence timer
 */
void rs485_silence_reset(void);

/**
 * @brief Get the number of transmitted RS485 bytes
 * @return transmitted byte count
 */
uint32_t rs485_bytes_transmitted(void);

/**
 * @brief Get the number of received RS485 bytes
 * @return received byte count
 */
uint32_t rs485_bytes_received(void);

#ifdef __cplusplus
}
#endif

#endif
