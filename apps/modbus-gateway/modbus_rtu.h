/**
 * @file modbus_rtu.h
 * @brief Generic Modbus RTU master driver header.
 *
 * Supported function codes:
 *   FC01 - Read Coils
 *   FC02 - Read Discrete Inputs
 *   FC03 - Read Holding Registers
 *   FC04 - Read Input Registers
 *   FC05 - Write Single Coil
 *   FC06 - Write Single Register
 *
 * @author Jihye Ahn <jen.ahn@lge.com>
 *
 * @copyright Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Configuration constants
 * -----------------------------------------------------------------------*/

/** Default serial port for the Modbus RTU sensor (RS-485 to USB adapter) */
#define MODBUS_DEFAULT_PORT "/dev/ttyUSB1"

/** Default baud rate (typical factory default for temperature/humidity sensors)
 */
#define MODBUS_DEFAULT_BAUD 9600

/** Maximum response wait time for Modbus RTU [ms] */
#define MODBUS_TIMEOUT_MS 500

/** Maximum number of retries on communication failure */
#define MODBUS_MAX_RETRY 3

/** Default Modbus Slave ID for the sensor (adjust to match the actual device)
 */
#define MODBUS_SENSOR_SLAVE_ID 1

/* Modbus function codes */
#define MODBUS_FC_READ_COILS 0x01
#define MODBUS_FC_READ_DISCRETE_INPUTS 0x02
#define MODBUS_FC_READ_HOLDING_REGS 0x03
#define MODBUS_FC_READ_INPUT_REGS 0x04
#define MODBUS_FC_WRITE_SINGLE_COIL 0x05
#define MODBUS_FC_WRITE_SINGLE_REG 0x06

/* -----------------------------------------------------------------------
 * Data structures
 * -----------------------------------------------------------------------*/

/** Modbus RTU port handle */
typedef struct {
#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    int fd; /**< POSIX file descriptor */
#else
    void *handle; /**< Platform-specific port handle (unused/stub) */
#endif
    char port[64]; /**< Port name */
    uint32_t baud; /**< Baud rate */
    uint8_t slave_id; /**< Default slave ID */
} MODBUS_PORT;

/** Temperature and humidity sensor data (legacy, kept for compatibility) */
typedef struct {
    float temperature; /**< Temperature [deg C] */
    float humidity; /**< Relative humidity [%RH] */
    bool valid; /**< True when data is valid */
} SENSOR_DATA;

/* -----------------------------------------------------------------------
 * API
 * -----------------------------------------------------------------------*/

/**
 * @brief Initialize the Modbus RTU serial port.
 * @param port   Path to the serial port (e.g. "/dev/ttyUSB1")
 * @param baud   Baud rate (e.g. 9600)
 * @param slave  Default slave ID
 * @return true on success
 */
bool Modbus_RTU_Init(const char *port, uint32_t baud, uint8_t slave);

/**
 * @brief Close and release the Modbus RTU port.
 */
void Modbus_RTU_Cleanup(void);

/**
 * @brief FC03 - Read Holding Registers.
 * @param slave_id   Slave ID
 * @param reg_addr   Starting register address
 * @param count      Number of registers to read
 * @param regs       Output buffer (uint16_t array, at least @p count elements)
 * @return true on success
 */
bool Modbus_Read_Holding_Registers(
    uint8_t slave_id, uint16_t reg_addr, uint8_t count, uint16_t *regs);

/**
 * @brief Generic register read (FC03 or FC04).
 * @param slave_id   Slave ID
 * @param func_code  MODBUS_FC_READ_HOLDING_REGS or MODBUS_FC_READ_INPUT_REGS
 * @param reg_addr   Register address
 * @param out_value  Single register value output
 * @return true on success
 */
bool Modbus_Read_Register(
    uint8_t slave_id,
    uint8_t func_code,
    uint16_t reg_addr,
    uint16_t *out_value);

/**
 * @brief Generic bit read (FC01 or FC02).
 * @param slave_id   Slave ID
 * @param func_code  MODBUS_FC_READ_COILS or MODBUS_FC_READ_DISCRETE_INPUTS
 * @param bit_addr   Coil/input address
 * @param out_value  Output: 0 or 1
 * @return true on success
 */
bool Modbus_Read_Bit(
    uint8_t slave_id, uint8_t func_code, uint16_t bit_addr, uint8_t *out_value);

/**
 * @brief FC06 - Write Single Holding Register.
 * @param slave_id   Slave ID
 * @param reg_addr   Register address
 * @param value      16-bit value to write
 * @return true on success
 */
bool Modbus_Write_Register(uint8_t slave_id, uint16_t reg_addr, uint16_t value);

/**
 * @brief FC05 - Write Single Coil.
 * @param slave_id   Slave ID
 * @param coil_addr  Coil address
 * @param on         true = ON (0xFF00), false = OFF (0x0000)
 * @return true on success
 */
bool Modbus_Write_Coil(uint8_t slave_id, uint16_t coil_addr, bool on);

/**
 * @brief Read temperature and humidity from the Modbus RTU sensor (legacy).
 * @param slave_id   Slave ID
 * @param data       Output sensor data
 * @return true on success
 */
bool Modbus_Read_Sensor(uint8_t slave_id, SENSOR_DATA *data);

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_RTU_H */
