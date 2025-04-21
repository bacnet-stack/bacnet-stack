/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */
#include <math.h>
#include <zephyr/ztest.h>
#include <bacnet/wp.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Decode stub for WriteProperty service
 * @return number of bytes decoded, or #BACNET_STATUS_ERROR
 */
static int wp_decode_apdu(
    const uint8_t *apdu,
    unsigned apdu_size,
    uint8_t *invoke_id,
    BACNET_WRITE_PROPERTY_DATA *wpdata)
{
    int len = 0;
    int apdu_len = 0;

    if (!apdu) {
        return -1;
    }
    if (apdu_size < 4) {
        return BACNET_STATUS_ERROR;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST) {
        return BACNET_STATUS_ERROR;
    }
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    if (invoke_id) {
        *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
    }
    if (apdu[3] != SERVICE_CONFIRMED_WRITE_PROPERTY) {
        return BACNET_STATUS_ERROR;
    }
    len = 4;
    apdu_len += len;
    if (apdu_len < apdu_size) {
        len = wp_decode_service_request(
            &apdu[apdu_len], apdu_size - apdu_len, wpdata);
        if (len > 0) {
            apdu_len += len;
        } else {
            apdu_len = len;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

static BACNET_UNSIGNED_INTEGER Test_Unsigned_Value;
static uint32_t Test_Object_Instance;
/**
 * @brief API for setting a BACnet Unsigned Integer property value
 * @param object_instance [in] Object instance number
 * @param value [in] New value to set
 * @return true if successful, else false
 */
static bool test_bacnet_property_unsigned_set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    Test_Object_Instance = object_instance;
    Test_Unsigned_Value = value;
    return true;
}

static void testWritePropertyTag(const BACNET_APPLICATION_DATA_VALUE *value)
{
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    BACNET_WRITE_PROPERTY_DATA test_data = { 0 };
    BACNET_APPLICATION_DATA_VALUE test_value = { 0 };
    BACNET_UNSIGNED_INTEGER test_unsigned_value_max = 0;
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int null_len = 0;
    int apdu_len = 0;
    uint8_t invoke_id = 128;
    uint8_t test_invoke_id = 0;
    bool status = false;

    wpdata.application_data_len =
        bacapp_encode_application_data(&wpdata.application_data[0], value);
    null_len = wp_encode_apdu(NULL, invoke_id, &wpdata);
    len = wp_encode_apdu(&apdu[0], invoke_id, &wpdata);
    zassert_equal(null_len, len, "null_len=%d len=%d", null_len, len);
    zassert_not_equal(len, 0, "len=%d", len);
    /* decode the data */
    apdu_len = len;
    null_len = wp_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, NULL);
    len = wp_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
    zassert_equal(null_len, len, "null_len=%d len=%d", null_len, len);
    zassert_true(len > 0, "len=%d", len);
    zassert_equal(test_data.object_type, wpdata.object_type, NULL);
    zassert_equal(test_data.object_instance, wpdata.object_instance, NULL);
    zassert_equal(test_data.object_property, wpdata.object_property, NULL);
    zassert_equal(test_data.array_index, wpdata.array_index, NULL);
    /* test the OPTIONAL property-array-index */
    wpdata.array_index = BACNET_ARRAY_ALL;
    len = wp_encode_apdu(&apdu[0], invoke_id, &wpdata);
    apdu_len = wp_decode_apdu(&apdu[0], len, &test_invoke_id, &test_data);
    zassert_equal(apdu_len, len, "apdu_len=%d len=%d", apdu_len, len);
    zassert_true(len > 0, "len=%d", len);
    wpdata.array_index = 0;
    /* test the OPTIONAL priority */
    wpdata.priority = BACNET_MAX_PRIORITY;
    len = wp_encode_apdu(&apdu[0], invoke_id, &wpdata);
    apdu_len = wp_decode_apdu(&apdu[0], len, &test_invoke_id, &test_data);
    zassert_equal(apdu_len, len, "apdu_len=%d len=%d", apdu_len, len);
    zassert_true(len > 0, "len=%d", len);
    wpdata.priority = 0;
    /* decode the application value of the request */
    len = wp_encode_apdu(&apdu[0], invoke_id, &wpdata);
    apdu_len = wp_decode_apdu(&apdu[0], len, &test_invoke_id, &test_data);
    apdu_len = len;
    len = bacapp_decode_application_data(
        test_data.application_data, test_data.application_data_len,
        &test_value);
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
            test_unsigned_value_max = test_value.type.Unsigned_Int;
            status = write_property_unsigned_decode(
                &test_data, &test_value, test_bacnet_property_unsigned_set,
                test_unsigned_value_max);
            zassert_equal(status, true, NULL);
            zassert_equal(
                Test_Object_Instance, test_data.object_instance, NULL);
            zassert_equal(Test_Unsigned_Value, value->type.Unsigned_Int, NULL);
            if (test_value.type.Unsigned_Int != 0) {
                test_unsigned_value_max = 0;
                status = write_property_unsigned_decode(
                    &test_data, &test_value, test_bacnet_property_unsigned_set,
                    test_unsigned_value_max);
                zassert_equal(status, false, NULL);
            }
            break;
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            zassert_equal(
                test_value.type.Signed_Int, value->type.Signed_Int, NULL);
            break;
        case BACNET_APPLICATION_TAG_REAL:
            zassert_false(
                islessgreater(test_value.type.Real, value->type.Real), NULL);
            break;
        case BACNET_APPLICATION_TAG_ENUMERATED:
            zassert_equal(
                test_value.type.Enumerated, value->type.Enumerated, NULL);
            break;
        case BACNET_APPLICATION_TAG_DATE:
            zassert_equal(
                test_value.type.Date.year, value->type.Date.year, NULL);
            zassert_equal(
                test_value.type.Date.month, value->type.Date.month, NULL);
            zassert_equal(test_value.type.Date.day, value->type.Date.day, NULL);
            zassert_equal(
                test_value.type.Date.wday, value->type.Date.wday, NULL);
            break;
        case BACNET_APPLICATION_TAG_TIME:
            zassert_equal(
                test_value.type.Time.hour, value->type.Time.hour, NULL);
            zassert_equal(test_value.type.Time.min, value->type.Time.min, NULL);
            zassert_equal(test_value.type.Time.sec, value->type.Time.sec, NULL);
            zassert_equal(
                test_value.type.Time.hundredths, value->type.Time.hundredths,
                NULL);
            break;
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            zassert_equal(
                test_value.type.Object_Id.type, value->type.Object_Id.type,
                NULL);
            zassert_equal(
                test_value.type.Object_Id.instance,
                value->type.Object_Id.instance, NULL);
            break;
        default:
            break;
    }
    /* test packets that are too short */
    while (apdu_len) {
        apdu_len--;
        len = wp_decode_apdu(&apdu[0], apdu_len, &test_invoke_id, &test_data);
        zassert_true(len <= 0, "len=%d tag=%d", len, value->tag);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(wp_tests, testWriteProperty)
#else
static void testWriteProperty(void)
#endif
{
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

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
    value.type.Real = 3.14159f;
    testWritePropertyTag(&value);
    value.type.Real = -3.14159f;
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
}

static bool Is_Property_Member;
bool test_write_property_member_of_object(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property)
{
    (void)object_type;
    (void)object_instance;
    (void)object_property;
    return Is_Property_Member;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(wp_tests, testWritePropertyNull)
#else
static void testWritePropertyNull(void)
#endif
{
    bool bypass = false;
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };

    bypass = write_property_relinquish_bypass(NULL, NULL);
    zassert_equal(bypass, false, NULL);
    wp_data.object_type = OBJECT_ANALOG_OUTPUT;
    wp_data.object_instance = 0;
    wp_data.object_property = PROP_PRESENT_VALUE;
    wp_data.application_data_len =
        encode_application_null(&wp_data.application_data[0]);
    Is_Property_Member = true;
    bypass = write_property_relinquish_bypass(
        &wp_data, test_write_property_member_of_object);
    zassert_equal(bypass, false, NULL);
    Is_Property_Member = false;
    bypass = write_property_relinquish_bypass(
        &wp_data, test_write_property_member_of_object);
    zassert_equal(bypass, true, NULL);
    wp_data.object_property = PROP_OUT_OF_SERVICE;
    Is_Property_Member = true;
    bypass = write_property_relinquish_bypass(
        &wp_data, test_write_property_member_of_object);
    zassert_equal(bypass, true, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(wp_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        wp_tests, ztest_unit_test(testWriteProperty),
        ztest_unit_test(testWritePropertyNull));
    ztest_run_test_suite(wp_tests);
}
#endif
