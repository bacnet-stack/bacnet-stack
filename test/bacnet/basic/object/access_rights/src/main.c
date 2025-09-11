/**
 * @file
 * @brief test BACnet Access Rights object APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/access_rights.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(access_rights_tests, testAccessRights)
#else
static void testAccessRights(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = 0;
    const int skip_fail_property_list[] = { -1 };

    Access_Rights_Init();
    count = Access_Rights_Count();
    zassert_true(count > 0, NULL);
    object_instance = Access_Rights_Index_To_Instance(0);
    status = Access_Rights_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    /* perform a general test for RP/WP */
    bacnet_object_properties_read_write_test(
        OBJECT_ACCESS_RIGHTS, object_instance, Access_Rights_Property_Lists,
        Access_Rights_Read_Property, Access_Rights_Write_Property,
        skip_fail_property_list);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(access_rights_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(access_rights_tests, ztest_unit_test(testAccessRights));

    ztest_run_test_suite(access_rights_tests);
}
#endif
