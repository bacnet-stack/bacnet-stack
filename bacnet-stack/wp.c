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
#include "bacenum.h"
#include "bacdcode.h"
#include "bacdef.h"
#include "device.h"
#include "wp.h"

/* encode service */
int wp_encode_apdu(uint8_t * apdu,
    uint8_t invoke_id, BACNET_WRITE_PROPERTY_DATA * data)
{
    int apdu_len = 0;           /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] =
            encode_max_segs_max_apdu(0, Device_Max_APDU_Length_Accepted());
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_WRITE_PROPERTY;     /* service choice */
        apdu_len = 4;
        apdu_len += encode_context_object_id(&apdu[apdu_len], 0,
            data->object_type, data->object_instance);
        apdu_len += encode_context_enumerated(&apdu[apdu_len], 1,
            data->object_property);
        /* optional array index; ALL is -1 which is assumed when missing */
        if (data->array_index != BACNET_ARRAY_ALL)
            apdu_len += encode_context_unsigned(&apdu[apdu_len], 2,
                data->array_index);
        /* propertyValue */
        apdu_len += encode_opening_tag(&apdu[apdu_len], 3);
        apdu_len +=
            bacapp_encode_application_data(&apdu[apdu_len], &data->value);
        apdu_len += encode_closing_tag(&apdu[apdu_len], 3);
        /* optional priority - 0 if not set, 1..16 if set */
        if (data->priority != BACNET_NO_PRIORITY)
            apdu_len += encode_context_unsigned(&apdu[apdu_len], 4,
                data->priority);
    }

    return apdu_len;
}

/* decode the service request only */
/* FIXME: there could be various error messages returned
   using unique values less than zero */
int wp_decode_service_request(uint8_t * apdu,
    unsigned apdu_len, BACNET_WRITE_PROPERTY_DATA * data)
{
    int len = 0;
    int tag_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    int type = 0;               /* for decoding */
    int property = 0;           /* for decoding */
    uint32_t unsigned_value = 0;

    /* check for value pointers */
    if (apdu_len && data) {
        /* Tag 0: Object ID          */
        if (!decode_is_context_tag(&apdu[len++], 0))
            return -1;
        len += decode_object_id(&apdu[len], &type, &data->object_instance);
        data->object_type = type;
        /* Tag 1: Property ID */
        len += decode_tag_number_and_value(&apdu[len],
            &tag_number, &len_value_type);
        if (tag_number != 1)
            return -1;
        len += decode_enumerated(&apdu[len], len_value_type, &property);
        data->object_property = property;
        /* Tag 2: Optional Array Index */
        /* note: decode without incrementing len so we can check for opening tag */
        tag_len = decode_tag_number_and_value(&apdu[len],
            &tag_number, &len_value_type);
        if (tag_number == 2) {
            len += tag_len;
            len += decode_unsigned(&apdu[len], len_value_type,
                &unsigned_value);
            data->array_index = unsigned_value;
        } else
            data->array_index = BACNET_ARRAY_ALL;
        /* Tag 3: opening context tag */
        if (!decode_is_opening_tag_number(&apdu[len], 3))
            return -1;
        /* a tag number of 3 is not extended so only one octet */
        len++;
        len += bacapp_decode_application_data(&apdu[len],
            apdu_len - len, &data->value);
        /* FIXME: check the return value; abort if no valid data? */
        /* FIXME: there might be more than one data element in here! */
        if (!decode_is_closing_tag_number(&apdu[len], 3))
            return -1;
        /* a tag number of 3 is not extended so only one octet */
        len++;
        /* Tag 4: optional Priority - assumed MAX if not explicitly set */
        data->priority = BACNET_MAX_PRIORITY;
        if ((unsigned) len < apdu_len) {
            tag_len = decode_tag_number_and_value(&apdu[len],
                &tag_number, &len_value_type);
            if (tag_number == 4) {
                len += tag_len;
                len =
                    decode_unsigned(&apdu[len], len_value_type,
                    &unsigned_value);
                if ((unsigned_value >= BACNET_MIN_PRIORITY)
                    && (unsigned_value <= BACNET_MAX_PRIORITY)) {
                    data->priority = (uint8_t) unsigned_value;
                } else
                    return -1;
            }
        }
    }

    return len;
}

int wp_decode_apdu(uint8_t * apdu,
    unsigned apdu_len,
    uint8_t * invoke_id, BACNET_WRITE_PROPERTY_DATA * data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST)
        return -1;
    /*  apdu[1] = encode_max_segs_max_apdu(0, Device_Max_APDU_Length_Accepted()); */
    *invoke_id = apdu[2];       /* invoke id - filled in by net layer */
    if (apdu[3] != SERVICE_CONFIRMED_WRITE_PROPERTY)
        return -1;
    offset = 4;

    if (apdu_len > offset) {
        len = wp_decode_service_request(&apdu[offset],
            apdu_len - offset, data);
    }

    return len;
}





#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testWritePropertyTag(Test * pTest, BACNET_WRITE_PROPERTY_DATA * data)
{
    BACNET_WRITE_PROPERTY_DATA test_data = { 0 };
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;

    len = wp_encode_apdu(&apdu[0], invoke_id, data);
    ct_test(pTest, len != 0);
    apdu_len = len;

    len = wp_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
    ct_test(pTest, len != -1);
    ct_test(pTest, test_data.object_type == data->object_type);
    ct_test(pTest, test_data.object_instance == data->object_instance);
    ct_test(pTest, test_data.object_property == data->object_property);
    ct_test(pTest, test_data.array_index == data->array_index);
    ct_test(pTest, test_data.value.tag == data->value.tag);
    switch (test_data.value.tag) {
    case BACNET_APPLICATION_TAG_NULL:
        break;
    case BACNET_APPLICATION_TAG_BOOLEAN:
        ct_test(pTest, test_data.value.type.Boolean ==
            data->value.type.Boolean);
        break;
    case BACNET_APPLICATION_TAG_UNSIGNED_INT:
        ct_test(pTest, test_data.value.type.Unsigned_Int ==
            data->value.type.Unsigned_Int);
        break;
    case BACNET_APPLICATION_TAG_SIGNED_INT:
        ct_test(pTest, test_data.value.type.Signed_Int ==
            data->value.type.Signed_Int);
        break;
    case BACNET_APPLICATION_TAG_REAL:
        ct_test(pTest, test_data.value.type.Real == data->value.type.Real);
        break;
    case BACNET_APPLICATION_TAG_ENUMERATED:
        ct_test(pTest, test_data.value.type.Enumerated ==
            data->value.type.Enumerated);
        break;
    case BACNET_APPLICATION_TAG_DATE:
        ct_test(pTest, test_data.value.type.Date.year ==
            data->value.type.Date.year);
        ct_test(pTest, test_data.value.type.Date.month ==
            data->value.type.Date.month);
        ct_test(pTest, test_data.value.type.Date.day ==
            data->value.type.Date.day);
        ct_test(pTest, test_data.value.type.Date.wday ==
            data->value.type.Date.wday);
        break;
    case BACNET_APPLICATION_TAG_TIME:
        ct_test(pTest, test_data.value.type.Time.hour ==
            data->value.type.Time.hour);
        ct_test(pTest, test_data.value.type.Time.min ==
            data->value.type.Time.min);
        ct_test(pTest, test_data.value.type.Time.sec ==
            data->value.type.Time.sec);
        ct_test(pTest, test_data.value.type.Time.hundredths ==
            data->value.type.Time.hundredths);
        break;
    case BACNET_APPLICATION_TAG_OBJECT_ID:
        ct_test(pTest, test_data.value.type.Object_Id.type ==
            data->value.type.Object_Id.type);
        ct_test(pTest, test_data.value.type.Object_Id.instance ==
            data->value.type.Object_Id.instance);
        break;
    default:
        break;
    }
}

void testWriteProperty(Test * pTest)
{
    BACNET_WRITE_PROPERTY_DATA data = { 0 };

    data.value.tag = BACNET_APPLICATION_TAG_NULL;
    testWritePropertyTag(pTest, &data);

    data.value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    data.value.type.Boolean = true;
    testWritePropertyTag(pTest, &data);
    data.value.type.Boolean = false;
    testWritePropertyTag(pTest, &data);

    data.value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    data.value.type.Unsigned_Int = 0;
    testWritePropertyTag(pTest, &data);
    data.value.type.Unsigned_Int = 0xFFFF;
    testWritePropertyTag(pTest, &data);
    data.value.type.Unsigned_Int = 0xFFFFFFFF;
    testWritePropertyTag(pTest, &data);

    data.value.tag = BACNET_APPLICATION_TAG_SIGNED_INT;
    data.value.type.Signed_Int = 0;
    testWritePropertyTag(pTest, &data);
    data.value.type.Signed_Int = -1;
    testWritePropertyTag(pTest, &data);
    data.value.type.Signed_Int = 32768;
    testWritePropertyTag(pTest, &data);
    data.value.type.Signed_Int = -32768;
    testWritePropertyTag(pTest, &data);

    data.value.tag = BACNET_APPLICATION_TAG_REAL;
    data.value.type.Real = 0.0;
    testWritePropertyTag(pTest, &data);
    data.value.type.Real = -1.0;
    testWritePropertyTag(pTest, &data);
    data.value.type.Real = 1.0;
    testWritePropertyTag(pTest, &data);
    data.value.type.Real = 3.14159;
    testWritePropertyTag(pTest, &data);
    data.value.type.Real = -3.14159;
    testWritePropertyTag(pTest, &data);

    data.value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
    data.value.type.Enumerated = 0;
    testWritePropertyTag(pTest, &data);
    data.value.type.Enumerated = 0xFFFF;
    testWritePropertyTag(pTest, &data);
    data.value.type.Enumerated = 0xFFFFFFFF;
    testWritePropertyTag(pTest, &data);

    data.value.tag = BACNET_APPLICATION_TAG_DATE;
    data.value.type.Date.year = 2005;
    data.value.type.Date.month = 5;
    data.value.type.Date.day = 22;
    data.value.type.Date.wday = 1;
    testWritePropertyTag(pTest, &data);

    data.value.tag = BACNET_APPLICATION_TAG_TIME;
    data.value.type.Time.hour = 23;
    data.value.type.Time.min = 59;
    data.value.type.Time.sec = 59;
    data.value.type.Time.hundredths = 12;
    testWritePropertyTag(pTest, &data);

    data.value.tag = BACNET_APPLICATION_TAG_OBJECT_ID;
    data.value.type.Object_Id.type = OBJECT_ANALOG_INPUT;
    data.value.type.Object_Id.instance = 0;
    testWritePropertyTag(pTest, &data);
    data.value.type.Object_Id.type = OBJECT_LIFE_SAFETY_ZONE;
    data.value.type.Object_Id.instance = BACNET_MAX_INSTANCE;
    testWritePropertyTag(pTest, &data);

    return;
}

#ifdef TEST_WRITE_PROPERTY
uint16_t Device_Max_APDU_Length_Accepted(void)
{
    return MAX_APDU;
}

int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet WriteProperty", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testWriteProperty);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_WRITE_PROPERTY */
#endif                          /* TEST */
