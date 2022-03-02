/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/wp.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test encode/decode API for unsigned 16b integers
 */
static int wp_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    BACNET_WRITE_PROPERTY_DATA *wpdata)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST)
        return -1;
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
    if (apdu[3] != SERVICE_CONFIRMED_WRITE_PROPERTY)
        return -1;
    offset = 4;

    if (apdu_len > offset) {
        len =
            wp_decode_service_request(&apdu[offset], apdu_len - offset, wpdata);
    }

    return len;
}

static void testWritePropertyTag(BACNET_APPLICATION_DATA_VALUE *value)
{
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    BACNET_WRITE_PROPERTY_DATA test_data = { 0 };
    BACNET_APPLICATION_DATA_VALUE test_value;
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;
    bool status = false;

    wpdata.application_data_len =
        bacapp_encode_application_data(&wpdata.application_data[0], value);
    len = wp_encode_apdu(&apdu[0], invoke_id, &wpdata);
    zassert_not_equal(len, 0, NULL);
    /* decode the data */
    apdu_len = len;
    len = wp_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(test_data.object_type, wpdata.object_type, NULL);
    zassert_equal(test_data.object_instance, wpdata.object_instance, NULL);
    zassert_equal(test_data.object_property, wpdata.object_property, NULL);
    zassert_equal(test_data.array_index, wpdata.array_index, NULL);
    /* decode the application value of the request */
    len = bacapp_decode_application_data(test_data.application_data,
        test_data.application_data_len, &test_value);
    zassert_equal(test_value.tag, value->tag, NULL);
    status = write_property_type_valid(&wpdata, value, test_value.tag);
    zassert_equal(status, true, NULL);
    switch (test_value.tag) {
        case BACNET_APPLICATION_TAG_NULL:
            break;
        case BACNET_APPLICATION_TAG_BOOLEAN:
            zassert_equal(test_value.type.Boolean, value->type.Boolean, NULL);
            break;
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            zassert_equal(
                test_value.type.Unsigned_Int, value->type.Unsigned_Int, NULL);
            break;
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            zassert_equal(
                test_value.type.Signed_Int, value->type.Signed_Int, NULL);
            break;
        case BACNET_APPLICATION_TAG_REAL:
            zassert_equal(test_value.type.Real, value->type.Real, NULL);
            break;
        case BACNET_APPLICATION_TAG_ENUMERATED:
            zassert_equal(
                test_value.type.Enumerated, value->type.Enumerated, NULL);
            break;
        case BACNET_APPLICATION_TAG_DATE:
            zassert_equal(test_value.type.Date.year, value->type.Date.year, NULL);
            zassert_equal(
                test_value.type.Date.month, value->type.Date.month, NULL);
            zassert_equal(test_value.type.Date.day, value->type.Date.day, NULL);
            zassert_equal(test_value.type.Date.wday, value->type.Date.wday, NULL);
            break;
        case BACNET_APPLICATION_TAG_TIME:
            zassert_equal(test_value.type.Time.hour, value->type.Time.hour, NULL);
            zassert_equal(test_value.type.Time.min, value->type.Time.min, NULL);
            zassert_equal(test_value.type.Time.sec, value->type.Time.sec, NULL);
            zassert_equal(
                test_value.type.Time.hundredths, value->type.Time.hundredths, NULL);
            break;
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            zassert_equal(
                test_value.type.Object_Id.type, value->type.Object_Id.type, NULL);
            zassert_equal(
                test_value.type.Object_Id.instance,
                    value->type.Object_Id.instance, NULL);
            break;
        default:
            break;
    }
}

static void testWriteProperty(void)
{
    BACNET_APPLICATION_DATA_VALUE value;

    value.tag = BACNET_APPLICATION_TAG_NULL;
    testWritePropertyTag(&value);

    value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    value.type.Boolean = true;
    testWritePropertyTag(&value);
    value.type.Boolean = false;
    testWritePropertyTag(&value);

    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = 0;
    testWritePropertyTag(&value);
    value.type.Unsigned_Int = 0xFFFF;
    testWritePropertyTag(&value);
    value.type.Unsigned_Int = 0xFFFFFFFF;
    testWritePropertyTag(&value);

    value.tag = BACNET_APPLICATION_TAG_SIGNED_INT;
    value.type.Signed_Int = 0;
    testWritePropertyTag(&value);
    value.type.Signed_Int = -1;
    testWritePropertyTag(&value);
    value.type.Signed_Int = 32768;
    testWritePropertyTag(&value);
    value.type.Signed_Int = -32768;
    testWritePropertyTag(&value);

    value.tag = BACNET_APPLICATION_TAG_REAL;
    value.type.Real = 0.0;
    testWritePropertyTag(&value);
    value.type.Real = -1.0;
    testWritePropertyTag(&value);
    value.type.Real = 1.0;
    testWritePropertyTag(&value);
    value.type.Real = 3.14159;
    testWritePropertyTag(&value);
    value.type.Real = -3.14159;
    testWritePropertyTag(&value);

    value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
    value.type.Enumerated = 0;
    testWritePropertyTag(&value);
    value.type.Enumerated = 0xFFFF;
    testWritePropertyTag(&value);
    value.type.Enumerated = 0xFFFFFFFF;
    testWritePropertyTag(&value);

    value.tag = BACNET_APPLICATION_TAG_DATE;
    value.type.Date.year = 2005;
    value.type.Date.month = 5;
    value.type.Date.day = 22;
    value.type.Date.wday = 1;
    testWritePropertyTag(&value);

    value.tag = BACNET_APPLICATION_TAG_TIME;
    value.type.Time.hour = 23;
    value.type.Time.min = 59;
    value.type.Time.sec = 59;
    value.type.Time.hundredths = 12;
    testWritePropertyTag(&value);

    value.tag = BACNET_APPLICATION_TAG_OBJECT_ID;
    value.type.Object_Id.type = OBJECT_ANALOG_INPUT;
    value.type.Object_Id.instance = 0;
    testWritePropertyTag(&value);
    value.type.Object_Id.type = OBJECT_LIFE_SAFETY_ZONE;
    value.type.Object_Id.instance = BACNET_MAX_INSTANCE;
    testWritePropertyTag(&value);

    return;
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(wp_tests,
     ztest_unit_test(testWriteProperty)
     );

    ztest_run_test_suite(wp_tests);
}
