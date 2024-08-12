/**************************************************************************
 *
 * Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *********************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "hardware.h"
#include "eeprom.h"

/* Internal EEPROM of the AVR - http://supp.iar.com/Support/?note=45745 */

int eeprom_bytes_read(
    uint16_t eeaddr, /* EEPROM starting memory address (offset of zero) */
    uint8_t *buf, /* data to store */
    int len)
{ /* number of bytes of data to read */
    int count = 0; /* return value */

    while (len) {
        __EEGET(buf[count], eeaddr);
        count++;
        eeaddr++;
        len--;
    }

    return count;
}

int eeprom_bytes_write(uint16_t eeaddr, /* EEPROM starting memory address */
    uint8_t *buf, /* data to send */
    int len)
{ /* number of bytes of data */
    int count = 0;

    while (len) {
        __EEPUT(eeaddr, buf[count]);
        count++;
        eeaddr++;
        len--;
    }

    return count;
}
