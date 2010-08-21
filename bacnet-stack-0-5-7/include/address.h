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
#ifndef ADDRESS_H
#define ADDRESS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "bacdef.h"
#include "readrange.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void address_init(
        struct bacnet_session_object *sess);

    void address_init_partial(
        struct bacnet_session_object *sess);

    void address_add(
        struct bacnet_session_object *sess,
        uint32_t device_id,
        unsigned max_apdu,
        uint8_t segmentation,
        BACNET_ADDRESS * src);

    void address_remove_device(
        struct bacnet_session_object *sess,
        uint32_t device_id);

    bool address_get_by_device(
        struct bacnet_session_object *sess,
        uint32_t device_id,
        unsigned *max_apdu,
        uint8_t * segmentation,
        BACNET_ADDRESS * src);

    bool address_get_by_index(
        struct bacnet_session_object *sess,
        unsigned index,
        uint32_t * device_id,
        unsigned *max_apdu,
        uint8_t * segmentation,
        BACNET_ADDRESS * src);

    bool address_get_device_id(
        struct bacnet_session_object *sess,
        BACNET_ADDRESS * src,
        uint32_t * device_id);

    unsigned address_count(
        struct bacnet_session_object *sess);

    bool address_match(
        BACNET_ADDRESS * dest,
        BACNET_ADDRESS * src);

    bool address_bind_request(
        struct bacnet_session_object *sess,
        uint32_t device_id,
        unsigned *max_apdu,
        uint8_t * segmentation,
        BACNET_ADDRESS * src);

    void address_add_binding(
        struct bacnet_session_object *sess,
        uint32_t device_id,
        unsigned max_apdu,
        uint8_t segmentation,
        BACNET_ADDRESS * src);

    int address_list_encode(
        struct bacnet_session_object *sess,
        uint8_t * apdu,
        unsigned apdu_len);

    int rr_address_list_encode(
        struct bacnet_session_object *sess,
        uint8_t * apdu,
        BACNET_READ_RANGE_DATA * pRequest);

    void address_set_device_TTL(
        struct bacnet_session_object *sess,
        uint32_t device_id,
        uint32_t TimeOut,
        bool StaticFlag);

    void address_cache_timer(
        struct bacnet_session_object *sess,
        uint16_t uSeconds);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
