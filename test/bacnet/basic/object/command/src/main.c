/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacapp.h>
#include <bacnet/bacstr.h>
#include <bacnet/bacdcode.h>
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
 * @brief Test writable PROP_ACTION array resize and element read/write.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_command, test_object_command_action_array_write)
#else
static void test_object_command_action_array_write(void)
#endif
{
    uint32_t object_instance = BACNET_MAX_INSTANCE;
    bool status = false;
    int len = 0;
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    BACNET_READ_PROPERTY_DATA rp_data = { 0 };
    BACNET_ACTION_LIST action_expected = { 0 };
    BACNET_ACTION_LIST action_decoded = { 0 };
    BACNET_ACTION_LIST *pAction = NULL;
    uint8_t apdu[MAX_APDU] = { 0 };

    Command_Cleanup();
    object_instance = Command_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    wp_data.object_type = OBJECT_COMMAND;
    wp_data.object_instance = object_instance;
    wp_data.object_property = PROP_ACTION;
    wp_data.priority = BACNET_NO_PRIORITY;

    /* Resize Action array by writing array index 0. */
    wp_data.array_index = 0;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 2);
    status = Command_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_equal(Command_Action_List_Count(object_instance), 2, NULL);

    /* Write first action element (array index 1). */
    action_expected.Device_Id.type = OBJECT_DEVICE;
    action_expected.Device_Id.instance = 1234;
    action_expected.Object_Id.type = OBJECT_ANALOG_VALUE;
    action_expected.Object_Id.instance = 7;
    action_expected.Property_Identifier = PROP_PRESENT_VALUE;
    action_expected.Property_Array_Index = BACNET_ARRAY_ALL;
    action_expected.Value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    action_expected.Value.type.Unsigned_Int = 42;
    action_expected.Priority = 8;
    action_expected.Post_Delay = 10;
    action_expected.Quit_On_Failure = true;
    action_expected.Write_Successful = false;

    wp_data.array_index = 1;
    wp_data.application_data_len = bacnet_action_command_encode(
        wp_data.application_data, &action_expected);
    status = Command_Write_Property(&wp_data);
    zassert_true(status, NULL);

    pAction = Command_Action_List_Entry(object_instance, 0);
    zassert_not_null(pAction, NULL);
    zassert_true(bacnet_action_command_same(&action_expected, pAction), NULL);

    /* Read back first action element and verify round-trip decode. */
    rp_data.object_type = OBJECT_COMMAND;
    rp_data.object_instance = object_instance;
    rp_data.object_property = PROP_ACTION;
    rp_data.array_index = 1;
    rp_data.application_data = apdu;
    rp_data.application_data_len = sizeof(apdu);
    len = Command_Read_Property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacnet_action_command_decode(apdu, len, &action_decoded);
    zassert_true(len > 0, NULL);
    zassert_true(
        bacnet_action_command_same(&action_expected, &action_decoded), NULL);

    status = Command_Delete(object_instance);
    zassert_true(status, NULL);
    Command_Cleanup();
}

/**
 * @brief Test writable PROP_OBJECT_NAME and PROP_DESCRIPTION.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_command, test_object_command_name_description_write)
#else
static void test_object_command_name_description_write(void)
#endif
{
    uint32_t object_instance = BACNET_MAX_INSTANCE;
    bool status = false;
    int len = 0;
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    BACNET_READ_PROPERTY_DATA rp_data = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_CHARACTER_STRING cstring = { 0 };
    BACNET_CHARACTER_STRING object_name = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    const char *test_name = "COMMAND-NAME-WP";
    const char *test_description = "Command description written via WP";

    Command_Cleanup();
    object_instance = Command_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    wp_data.object_type = OBJECT_COMMAND;
    wp_data.object_instance = object_instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;

    /* Write object-name using WriteProperty. */
    status = characterstring_init_ansi(&cstring, test_name);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_OBJECT_NAME;
    wp_data.application_data_len =
        encode_application_character_string(wp_data.application_data, &cstring);
    status = Command_Write_Property(&wp_data);
    zassert_true(status, NULL);

    status = Command_Object_Name(object_instance, &object_name);
    zassert_true(status, NULL);
    status = characterstring_ansi_same(&object_name, test_name);
    zassert_true(status, NULL);

    /* Read object-name and verify decode value. */
    rp_data.object_type = OBJECT_COMMAND;
    rp_data.object_instance = object_instance;
    rp_data.object_property = PROP_OBJECT_NAME;
    rp_data.array_index = BACNET_ARRAY_ALL;
    rp_data.application_data = apdu;
    rp_data.application_data_len = sizeof(apdu);
    len = Command_Read_Property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    status = characterstring_ansi_same(&value.type.Character_String, test_name);
    zassert_true(status, NULL);

    /* Write description using WriteProperty. */
    status = characterstring_init_ansi(&cstring, test_description);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_DESCRIPTION;
    wp_data.application_data_len =
        encode_application_character_string(wp_data.application_data, &cstring);
    status = Command_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_not_null(Command_Description(object_instance), NULL);
    zassert_equal(
        strcmp(Command_Description(object_instance), test_description), 0,
        NULL);

    /* Read description and verify decode value. */
    rp_data.object_property = PROP_DESCRIPTION;
    len = Command_Read_Property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    status = characterstring_ansi_same(
        &value.type.Character_String, test_description);
    zassert_true(status, NULL);

    status = Command_Delete(object_instance);
    zassert_true(status, NULL);
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
        ztest_unit_test(test_object_command_dynamic),
        ztest_unit_test(test_object_command_action_array_write),
        ztest_unit_test(test_object_command_name_description_write));

    ztest_run_test_suite(tests_object_command);
}
#endif
