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
#include <stdlib.h>
#include "bacnet/basic/object/program.h"
#include "bacnet/basic/program/ubasic/ubasic.h"
#include "bacnet/basic/sys/mstimer.h"

/* uBASIC-Plus program object */
static struct ubasic_data UBASIC_DATA = { 0 };
static struct mstimer UBASIC_Timer;
static uint32_t UBASIC_Instance = 0;
static const char *UBASIC_Program =
    /* program listing with either \0, \n, or ';' at the end of each line.
       note: indentation is not required */
    "println 'Demo - BACnet & GPIO';"
    "bac_create(0, 1, 'ADC-1');"
    "bac_create(0, 2, 'ADC-2');"
    "bac_create(4, 1, 'LED-1');"
    "bac_create(4, 2, 'LED-2');"
    ":startover;"
    "  a = aread(1);"
    "  c = avgw(a, c, 10);"
    "  bac_write(0, 1, 85, c);"
    "  b = aread(2);"
    "  d = avgw(b, d, 10);"
    "  bac_write(0, 2, 85, d);"
    "  h = bac_read(4, 1, 85);"
    "  dwrite(1, (h % 2));"
    "  i = bac_read(4, 2, 85);"
    "  dwrite(2, (i % 2));"
    "  sleep (0.2);"
    "goto startover;"
    "end;";

/**
 * @brief Load the program into the uBASIC interpreter
 * @param context Pointer to the uBASIC data structure
 * @return 0 on success
 */
static int Program_Load(void *context)
{
    struct ubasic_data *data = (struct ubasic_data *)context;

    ubasic_load_program(data, UBASIC_Program);
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
    struct ubasic_data *data = (struct ubasic_data *)context;
    int result = 0;

    result = ubasic_run_program(data);
    if (result <= 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief Halt the program in the uBASIC interpreter
 * @param context Pointer to the uBASIC data structure
 * @return 0 on success, non-zero on error
 */
static int Program_Halt(void *context)
{
    struct ubasic_data *data = (struct ubasic_data *)context;

    data->status.bit.isRunning = 0;

    return 0;
}

/**
 * @brief Restart the program in the uBASIC interpreter
 * @param context Pointer to the uBASIC data structure
 * @return 0 on success, non-zero on error
 */
static int Program_Restart(void *context)
{
    struct ubasic_data *data = (struct ubasic_data *)context;

    ubasic_clear_variables(data);
    ubasic_load_program(data, UBASIC_Program);

    return 0;
}

/**
 * @brief Unload the program in the uBASIC interpreter
 * @param context Pointer to the uBASIC data structure
 * @return 0 on success, non-zero on error
 */
static int Program_Unload(void *context)
{
    struct ubasic_data *data = (struct ubasic_data *)context;

    ubasic_clear_variables(data);
    return 0;
}

/**
 * @brief Timer task for the uBASIC program object
 */
void Program_UBASIC_Task(void)
{
    if (mstimer_expired(&UBASIC_Timer)) {
        mstimer_reset(&UBASIC_Timer);
        Program_Timer(UBASIC_Instance, mstimer_interval(&UBASIC_Timer));
    }
}

/**
 * @brief Initialize the uBASIC program object
 * @param instance Instance number of the program object
 * @return 0 on success, non-zero on error
 */
void Program_UBASIC_Init(uint32_t instance)
{
    ubasic_port_init(&UBASIC_DATA);
    UBASIC_Instance = Program_Create(instance);
    Program_Context_Set(UBASIC_Instance, &UBASIC_DATA);
    Program_Load_Set(UBASIC_Instance, Program_Load);
    Program_Run_Set(UBASIC_Instance, Program_Run);
    Program_Halt_Set(UBASIC_Instance, Program_Halt);
    Program_Restart_Set(UBASIC_Instance, Program_Restart);
    Program_Unload_Set(UBASIC_Instance, Program_Unload);
    /* auto-run the program */
    Program_Change_Set(UBASIC_Instance, PROGRAM_REQUEST_RUN);
    /* start the cyclic 10ms run timer for the program object */
    mstimer_set(&UBASIC_Timer, 10);
}
