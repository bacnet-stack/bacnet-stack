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
#include "bacnet/basic/object/ao.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"

static const char *Device_Name = "BACnet Smart Sensor (B-SS)";
#define SENSOR_ID 1

/* table of default point names and units */
struct BACnetObjectTable {
    const uint8_t type;
    const uint8_t instance;
    const uint16_t units;
    const float value;
    const char *name;
    const char *description;
};
static struct BACnetObjectTable Object_Table[] = {
    { OBJECT_ANALOG_INPUT, SENSOR_ID, UNITS_DEGREES_CELSIUS, 25.0f,
      "Indoor Air Temperature", "indoor air temperature" },
    { OBJECT_ANALOG_OUTPUT, 1, UNITS_DEGREES_ANGULAR, 0.0f, "VAV Actuator",
      "variable air valve actuator" },
    { OBJECT_ANALOG_OUTPUT, 2, UNITS_DEGREES_CELSIUS, 25.0f,
      "Temperature Setpoint", "indoor air temperature setpoint" },
};
/* timer for Sensor Update Interval */
static struct mstimer Sensor_Update_Timer;

/**
 * @brief BACnet Project Initialization Handler
 * @param context [in] The context to pass to the callback function
 * @note This is called from the BACnet task
 */
static void BACnet_Object_Table_Init(void *context)
{
    unsigned i;

    (void)context;
    /* initialize child objects for this basic sample */
    for (i = 0; i < ARRAY_SIZE(Object_Table); i++) {
        switch (Object_Table[i].type) {
            case OBJECT_ANALOG_INPUT:
                Analog_Input_Create(Object_Table[i].instance);
                Analog_Input_Name_Set(
                    Object_Table[i].instance, Object_Table[i].name);
                Analog_Input_Present_Value_Set(
                    Object_Table[i].instance, Object_Table[i].value);
                Analog_Input_Units_Set(
                    Object_Table[i].instance, Object_Table[i].units);
                Analog_Input_Description_Set(
                    Object_Table[i].instance, Object_Table[i].description);
                break;
            case OBJECT_ANALOG_OUTPUT:
                Analog_Output_Create(Object_Table[i].instance);
                Analog_Output_Name_Set(
                    Object_Table[i].instance, Object_Table[i].name);
                Analog_Output_Relinquish_Default_Set(
                    Object_Table[i].instance, Object_Table[i].value);
                Analog_Output_Units_Set(
                    Object_Table[i].instance, Object_Table[i].units);
                Analog_Output_Description_Set(
                    Object_Table[i].instance, Object_Table[i].description);
                break;
            default:
                break;
        }
    }
    /* start the seconds cyclic timer */
    mstimer_set(&Sensor_Update_Timer, 1000);
    srand(0);
}

/**
 * @brief BACnet Project Task Handler
 * @param context [in] The context to pass to the callback function
 * @note This is called from the BACnet task
 */
static void BACnet_Object_Task(void *context)
{
    float temperature = 0.0f, change = 0.0f;

    (void)context;
    if (mstimer_expired(&Sensor_Update_Timer)) {
        mstimer_reset(&Sensor_Update_Timer);
        /* simulate a sensor reading, and update the BACnet object values */
        if (Analog_Input_Out_Of_Service(SENSOR_ID)) {
            return;
        }
        temperature = Analog_Input_Present_Value(SENSOR_ID);
        change = -1.0f + 2.0f * ((float)rand()) / RAND_MAX;
        temperature += change;
        Analog_Input_Present_Value_Set(SENSOR_ID, temperature);
    }
}

/**
 * @brief Store the BACnet data after a WriteProperty for object property
 * @param object_type - BACnet object type
 * @param object_instance - BACnet object instance
 * @param object_property - BACnet object property
 * @param array_index - BACnet array index
 * @param application_data - pointer to the data
 * @param application_data_len - length of the data
 */
static void BACnet_Basic_Store(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_ARRAY_INDEX array_index,
    uint8_t *application_data,
    int application_data_len)
{
    (void)array_index;
    (void)application_data;
    (void)application_data_len;
    debug_printf_stdout(
        "BACnet Store: %s[%lu]-%s\n", bactext_object_type_name(object_type),
        (unsigned long)object_instance, bactext_property_name(object_property));
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
    bacnet_basic_init_callback_set(BACnet_Object_Table_Init, NULL);
    bacnet_basic_task_callback_set(BACnet_Object_Task, NULL);
    bacnet_basic_store_callback_set(BACnet_Basic_Store);
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
