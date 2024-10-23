/**
 * @brief This module manages the internal AVR non-volatile data storage
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "hardware.h"
#include "eeprom.h"

/**
 * @brief Read a block of bytes from the EEPROM
 * @param eeaddr - EEPROM starting memory address (offset of zero)
 * @param buf - where to store the data that is read
 * @param len - number of bytes of data to read
 * @return The number of bytes read
 */
int eeprom_bytes_read(uint16_t eeaddr, uint8_t *buf, int len)
{
    int count = 0; /* return value */

    while (len) {
        __EEGET(buf[count], eeaddr);
        count++;
        eeaddr++;
        len--;
    }

    return count;
}

/**
 * @brief Write a block of bytes to the EEPROM
 * @param eeaddr - EEPROM starting memory address (offset of zero)
 * @param buf - data to write to the EEPROM
 * @param len - number of bytes of data
 * @return The number of bytes written
 */
int eeprom_bytes_write(uint16_t eeaddr, const uint8_t *buf, int len)
{
    int count = 0;

    while (len) {
        __EEPUT(eeaddr, buf[count]);
        count++;
        eeaddr++;
        len--;
    }

    return count;
}
