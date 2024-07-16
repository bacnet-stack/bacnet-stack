/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic NPDU handler
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
#ifndef NPDU_HANDLER_H
#define NPDU_HANDLER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void npdu_handler(
        BACNET_ADDRESS * src,
        uint8_t * pdu,
        uint16_t pdu_len);

    BACNET_STACK_EXPORT
    uint16_t npdu_network_number(void);
    BACNET_STACK_EXPORT
    void npdu_network_number_set(uint16_t net);
    BACNET_STACK_EXPORT
    int npdu_send_network_number_is(
        BACNET_ADDRESS *dst,
        uint16_t net,
        uint8_t status);
    BACNET_STACK_EXPORT
    int npdu_send_what_is_network_number(
        BACNET_ADDRESS *dst);

    BACNET_STACK_EXPORT
    void npdu_handler_cleanup(void);
    BACNET_STACK_EXPORT
    void npdu_handler_init(
        uint16_t bip_net,
        uint16_t mstp_net);
    BACNET_STACK_EXPORT
    void npdu_router_handler(
        uint16_t snet,
        BACNET_ADDRESS * src,
        uint8_t * pdu,
        uint16_t pdu_len);
    BACNET_STACK_EXPORT
    int npdu_router_send_pdu(
        uint16_t dnet,
        BACNET_ADDRESS * dest,
        BACNET_NPDU_DATA * npdu_data,
        uint8_t * pdu,
        unsigned int pdu_len);
    BACNET_STACK_EXPORT
    void npdu_router_get_my_address(
        uint16_t dnet,
        BACNET_ADDRESS * my_address);
    BACNET_STACK_EXPORT
    int npdu_send_reject_message_to_network(
        BACNET_ADDRESS *dst,
        uint16_t net,
        uint8_t status);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
