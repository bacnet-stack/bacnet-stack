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
#ifndef BACDCODE_H
#define BACDCODE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bacdef.h"
#include "datetime.h"
#include "bacstr.h"

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

/* from clause 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_tag(uint8_t * apdu, uint8_t tag_number,
        bool context_specific, uint32_t len_value_type);

/* from clause 20.2.1.3.2 Constructed Data */
/* returns the number of apdu bytes consumed */
    int encode_opening_tag(uint8_t * apdu, uint8_t tag_number);
    int encode_closing_tag(uint8_t * apdu, uint8_t tag_number);
    int decode_tag_number(uint8_t * apdu, uint8_t * tag_number);
    int decode_tag_number_and_value(uint8_t * apdu, uint8_t * tag_number,
        uint32_t * value);
/* returns true if the tag is context specific */
    bool decode_is_context_specific(uint8_t * apdu);
/* returns true if the tag is an opening tag and matches */
    bool decode_is_opening_tag_number(uint8_t * apdu, uint8_t tag_number);
/* returns true if the tag is a closing tag and matches */
    bool decode_is_closing_tag_number(uint8_t * apdu, uint8_t tag_number);
/* returns true if the tag is context specific and matches */
    bool decode_is_context_tag(uint8_t * apdu, uint8_t tag_number);
    /* returns true if the tag is an opening tag */
    bool decode_is_opening_tag(uint8_t * apdu);
    /* returns true if the tag is a closing tag */
    bool decode_is_closing_tag(uint8_t * apdu);

/* from clause 20.2.2 Encoding of a Null Value */
    int encode_tagged_null(uint8_t * apdu);
    int encode_context_null(uint8_t * apdu, int tag_number);

/* from clause 20.2.3 Encoding of a Boolean Value */
    int encode_tagged_boolean(uint8_t * apdu, bool boolean_value);
    bool decode_boolean(uint32_t len_value);
    int encode_context_boolean(uint8_t * apdu, int tag_number,
        bool boolean_value);
    bool decode_context_boolean(uint8_t * apdu);

/* from clause 20.2.10 Encoding of a Bit String Value */
/* returns the number of apdu bytes consumed */
    int decode_bitstring(uint8_t * apdu, uint32_t len_value,
        BACNET_BIT_STRING * bit_string);
/* returns the number of apdu bytes consumed */
    int encode_bitstring(uint8_t * apdu, BACNET_BIT_STRING * bit_string);
    int encode_tagged_bitstring(uint8_t * apdu,
        BACNET_BIT_STRING * bit_string);
    int encode_context_bitstring(uint8_t * apdu, int tag_number,
        BACNET_BIT_STRING * bit_string);

/* from clause 20.2.6 Encoding of a Real Number Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int decode_real(uint8_t * apdu, float *real_value);
    int encode_bacnet_real(float value, uint8_t * apdu);
    int encode_tagged_real(uint8_t * apdu, float value);
    int encode_context_real(uint8_t * apdu, int tag_number, float value);

/* from clause 20.2.14 Encoding of an Object Identifier Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int decode_object_id(uint8_t * apdu, int *object_type,
        uint32_t * instance);
    int encode_bacnet_object_id(uint8_t * apdu, int object_type,
        uint32_t instance);
    int encode_context_object_id(uint8_t * apdu, int tag_number,
        int object_type, uint32_t instance);
    int encode_tagged_object_id(uint8_t * apdu, int object_type,
        uint32_t instance);

/* from clause 20.2.8 Encoding of an Octet String Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_octet_string(uint8_t * apdu,
        BACNET_OCTET_STRING * octet_string);
    int encode_tagged_octet_string(uint8_t * apdu,
        BACNET_OCTET_STRING * octet_string);
    int encode_context_octet_string(uint8_t * apdu,
        int tag_number, BACNET_OCTET_STRING * octet_string);
    int decode_octet_string(uint8_t * apdu, uint32_t len_value,
        BACNET_OCTET_STRING * octet_string);


/* from clause 20.2.9 Encoding of a Character String Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_bacnet_character_string(uint8_t * apdu,
        BACNET_CHARACTER_STRING * char_string);
    int encode_tagged_character_string(uint8_t * apdu,
        BACNET_CHARACTER_STRING * char_string);
    int encode_context_character_string(uint8_t * apdu, int tag_number,
        BACNET_CHARACTER_STRING * char_string);
    int decode_character_string(uint8_t * apdu, uint32_t len_value,
        BACNET_CHARACTER_STRING * char_string);

/* from clause 20.2.4 Encoding of an Unsigned Integer Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_bacnet_unsigned(uint8_t * apdu, uint32_t value);
    int encode_context_unsigned(uint8_t * apdu, int tag_number,
        uint32_t value);
    int encode_tagged_unsigned(uint8_t * apdu, uint32_t value);
    int decode_unsigned(uint8_t * apdu, uint32_t len_value,
        uint32_t * value);

/* from clause 20.2.5 Encoding of a Signed Integer Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_bacnet_signed(uint8_t * apdu, int32_t value);
    int encode_tagged_signed(uint8_t * apdu, int32_t value);
    int encode_context_signed(uint8_t * apdu, int tag_number,
        int32_t value);
    int decode_signed(uint8_t * apdu, uint32_t len_value, int32_t * value);

/* from clause 20.2.11 Encoding of an Enumerated Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int decode_enumerated(uint8_t * apdu, uint32_t len_value, int *value);
    int encode_bacnet_enumerated(uint8_t * apdu, int value);
    int encode_tagged_enumerated(uint8_t * apdu, int value);
    int encode_context_enumerated(uint8_t * apdu, int tag_number,
        int value);

/* from clause 20.2.13 Encoding of a Time Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_bacnet_time(uint8_t * apdu, BACNET_TIME * btime);
    int encode_tagged_time(uint8_t * apdu, BACNET_TIME * btime);
    int decode_bacnet_time(uint8_t * apdu, BACNET_TIME * btime);
    int encode_context_time(uint8_t * apdu, int tag_number,
        BACNET_TIME * btime);

/* BACnet Date */
/* year = years since 1900 */
/* month 1=Jan */
/* day = day of month */
/* wday 1=Monday...7=Sunday */

/* from clause 20.2.12 Encoding of a Date Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_bacnet_date(uint8_t * apdu, BACNET_DATE * bdate);
    int encode_tagged_date(uint8_t * apdu, BACNET_DATE * bdate);
    int encode_context_date(uint8_t * apdu, int tag_number,
        BACNET_DATE * bdate);
    int decode_date(uint8_t * apdu, BACNET_DATE * bdate);

/* two octet unsigned16 */
    int encode_unsigned16(uint8_t * apdu, uint16_t value);
    int decode_unsigned16(uint8_t * apdu, uint16_t * value);
/* four octet unsigned32 */
    int encode_unsigned32(uint8_t * apdu, uint32_t value);
    int decode_unsigned32(uint8_t * apdu, uint32_t * value);

/* from clause 20.1.2.4 max-segments-accepted */
/* and clause 20.1.2.5 max-APDU-length-accepted */
/* returns the encoded octet */
    uint8_t encode_max_segs_max_apdu(int max_segs, int max_apdu);
    int decode_max_segs(uint8_t octet);
    int decode_max_apdu(uint8_t octet);

/* returns the number of apdu bytes consumed */
    int encode_simple_ack(uint8_t * apdu, uint8_t invoke_id,
        uint8_t service_choice);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif
