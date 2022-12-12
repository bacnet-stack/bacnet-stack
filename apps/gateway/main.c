/**************************************************************************
 *
 * Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *********************************************************************/
/**
 * Code for this project began with code from the demo/server project and
 * Paul Chapman's vmac project.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "bacnet/config.h"
#include "gateway.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/bacdef.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/iam.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/dcc.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/object/lc.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/version.h"
/* include the device object */
#include "bacnet/basic/object/device.h"
#ifdef BACNET_TEST_VMAC
#include "bacnet/basic/bbmd6/vmac.h"
#endif

/** @file gateway/main.c  Example virtual gateway application using the BACnet
 * Stack. */

/* Prototypes */

/* (Doxygen note: The next two lines pull all the following Javadoc
 *  into the GatewayDemo module.) */
/** @addtogroup GatewayDemo */
/*@{*/

/** Buffer used for receiving */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/** The list of DNETs that our router can reach.
 *  Only one entry since we don't support downstream routers.
 */
int DNET_list[2] = {
    VIRTUAL_DNET, -1 /* Need -1 terminator */
};

/* current version of the BACnet stack */
static const char *BACnet_Version = BACNET_VERSION_TEXT;

/* routed devices - I-Am on startup */
static unsigned Routed_Device_Index;

/** Initialize the Device Objects and each of the child Object instances.
 * @param first_object_instance Set the first (gateway) Device to this
            instance number, and subsequent devices to incremented values.
 */
static void Devices_Init(uint32_t first_object_instance)
{
    int i;
    char nameText[MAX_DEV_NAME_LEN];
    char descText[MAX_DEV_DESC_LEN];
    BACNET_CHARACTER_STRING name_string;

    /* Gateway Device has already been initialized.
     * But give it a better Description. */
    Routed_Device_Set_Description(DEV_DESCR_GATEWAY, strlen(DEV_DESCR_GATEWAY));

    /* Now initialize the remote Device objects. */
    for (i = 1; i < MAX_NUM_DEVICES; i++) {
        snprintf(nameText, MAX_DEV_NAME_LEN, "%s %d", DEV_NAME_BASE, i + 1);
        snprintf(descText, MAX_DEV_DESC_LEN, "%s %d", DEV_DESCR_REMOTE, i);
        characterstring_init_ansi(&name_string, nameText);

        Add_Routed_Device((first_object_instance + i), &name_string, descText);
    }
}

/** Initialize the BACnet Device Addresses for each Device object.
 * The gateway has already gotten the normal address (eg, PC's IP for BIP) and
 * the remote devices get
 * - For BIP, the IP address reversed, and 4th byte equal to index.
 * (Eg, 11.22.33.44 for the gateway becomes 44.33.22.01 for the first remote
 * device.) This is sure to be unique! The port number stays the same.
 * - For MS/TP, [Steve inserts a good idea here]
 */
static void Initialize_Device_Addresses(void)
{
    int i = 0; /* First entry is Gateway Device */
    uint32_t virtual_mac = 0;
    BACNET_ADDRESS virtual_address = { 0 };
    DEVICE_OBJECT_DATA *pDev = NULL;
    /* Setup info for the main gateway device first */
    pDev = Get_Routed_Device_Object(i);

    /* we can't use datalink_get_my_address() since it is
       mapped to routed_get_my_address() in this app
       to get the parent device address */
#if defined(BACDL_BIP)
    bip_get_my_address(&virtual_address);
#elif defined(BACDL_MSTP)
    dlmstp_get_my_address(&virtual_address);
#elif defined(BACDL_ARCNET)
    arcnet_get_my_address(&virtual_address);
#elif defined(BACDL_ETHERNET)
    ethernet_get_my_address(&virtual_address);
#elif defined(BACDL_BIP6)
    bip6_get_my_address(&virtual_address);
#else
#error "No support for this Data Link Layer type "
#endif
    bacnet_address_copy(&pDev->bacDevAddr, &virtual_address);
    /* broadcast an I-Am on startup */
    Send_I_Am(&Handler_Transmit_Buffer[0]);

    for (i = 1; i < MAX_NUM_DEVICES; i++) {
        pDev = Get_Routed_Device_Object(i);
        if (pDev == NULL) {
            continue;
        }
        /* start with the router address */
        bacnet_address_copy(&pDev->bacDevAddr, &virtual_address);
        /* add the network number to each gateway device */
        pDev->bacDevAddr.net = VIRTUAL_DNET;
        /* use a virtual MAC for each gateway device */
        virtual_mac = pDev->bacObj.Object_Instance_Number;
        encode_unsigned24(&pDev->bacDevAddr.adr[0], virtual_mac);
        pDev->bacDevAddr.len = 3;
    }
}

/** Initialize the handlers we will utilize.
 * @see Device_Init, apdu_set_unconfirmed_handler, apdu_set_confirmed_handler
 */
static void Init_Service_Handlers(uint32_t first_object_instance)
{
    Device_Init(NULL);
    Routing_Device_Init(first_object_instance);

    /* we need to handle who-is to support dynamic device binding
     * For the gateway, we will use the unicast variety so we can
     * get back through switches to different subnets.
     * Don't need the routed versions, since the npdu handler calls
     * each device in turn.
     */
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_WHO_IS, handler_who_is_unicast);
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
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_COV_NOTIFICATION, handler_ucov_notification);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
}

/** Main function of server demo.
 *
 * @see Device_Set_Object_Instance_Number, dlenv_init, Send_I_Am,
 *      datalink_receive, npdu_handler,
 *      dcc_timer_seconds, datalink_maintenance_timer,
 *      Load_Control_State_Machine_Handler, handler_cov_task,
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
    unsigned timeout = 1000; /* milliseconds */
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    uint32_t elapsed_seconds = 0;
    uint32_t elapsed_milliseconds = 0;
    uint32_t first_object_instance = FIRST_DEVICE_NUMBER;
#ifdef BACNET_TEST_VMAC
    /* Router data */
    BACNET_DEVICE_PROFILE *device;
    BACNET_VMAC_ADDRESS adr;
#endif

    /* allow the device ID to be set */
    if (argc > 1) {
        first_object_instance = strtol(argv[1], NULL, 0);
        if ((first_object_instance == 0) ||
            (first_object_instance >= BACNET_MAX_INSTANCE)) {
            printf("Error: Invalid Object Instance %s \n", argv[1]);
            printf(
                "Provide a number from 1 to %ul \n", BACNET_MAX_INSTANCE - 1);
            exit(1);
        }
    }
    printf("BACnet Router Demo\n"
           "BACnet Stack Version %s\n"
           "BACnet Device ID: %u\n"
           "Max APDU: %d\n"
           "Max Devices: %d\n",
        BACnet_Version, first_object_instance, MAX_APDU, MAX_NUM_DEVICES);
    Init_Service_Handlers(first_object_instance);
    dlenv_init();
    atexit(datalink_cleanup);
    Devices_Init(first_object_instance);
    Initialize_Device_Addresses();

#ifdef BACNET_TEST_VMAC
    /* initialize vmac table and router device */
    device = vmac_initialize(99, 2001);
    debug_printf(device->name, "ROUTER:%u", vmac_get_subnet());
#endif
    /* configure the timeout values */
    last_seconds = time(NULL);

    /* broadcast an I-am-router-to-network on startup */
    printf("Remote Network DNET Number %d \n", DNET_list[0]);
    Send_I_Am_Router_To_Network(DNET_list);

    /* loop forever */
    for (;;) {
        /* input */
        current_seconds = time(NULL);

        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

        /* process */
        if (pdu_len) {
            routing_npdu_handler(&src, DNET_list, &Rx_Buf[0], pdu_len);
        }
        /* at least one second has passed */
        elapsed_seconds = current_seconds - last_seconds;
        if (elapsed_seconds) {
            last_seconds = current_seconds;
            dcc_timer_seconds(elapsed_seconds);
            datalink_maintenance_timer(elapsed_seconds);
            dlenv_maintenance_timer(elapsed_seconds);
            Load_Control_State_Machine_Handler();
            elapsed_milliseconds = elapsed_seconds * 1000;
            tsm_timer_milliseconds(elapsed_milliseconds);
        }
        handler_cov_task();
        /* output */
        if (Routed_Device_Index < MAX_NUM_DEVICES) {
            Routed_Device_Index++;
            Get_Routed_Device_Object(Routed_Device_Index);
            /* broadcast an I-Am for each routed Device now */
            Send_I_Am(&Handler_Transmit_Buffer[0]);
        }
    }
    /* Dummy return */
    return 0;
}

/* @} */

/* End group GatewayDemo */
