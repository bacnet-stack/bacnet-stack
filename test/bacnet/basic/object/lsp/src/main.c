/**
 * @file
 * @brief test BACnet Life Safety Point object APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/lsp.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(lsp_tests, testLifeSafetyPoint)
#else
static void testLifeSafetyPoint(void)
#endif
{
    BACNET_OBJECT_TYPE object_type = OBJECT_LIFE_SAFETY_POINT;
    const int32_t skip_fail_property_list[] = { -1 };
    const uint32_t instance = 123;
    bool status = false;
    unsigned index;
    const char *test_name = NULL;
    const char *test_description = NULL;
    char *sample_name = "sample";
    char *sample_description = "description";

    Life_Safety_Point_Init();
    Life_Safety_Point_Create(instance);
    status = Life_Safety_Point_Valid_Instance(instance);
    zassert_true(status, NULL);
    index = Life_Safety_Point_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);

    bacnet_object_properties_read_write_test(
        object_type, instance, Life_Safety_Point_Property_Lists,
        Life_Safety_Point_Read_Property, Life_Safety_Point_Write_Property,
        skip_fail_property_list);
    /* test the ASCII name get/set */
    status = Life_Safety_Point_Name_Set(instance, sample_name);
    zassert_true(status, NULL);
    test_name = Life_Safety_Point_Name_ASCII(instance);
    zassert_equal(test_name, sample_name, NULL);
    status = Life_Safety_Point_Name_Set(instance, NULL);
    zassert_true(status, NULL);
    test_name = Life_Safety_Point_Name_ASCII(instance);
    zassert_equal(test_name, NULL, NULL);
    status = Life_Safety_Point_Description_Set(instance, sample_description);
    zassert_true(status, NULL);
    test_description = Life_Safety_Point_Description(instance);
    zassert_equal(test_description, sample_description, NULL);
    status = Life_Safety_Point_Description_Set(instance, NULL);
    zassert_true(status, NULL);
    zassert_true(
        strcmp(Life_Safety_Point_Description(instance), "") == 0, NULL);
    bacnet_object_name_ascii_duplicate_test(
        instance, Life_Safety_Point_Name_Duplicate,
        Life_Safety_Point_Name_ASCII);
    bacnet_object_name_ascii_duplicate_test(
        instance, Life_Safety_Point_Description_Duplicate,
        Life_Safety_Point_Description);
    /* cleanup */
    status = Life_Safety_Point_Delete(instance);
    zassert_true(status, NULL);

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(lsp_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(lsp_tests, ztest_unit_test(testLifeSafetyPoint));

    ztest_run_test_suite(lsp_tests);
}
#endif
