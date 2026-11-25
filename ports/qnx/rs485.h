/**
 * @file
 * @brief RS-485 serial interface for BACnet MS/TP on QNX Neutrino (QNX 8.0)
 * @author Vihar N
 * @date 2026
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#ifndef BACNET_PORT_QNX_RS485_H
#define BACNET_PORT_QNX_RS485_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/basic/sys/bacnet_stack_exports.h"
#include "bacnet/datalink/mstp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void RS485_Set_Interface(const char *ifname);
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
uint32_t RS485_Get_Port_Baud_Rate(struct mstp_port_struct_t *mstp_port);
BACNET_STACK_EXPORT
uint32_t RS485_Get_Baud_Rate(void);
BACNET_STACK_EXPORT
bool RS485_Set_Baud_Rate(uint32_t baud);

/* QNX: drive the RS-485 transmitter enable from RTS (for transceivers
   without automatic direction control). Uses devctl(DCMD_CHR_SERCTL). */
BACNET_STACK_EXPORT
void RS485_Set_RTS_Direction_Control(bool enable);
BACNET_STACK_EXPORT
bool RS485_RTS_Direction_Control(void);

/* Statistics useful for checking real-time behaviour */
typedef struct rs485_statistics {
    uint32_t frames_sent;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t write_errors;
    /* turnaround delay actually achieved, microseconds */
    uint32_t turnaround_usec_last;
    uint32_t turnaround_usec_max;
} RS485_STATISTICS;

BACNET_STACK_EXPORT
void RS485_Statistics(RS485_STATISTICS *stats);

BACNET_STACK_EXPORT
void RS485_Cleanup(void);

BACNET_STACK_EXPORT
void RS485_Print_Ports(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
