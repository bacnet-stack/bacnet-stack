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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "config.h"
#include "address.h"
#include "bacdef.h"
#include "handlers.h"
#include "client.h"
#include "bacdcode.h"
#include "npdu.h"
#include "apdu.h"
#include "iam.h"
#include "tsm.h"
#include "device.h"
#include "bacfile.h"
#include "datalink.h"
#include "dcc.h"
#include "net.h"
#include "txbuf.h"

/* This is an example server application using the BACnet Stack */

/* buffers used for receiving */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

static void Init_Service_Handlers(void)
{
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS,
        handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS,
        handler_who_has);
    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY,
        handler_write_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_ATOMIC_READ_FILE,
        handler_atomic_read_file);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
        handler_reinitialize_device);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION,
        handler_timesync_utc);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION,
        handler_timesync);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler
        (SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
}

static void cleanup(void)
{
    datalink_cleanup();
}

static void print_address(char *name, BACNET_ADDRESS * dest)
{                               /* destination address */
    int i = 0;                  /* counter */

    if (dest) {
        printf("%s: ", name);
        for (i = 0; i < dest->mac_len; i++) {
            printf("%02X", dest->mac[i]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100;     /* milliseconds */
    BACNET_ADDRESS my_address, broadcast_address;
    time_t last_seconds = 0;
    time_t current_seconds = 0;

    /* allow the device ID to be set */
    if (argc > 1)
        Device_Set_Object_Instance_Number(strtol(argv[1], NULL, 0));
    if (argc > 2)
        bip_set_port(strtol(argv[2], NULL, 0));
    printf("BACnet Server Demo - Device #%u\r\n",
        Device_Object_Instance_Number());
    Init_Service_Handlers();
#ifdef BACDL_ETHERNET
    /* init the physical layer */
    if (!ethernet_init("eth0"))
        return 1;
#endif
#ifdef BACDL_BIP
    bip_set_interface("eth0");
    if (!bip_init())
        return 1;
    printf("bip: using port %hu\r\n", bip_get_port());
#endif
#ifdef BACDL_ARCNET
    if (!arcnet_init("arc0"))
        return 1;
#endif
    datalink_get_broadcast_address(&broadcast_address);
    print_address("Broadcast", &broadcast_address);
    datalink_get_my_address(&my_address);
    print_address("Address", &my_address);
    atexit(cleanup);
    /* configure the timeout values */
    last_seconds = time(NULL);
    /* broadcast an I-Am on startup */
    iam_send(&Handler_Transmit_Buffer[0]);
    /* loop forever */
    for (;;) {
        /* input */
        current_seconds = time(NULL);

        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        /* at least one second has passed */
        if (current_seconds != last_seconds)
            dcc_timer_seconds(current_seconds - last_seconds);
        /* output */

        /* blink LEDs, Turn on or off outputs, etc */
    }
}
