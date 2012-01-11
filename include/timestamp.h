/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2008 John Minack

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

#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#include "bacdcode.h"

typedef enum {
    TIME_STAMP_TIME = 0,
    TIME_STAMP_SEQUENCE = 1,
    TIME_STAMP_DATETIME = 2
} BACNET_TIMESTAMP_TAG;

typedef uint8_t TYPE_BACNET_TIMESTAMP_TYPE;

typedef struct {
    TYPE_BACNET_TIMESTAMP_TYPE tag;
    union {
        BACNET_TIME time;
        uint16_t sequenceNum;
        BACNET_DATE_TIME dateTime;
    } value;
} BACNET_TIMESTAMP;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


    void bacapp_timestamp_copy(
        BACNET_TIMESTAMP * dest,
        BACNET_TIMESTAMP * src);

    int bacapp_encode_timestamp(
        uint8_t * apdu,
        BACNET_TIMESTAMP * value);
    int bacapp_decode_timestamp(
        uint8_t * apdu,
        BACNET_TIMESTAMP * value);


    int bacapp_encode_context_timestamp(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_TIMESTAMP * value);
    int bacapp_decode_context_timestamp(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_TIMESTAMP * value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
