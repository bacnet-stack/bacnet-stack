/**
 * @file
 * @brief Example application using the BACnet Stack on a Raspberry Pi
 * with Blinkt! LEDs.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2023
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/iam.h"
#include "bacnet/dcc.h"
#include "bacnet/getevent.h"
#include "bacnet/lighting.h"
/* demo services */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/color_rgb.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/linear.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/version.h"
/* include the device object */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/object/lo.h"
#include "bacnet/basic/object/channel.h"
#include "bacnet/basic/object/color_object.h"
#include "bacnet/basic/object/color_temperature.h"
/* local includes */
#include "bacport.h"
#include "blinkt.h"

/* (Doxygen note: The next two lines pull all the following Javadoc
 *  into the ServerDemo module.) */
/** @addtogroup ServerDemo */
/*@{*/

/** Buffer used for receiving */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };
/* current version of the BACnet stack */
static const char *BACnet_Version = BACNET_VERSION_TEXT;
/* task timer for various BACnet timeouts */
static struct mstimer BACnet_Task_Timer;
/* task timer for TSM timeouts */
static struct mstimer BACnet_TSM_Timer;
/* task timer for address binding timeouts */
static struct mstimer BACnet_Address_Timer;
/* task timer for object functionality */
static struct mstimer BACnet_Object_Timer;

/** Initialize the handlers we will utilize.
 * @see Device_Init, apdu_set_unconfirmed_handler, apdu_set_confirmed_handler
 */
static void Init_Service_Handlers(void)
{
    Device_Init(NULL);
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    /* handle i-am to support binding to other devices */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_bind);
    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
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
        SERVICE_CONFIRMED_READ_RANGE, handler_read_range);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, handler_timesync_utc);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, handler_timesync);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_COV_NOTIFICATION, handler_ucov_notification);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    /* configure the cyclic timers */
    mstimer_set(&BACnet_Task_Timer, 1000UL);
    mstimer_set(&BACnet_TSM_Timer, 50UL);
    mstimer_set(&BACnet_Address_Timer, 60UL * 1000UL);
    mstimer_set(&BACnet_Object_Timer, 100UL);
}

/**
 * Clean up the Blinkt! interface
 */
static void blinkt_cleanup(void)
{
    blinkt_stop();
}

/**
 * @brief Callback for tracking value
 * @param  object_instance - object-instance number of the object
 * @param  old_value - Color temperature value prior to write
 * @param  value - Color temperature value of the write
 */
static void Lighting_Output_Write_Value_Handler(
    uint32_t object_instance, float old_value, float value)
{
    uint8_t index = 255;
    uint8_t brightness;

    (void)old_value;
    if (object_instance > 0) {
        index = object_instance - 1;
    }
    if (index < blinkt_led_count()) {
        /* brightness intensity from 0..31, 0=OFF, 1=dimmest, 31=brightest */
        if (isgreaterequal(value, 1.0)) {
            brightness = linear_interpolate(1.0, value, 100.0, 1, 31);
        } else {
            brightness = 0;
        }
        blinkt_set_pixel_brightness(index, brightness);
        printf("LED[%u]=%.1f%% (%u)\n", (unsigned)index, value,
            (unsigned)brightness);
    }
}

/**
 * @brief Callback for tracking value
 * @param  object_instance - object-instance number of the object
 * @param  old_value - Color temperature value prior to write
 * @param  value - Color temperature value of the write
 */
static void Color_Temperature_Write_Value_Handler(
    uint32_t object_instance, uint32_t old_value, uint32_t value)
{
    uint8_t red, green, blue;
    uint8_t index = 255;

    (void)old_value;
    if (object_instance > 0) {
        index = object_instance - 1;
    }
    if (index < blinkt_led_count()) {
        color_rgb_from_temperature(value, &red, &green, &blue);
        blinkt_set_pixel(index, red, green, blue);
        printf("%u Kelvin RGB[%u]=%u,%u,%u\n", (unsigned)value, (unsigned)index,
            (unsigned)red, (unsigned)green, (unsigned)blue);
    }
}

/**
 * @brief Callback for tracking value
 * @param  object_instance - object-instance number of the object
 * @param  old_value - BACnetXYColor value prior to write
 * @param  value - BACnetXYColor value of the write
 */
static void Color_Write_Value_Handler(uint32_t object_instance,
    BACNET_XY_COLOR *old_value,
    BACNET_XY_COLOR *value)
{
    uint8_t red, green, blue;
    float brightness_percent = 100.0;
    uint8_t index = 255;

    (void)old_value;
    if (object_instance > 0) {
        index = object_instance - 1;
    }
    if (index < blinkt_led_count()) {
        color_rgb_from_xy(&red, &green, &blue, value->x_coordinate,
            value->y_coordinate, brightness_percent);
        blinkt_set_pixel(index, red, green, blue);
        printf("x,y=%0.2f,%0.2f(%.1f%%) RGB[%u]=%u,%u,%u\n",
            value->x_coordinate, value->y_coordinate, brightness_percent,
            (unsigned)index, (unsigned)red, (unsigned)green, (unsigned)blue);
    }
}

/**
 * @brief Create the objects and configure the callbacks for BACnet objects
 */
static void bacnet_output_init(void)
{
    unsigned i = 0;
    uint8_t led_max;
    uint32_t object_instance = 1;
    BACNET_COLOR_COMMAND command = { 0 };
    BACNET_OBJECT_ID object_id;
    uint32_t light_channel_instance = 1;
    uint32_t color_channel_instance = 2;
    uint32_t temp_channel_instance = 3;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE member;

    Channel_Create(light_channel_instance);
    Channel_Name_Set(light_channel_instance, "Lights");
    Channel_Create(color_channel_instance);
    Channel_Name_Set(color_channel_instance, "Colors");
    Channel_Create(temp_channel_instance);
    Channel_Name_Set(temp_channel_instance, "Color-Temperatures");
    led_max = blinkt_led_count();
    for (i = 0; i < led_max; i++) {
        /* color */
        Color_Create(object_instance);
        Color_Write_Enable(object_instance);
        /* fade to black */
        Color_Command(object_instance, &command);
        command.operation = BACNET_COLOR_OPERATION_FADE_TO_COLOR;
        command.target.color.x_coordinate = 0.0;
        command.target.color.y_coordinate = 0.0;
        command.transit.fade_time = 0;
        Color_Command_Set(object_instance, &command);

        /* configure channel members */
        member.objectIdentifier.type = OBJECT_COLOR;
        member.objectIdentifier.instance = object_instance;
        member.propertyIdentifier = PROP_PRESENT_VALUE;
        member.arrayIndex = BACNET_ARRAY_ALL;
        member.deviceIdentifier.type = OBJECT_DEVICE;
        member.deviceIdentifier.instance = Device_Object_Instance_Number();
        Channel_Reference_List_Member_Element_Set(
            color_channel_instance, 1 + i, &member);

        /* color temperature */
        Color_Temperature_Create(object_instance);
        Color_Temperature_Write_Enable(object_instance);
        /* stop the color temperature */
        Color_Temperature_Command(object_instance, &command);
        command.operation = BACNET_COLOR_OPERATION_STOP;
        Color_Temperature_Command_Set(object_instance, &command);

        /* configure channel members */
        member.objectIdentifier.type = OBJECT_COLOR_TEMPERATURE;
        member.objectIdentifier.instance = object_instance;
        member.propertyIdentifier = PROP_PRESENT_VALUE;
        member.arrayIndex = BACNET_ARRAY_ALL;
        member.deviceIdentifier.type = OBJECT_DEVICE;
        member.deviceIdentifier.instance = Device_Object_Instance_Number();
        Channel_Reference_List_Member_Element_Set(
            temp_channel_instance, 1 + i, &member);

        /* lighting output */
        Lighting_Output_Create(object_instance);

        /* configure references */
        object_id.type = OBJECT_COLOR;
        object_id.instance = object_instance;
        Lighting_Output_Color_Reference_Set(object_instance, &object_id);

        /* configure channel members */
        member.objectIdentifier.type = OBJECT_LIGHTING_OUTPUT;
        member.objectIdentifier.instance = object_instance;
        member.propertyIdentifier = PROP_PRESENT_VALUE;
        member.arrayIndex = BACNET_ARRAY_ALL;
        member.deviceIdentifier.type = OBJECT_DEVICE;
        member.deviceIdentifier.instance = Device_Object_Instance_Number();
        Channel_Reference_List_Member_Element_Set(
            light_channel_instance, 1 + i, &member);

        object_instance = 1 + i;
    }
    Color_Write_Present_Value_Callback_Set(Color_Write_Value_Handler);
    Color_Temperature_Write_Present_Value_Callback_Set(
        Color_Temperature_Write_Value_Handler);
    Lighting_Output_Write_Present_Value_Callback_Set(
        Lighting_Output_Write_Value_Handler);
}

/**
 * @brief Print the terse usage info
 * @param filename - this application file name
 */
static void print_usage(const char *filename)
{
    printf("Usage: %s [device-instance]\n", filename);
    printf("       [--device N][--test]\n");
    printf("       [--version][--help]\n");
}

/**
 * @brief Print the verbose usage info
 * @param filename - this application file name
 */
static void print_help(const char *filename)
{
    printf("BACnet Blinkt! server device.\n");
    printf("device-instance:\n"
           "--device N:\n"
           "BACnet Device Object Instance number of this device.\n"
           "This number will be used when other devices\n"
           "try and bind with this device using Who-Is and\n"
           "I-Am services.\n");
    printf("\n");
    printf("--test:\n"
           "Test the Blinkt! RGB LEDs with a cycling pattern.\n");
    printf("\n");
    printf("Example:\n"
           "%s 9009\n",
        filename);
}

/** Main function of server demo.
 *
 * @see Device_Set_Object_Instance_Number, dlenv_init, Send_I_Am,
 *      datalink_receive, npdu_handler,
 *      dcc_timer_seconds, datalink_maintenance_timer,
 *      handler_cov_task,
 *      tsm_timer_milliseconds
 *
 * @param argc [in] Arg count.
 * @param argv [in] Takes one argument: the Device Instance #.
 * @return 0 on success.
 */
int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout_ms = 1;
    unsigned long seconds = 0;
    unsigned long milliseconds;
    bool blinkt_test = false;
    unsigned int target_args = 0;
    uint32_t device_id = BACNET_MAX_INSTANCE;
    int argi = 0;
    char *filename = NULL;

    filename = filename_remove_path(argv[0]);
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--help") == 0) {
            print_usage(filename);
            print_help(filename);
            return 0;
        }
        if (strcmp(argv[argi], "--version") == 0) {
            printf("%s %s\n", filename, BACNET_VERSION_TEXT);
            printf("Copyright (C) 2023 by Steve Karg and others.\n"
                   "This is free software; see the source for copying "
                   "conditions.\n"
                   "There is NO warranty; not even for MERCHANTABILITY or\n"
                   "FITNESS FOR A PARTICULAR PURPOSE.\n");
            return 0;
        }
        if (strcmp(argv[argi], "--device") == 0) {
            if (++argi < argc) {
                device_id = strtol(argv[argi], NULL, 0);
            }
        } else if (strcmp(argv[argi], "--test") == 0) {
            blinkt_test = true;
        } else {
            if (target_args == 0) {
                device_id = strtol(argv[argi], NULL, 0);
                target_args++;
            }
        }
    }
    if (device_id > BACNET_MAX_INSTANCE) {
        fprintf(stderr, "device=%u - not greater than %u\n", device_id,
            BACNET_MAX_INSTANCE);
        return 1;
    }
    Device_Set_Object_Instance_Number(device_id);
    printf("BACnet Raspberry Pi Blinkt! Demo\n"
           "BACnet Stack Version %s\n"
           "BACnet Device ID: %u\n"
           "Max APDU: %d\n",
        BACnet_Version, Device_Object_Instance_Number(), MAX_APDU);
    /* load any static address bindings to show up
       in our device bindings list */
    address_init();
    Init_Service_Handlers();
    dlenv_init();
    atexit(datalink_cleanup);
    blinkt_init();
    atexit(blinkt_cleanup);
    bacnet_output_init();
    /* configure the timeout values */
    /* broadcast an I-Am on startup */
    Send_I_Am(&Handler_Transmit_Buffer[0]);
    /* loop forever */
    for (;;) {
        /* input */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout_ms);
        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        if (mstimer_expired(&BACnet_Task_Timer)) {
            mstimer_reset(&BACnet_Task_Timer);
            /* 1 second tasks */
            dcc_timer_seconds(1);
            datalink_maintenance_timer(1);
            dlenv_maintenance_timer(1);
            handler_cov_timer_seconds(1);
        }
        if (mstimer_expired(&BACnet_TSM_Timer)) {
            mstimer_reset(&BACnet_TSM_Timer);
            tsm_timer_milliseconds(mstimer_interval(&BACnet_TSM_Timer));
        }
        handler_cov_task();
        if (mstimer_expired(&BACnet_Address_Timer)) {
            mstimer_reset(&BACnet_Address_Timer);
            /* address cache */
            seconds = mstimer_interval(&BACnet_Address_Timer) / 1000;
            address_cache_timer(seconds);
        }
        /* output/input */
        if (blinkt_test) {
            blinkt_test_task();
        } else {
            if (mstimer_expired(&BACnet_Object_Timer)) {
                mstimer_reset(&BACnet_Object_Timer);
                milliseconds = mstimer_interval(&BACnet_Object_Timer);
                Device_Timer(milliseconds);
                blinkt_show();
            }
        }
    }

    return 0;
}

/* @} */

/* End group ServerDemo */
