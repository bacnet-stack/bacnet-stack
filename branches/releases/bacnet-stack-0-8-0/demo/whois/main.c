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

/* command line tool that sends a BACnet service, and displays the reply */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>       /* for time */
#include <errno.h>
#include "bactext.h"
#include "iam.h"
#include "address.h"
#include "config.h"
#include "bacdef.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "datalink.h"
#include "bactext.h"
/* some demo stuff needed */
#include "filename.h"
#include "handlers.h"
#include "client.h"
#include "txbuf.h"
#if defined(BACDL_MSTP)
#include "rs485.h"
#endif
#include "dlenv.h"
#include "net.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static int32_t Target_Object_Instance_Min = -1;
static int32_t Target_Object_Instance_Max = -1;
static bool Error_Detected = false;

#define BAC_ADDRESS_MULT 1

struct address_entry {
    struct address_entry *next;
    uint8_t Flags;
    uint32_t device_id;
    unsigned max_apdu;
    BACNET_ADDRESS address;
};

static struct address_table {
    struct address_entry *first;
    struct address_entry *last;
} Address_Table = {
0};


struct address_entry *alloc_address_entry(
    void)
{
    struct address_entry *rval;
    rval = (struct address_entry *) calloc(1, sizeof(struct address_entry));
    if (Address_Table.first == 0) {
        Address_Table.first = Address_Table.last = rval;
    } else {
        Address_Table.last->next = rval;
        Address_Table.last = rval;
    }
    return rval;
}



bool bacnet_address_matches(
    BACNET_ADDRESS * a1,
    BACNET_ADDRESS * a2)
{
    int i = 0;
    if (a1->net != a2->net)
        return false;
    if (a1->len != a2->len)
        return false;
    for (; i < a1->len; i++)
        if (a1->adr[i] != a2->adr[i])
            return false;
    return true;
}

void address_table_add(
    uint32_t device_id,
    unsigned max_apdu,
    BACNET_ADDRESS * src)
{
    struct address_entry *pMatch;
    uint8_t flags = 0;

    pMatch = Address_Table.first;
    while (pMatch) {
        if (pMatch->device_id == device_id) {
            if (bacnet_address_matches(&pMatch->address, src))
                return;
            flags |= BAC_ADDRESS_MULT;
            pMatch->Flags |= BAC_ADDRESS_MULT;
        }
        pMatch = pMatch->next;
    }

    pMatch = alloc_address_entry();

    pMatch->Flags = flags;
    pMatch->device_id = device_id;
    pMatch->max_apdu = max_apdu;
    pMatch->address = *src;

    return;
}



void my_i_am_handler(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src)
{
    int len = 0;
    uint32_t device_id = 0;
    unsigned max_apdu = 0;
    int segmentation = 0;
    uint16_t vendor_id = 0;
    unsigned i = 0;

    (void) service_len;
    len =
        iam_decode_service_request(service_request, &device_id, &max_apdu,
        &segmentation, &vendor_id);
#if PRINT_ENABLED
    fprintf(stderr, "Received I-Am Request");
#endif
    if (len != -1) {
#if PRINT_ENABLED
        fprintf(stderr, " from %lu, MAC = ", (unsigned long) device_id);
        if ((src->mac_len == 6) && (src->len == 0)) {
            fprintf(stderr, "%u.%u.%u.%u %02X%02X\n",
                (unsigned)src->mac[0], (unsigned)src->mac[1],
                (unsigned)src->mac[2], (unsigned)src->mac[3],
                (unsigned)src->mac[4], (unsigned)src->mac[5]);
        } else {
            for (i = 0; i < src->mac_len; i++) {
                fprintf(stderr, "%02X", (unsigned)src->mac[i]);
                if (i < (src->mac_len-1)) {
                    fprintf(stderr, ":");
                }
            }
            fprintf(stderr, "\n");
        }
#endif
        address_table_add(device_id, max_apdu, src);
    } else {
#if PRINT_ENABLED
        fprintf(stderr, ", but unable to decode it.\n");
#endif
    }

    return;
}

void MyAbortHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t abort_reason,
    bool server)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    (void) server;
    fprintf(stderr, "BACnet Abort: %s\r\n",
        bactext_abort_reason_name(abort_reason));
    Error_Detected = true;
}

void MyRejectHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    fprintf(stderr, "BACnet Reject: %s\r\n",
        bactext_reject_reason_name(reject_reason));
    Error_Detected = true;
}

static void init_service_handlers(
    void)
{
    Device_Init(NULL);
    /* Note: this applications doesn't need to handle who-is
       it is confusing for the user! */
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    /* handle the reply (request) coming back */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, my_i_am_handler);
    /* handle any errors coming back */
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

void print_macaddr(
    uint8_t * addr,
    int len)
{
    int j = 0;

    while (j < len) {
        if (j != 0) {
            printf(":");
        }
        printf("%02X", addr[j]);
        j++;
    }
    while (j < MAX_MAC_LEN) {
        printf("   ");
        j++;
    }
}

static void print_address_cache(
    void)
{
    BACNET_ADDRESS address;
    unsigned total_addresses = 0;
    unsigned dup_addresses = 0;
    struct address_entry *addr;
    uint8_t local_sadr = 0;

    /*  NOTE: this string format is parsed by src/address.c,
       so these must be compatible. */

    printf(";%-7s  %-20s %-5s %-20s %-4s\n", "Device", "MAC (hex)", "SNET",
        "SADR (hex)", "APDU");
    printf(";-------- -------------------- ----- -------------------- ----\n");


    addr = Address_Table.first;
    while (addr) {
        bacnet_address_copy(&address, &addr->address);
        total_addresses++;
        if (addr->Flags & BAC_ADDRESS_MULT) {
            dup_addresses++;
            printf(";");
        } else {
            printf(" ");
        }
        printf(" %-7u ", addr->device_id);
        print_macaddr(address.mac, address.mac_len);
        printf(" %-5hu ", address.net);
        if (address.net) {
            print_macaddr(address.adr, address.len);
        } else {
            print_macaddr(&local_sadr, 1);
        }
        printf(" %-4hu ", addr->max_apdu);
        printf("\n");

        addr = addr->next;
    }
    printf(";\n; Total Devices: %u\n", total_addresses);
    if (dup_addresses) {
        printf("; * Duplicate Devices: %u\n", dup_addresses);
    }
}

static int print_usage(
    char *exe_name)
{
    printf("Usage:\n" "\n" "%s [[network]:[address]] "
        "[device-instance-min [device-instance-max]] [--help]\n", exe_name);
    return 1;
}


static int print_help(
    char *exe_name)
{
    printf("Usage:\n" "\n" "%s [[network]:[address]] "
        "[device-instance-min [device-instance-max]] [--help]\n" "\n"
        "  Send BACnet WhoIs service request to a device or multiple devices, and wait\n"
        "  for responses. Displays any devices found and their network information.\n"
        "\n" "device-instance:\r\n"
        "  BACnet Device Object Instance number that you are trying to send a Who-Is\n"
        "  service request. The value should be in  the range of 0 to 4194303. A range\n"
        "  of values can also be specified by using a minimum value and a maximum value.\n"
        "\n" "network:\n"
        "  BACnet network number for directed requests. Valid range is from 0 to 65535\n"
        "  where 0 is the local connection and 65535 is network broadcast.\n"
        "\n" "address:\n"
        "  BACnet mac address number. Valid ranges are from 0 to 255 or a IP connection \n"
        "  string including port number like 10.1.2.3:47808.\n" "\n"
        "Examples:\n\n" "To send a WhoIs request to Network 123:\n"
        "%s 123:\n\n" "To send a WhoIs request to Network 123 Address 5:\n"
        "%s 123:5\n\n" "To send a WhoIs request to Device 123:\n" "%s 123\n\n"
        "To send a WhoIs request to Devices from 1000 to 9000:\n"
        "%s 1000 9000\n\n"
        "To send a WhoIs request to Devices from 1000 to 9000 on Network 123:\n"
        "%s 123: 1000 9000\n\n" "To send a WhoIs request to all devices:\n"
        "%s\n\n", exe_name, exe_name, exe_name, exe_name, exe_name, exe_name,
        exe_name);
    return 1;
}


/* Parse a string for a bacnet-address
**
** @return length of address parsed in bytes
*/
static int parse_bac_address(
    BACNET_ADDRESS * dest,      /* [out] BACNET Address */
    char *src   /* [in] nul terminated string to parse */
    )
{
    int i = 0;
    uint16_t s;
    int a[4], p;
    int c = sscanf(src, "%u.%u.%u.%u:%u", &a[0], &a[1], &a[2], &a[3], &p);

    dest->len = 0;

    if (c == 1) {
        if (a[0] < 256) {       /* mstp */
            dest->adr[0] = a[0];
            dest->len = 1;
        } else if (a[0] < 0x0FFFF) {    /* lon */
            s = htons((uint16_t) a[0]);
            memcpy(&dest->adr[0], &s, 2);
            dest->len = 2;
        } else
            return 0;
    } else if (c == 5) {        /* ip address */
        for (i = 0; i < 4; i++) {
            if ((a[i] < 0) || (a[i] > 255)) {
                return 0;
            }
            dest->adr[i] = a[i];
        }
        s = htons((uint16_t) p);
        memcpy(&dest->adr[i], &s, 2);
        dest->len = 6;
    }
    return dest->len;
}

int main(
    int argc,
    char *argv[])
{
    BACNET_ADDRESS src = {
        0
    };  /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100;     /* milliseconds */
    time_t total_seconds = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    BACNET_ADDRESS dest;
    int argi;

    /* print help if requested */
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--help") == 0) {
            print_help(filename_remove_path(argv[0]));
            return 0;
        }
    }

    datalink_get_broadcast_address(&dest);

    /* decode the command line parameters */
    if (argc >= 2) {
        char *s;
        long v = strtol(argv[1], &s, 0);
        if (*s++ == ':') {
            if (argv[1][0] != ':')
                dest.net = (uint16_t) v;
            dest.mac_len = 0;
            if (isdigit(*s))
                parse_bac_address(&dest, s);
        } else {
            Target_Object_Instance_Min = Target_Object_Instance_Max = v;
        }
    }

    if (argc <= 2) {
        /* empty */
    } else if (argc == 3) {
        if (Target_Object_Instance_Min == -1)
            Target_Object_Instance_Min = Target_Object_Instance_Max =
                strtol(argv[2], NULL, 0);
        else
            Target_Object_Instance_Max = strtol(argv[2], NULL, 0);
    } else if (argc == 4) {
        Target_Object_Instance_Min = strtol(argv[2], NULL, 0);
        Target_Object_Instance_Max = strtol(argv[3], NULL, 0);
    } else {
        print_usage(filename_remove_path(argv[0]));
        return 1;
    }

    if (Target_Object_Instance_Min > BACNET_MAX_INSTANCE) {
        fprintf(stderr, "device-instance-min=%u - it must be less than %u\r\n",
            Target_Object_Instance_Min, BACNET_MAX_INSTANCE + 1);
        return 1;
    }
    if (Target_Object_Instance_Max > BACNET_MAX_INSTANCE) {
        fprintf(stderr, "device-instance-max=%u - it must be less than %u\r\n",
            Target_Object_Instance_Max, BACNET_MAX_INSTANCE + 1);
        return 1;
    }

    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    init_service_handlers();
    address_init();
    dlenv_init();
    atexit(datalink_cleanup);
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = apdu_timeout() / 1000;
    /* send the request */
    Send_WhoIs_To_Network(&dest, Target_Object_Instance_Min,
        Target_Object_Instance_Max);
    /* loop forever */
    for (;;) {
        /* increment timer - exit if timed out */
        current_seconds = time(NULL);
        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        if (Error_Detected)
            break;
        /* increment timer - exit if timed out */
        elapsed_seconds = current_seconds - last_seconds;
        if (elapsed_seconds) {
#if defined(BACDL_BIP) && BBMD_ENABLED
            bvlc_maintenance_timer(elapsed_seconds);
#endif
        }
        total_seconds += elapsed_seconds;
        if (total_seconds > timeout_seconds)
            break;
        /* keep track of time for next check */
        last_seconds = current_seconds;
    }
    print_address_cache();

    return 0;
}
