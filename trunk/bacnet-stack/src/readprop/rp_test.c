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
#include "rp.h"
#include <assert.h>
#include <string.h>
#include "ctest.h"

int rp_decode_apdu(uint8_t * apdu,
    unsigned apdu_len,
    uint8_t * invoke_id, BACNET_READ_PROPERTY_DATA * data)
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
    if (apdu[3] != SERVICE_CONFIRMED_READ_PROPERTY)
        return -1;
    offset = 4;

    if (apdu_len > offset) {
        len = rp_decode_service_request(&apdu[offset],
            apdu_len - offset, data);
    }

    return len;
}

int rp_ack_decode_apdu(uint8_t * apdu, int apdu_len,    /* total length of the apdu */
    uint8_t * invoke_id, BACNET_READ_PROPERTY_DATA * data)
{
    int len = 0;
    int offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_COMPLEX_ACK)
        return -1;
    *invoke_id = apdu[1];
    if (apdu[2] != SERVICE_CONFIRMED_READ_PROPERTY)
        return -1;
    offset = 3;
    if (apdu_len > offset) {
        len = rp_ack_decode_service_request(&apdu[offset],
            apdu_len - offset, data);
    }

    return len;
}

void testReadPropertyAck(Test * pTest)
{
    uint8_t apdu[480] = { 0 };
    uint8_t apdu2[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 1;
    uint8_t test_invoke_id = 0;
    BACNET_READ_PROPERTY_DATA data;
    BACNET_READ_PROPERTY_DATA test_data;
    BACNET_OBJECT_TYPE object_type = OBJECT_DEVICE;
    uint32_t object_instance = 0;
    int object = 0;

    data.object_type = OBJECT_DEVICE;
    data.object_instance = 1;
    data.object_property = PROP_OBJECT_IDENTIFIER;
    data.array_index = BACNET_ARRAY_ALL;

    data.application_data_len = encode_bacnet_object_id(&apdu2[0],
        data.object_type, data.object_instance);
    data.application_data = &apdu2[0];

    len = rp_ack_encode_apdu(&apdu[0], invoke_id, &data);
    ct_test(pTest, len != 0);
    ct_test(pTest, len != -1);
    apdu_len = len;
    len = rp_ack_decode_apdu(&apdu[0], apdu_len,        /* total length of the apdu */
        &test_invoke_id, &test_data);
    ct_test(pTest, len != -1);
    ct_test(pTest, test_invoke_id == invoke_id);

    ct_test(pTest, test_data.object_type == data.object_type);
    ct_test(pTest, test_data.object_instance == data.object_instance);
    ct_test(pTest, test_data.object_property == data.object_property);
    ct_test(pTest, test_data.array_index == data.array_index);
    ct_test(pTest,
        test_data.application_data_len == data.application_data_len);

    /* since object property == object_id, decode the application data using
       the appropriate decode function */
    len = decode_object_id(test_data.application_data,
        &object, &object_instance);
    object_type = object;
    ct_test(pTest, object_type == data.object_type);
    ct_test(pTest, object_instance == data.object_instance);
}

void testReadProperty(Test * pTest)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;
    BACNET_READ_PROPERTY_DATA data;
    BACNET_READ_PROPERTY_DATA test_data;

    data.object_type = OBJECT_DEVICE;
    data.object_instance = 1;
    data.object_property = PROP_OBJECT_IDENTIFIER;
    data.array_index = BACNET_ARRAY_ALL;
    len = rp_encode_apdu(&apdu[0], invoke_id, &data);
    ct_test(pTest, len != 0);
    apdu_len = len;

    len = rp_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
    ct_test(pTest, len != -1);
    ct_test(pTest, test_data.object_type == data.object_type);
    ct_test(pTest, test_data.object_instance == data.object_instance);
    ct_test(pTest, test_data.object_property == data.object_property);
    ct_test(pTest, test_data.array_index == data.array_index);

    return;
}
