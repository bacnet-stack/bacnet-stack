/**
 * @brief This module contains the API for internal AVR non-volatile data
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef EEPROM_H
#define EEPROM_H

#include <stdbool.h>
#include <stdint.h>

#ifndef EEPROM_BYTES_MAX
#define EEPROM_BYTES_MAX (1 * 1024)
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int eeprom_bytes_read(
    uint16_t ee_address, /* EEPROM starting memory address */
    uint8_t *buffer, /* data to store */
    int nbytes); /* number of bytes of data to read */
int eeprom_bytes_write(
    uint16_t ee_address, /* EEPROM starting memory address */
    const uint8_t *buffer, /* data to send */
    int nbytes); /* number of bytes of data */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
