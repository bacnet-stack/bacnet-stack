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
#include "bacnet/datalink/mstp.h"

extern uint32_t RS485_Baud_Rate;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void RS485_Reinit(
        void);
    void RS485_Initialize(
        void);

    void RS485_Disable(
        void);

    void RS485_Send_Frame(
        volatile struct mstp_port_struct_t *mstp_port,  /* port specific data */
        const uint8_t * buffer, /* frame to send (up to 501 bytes of data) */
        uint16_t nbytes);       /* number of bytes of data (up to 501) */

    /* returns true if there is more data waiting */
    bool RS485_Check_UART_Data(
        volatile struct mstp_port_struct_t *mstp_port); /* port specific data */

    void RS485_Interrupt_Rx(
        void);

    void RS485_Interrupt_Tx(
        void);

    uint32_t RS485_Get_Baud_Rate(
        void);
    bool RS485_Set_Baud_Rate(
        uint32_t baud);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
