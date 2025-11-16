/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/

/* This is one way to use the embedded BACnet stack under Win32 */
/* compiled with Borland C++ 5.02 or Visual C++ 6.0 */
#include <winsock2.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <conio.h> /* for kbhit and getch */
#include "bacnet/iam.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/dlenv.h"
/* include the objects */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/ao.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bo.h"
#include "bacnet/basic/object/bv.h"
#include "bacnet/basic/object/lc.h"
#include "bacnet/basic/object/lsp.h"
#include "bacnet/basic/object/mso.h"
#include "bacnet/basic/object/ms-input.h"
#include "bacnet/basic/object/trendlog.h"
#if defined(BACFILE)
#include "bacnet/basic/object/bacfile.h"
#endif

/** Static receive buffer, initialized with zeros by the C Library Startup Code.
 */

static uint8_t Rx_Buf
    [MAX_MPDU + 16 /* Add a little safety margin to the buffer,
                    * so that in the rare case, the message
                    * would be filled up to MAX_MPDU and some
                    * decoding functions would overrun, these
                    * decoding functions will just end up in
                    * a safe field of static zeros. */
] = { 0 };

/* send a whois to see who is on the network */
static bool Who_Is_Request = true;
bool I_Am_Request = true;

static void Read_Properties(void)
{
    uint32_t device_id = 0;
    bool status = false;
    unsigned max_apdu = 0;
    BACNET_ADDRESS src;
    bool next_device = false;
    static unsigned index = 0;
    static unsigned property = 0;
    /* list of required (and some optional) properties in the
       Device Object
       note: you could just loop through
       all the properties in all the objects. */
    const int32_t object_props[] = {
        PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME, PROP_OBJECT_TYPE,
        PROP_SYSTEM_STATUS, PROP_VENDOR_NAME, PROP_VENDOR_IDENTIFIER,
        PROP_MODEL_NAME, PROP_FIRMWARE_REVISION,
        PROP_APPLICATION_SOFTWARE_VERSION, PROP_PROTOCOL_VERSION,
        PROP_PROTOCOL_SERVICES_SUPPORTED, PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED,
        PROP_MAX_APDU_LENGTH_ACCEPTED, PROP_SEGMENTATION_SUPPORTED,
        PROP_LOCAL_TIME, PROP_LOCAL_DATE, PROP_UTC_OFFSET,
        PROP_DAYLIGHT_SAVINGS_STATUS, PROP_APDU_SEGMENT_TIMEOUT,
        PROP_APDU_TIMEOUT, PROP_NUMBER_OF_APDU_RETRIES,
        PROP_TIME_SYNCHRONIZATION_RECIPIENTS, PROP_MAX_MASTER,
        PROP_MAX_INFO_FRAMES, PROP_DEVICE_ADDRESS_BINDING,
        /* note: PROP_OBJECT_LIST is missing cause
           we need to get it with an index method since
           the list could be very large */
        /* some proprietary properties */
        514, 515,
        /* end of list */
        -1
    };

    if (address_count()) {
        if (address_get_by_index(index, &device_id, &max_apdu, &src)) {
            if (object_props[property] < 0) {
                next_device = true;
            } else {
                status = Send_Read_Property_Request(
                    device_id, /* destination device */
                    OBJECT_DEVICE, device_id, object_props[property],
                    BACNET_ARRAY_ALL);
                if (status) {
                    property++;
                }
            }
        } else {
            next_device = true;
        }
        if (next_device) {
            next_device = false;
            index++;
            if (index >= MAX_ADDRESS_CACHE) {
                index = 0;
            }
            property = 0;
        }
    }

    return;
}

static void LocalIAmHandler(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len = 0;
    uint32_t device_id = 0;
    unsigned max_apdu = 0;
    int segmentation = 0;
    uint16_t vendor_id = 0;

    (void)src;
    len = bacnet_iam_request_decode(
        service_request, service_len, &device_id, &max_apdu, &segmentation,
        &vendor_id);
    fprintf(stderr, "Received I-Am Request");
    if (len != -1) {
        fprintf(stderr, " from %u!\n", device_id);
        address_add(device_id, max_apdu, src);
    } else {
        fprintf(stderr, "!\n");
    }

    return;
}

static void Init_Service_Handlers(void)
{
    Device_Init(NULL);

    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, LocalIAmHandler);

    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property_ack);
#if defined(BACFILE)
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_ATOMIC_READ_FILE, handler_atomic_read_file);
#endif
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);

#if 0
    /* Adding these handlers require the project(s) to change. */
#if defined(BACFILE)
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_ATOMIC_WRITE_FILE,
        handler_atomic_write_file);
#endif
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_RANGE,
        handler_read_range);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
        handler_reinitialize_device);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION,
        handler_timesync_utc);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION,
        handler_timesync);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_COV_NOTIFICATION,
        handler_ucov_notification);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
#endif
}

static void print_address(char *name, BACNET_ADDRESS *dest)
{ /* destination address */
    int i = 0; /* counter */

    if (dest) {
        printf("%s: ", name);
        for (i = 0; i < dest->mac_len; i++) {
            printf("%02X", dest->mac[i]);
        }
        printf("\n");
    }
}

static void print_address_cache(void)
{
    int i, j;
    BACNET_ADDRESS address;
    uint32_t device_id = 0;
    unsigned max_apdu = 0;

    fprintf(stderr, "Device\tMAC\tMaxAPDU\tNet\n");
    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        if (address_get_by_index(i, &device_id, &max_apdu, &address)) {
            fprintf(stderr, "%u\t", device_id);
            for (j = 0; j < address.mac_len; j++) {
                fprintf(stderr, "%02X", address.mac[j]);
            }
            fprintf(stderr, "\t");
            fprintf(stderr, "%hu\t", max_apdu);
            fprintf(stderr, "%hu\n", address.net);
        }
    }
}

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100; /* milliseconds */
    BACNET_ADDRESS my_address, broadcast_address;

    (void)argc;
    (void)argv;
    Device_Set_Object_Instance_Number(4194300);
    address_init();
    Init_Service_Handlers();
    dlenv_init();
    datalink_get_broadcast_address(&broadcast_address);
    print_address("Broadcast", &broadcast_address);
    datalink_get_my_address(&my_address);
    print_address("Address", &my_address);
    printf("BACnet stack running...\n");
    /* loop forever */
    for (;;) {
        /* input */

        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        if (I_Am_Request) {
            I_Am_Request = false;
            Send_I_Am(&Handler_Transmit_Buffer[0]);
        } else if (Who_Is_Request) {
            Who_Is_Request = false;
            Send_WhoIs(-1, -1);
        } else {
            Read_Properties();
        }

        /* output */

        /* blink LEDs, Turn on or off outputs, etc */

        /* wait for ESC from keyboard before quitting */
        if (kbhit() && (getch() == 0x1B)) {
            break;
        }
    }

    print_address_cache();

    return 0;
}
