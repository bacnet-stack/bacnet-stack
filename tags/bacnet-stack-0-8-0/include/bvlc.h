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
#ifndef BVLC_H
#define BVLC_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include "bacdef.h"
#include "npdu.h"

struct sockaddr_in;     /* Defined elsewhere, needed here. */

#ifdef __cplusplus
extern "C" {

#endif /* __cplusplus */

#if defined(BBMD_ENABLED) && BBMD_ENABLED
    void bvlc_maintenance_timer(
        time_t seconds);
#else
#define bvlc_maintenance_timer(x)
#endif

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

    int bvlc_send_mpdu(
        struct sockaddr_in *dest,
        uint8_t * mtu,
        uint16_t mtu_len);

#if defined(BBMD_CLIENT_ENABLED) && BBMD_CLIENT_ENABLED
    int bvlc_encode_write_bdt_init(
        uint8_t * pdu,
        unsigned entries);
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
    int bvlc_encode_read_bdt(
        uint8_t * pdu);
    int bvlc_bbmd_read_bdt(
        uint32_t bbmd_address,
        uint16_t bbmd_port);

    /* registers with a bbmd as a foreign device */
    int bvlc_register_with_bbmd(
        uint32_t bbmd_address,  /* in network byte order */
        uint16_t bbmd_port,     /* in network byte order */
        uint16_t time_to_live_seconds);

    /* Note any BVLC_RESULT code, or NAK the BVLL message in the unsupported cases. */
    int bvlc_for_non_bbmd(
        struct sockaddr_in *sout,
        uint8_t * npdu,
        uint16_t received_bytes);

    /* Returns the last BVLL Result we received, either as the result of a BBMD
     * request we sent, or (if not a BBMD or Client), from trying to register
     * as a foreign device. */
    BACNET_BVLC_RESULT bvlc_get_last_result(
        void);

    /* Returns the current BVLL Function Code we are processing.
     * We have to store this higher layer code for when the lower layers
     * need to know what it is, especially to differentiate between
     * BVLC_ORIGINAL_UNICAST_NPDU and BVLC_ORIGINAL_BROADCAST_NPDU.  */
    BACNET_BVLC_FUNCTION bvlc_get_function_code(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* */
