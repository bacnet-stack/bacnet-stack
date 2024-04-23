/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2024
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/credential_data_input.h>
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
ZTEST(credential_data_input_tests, testCredentialDataInput)
#else
static void testCredentialDataInput(void)
#endif
{
    unsigned count = 0;
    uint32_t object_instance = 0;
    const int skip_fail_property_list[] = { 
        PROP_PRESENT_VALUE,
        PROP_UPDATE_TIME,
        PROP_SUPPORTED_FORMATS,
        -1 };

    Credential_Data_Input_Init();
    count = Credential_Data_Input_Count();
    zassert_true(count > 0, NULL);
    object_instance = Credential_Data_Input_Index_To_Instance(0);
    bacnet_object_properties_read_write_test(
        OBJECT_CREDENTIAL_DATA_INPUT,
        object_instance,
        Credential_Data_Input_Property_Lists,
        Credential_Data_Input_Read_Property,
        Credential_Data_Input_Write_Property,
        skip_fail_property_list);
}
/**
 * @}
 */


#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(credential_data_input_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(credential_data_input_tests,
     ztest_unit_test(testCredentialDataInput)
     );

    ztest_run_test_suite(credential_data_input_tests);
}
#endif
