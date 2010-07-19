/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
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

/* environment variables used for the command line tools */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "bacdef.h"
#include "apdu.h"
#include "datalink.h"
#include "handlers.h"
#include "client.h"
#include "tsm.h"

/** @file dlenv.c  Initialize the DataLink configuration. */

/* timer used to renew Foreign Device Registration */
static uint16_t BBMD_Timer_Seconds;

/** Register as a Foreign Device
 * @ingroup DataLink
 * The Environment Variables depend on defines BACDL_BIP and BBMD_ENABLED:
 *     - BACNET_BBMD_PORT - 0..65534, defaults to 47808
 *     - BACNET_BBMD_TIMETOLIVE - 0..65535 seconds, defaults to 60000
 *     - BACNET_BBMD_ADDRESS - dotted IPv4 address
 */
void dlenv_register_as_foreign_device(
    void)
{
#if defined(BACDL_BIP) && BBMD_ENABLED
    char *pEnv = NULL;
    long bbmd_timetolive_seconds = 60000;
    long bbmd_port = 0xBAC0;
    long bbmd_address = 0;

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
            fprintf(stderr,
                "Registering with BBMD at %s:%ld for %ld seconds\n",
                inet_ntoa(addr), bbmd_port, bbmd_timetolive_seconds);
            bvlc_register_with_bbmd(bbmd_address, (uint16_t) bbmd_port,
                (uint16_t) bbmd_timetolive_seconds);
            BBMD_Timer_Seconds = bbmd_timetolive_seconds;
        }
    }
#endif
}

/** Datalink maintenance timer
 * @ingroup DataLink
 *
 * Call this function to renew Foreign Device Registration
 */
void dlenv_maintenance_timer(
    uint16_t elapsed_seconds)
{
    if (BBMD_Timer_Seconds) {
        if (BBMD_Timer_Seconds <= elapsed_seconds) {
            BBMD_Timer_Seconds = 0;
        } else {
            BBMD_Timer_Seconds -= elapsed_seconds;
        }
        if (BBMD_Timer_Seconds == 0) {
            dlenv_register_as_foreign_device();
        }
    }
}

/** Initialize the DataLink configuration from Environment variables,
 * or else to defaults.
 * @ingroup DataLink
 * The items configured depend on which BACDL_ the code is built for,
 * eg, BACDL_BIP.
 *
 * For most items, checks first for an environment variable, and, if
 * found, uses that to set the item's value.  Otherwise, will set
 * to a default value.
 *
 * The Environment Variables, by BACDL_ type, are:
 * - BACDL_ALL: (the general-purpose solution)
 *   - BACNET_DATALINK to set which BACDL_ type we are using.
 * - (Any):
 *   - BACNET_APDU_TIMEOUT
 *   - BACNET_IFACE
 * - BACDL_BIP: (BACnet/IP)
 *   - BACNET_IP_PORT
 *   - with BBMD_ENABLED also:
 *     - BACNET_BBMD_PORT
 *     - BACNET_BBMD_TIMETOLIVE
 *     - BACNET_BBMD_ADDRESS
 * - BACDL_MSTP: (BACnet MS/TP)
 *   - BACNET_MAX_INFO_FRAMES
 *   - BACNET_MAX_MASTER
 *   - BACNET_MSTP_BAUD
 *   - BACNET_MSTP_MAC
 */
void dlenv_init(
    void)
{
    char *pEnv = NULL;

#if defined(BACDL_ALL)
    pEnv = getenv("BACNET_DATALINK");
    if (pEnv) {
        datalink_set(pEnv));
    } else {
        datalink_set(NULL);
    }
#endif
#if defined(BACDL_BIP)
#if defined(BIP_DEBUG)
    BIP_Debug = true;
#endif
    pEnv = getenv("BACNET_IP_PORT");
    if (pEnv) {
        bip_set_port((uint16_t) strtol(pEnv, NULL, 0));
    } else {
        /* BIP_Port is statically initialized to 0xBAC0,
         * so if it is different, then it was programmatically altered,
         * and we shouldn't just stomp on it here.
         * Unless it is set below 1024, since:
         * "The range for well-known ports managed by the IANA is 0-1023."
         */
        if (bip_get_port() < 1024)
            bip_set_port(0xBAC0);
    }
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
        dlmstp_set_baud_rate(strtol(pEnv, NULL, 0));
    } else {
        dlmstp_set_baud_rate(38400);
    }
    pEnv = getenv("BACNET_MSTP_MAC");
    if (pEnv) {
        dlmstp_set_mac_address(strtol(pEnv, NULL, 0));
    } else {
        dlmstp_set_mac_address(127);
    }
#endif
    pEnv = getenv("BACNET_APDU_TIMEOUT");
    if (pEnv) {
        apdu_timeout_set((uint16_t) strtol(pEnv, NULL, 0));
        fprintf(stderr, "BACNET_APDU_TIMEOUT=%s\r\n", pEnv);
    } else {
#if defined(BACDL_MSTP)
        apdu_timeout_set(60000);
#endif
    }
    if (!datalink_init(getenv("BACNET_IFACE"))) {
        exit(1);
    }
    pEnv = getenv("BACNET_INVOKE_ID");
    if (pEnv) {
        tsm_invokeID_set((uint8_t) strtol(pEnv, NULL, 0));
    }
    dlenv_register_as_foreign_device();
}
