/**************************************************************************
 *
 * Copyright (C) 2004 Steve Karg
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
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
        uint8_t * buffer,       /* data to send */
        uint16_t nbytes);       /* number of bytes of data */

    bool RS485_ReceiveError(
        void);
    bool RS485_DataAvailable(
        uint8_t * data);

    void RS485_Turnaround_Delay(
        void);
    uint32_t RS485_Get_Baud_Rate(
        void);
    bool RS485_Set_Baud_Rate(
        uint32_t baud);

    void RS485_LED_Timers(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
