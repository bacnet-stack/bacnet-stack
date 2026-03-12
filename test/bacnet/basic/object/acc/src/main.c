/**
 * @file
 * @brief test BACnet Accumulator object APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2017
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/acc.h>
#include <bacnet/bactext.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(acc_tests, test_Accumulator)
#else
static void test_Accumulator(void)
#endif
{
    const uint32_t instance = 123;
    unsigned count = 0;
    unsigned index = 0;
    int len = 0;
    int test_len = 0;
    uint32_t test_instance = 0;
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_UNSIGNED_INTEGER unsigned_value = 1;
    bool status = false;
    const int32_t *writable_properties;
    const int32_t skip_fail_property_list[] = { -1 };
    char *sample_context = "context";

    Accumulator_Init();
    test_instance = Accumulator_Create(BACNET_MAX_INSTANCE + 1);
    zassert_equal(test_instance, BACNET_MAX_INSTANCE, NULL);
    test_instance = Accumulator_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(test_instance, BACNET_MAX_INSTANCE, NULL);
    status = Accumulator_Delete(test_instance);
    zassert_true(status, NULL);
    test_instance = Accumulator_Create(instance);
    zassert_equal(test_instance, instance, NULL);
    status = Accumulator_Valid_Instance(instance);
    zassert_true(status, NULL);
    status = Accumulator_Valid_Instance(instance - 1);
    zassert_false(status, NULL);
    index = Accumulator_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);
    test_instance = Accumulator_Index_To_Instance(index);
    zassert_equal(instance, test_instance, NULL);
    count = Accumulator_Count();
    zassert_true(count > 0, NULL);
    /* perform a general test for RP/WP */
    bacnet_object_properties_read_write_test(
        OBJECT_ACCUMULATOR, instance, Accumulator_Property_Lists,
        Accumulator_Read_Property, Accumulator_Write_Property,
        skip_fail_property_list);
    /* test 1-bit to 64-bit encode/decode of present-value */
    rpdata.object_type = OBJECT_ACCUMULATOR;
    rpdata.object_instance = instance;
    rpdata.object_property = PROP_PRESENT_VALUE;
    rpdata.array_index = BACNET_ARRAY_ALL;
    rpdata.error_class = ERROR_CLASS_PROPERTY;
    rpdata.error_code = ERROR_CODE_SUCCESS;
    rpdata.application_data = apdu;
    rpdata.application_data_len = sizeof(apdu);
    while (unsigned_value != BACNET_UNSIGNED_INTEGER_MAX) {
        Accumulator_Present_Value_Set(instance, unsigned_value);
        len = Accumulator_Read_Property(&rpdata);
        zassert_not_equal(len, 0, NULL);
        test_len = bacapp_decode_application_data(
            rpdata.application_data, len, &value);
        zassert_equal(len, test_len, NULL);
        zassert_equal(rpdata.error_code, ERROR_CODE_SUCCESS, NULL);
        zassert_equal(value.tag, BACNET_APPLICATION_TAG_UNSIGNED_INT, NULL);
        zassert_equal(value.type.Unsigned_Int, unsigned_value, NULL);
        unsigned_value |= (unsigned_value << 1);
    }
    status = Accumulator_Description_Set(instance, "Test Accumulator");
    zassert_true(status, NULL);
    Accumulator_Writable_Property_List(instance, &writable_properties);
    zassert_not_null(writable_properties, NULL);
    /* context API */
    Accumulator_Context_Set(instance, sample_context);
    zassert_true(sample_context == Accumulator_Context_Get(instance), NULL);
    zassert_true(NULL == Accumulator_Context_Get(instance + 1), NULL);
    Accumulator_Delete(instance);
    status = Accumulator_Valid_Instance(instance);
    zassert_false(status, NULL);

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(acc_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(acc_tests, ztest_unit_test(test_Accumulator));

    ztest_run_test_suite(acc_tests);
}
#endif
