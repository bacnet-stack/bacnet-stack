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
#include "dlenv.h"
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
#include "lc.h"
#include "version.h"
/* include the device object */
#include "device.h"

/** @file server/main.c  Example server application using the BACnet Stack */

/** @defgroup Demos Demos of Servers and Clients
 * Most of the folders under the /demo folder contain demonstration (ie, sample) 
 * code that implements the name functionality.
 * 
 * The exceptions to this general rule, /handler and /object folders, are 
 * described in the various BIBBs and Object Framework modules. 
 */

/** @defgroup ServerDemo Demo of a BACnet Server (Device)
 * @ingroup Demos
 * This is a basic demonstration of a simple BACnet Device consisting of
 * the services and properties shown in its PICS (output provided by
 * the demo/epics/epics program):
 * @verbatim
List of Objects in test device:                                                                                                     
{                                                                                                                                   
    object-identifier: (Device, 1234)                                                                                               
    object-name: "SimpleServer"                                                                                                     
    object-type: Device                                                                                                             
    system-status: operational                                                                                                      
    vendor-name: "BACnet Stack at SourceForge"
    vendor-identifier: 260
    model-name: "GNU"
    firmware-version: "0.5.5"
    application-software-version: "1.0"
    protocol-version: 1
    protocol-revision: 5
    protocol-services-supported: {
        false,false,false,false,false, true, true, true,   # ,,,,, Subscribe-COV, Atomic-Read-File, Atomic-Write-File,
        false,false,false,false, true,false, true, true,   # ,,,, Read-Property,, Read-Property-Multiple, Write-Property,
        false, true,false,false, true,false,false,false,   # , Device-Communication-Control,,, Reinitialize-Device,,,,
        false,false,false,false, true,false,false,false,   # ,,,, COV-Notification,,,,
         true, true, true,false, true,false,false,false    #  Time-Synchronization, Who-Has, Who-Is,, UTC-Time-Synchronization,,,,
        }
    protocol-object-types-supported: {
         true, true, true, true, true, true,false,false,   #  Analog Input, Analog Output, Analog Value, Binary Input, Binary Output, Binary Value,,,
         true,false, true,false,false, true, true,false,   #  Device,, File,,, Multi-State Input, Multi-State Output,,
        false,false,false,false, true, true,false,false,   # ,,,, Trendlog, Life Safety Point,,,
        false,false,false,false, true,false,false,false,   # ,,,, Load-Control,,,,
        false,false,false,false,false,false    # ,,,,,,
        }
    object-list: {(Device, 1234),(Analog Input, 0),(Analog Input, 1),
        (Analog Input, 2),(Analog Input, 3),(Analog Output, 0),(Analog Output, 1),
        (Analog Output, 2),(Analog Output, 3),(Analog Value, 0),(Analog Value, 1),
        (Analog Value, 2),(Analog Value, 3),(Binary Input, 0),(Binary Input, 1),
        (Binary Input, 2),(Binary Input, 3),(Binary Input, 4),(Binary Output, 0),
        (Binary Output, 1),(Binary Output, 2),(Binary Output, 3),(Binary Value, 0),
        (Binary Value, 1),(Binary Value, 2),(Binary Value, 3),(Binary Value, 4),
        (Binary Value, 5),(Binary Value, 6),(Binary Value, 7),(Binary Value, 8),
        (Binary Value, 9),(Life Safety Point, 0),(Life Safety Point, 1),(Life Safety Point, 2),
        (Life Safety Point, 3),(Life Safety Point, 4),(Life Safety Point, 5),(Life Safety Point, 6),
        (Load-Control, 0),(Load-Control, 1),(Load-Control, 2),(Load-Control, 3),
        (Multi-State Output, 0),(Multi-State Output, 1),(Multi-State Output, 2),(Multi-State Output, 3),
        (Multi-State Input, 0),(Trendlog, 0),(Trendlog, 1),(Trendlog, 2),
        (Trendlog, 3),(Trendlog, 4),(Trendlog, 5),(Trendlog, 6),
        (Trendlog, 7),(File, 0),(File, 1),(File, 2)}
    max-apdu-length-accepted: 1476
    segmentation-supported: no-segmentation
    apdu-timeout: 3000
    number-of-APDU-retries: 3
    device-address-binding: Null
    database-revision: 1
}   @endverbatim
 */
/*@{*/

/** Buffer used for receiving */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/** Initialize the handlers we will utilize. */
static void Init_Service_Handlers(
    void)
{
    Device_Init();
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE,
        handler_read_property_multiple);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY,
        handler_write_property);
#if defined(BACFILE)
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_ATOMIC_READ_FILE,
        handler_atomic_read_file);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_ATOMIC_WRITE_FILE,
        handler_atomic_write_file);
#endif
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
        handler_reinitialize_device);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION,
        handler_timesync_utc);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION,
        handler_timesync);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_SUBSCRIBE_COV,
        handler_cov_subscribe);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_COV_NOTIFICATION,
        handler_ucov_notification);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
}

/** Handler registered with atexit() inside main function to, well, cleanup.
 * Especially if we don't end normally.
 */
static void cleanup(
    void)
{
    datalink_cleanup();
}

/** Main function of server demo.
 * 
 * @param argc [in] Arg count.
 * @param argv [in] Takes one argument: the Device Instance #.
 * @return 0 on success.
 */
int main(
    int argc,
    char *argv[])
{
    BACNET_ADDRESS src = {
        0
    };  /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 1000;    /* milliseconds */
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    uint32_t elapsed_seconds = 0;
    uint32_t elapsed_milliseconds = 0;

    /* allow the device ID to be set */
    if (argc > 1)
        Device_Set_Object_Instance_Number(strtol(argv[1], NULL, 0));
    printf("BACnet Server Demo\n" "BACnet Stack Version %s\n"
        "BACnet Device ID: %u\n" "Max APDU: %d\n", BACnet_Version,
        Device_Object_Instance_Number(), MAX_APDU);
    Init_Service_Handlers();
    dlenv_init();
    atexit(cleanup);
    /* configure the timeout values */
    last_seconds = time(NULL);
    /* broadcast an I-Am on startup */
    Send_I_Am(&Handler_Transmit_Buffer[0]);
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
        elapsed_seconds = current_seconds - last_seconds;
        if (elapsed_seconds) {
            last_seconds = current_seconds;
            dcc_timer_seconds(elapsed_seconds);
#if defined(BACDL_BIP) && BBMD_ENABLED
            bvlc_maintenance_timer(elapsed_seconds);
#endif
            Load_Control_State_Machine_Handler();
            elapsed_milliseconds = elapsed_seconds * 1000;
            handler_cov_task(elapsed_seconds);
            tsm_timer_milliseconds(elapsed_milliseconds);
        }
        /* output */

        /* blink LEDs, Turn on or off outputs, etc */
    }
}

/*@}*/      /* End group ServerDemo */
