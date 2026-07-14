/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/command.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_command, test_object_command)
#else
static void test_object_command(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE;
    const int32_t skip_fail_property_list[] = { -1 };
    BACNET_ACTION_LIST *pAction;

    Command_Cleanup();
    object_instance = Command_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);
    count = Command_Count();
    zassert_true(count > 0, NULL);
    status = Command_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    count = Command_Action_List_Count(object_instance);
    zassert_true(count > 0, NULL);
    /* configure the instance property values */
    pAction = Command_Action_List_Entry(object_instance, 0);
    zassert_not_null(pAction, NULL);
    pAction->Device_Id.type = OBJECT_DEVICE;
    pAction->Device_Id.instance = 4194303;
    pAction->Object_Id.type = OBJECT_ANALOG_INPUT;
    pAction->Object_Id.instance = 4194303;
    pAction->Property_Identifier = PROP_PRESENT_VALUE;
    pAction->Property_Array_Index = BACNET_ARRAY_ALL;
    pAction->Priority = 16;
    pAction->Value.tag = BACNET_APPLICATION_TAG_REAL;
    pAction->Value.type.Real = 3.14159f;
    pAction->Post_Delay = 0;
    pAction->Quit_On_Failure = false;
    pAction->Write_Successful = false;
    pAction->next = NULL;
    Command_In_Process_Set(object_instance, false);
    Command_All_Writes_Successful_Set(object_instance, false);
    /* perform a general test for RP/WP */
    bacnet_object_properties_read_write_test(
        OBJECT_COMMAND, object_instance, Command_Property_Lists,
        Command_Read_Property, Command_Write_Property, skip_fail_property_list);

    status = Command_Delete(object_instance);
    zassert_true(status, NULL);
    Command_Cleanup();
}

/**
 * @brief Test dynamic object and action list lifecycle APIs
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_command, test_object_command_dynamic)
#else
static void test_object_command_dynamic(void)
#endif
{
    uint32_t object_instance = BACNET_MAX_INSTANCE;
    bool status;
    unsigned count;
    unsigned i;
    BACNET_ACTION_LIST *pAction;

    Command_Cleanup();
    zassert_equal(Command_Count(), 0, NULL);

    object_instance = Command_Create(BACNET_MAX_INSTANCE + 1);
    zassert_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    object_instance = Command_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);
    zassert_true(Command_Valid_Instance(object_instance), NULL);
    zassert_equal(Command_Count(), 1, NULL);

    count = Command_Action_List_Count(object_instance);
    zassert_equal(count, MAX_COMMAND_ACTIONS, NULL);
    for (i = 0; i < count; i++) {
        pAction = Command_Action_List_Entry(object_instance, i);
        zassert_not_null(pAction, NULL);
        pAction->Priority = i + 1;
    }

    status = Command_Delete(object_instance);
    zassert_true(status, NULL);
    zassert_false(Command_Valid_Instance(object_instance), NULL);
    zassert_equal(Command_Count(), 0, NULL);

    status = Command_Delete(object_instance);
    zassert_false(status, NULL);
    Command_Cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(tests_object_command, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        tests_object_command, ztest_unit_test(test_object_command),
        ztest_unit_test(test_object_command_dynamic));

    ztest_run_test_suite(tests_object_command);
}
#endif
