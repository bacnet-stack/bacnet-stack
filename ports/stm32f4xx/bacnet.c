/**************************************************************************
 *
 * Copyright (C) 2011 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#include <stdint.h>
#include <stdbool.h>
/* hardware layer includes */
#include "bacnet/basic/sys/mstimer.h"
#include "rs485.h"
/* BACnet Stack includes */
#include "bacnet/datalink/datalink.h"
#include "bacnet/npdu.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datetime.h"
#include "bacnet/dcc.h"
#include "bacnet/iam.h"
/* BACnet objects */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/program.h"
#include "bacnet/basic/program/ubasic/ubasic.h"
/* me */
#include "bacnet.h"

/* timer for device communications control */
static struct mstimer DCC_Timer;
#define DCC_CYCLE_SECONDS 1
/* Device ID to track changes */
static uint32_t Device_ID = 0xFFFFFFFF;
/* uBASIC-Plus program object */
static struct ubasic_data UBASIC_DATA = { 0 };
static struct mstimer UBASIC_Timer;
static uint32_t UBASIC_Instance = 1;

/**
 * @brief Load the program into the uBASIC interpreter
 * @param context Pointer to the uBASIC data structure
 * @return 0 on success
 */
static int Program_Load(void *context)
{
    struct ubasic_data *data = (struct ubasic_data *)context;
    const char *program = NULL;

    if (data->status.bit.isRunning == 0) {
        program = data->program_ptr;
    }
    ubasic_load_program(data, program);
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

    ubasic_clear_variables(data);
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
 * @brief Initialize the BACnet device object, the service handlers, and timers
 */
void bacnet_init(void)
{
    uint32_t instance;
    const char *ubasic_program_1 =
        /* program listing with either \0, \n, or ';' at the end of each line.
           note: indentation is not required */
        "println 'Demo - BACnet';"
        "bac_create(0, 1, 'AI-1');"
        "bac_create(0, 2, 'AI-2');"
        "bac_create(1, 1, 'AO-1');"
        "bac_create(1, 2, 'AO-2');"
        "bac_create(2, 1, 'AV-1');"
        "bac_create(2, 2, 'AV-2');"
        "bac_create(4, 1, 'BO-1');"
        "bac_create(4, 2, 'BO-2');"
        "for i = 1 to 255;"
        "  bac_write(0, 1, 85, i);"
        "  bac_write(0, 2, 85, i);"
        "  bac_write(1, 1, 85, i);"
        "  bac_write(1, 2, 85, i);"
        "  bac_write(2, 1, 85, i);"
        "  bac_write(2, 2, 85, i);"
        "  bac_write(4, 1, 85, i);"
        "  bac_write(4, 2, 85, i);"
        "  sleep (0.5);"
        "next i;"
        "end;";
    const char *ubasic_program_2 =
        /* program listing with either \0, \n, or ';' at the end of each line.
           note: indentation is not required */
        "println 'Demo - GPIO';"
        ":startover;"
        "  dwrite(1, 1);"
        "  dwrite(2, 1);"
        "  sleep (0.5);"
        "  dwrite(1, 0);"
        "  dwrite(2, 0);"
        "  sleep (0.5);"
        "goto startover;"
        "end;";
    const char *ubasic_program_3 =
        "println 'Demo - ADC';"
        ":startover;"
        "  a = aread(1);"
        "  c = avgw(a, c, 10)"
        "  println 'ADC-1 = ' c;"
        "  b = aread(2);"
        "  d = avgw(b, d, 10)"
        "  println 'ADC-2 = ' d;"
        "  sleep (0.2);"
        "goto startover;"
        "end;";
    VARIABLE_TYPE value = 0;
    struct ubasic_data *data;

    /* initialize objects */
    Device_Init(NULL);
    /* setup the uBASIC program and link to program object */
    data = &UBASIC_DATA;
    ubasic_port_init(data);
    data->program_ptr = ubasic_program_2;
    Program_Create(UBASIC_Instance);
    Program_Context_Set(UBASIC_Instance, data);
    Program_Load_Set(UBASIC_Instance, Program_Load);
    Program_Run_Set(UBASIC_Instance, Program_Run);
    Program_Halt_Set(UBASIC_Instance, Program_Halt);
    Program_Restart_Set(UBASIC_Instance, Program_Restart);
    Program_Unload_Set(UBASIC_Instance, Program_Unload);
    /* set up our confirmed service unrecognized service handler - required! */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    /* local time and date */
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION,
        handler_timesync);
    handler_timesync_set_callback_set(datetime_timesync);
    datetime_init();
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    /* start the cyclic 1 second timer for DCC */
    mstimer_set(&DCC_Timer, DCC_CYCLE_SECONDS * 1000);
    mstimer_set(&UBASIC_Timer, 10);
}

/* local buffer for incoming PDUs to process */
static uint8_t PDUBuffer[MAX_MPDU];

/**
 * @brief non-blocking BACnet task
 */
void bacnet_task(void)
{
    bool hello_world = false;
    uint16_t pdu_len = 0;
    BACNET_ADDRESS src = { 0 };

    /* hello, World! */
    if (Device_ID != Device_Object_Instance_Number()) {
        Device_ID = Device_Object_Instance_Number();
        hello_world = true;
    }
    if (hello_world) {
        Send_I_Am(&Handler_Transmit_Buffer[0]);
    }
    /* handle the communication timer */
    if (mstimer_expired(&DCC_Timer)) {
        mstimer_reset(&DCC_Timer);
        dcc_timer_seconds(DCC_CYCLE_SECONDS);
    }
    if (mstimer_expired(&UBASIC_Timer)) {
        mstimer_reset(&UBASIC_Timer);
        Program_Timer(UBASIC_Instance, mstimer_interval(&UBASIC_Timer));
    }
    /* handle the messaging */
    pdu_len = datalink_receive(&src, &PDUBuffer[0], sizeof(PDUBuffer), 0);
    if (pdu_len) {
        npdu_handler(&src, &PDUBuffer[0], pdu_len);
    }
}
