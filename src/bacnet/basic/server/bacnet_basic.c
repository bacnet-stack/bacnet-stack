/**
 * @file
 * @brief BACnet Stack initialization and task handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date March 2024
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack core API */
#include "bacnet/npdu.h"
#include "bacnet/dcc.h"
#include "bacnet/iam.h"
/* BACnet Stack basic services */
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
/* BACnet Stack basic objects */
#include "bacnet/basic/object/device.h"
/* objects */
#if (BACNET_PROTOCOL_REVISION >= 17)
#include "bacnet/basic/object/netport.h"
#endif
#include "bacnet/basic/object/device.h"
/* me */
#include "bacnet/basic/server/bacnet_basic.h"
#include "bacnet/basic/server/bacnet_port.h"

/* 1s timer for basic non-critical timed tasks */
static struct mstimer BACnet_Task_Timer;
/* task timer for object functionality */
static struct mstimer BACnet_Object_Timer;
/* uptimer for BACnet task */
static unsigned long BACnet_Uptime_Seconds;
/* packet counter for BACnet task */
static unsigned long BACnet_Packet_Count;
/* local Device ID to track changes */
static uint32_t Device_ID = 0xFFFFFFFF;
/* callbacks for custom features in BACnet thread */
static bacnet_basic_callback BACnet_Init_Callback;
static void *BACnet_Init_Context;
static bacnet_basic_callback BACnet_Task_Callback;
static void *BACnet_Task_Context;
static bacnet_basic_store_callback BACnet_Store_Callback;

/**
 * @brief Set the callback for the BACnet initialization
 * @param callback [in] The callback function called after initialization
 * @param context [in] The context to pass to the callback function
 */
void bacnet_basic_init_callback_set(
    bacnet_basic_callback callback, void *context)
{
    BACnet_Init_Callback = callback;
    BACnet_Init_Context = context;
}

/**
 * @brief BACnet Task Callback Handler
 */
static void bacnet_init_callback_handler(void)
{
    if (BACnet_Init_Callback) {
        BACnet_Init_Callback(BACnet_Init_Context);
    }
}

/**
 * @brief Set the callback for the BACnet Task
 * @param callback [in] The callback function to call during the task
 * @param context [in] The context to pass to the callback function
 */
void bacnet_basic_task_callback_set(
    bacnet_basic_callback callback, void *context)
{
    BACnet_Task_Callback = callback;
    BACnet_Task_Context = context;
}

/**
 * @brief BACnet Task Callback Handler
 */
static void bacnet_task_callback_handler(void)
{
    if (BACnet_Task_Callback) {
        BACnet_Task_Callback(BACnet_Task_Context);
    }
}

/**
 * @brief Set the callback for the BACnet WriteProperty Store
 * @param callback [in] The callback function to call after a succesful
 *  WriteProperty with the data in BACnet binary encoded format (small!)
 */
void bacnet_basic_store_callback_set(bacnet_basic_store_callback callback)
{
    BACnet_Store_Callback = callback;
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
static void bacnet_store_callback_handler(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_ARRAY_INDEX array_index,
    uint8_t *application_data,
    int application_data_len)
{
    if (BACnet_Store_Callback) {
        BACnet_Store_Callback(
            object_type, object_instance, object_property, array_index,
            application_data, application_data_len);
    }
}

/**
 * @brief Get the BACnet device uptime in seconds
 * @return The number of seconds the BACnet device has been running
 */
unsigned long bacnet_basic_uptime_seconds(void)
{
    return BACnet_Uptime_Seconds;
}

/**
 * @brief Get the BACnet device uptime in seconds
 * @return The number of seconds the BACnet device has been running
 */
unsigned long bacnet_basic_packet_count(void)
{
    return BACnet_Packet_Count;
}

/**
 * @brief Set the BACnet task device object timer interval
 * @param milliseconds [in] The number of milliseconds for the timer interval
 */
void bacnet_basic_task_object_timer_set(unsigned long milliseconds)
{
    mstimer_set(&BACnet_Object_Timer, milliseconds);
}

/**
 * @brief Store the BACnet data after a successful WriteProperty for
 * an object property
 * @param wp_data - pointer to the write property data
 */
bool bacnet_basic_write_property_store(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    BACNET_ARRAY_INDEX array_index = BACNET_ARRAY_ALL;

    if (property_list_bacnet_array_member(
            wp_data->object_type, wp_data->object_property)) {
        array_index = wp_data->array_index;
    } else if (wp_data->object_property == PROP_PRESENT_VALUE) {
        /* indirect Priority_Array write */
        if (Device_Objects_Property_List_Member(
                wp_data->object_type, wp_data->object_instance,
                PROP_PRIORITY_ARRAY)) {
            /* store the priority as an array index to be used on restore */
            array_index = wp_data->priority;
        }
    } else {
        array_index = wp_data->array_index;
    }
    bacnet_store_callback_handler(
        wp_data->object_type, wp_data->object_instance,
        wp_data->object_property, array_index, wp_data->application_data,
        wp_data->application_data_len);

    return true;
}

/**
 * @brief Initialize the BACnet device object, the service handlers, and timers
 */
void bacnet_basic_init(void)
{
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
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE, handler_write_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);
    /* handle communication so we can shutup when asked, or restart */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);
    /* start the 1 second timer for non-critical cyclic tasks */
    mstimer_set(&BACnet_Task_Timer, 1000L);
    /* start the timer for more time sensitive object specific cyclic tasks */
    if (mstimer_interval(&BACnet_Object_Timer) == 0) {
        mstimer_set(&BACnet_Object_Timer, 100UL);
    }
    Device_Write_Property_Store_Callback_Set(bacnet_basic_write_property_store);
    Device_Init(NULL);
    /* initialize user data in this thread */
    bacnet_init_callback_handler();
}

/* local buffer for incoming PDUs to process */
static uint8_t PDUBuffer[MAX_MPDU];

/**
 * @brief non-blocking BACnet task
 */
void bacnet_basic_task(void)
{
    bool hello_world = false;
    uint16_t pdu_len = 0;
    BACNET_ADDRESS src = { 0 };
    uint32_t elapsed_milliseconds = 0;
    uint32_t elapsed_seconds = 0;

    /* hello, World! */
    if (Device_ID != Device_Object_Instance_Number()) {
        Device_ID = Device_Object_Instance_Number();
        hello_world = true;
    }
    if (hello_world) {
        Send_I_Am(&Handler_Transmit_Buffer[0]);
    }
    /* handle non-time-critical cyclic tasks */
    if (mstimer_expired(&BACnet_Task_Timer)) {
        /* 1 second tasks */
        mstimer_reset(&BACnet_Task_Timer);
        /* presume that the elapsed time is the interval time */
        elapsed_milliseconds = mstimer_interval(&BACnet_Task_Timer);
        elapsed_seconds = elapsed_milliseconds / 1000;
        BACnet_Uptime_Seconds += elapsed_seconds;
        dcc_timer_seconds(elapsed_seconds);
        datalink_maintenance_timer(elapsed_seconds);
        handler_cov_timer_seconds(elapsed_seconds);
    }
    while (!handler_cov_fsm()) {
        /* waiting for COV processing to be IDLE */
    }
    /* object specific cyclic tasks */
    if (mstimer_expired(&BACnet_Object_Timer)) {
        elapsed_milliseconds = mstimer_elapsed(&BACnet_Object_Timer);
        mstimer_restart(&BACnet_Object_Timer);
        Device_Timer(elapsed_milliseconds);
    }
    /* handle the messaging */
    pdu_len = datalink_receive(&src, &PDUBuffer[0], sizeof(PDUBuffer), 0);
    if (pdu_len) {
        npdu_handler(&src, &PDUBuffer[0], pdu_len);
        BACnet_Packet_Count++;
    }
    /* call user task in this thread */
    bacnet_task_callback_handler();
}
