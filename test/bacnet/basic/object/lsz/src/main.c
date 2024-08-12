/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/lsz.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(testsLifeSafetyZone, testLifeSafetyZone)
#else
static void testLifeSafetyZone(void)
#endif
{
    unsigned count = 0;
    uint32_t object_instance = 0, test_object_instance = 0;
    const int skip_fail_property_list[] = { -1 };

    Life_Safety_Zone_Init();
    object_instance = Life_Safety_Zone_Create(BACNET_MAX_INSTANCE);
    count = Life_Safety_Zone_Count();
    zassert_true(count > 0, NULL);
    test_object_instance = Life_Safety_Zone_Index_To_Instance(0);
    zassert_equal(test_object_instance, object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_LIFE_SAFETY_ZONE, object_instance,
        Life_Safety_Zone_Property_Lists, Life_Safety_Zone_Read_Property,
        Life_Safety_Zone_Write_Property, skip_fail_property_list);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(testsLifeSafetyZone, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(testsLifeSafetyZone, ztest_unit_test(testLifeSafetyZone));

    ztest_run_test_suite(testsLifeSafetyZone);
}
#endif
