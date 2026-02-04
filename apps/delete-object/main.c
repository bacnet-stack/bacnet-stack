/**
 * @file
 * @brief Application to send a BACnet DeleteObject
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 *
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#define PRINT_ENABLED 1
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/bacdest.h"
#include "bacnet/bacerror.h"
#include "bacnet/iam.h"
#include "bacnet/arf.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/create_object.h"
#include "bacnet/whois.h"
#include "bacnet/abort.h"
#include "bacnet/reject.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"

#if BACNET_SVC_SERVER
#error "App requires server-only features disabled! Set BACNET_SVC_SERVER=0"
#endif

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };
/* target device data for the request */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_OBJECT_TYPE Target_Object_Type;
static uint32_t Target_Object_Instance = BACNET_MAX_INSTANCE;
/* needed to filter incoming messages */
static uint8_t Request_Invoke_ID = 0;
static BACNET_ADDRESS Target_Address;
/* needed for return value of main application */
static bool Error_Detected = false;
/* Used for verbose */
static bool Verbose = false;

static void
MyPrintHandler(BACNET_ERROR_CLASS error_class, BACNET_ERROR_CODE error_code)
{
    printf(
        "[{\n  \"%s\": {\n"
        "    \"object-type\": \"%s\",\n    \"object-instance\": %lu,\n"
        "    \"error-class\": \"%s\",\n    \"error-code\": \"%s\"\n  }\n}]\n",
        bactext_confirmed_service_name(SERVICE_CONFIRMED_DELETE_OBJECT),
        bactext_object_type_name(Target_Object_Type),
        (unsigned long)Target_Object_Instance,
        bactext_error_class_name((int)error_class),
        bactext_error_code_name((int)error_code));
}

static void MyErrorHandler(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        MyPrintHandler(error_class, error_code);
        Error_Detected = true;
    }
}

static void MySimpleAckHandler(BACNET_ADDRESS *src, uint8_t invoke_id)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        MyPrintHandler(ERROR_CLASS_SERVICES, ERROR_CODE_SUCCESS);
    }
}

static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    (void)server;
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        MyPrintHandler(
            ERROR_CLASS_SERVICES, abort_convert_to_error_code(abort_reason));
        Error_Detected = true;
    }
}

static void
MyRejectHandler(BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        MyPrintHandler(
            ERROR_CLASS_SERVICES, reject_convert_to_error_code(reject_reason));
        Error_Detected = true;
    }
}

static void Init_Service_Handlers(void)
{
    Device_Init(NULL);
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* handle i-am to support binding to other devices */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_bind);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    /* handle the ack or error coming back from confirmed request */
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_DELETE_OBJECT, MySimpleAckHandler);
    apdu_set_error_handler(SERVICE_CONFIRMED_DELETE_OBJECT, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void print_usage(const char *filename)
{
    printf("Usage: %s device-instance object-type object-instance\n", filename);
    printf("       [--dnet][--dadr][--mac]\n");
    printf("       [--version][--help][--verbose]\n");
}

static void print_help(const char *filename)
{
    printf("Create an object in a BACnet device.\n");
    printf("\n");
    printf("device-instance:\n"
           "BACnet Device Object Instance number that you are\n"
           "trying to communicate to.  This number will be used\n"
           "to try and bind with the device using Who-Is and\n"
           "I-Am services.  For example, if you were writing\n"
           "Device Object 123, the device-instance would be 123.\n");
    printf("\n");
    printf("object-type:\n"
           "The object type is object that you are deleting. It\n"
           "can be defined either as the object-type name string\n"
           "as defined in the BACnet specification, or as the\n"
           "integer value of the enumeration BACNET_OBJECT_TYPE\n"
           "in bacenum.h. For example if you were reading Analog\n"
           "Output 2, the object-type would be analog-output or 1.\n");
    printf("\n");
    printf("object-instance:\n"
           "This is the object instance number of the object that\n"
           "you are deleting.  For example, if you were deleting\n"
           "Analog Output 2, the object-instance would be 2.\n");
    printf("\n");
    printf(
        "Example:\n"
        "If you want to DeleteObject an Analog Input 1\n"
        "send the following command:\n"
        "%s 123 0 1\n",
        filename);
}

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100; /* milliseconds */
    unsigned max_apdu = 0;
    struct mstimer apdu_timer;
    struct mstimer maintenance_timer;
    bool found = false;
    unsigned long object_type = 0;
    unsigned long object_instance = 0;
    long dnet = -1;
    BACNET_MAC_ADDRESS mac = { 0 };
    BACNET_MAC_ADDRESS adr = { 0 };
    BACNET_ADDRESS dest = { 0 };
    bool specific_address = false;
    unsigned int target_args = 0;
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
            printf("Copyright (C) 2023 by Steve Karg and others.\n"
                   "This is free software; see the source for copying "
                   "conditions.\n"
                   "There is NO warranty; not even for MERCHANTABILITY or\n"
                   "FITNESS FOR A PARTICULAR PURPOSE.\n");
            return 0;
        }
        if (strcmp(argv[argi], "--mac") == 0) {
            if (++argi < argc) {
                if (bacnet_address_mac_from_ascii(&mac, argv[argi])) {
                    specific_address = true;
                }
            }
        } else if (strcmp(argv[argi], "--dnet") == 0) {
            if (++argi < argc) {
                if (!bacnet_strtol(argv[argi], &dnet)) {
                    fprintf(stderr, "dnet=%s invalid\n", argv[argi]);
                    return 1;
                }
                if ((dnet >= 0) && (dnet <= UINT16_MAX)) {
                    specific_address = true;
                }
            }
        } else if (strcmp(argv[argi], "--dadr") == 0) {
            if (++argi < argc) {
                if (bacnet_address_mac_from_ascii(&adr, argv[argi])) {
                    specific_address = true;
                }
            }
        } else if (strcmp(argv[argi], "--verbose") == 0) {
            Verbose = true;
        } else {
            if (target_args == 0) {
                if (!bacnet_strtoul(argv[argi], &object_instance)) {
                    fprintf(stderr, "device-instance=%s invalid\n", argv[argi]);
                    return 1;
                }
                if (object_instance > BACNET_MAX_INSTANCE) {
                    fprintf(
                        stderr, "device-instance=%lu - not greater than %u\n",
                        object_instance, BACNET_MAX_INSTANCE);
                    return 1;
                }
                Target_Device_Object_Instance = object_instance;
                target_args++;
            } else if (target_args == 1) {
                if (!bacnet_strtoul(argv[argi], &object_type)) {
                    fprintf(stderr, "object-type=%s invalid\n", argv[argi]);
                    return 1;
                }
                Target_Object_Type = object_type;
                target_args++;
            } else if (target_args == 2) {
                if (!bacnet_strtoul(argv[argi], &object_instance)) {
                    fprintf(stderr, "object-instance=%s invalid\n", argv[argi]);
                    return 1;
                }
                if (object_instance > BACNET_MAX_INSTANCE) {
                    fprintf(
                        stderr, "object-instance=%u - not greater than %u\n",
                        Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
                    return 1;
                }
                Target_Object_Instance = object_instance;
                if (Verbose) {
                    printf("Instance=%lu=%s\n", object_instance, argv[argi]);
                }
                target_args++;
            }
        }
    }
    if (target_args < 2) {
        print_usage(filename);
        return 0;
    }
    /* setup my info */
    address_init();
    if (specific_address) {
        bacnet_address_init(&dest, &mac, dnet, &adr);
        address_add(Target_Device_Object_Instance, MAX_APDU, &dest);
    }
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    dlenv_init();
    atexit(datalink_cleanup);
    mstimer_init();
    mstimer_set(&apdu_timer, apdu_timeout());
    mstimer_set(&maintenance_timer, 1000);
    /* try to bind with the device */
    found = address_bind_request(
        Target_Device_Object_Instance, &max_apdu, &Target_Address);
    if (found) {
        if (Verbose) {
            printf(
                "Found Device %u in address_cache.\n",
                Target_Device_Object_Instance);
        }
    } else {
        Send_WhoIs(
            Target_Device_Object_Instance, Target_Device_Object_Instance);
    }
    /* loop forever */
    for (;;) {
        if (found) {
            /* device is bound! */
            if (Request_Invoke_ID == 0) {
                if (Verbose) {
                    printf(
                        "Sending DeleteObject to Device %u.\n",
                        Target_Device_Object_Instance);
                }
                Request_Invoke_ID = Send_Delete_Object_Request(
                    Target_Device_Object_Instance, Target_Object_Type,
                    Target_Object_Instance);
            } else if (tsm_invoke_id_free(Request_Invoke_ID)) {
                break;
            } else if (tsm_invoke_id_failed(Request_Invoke_ID)) {
                MyPrintHandler(
                    ERROR_CLASS_COMMUNICATION, ERROR_CODE_ABORT_TSM_TIMEOUT);
                tsm_free_invoke_id(Request_Invoke_ID);
                Error_Detected = true;
                /* abort */
                break;
            }
        } else {
            found = address_bind_request(
                Target_Device_Object_Instance, &max_apdu, &Target_Address);
        }
        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        if (mstimer_expired(&maintenance_timer)) {
            mstimer_reset(&maintenance_timer);
            tsm_timer_milliseconds(mstimer_interval(&maintenance_timer));
            datalink_maintenance_timer(
                mstimer_interval(&maintenance_timer) / 1000L);
        }
        if (mstimer_expired(&apdu_timer)) {
            MyPrintHandler(
                ERROR_CLASS_COMMUNICATION,
                ERROR_CODE_ABORT_APPLICATION_EXCEEDED_REPLY_TIME);
            Error_Detected = true;
        }
        if (Error_Detected) {
            break;
        }
    }
    if (Error_Detected) {
        return 1;
    }

    return 0;
}
