/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
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

    void RS485_Initialize(
        void);

    void RS485_Transmitter_Enable(
        bool enable);

    void RS485_Send_Data(
        const uint8_t * buffer, /* data to send */
        uint16_t nbytes);       /* number of bytes of data */

    bool RS485_ReceiveError(
        void);
    bool RS485_DataAvailable(
        uint8_t * data);

    void RS485_Turnaround_Delay(
        void);
    uint32_t rs485_baud_rate(
        void);
    bool rs485_baud_rate_set(
        uint32_t baud);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
