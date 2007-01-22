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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "bacenum.h"
#include "bacdcode.h"
#include "bacdef.h"
#include "bacapp.h"
#include "bactext.h"
#include "datetime.h"

int bacapp_encode_application_data(uint8_t * apdu,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    int apdu_len = 0;           /* total length of the apdu, return value */

    if (value && apdu) {
        switch (value->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            apdu[0] = value->tag;
            apdu_len++;
            break;
        case BACNET_APPLICATION_TAG_BOOLEAN:
            apdu_len = encode_tagged_boolean(&apdu[0],
                value->type.Boolean);
            break;
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            apdu_len = encode_tagged_unsigned(&apdu[0],
                value->type.Unsigned_Int);
            break;
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            apdu_len = encode_tagged_signed(&apdu[0],
                value->type.Signed_Int);
            break;
        case BACNET_APPLICATION_TAG_REAL:
            apdu_len = encode_tagged_real(&apdu[0], value->type.Real);
            break;
        case BACNET_APPLICATION_TAG_ENUMERATED:
            apdu_len = encode_tagged_enumerated(&apdu[0],
                value->type.Enumerated);
            break;
        case BACNET_APPLICATION_TAG_DATE:
            apdu_len = encode_tagged_date(&apdu[0], &value->type.Date);
            break;
        case BACNET_APPLICATION_TAG_TIME:
            apdu_len = encode_tagged_time(&apdu[0], &value->type.Time);
            break;
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            apdu_len = encode_tagged_object_id(&apdu[0],
                value->type.Object_Id.type,
                value->type.Object_Id.instance);
            break;
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            apdu_len = encode_tagged_octet_string(&apdu[0],
                &value->type.Octet_String);
            break;
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            apdu_len = encode_tagged_character_string(&apdu[0],
                &value->type.Character_String);
            break;
        case BACNET_APPLICATION_TAG_BIT_STRING:
            apdu_len = encode_tagged_bitstring(&apdu[0],
                &value->type.Bit_String);
            break;
        case BACNET_APPLICATION_TAG_DOUBLE:
            /* FIXME: double is not implemented yet.
               apdu_len = encode_tagged_double(&apdu[0],
               value->type.Double);
             */
        default:
            break;
        }
    }

    return apdu_len;
}

int bacapp_decode_application_data(uint8_t * apdu,
    int max_apdu_len, BACNET_APPLICATION_DATA_VALUE * value)
{
    int len = 0;
    int tag_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    int object_type = 0;
    uint32_t instance = 0;

    /* FIXME: use max_apdu_len! */
    (void) max_apdu_len;
    if (apdu && !decode_is_context_specific(apdu)) {
        tag_len = decode_tag_number_and_value(&apdu[0],
            &tag_number, &len_value_type);
        if (tag_len) {
            len += tag_len;
            value->tag = tag_number;
            if (tag_number == BACNET_APPLICATION_TAG_NULL) {
                /* nothing else to do */
            } else if (tag_number == BACNET_APPLICATION_TAG_BOOLEAN)
                value->type.Boolean = decode_boolean(len_value_type);
            else if (tag_number == BACNET_APPLICATION_TAG_UNSIGNED_INT)
                len += decode_unsigned(&apdu[len],
                    len_value_type, &value->type.Unsigned_Int);
            else if (tag_number == BACNET_APPLICATION_TAG_SIGNED_INT)
                len += decode_signed(&apdu[len],
                    len_value_type, &value->type.Signed_Int);
            else if (tag_number == BACNET_APPLICATION_TAG_REAL)
                len += decode_real(&apdu[len], &(value->type.Real));
#if 0
            else if (tag_number == BACNET_APPLICATION_TAG_DOUBLE)
                len += decode_double(&apdu[len], &(value->type.Double));
#endif
            else if (tag_number == BACNET_APPLICATION_TAG_OCTET_STRING)
                len += decode_octet_string(&apdu[len],
                    len_value_type, &value->type.Octet_String);
            else if (tag_number == BACNET_APPLICATION_TAG_CHARACTER_STRING)
                len += decode_character_string(&apdu[len],
                    len_value_type, &value->type.Character_String);
            else if (tag_number == BACNET_APPLICATION_TAG_BIT_STRING)
                len += decode_bitstring(&apdu[len],
                    len_value_type, &value->type.Bit_String);
            else if (tag_number == BACNET_APPLICATION_TAG_ENUMERATED)
                len += decode_enumerated(&apdu[len],
                    len_value_type, &value->type.Enumerated);
            else if (tag_number == BACNET_APPLICATION_TAG_DATE)
                len += decode_date(&apdu[len], &value->type.Date);
            else if (tag_number == BACNET_APPLICATION_TAG_TIME)
                len += decode_bacnet_time(&apdu[len], &value->type.Time);
            else if (tag_number == BACNET_APPLICATION_TAG_OBJECT_ID) {
                len += decode_object_id(&apdu[len],
                    &object_type, &instance);
                value->type.Object_Id.type = object_type;
                value->type.Object_Id.instance = instance;
            }
        }
    }

    return len;
}

int bacapp_encode_context_data(uint8_t * apdu,
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID property)
{
    int apdu_len = 0;           /* total length of the apdu, return value */

    if (value && apdu) {
        switch (property) {
            case PROP_REQUESTED_SHED_LEVEL:
                switch (value->tag) {
                    case 0:
                    case 1:
                        apdu_len = encode_tagged_unsigned(&apdu[0],
                            value->type.Unsigned_Int);
                        break;
                    case 2:
                        apdu_len = encode_tagged_real(&apdu[0],
                            value->type.Real);
                        break;
                    default:
                        apdu_len = 0;
                        break;
                }
                break;
            default:
                break;
        }
    }
    return apdu_len;
}

int bacapp_decode_context_data(uint8_t * apdu,
    int max_apdu_len, BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID property)
{
    int len = 0;
    int tag_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;

    /* FIXME: use max_apdu_len! */
    (void) max_apdu_len;
    if (apdu && decode_is_context_specific(apdu)) {
        tag_len = decode_tag_number_and_value(&apdu[0],
            &tag_number, &len_value_type);
        if (tag_len) {
            len += tag_len;
            value->tag = tag_number;
            switch (property) {
                case PROP_REQUESTED_SHED_LEVEL:
                    switch (tag_number) {
                        case 0:
                        case 1:
                            len += decode_unsigned(&apdu[len],
                                len_value_type,
                                &(value->type.Unsigned_Int));
                            break;
                        case 2:
                            len += decode_real(&apdu[len],
                                &(value->type.Real));
                            break;
                        default:
                            len = 0;
                            break;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    return len;
}


bool bacapp_copy(BACNET_APPLICATION_DATA_VALUE * dest_value,
    BACNET_APPLICATION_DATA_VALUE * src_value)
{
    bool status = true;         /*return value */

    if (dest_value && src_value) {
        dest_value->tag = src_value->tag;
        switch (src_value->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            break;
        case BACNET_APPLICATION_TAG_BOOLEAN:
            dest_value->type.Boolean = src_value->type.Boolean;
            break;
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            dest_value->type.Unsigned_Int = src_value->type.Unsigned_Int;
            break;
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            dest_value->type.Signed_Int = src_value->type.Signed_Int;
            break;
        case BACNET_APPLICATION_TAG_REAL:
            dest_value->type.Real = src_value->type.Real;
            break;
        case BACNET_APPLICATION_TAG_DOUBLE:
            dest_value->type.Double = src_value->type.Double;
            break;
        case BACNET_APPLICATION_TAG_ENUMERATED:
            dest_value->type.Enumerated = src_value->type.Enumerated;
            break;
        case BACNET_APPLICATION_TAG_DATE:
            datetime_copy_date(&dest_value->type.Date, &src_value->type.Date);
            break;
        case BACNET_APPLICATION_TAG_TIME:
            datetime_copy_time(&dest_value->type.Time, &src_value->type.Time);
            break;
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            dest_value->type.Object_Id.type =
                src_value->type.Object_Id.type;
            dest_value->type.Object_Id.instance =
                src_value->type.Object_Id.instance;
            break;
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            octetstring_copy(&dest_value->type.Octet_String,
                &src_value->type.Octet_String);
            break;
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            characterstring_copy(&dest_value->type.Character_String,
                &src_value->type.Character_String);
            break;
        case BACNET_APPLICATION_TAG_BIT_STRING:
        default:
            status = false;
            break;
        }
    }

    return status;
}

/* returns the length of data between an opening tag and a closing tag.
   Expects that the first octet contain the opening tag.
   Include a value property identifier for context specific data
   such as the value received in a WriteProperty request */
int bacapp_data_len(uint8_t *apdu, int max_apdu_len,
    BACNET_PROPERTY_ID property)
{
  int len = 0;
  int total_len = 0;
  int apdu_len = 0;
  uint8_t tag_number = 0;
  uint8_t opening_tag_number = 0;
  uint8_t opening_tag_number_counter = 0;
  uint32_t value = 0;
  BACNET_APPLICATION_DATA_VALUE application_value;

  if (decode_is_opening_tag(&apdu[0])) {
      len = decode_tag_number_and_value(&apdu[apdu_len],
          &tag_number, &value);
      apdu_len += len;
      opening_tag_number = tag_number;
      opening_tag_number_counter = 1;
      while (opening_tag_number_counter) {
          if (decode_is_opening_tag(&apdu[apdu_len])) {
              len = decode_tag_number_and_value(&apdu[apdu_len],
                  &tag_number, &value);
              if (tag_number == opening_tag_number)
                  opening_tag_number_counter++;
          } else if (decode_is_closing_tag(&apdu[apdu_len])) {
              len = decode_tag_number_and_value(&apdu[apdu_len],
                  &tag_number, &value);
              if (tag_number == opening_tag_number)
                  opening_tag_number_counter--;
          } else if (decode_is_context_specific(&apdu[apdu_len])) {
              /* context-specific tagged data */
              len = bacapp_decode_context_data(&apdu[apdu_len],
                  max_apdu_len - apdu_len, &application_value, property);
          } else {
              /* application tagged data */
              len = bacapp_decode_application_data(&apdu[apdu_len],
                  max_apdu_len - apdu_len, &application_value);
          }
          apdu_len += len;
          if (opening_tag_number_counter)
            total_len += len;
          /* ERROR! */
          if (apdu_len > max_apdu_len)
          {
            total_len = -1;
            break;
          }
      }
  }

  return total_len;
}

#ifdef BACAPP_PRINT_ENABLED
bool bacapp_print_value(FILE * stream,
    BACNET_APPLICATION_DATA_VALUE * value, BACNET_PROPERTY_ID property)
{
    bool status = true;         /*return value */
    size_t len = 0, i = 0;
    char *char_str;
    uint8_t *octet_str;

    if (value) {
        switch (value->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            fprintf(stream, "Null");
            break;
        case BACNET_APPLICATION_TAG_BOOLEAN:
            fprintf(stream, "%s", value->type.Boolean ? "TRUE" : "FALSE");
            break;
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            fprintf(stream, "%u", value->type.Unsigned_Int);
            break;
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            fprintf(stream, "%d", value->type.Signed_Int);
            break;
        case BACNET_APPLICATION_TAG_REAL:
            fprintf(stream, "%f", (double) value->type.Real);
            break;
        case BACNET_APPLICATION_TAG_DOUBLE:
            fprintf(stream, "%f", value->type.Double);
            break;
        case BACNET_APPLICATION_TAG_ENUMERATED:
            switch (property) {
            case PROP_OBJECT_TYPE:
                fprintf(stream, "%s",
                    bactext_object_type_name(value->type.Enumerated));
                break;
            case PROP_EVENT_STATE:
                fprintf(stream, "%s",
                    bactext_event_state_name(value->type.Enumerated));
                break;
            case PROP_UNITS:
                fprintf(stream, "%s",
                    bactext_engineering_unit_name(value->type.Enumerated));
                break;
            case PROP_PRESENT_VALUE:
                fprintf(stream, "%s",
                    bactext_binary_present_value_name(value->type.
                        Enumerated));
                break;
            case PROP_RELIABILITY:
                fprintf(stream, "%s",
                    bactext_reliability_name(value->type.Enumerated));
                break;
            case PROP_SYSTEM_STATUS:
                fprintf(stream, "%s",
                    bactext_device_status_name(value->type.Enumerated));
                break;
            case PROP_SEGMENTATION_SUPPORTED:
                fprintf(stream, "%s",
                    bactext_segmentation_name(value->type.Enumerated));
                break;
            default:
                fprintf(stream, "%u", value->type.Enumerated);
                break;
            }
            break;
        case BACNET_APPLICATION_TAG_DATE:
            fprintf(stream, "%s, %s %u, %u",
                bactext_day_of_week_name(value->type.Date.wday),
                bactext_month_name(value->type.Date.month),
                (unsigned) value->type.Date.day,
                (unsigned) value->type.Date.year);
            break;
        case BACNET_APPLICATION_TAG_TIME:
            fprintf(stream, "%02u:%02u:%02u.%03u",
                (unsigned) value->type.Time.hour,
                (unsigned) value->type.Time.min,
                (unsigned) value->type.Time.sec,
                (unsigned) value->type.Time.hundredths);
            break;
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            fprintf(stream, "%s %u",
                bactext_object_type_name(value->type.Object_Id.type),
                value->type.Object_Id.instance);
            break;
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            len = octetstring_length(&value->type.Octet_String);
            octet_str = octetstring_value(&value->type.Octet_String);
            for (i = 0; i < len; i++) {
                fprintf(stream, "%02X", *octet_str);
                octet_str++;
            }
            break;
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            len = characterstring_length(&value->type.Character_String);
            char_str =
                characterstring_value(&value->type.Character_String);
            fprintf(stream, "\"");
            for (i = 0; i < len; i++) {
                if (isprint(*char_str))
                    fprintf(stream, "%c", *char_str);
                else
                    fprintf(stream, ".");
                char_str++;
            }
            fprintf(stream, "\"");
            break;
        case BACNET_APPLICATION_TAG_BIT_STRING:
            len = bitstring_bits_used(&value->type.Bit_String);
            fprintf(stream, "{");
            for (i = 0; i < len; i++) {
                fprintf(stream, "%s",
                    bitstring_bit(&value->type.Bit_String,
                        (uint8_t)i) ? "true" : "false");
                if (i < len - 1)
                    fprintf(stream, ",");
            }
            fprintf(stream, "}");
            break;
        default:
            status = false;
            break;
        }
    }

    return status;
}

/* used to load the app data struct with the proper data
   converted from a command line argument */
bool bacapp_parse_application_data(BACNET_APPLICATION_TAG tag_number,
    const char *argv, BACNET_APPLICATION_DATA_VALUE * value)
{
    int hour, min, sec, hundredths;
    int year, month, day, wday;
    int object_type = 0;
    uint32_t instance = 0;
    bool status = false;
    long long_value = 0;
    unsigned long unsigned_long_value = 0;
    double double_value = 0.0;
    int count = 0;

    if (value && (tag_number < MAX_BACNET_APPLICATION_TAG)) {
        status = true;
        value->tag = tag_number;
        if (tag_number == BACNET_APPLICATION_TAG_BOOLEAN) {
            long_value = strtol(argv, NULL, 0);
            if (long_value)
                value->type.Boolean = true;
            else
                value->type.Boolean = false;
        } else if (tag_number == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
            unsigned_long_value = strtoul(argv, NULL, 0);
            value->type.Unsigned_Int = unsigned_long_value;
        } else if (tag_number == BACNET_APPLICATION_TAG_SIGNED_INT) {
            long_value = strtol(argv, NULL, 0);
            value->type.Signed_Int = long_value;
        } else if (tag_number == BACNET_APPLICATION_TAG_REAL) {
            double_value = strtod(argv, NULL);
            value->type.Real = (float) double_value;
        } else if (tag_number == BACNET_APPLICATION_TAG_DOUBLE) {
            double_value = strtod(argv, NULL);
            value->type.Double = double_value;
        } else if (tag_number == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
            status =
                characterstring_init_ansi(&value->type.Character_String,
                (char *) argv);
        } else if (tag_number == BACNET_APPLICATION_TAG_OCTET_STRING) {
            status = octetstring_init(&value->type.Octet_String,
                (uint8_t *) argv, strlen(argv));
        } else if (tag_number == BACNET_APPLICATION_TAG_ENUMERATED) {
            unsigned_long_value = strtoul(argv, NULL, 0);
            value->type.Enumerated = unsigned_long_value;
        } else if (tag_number == BACNET_APPLICATION_TAG_DATE) {
            count =
                sscanf(argv, "%d/%d/%d:%d", &year, &month, &day, &wday);
            if (count == 4) {
                value->type.Date.year = year;
                value->type.Date.month = month;
                value->type.Date.day = day;
                value->type.Date.wday = wday;
            } else
                status = false;
        } else if (tag_number == BACNET_APPLICATION_TAG_TIME) {
            count =
                sscanf(argv, "%d:%d:%d.%d", &hour, &min, &sec,
                &hundredths);
            if (count == 4) {
                value->type.Time.hour = hour;
                value->type.Time.min = min;
                value->type.Time.sec = sec;
                value->type.Time.hundredths = hundredths;
            } else
                status = false;
        } else if (tag_number == BACNET_APPLICATION_TAG_OBJECT_ID) {
            count = sscanf(argv, "%d:%d", &object_type, &instance);
            if (count == 2) {
                value->type.Object_Id.type = object_type;
                value->type.Object_Id.instance = instance;
            } else
                status = false;
        }
    }

    return status;
}
#endif

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testBACnetApplicationDataLength(Test * pTest)
{
    int apdu_len = 0;           /* total length of the apdu, return value */
    int len = 0;                /* total length of the apdu, return value */
    int test_len = 0; /* length of the data */
    uint8_t apdu[480] = { 0 };
    BACNET_TIME local_time;
    BACNET_DATE local_date;

    /* create some constructed data */
    /* 1. zero elements */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len = bacapp_data_len(&apdu[0], apdu_len,
        PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES);
    ct_test(pTest, test_len == len);

    /* 2. application tagged data, one element */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_tagged_unsigned(&apdu[apdu_len], 4194303);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len = bacapp_data_len(&apdu[0], apdu_len,
        PROP_OBJECT_IDENTIFIER);
    ct_test(pTest, test_len == len);

    /* 3. application tagged data, multiple elements */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_tagged_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_unsigned(&apdu[apdu_len], 1);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_unsigned(&apdu[apdu_len], 42);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_unsigned(&apdu[apdu_len], 91);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_tagged_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len = bacapp_data_len(&apdu[0], apdu_len,
        PROP_PRIORITY_ARRAY);
    ct_test(pTest, test_len == len);

    /* 4. complex datatype - one element */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    test_len += len;
    apdu_len += len;
    local_date.year = 2006; /* AD */
    local_date.month = 4;   /* 1=Jan */
    local_date.day = 1;     /* 1..31 */
    local_date.wday = 6;    /* 1=Monday */
    len = encode_tagged_date(&apdu[apdu_len], &local_date);
    test_len += len;
    apdu_len += len;
    local_time.hour = 7;
    local_time.min = 0;
    local_time.sec = 3;
    local_time.hundredths = 1;
    len = encode_tagged_time(&apdu[apdu_len], &local_time);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len = bacapp_data_len(&apdu[0], apdu_len,
        PROP_START_TIME);
    ct_test(pTest, test_len == len);

    /* 5. complex datatype - multiple elements */



    /* 6. context tagged data, one element */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_context_unsigned(&apdu[apdu_len], 1, 91);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len = bacapp_data_len(&apdu[0], apdu_len,
        PROP_REQUESTED_SHED_LEVEL);
    ct_test(pTest, test_len == len);
}

/* generic - can be used by other unit tests
   returns true if matching or same, false if different */
bool bacapp_same_value(BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_APPLICATION_DATA_VALUE * test_value)
{
    bool status = true;         /*return value */

    /* does the tag match? */
    if (test_value->tag != value->tag)
        status = false;
    if (status) {
        /* does the value match? */
        switch (test_value->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            break;
        case BACNET_APPLICATION_TAG_BOOLEAN:
            if (test_value->type.Boolean != value->type.Boolean)
                status = false;
            break;
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            if (test_value->type.Unsigned_Int != value->type.Unsigned_Int)
                status = false;
            break;
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            if (test_value->type.Signed_Int != value->type.Signed_Int)
                status = false;
            break;
        case BACNET_APPLICATION_TAG_REAL:
            if (test_value->type.Real != value->type.Real)
                status = false;
            break;
        case BACNET_APPLICATION_TAG_DOUBLE:
            if (test_value->type.Double != value->type.Double)
                status = false;
            break;
        case BACNET_APPLICATION_TAG_ENUMERATED:
            if (test_value->type.Enumerated != value->type.Enumerated)
                status = false;
            break;
        case BACNET_APPLICATION_TAG_DATE:
            if (datetime_compare_date(&test_value->type.Date,
                &value->type.Date) != 0)
                status = false;
            break;
        case BACNET_APPLICATION_TAG_TIME:
            if (datetime_compare_time(&test_value->type.Time,
                &value->type.Time) != 0)
                status = false;
            break;
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            if (test_value->type.Object_Id.type !=
                value->type.Object_Id.type)
                status = false;
            if (test_value->type.Object_Id.instance !=
                value->type.Object_Id.instance)
                status = false;
            break;
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            status = characterstring_same(&value->type.Character_String,
                &test_value->type.Character_String);
            break;
        case BACNET_APPLICATION_TAG_BIT_STRING:
        default:
            status = false;
            break;
        }
    }
    return status;
}

static bool testBACnetApplicationDataValue(BACNET_APPLICATION_DATA_VALUE *
    value)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    BACNET_APPLICATION_DATA_VALUE test_value;

    apdu_len = bacapp_encode_application_data(&apdu[0], value);
    len = bacapp_decode_application_data(&apdu[0], apdu_len, &test_value);

    return bacapp_same_value(value, &test_value);
}

void testBACnetApplicationData(Test * pTest)
{
    BACNET_APPLICATION_DATA_VALUE value;
    bool status = false;


    status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_NULL,
        NULL, &value);
    ct_test(pTest, status == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN,
        "1", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Boolean == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN,
        "0", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Boolean == false);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_UNSIGNED_INT,
        "0", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Unsigned_Int == 0);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_UNSIGNED_INT,
        "0xFFFF", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Unsigned_Int == 0xFFFF);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_UNSIGNED_INT,
        "0xFFFFFFFF", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Unsigned_Int == 0xFFFFFFFF);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_SIGNED_INT,
        "0", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Signed_Int == 0);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_SIGNED_INT,
        "-1", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Signed_Int == -1);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_SIGNED_INT,
        "32768", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Signed_Int == 32768);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_SIGNED_INT,
        "-32768", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Signed_Int == -32768);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL,
        "0.0", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL,
        "-1.0", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL,
        "1.0", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL,
        "3.14159", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL,
        "-3.14159", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_ENUMERATED,
        "0", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Enumerated == 0);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_ENUMERATED,
        "0xFFFF", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Enumerated == 0xFFFF);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_ENUMERATED,
        "0xFFFFFFFF", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Enumerated == 0xFFFFFFFF);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_DATE,
        "2005/5/22:1", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Date.year == 2005);
    ct_test(pTest, value.type.Date.month == 5);
    ct_test(pTest, value.type.Date.day == 22);
    ct_test(pTest, value.type.Date.wday == 1);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status = bacapp_parse_application_data(BACNET_APPLICATION_TAG_TIME,
        "23:59:59.12", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Time.hour == 23);
    ct_test(pTest, value.type.Time.min == 59);
    ct_test(pTest, value.type.Time.sec == 59);
    ct_test(pTest, value.type.Time.hundredths == 12);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_OBJECT_ID,
        "0:100", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Object_Id.type == 0);
    ct_test(pTest, value.type.Object_Id.instance == 100);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data
        (BACNET_APPLICATION_TAG_CHARACTER_STRING, "Karg!", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    return;
}

#ifdef TEST_BACNET_APPLICATION_DATA
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Application Data", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testBACnetApplicationData);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetApplicationDataLength);
    assert(rc);
    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_BACNET_APPLICATION_DATA */
#endif                          /* TEST */
