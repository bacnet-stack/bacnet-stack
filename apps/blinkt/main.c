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
#include "bacnet/bactext.h"
#include "bacnet/version.h"
#include "bacnet/bacdcode.h"
#include "bacnet/lighting.h"
/* BACnet System API */
#include "bacnet/basic/sys/color_rgb.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/linear.h"
#include "bacnet/basic/sys/mstimer.h"
/* BACnet basic server & services API */
#include "bacnet/basic/server/bacnet_basic.h"
#include "bacnet/basic/server/bacnet_port.h"
#include "bacnet/basic/services.h"
/* BACnet basic datalink API */
#include "bacnet/datalink/dlenv.h"
#include "bacnet/datalink/datalink.h"
/* BACnet basic object */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/object/lo.h"
#include "bacnet/basic/object/channel.h"
#include "bacnet/basic/object/color_object.h"
#include "bacnet/basic/object/color_temperature.h"
#include "bacnet/basic/object/timer.h"
/* local includes */
#include "bacport.h"
#include "blinkt.h"

/* some device customization */
static const char *Device_Name = "Blinkt! Server";
static uint32_t Device_ID = 260001;
/* task timer for Blinkt! RGB LED display */
static struct mstimer Blinkt_Task;
/* flag to enable the Blinkt! test */
static bool Blinkt_Test = false;
/* observer for WriteGroup notifications */
static BACNET_WRITE_GROUP_NOTIFICATION Write_Group_Notification;
/* observers for internal object to object writes */
static struct channel_write_property_notification
    Channel_Write_Property_Observer;
static struct timer_write_property_notification Timer_Write_Property_Observer;

/* object instances */
static uint32_t Light_Channel_Instance = 1;
static uint32_t Color_Channel_Instance = 2;
static uint32_t CCT_Channel_Instance = 3;
static uint32_t Vacancy_Timer_Instance = 1;
static uint32_t Vacancy_Timeout_Milliseconds = 30UL * 60UL * 1000UL;
static unsigned Default_Priority = 16;
/**
 * Clean up the Blinkt! interface
 */
static void blinkt_cleanup(void)
{
    blinkt_stop();
}

/**
 * @brief Log internal object to object WriteProperty calls
 * @param object_type The object type being written to
 * @param instance The object instance being written to
 * @param status The status of the WriteProperty
 * @param wp_data The WriteProperty data
 */
static void write_property_observer(
    BACNET_OBJECT_TYPE object_type,
    uint32_t instance,
    bool status,
    BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    unsigned i;
    char value_string[64 + 1] = { 0 };

    if (status) {
        printf(
            "WriteProperty: %s-%d to %s-%u %s@%u\n",
            bactext_object_type_name(object_type), instance,
            bactext_object_type_name(wp_data->object_type),
            wp_data->object_instance,
            bactext_property_name(wp_data->object_property), wp_data->priority);
    } else {
        for (i = 0; i < wp_data->application_data_len &&
             (i * 2) < sizeof(value_string) - 1;
             i++) {
            snprintf(
                &value_string[i * 2], 3, "%02X", wp_data->application_data[i]);
        }
        printf(
            "WriteProperty: %s-%d to %s-%u %s@%u %s %s-%s\n",
            bactext_object_type_name(object_type), instance,
            bactext_object_type_name(wp_data->object_type),
            wp_data->object_instance,
            bactext_property_name(wp_data->object_property), wp_data->priority,
            value_string, bactext_error_class_name(wp_data->error_class),
            bactext_error_code_name(wp_data->error_code));
    }
}

/**
 * @brief Log internal object to object WriteProperty calls
 * @param instance The object instance being written to
 * @param status The status of the WriteProperty
 * @param wp_data The WriteProperty data
 */
static void channel_write_property_observer(
    uint32_t instance, bool status, BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    write_property_observer(OBJECT_CHANNEL, instance, status, wp_data);
}

/**
 * @brief Log internal object to object WriteProperty calls
 * @param instance The object instance being written to
 * @param status The status of the WriteProperty
 * @param wp_data The WriteProperty data
 */
static void timer_write_property_observer(
    uint32_t instance, bool status, BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    write_property_observer(OBJECT_TIMER, instance, status, wp_data);
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
        printf(
            "LED[%u]=%.1f%% (%u)\n", (unsigned)index, value,
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
        printf(
            "%u Kelvin RGB[%u]=%u,%u,%u\n", (unsigned)value, (unsigned)index,
            (unsigned)red, (unsigned)green, (unsigned)blue);
    }
}

/**
 * @brief Callback for tracking value
 * @param  object_instance - object-instance number of the object
 * @param  old_value - BACnetXYColor value prior to write
 * @param  value - BACnetXYColor value of the write
 */
static void Color_Write_Value_Handler(
    uint32_t object_instance,
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
        color_rgb_from_xy(
            &red, &green, &blue, value->x_coordinate, value->y_coordinate,
            brightness_percent);
        blinkt_set_pixel(index, red, green, blue);
        printf(
            "x,y=%0.2f,%0.2f(%.1f%%) RGB[%u]=%u,%u,%u\n", value->x_coordinate,
            value->y_coordinate, brightness_percent, (unsigned)index,
            (unsigned)red, (unsigned)green, (unsigned)blue);
    }
}

/**
 * @brief Create the objects and configure the callbacks for BACnet objects
 */
static void BACnet_Object_Table_Init(void *context)
{
    unsigned i = 0;
    uint8_t led_max;
    uint32_t object_instance = 0, member_element = 0;
    BACNET_COLOR_COMMAND command = { 0 };
    BACNET_OBJECT_ID object_id = { 0 };
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE member = { 0 };
    BACNET_TIMER_STATE_CHANGE_VALUE timer_transition = { 0 };

    (void)context;
    Device_Set_Object_Instance_Number(Device_ID);
    Device_Object_Name_ANSI_Init(Device_Name);
    /* create the objects */
    Channel_Create(Light_Channel_Instance);
    Channel_Name_Set(Light_Channel_Instance, "Lights");
    Channel_Number_Set(Light_Channel_Instance, 1);
    Channel_Control_Groups_Element_Set(Light_Channel_Instance, 1, 1);
    Channel_Create(Color_Channel_Instance);
    Channel_Name_Set(Color_Channel_Instance, "Colors");
    Channel_Number_Set(Color_Channel_Instance, 2);
    Channel_Control_Groups_Element_Set(Color_Channel_Instance, 1, 2);
    Channel_Create(CCT_Channel_Instance);
    Channel_Name_Set(CCT_Channel_Instance, "Color-Temperatures");
    Channel_Number_Set(CCT_Channel_Instance, 3);
    Channel_Control_Groups_Element_Set(CCT_Channel_Instance, 1, 3);
    /* timer to automatically turn off the lights */
    Timer_Create(Vacancy_Timer_Instance);
    Timer_Name_Set(Vacancy_Timer_Instance, "Vacancy-Timer");
    Timer_Default_Timeout_Set(
        Vacancy_Timer_Instance, Vacancy_Timeout_Milliseconds);
    printf(
        "Vacancy timeout: %lu milliseconds\n",
        (unsigned long)Vacancy_Timeout_Milliseconds);
    /* to running */
    timer_transition.next = NULL;
    timer_transition.tag = BACNET_APPLICATION_TAG_REAL;
    timer_transition.type.Real = BACNET_LIGHTING_SPECIAL_VALUE_RESTORE_ON;
    Timer_State_Change_Value_Set(
        Vacancy_Timer_Instance, TIMER_TRANSITION_IDLE_TO_RUNNING,
        &timer_transition);
    Timer_State_Change_Value_Set(
        Vacancy_Timer_Instance, TIMER_TRANSITION_RUNNING_TO_RUNNING,
        &timer_transition);
    Timer_State_Change_Value_Set(
        Vacancy_Timer_Instance, TIMER_TRANSITION_EXPIRED_TO_RUNNING,
        &timer_transition);
    /* to expired */
    timer_transition.next = NULL;
    timer_transition.tag = BACNET_APPLICATION_TAG_REAL;
    timer_transition.type.Real = BACNET_LIGHTING_SPECIAL_VALUE_WARN_RELINQUISH;
    Timer_State_Change_Value_Set(
        Vacancy_Timer_Instance, TIMER_TRANSITION_RUNNING_TO_EXPIRED,
        &timer_transition);
    /* timer members */
    member.objectIdentifier.type = OBJECT_CHANNEL;
    member.objectIdentifier.instance = Light_Channel_Instance;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    member.deviceIdentifier.type = OBJECT_DEVICE;
    member.deviceIdentifier.instance = Device_ID;
    Timer_Reference_List_Member_Element_Add(Vacancy_Timer_Instance, &member);
    Timer_Priority_For_Writing_Set(Vacancy_Timer_Instance, Default_Priority);
    /* configure outputs and bindings */
    led_max = blinkt_led_count();
    for (i = 0; i < led_max; i++) {
        object_instance = 1 + i;
        member_element = 1 + i;
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
        member.deviceIdentifier.instance = Device_ID;
        Channel_Reference_List_Member_Element_Set(
            Color_Channel_Instance, member_element, &member);

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
        member.deviceIdentifier.instance = Device_ID;
        Channel_Reference_List_Member_Element_Set(
            CCT_Channel_Instance, member_element, &member);

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
        member.deviceIdentifier.instance = Device_ID;
        Channel_Reference_List_Member_Element_Set(
            Light_Channel_Instance, member_element, &member);
    }
    /* enable the callbacks for control */
    Color_Write_Present_Value_Callback_Set(Color_Write_Value_Handler);
    Color_Temperature_Write_Present_Value_Callback_Set(
        Color_Temperature_Write_Value_Handler);
    Lighting_Output_Write_Present_Value_Callback_Set(
        Lighting_Output_Write_Value_Handler);
    /* set the observer callbacks.  log the internal object-to-object writes */
    Channel_Write_Property_Observer.callback = channel_write_property_observer;
    Channel_Write_Property_Notification_Add(&Channel_Write_Property_Observer);
    Timer_Write_Property_Observer.callback = timer_write_property_observer;
    Timer_Write_Property_Notification_Add(&Timer_Write_Property_Observer);
    Write_Group_Notification.callback = Channel_Write_Group;
    handler_write_group_notification_add(&Write_Group_Notification);
    /* LEDs run at 0.1s intervals */
    bacnet_basic_task_object_timer_set(100);
    mstimer_set(&Blinkt_Task, 100);
}

/**
 * @brief BACnet object value initialization
 * @param color_name - color name string
 */
static void BACnet_Object_Value_Init(const char *color_name)
{
    BACNET_CHANNEL_VALUE value = { 0 };
    float x_coordinate = 1.0f, y_coordinate = 1.0f;
    uint8_t brightness = 0;

    /* update the lighting-output and color-temperature */
    if (color_rgb_xy_from_ascii(
            &x_coordinate, &y_coordinate, &brightness, color_name)) {
        printf(
            "Initial color: %s x=%.2f y=%.2f brightness=%u/255\n", color_name,
            x_coordinate, y_coordinate, (unsigned)brightness);
        /* Set the Color */
        value.tag = BACNET_APPLICATION_TAG_XY_COLOR;
        value.type.XY_Color.x_coordinate = x_coordinate;
        value.type.XY_Color.y_coordinate = y_coordinate;
        Channel_Present_Value_Set(Color_Channel_Instance, 16, &value);
        /* Set the Brightness */
        value.tag = BACNET_APPLICATION_TAG_REAL;
        value.type.Real =
            linear_interpolate(0.0f, brightness, 255.0f, 0.0f, 100.0f);
        Channel_Present_Value_Set(
            Light_Channel_Instance, Default_Priority, &value);
        /* start the vacancy timer */
        Timer_Running_Set(Vacancy_Timer_Instance, true);
    } else {
        printf("Initial color: %s unknown\n", color_name);
    }
}

static void BACnet_Object_Task(void *context)
{
    (void)context;
    /* input/process/output */
    if (Blinkt_Test) {
        blinkt_test_task();
    }
    if (mstimer_expired(&Blinkt_Task)) {
        mstimer_restart(&Blinkt_Task);
        /* run at the same interval as the BACnet basic objects */
        if (!Blinkt_Test) {
            blinkt_show();
        }
    }
}

/**
 * @brief Print the terse usage info
 * @param filename - this application file name
 */
static void print_usage(const char *filename)
{
    printf("Usage: %s [device-instance]\n", filename);
    printf("       [--device N][--test][--color COLOR][--vacancy MS]\n");
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
    printf(
        "--color:\n"
        "Default CSS color name from W3C, such as black, red, green, etc.\n");
    printf("\n");
    printf("--vacancy:\n"
           "Vacancy timeout in milliseconds.\n");
    printf("\n");
    printf("--test:\n"
           "Test the Blinkt! RGB LEDs with a cycling pattern.\n");
    printf("\n");
    printf(
        "Example:\n"
        "%s 9009\n",
        filename);
}

/** Main function of server demo.
 * @param argc [in] Arg count.
 * @param argv [in] Argument list.
 * @return 0 on success.
 */
int main(int argc, char *argv[])
{
    unsigned int target_args = 0;
    uint32_t device_id = BACNET_MAX_INSTANCE, vacancy_timeout = 0;
    int argi = 0;
    const char *filename = NULL;
    const char *color_name = "darkred";

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
            /* allow the device ID to be set */
            if (++argi < argc) {
                if (!bacnet_string_to_uint32(argv[argi], &device_id)) {
                    fprintf(stderr, "device-instance=%s invalid\n", argv[argi]);
                    return 1;
                }
                if (device_id > BACNET_MAX_INSTANCE) {
                    fprintf(stderr, "device-instance=%s invalid\n", argv[argi]);
                    return 1;
                } else {
                    Device_ID = device_id;
                }
            }
        } else if (strcmp(argv[argi], "--test") == 0) {
            /* test the hardware */
            Blinkt_Test = true;
        } else if (strcmp(argv[argi], "--color") == 0) {
            /* initial color */
            if (++argi < argc) {
                color_name = argv[argi];
            } else {
                fprintf(stderr, "Missing color name after --color\n");
                print_usage(filename);
                return 1;
            }
        } else if (strcmp(argv[argi], "--vacancy") == 0) {
            /* allow the device ID to be set */
            if (++argi < argc) {
                if (!bacnet_string_to_uint32(argv[argi], &vacancy_timeout)) {
                    fprintf(stderr, "vacancy=%s invalid\n", argv[argi]);
                    return 1;
                }
                Vacancy_Timeout_Milliseconds = vacancy_timeout;
            }
        } else {
            if (target_args == 0) {
                /* allow the device ID to be set */
                if (!bacnet_string_to_uint32(argv[argi], &device_id)) {
                    fprintf(stderr, "device-instance=%s invalid\n", argv[argi]);
                    return 1;
                }
                if (device_id > BACNET_MAX_INSTANCE) {
                    fprintf(stderr, "device-instance=%s invalid\n", argv[argi]);
                    return 1;
                } else {
                    Device_ID = device_id;
                }
                target_args++;
            } else if (target_args == 1) {
                /* allow the device name to be set */
                Device_Name = argv[argi];
                target_args++;
            }
        }
    }
    /* hardware init */
    blinkt_init();
    atexit(blinkt_cleanup);
    debug_printf_stdout("Blinkt! initialized\n");
    /* application init */
    bacnet_basic_init_callback_set(BACnet_Object_Table_Init, NULL);
    bacnet_basic_task_callback_set(BACnet_Object_Task, NULL);
    bacnet_basic_init();
    if (bacnet_port_init()) {
        /* OS based apps use DLENV for environment variables */
        dlenv_init();
        atexit(datalink_cleanup);
    }
    debug_printf_stdout("BACnet initialized\n");
    /* application info */
    printf(
        "BACnet Raspberry Pi Blinkt! Demo %s\n"
        "BACnet Stack Version %s\n"
        "BACnet Device ID: %u\n"
        "Max APDU: %d\n",
        Device_Application_Software_Version(), Device_Firmware_Revision(),
        Device_Object_Instance_Number(), MAX_APDU);
    /* operation */
    BACnet_Object_Value_Init(color_name);
    for (;;) {
        bacnet_basic_task();
        bacnet_port_task();
    }

    return 0;
}
