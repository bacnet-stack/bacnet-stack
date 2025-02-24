/**
 * @file
 * @brief BACnet Stack sample Smart Sensor (B-SS) main file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/server/bacnet_basic.h"
#include "bacnet/basic/server/bacnet_port.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"

static const char *Device_Name = "BACnet Smart Sensor (B-SS)";
/* object instances */
static const uint32_t Sensor_Instance = 1;
/* timer for Sensor Update Interval */
static struct mstimer Sensor_Update_Timer;

/**
 * @brief BACnet Project Initialization Handler
 * @param context [in] The context to pass to the callback function
 * @note This is called from the BACnet task
 */
static void BACnet_Smart_Sensor_Init_Handler(void *context)
{
    (void)context;
    /* initialize child objects for this basic sample */
    Analog_Input_Create(Sensor_Instance);
    Analog_Input_Name_Set(Sensor_Instance, "Sensor");
    Analog_Input_Present_Value_Set(Sensor_Instance, 25.0f);
    debug_printf_stdout(
        "BACnet Device ID: %u\n", Device_Object_Instance_Number());
    /* start the seconds cyclic timer */
    mstimer_set(&Sensor_Update_Timer, 1000);
    srand(0);
}

/**
 * @brief BACnet Project Task Handler
 * @param context [in] The context to pass to the callback function
 * @note This is called from the BACnet task
 */
static void BACnet_Smart_Sensor_Task_Handler(void *context)
{
    float temperature = 0.0f, change = 0.0f;

    (void)context;
    if (mstimer_expired(&Sensor_Update_Timer)) {
        mstimer_reset(&Sensor_Update_Timer);
        /* simulate a sensor reading, and update the BACnet object values */
        if (Analog_Input_Out_Of_Service(Sensor_Instance)) {
            return;
        }
        temperature = Analog_Input_Present_Value(Sensor_Instance);
        change = -1.0f + 2.0f * ((float)rand()) / RAND_MAX;
        temperature += change;
        Analog_Input_Present_Value_Set(Sensor_Instance, temperature);
    }
}

/** Main function of server demo.
 *
 * @see Device_Set_Object_Instance_Number, dlenv_init, Send_I_Am,
 *      datalink_receive, npdu_handler,
 *      dcc_timer_seconds, datalink_maintenance_timer,
 *      handler_cov_task, tsm_timer_milliseconds
 *
 * @param argc [in] Arg count.
 * @param argv [in] Takes one argument: the Device Instance #.
 * @return 0 on success.
 */
int main(int argc, char *argv[])
{
    if (argc > 1) {
        /* allow the device ID to be set */
        Device_Set_Object_Instance_Number(strtol(argv[1], NULL, 0));
    }

    if (argc > 2) {
        /* allow the device name to be set */
        Device_Name = argv[2];
    }
    Device_Object_Name_ANSI_Init(Device_Name);
    debug_printf_stdout("BACnet Device: %s\n", Device_Name);
    debug_printf_stdout("BACnet Stack Version %s\n", BACNET_VERSION_TEXT);
    debug_printf_stdout("BACnet Stack Max APDU: %d\n", MAX_APDU);
    bacnet_basic_init_callback_set(BACnet_Smart_Sensor_Init_Handler, NULL);
    bacnet_basic_task_callback_set(BACnet_Smart_Sensor_Task_Handler, NULL);
    bacnet_basic_init();
    if (bacnet_port_init()) {
        /* OS based apps use DLENV for environment variables */
        dlenv_init();
        atexit(datalink_cleanup);
    }
    debug_printf_stdout("Server: initialized\n");
    for (;;) {
        bacnet_basic_task();
        bacnet_port_task();
    }

    return 0;
}
