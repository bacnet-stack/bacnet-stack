/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic WritePropertyMultiple service send
* @copyright SPDX-License-Identifier: MIT
*/
#ifndef BACNET_BASIC_NPDU_SEND_ROUTER_H
#define BACNET_BASIC_NPDU_SEND_ROUTER_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int Send_Network_Layer_Message(
        BACNET_NETWORK_MESSAGE_TYPE network_message_type,
        BACNET_ADDRESS * dst,
        int *iArgs);
    BACNET_STACK_EXPORT
    void Send_Who_Is_Router_To_Network(
        BACNET_ADDRESS * dst,
        int dnet);
    BACNET_STACK_EXPORT
    void Send_I_Am_Router_To_Network(
        const int DNET_list[]);
    BACNET_STACK_EXPORT
    void Send_Reject_Message_To_Network(
        BACNET_ADDRESS * dst,
        uint8_t reject_reason,
        int dnet);
    BACNET_STACK_EXPORT
    void Send_Initialize_Routing_Table(
        BACNET_ADDRESS * dst,
        const int DNET_list[]);
    BACNET_STACK_EXPORT
    void Send_Initialize_Routing_Table_Ack(
        BACNET_ADDRESS * dst,
        const int DNET_list[]);
    BACNET_STACK_EXPORT
    void Send_Network_Number_Is(
        BACNET_ADDRESS *dst,
        int dnet,
        int status);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
