/**
 * @file
 * @brief Example server application using the BACnet Stack on a Raspberry Pi
 * with a PiFace Digital I/O card.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date January 2023
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
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/dcc.h"
#include "bacnet/getevent.h"
#include "bacport.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/version.h"
/* include the device object */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/blo.h"
#include "bacnet/basic/object/bo.h"

/** @file server/main.c  Example server application using the BACnet Stack. */

/* number of binary outputs on the PiFace card */
#define PIFACE_OUTPUTS_MAX 8

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
/* track the state of of the output */
static bool PiFace_Output_State[PIFACE_OUTPUTS_MAX];

#ifndef BUILD_PIPELINE
#include "pifacedigital.h"
#else
#define pifacedigital_digital_write(a, b) printf("PiFace write[%u]=%d\n", a, b)
#define pifacedigital_digital_read(a) printf("PiFace read[%u]\n", a)
#define pifacedigital_open(a) printf("PiFace Open=%d\n", a)
#define pifacedigital_close(a) printf("PiFace Close=%d\n", a)
#endif

/**
 * @brief output write value request
 * @param index - 0..N index of the output
 * @param value - value of the write ON or OFF
 */
static void piface_write_output(int index, BACNET_BINARY_LIGHTING_PV value)
{
    if (index < PIFACE_OUTPUTS_MAX) {
        if (value == BINARY_LIGHTING_PV_OFF) {
            pifacedigital_digital_write(index, 0);
            PiFace_Output_State[index] = false;
            printf("OUTPUT[%u]=OFF\n", index);
        } else if (value == BINARY_LIGHTING_PV_ON) {
            pifacedigital_digital_write(index, 1);
            PiFace_Output_State[index] = true;
            printf("OUTPUT[%u]=ON\n", index);
        }
    }
}

/**
 * @brief Callback for write value request
 * @param  object_instance - object-instance number of the object
 * @param  old_value - value prior to write
 * @param  value - value of the write
 */
static void Binary_Lighting_Output_Write_Value_Handler(
    uint32_t object_instance,
    BACNET_BINARY_LIGHTING_PV old_value,
    BACNET_BINARY_LIGHTING_PV value)
{
    unsigned index;

    index = Binary_Lighting_Output_Instance_To_Index(object_instance);
    if (index < PIFACE_OUTPUTS_MAX) {
        printf(
            "BLO-WRITE: OUTPUT[%u]=%d present=%d feedback=%d target=%d\n",
            index, (int)value,
            (int)Binary_Lighting_Output_Present_Value(object_instance),
            (int)old_value,
            (int)Binary_Lighting_Output_Lighting_Command_Target_Value(
                object_instance));
        piface_write_output(index, value);
    }
}

/**
 * @brief Callback for blink warning notification
 * @param  object_instance - object-instance number of the object
 */
static void Binary_Lighting_Output_Blink_Warn_Handler(uint32_t object_instance)
{
    unsigned index;

    index = Binary_Lighting_Output_Instance_To_Index(object_instance);
    if (index < PIFACE_OUTPUTS_MAX) {
        /* blink is just toggle on/off every one second */
        if (PiFace_Output_State[index]) {
            printf("BLO-BLINK: OUTPUT[%u]=%d\n", index, BINARY_LIGHTING_PV_OFF);
            piface_write_output(index, BINARY_LIGHTING_PV_OFF);
        } else {
            printf("BLO-BLINK: OUTPUT[%u]=%d\n", index, BINARY_LIGHTING_PV_ON);
            piface_write_output(index, BINARY_LIGHTING_PV_ON);
        }
    }
}

/** Initialize the handlers we will utilize.
 * @see Device_Init, apdu_set_unconfirmed_handler, apdu_set_confirmed_handler
 */
static void Init_Service_Handlers(void)
{
    uint32_t i = 0;
    uint32_t object_instance;

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
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    /* configure the cyclic timers */
    mstimer_set(&BACnet_Task_Timer, 1000UL);
    mstimer_set(&BACnet_TSM_Timer, 50UL);
    mstimer_set(&BACnet_Address_Timer, 60UL * 1000UL);
    mstimer_set(&BACnet_Object_Timer, 1000UL);
    /* create some objects */
    for (i = 0; i < PIFACE_OUTPUTS_MAX; i++) {
        object_instance = 1 + i;
        Binary_Lighting_Output_Create(object_instance);
        Binary_Output_Create(object_instance);
    }
    Binary_Lighting_Output_Write_Value_Callback_Set(
        Binary_Lighting_Output_Write_Value_Handler);
    Binary_Lighting_Output_Blink_Warn_Callback_Set(
        Binary_Lighting_Output_Blink_Warn_Handler);
}

static void piface_init(void)
{
    int hw_addr = 0; /**< PiFaceDigital hardware address  */
#ifdef PIFACE_INTERRUPT_ENABLE
    int intenable = 1; /**< Whether or not interrupts are enabled  */
#endif

    /**
     * Open piface digital SPI connection(s)
     */
    printf("Opening piface digital connection at location %d\n", hw_addr);
    pifacedigital_open(hw_addr);

#ifdef PIFACE_INTERRUPT_ENABLE
    /**
     * Enable interrupt processing (only required for all
     * blocking/interrupt methods)
     */
    intenable = pifacedigital_enable_interrupts();
    if (intenable == 0) {
        printf("Interrupts enabled.\n");
    } else {
        printf("Could not enable interrupts.  "
               "Try running using sudo to enable PiFaceDigital interrupts.\n");
    }
#endif
}

/* track the Piface pin state to react on changes only */
static bool PiFace_Pin_Status[MAX_BINARY_INPUTS];

/**
 * Clean up the PiFace interface
 */
static void piface_cleanup(void)
{
    pifacedigital_close(0);
}

/**
 * Perform a periodic task for the PiFace card
 */
static void piface_task(void)
{
    unsigned i = 0;
    BACNET_BINARY_PV present_value = BINARY_INACTIVE;
    bool pin_status = false;
    uint32_t object_instance;

    for (i = 0; i < MAX_BINARY_INPUTS; i++) {
        if (!Binary_Input_Out_Of_Service(i)) {
            present_value = Binary_Input_Present_Value(i);
            pin_status = false;
            if (pifacedigital_digital_read(i)) {
                pin_status = true;
            }
            if (pin_status != PiFace_Pin_Status[i]) {
                PiFace_Pin_Status[i] = pin_status;
                if (pin_status) {
                    /* toggle the input only when button is pressed */
                    if (present_value == BINARY_INACTIVE) {
                        present_value = BINARY_ACTIVE;
                    } else {
                        present_value = BINARY_INACTIVE;
                    }
                    Binary_Input_Present_Value_Set(i, present_value);
                }
            }
        }
    }
    for (i = 0; i < PIFACE_OUTPUTS_MAX; i++) {
        object_instance = Binary_Output_Index_To_Instance(i);
        if (Binary_Output_Valid_Instance(object_instance)) {
            if (!Binary_Output_Out_Of_Service(object_instance)) {
                present_value = Binary_Output_Present_Value(object_instance);
                if (present_value == BINARY_INACTIVE) {
                    if (PiFace_Output_State[i]) {
                        printf(
                            "BO-WRITE: OUTPUT[%u]=%d\n", i,
                            BINARY_LIGHTING_PV_OFF);
                        piface_write_output(i, BINARY_LIGHTING_PV_OFF);
                    }
                } else {
                    if (!PiFace_Output_State[i]) {
                        printf(
                            "BO-WRITE: OUTPUT[%u]=%d\n", i,
                            BINARY_LIGHTING_PV_OFF);
                        piface_write_output(i, BINARY_LIGHTING_PV_ON);
                    }
                }
            }
        }
    }
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
    unsigned timeout_ms = 1; /* milliseconds */
    unsigned long seconds = 0;
    unsigned long milliseconds;

    /* allow the device ID to be set */
    if (argc > 1) {
        Device_Set_Object_Instance_Number(strtol(argv[1], NULL, 0));
    }
    printf(
        "BACnet Raspberry Pi PiFace Digital Demo\n"
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
    piface_init();
    atexit(piface_cleanup);
    /* broadcast an I-Am on startup */
    Send_I_Am(&Handler_Transmit_Buffer[0]);
    /* loop forever */
    for (;;) {
        /* input */
        /* returns 0 bytes on timeout */
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
        if (mstimer_expired(&BACnet_Object_Timer)) {
            mstimer_reset(&BACnet_Object_Timer);
            milliseconds = mstimer_interval(&BACnet_Object_Timer);
            Device_Timer(milliseconds);
        }
        piface_task();
    }

    return 0;
}

/* @} */

/* End group ServerDemo */
