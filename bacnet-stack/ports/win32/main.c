/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
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

/* This is one way to use the embedded BACnet stack under Win32 */
/* compiled with Borland C++ 5.02 or Visual C++ 6.0 */
#include <winsock2.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <conio.h>      /* for kbhit and getch */
#include "iam.h"
#include "address.h"
#include "config.h"
#include "bacdef.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "handlers.h"
#include "client.h"
#include "datalink.h"
#include "txbuf.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* send a whois to see who is on the network */
static bool Who_Is_Request = true;
bool I_Am_Request = true;

static void Read_Properties(
    void)
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
    const int object_props[] = {
        PROP_OBJECT_IDENTIFIER,
        PROP_OBJECT_NAME,
        PROP_OBJECT_TYPE,
        PROP_SYSTEM_STATUS,
        PROP_VENDOR_NAME,
        PROP_VENDOR_IDENTIFIER,
        PROP_MODEL_NAME,
        PROP_FIRMWARE_REVISION,
        PROP_APPLICATION_SOFTWARE_VERSION,
        PROP_PROTOCOL_VERSION,
        PROP_PROTOCOL_CONFORMANCE_CLASS,
        PROP_PROTOCOL_SERVICES_SUPPORTED,
        PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED,
        PROP_MAX_APDU_LENGTH_ACCEPTED,
        PROP_SEGMENTATION_SUPPORTED,
        PROP_LOCAL_TIME,
        PROP_LOCAL_DATE,
        PROP_UTC_OFFSET,
        PROP_DAYLIGHT_SAVINGS_STATUS,
        PROP_APDU_SEGMENT_TIMEOUT,
        PROP_APDU_TIMEOUT,
        PROP_NUMBER_OF_APDU_RETRIES,
        PROP_TIME_SYNCHRONIZATION_RECIPIENTS,
        PROP_MAX_MASTER,
        PROP_MAX_INFO_FRAMES,
        PROP_DEVICE_ADDRESS_BINDING,
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
            if (object_props[property] < 0)
                next_device = true;
            else {
                status = Send_Read_Property_Request(device_id,  /* destination device */
                    OBJECT_DEVICE, device_id, object_props[property],
                    BACNET_ARRAY_ALL);
                if (status)
                    property++;
            }
        } else
            next_device = true;
        if (next_device) {
            next_device = false;
            index++;
            if (index >= MAX_ADDRESS_CACHE)
                index = 0;
            property = 0;
        }
    }

    return;
}

static void LocalIAmHandler(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src)
{
    int len = 0;
    uint32_t device_id = 0;
    unsigned max_apdu = 0;
    int segmentation = 0;
    uint16_t vendor_id = 0;

    (void) src;
    (void) service_len;
    len =
        iam_decode_service_request(service_request, &device_id, &max_apdu,
        &segmentation, &vendor_id);
    fprintf(stderr, "Received I-Am Request");
    if (len != -1) {
        fprintf(stderr, " from %u!\n", device_id);
        address_add(device_id, max_apdu, src);
    } else
        fprintf(stderr, "!\n");

    return;
}

static void Init_Service_Handlers(
    void)
{
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, LocalIAmHandler);

    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY,
        handler_write_property);
    /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property_ack);
}

static void print_address(
    char *name,
    BACNET_ADDRESS * dest)
{       /* destination address */
    int i = 0;  /* counter */

    if (dest) {
        printf("%s: ", name);
        for (i = 0; i < dest->mac_len; i++) {
            printf("%02X", dest->mac[i]);
        }
        printf("\n");
    }
}

static void print_address_cache(
    void)
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

static void Init_DataLink(
    void)
{
    char *pEnv = NULL;
#if defined(BACDL_BIP) && BBMD_ENABLED
    long bbmd_port = 0xBAC0;
    long bbmd_address = 0;
    long bbmd_timetolive_seconds = 60000;
#endif

#if defined(BACDL_ALL)
    pEnv = getenv("BACNET_DATALINK");
    if (pEnv) {
        datalink_set(pEnv));
    } else {
        datalink_set(NULL);
    }
#endif

#if defined(BACDL_BIP)
    pEnv = getenv("BACNET_IP_PORT");
    if (pEnv) {
        bip_set_port(strtol(pEnv, NULL, 0));
    } else {
        bip_set_port(0xBAC0);
    }
    BIP_Debug = true;
#elif defined(BACDL_MSTP)
    pEnv = getenv("BACNET_MAX_INFO_FRAMES");
    if (pEnv) {
        dlmstp_set_max_info_frames(strtol(pEnv, NULL, 0));
    } else {
        dlmstp_set_max_info_frames(1);
    }
    pEnv = getenv("BACNET_MAX_MASTER");
    if (pEnv) {
        dlmstp_set_max_master(strtol(pEnv, NULL, 0));
    } else {
        dlmstp_set_max_master(127);
    }
    pEnv = getenv("BACNET_MSTP_BAUD");
    if (pEnv) {
        RS485_Set_Baud_Rate(strtol(pEnv, NULL, 0));
    } else {
        RS485_Set_Baud_Rate(38400);
    }
    pEnv = getenv("BACNET_MSTP_MAC");
    if (pEnv) {
        dlmstp_set_mac_address(strtol(pEnv, NULL, 0));
    } else {
        dlmstp_set_mac_address(127);
    }
#endif
    if (!datalink_init(getenv("BACNET_IFACE"))) {
        exit(1);
    }
#if defined(BACDL_BIP) && BBMD_ENABLED
    pEnv = getenv("BACNET_BBMD_PORT");
    if (pEnv) {
        bbmd_port = strtol(pEnv, NULL, 0);
        if (bbmd_port > 0xFFFF) {
            bbmd_port = 0xBAC0;
        }
    }
    pEnv = getenv("BACNET_BBMD_TIMETOLIVE");
    if (pEnv) {
        bbmd_timetolive_seconds = strtol(pEnv, NULL, 0);
        if (bbmd_timetolive_seconds > 0xFFFF) {
            bbmd_timetolive_seconds = 0xFFFF;
        }
    }
    pEnv = getenv("BACNET_BBMD_ADDRESS");
    if (pEnv) {
        bbmd_address = bip_getaddrbyname(pEnv);
        if (bbmd_address) {
            struct in_addr addr;
            addr.s_addr = bbmd_address;
            printf("Server: Registering with BBMD at %s:%ld for %ld seconds\n",
                inet_ntoa(addr), bbmd_port, bbmd_timetolive_seconds);
            bvlc_register_with_bbmd(bbmd_address, bbmd_port,
                bbmd_timetolive_seconds);
        }
    }
#endif
}

int main(
    int argc,
    char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100;     /* milliseconds */
    BACNET_ADDRESS my_address, broadcast_address;

    (void) argc;
    (void) argv;
    Device_Set_Object_Instance_Number(4194303);
    Init_Service_Handlers();
    Init_DataLink();
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
        if (kbhit() && (getch() == 0x1B))
            break;
    }

    print_address_cache();

    return 0;
}
