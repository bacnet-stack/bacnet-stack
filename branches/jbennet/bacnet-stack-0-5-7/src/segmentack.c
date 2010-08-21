/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2010 Julien Bennet

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
#include "segmentack.h"
#include <stdint.h>
#include "bacenum.h"
#include "bacdcode.h"
#include "bacdef.h"


/* encode service */
int segmentack_encode_apdu(
    uint8_t * apdu,
    bool negativeack,
    bool server,
    uint8_t invoke_id,
    uint8_t sequence_number,
    uint8_t actual_window_size)
{
    int apdu_len = 0;   /* total length of the apdu, return value */
    uint8_t server_code = server ? 0x01 : 0x00;
    uint8_t nak_code = negativeack ? 0x02 : 0x00;

    if (apdu) {
        apdu[0] = PDU_TYPE_SEGMENT_ACK | server_code | nak_code;
        apdu[1] = invoke_id;
        apdu[2] = sequence_number;
        apdu[3] = actual_window_size;
        apdu_len = 4;
    }

    return apdu_len;
}

/* decode the service request only */
int segmentack_decode_service_request(
    uint8_t * apdu,
    unsigned apdu_len,
    /*bool * negativeack, // shall be decoded by caller (if we keep the conventions) */
    /*bool * server, */
    uint8_t * invoke_id,
    uint8_t * sequence_number,
    uint8_t * actual_window_size)
{
    int len = 0;

    if (apdu_len >= 3) {
        if (invoke_id)
            *invoke_id = apdu[0];
        if (sequence_number)
            *sequence_number = apdu[1];
        if (actual_window_size)
            *actual_window_size = apdu[2];
    }

    return len;
}
