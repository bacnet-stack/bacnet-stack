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
#ifndef BACNET_SEGMENT_ACK_H
#define BACNET_SEGMENT_ACK_H

#include <stdint.h>
#include <stdbool.h>

#include "bacenum.h"
#include "bacdcode.h"
#include "bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int segmentack_encode_apdu(
    uint8_t *apdu,
    bool negativeack,
    bool server,
    uint8_t invoke_id,
    uint8_t sequence_number,
    uint8_t actual_window_size);

BACNET_STACK_EXPORT
int segmentack_decode_service_request(
    uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    uint8_t *sequence_number,
    uint8_t *actual_window_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* SEGMENT_ACK_H */
