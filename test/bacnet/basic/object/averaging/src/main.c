/**
 * @file
 * @brief Unit test for averaging object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2026
 * @section LICENSE
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <math.h>
#include <zephyr/ztest.h>
#include <bacnet/basic/object/averaging.h>
#include <property_test.h>

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(averaging_tests, testAveragingReadWrite)
#else
static void testAveragingReadWrite(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int32_t skip_fail_property_list[] = { -1 };

    Averaging_Init();
    object_instance = Averaging_Create(object_instance);
    count = Averaging_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Averaging_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_AVERAGING, object_instance, Averaging_Property_Lists,
        Averaging_Read_Property, Averaging_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Averaging_Name_Set, Averaging_Name_ASCII);
    status = Averaging_Delete(object_instance);
    zassert_true(status, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(averaging_tests, testAveragingSlidingWindow)
#else
static void testAveragingSlidingWindow(void)
#endif
{
    uint32_t object_instance = BACNET_MAX_INSTANCE;
    bool status = false;

    Averaging_Init();
    object_instance = Averaging_Create(object_instance);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    zassert_true(isinf(Averaging_Minimum_Value(object_instance)), NULL);
    zassert_true(isnan(Averaging_Average_Value(object_instance)), NULL);
    zassert_true(isnan(Averaging_Variance_Value(object_instance)), NULL);
    zassert_true(isinf(Averaging_Maximum_Value(object_instance)), NULL);

    status = Averaging_Sample_Add(object_instance, 1.0f);
    zassert_true(status, NULL);
    status = Averaging_Sample_Add(object_instance, 3.0f);
    zassert_true(status, NULL);
    status = Averaging_Sample_Add(object_instance, 5.0f);
    zassert_true(status, NULL);

    zassert_equal(Averaging_Attempted_Samples(object_instance), 3, NULL);
    zassert_equal(Averaging_Valid_Samples(object_instance), 3, NULL);
    zassert_true(
        fabsf(Averaging_Minimum_Value(object_instance) - 1.0f) < 0.001f, NULL);
    zassert_true(
        fabsf(Averaging_Maximum_Value(object_instance) - 5.0f) < 0.001f, NULL);
    zassert_true(
        fabsf(Averaging_Average_Value(object_instance) - 3.0f) < 0.001f, NULL);
    zassert_true(
        fabsf(Averaging_Variance_Value(object_instance) - (8.0f / 3.0f)) <
            0.001f,
        NULL);

    status = Averaging_Sample_Miss(object_instance);
    zassert_true(status, NULL);
    zassert_equal(Averaging_Attempted_Samples(object_instance), 4, NULL);
    zassert_equal(Averaging_Valid_Samples(object_instance), 3, NULL);

    status = Averaging_Delete(object_instance);
    zassert_true(status, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(averaging_tests, testAveragingResetAndWrite)
#else
static void testAveragingResetAndWrite(void)
#endif
{
    uint32_t object_instance = BACNET_MAX_INSTANCE;
    bool status = false;
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE ref = { 0 };

    Averaging_Init();
    object_instance = Averaging_Create(object_instance);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    status = Averaging_Sample_Add(object_instance, 11.0f);
    zassert_true(status, NULL);
    status = Averaging_Sample_Add(object_instance, 22.0f);
    zassert_true(status, NULL);
    zassert_equal(Averaging_Attempted_Samples(object_instance), 2, NULL);

    wp_data.object_type = OBJECT_AVERAGING;
    wp_data.object_instance = object_instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;

    wp_data.object_property = PROP_ATTEMPTED_SAMPLES;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 0);
    status = Averaging_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_equal(Averaging_Attempted_Samples(object_instance), 0, NULL);
    zassert_equal(Averaging_Valid_Samples(object_instance), 0, NULL);
    zassert_true(isinf(Averaging_Minimum_Value(object_instance)), NULL);
    zassert_true(isnan(Averaging_Average_Value(object_instance)), NULL);

    wp_data.object_property = PROP_ATTEMPTED_SAMPLES;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 1);
    status = Averaging_Write_Property(&wp_data);
    zassert_false(status, NULL);

    wp_data.object_property = PROP_WINDOW_SAMPLES;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 3);
    status = Averaging_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_equal(Averaging_Window_Samples(object_instance), 3, NULL);

    ref.objectIdentifier.type = OBJECT_ANALOG_VALUE;
    ref.objectIdentifier.instance = 123;
    ref.propertyIdentifier = PROP_PRESENT_VALUE;
    ref.arrayIndex = BACNET_ARRAY_ALL;
    ref.deviceIdentifier.type = BACNET_NO_DEV_TYPE;
    ref.deviceIdentifier.instance = BACNET_NO_DEV_ID;

    wp_data.object_property = PROP_OBJECT_PROPERTY_REFERENCE;
    wp_data.application_data_len =
        bacapp_encode_device_obj_property_ref(wp_data.application_data, &ref);
    status = Averaging_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_true(
        Averaging_Object_Property_Reference(object_instance, &ref), NULL);
    zassert_equal(ref.objectIdentifier.instance, 123, NULL);

    status = Averaging_Delete(object_instance);
    zassert_true(status, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(averaging_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        averaging_tests, ztest_unit_test(testAveragingReadWrite),
        ztest_unit_test(testAveragingSlidingWindow),
        ztest_unit_test(testAveragingResetAndWrite));

    ztest_run_test_suite(averaging_tests);
}
#endif
