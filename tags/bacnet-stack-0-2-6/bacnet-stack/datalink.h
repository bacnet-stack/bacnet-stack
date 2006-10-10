/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

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
#ifndef DATALINK_H
#define DATALINK_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "bacdef.h"
#include "npdu.h"

#if defined(BACDL_ETHERNET)
#include "ethernet.h"
#elif defined(BACDL_ARCNET)
#include "arcnet.h"
#elif defined(BACDL_MSTP)
#include "dlmstp.h"
#elif defined(BACDL_BIP)
#include "bip.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

/* returns number of bytes sent on success, negative on failure */
    int datalink_send_pdu(BACNET_ADDRESS * dest,        /* destination address */
        BACNET_NPDU_DATA * npdu_data,   /* network information */
        uint8_t * pdu,          /* any data to be sent - may be null */
        unsigned pdu_len);      /* number of bytes of data */

/* returns the number of octets in the PDU, or zero on failure */
    uint16_t datalink_receive(BACNET_ADDRESS * src,     /* source address */
        uint8_t * pdu,          /* PDU data */
        uint16_t max_pdu,       /* amount of space available in the PDU  */
        unsigned timeout);      /* number of milliseconds to wait for a packet */

    void datalink_cleanup(void);

    void datalink_get_broadcast_address(BACNET_ADDRESS * dest); /* destination address */

    void datalink_get_my_address(BACNET_ADDRESS * my_address);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif
