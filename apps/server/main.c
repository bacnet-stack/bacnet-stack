/**
 * @file
 * @brief command line tool that simulates a BACnet server device on the
 * network using the BACnet Stack and all the example object types.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bactext.h"
#include "bacnet/bacenum.h"
#include "bacnet/dcc.h"
#include "bacnet/getevent.h"
#include "bacnet/iam.h"
#include "bacnet/npdu.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/datetime.h"
/* include the device object */
#include "bacnet/basic/object/device.h"
/* objects that have tasks inside them */
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/ao.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bo.h"
#include "bacnet/basic/object/bv.h"

#include "bacnet/basic/object/calendar.h"

#include "bacnet/basic/object/structured_view.h"
#include "bacnet/basic/object/trendlog.h"
#include "bacnet/basic/object/ms-input.h"

#if (BACNET_PROTOCOL_REVISION >= 14)
#include "bacnet/basic/object/lo.h"
#include "bacnet/basic/object/channel.h"
#endif
#if (BACNET_PROTOCOL_REVISION >= 24)
#include "bacnet/basic/object/color_object.h"
#include "bacnet/basic/object/color_temperature.h"
#endif
#include "bacnet/basic/object/lc.h"
#include "bacnet/basic/object/trendlog.h"
#include "bacnet/basic/object/structured_view.h"
#if defined(INTRINSIC_REPORTING)
#include "bacnet/basic/object/nc.h"
#endif /* defined(INTRINSIC_REPORTING) */
#if defined(BACFILE)
#include "bacnet/basic/object/bacfile.h"
#endif /* defined(BACFILE) */
#if defined(BAC_UCI)
#include "bacnet/basic/ucix/ucix.h"
#endif /* defined(BAC_UCI) */

/* (Doxygen note: The next two lines pull all the following Javadoc
 *  into the ServerDemo module.) */
/** @addtogroup ServerDemo */
/*@{*/

/* current version of the BACnet stack */
static const char *BACnet_Version = BACNET_VERSION_TEXT;
/* task timer for various BACnet timeouts */
static struct mstimer BACnet_Task_Timer;
/* task timer for TSM timeouts */
static struct mstimer BACnet_TSM_Timer;
/* task timer for address binding timeouts */
static struct mstimer BACnet_Address_Timer;
#if defined(INTRINSIC_REPORTING)
/* task timer for notification recipient timeouts */
static struct mstimer BACnet_Notification_Timer;
#endif
/* task timer for objects */
static struct mstimer BACnet_Object_Timer;
/** Buffer used for receiving */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* configure an example structured view object subordinate list */
#if (BACNET_PROTOCOL_REVISION >= 4)
#define LIGHTING_OBJECT_WATTS OBJECT_ACCUMULATOR
#else
#define LIGHTING_OBJECT_WATTS OBJECT_ANALOG_INPUT
#endif
#if (BACNET_PROTOCOL_REVISION >= 6)
#define LIGHTING_OBJECT_ADR OBJECT_LOAD_CONTROL
#else
#define LIGHTING_OBJECT_ADR OBJECT_MULTISTATE_OUTPUT
#endif
#if (BACNET_PROTOCOL_REVISION >= 6)
#define LIGHTING_OBJECT_ADR OBJECT_LOAD_CONTROL
#else
#define LIGHTING_OBJECT_ADR OBJECT_MULTISTATE_OUTPUT
#endif
#if (BACNET_PROTOCOL_REVISION >= 14)
#define LIGHTING_OBJECT_SCENE OBJECT_CHANNEL
#define LIGHTING_OBJECT_LIGHT OBJECT_LIGHTING_OUTPUT
#else
#define LIGHTING_OBJECT_SCENE OBJECT_ANALOG_VALUE
#define LIGHTING_OBJECT_LIGHT OBJECT_ANALOG_OUTPUT
#endif
#if (BACNET_PROTOCOL_REVISION >= 16)
#define LIGHTING_OBJECT_RELAY OBJECT_BINARY_LIGHTING_OUTPUT
#else
#define LIGHTING_OBJECT_RELAY OBJECT_BINARY_OUTPUT
#endif

static BACNET_SUBORDINATE_DATA Lighting_Subordinate[] = {
    { 0, LIGHTING_OBJECT_WATTS, 1, "watt-hours", 0, 0, NULL },
    { 0, LIGHTING_OBJECT_ADR, 1, "demand-response", 0, 0, NULL },
    { 0, LIGHTING_OBJECT_SCENE, 1, "scene", 0, 0, NULL },
    { 0, LIGHTING_OBJECT_LIGHT, 1, "light", 0, 0, NULL },
    { 0, LIGHTING_OBJECT_RELAY, 1, "relay", 0, 0, NULL },
#if (BACNET_PROTOCOL_REVISION >= 24)
    { 0, OBJECT_COLOR, 1, "color", 0, 0, NULL },
    { 0, OBJECT_COLOR_TEMPERATURE, 1, "color-temperature", 0, 0, NULL },
#endif
};
static BACNET_WRITE_GROUP_NOTIFICATION Write_Group_Notification = { 0 };

/**
 * @brief Update the strcutured view static data with device ID and linked lists
 * @param device_id Device Instance to assign to every subordinate
 */
static void Structured_View_Update(void)
{
    uint32_t device_id, instance;
    BACNET_DEVICE_OBJECT_REFERENCE represents = { 0 };
    size_t i;

    device_id = Device_Object_Instance_Number();
    for (i = 0; i < ARRAY_SIZE(Lighting_Subordinate); i++) {
        /* link the lists */
        if (i < (ARRAY_SIZE(Lighting_Subordinate) - 1)) {
            Lighting_Subordinate[i].next = &Lighting_Subordinate[i + 1];
        }
        /* update the device instance to internal */
        Lighting_Subordinate[i].Device_Instance = device_id;
        /* update the common node data */
        Lighting_Subordinate[i].Node_Type = BACNET_NODE_ROOM;
        Lighting_Subordinate[i].Relationship = BACNET_RELATIONSHIP_CONTAINS;
    }
    instance = Structured_View_Index_To_Instance(0);
    Structured_View_Subordinate_List_Set(instance, Lighting_Subordinate);
    /* In some cases, the Structure View object will abstractly represent
       this entity by itself, and this property will either be absent,
       unconfigured, or point to itself. */
    represents.deviceIdentifier.type = OBJECT_NONE;
    represents.deviceIdentifier.instance = BACNET_MAX_INSTANCE;
    represents.objectIdentifier.type = OBJECT_DEVICE;
    represents.objectIdentifier.instance = Device_Object_Instance_Number();
    Structured_View_Represents_Set(instance, &represents);
    Structured_View_Node_Type_Set(instance, BACNET_NODE_ROOM);
}

/** Initialize the handlers we will utilize.
 * @see Device_Init, apdu_set_unconfirmed_handler, apdu_set_confirmed_handler
 */
static void Init_Service_Handlers(void)
{
    BACNET_CREATE_OBJECT_DATA object_data = { 0 };
    unsigned int i = 0;

    Device_Init(NULL);
    /* create some dynamically created objects as examples */
    object_data.object_instance = BACNET_MAX_INSTANCE;
    for (i = 0; i <= BACNET_OBJECT_TYPE_RESERVED_MIN; i++) {
        object_data.object_type = i;
        if (Device_Create_Object(&object_data)) {
            printf(
                "Created object %s-%u\n", bactext_object_type_name(i),
                (unsigned)object_data.object_instance);
        }
    }
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_WHO_IS, handler_who_is_who_am_i_unicast);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
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
#if defined(BACFILE)
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_ATOMIC_READ_FILE, handler_atomic_read_file);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_ATOMIC_WRITE_FILE, handler_atomic_write_file);
#endif
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, handler_timesync_utc);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, handler_timesync);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_YOU_ARE, handler_you_are_json_print);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_COV_NOTIFICATION, handler_ucov_notification);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    /* handle the data coming back from private requests */
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_PRIVATE_TRANSFER,
        handler_unconfirmed_private_transfer);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_WRITE_GROUP, handler_write_group);
    /* add WriteGroup iterator to the Channel objects */
    Write_Group_Notification.callback = Channel_Write_Group;
    handler_write_group_notification_add(&Write_Group_Notification);
#if defined(INTRINSIC_REPORTING)
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM, handler_alarm_ack);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_GET_EVENT_INFORMATION, handler_get_event_information);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_GET_ALARM_SUMMARY, handler_get_alarm_summary);
#endif /* defined(INTRINSIC_REPORTING) */
#if defined(BACNET_TIME_MASTER)
    handler_timesync_init();
#endif
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_CREATE_OBJECT, handler_create_object);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DELETE_OBJECT, handler_delete_object);
    /* configure the cyclic timers */
    mstimer_set(&BACnet_Task_Timer, 1000UL);
    mstimer_set(&BACnet_TSM_Timer, 50UL);
    mstimer_set(&BACnet_Address_Timer, 60UL * 1000UL);
    mstimer_set(&BACnet_Object_Timer, 100UL);
#if defined(INTRINSIC_REPORTING)
    mstimer_set(&BACnet_Notification_Timer, NC_RESCAN_RECIPIENTS_SECS * 1000UL);
#endif
    /*Configre Basic Objects*/ 
    //Calendar_Create(1); schon da
    Calendar_Name_Set(1,"ORTS-BAS_480_ASP42_#####_###_AS~01_#####_CAL01");
    Calendar_Description_Set(1, "Gebäudeautomation Automationsschwerpunkt AS Kalender Feiertage");
    Calendar_Create(2);
    Calendar_Name_Set(2,"ORTS-BAS_480_ASP42_#####_###_AS~01_#####_CAL02");
    Calendar_Description_Set(2, "Gebäudeautomation Automationsschwerpunkt AS Kalender Ferien");

    //Binary_Input_Create(1); schon da
    Binary_Input_Name_Set(1, "ORTS-BAS_480_ASP42_#####_###_SSK01_GS~01_RMA01");
    Binary_Input_Description_Set(1, "Gebäudeautomation Automationsschwerpunkt Schaltschrank Öffnungsüberwachung");
    Binary_Input_Inactive_Text_Set(1,"Zu"); Binary_Input_Active_Text_Set(1,"Auf"); 
    Binary_Input_Polarity_Set(1,POLARITY_NORMAL);
    Binary_Input_Alarm_Value_Set(1,BINARY_ACTIVE);
    Binary_Input_Event_Detection_Enable_Set(1,true);
    Binary_Input_Event_Enable_Set(1, 0b111);

    /* Configure Ventil */
    Analog_Input_Create(100);
    Analog_Input_Name_Set(100, "ORTS-BAS_420_VTA01_STH01_HZV_VEN01_MOT01_RW~01");
    Analog_Input_Description_Set(100, "Verteilanlage 1 Statische Heizung 1 Heizwasser Vorlauf Ventil Rückführwert");
    Analog_Input_Units_Set(100, UNITS_PERCENT);
    Analog_Input_Present_Value_Set(100, 22.5);

    Analog_Output_Create(100);
    Analog_Output_Name_Set(100, "ORTS-BAS_420_VTA01_STH01_HZV_VEN01_MOT01_ST~01");
    Analog_Output_Description_Set(100, "Verteilanlage 1 Statische Heizung 1 Heizwasser Vorlauf Ventil Stellsignal");
    Analog_Output_Units_Set(100, UNITS_PERCENT);
    Analog_Output_Present_Value_Set(100, 22.8, BACNET_MAX_PRIORITY);

    Binary_Input_Create(100);
    Binary_Input_Name_Set(100, "ORTS-BAS_420_VTA01_STH01_HZV_VEN01_LVB01_HDG01");
    Binary_Input_Description_Set(100, "Verteilanlage 1 Statische Heizung 1 Heizwasser Vorlauf Ventil LVB Hand stellen");
    Binary_Input_Inactive_Text_Set(100,"Auto"); Binary_Input_Active_Text_Set(100,"Hand"); 
    Binary_Input_Polarity_Set(100,POLARITY_NORMAL);
    Binary_Input_Alarm_Value_Set(100,BINARY_INACTIVE);
    Binary_Input_Event_Detection_Enable_Set(100,true);
    Binary_Input_Event_Enable_Set(100, 0b111);

    Structured_View_Create(100);
    Structured_View_Name_Set(100, "ORTS-BAS_420_VTA01_STH01_HZV_VEN01_#####_SV~01");
    Structured_View_Description_Set(100,"Verteilanlage 1 Statische Heizung 1 Heizwasser Vorlauf Ventil");

    //bo_instance = Binary_Output_Create(0);
}

static void print_usage(const char *filename)
{
    printf("Usage: %s [device-instance [device-name]]\n", filename);
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf("Simulate a BACnet server device\n"
           "device-instance:\n"
           "BACnet Device Object Instance number that you are\n"
           "trying simulate.\n"
           "device-name:\n"
           "The Device object-name is the text name for the device.\n"
           "\nExample:\n");
    printf(
        "To simulate Device 123, use the following command:\n"
        "%s 123\n",
        filename);
    printf(
        "To simulate Device 123 named Fred, use following command:\n"
        "%s 123 Fred\n",
        filename);
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
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 1; /* milliseconds */
    uint32_t elapsed_milliseconds = 0;
    uint32_t elapsed_seconds = 0;
    BACNET_CHARACTER_STRING DeviceName;
    uint32_t device_id = 0xFFFFFFFF;
#if defined(BACNET_TIME_MASTER)
    BACNET_DATE_TIME bdatetime;
#endif
#if defined(BAC_UCI)
    int uciId = 0;
    struct uci_context *ctx;
#endif
    int argi = 0;
    const char *filename = NULL;

    filename = filename_remove_path(argv[0]);
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--help") == 0) {
            print_usage(filename);
            print_help(filename);
            return 0;
        }
        if (strcmp(argv[argi], "--version") == 0) {
            printf("%s %s\n", filename, BACNET_VERSION_TEXT);
            printf("Copyright (C) 2014 by Steve Karg and others.\n"
                   "This is free software; see the source for copying "
                   "conditions.\n"
                   "There is NO warranty; not even for MERCHANTABILITY or\n"
                   "FITNESS FOR A PARTICULAR PURPOSE.\n");
            return 0;
        }
    }
#if defined(BAC_UCI)
    ctx = ucix_init("bacnet_dev");
    if (!ctx) {
        fprintf(stderr, "Failed to load config file bacnet_dev\n");
    }
    uciId = ucix_get_option_int(ctx, "bacnet_dev", "0", "Id", 0);
    if (uciId != 0) {
        Device_Set_Object_Instance_Number(uciId);
    } else {
#endif /* defined(BAC_UCI) */
        /* allow the device ID to be set */
        if (argc > 1) {
            Device_Set_Object_Instance_Number(strtol(argv[1], NULL, 0));
        }

#if defined(BAC_UCI)
    }
    ucix_cleanup(ctx);
#endif /* defined(BAC_UCI) */

    printf(
        "BACnet Server Demo\n"
        "BACnet Stack Version %s\n"
        "BACnet Device ID: %u\n"
        "Max APDU: %d\n",
        BACnet_Version, Device_Object_Instance_Number(), MAX_APDU);
    /* load any static address bindings to show up
       in our device bindings list */
    address_init();
    Init_Service_Handlers();
    /* initialize timesync callback function. */
    handler_timesync_set_callback_set(&datetime_timesync);

#if defined(BAC_UCI)
    const char *uciname;
    ctx = ucix_init("bacnet_dev");
    if (!ctx) {
        fprintf(stderr, "Failed to load config file bacnet_dev\n");
    }
    uciname = ucix_get_option(ctx, "bacnet_dev", "0", "Name");
    if (uciname != 0) {
        Device_Object_Name_ANSI_Init(uciname);
    } else {
#endif /* defined(BAC_UCI) */
        if (argc > 2) {
            Device_Object_Name_ANSI_Init(argv[2]);
        }
#if defined(BAC_UCI)
    }
    ucix_cleanup(ctx);
#endif /* defined(BAC_UCI) */
    if (Device_Object_Name(Device_Object_Instance_Number(), &DeviceName)) {
        printf("BACnet Device Name: %s\n", DeviceName.value);
    }
    dlenv_init();
    atexit(datalink_cleanup);
#if BACNET_PROTOCOL_REVISION >= 22
    if (Device_Object_Instance_Number() == BACNET_MAX_INSTANCE) {
        apdu_set_unconfirmed_handler(
            SERVICE_UNCONFIRMED_YOU_ARE, handler_you_are_device_id_set);
        /* The Who-Am-I service is used by a sending BACnet-user
           to indicate that it requires identity configuration
           via the You-Are service. */
        Send_Who_Am_I_Broadcast(
            Device_Vendor_Identifier(), Device_Model_Name(),
            Device_Serial_Number());
    }
#endif
    /* loop forever */
    for (;;) {
        if (device_id != Device_Object_Instance_Number()) {
            device_id = Device_Object_Instance_Number();
            /* update structured view with this device instance */
            Structured_View_Update();
            if (Device_Object_Instance_Number() != BACNET_MAX_INSTANCE) {
                /* broadcast an I-Am on startup */
                Send_I_Am(&Handler_Transmit_Buffer[0]);
            }
        }
        /* input */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        if (mstimer_expired(&BACnet_Task_Timer)) {
            mstimer_reset(&BACnet_Task_Timer);
            elapsed_milliseconds = mstimer_interval(&BACnet_Task_Timer);
            elapsed_seconds = elapsed_milliseconds / 1000;
            /* 1 second tasks */
            dcc_timer_seconds(elapsed_seconds);
            datalink_maintenance_timer(elapsed_seconds);
            dlenv_maintenance_timer(elapsed_seconds);
            handler_cov_timer_seconds(elapsed_seconds);
            trend_log_timer(elapsed_seconds);
#if defined(INTRINSIC_REPORTING)
            Device_local_reporting();
#endif
#if defined(BACNET_TIME_MASTER)
            Device_getCurrentDateTime(&bdatetime);
            handler_timesync_task(&bdatetime);
#endif
        }
        if (mstimer_expired(&BACnet_TSM_Timer)) {
            mstimer_reset(&BACnet_TSM_Timer);
            elapsed_milliseconds = mstimer_interval(&BACnet_TSM_Timer);
            tsm_timer_milliseconds(elapsed_milliseconds);
        }
        if (mstimer_expired(&BACnet_Address_Timer)) {
            mstimer_reset(&BACnet_Address_Timer);
            elapsed_milliseconds = mstimer_interval(&BACnet_Address_Timer);
            elapsed_seconds = elapsed_milliseconds / 1000;
            address_cache_timer(elapsed_seconds);
        }
        handler_cov_task();
#if defined(INTRINSIC_REPORTING)
        if (mstimer_expired(&BACnet_Notification_Timer)) {
            mstimer_reset(&BACnet_Notification_Timer);
            Notification_Class_find_recipient();
        }
#endif
        /* output */
        if (mstimer_expired(&BACnet_Object_Timer)) {
            mstimer_reset(&BACnet_Object_Timer);
            elapsed_milliseconds = mstimer_interval(&BACnet_Object_Timer);
            Device_Timer(elapsed_milliseconds);
        }
    }

    return 0;
}

/* @} */

/* End group ServerDemo */
