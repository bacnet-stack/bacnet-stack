/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef SEEPROM_H
#define SEEPROM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    int seeprom_bytes_read(
        uint16_t ee_address,    /* SEEPROM starting memory address */
        uint8_t * buffer,       /* data to store */
        int nbytes);    /* number of bytes of data to read */
    int seeprom_bytes_write(
        uint16_t ee_address,    /* SEEPROM starting memory address */
        uint8_t * buffer,       /* data to send */
        int nbytes);    /* number of bytes of data */
    void seeprom_init(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
