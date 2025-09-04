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
ZTEST(ao_tests, testProgramObject)
#else
static void testProgramObject(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int skip_fail_property_list[] = { -1 };

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

    test_program_task(object_instance);

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
    ztest_test_suite(program_object_tests, ztest_unit_test(testProgramObject));

    ztest_run_test_suite(program_object_tests);
}
#endif
