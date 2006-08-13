/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2006 Steve Karg

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
#include "bacapp.h"
#include "cov.h"
#include "device.h"
#include "datalink.h"
#include "npdu.h"

/* encode service */

/* Change-Of-Value Services
COV Subscribe
COV Subscribe Property
COV Notification
Unconfirmed COV Notification
*/
static int notify_encode_adpu(uint8_t * apdu, BACNET_COV_DATA * data)
{
    int len = 0;                /* length of each encoding */
    int apdu_len = 0;           /* total length of the apdu, return value */
    BACNET_PROPERTY_VALUE *value = NULL;        /* value in list */

    if (apdu) {
        /* tag 0 - subscriberProcessIdentifier */
        len = encode_context_unsigned(&apdu[apdu_len],
            0, data->subscriberProcessIdentifier);
        apdu_len += len;
        /* tag 1 - initiatingDeviceIdentifier */
        len = encode_context_object_id(&apdu[apdu_len],
            1, OBJECT_DEVICE, data->initiatingDeviceIdentifier);
        apdu_len += len;
        /* tag 2 - monitoredObjectIdentifier */
        len = encode_context_object_id(&apdu[apdu_len],
            2,
            data->monitoredObjectIdentifier.type,
            data->monitoredObjectIdentifier.instance);
        apdu_len += len;
        /* tag 3 - timeRemaining */
        len = encode_context_unsigned(&apdu[apdu_len],
            3, data->timeRemaining);
        apdu_len += len;
        /* tag 4 - listOfValues */
        len = encode_opening_tag(&apdu[apdu_len], 4);
        apdu_len += len;
        /* the first value includes a pointer to the next value, etc */
        /* FIXME: for small implementations, we might try a partial 
           approach like the rpm.c where the values are encoded with
           a separate function */
        value = &data->listOfValues;
        while (value != NULL) {
            /* tag 0 - propertyIdentifier */
            len = encode_context_enumerated(&apdu[apdu_len],
                0, value->propertyIdentifier);
            apdu_len += len;
            /* tag 1 - propertyArrayIndex OPTIONAL */
            if (value->propertyArrayIndex != BACNET_ARRAY_ALL) {
                len = encode_context_unsigned(&apdu[apdu_len],
                    1, value->propertyArrayIndex);
                apdu_len += len;
            }
            /* tag 2 - value */
            /* abstract syntax gets enclosed in a context tag */
            len = encode_opening_tag(&apdu[apdu_len], 2);
            apdu_len += len;
            len = bacapp_encode_application_data(&apdu[apdu_len],
                &value->value);
            apdu_len += len;
            len = encode_closing_tag(&apdu[apdu_len], 2);
            apdu_len += len;
            /* tag 3 - priority OPTIONAL */
            if (value->priority != BACNET_NO_PRIORITY) {
                len = encode_context_unsigned(&apdu[apdu_len], 3,
                    value->priority);
                apdu_len += len;
            }
            /* is there another one to encode? */
            /* FIXME: check to see if there is room in the APDU */
            value = value->next;
        }
        len = encode_closing_tag(&apdu[apdu_len], 4);
        apdu_len += len;
    }

    return apdu_len;
}

int ccov_notify_encode_apdu(uint8_t * apdu,
    uint8_t invoke_id, BACNET_COV_DATA * data)
{
    int len = 0;                /* length of each encoding */
    int apdu_len = 0;           /* total length of the apdu, return value */
    uint16_t max_apdu = Device_Max_APDU_Length_Accepted();

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, max_apdu);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_COV_NOTIFICATION;
        apdu_len = 4;
        len = notify_encode_adpu(&apdu[apdu_len], data);
        apdu_len += len;
    }

    return apdu_len;
}

int ucov_notify_encode_apdu(uint8_t * apdu, BACNET_COV_DATA * data)
{
    int len = 0;                /* length of each encoding */
    int apdu_len = 0;           /* total length of the apdu, return value */

    if (apdu && data) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_COV_NOTIFICATION; /* service choice */
        apdu_len = 2;
        len = notify_encode_adpu(&apdu[apdu_len], data);
        apdu_len += len;
    }

    return apdu_len;
}

/* decode the service request only */
/* COV and Unconfirmed COV are the same */
int cov_notify_decode_service_request(uint8_t * apdu,
    unsigned apdu_len, BACNET_COV_DATA * data)
{
    int len = 0;                /* return value */
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    uint32_t decoded_value = 0; /* for decoding */
    int decoded_type = 0;       /* for decoding */
    int property = 0;           /* for decoding */
    BACNET_PROPERTY_VALUE *value = NULL;        /* value in list */

    if (apdu_len && data) {
        /* tag 0 - subscriberProcessIdentifier */
        if (decode_is_context_tag(&apdu[len], 0)) {
            len +=
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value);
            len += decode_unsigned(&apdu[len], len_value, &decoded_value);
            data->subscriberProcessIdentifier = decoded_value;
        } else
            return -1;
        /* tag 1 - initiatingDeviceIdentifier */
        if (decode_is_context_tag(&apdu[len], 1)) {
            len +=
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value);
            len +=
                decode_object_id(&apdu[len], &decoded_type,
                &data->initiatingDeviceIdentifier);
            if (decoded_type != OBJECT_DEVICE)
                return -1;
        } else
            return -1;
        /* tag 2 - monitoredObjectIdentifier */
        if (decode_is_context_tag(&apdu[len], 2)) {
            len +=
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value);
            len +=
                decode_object_id(&apdu[len], &decoded_type,
                &data->monitoredObjectIdentifier.instance);
            data->monitoredObjectIdentifier.type = decoded_type;
        } else
            return -1;
        /* tag 3 - timeRemaining */
        if (decode_is_context_tag(&apdu[len], 3)) {
            len +=
                decode_tag_number_and_value(&apdu[len], &tag_number,
                &len_value);
            len += decode_unsigned(&apdu[len], len_value, &decoded_value);
            data->timeRemaining = decoded_value;
        } else
            return -1;
        /* tag 4: opening context tag - listOfValues */
        if (!decode_is_opening_tag_number(&apdu[len], 4))
            return -1;
        /* a tag number of 4 is not extended so only one octet */
        len++;
        /* the first value includes a pointer to the next value, etc */
        value = &data->listOfValues;
        while (value != NULL) {
            /* tag 0 - propertyIdentifier */
            if (decode_is_context_tag(&apdu[len], 0)) {
                len +=
                    decode_tag_number_and_value(&apdu[len], &tag_number,
                    &len_value);
                len += decode_enumerated(&apdu[len], len_value, &property);
                value->propertyIdentifier = property;
            } else
                return -1;
            /* tag 1 - propertyArrayIndex OPTIONAL */
            if (decode_is_context_tag(&apdu[len], 1)) {
                len +=
                    decode_tag_number_and_value(&apdu[len], &tag_number,
                    &len_value);
                len +=
                    decode_unsigned(&apdu[len], len_value, &decoded_value);
                value->propertyArrayIndex = decoded_value;
            } else
                value->propertyArrayIndex = BACNET_ARRAY_ALL;
            /* tag 2: opening context tag - value */
            if (!decode_is_opening_tag_number(&apdu[len], 2))
                return -1;
            /* a tag number of 2 is not extended so only one octet */
            len++;
            len += bacapp_decode_application_data(&apdu[len],
                apdu_len - len, &value->value);
            /* FIXME: check the return value; abort if no valid data? */
            /* FIXME: there might be more than one data element in here! */
            if (!decode_is_closing_tag_number(&apdu[len], 2))
                return -1;
            /* a tag number of 2 is not extended so only one octet */
            len++;
            /* tag 3 - priority OPTIONAL */
            if (decode_is_context_tag(&apdu[len], 3)) {
                len +=
                    decode_tag_number_and_value(&apdu[len], &tag_number,
                    &len_value);
                len +=
                    decode_unsigned(&apdu[len], len_value, &decoded_value);
                value->priority = decoded_value;
            } else
                value->priority = BACNET_NO_PRIORITY;
            /* end of list? */
            if (decode_is_closing_tag_number(&apdu[len], 4))
                break;
            /* is there another one to decode? */
            value = value->next;
            /* out of room to store more values */
            if (value == NULL)
                return -1;
        }
    }

    return len;
}

int ucov_notify_send(uint8_t * buffer, BACNET_COV_DATA * data)
{
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

    /* unconfirmed is a broadcast */
    datalink_get_broadcast_address(&dest);
    /* encode the APDU portion of the packet */
    pdu_len = ucov_notify_encode_apdu(&buffer[0], data);
    /* encode the NPDU portion of the packet */
    npdu_encode_unconfirmed_apdu(&npdu_data, MESSAGE_PRIORITY_NORMAL);
    /* send the data */
    bytes_sent = datalink_send_pdu(&dest, &npdu_data, &buffer[0], pdu_len);

    return bytes_sent;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"
#include "bacapp.h"

int ccov_notify_decode_apdu(uint8_t * apdu,
    unsigned apdu_len, uint8_t * invoke_id, BACNET_COV_DATA * data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST)
        return -2;
    /*  apdu[1] = encode_max_segs_max_apdu(0, Device_Max_APDU_Length_Accepted()); */
    *invoke_id = apdu[2];       /* invoke id - filled in by net layer */
    if (apdu[3] != SERVICE_CONFIRMED_COV_NOTIFICATION)
        return -3;
    offset = 4;

    /* optional limits - must be used as a pair */
    if (apdu_len > offset) {
        len =
            cov_notify_decode_service_request(&apdu[offset],
            apdu_len - offset, data);
    }

    return len;
}

int ucov_notify_decode_apdu(uint8_t * apdu,
    unsigned apdu_len, BACNET_COV_DATA * data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST)
        return -2;
    if (apdu[1] != SERVICE_UNCONFIRMED_COV_NOTIFICATION)
        return -3;
    /* optional limits - must be used as a pair */
    offset = 2;
    if (apdu_len > offset) {
        len =
            cov_notify_decode_service_request(&apdu[offset],
            apdu_len - offset, data);
    }

    return len;
}


/* dummy function stubs */
void npdu_encode_unconfirmed_apdu(BACNET_NPDU_DATA * npdu,
    BACNET_MESSAGE_PRIORITY priority)
{
    return;
}

/* dummy function stubs */
int datalink_send_pdu(BACNET_ADDRESS * dest,    /* destination address */
    BACNET_NPDU_DATA * npdu_data,       /* network information */
    uint8_t * pdu,              /* any data to be sent - may be null */
    unsigned pdu_len)
{                               /* number of bytes of data */
    return 0;
}

/* dummy function stubs */
void datalink_get_broadcast_address(BACNET_ADDRESS * dest)
{
}

/* dummy function stubs */
uint16_t Device_Max_APDU_Length_Accepted(void)
{
    return MAX_APDU;
}

void testCOVNotifyData(Test * pTest,
    BACNET_COV_DATA * data, BACNET_COV_DATA * test_data)
{
    ct_test(pTest,
        test_data->subscriberProcessIdentifier ==
        data->subscriberProcessIdentifier);
    ct_test(pTest,
        test_data->initiatingDeviceIdentifier ==
        data->initiatingDeviceIdentifier);
    ct_test(pTest,
        test_data->monitoredObjectIdentifier.type ==
        data->monitoredObjectIdentifier.type);
    ct_test(pTest,
        test_data->monitoredObjectIdentifier.instance ==
        data->monitoredObjectIdentifier.instance);
    ct_test(pTest, test_data->timeRemaining == data->timeRemaining);
    /* FIXME: test the listOfValues in some clever manner */
}

void testUCOVNotifyData(Test * pTest, BACNET_COV_DATA * data)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    BACNET_COV_DATA test_data;

    len = ucov_notify_encode_apdu(&apdu[0], data);
    ct_test(pTest, len > 0);
    apdu_len = len;

    test_data.listOfValues.next = NULL;
    len = ucov_notify_decode_apdu(&apdu[0], apdu_len, &test_data);
    ct_test(pTest, len != -1);
    testCOVNotifyData(pTest, data, &test_data);
}

void testCCOVNotifyData(Test * pTest, uint8_t invoke_id,
    BACNET_COV_DATA * data)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    BACNET_COV_DATA test_data;
    uint8_t test_invoke_id = 0;

    len = ccov_notify_encode_apdu(&apdu[0], invoke_id, data);
    ct_test(pTest, len != 0);
    apdu_len = len;

    test_data.listOfValues.next = NULL;
    len = ccov_notify_decode_apdu(&apdu[0], apdu_len,
        &test_invoke_id, &test_data);
    ct_test(pTest, len > 0);
    ct_test(pTest, test_invoke_id == invoke_id);
    testCOVNotifyData(pTest, data, &test_data);
}

void testCOVNotify(Test * pTest)
{
    uint8_t invoke_id = 12;
    BACNET_COV_DATA data;
    /* BACNET_PROPERTY_VALUE value2; */

    data.subscriberProcessIdentifier = 1;
    data.initiatingDeviceIdentifier = 123;
    data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    data.monitoredObjectIdentifier.instance = 321;
    data.timeRemaining = 456;

    data.listOfValues.propertyIdentifier = PROP_PRESENT_VALUE;
    data.listOfValues.propertyArrayIndex = BACNET_ARRAY_ALL;
    bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL,
        "21.0", &data.listOfValues.value);
    data.listOfValues.priority = 0;
    data.listOfValues.next = NULL;

    testUCOVNotifyData(pTest, &data);
    testCCOVNotifyData(pTest, invoke_id, &data);

    /* FIXME: add more values to the list of values */
}

#ifdef TEST_COV
int main(int argc, char *argv[])
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet COV", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testCOVNotify);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_COV */
#endif                          /* TEST */
