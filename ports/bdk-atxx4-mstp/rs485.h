/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef RS485_H
#define RS485_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void rs485_init(
        void);
    void rs485_rts_enable(
        bool enable);
    bool rs485_byte_available(
        uint8_t * data_register);
    bool rs485_receive_error(
        void);
    void rs485_bytes_send(
        const uint8_t * buffer,       /* data to send */
        uint16_t nbytes);       /* number of bytes of data */
    uint32_t rs485_baud_rate(
        void);
    bool rs485_baud_rate_set(
        uint32_t baud);

    void rs485_turnaround_delay(
        void);
    void rs485_silence_time_reset(
        void);
    bool rs485_silence_time_elapsed(
        uint16_t milliseconds);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
