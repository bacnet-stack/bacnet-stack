/**
 * @file
 * @brief Unit test for BACnetAccessRule encode and decode API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date September 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/authentication_factor_format.h>
#include <bacnet/bacdcode.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void test_authentication_factor_format_context(
    uint8_t tag, BACNET_AUTHENTICATION_FACTOR_FORMAT *data)
{
    BACNET_AUTHENTICATION_FACTOR_FORMAT test_data = { 0 };
    uint8_t apdu[MAX_APDU];
    int apdu_len;
    int test_len;
    int null_len;
    null_len =
        bacapp_encode_context_authentication_factor_format(NULL, tag, data);
    apdu_len =
        bacapp_encode_context_authentication_factor_format(apdu, tag, data);
    zassert_equal(null_len, apdu_len, NULL);
    test_len = bacnet_authentication_factor_format_context_decode(
        apdu, apdu_len, tag, &test_data);
    zassert_equal(
        test_len, apdu_len, "test_len=%d apdu_len=%d", test_len, apdu_len);
    zassert_equal(data->format_type, test_data.format_type, "format_type");
    if (data->format_type == AUTHENTICATION_FACTOR_CUSTOM) {
        zassert_equal(
            data->vendor_format, test_data.vendor_format, "vendor_format");
        zassert_equal(data->vendor_id, test_data.vendor_id, "vendor_id");
    }
    /* test the deprecated function */
    test_len = bacapp_decode_context_authentication_factor_format(
        apdu, tag, &test_data);
    zassert_equal(
        test_len, apdu_len, "test_len=%d apdu_len=%d", test_len, apdu_len);
    /* shrink the buffer, get some errors */
    while (apdu_len) {
        apdu_len--;
        test_len = bacnet_authentication_factor_format_context_decode(
            apdu, apdu_len, tag, &test_data);
        zassert_true(
            test_len < 0, "test_len=%d apdu_len=%d", test_len, apdu_len);
    }
}

static void test_authentication_factor_format_positive(
    BACNET_AUTHENTICATION_FACTOR_FORMAT *data)
{
    BACNET_AUTHENTICATION_FACTOR_FORMAT test_data = { 0 };
    uint8_t apdu[MAX_APDU];
    int apdu_len;
    int test_len;
    int null_len;

    null_len = bacapp_encode_authentication_factor_format(NULL, data);
    apdu_len = bacapp_encode_authentication_factor_format(apdu, data);
    zassert_equal(null_len, apdu_len, NULL);
    test_len =
        bacnet_authentication_factor_format_decode(apdu, apdu_len, &test_data);
    zassert_equal(
        test_len, apdu_len, "test_len=%d apdu_len=%d", test_len, apdu_len);
    zassert_equal(data->format_type, test_data.format_type, "format_type");
    if (data->format_type == AUTHENTICATION_FACTOR_CUSTOM) {
        zassert_equal(
            data->vendor_format, test_data.vendor_format, "vendor_format");
        zassert_equal(data->vendor_id, test_data.vendor_id, "vendor_id");
    }
    /* test the deprecated function */
    test_len = bacapp_decode_authentication_factor_format(apdu, &test_data);
    zassert_equal(
        test_len, apdu_len, "test_len=%d apdu_len=%d", test_len, apdu_len);
    /* shrink the buffer, get some errors */
    while (apdu_len) {
        apdu_len--;
        test_len = bacnet_authentication_factor_format_decode(
            apdu, apdu_len, &test_data);
        zassert_true(
            test_len < 0, "test_len=%d apdu_len=%d", test_len, apdu_len);
    }
}

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(authentication_factor_format_tests, test_authentication_factor_format)
#else
static void test_authentication_factor_format(void)
#endif
{
    BACNET_AUTHENTICATION_FACTOR_FORMAT data = { 0 };

    data.format_type = AUTHENTICATION_FACTOR_CUSTOM;
    data.vendor_id = 1;
    data.vendor_format = 2;
    test_authentication_factor_format_positive(&data);
    test_authentication_factor_format_context(1, &data);

    data.format_type = AUTHENTICATION_FACTOR_UNDEFINED;
    data.vendor_id = 1;
    data.vendor_format = 2;
    test_authentication_factor_format_positive(&data);
    test_authentication_factor_format_context(1, &data);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(authentication_factor_format_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        authentication_factor_format_tests,
        ztest_unit_test(test_authentication_factor_format));

    ztest_run_test_suite(authentication_factor_format_tests);
}
#endif
