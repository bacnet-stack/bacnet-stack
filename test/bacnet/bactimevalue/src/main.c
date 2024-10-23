/**
 * @file
 * @brief Unit test for BACnetTimeValue
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Greg Shue <greg.shue@outlook.com>
 * @date June 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <zephyr/ztest.h>
#include "bacnet/bactimevalue.h"
#include "bacnet/datetime.h"
#include "bacnet/bacapp.h"

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test encode/decode API
 */
static void test_BACnetTimeValue(BACNET_TIME_VALUE *value)
{
    int len, apdu_len, null_len = 0;
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_TIME_VALUE test_value = { 0 };
    int diff = 0;
    bool status = false;
    uint8_t tag_number = 0;

    null_len = bacnet_time_value_encode(NULL, value);
    len = bacnet_time_value_encode(apdu, value);
    zassert_equal(len, null_len, NULL);
    apdu_len = bacnet_time_value_decode(apdu, len, &test_value);
    zassert_true(len > 0, NULL);
    zassert_true(apdu_len > 0, NULL);
    diff = datetime_compare_time(&test_value.Time, &value->Time);
    zassert_true(diff == 0, NULL);
    status = bacapp_same_value(
        (BACNET_APPLICATION_DATA_VALUE *)&test_value.Value,
        (BACNET_APPLICATION_DATA_VALUE *)&value->Value);
    zassert_true(status, NULL);
    /* apdu too short testing */
    while (--apdu_len) {
        len = bacnet_time_value_decode(apdu, apdu_len, &test_value);
        zassert_true(len < 0, NULL);
    }

    len = bacnet_time_value_context_encode(apdu, tag_number, value);
    null_len = bacnet_time_value_context_encode(NULL, tag_number, value);
    zassert_equal(len, null_len, NULL);
    apdu_len =
        bacnet_time_value_context_decode(apdu, len, tag_number, &test_value);
    zassert_true(len > 0, NULL);
    zassert_true(apdu_len > 0, NULL);
    diff = datetime_compare_time(&test_value.Time, &value->Time);
    zassert_true(diff == 0, NULL);
    status = bacapp_same_value(
        (BACNET_APPLICATION_DATA_VALUE *)&test_value.Value,
        (BACNET_APPLICATION_DATA_VALUE *)&value->Value);
    zassert_true(status, NULL);
    /* apdu too short testing */
    while (--apdu_len) {
        len = bacnet_time_value_context_decode(
            apdu, apdu_len, tag_number, &test_value);
        zassert_true(len < 0, NULL);
    }
    /* negative testing */
    tag_number++;
    apdu_len =
        bacnet_time_value_context_decode(apdu, len, tag_number, &test_value);
    zassert_true(apdu_len < 0, NULL);
}

/**
 * @brief Test encode/decode API
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetTimeValue_tests, test_BACnetTimeValues)
#else
static void test_BACnetTimeValues(void)
#endif
{
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_TIME_VALUE time_value = { 0 };
    int rc = BACNET_STATUS_OK;
    bool status = false;

    test_BACnetTimeValue(&time_value);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_BOOLEAN, "active", &value);
    zassert_true(status, NULL);
    rc = bacnet_application_to_primitive_data_value(&time_value.Value, &value);
    zassert_equal(rc, BACNET_STATUS_OK, NULL);
    rc = bacnet_primitive_to_application_data_value(&value, &time_value.Value);
    zassert_equal(rc, BACNET_STATUS_OK, NULL);
    datetime_time_init_ascii(&time_value.Time, "00:00.01");
    test_BACnetTimeValue(&time_value);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_UNSIGNED_INT, "99999", &value);
    zassert_true(status, NULL);
    rc = bacnet_application_to_primitive_data_value(&time_value.Value, &value);
    zassert_equal(rc, BACNET_STATUS_OK, NULL);
    rc = bacnet_primitive_to_application_data_value(&value, &time_value.Value);
    zassert_equal(rc, BACNET_STATUS_OK, NULL);
    status = datetime_time_init_ascii(&time_value.Time, "23:59:59");
    zassert_true(status, NULL);
    test_BACnetTimeValue(&time_value);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_SIGNED_INT, "-42", &value);
    zassert_true(status, NULL);
    rc = bacnet_application_to_primitive_data_value(&time_value.Value, &value);
    zassert_equal(rc, BACNET_STATUS_OK, NULL);
    rc = bacnet_primitive_to_application_data_value(&value, &time_value.Value);
    zassert_equal(rc, BACNET_STATUS_OK, NULL);
    status = datetime_time_init_ascii(&time_value.Time, "13:00:59.99");
    zassert_true(status, NULL);
    test_BACnetTimeValue(&time_value);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_REAL, "4.2", &value);
    zassert_true(status, NULL);
    rc = bacnet_application_to_primitive_data_value(&time_value.Value, &value);
    zassert_equal(rc, BACNET_STATUS_OK, NULL);
    rc = bacnet_primitive_to_application_data_value(&value, &time_value.Value);
    zassert_equal(rc, BACNET_STATUS_OK, NULL);
    status = datetime_time_init_ascii(&time_value.Time, "12:00");
    zassert_true(status, NULL);
    test_BACnetTimeValue(&time_value);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_DOUBLE, "3.141593", &value);
    zassert_true(status, NULL);
    rc = bacnet_application_to_primitive_data_value(&time_value.Value, &value);
    zassert_equal(rc, BACNET_STATUS_OK, NULL);
    rc = bacnet_primitive_to_application_data_value(&value, &time_value.Value);
    zassert_equal(rc, BACNET_STATUS_OK, NULL);
    status = datetime_time_init_ascii(&time_value.Time, "3:14.15.93");
    zassert_true(status, NULL);
    test_BACnetTimeValue(&time_value);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_ENUMERATED, "42", &value);
    zassert_true(status, NULL);
    rc = bacnet_application_to_primitive_data_value(&time_value.Value, &value);
    zassert_equal(rc, BACNET_STATUS_OK, NULL);
    rc = bacnet_primitive_to_application_data_value(&value, &time_value.Value);
    zassert_equal(rc, BACNET_STATUS_OK, NULL);
    status = datetime_time_init_ascii(&time_value.Time, "8:00.00.00");
    zassert_true(status, NULL);
    test_BACnetTimeValue(&time_value);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_OBJECT_ID, "8:4194303", &value);
    zassert_true(status, NULL);
    rc = bacnet_application_to_primitive_data_value(&time_value.Value, &value);
    zassert_equal(rc, BACNET_STATUS_ERROR, "rc=%d", rc);
    rc = bacnet_application_to_primitive_data_value(NULL, &value);
    zassert_equal(rc, BACNET_STATUS_ERROR, "rc=%d", rc);
    rc = bacnet_application_to_primitive_data_value(&time_value.Value, NULL);
    zassert_equal(rc, BACNET_STATUS_ERROR, "rc=%d", rc);
    rc = bacnet_primitive_to_application_data_value(NULL, &time_value.Value);
    zassert_equal(rc, BACNET_STATUS_ERROR, NULL);
    rc = bacnet_primitive_to_application_data_value(&value, NULL);
    zassert_equal(rc, BACNET_STATUS_ERROR, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(BACnetTimeValue_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        BACnetTimeValue_tests, ztest_unit_test(test_BACnetTimeValues));

    ztest_run_test_suite(BACnetTimeValue_tests);
}
#endif
