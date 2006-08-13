/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2004 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "datalink.h"
/* currently this is oriented to a single data link */
/* However, it could handle multiple data links with the */
/* addition of passing a network number or datalink number */
/* as part of the calls. */

/* returns number of bytes sent on success, negative on failure */
int datalink_send_pdu(BACNET_ADDRESS * dest,    /* destination address */
    BACNET_NPDU_DATA * npdu_data, /* network information */
    uint8_t * pdu,              /* any data to be sent - may be null */
    unsigned pdu_len)
{                               /* number of bytes of data */
#if defined(BACDL_ARCNET)
    return arcnet_send_pdu(dest, npdu_data, pdu, pdu_len);
#elif defined(BACDL_MSTP)
    return dlmstp_send_pdu(dest, npdu_data, pdu, pdu_len);
#elif defined(BACDL_ETHERNET)
    return ethernet_send_pdu(dest, npdu_data, pdu, pdu_len);
#elif defined(BACDL_BIP)
    return bip_send_pdu(dest, npdu_data, pdu, pdu_len);
#else
    return 0;
#endif
}

/* returns the number of octets in the PDU, or zero on failure */
uint16_t datalink_receive(BACNET_ADDRESS * src, /* source address */
    uint8_t * pdu,              /* PDU data */
    uint16_t max_pdu,           /* amount of space available in the PDU  */
    unsigned timeout)
{                               /* number of milliseconds to wait for a packet */
#if defined(BACDL_ARCNET)
    return arcnet_receive(src, pdu, max_pdu, timeout);
#elif defined(BACDL_MSTP)
    return dlmstp_receive(src, pdu, max_pdu, timeout);
#elif defined(BACDL_ETHERNET)
    return ethernet_receive(src, pdu, max_pdu, timeout);
#elif defined(BACDL_BIP)
    return bip_receive(src, pdu, max_pdu, timeout);
#else
    return 0;
#endif
}

void datalink_cleanup(void)
{
#if defined(BACDL_ARCNET)
    arcnet_cleanup();
#elif defined(BACDL_MSTP)
    dlmstp_cleanup();
#elif defined(BACDL_ETHERNET)
    ethernet_cleanup();
#elif defined(BACDL_BIP)
    bip_cleanup();
#endif
}

void datalink_get_broadcast_address(BACNET_ADDRESS * dest)
{                               /* destination address */
#if defined(BACDL_ARCNET)
    arcnet_get_broadcast_address(dest);
#elif defined(BACDL_MSTP)
    dlmstp_get_broadcast_address(dest);
#elif defined(BACDL_ETHERNET)
    ethernet_get_broadcast_address(dest);
#elif defined(BACDL_BIP)
    bip_get_broadcast_address(dest);
#endif
}

void datalink_get_my_address(BACNET_ADDRESS * my_address)
{
#if defined(BACDL_ARCNET)
    arcnet_get_my_address(my_address);
#elif defined(BACDL_MSTP)
    dlmstp_get_my_address(my_address);
#elif defined(BACDL_ETHERNET)
    ethernet_get_my_address(my_address);
#elif defined(BACDL_BIP)
    bip_get_my_address(my_address);
#endif
}
