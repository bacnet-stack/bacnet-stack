/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    bool serial_byte_get(
        uint8_t * data_register);
    bool serial_byte_peek(
        uint8_t * data_register);

    void serial_bytes_send(
        uint8_t * buffer,       /* data to send */
        uint16_t nbytes);       /* number of bytes of data */

    /* byte transmit */
    void serial_byte_send(
        uint8_t ch);
    void serial_byte_transmit_complete(
        void);

    uint32_t serial_baud_rate(
        void);
    bool serial_baud_rate_set(
        uint32_t baud);

    void serial_init(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
