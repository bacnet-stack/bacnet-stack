/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2007 Steve Karg

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
#ifndef BVLC_H
#define BVLC_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include "bacdef.h"
#include "npdu.h"

#ifdef __cplusplus
extern "C" {

#endif /* __cplusplus */

#if defined(BBMD_ENABLED) && BBMD_ENABLED
    void bvlc_maintenance_timer(
        time_t seconds);
#else
#define bvlc_maintenance_timer(x)
#endif
    /* registers with a bbmd as a foreign device */
    void bvlc_register_with_bbmd(
        uint32_t bbmd_address,  /* in network byte order */
        uint16_t bbmd_port,     /* in network byte order */
        uint16_t time_to_live_seconds);

    uint16_t bvlc_receive(
        BACNET_ADDRESS * src,   /* returns the source address */
        uint8_t * npdu, /* returns the NPDU */
        uint16_t max_npdu,      /* amount of space available in the NPDU  */
        unsigned timeout);      /* number of milliseconds to wait for a packet */

    int bvlc_send_pdu(
        BACNET_ADDRESS * dest,  /* destination address */
        BACNET_NPDU_DATA * npdu_data,   /* network information */
        uint8_t * pdu,  /* any data to be sent - may be null */
        unsigned pdu_len);

#if defined(BBMD_CLIENT_ENABLED) && BBMD_CLIENT_ENABLED
    int bvlc_encode_write_bdt_init(
        uint8_t * pdu,
        unsigned entries);
    int bvlc_encode_read_bdt(
        uint8_t * pdu);
    int bvlc_encode_read_fdt(
        uint8_t * pdu);
    int bvlc_encode_delete_fdt_entry(
        uint8_t * pdu,
        uint32_t address,       /* in network byte order */
        uint16_t port); /* in network byte order */
    int bvlc_encode_original_unicast_npdu(
        uint8_t * pdu,
        uint8_t * npdu,
        unsigned npdu_length);
    int bvlc_encode_original_broadcast_npdu(
        uint8_t * pdu,
        uint8_t * npdu,
        unsigned npdu_length);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* */
