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
#ifndef BACAPP_H
#define BACAPP_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "bacdef.h"
#include "bacstr.h"

typedef struct BACnet_Application_Data_Value {
    uint8_t tag;
    union {
        /* NULL - not needed as it is encoded in the tag alone */
        bool Boolean;
        uint32_t Unsigned_Int;
        int32_t Signed_Int;
        float Real;
        double Double;
        BACNET_OCTET_STRING Octet_String;
        BACNET_CHARACTER_STRING Character_String;
        BACNET_BIT_STRING Bit_String;
        int Enumerated;
        BACNET_DATE Date;
        BACNET_TIME Time;
        BACNET_OBJECT_ID Object_Id;
    } type;
} BACNET_APPLICATION_DATA_VALUE;

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

    int bacapp_decode_application_data(uint8_t * apdu,
        int max_apdu_len, BACNET_APPLICATION_DATA_VALUE * value);

    int bacapp_encode_application_data(uint8_t * apdu,
        BACNET_APPLICATION_DATA_VALUE * value);

    int bacapp_decode_context_data(uint8_t * apdu,
        int max_apdu_len, BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_PROPERTY_ID property);

    int bacapp_encode_context_data(uint8_t * apdu,
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_PROPERTY_ID property);

    bool bacapp_copy(BACNET_APPLICATION_DATA_VALUE * dest_value,
        BACNET_APPLICATION_DATA_VALUE * src_value);

    /* returns the length of data between an opening tag and a closing tag.
       Expects that the first octet contain the opening tag.
       Include a value property identifier for context specific data
       such as the value received in a WriteProperty request */
    int bacapp_data_len(uint8_t *apdu, int max_apdu_len,
        BACNET_PROPERTY_ID property);

#if PRINT_ENABLED
#define BACAPP_PRINT_ENABLED
#else
#ifdef TEST
#define BACAPP_PRINT_ENABLED
#endif
#endif

#ifdef BACAPP_PRINT_ENABLED
    bool bacapp_parse_application_data(BACNET_APPLICATION_TAG tag_number,
        const char *argv, BACNET_APPLICATION_DATA_VALUE * value);

    bool bacapp_print_value(FILE * stream,
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_PROPERTY_ID property);
#else
#define bacapp_parse_application_data(x,y,z) {(void)x;(void)y;(void)z;}
#define bacapp_print_value(x,y,z) {(void)x;(void)y;(void)z;}
#endif

#ifdef TEST
#include "ctest.h"
    bool bacapp_same_time(BACNET_TIME * time1, BACNET_TIME * time2);
    bool bacapp_same_date(BACNET_DATE * date1, BACNET_DATE * date2);
    bool bacapp_same_value(BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_APPLICATION_DATA_VALUE * test_value);

    void testBACnetApplicationDataLength(Test * pTest);
    void testBACnetApplicationData(Test * pTest);
#endif

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif
