/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic WritePropertyMultiple service send
*
* @section LICENSE
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef SEND_ROUTER_H
#define SEND_ROUTER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
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
