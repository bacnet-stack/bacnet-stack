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
#include "bacport.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void RS485_Set_Interface(char *ifname);
BACNET_STACK_EXPORT
const char *RS485_Interface(void);

BACNET_STACK_EXPORT
void RS485_Initialize(void);

BACNET_STACK_EXPORT
void RS485_Send_Frame(
    struct mstp_port_struct_t *mstp_port, /* port specific data */
    const uint8_t *buffer, /* frame to send (up to 501 bytes of data) */
    uint16_t nbytes); /* number of bytes of data (up to 501) */

BACNET_STACK_EXPORT
void RS485_Check_UART_Data(
    struct mstp_port_struct_t *mstp_port); /* port specific data */

BACNET_STACK_EXPORT
uint32_t RS485_Get_Baud_Rate(void);
BACNET_STACK_EXPORT
bool RS485_Set_Baud_Rate(uint32_t baud);

BACNET_STACK_EXPORT
void RS485_Print_Error(void);

BACNET_STACK_EXPORT
bool RS485_Interface_Valid(unsigned port_number);
BACNET_STACK_EXPORT
void RS485_Print_Ports(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
