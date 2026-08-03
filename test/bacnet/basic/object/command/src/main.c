/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <math.h>
#include <bacnet/bacapp.h>
#include <bacnet/bacstr.h>
#include <bacnet/bacdcode.h>
#include <bacnet/basic/object/command.h>
#include <property_test.h>

/**
 * @brief compare two floating point values to 3 decimal places
 *
 * @param x1 - first comparison value
 * @param x2 - second comparison value
 * @return true if the value is the same to 3 decimal points
 */
static bool is_float_equal(float x1, float x2)
{
    return fabsf(x1 - x2) < 0.001f;
}

static float Test_Action_Real_Value;
static bool Test_Action_Write_Success;

static void action_list_set_property_value(
    BACNET_ACTION_LIST *action, const BACNET_ACTION_PROPERTY_VALUE *value)
{
    int len = 0;

    if (!action || !value) {
        return;
    }
    len =
        bacnet_action_property_value_encode(action->Property_Value.data, value);
    if ((len > 0) && (len <= (int)sizeof(action->Property_Value.data))) {
        action->Property_Value.data_len = (uint16_t)len;
    } else {
        action->Property_Value.data_len = 0;
    }
}

uint32_t Device_Object_Instance_Number(void)
{
    return 260001;
}

static bool Write_Property_Internal(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    int len;

    if (!wp_data) {
        return false;
    }
    len = bacapp_decode_application_data(
        wp_data->application_data, wp_data->application_data_len, &value);
    if (len < 0) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    if ((wp_data->object_type == OBJECT_ANALOG_VALUE) &&
        (wp_data->object_instance == 1) &&
        (wp_data->object_property == PROP_PRESENT_VALUE) &&
        (value.tag == BACNET_APPLICATION_TAG_REAL)) {
        Test_Action_Real_Value = value.type.Real;
        Test_Action_Write_Success = true;
        return true;
    }

    wp_data->error_class = ERROR_CLASS_PROPERTY;
    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
    return false;
}

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
    BACNET_ACTION_PROPERTY_VALUE action_value = { 0 };

    Command_Cleanup();
    object_instance = Command_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);
    count = Command_Count();
    zassert_true(count > 0, NULL);
    status = Command_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    count = Command_Action_Array_Count(object_instance);
    zassert_true(count > 0, NULL);
    /* configure the instance property values */
    pAction = Command_Action_List_Member(object_instance, 0, 0);
    zassert_not_null(pAction, NULL);
    pAction->Device_Id.type = OBJECT_DEVICE;
    pAction->Device_Id.instance = 4194303;
    pAction->Object_Id.type = OBJECT_ANALOG_INPUT;
    pAction->Object_Id.instance = 4194303;
    pAction->Property_Identifier = PROP_PRESENT_VALUE;
    pAction->Property_Array_Index = BACNET_ARRAY_ALL;
    pAction->Priority = 16;
    action_value.tag = BACNET_APPLICATION_TAG_REAL;
    action_value.type.Real = 3.14159f;
    action_list_set_property_value(pAction, &action_value);
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
 * @brief Test busy error when Present_Value is written while in-process.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_command, test_object_command_present_value_write_busy)
#else
static void test_object_command_present_value_write_busy(void)
#endif
{
    uint32_t object_instance = BACNET_MAX_INSTANCE;
    bool status = false;
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };

    Command_Cleanup();
    object_instance = Command_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    status = Command_In_Process_Set(object_instance, true);
    zassert_true(status, NULL);
    zassert_true(Command_In_Process(object_instance), NULL);

    wp_data.object_type = OBJECT_COMMAND;
    wp_data.object_instance = object_instance;
    wp_data.object_property = PROP_PRESENT_VALUE;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 1);

    status = Command_Write_Property(&wp_data);
    zassert_false(status, NULL);
    zassert_equal(wp_data.error_class, ERROR_CLASS_OBJECT, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_BUSY, NULL);
    zassert_equal(Command_Present_Value(object_instance), 0, NULL);
    zassert_true(Command_In_Process(object_instance), NULL);

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

    Command_Cleanup();
    zassert_equal(Command_Count(), 0, NULL);

    object_instance = Command_Create(BACNET_MAX_INSTANCE + 1);
    zassert_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    object_instance = Command_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);
    zassert_true(Command_Valid_Instance(object_instance), NULL);
    zassert_equal(Command_Count(), 1, NULL);

    count = Command_Action_Array_Count(object_instance);
    zassert_true(count > 0, NULL);

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
    BACNET_ACTION_PROPERTY_VALUE action_value = { 0 };
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
    zassert_equal(Command_Action_Array_Count(object_instance), 2, NULL);

    /* Write first action element (array index 1). */
    action_expected.Device_Id.type = OBJECT_DEVICE;
    action_expected.Device_Id.instance = 1234;
    action_expected.Object_Id.type = OBJECT_ANALOG_VALUE;
    action_expected.Object_Id.instance = 7;
    action_expected.Property_Identifier = PROP_PRESENT_VALUE;
    action_expected.Property_Array_Index = BACNET_ARRAY_ALL;
    action_value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    action_value.type.Unsigned_Int = 42;
    action_list_set_property_value(&action_expected, &action_value);
    action_expected.Priority = 8;
    action_expected.Post_Delay = 10;
    action_expected.Quit_On_Failure = true;
    action_expected.Write_Successful = false;

    wp_data.array_index = 1;
    wp_data.application_data_len = bacnet_action_command_encode(
        wp_data.application_data, &action_expected);
    status = Command_Write_Property(&wp_data);
    zassert_true(status, NULL);

    pAction = Command_Action_List_Member(object_instance, 0, 0);
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
 * @brief Test Command_Timer executes action list and updates status flags.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_command, test_object_command_timer_success)
#else
static void test_object_command_timer_success(void)
#endif
{
    uint32_t command_instance = BACNET_MAX_INSTANCE;
    bool status;
    BACNET_ACTION_LIST *pAction;
    BACNET_ACTION_PROPERTY_VALUE action_value = { 0 };
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };

    Command_Cleanup();
    command_instance = Command_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(command_instance, BACNET_MAX_INSTANCE, NULL);
    Command_Write_Property_Internal_Callback_Set(Write_Property_Internal);
    Test_Action_Real_Value = 0.0f;
    Test_Action_Write_Success = false;

    wp_data.object_type = OBJECT_COMMAND;
    wp_data.object_instance = command_instance;
    wp_data.object_property = PROP_ACTION;
    wp_data.array_index = 0;
    wp_data.priority = BACNET_NO_PRIORITY;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 1);
    status = Command_Write_Property(&wp_data);
    zassert_true(status, NULL);

    status = Command_Present_Value_Set(command_instance, 1);
    zassert_true(status, NULL);
    pAction = Command_Action_List_Member(command_instance, 0, 0);
    zassert_not_null(pAction, NULL);
    pAction->Object_Id.type = OBJECT_ANALOG_VALUE;
    pAction->Object_Id.instance = 1;
    pAction->Property_Identifier = PROP_PRESENT_VALUE;
    pAction->Property_Array_Index = BACNET_ARRAY_ALL;
    action_value.tag = BACNET_APPLICATION_TAG_REAL;
    action_value.type.Real = 12.5f;
    action_list_set_property_value(pAction, &action_value);
    pAction->Priority = 8;
    pAction->Post_Delay = 0;
    pAction->Quit_On_Failure = false;
    pAction->Write_Successful = false;
    pAction->next = NULL;

    Command_Timer(command_instance, 1000);

    zassert_false(Command_In_Process(command_instance), NULL);
    zassert_true(Command_All_Writes_Successful(command_instance), NULL);
    zassert_true(pAction->Write_Successful, NULL);
    zassert_true(Test_Action_Write_Success, NULL);
    zassert_true(is_float_equal(Test_Action_Real_Value, 12.5f), NULL);

    status = Command_Delete(command_instance);
    zassert_true(status, NULL);
    Command_Cleanup();
}

/**
 * @brief Test Command_Timer delay and quit-on-failure semantics.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_command, test_object_command_timer_delay_and_failure)
#else
static void test_object_command_timer_delay_and_failure(void)
#endif
{
    uint32_t command_instance = BACNET_MAX_INSTANCE;
    bool status;
    BACNET_ACTION_LIST *pAction0;
    BACNET_ACTION_LIST *pAction1;
    BACNET_ACTION_PROPERTY_VALUE action_value = { 0 };
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };

    Command_Cleanup();
    command_instance = Command_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(command_instance, BACNET_MAX_INSTANCE, NULL);
    Command_Write_Property_Internal_Callback_Set(Write_Property_Internal);
    Test_Action_Real_Value = 0.0f;
    Test_Action_Write_Success = false;

    wp_data.object_type = OBJECT_COMMAND;
    wp_data.object_instance = command_instance;
    wp_data.object_property = PROP_ACTION;
    wp_data.array_index = 0;
    wp_data.priority = BACNET_NO_PRIORITY;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 1);
    status = Command_Write_Property(&wp_data);
    zassert_true(status, NULL);

    /* build a 2-command list in slot 0: cmd0 (real write, 1s post-delay)
     * followed by cmd1 (fails quit-on-failure) */
    BACNET_ACTION_LIST setup_cmds[2] = { 0 };
    setup_cmds[0].Device_Id.type = OBJECT_DEVICE;
    setup_cmds[0].Device_Id.instance = BACNET_MAX_INSTANCE; /* local */
    setup_cmds[0].Object_Id.type = OBJECT_ANALOG_VALUE;
    setup_cmds[0].Object_Id.instance = 1;
    setup_cmds[0].Property_Identifier = PROP_PRESENT_VALUE;
    setup_cmds[0].Property_Array_Index = BACNET_ARRAY_ALL;
    action_value.tag = BACNET_APPLICATION_TAG_REAL;
    action_value.type.Real = 7.0f;
    action_list_set_property_value(&setup_cmds[0], &action_value);
    setup_cmds[0].Priority = 8;
    setup_cmds[0].Post_Delay = 1;
    setup_cmds[0].Quit_On_Failure = false;

    setup_cmds[1].Device_Id.type = OBJECT_DEVICE;
    setup_cmds[1].Device_Id.instance = BACNET_MAX_INSTANCE; /* local */
    setup_cmds[1].Object_Id.type = OBJECT_ANALOG_VALUE;
    setup_cmds[1].Object_Id.instance = 1;
    setup_cmds[1].Property_Identifier = PROP_OBJECT_NAME;
    setup_cmds[1].Property_Array_Index = BACNET_ARRAY_ALL;
    action_value.tag = BACNET_APPLICATION_TAG_NULL;
    action_list_set_property_value(&setup_cmds[1], &action_value);
    setup_cmds[1].Priority = BACNET_NO_PRIORITY;
    setup_cmds[1].Post_Delay = 0;
    setup_cmds[1].Quit_On_Failure = true;
    setup_cmds[0].next = &setup_cmds[1];
    Command_Action_List_Set(command_instance, 0, &setup_cmds[0]);

    /* retrieve stored copies to check Write_Successful after timer */
    pAction0 = Command_Action_List_Member(command_instance, 0, 0);
    pAction1 = Command_Action_List_Member(command_instance, 0, 1);
    zassert_not_null(pAction0, NULL);
    zassert_not_null(pAction1, NULL);

    status = Command_Present_Value_Set(command_instance, 1);
    zassert_true(status, NULL);

    Command_Timer(command_instance, 100);
    zassert_true(pAction0->Write_Successful, NULL);
    zassert_true(Command_In_Process(command_instance), NULL);
    zassert_false(pAction1->Write_Successful, NULL);

    Command_Timer(command_instance, 500);
    zassert_true(Command_In_Process(command_instance), NULL);
    zassert_false(pAction1->Write_Successful, NULL);
    zassert_true(Test_Action_Write_Success, NULL);
    zassert_true(is_float_equal(Test_Action_Real_Value, 7.0f), NULL);

    Command_Timer(command_instance, 600);
    zassert_false(Command_In_Process(command_instance), NULL);
    zassert_false(Command_All_Writes_Successful(command_instance), NULL);
    zassert_false(pAction1->Write_Successful, NULL);

    status = Command_Delete(command_instance);
    zassert_true(status, NULL);
    Command_Cleanup();
}

/**
 * @brief Test Present_Value of zero completes immediately with success.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_command, test_object_command_timer_present_value_zero)
#else
static void test_object_command_timer_present_value_zero(void)
#endif
{
    uint32_t command_instance = BACNET_MAX_INSTANCE;
    bool status;

    Command_Cleanup();
    command_instance = Command_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(command_instance, BACNET_MAX_INSTANCE, NULL);

    status = Command_Present_Value_Set(command_instance, 0);
    zassert_true(status, NULL);
    zassert_true(Command_In_Process(command_instance), NULL);

    Command_Timer(command_instance, 1);

    zassert_false(Command_In_Process(command_instance), NULL);
    zassert_true(Command_All_Writes_Successful(command_instance), NULL);

    status = Command_Delete(command_instance);
    zassert_true(status, NULL);
    Command_Cleanup();
}

/* re-entrant callback: tries to shrink Action[0] of the same command object */
static uint32_t Reentrant_Command_Instance;
static bool Reentrant_WP_Called;

static bool Write_Property_Reentrant_Shrink(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    BACNET_WRITE_PROPERTY_DATA inner_wp = { 0 };

    (void)wp_data;
    Reentrant_WP_Called = true;
    inner_wp.object_type = OBJECT_COMMAND;
    inner_wp.object_instance = Reentrant_Command_Instance;
    inner_wp.object_property = PROP_ACTION;
    inner_wp.array_index = 0;
    /* attempt to shrink array to 0 while the command is executing */
    inner_wp.application_data_len =
        encode_application_unsigned(inner_wp.application_data, 0);
    /* must be rejected with BUSY while in-process */
    zassert_false(Command_Write_Property(&inner_wp), NULL);
    zassert_equal(inner_wp.error_class, ERROR_CLASS_OBJECT, NULL);
    zassert_equal(inner_wp.error_code, ERROR_CODE_BUSY, NULL);
    return true;
}

/**
 * @brief Test PROP_ACTION write is blocked (BUSY) while command is executing.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_command, test_object_command_action_write_busy_during_exec)
#else
static void test_object_command_action_write_busy_during_exec(void)
#endif
{
    BACNET_ACTION_LIST cmd = { 0 };
    BACNET_ACTION_PROPERTY_VALUE val = { 0 };
    bool status;

    Command_Cleanup();
    Reentrant_Command_Instance = Command_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(Reentrant_Command_Instance, BACNET_MAX_INSTANCE, NULL);

    cmd.Device_Id.type = OBJECT_DEVICE;
    cmd.Device_Id.instance = BACNET_MAX_INSTANCE; /* local */
    cmd.Object_Id.type = OBJECT_ANALOG_VALUE;
    cmd.Object_Id.instance = 99;
    cmd.Property_Identifier = PROP_PRESENT_VALUE;
    cmd.Property_Array_Index = BACNET_ARRAY_ALL;
    val.tag = BACNET_APPLICATION_TAG_REAL;
    val.type.Real = 1.0f;
    action_list_set_property_value(&cmd, &val);
    cmd.Priority = BACNET_NO_PRIORITY;
    cmd.Post_Delay = UINT32_MAX;
    cmd.Quit_On_Failure = false;
    Command_Action_List_Set(Reentrant_Command_Instance, 0, &cmd);

    Command_Write_Property_Internal_Callback_Set(
        Write_Property_Reentrant_Shrink);
    Reentrant_WP_Called = false;

    status = Command_Present_Value_Set(Reentrant_Command_Instance, 1);
    zassert_true(status, NULL);
    Command_Timer(Reentrant_Command_Instance, 0);

    zassert_true(Reentrant_WP_Called, NULL);
    /* action list must still be intact after the blocked shrink attempt */
    zassert_equal(
        Command_Action_Array_Count(Reentrant_Command_Instance), 1u, NULL);
    zassert_false(Command_In_Process(Reentrant_Command_Instance), NULL);

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
        ztest_unit_test(test_object_command_present_value_write_busy),
        ztest_unit_test(test_object_command_name_description_write),
        ztest_unit_test(test_object_command_timer_success),
        ztest_unit_test(test_object_command_timer_delay_and_failure),
        ztest_unit_test(test_object_command_timer_present_value_zero),
        ztest_unit_test(test_object_command_action_write_busy_during_exec));

    ztest_run_test_suite(tests_object_command);
}
#endif
