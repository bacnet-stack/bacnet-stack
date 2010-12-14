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
#include "bacint.h"
#include "bacreal.h"
#include "bits.h"
#include "bacothertypes.h"

struct BACnet_Destination;
struct BACnet_Recipient;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* from clause 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_tag(
        uint8_t * apdu,
        uint8_t tag_number,
        bool context_specific,
        uint32_t len_value_type);

/* from clause 20.2.1.3.2 Constructed Data */
/* returns the number of apdu bytes consumed */
    int encode_opening_tag(
        uint8_t * apdu,
        uint8_t tag_number);
    int encode_closing_tag(
        uint8_t * apdu,
        uint8_t tag_number);
    int decode_tag_number(
        uint8_t * apdu,
        uint8_t * tag_number);
    int decode_tag_number_and_value(
        uint8_t * apdu,
        uint8_t * tag_number,
        uint32_t * value);
    int decode_tag_number_and_value_safe(
        uint8_t * apdu,
        uint32_t apdu_len_remaining,
        uint8_t * tag_number,
        uint32_t * value);
/* returns true if the tag is an opening tag and matches */
    bool decode_is_opening_tag_number(
        uint8_t * apdu,
        uint8_t tag_number);
/* returns true if the tag is a closing tag and matches */
    bool decode_is_closing_tag_number(
        uint8_t * apdu,
        uint8_t tag_number);
/* returns true if the tag is context specific and matches */
    bool decode_is_context_tag(
        uint8_t * apdu,
        uint8_t tag_number);
    /* returns true if the tag is an opening tag */
    bool decode_is_opening_tag(
        uint8_t * apdu);
    /* returns true if the tag is a closing tag */
    bool decode_is_closing_tag(
        uint8_t * apdu);

/* from clause 20.2.2 Encoding of a Null Value */
    int encode_application_null(
        uint8_t * apdu);
    int encode_context_null(
        uint8_t * apdu,
        uint8_t tag_number);

/* from clause 20.2.3 Encoding of a Boolean Value */
    int encode_application_boolean(
        uint8_t * apdu,
        bool boolean_value);
    bool decode_boolean(
        uint32_t len_value);
    int encode_context_boolean(
        uint8_t * apdu,
        uint8_t tag_number,
        bool boolean_value);
    bool decode_context_boolean(
        uint8_t * apdu);

    int decode_context_boolean2(
        uint8_t * apdu,
        uint8_t tag_number,
        bool * boolean_value);

/* from clause 20.2.10 Encoding of a Bit String Value */
/* returns the number of apdu bytes consumed */
    int decode_bitstring(
        uint8_t * apdu,
        uint32_t len_value,
        BACNET_BIT_STRING * bit_string);

    int decode_context_bitstring(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_BIT_STRING * bit_string);
/* returns the number of apdu bytes consumed */
    int encode_bitstring(
        uint8_t * apdu,
        BACNET_BIT_STRING * bit_string);
    int encode_application_bitstring(
        uint8_t * apdu,
        BACNET_BIT_STRING * bit_string);
    int encode_context_bitstring(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_BIT_STRING * bit_string);

/* from clause 20.2.6 Encoding of a Real Number Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_application_real(
        uint8_t * apdu,
        float value);
    int encode_context_real(
        uint8_t * apdu,
        uint8_t tag_number,
        float value);

/* from clause 20.2.7 Encoding of a Double Precision Real Number Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_application_double(
        uint8_t * apdu,
        double value);

    int encode_context_double(
        uint8_t * apdu,
        uint8_t tag_number,
        double value);

/* from clause 20.2.14 Encoding of an Object Identifier Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int decode_object_id(
        uint8_t * apdu,
        uint16_t * object_type,
        uint32_t * instance);

    int decode_object_id_safe(
        uint8_t * apdu,
        uint32_t len_value,
        uint16_t * object_type,
        uint32_t * instance);

    int decode_context_object_id(
        uint8_t * apdu,
        uint8_t tag_number,
        uint16_t * object_type,
        uint32_t * instance);

    int encode_bacnet_object_id(
        uint8_t * apdu,
        int object_type,
        uint32_t instance);
    int encode_context_object_id(
        uint8_t * apdu,
        uint8_t tag_number,
        int object_type,
        uint32_t instance);
    int encode_application_object_id(
        uint8_t * apdu,
        int object_type,
        uint32_t instance);

/* from clause 20.2.8 Encoding of an Octet String Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_octet_string(
        uint8_t * apdu,
        BACNET_OCTET_STRING * octet_string);
    int encode_application_octet_string(
        uint8_t * apdu,
        BACNET_OCTET_STRING * octet_string);
    int encode_context_octet_string(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_OCTET_STRING * octet_string);
    int decode_octet_string(
        uint8_t * apdu,
        uint32_t len_value,
        BACNET_OCTET_STRING * octet_string);
    int decode_context_octet_string(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_OCTET_STRING * octet_string);


/* from clause 20.2.9 Encoding of a Character String Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_bacnet_character_string(
        uint8_t * apdu,
        BACNET_CHARACTER_STRING * char_string);
    int encode_application_character_string(
        uint8_t * apdu,
        BACNET_CHARACTER_STRING * char_string);
    int encode_context_character_string(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_CHARACTER_STRING * char_string);
    int decode_character_string(
        uint8_t * apdu,
        uint32_t len_value,
        BACNET_CHARACTER_STRING * char_string);
    int decode_context_character_string(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_CHARACTER_STRING * char_string);


/* from clause 20.2.4 Encoding of an Unsigned Integer Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_bacnet_unsigned(
        uint8_t * apdu,
        uint32_t value);
    int encode_context_unsigned(
        uint8_t * apdu,
        uint8_t tag_number,
        uint32_t value);
    int encode_application_unsigned(
        uint8_t * apdu,
        uint32_t value);
    int decode_unsigned(
        uint8_t * apdu,
        uint32_t len_value,
        uint32_t * value);
    int decode_context_unsigned(
        uint8_t * apdu,
        uint8_t tag_number,
        uint32_t * value);

/* from clause 20.2.5 Encoding of a Signed Integer Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_bacnet_signed(
        uint8_t * apdu,
        int32_t value);
    int encode_application_signed(
        uint8_t * apdu,
        int32_t value);
    int encode_context_signed(
        uint8_t * apdu,
        uint8_t tag_number,
        int32_t value);
    int decode_signed(
        uint8_t * apdu,
        uint32_t len_value,
        int32_t * value);
    int decode_context_signed(
        uint8_t * apdu,
        uint8_t tag_number,
        int32_t * value);


/* from clause 20.2.11 Encoding of an Enumerated Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int decode_enumerated(
        uint8_t * apdu,
        uint32_t len_value,
        uint32_t * value);
    int decode_context_enumerated(
        uint8_t * apdu,
        uint8_t tag_value,
        uint32_t * value);
    int encode_bacnet_enumerated(
        uint8_t * apdu,
        uint32_t value);
    int encode_application_enumerated(
        uint8_t * apdu,
        uint32_t value);
    int encode_context_enumerated(
        uint8_t * apdu,
        uint8_t tag_number,
        uint32_t value);

/* from clause 20.2.13 Encoding of a Time Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_bacnet_time(
        uint8_t * apdu,
        BACNET_TIME * btime);
    int encode_application_time(
        uint8_t * apdu,
        BACNET_TIME * btime);
    int decode_bacnet_time(
        uint8_t * apdu,
        BACNET_TIME * btime);
    int decode_bacnet_time_safe(
        uint8_t * apdu,
        uint32_t len_value,
        BACNET_TIME * btime);
    int encode_context_time(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_TIME * btime);
    int decode_application_time(
        uint8_t * apdu,
        BACNET_TIME * btime);
    int decode_context_bacnet_time(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_TIME * btime);


/* BACnet Date */
/* year = years since 1900 */
/* month 1=Jan */
/* day = day of month */
/* wday 1=Monday...7=Sunday */

/* from clause 20.2.12 Encoding of a Date Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
    int encode_bacnet_date(
        uint8_t * apdu,
        BACNET_DATE * bdate);
    int encode_application_date(
        uint8_t * apdu,
        BACNET_DATE * bdate);
    int encode_context_date(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_DATE * bdate);
    int decode_date(
        uint8_t * apdu,
        BACNET_DATE * bdate);
    int decode_date_safe(
        uint8_t * apdu,
        uint32_t len_value,
        BACNET_DATE * bdate);
    int decode_application_date(
        uint8_t * apdu,
        BACNET_DATE * bdate);
    int decode_context_date(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_DATE * bdate);

    int decode_daterange(
        uint8_t * apdu,
        BACNET_DATE_RANGE * bdaterange);
    int decode_weeknday(
        uint8_t * apdu,
        BACNET_WEEKNDAY * bweeknday);
    int encode_daterange(
        uint8_t * apdu,
        BACNET_DATE_RANGE * bdaterange);
    int encode_weeknday(
        uint8_t * apdu,
        BACNET_WEEKNDAY * bweeknday);
    int decode_context_daterange(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_DATE_RANGE * bdaterange);
    int decode_context_weeknday(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_WEEKNDAY * bweeknday);
    int encode_context_daterange(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_DATE_RANGE * bdaterange);
    int encode_context_weeknday(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_WEEKNDAY * bweeknday);

    /* recipient address */
    int encode_recipient(
        uint8_t * apdu,
        struct BACnet_Recipient *destination);
    /* BACnetDestination : one item of a recipient_list property */
    int encode_destination(
        uint8_t * apdu,
        int max_apdu_len,
        struct BACnet_Destination *destination);
    /* encode a simple BACnetDateTime value */
    int encode_application_datetime(
        uint8_t * apdu,
        BACNET_DATE_TIME * datetime);
    /* decode a simple BACnetDateTime value */
    int decode_application_datetime(
        uint8_t * apdu,
        BACNET_DATE_TIME * datetime);
    /* BACnetCOVSubscription */
    int encode_cov_subscription(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_COV_SUBSCRIPTION * covs);
    /*/ BACnetCalendarEntry */
    int encode_calendar_entry(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_CALENDAR_ENTRY * entry);
    /*/ BACnetDailySchedule x7 */
    int encode_weekly_schedule(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_WEEKLY_SCHEDULE * week);
    int encode_special_event(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_SPECIAL_EVENT * special);
    /* recipient address */
    int decode_recipient(
        uint8_t * apdu,
        BACNET_RECIPIENT * destination);
    /* BACnetDestination : one item of a recipient_list property */
    int decode_destination(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_DESTINATION * destination);
    /* BACnetCOVSubscription */
    int decode_cov_subscription(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_COV_SUBSCRIPTION * covs);
    int decode_calendar_entry(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_CALENDAR_ENTRY * entry);
    /*/ BACnetDailySchedule x7 / weekly-schedule */
    int decode_weekly_schedule(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_WEEKLY_SCHEDULE * week);
    /*/ BACnetSpecialEvent */
    int decode_special_event(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_SPECIAL_EVENT * special);
    /* ReadAccessSpecification */
    int encode_read_access_specification(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_READ_ACCESS_SPECIFICATION * entry);
    /* ReadAccessSpecification */
    int encode_context_read_access_specification(
        uint8_t * apdu,
        int max_apdu_len,
        uint8_t tag_number,
        BACNET_READ_ACCESS_SPECIFICATION * entry);
    /* ReadAccessSpecification */
    int decode_read_access_specification(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_READ_ACCESS_SPECIFICATION * entry);
    /* ReadAccessSpecification */
    int decode_context_read_access_specification(
        uint8_t * apdu,
        int max_apdu_len,
        uint8_t tag_number,
        BACNET_READ_ACCESS_SPECIFICATION * entry);

    /* room checks to prevent buffer overflows */
    bool check_write_apdu_space(
        int apdu_len,
        int max_apdu,
        int space_needed);

/* from clause 20.1.2.4 max-segments-accepted */
/* and clause 20.1.2.5 max-APDU-length-accepted */
/* returns the encoded octet */
    uint8_t encode_max_segs_max_apdu(
        int max_segs,
        int max_apdu);
    int decode_max_segs(
        uint8_t octet);
    int decode_max_apdu(
        uint8_t octet);

/* returns the number of apdu bytes consumed */
    int encode_simple_ack(
        uint8_t * apdu,
        uint8_t invoke_id,
        uint8_t service_choice);

/* from clause 20.2.1.2 Tag Number */
/* true if extended tag numbering is used */
#define IS_EXTENDED_TAG_NUMBER(x) ((x & 0xF0) == 0xF0)

/* from clause 20.2.1.3.1 Primitive Data */
/* true if the extended value is used */
#define IS_EXTENDED_VALUE(x) ((x & 0x07) == 5)

/* from clause 20.2.1.1 Class */
/* true if the tag is context specific */
#define IS_CONTEXT_SPECIFIC(x) ((x & BIT3) == BIT3)

/* from clause 20.2.1.3.2 Constructed Data */
/* true if the tag is an opening tag */
#define IS_OPENING_TAG(x) ((x & 0x07) == 6)

/* from clause 20.2.1.3.2 Constructed Data */
/* true if the tag is a closing tag */
#define IS_CLOSING_TAG(x) ((x & 0x07) == 7)


#ifdef __cplusplus

}
#endif /* __cplusplus */
#endif
