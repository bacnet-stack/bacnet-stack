/*
 * Copyright (C) 2020 Legrand North America, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <stdint.h>
#include <stdlib.h>

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack core API */
#include "bacnet/version.h"
#include "bacnet/basic/sys/mstimer.h"
/* BACnet Stack basic device API - see bacnet_basic/device.c for details */
#include "bacnet/basic/object/device.h"
/* BACnet Stack basic objects - also enable in prj.conf */
#include "bacnet/basic/object/ai.h"
#if (BACNET_PROTOCOL_REVISION >= 17)
#include "bacnet/basic/object/netport.h"
#endif
#include "bacnet_basic/bacnet_basic.h"

/* Logging module registration is already done in ports/zephyr/main.c */
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

static const uint32_t Device_Instance = 260123;
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
    LOG_INF("BACnet Stack Initialized");
    /* initialize objects for this basic sample */
    Device_Init(NULL);
    Device_Set_Object_Instance_Number(Device_Instance);
    Analog_Input_Create(Sensor_Instance);
    Analog_Input_Name_Set(Sensor_Instance, "Sensor");
    Analog_Input_Present_Value_Set(Sensor_Instance, 25.0f);
    LOG_INF("BACnet Device ID: %u", Device_Object_Instance_Number());
    /* start the seconds cyclic timer */
    mstimer_set(&Sensor_Update_Timer, 1000);
    srand(sys_rand32_get());
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
        change = -1.0f+2.0f*((float)rand())/RAND_MAX;
        temperature += change;
        Analog_Input_Present_Value_Set(Sensor_Instance, temperature);
    }
}

int main(void)
{
    LOG_INF("\n*** BACnet Profile B-SS Sample ***\n");
    LOG_INF("BACnet Stack Version " BACNET_VERSION_TEXT);
    LOG_INF("BACnet Stack Max APDU: %d", MAX_APDU);
    bacnet_basic_init_callback_set(BACnet_Smart_Sensor_Init_Handler, NULL);
    bacnet_basic_task_callback_set(BACnet_Smart_Sensor_Task_Handler, NULL);
    /* work happens in server module */
    for (;;) {
        k_sleep(K_MSEC(1000));
    }

    return 0;
}
