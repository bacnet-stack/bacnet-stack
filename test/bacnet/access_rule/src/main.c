/**
 * @file
 * @brief Unit test for BACnetAccessRule encode and decode API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date September 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/access_rule.h>
#include <bacnet/bacdcode.h>
#include <bacnet/bacdevobjpropref.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void test_access_rule_positive(BACNET_ACCESS_RULE *data)
{
    BACNET_ACCESS_RULE test_data = { 0 };
    uint8_t apdu[MAX_APDU];
    int len;
    int test_len;
    int null_len;
    bool status;

    null_len = bacapp_encode_access_rule(NULL, data);
    len = bacapp_encode_access_rule(apdu, data);
    zassert_equal(null_len, len, NULL);
    test_len = bacnet_access_rule_decode(apdu, len, &test_data);
    zassert_equal(test_len, len, NULL);
    zassert_equal(
        data->time_range_specifier, test_data.time_range_specifier, NULL);
    zassert_equal(data->location_specifier, test_data.location_specifier, NULL);
    zassert_equal(data->enable, test_data.enable, NULL);
    if (data->time_range_specifier == TIME_RANGE_SPECIFIER_SPECIFIED) {
        status = bacnet_device_object_property_reference_same(
            &data->time_range, &test_data.time_range);
        zassert_true(status, NULL);
    }
    if (data->location_specifier == LOCATION_SPECIFIER_SPECIFIED) {
        status = bacnet_device_object_reference_same(
            &data->location, &test_data.location);
        zassert_true(status, NULL);
    }
    while (--len) {
        test_len = bacnet_access_rule_decode(apdu, len, &test_data);
        zassert_true(test_len <= 0, NULL);
    }
}

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(access_rule_tests, test_access_rule)
#else
static void test_access_rule(void)
#endif
{
    BACNET_ACCESS_RULE data = { 0 };

    data.enable = true;
    data.time_range_specifier = TIME_RANGE_SPECIFIER_ALWAYS;
    data.location_specifier = LOCATION_SPECIFIER_ALL;
    test_access_rule_positive(&data);

    data.enable = false;
    data.time_range_specifier = TIME_RANGE_SPECIFIER_ALWAYS;
    data.location_specifier = LOCATION_SPECIFIER_ALL;
    test_access_rule_positive(&data);

    data.enable = true;
    data.time_range_specifier = TIME_RANGE_SPECIFIER_SPECIFIED;
    data.time_range.arrayIndex = 1;
    data.time_range.objectIdentifier.type = OBJECT_ANALOG_INPUT;
    data.time_range.objectIdentifier.instance = 1;
    data.time_range.propertyIdentifier = PROP_PRESENT_VALUE;
    data.time_range.deviceIdentifier.type = OBJECT_DEVICE;
    data.time_range.deviceIdentifier.instance = 1;
    data.location_specifier = LOCATION_SPECIFIER_ALL;
    test_access_rule_positive(&data);

    data.enable = true;
    data.time_range_specifier = TIME_RANGE_SPECIFIER_ALWAYS;
    data.location_specifier = LOCATION_SPECIFIER_SPECIFIED;
    data.location.objectIdentifier.type = OBJECT_ANALOG_INPUT;
    data.location.objectIdentifier.instance = 1;
    data.location.deviceIdentifier.type = OBJECT_DEVICE;
    data.location.deviceIdentifier.instance = 1;
    test_access_rule_positive(&data);

    data.enable = true;
    data.time_range_specifier = TIME_RANGE_SPECIFIER_SPECIFIED;
    data.time_range.arrayIndex = 1;
    data.time_range.objectIdentifier.type = OBJECT_ANALOG_INPUT;
    data.time_range.objectIdentifier.instance = 1;
    data.time_range.propertyIdentifier = PROP_PRESENT_VALUE;
    data.time_range.deviceIdentifier.type = OBJECT_DEVICE;
    data.time_range.deviceIdentifier.instance = 1;
    data.location_specifier = LOCATION_SPECIFIER_SPECIFIED;
    data.location.objectIdentifier.type = OBJECT_ANALOG_INPUT;
    data.location.objectIdentifier.instance = 1;
    data.location.deviceIdentifier.type = OBJECT_DEVICE;
    data.location.deviceIdentifier.instance = 1;
    test_access_rule_positive(&data);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(access_rule_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(access_rule_tests, ztest_unit_test(test_access_rule));

    ztest_run_test_suite(access_rule_tests);
}
#endif
