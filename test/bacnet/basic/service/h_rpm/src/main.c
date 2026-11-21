/**
 * @file
 * @brief Unit tests for handler_read_property_multiple service handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 *
 * Tests cover the special properties ALL, REQUIRED, and OPTIONAL:
 * Property_List shall not appear in the List of Results.
 */
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/ztest.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacenum.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/rpm.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"

/* Stub control variable defined in device_stub.c */
extern bool Stub_Optional_Only_Property_List;

#define STUB_PROPERTY_PROPRIETARY 512
#define MAX_RESULTS 16

/**
 * @brief Send an RPM request for one property of Analog Input 1
 *  and return the property identifiers found in the List of Results
 * @param property the (special) property to request
 * @param results [out] property identifiers in the order encoded
 * @return the number of property identifiers found, or -1 if the response
 *  is not a well-formed ComplexACK with a single result list
 */
static int rpm_results(BACNET_PROPERTY_ID property, uint32_t *results)
{
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data;
    uint8_t request[64] = { 0 };
    const uint8_t *apdu;
    int request_len = 0;
    int offset;
    int len;
    int count = 0;
    uint32_t value;
    uint8_t tag;

    request_len += rpm_encode_apdu_object_begin(
        &request[request_len], OBJECT_ANALOG_INPUT, 1);
    request_len += rpm_encode_apdu_object_property(
        &request[request_len], property, BACNET_ARRAY_ALL);
    request_len += rpm_encode_apdu_object_end(&request[request_len]);

    service_data.invoke_id = 1;
    service_data.priority = MESSAGE_PRIORITY_NORMAL;
    service_data.max_resp = MAX_APDU;
    memset(Handler_Transmit_Buffer, 0, sizeof(Handler_Transmit_Buffer));
    handler_read_property_multiple(
        request, (uint16_t)request_len, &src, &service_data);

    offset = bacnet_npdu_decode(
        Handler_Transmit_Buffer, MAX_PDU, NULL, NULL, &npdu_data);
    if (offset <= 0) {
        return -1;
    }
    apdu = &Handler_Transmit_Buffer[offset];
    if ((apdu[0] & 0xF0) != PDU_TYPE_COMPLEX_ACK) {
        return -1;
    }
    /* complex-ack header: PDU type, invoke ID, service choice */
    offset = 3;
    /* object identifier context tag 0 with 4 octets */
    if (apdu[offset] != 0x0C) {
        return -1;
    }
    offset += 5;
    /* list-of-results opening tag 1 */
    if (apdu[offset] != 0x1E) {
        return -1;
    }
    offset++;
    while (apdu[offset] != 0x1F) {
        /* property identifier context tag 2 with 1 or 2 octets */
        tag = apdu[offset];
        if ((tag & 0xF8) != 0x28) {
            return -1;
        }
        len = tag & 0x07;
        value = 0;
        offset++;
        while (len > 0) {
            value = (value << 8) | apdu[offset];
            offset++;
            len--;
        }
        if (count >= MAX_RESULTS) {
            return -1;
        }
        results[count] = value;
        count++;
        /* property-value opening tag 4, unsigned 1, closing tag 4 */
        if ((apdu[offset] != 0x4E) || (apdu[offset + 1] != 0x21) ||
            (apdu[offset + 2] != 0x01) || (apdu[offset + 3] != 0x4F)) {
            return -1;
        }
        offset += 4;
    }

    return count;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_rpm_tests, testHandlerRPM_All)
#else
static void testHandlerRPM_All(void)
#endif
{
    uint32_t results[MAX_RESULTS];
    int count;

    Stub_Optional_Only_Property_List = false;
    count = rpm_results(PROP_ALL, results);
    zassert_equal(count, 4, "count=%d", count);
    zassert_equal(results[0], PROP_OBJECT_IDENTIFIER, NULL);
    zassert_equal(results[1], PROP_OBJECT_TYPE, NULL);
    zassert_equal(results[2], PROP_DESCRIPTION, NULL);
    zassert_equal(results[3], STUB_PROPERTY_PROPRIETARY, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_rpm_tests, testHandlerRPM_Required)
#else
static void testHandlerRPM_Required(void)
#endif
{
    uint32_t results[MAX_RESULTS];
    int count;

    Stub_Optional_Only_Property_List = false;
    count = rpm_results(PROP_REQUIRED, results);
    zassert_equal(count, 2, "count=%d", count);
    zassert_equal(results[0], PROP_OBJECT_IDENTIFIER, NULL);
    zassert_equal(results[1], PROP_OBJECT_TYPE, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_rpm_tests, testHandlerRPM_Optional)
#else
static void testHandlerRPM_Optional(void)
#endif
{
    uint32_t results[MAX_RESULTS];
    int count;

    Stub_Optional_Only_Property_List = false;
    count = rpm_results(PROP_OPTIONAL, results);
    zassert_equal(count, 1, "count=%d", count);
    zassert_equal(results[0], PROP_DESCRIPTION, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_rpm_tests, testHandlerRPM_OptionalOnlyPropertyList)
#else
static void testHandlerRPM_OptionalOnlyPropertyList(void)
#endif
{
    uint32_t results[MAX_RESULTS];
    int count;

    Stub_Optional_Only_Property_List = true;
    /* an empty List of Results is returned for an object that exists */
    count = rpm_results(PROP_OPTIONAL, results);
    Stub_Optional_Only_Property_List = false;
    zassert_equal(count, 0, "count=%d", count);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_rpm_tests, testHandlerRPM_PropertyListExplicit)
#else
static void testHandlerRPM_PropertyListExplicit(void)
#endif
{
    uint32_t results[MAX_RESULTS];
    int count;

    /* an explicit request for Property_List is not a special property */
    count = rpm_results(PROP_PROPERTY_LIST, results);
    zassert_equal(count, 1, "count=%d", count);
    zassert_equal(results[0], PROP_PROPERTY_LIST, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(h_rpm_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        h_rpm_tests, ztest_unit_test(testHandlerRPM_All),
        ztest_unit_test(testHandlerRPM_Required),
        ztest_unit_test(testHandlerRPM_Optional),
        ztest_unit_test(testHandlerRPM_OptionalOnlyPropertyList),
        ztest_unit_test(testHandlerRPM_PropertyListExplicit));

    ztest_run_test_suite(h_rpm_tests);
}
#endif
