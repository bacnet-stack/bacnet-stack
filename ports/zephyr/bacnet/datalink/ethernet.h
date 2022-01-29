/**************************************************************************
*
* Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
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
*********************************************************************/
#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"

/* specific defines for Ethernet */
#define ETHERNET_HEADER_MAX (6+6+2+1+1+1)
#define ETHERNET_MPDU_MAX (ETHERNET_HEADER_MAX+MAX_PDU)

/* Unless we explicitly need these remaps to be NOT exposed
 * (e.g. implementation where we need both bacnet and Zephyr symbols),
 * replace the BACnet Ethernet API symbols with non-conflicting ones.
 */
#if !defined(BACNET_ETHERNET_NO_REMAP_DEFINES)
#define ethernet_valid bacnet_ethernet_valid
#define ethernet_cleanup bacnet_ethernet_cleanup
#define ethernet_init bacnet_ethernet_init
#define ethernet_send_pdu bacnet_ethernet_send_pdu
#define ethernet_receive bacnet_ethernet_receive
#define ethernet_set_my_address bacnet_ethernet_set_my_address
#define ethernet_get_my_address bacnet_ethernet_get_my_address
#define ethernet_get_broadcast_address bacnet_ethernet_get_broadcast_address
#define ethernet_debug_address bacnet_ethernet_debug_address
#define ethernet_send bacnet_ethernet_send
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    bool bacnet_ethernet_valid(
        void);
    BACNET_STACK_EXPORT
    void bacnet_ethernet_cleanup(
        void);
    BACNET_STACK_EXPORT
    bool bacnet_ethernet_init(
        char *interface_name);

/* function to send a packet out the 802.2 socket */
/* returns number of bytes sent on success, negative on failure */
    BACNET_STACK_EXPORT
    int bacnet_ethernet_send_pdu(
        BACNET_ADDRESS * dest,  /* destination address */
        BACNET_NPDU_DATA * npdu_data,   /* network information */
        uint8_t * pdu,  /* any data to be sent - may be null */
        unsigned pdu_len);      /* number of bytes of data */

/* receives an 802.2 framed packet */
/* returns the number of octets in the PDU, or zero on failure */
    BACNET_STACK_EXPORT
    uint16_t bacnet_ethernet_receive(
        BACNET_ADDRESS * src,   /* source address */
        uint8_t * pdu,  /* PDU data */
        uint16_t max_pdu,       /* amount of space available in the PDU  */
        unsigned timeout);      /* milliseconds to wait for a packet */

    BACNET_STACK_EXPORT
    void bacnet_ethernet_set_my_address(
        BACNET_ADDRESS * my_address);
    BACNET_STACK_EXPORT
    void bacnet_ethernet_get_my_address(
        BACNET_ADDRESS * my_address);
    BACNET_STACK_EXPORT
    void bacnet_ethernet_get_broadcast_address(
        BACNET_ADDRESS * dest); /* destination address */

    /* some functions from Linux driver */
    BACNET_STACK_EXPORT
    void bacnet_ethernet_debug_address(
        const char *info,
        BACNET_ADDRESS * dest);
    BACNET_STACK_EXPORT
    int bacnet_ethernet_send(
        uint8_t * mtu,
        int mtu_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
