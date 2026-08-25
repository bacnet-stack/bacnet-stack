/**
 * @file
 * @brief Unit test for the Program object type
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date March 2025
 * @section LICENSE
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/program.h>
#include <bacnet/proplist.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

static int Program_Load(void *context)
{
    /* Placeholder for load function */
    (void)context;

    return 0;
}

static int Program_Run(void *context)
{
    /* Placeholder for run function */
    (void)context;

    return 0;
}
static int Program_Halt(void *context)
{
    /* Placeholder for halt function */
    (void)context;

    return 0;
}
static int Program_Restart(void *context)
{
    /* Placeholder for restart function */
    (void)context;

    return 0;
}

static void test_program_task(uint32_t object_instance)
{
    int context = 0;
    uint16_t milliseconds = 1000;
    bool status = false;
    BACNET_PROGRAM_REQUEST program_change = PROGRAM_REQUEST_READY;

    Program_Context_Set(object_instance, &context);
    Program_Load_Set(object_instance, Program_Load);
    Program_Run_Set(object_instance, Program_Run);
    Program_Halt_Set(object_instance, Program_Halt);
    Program_Restart_Set(object_instance, Program_Restart);
    Program_Unload_Set(object_instance, NULL);

    status = Program_Change_Set(object_instance, program_change);
    zassert_true(status, NULL);

    Program_Timer(object_instance, milliseconds);
    zassert_equal(Program_Change(object_instance), PROGRAM_REQUEST_READY, NULL);
    zassert_equal(Program_State(object_instance), PROGRAM_STATE_IDLE, NULL);

    status = Program_Change_Set(object_instance, PROGRAM_REQUEST_LOAD);
    zassert_true(status, NULL);
    zassert_equal(Program_Change(object_instance), PROGRAM_REQUEST_LOAD, NULL);
    Program_Timer(object_instance, milliseconds);
    zassert_equal(Program_Change(object_instance), PROGRAM_REQUEST_READY, NULL);
    zassert_equal(Program_State(object_instance), PROGRAM_STATE_LOADING, NULL);
    Program_Timer(object_instance, milliseconds);
    zassert_equal(Program_State(object_instance), PROGRAM_STATE_HALTED, NULL);
    status = Program_Change_Set(object_instance, PROGRAM_REQUEST_RUN);
    zassert_true(status, NULL);
    zassert_equal(Program_Change(object_instance), PROGRAM_REQUEST_RUN, NULL);
    Program_Timer(object_instance, milliseconds);
    zassert_equal(Program_State(object_instance), PROGRAM_STATE_RUNNING, NULL);
    status = Program_Change_Set(object_instance, PROGRAM_REQUEST_HALT);
    zassert_true(status, NULL);
    zassert_equal(Program_Change(object_instance), PROGRAM_REQUEST_HALT, NULL);
    Program_Timer(object_instance, milliseconds);
    zassert_equal(Program_State(object_instance), PROGRAM_STATE_HALTED, NULL);
    status = Program_Change_Set(object_instance, PROGRAM_REQUEST_RESTART);
    zassert_true(status, NULL);
    zassert_equal(
        Program_Change(object_instance), PROGRAM_REQUEST_RESTART, NULL);
    Program_Timer(object_instance, milliseconds);
    zassert_equal(Program_State(object_instance), PROGRAM_STATE_RUNNING, NULL);
    status = Program_Change_Set(object_instance, PROGRAM_REQUEST_UNLOAD);
    zassert_true(status, NULL);
    zassert_equal(
        Program_Change(object_instance), PROGRAM_REQUEST_UNLOAD, NULL);
    Program_Timer(object_instance, milliseconds);
    zassert_equal(
        Program_State(object_instance), PROGRAM_STATE_UNLOADING, NULL);
}

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(program_object_tests, testProgramObject)
#else
static void testProgramObject(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int32_t *writable_properties = NULL;
    const int32_t skip_fail_property_list[] = { -1 };

    Program_Init();
    object_instance = Program_Create(object_instance);
    count = Program_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Program_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_PROGRAM, object_instance, Program_Property_Lists,
        Program_Read_Property, Program_Write_Property, skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Program_Name_Set, Program_Name_ASCII);
    Program_Writable_Property_List(object_instance, &writable_properties);
    zassert_not_null(writable_properties, NULL);
    zassert_true(
        property_list_member(writable_properties, PROP_OBJECT_NAME), NULL);
    zassert_true(
        property_list_member(writable_properties, PROP_DESCRIPTION), NULL);
    zassert_true(
        property_list_member(writable_properties, PROP_INSTANCE_OF), NULL);

    test_program_task(object_instance);

    status = Program_Delete(object_instance);
    zassert_true(status, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(program_object_tests, testProgramObject_character_string_write)
#else
static void testProgramObject_character_string_write(void)
#endif
{
    bool status = false;
    uint32_t object_instance = 99;
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    BACNET_READ_PROPERTY_DATA rp_data = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_CHARACTER_STRING cstring = { 0 };
    BACNET_CHARACTER_STRING object_name = { 0 };
    BACNET_CHARACTER_STRING description = { 0 };
    BACNET_CHARACTER_STRING instance_of = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    const char *test_name = "PROGRAM-OBJECT-NAME";
    const char *test_description = "Program description written via WP";
    const char *test_instance_of = "Program Instance Of";

    Program_Init();
    object_instance = Program_Create(object_instance);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    wp_data.object_type = OBJECT_PROGRAM;
    wp_data.object_instance = object_instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;

    status = characterstring_init_ansi(&cstring, test_name);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_OBJECT_NAME;
    wp_data.application_data_len =
        encode_application_character_string(wp_data.application_data, &cstring);
    status = Program_Write_Property(&wp_data);
    zassert_true(status, NULL);
    status = Program_Object_Name(object_instance, &object_name);
    zassert_true(status, NULL);
    zassert_true(characterstring_ansi_same(&object_name, test_name), NULL);

    rp_data.object_type = OBJECT_PROGRAM;
    rp_data.object_instance = object_instance;
    rp_data.object_property = PROP_OBJECT_NAME;
    rp_data.array_index = BACNET_ARRAY_ALL;
    rp_data.application_data = apdu;
    rp_data.application_data_len = sizeof(apdu);
    len = Program_Read_Property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    zassert_true(
        characterstring_ansi_same(&value.type.Character_String, test_name),
        NULL);

    status = characterstring_init_ansi(&cstring, test_description);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_DESCRIPTION;
    wp_data.application_data_len =
        encode_application_character_string(wp_data.application_data, &cstring);
    status = Program_Write_Property(&wp_data);
    zassert_true(status, NULL);
    status = Program_Description(object_instance, &description);
    zassert_true(status, NULL);
    zassert_true(
        characterstring_ansi_same(&description, test_description), NULL);

    rp_data.object_property = PROP_DESCRIPTION;
    len = Program_Read_Property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    zassert_true(
        characterstring_ansi_same(
            &value.type.Character_String, test_description),
        NULL);

    status = characterstring_init_ansi(&cstring, test_instance_of);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_INSTANCE_OF;
    wp_data.application_data_len =
        encode_application_character_string(wp_data.application_data, &cstring);
    status = Program_Write_Property(&wp_data);
    zassert_true(status, NULL);
    status = Program_Instance_Of(object_instance, &instance_of);
    zassert_true(status, NULL);
    zassert_true(
        characterstring_ansi_same(&instance_of, test_instance_of), NULL);

    rp_data.object_property = PROP_INSTANCE_OF;
    len = Program_Read_Property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    zassert_true(
        characterstring_ansi_same(
            &value.type.Character_String, test_instance_of),
        NULL);

    status = Program_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(program_object_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        program_object_tests, ztest_unit_test(testProgramObject),
        ztest_unit_test(testProgramObject_character_string_write));

    ztest_run_test_suite(program_object_tests);
}
#endif
