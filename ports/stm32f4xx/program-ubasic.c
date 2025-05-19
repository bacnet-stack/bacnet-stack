/**
 * @file
 * @brief uBASIC-Plus program object for BACnet
 * @author Steve Karg
 * @date 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "bacnet/basic/object/program.h"
#include "bacnet/basic/program/ubasic/ubasic.h"
#include "bacnet/basic/sys/mstimer.h"

/* timer for the task */
static struct mstimer UBASIC_Timer;
/* context structure */

/**
 * @brief Load the program into the uBASIC interpreter
 * @param context Pointer to the uBASIC data structure
 * @return 0 on success
 */
static int Program_Load(void *context)
{
    if (!context) {
        return -1;
    }
    struct ubasic_data *data = (struct ubasic_data *)context;

    ubasic_load_program(data, NULL);
    (void)ubasic_program_location(data);

    return 0;
}

/**
 * @brief Run the program in the uBASIC interpreter
 * @param context Pointer to the uBASIC data structure
 * @return 0 while the programm is running, non-zero when finished
 *         or an error occurred
 */
static int Program_Run(void *context)
{
    if (!context) {
        return -1;
    }
    struct ubasic_data *data = (struct ubasic_data *)context;
    int result = 0;

    result = ubasic_run_program(data);
    if (result <= 0) {
        return -1;
    }
    (void)ubasic_program_location(data);

    return 0;
}

/**
 * @brief Halt the program in the uBASIC interpreter
 * @param context Pointer to the uBASIC data structure
 * @return 0 on success, non-zero on error
 */
static int Program_Halt(void *context)
{
    if (!context) {
        return -1;
    }
    struct ubasic_data *data = (struct ubasic_data *)context;

    ubasic_halt_program(data);

    return 0;
}

/**
 * @brief Restart the program in the uBASIC interpreter
 * @param context Pointer to the uBASIC data structure
 * @return 0 on success, non-zero on error
 */
static int Program_Restart(void *context)
{
    if (!context) {
        return -1;
    }
    struct ubasic_data *data = (struct ubasic_data *)context;

    ubasic_clear_variables(data);
    ubasic_load_program(data, NULL);
    (void)ubasic_program_location(data);

    return 0;
}

/**
 * @brief Unload the program in the uBASIC interpreter
 * @param context Pointer to the uBASIC data structure
 * @return 0 on success, non-zero on error
 */
static int Program_Unload(void *context)
{
    if (!context) {
        return -1;
    }
    struct ubasic_data *data = (struct ubasic_data *)context;

    ubasic_clear_variables(data);

    return 0;
}

/**
 * @brief Timer task for the uBASIC program object
 */
void Program_UBASIC_Task(void)
{
    size_t index, max_index;
    uint32_t instance;

    if (mstimer_expired(&UBASIC_Timer)) {
        mstimer_reset(&UBASIC_Timer);
        max_index = Program_Count();
        for (index = 0; index < max_index; index++) {
            instance = Program_Index_To_Instance(index);
            Program_Timer(instance, mstimer_interval(&UBASIC_Timer));
        }
    }
}

/**
 * @brief Create one uBASIC program object
 * @param requested_instance Instance number of the program object
 * @param context Pointer to the uBASIC data structure
 * @param program Pointer to the uBASIC program
 * @return 0 on success, non-zero on error
 */
int Program_UBASIC_Create(
    uint32_t requested_instance,
    struct ubasic_data *context,
    const char *program)
{
    uint32_t instance = 0;

    if (!context) {
        return -1;
    }
    if (Program_Valid_Instance(requested_instance)) {
        instance = requested_instance;
        if (program) {
            context->program = program;
        }
        Program_Change_Set(instance, PROGRAM_REQUEST_RESTART);
    } else {
        instance = Program_Create(requested_instance);
        if (instance == BACNET_MAX_INSTANCE) {
            return -1;
        }
        if (program) {
            context->program = program;
        } else {
            context->program = "end;";
        }
        Program_Context_Set(instance, context);
        Program_Load_Set(instance, Program_Load);
        Program_Run_Set(instance, Program_Run);
        Program_Halt_Set(instance, Program_Halt);
        Program_Restart_Set(instance, Program_Restart);
        Program_Unload_Set(instance, Program_Unload);
        Program_Location_Set(instance, context->location);
        ubasic_port_init(context);
        /* auto-run the program */
        Program_Change_Set(instance, PROGRAM_REQUEST_RUN);
    }

    return 0;
}

/**
 * @brief Initialize the uBASIC program object
 * @param requested_instance Instance number of the program object
 * @return 0 on success, non-zero on error
 */
void Program_UBASIC_Init(unsigned long task_ms)
{
    /* start the cyclic 10ms run timer for the program object */
    mstimer_set(&UBASIC_Timer, task_ms);
}
