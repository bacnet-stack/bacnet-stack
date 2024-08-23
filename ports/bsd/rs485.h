/**
 * @file port/bsd/rs485.h
 * @brief Provides BSD/Darwin(macOS) specific functions for RS-485 serial
 * operation
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Michael O'Neill <em.pee.oh@gmail.com>
 * @date 2004, 2024
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef RS485_H
#define RS485_H

#include <stdint.h>
#include "bacnet/basic/sys/bacnet_stack_exports.h"
#include "bacnet/datalink/mstp.h"

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
    uint8_t *buffer, /* frame to send (up to 501 bytes of data) */
    uint16_t nbytes); /* number of bytes of data (up to 501) */

BACNET_STACK_EXPORT
void RS485_Check_UART_Data(
    struct mstp_port_struct_t *mstp_port); /* port specific data */
BACNET_STACK_EXPORT
uint32_t RS485_Get_Port_Baud_Rate(struct mstp_port_struct_t *mstp_port);
BACNET_STACK_EXPORT
uint32_t RS485_Get_Baud_Rate(void);
BACNET_STACK_EXPORT
bool RS485_Set_Baud_Rate(uint32_t baud);

BACNET_STACK_EXPORT
void RS485_Cleanup(void);
BACNET_STACK_EXPORT
void RS485_Print_Ports(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
