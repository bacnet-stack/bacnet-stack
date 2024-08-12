/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef EEPROM_H
#define EEPROM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    int eeprom_bytes_read(
        uint16_t ee_address,    /* EEPROM starting memory address */
        uint8_t * buffer,       /* data to store */
        int nbytes);    /* number of bytes of data to read */
    int eeprom_bytes_write(
        uint16_t ee_address,    /* EEPROM starting memory address */
        uint8_t * buffer,       /* data to send */
        int nbytes);    /* number of bytes of data */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
