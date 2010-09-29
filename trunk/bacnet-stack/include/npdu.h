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
#ifndef NPDU_H
#define NPDU_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacenum.h"

/** Use a hop count default that is generous but reasonable. */
#define DFLT_HOP_COUNT 15

/* an NPDU structure keeps the parameter stack to a minimum */
typedef struct bacnet_npdu_data_t {
    uint8_t protocol_version;
    /* parts of the control octet: */
    bool data_expecting_reply;
    bool network_layer_message; /* false if APDU */
    BACNET_MESSAGE_PRIORITY priority;
    /* optional network message info */
    BACNET_NETWORK_MESSAGE_TYPE network_message_type;   /* optional */
    uint16_t vendor_id; /* optional, if net message type is > 0x80 */
    uint8_t hop_count;
} BACNET_NPDU_DATA;

struct router_port_t;
/** The info[] string has no agreed-upon purpose, hence it is useless. 
 * Keeping it short here. This size could be 0-255. */
#define ROUTER_PORT_INFO_LEN 2
/** Port Info structure used by Routers for their routing table. */
typedef struct router_port_t {
    uint16_t dnet;		/**< The DNET number that identifies this port. */
    uint8_t id;			/**< Either 0 or some ill-defined, meaningless value. */
    uint8_t info[ROUTER_PORT_INFO_LEN];  /**< Info like 'modem dialing string' */
    uint8_t info_len;	/**< Length of info[]. */
    struct router_port_t *next; 	/**< Point to next in linked list */
} BACNET_ROUTER_PORT;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    uint8_t npdu_encode_max_seg_max_apdu(
        int max_segs,
        int max_apdu);

    int npdu_encode_pdu(
        uint8_t * npdu,
        BACNET_ADDRESS * dest,
        BACNET_ADDRESS * src,
        BACNET_NPDU_DATA * npdu_data);

    void npdu_encode_npdu_data(
        BACNET_NPDU_DATA * npdu,
        bool data_expecting_reply,
        BACNET_MESSAGE_PRIORITY priority);

    void npdu_copy_data(
        BACNET_NPDU_DATA * dest,
        BACNET_NPDU_DATA * src);

    int npdu_decode(
        uint8_t * npdu,
        BACNET_ADDRESS * dest,
        BACNET_ADDRESS * src,
        BACNET_NPDU_DATA * npdu_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
