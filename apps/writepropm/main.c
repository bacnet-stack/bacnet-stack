/**
 * @file
 * @brief command line tool that uses BACnet WritePropertyMultiple service
 * message to write object property values to another device on
 * the network and prints an acknowledgment or error response of
 * this confirmed service request.  This is useful for testing
 * the WritePropertyMultiple service.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2017
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <time.h> /* for time */
#define PRINT_ENABLED 1
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/bacerror.h"
#include "bacnet/iam.h"
#include "bacnet/apdu.h"
#include "bacnet/arf.h"
#include "bacnet/npdu.h"
#include "bacnet/rpm.h"
#include "bacnet/whois.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
#include "bacport.h"

#if BACNET_SVC_SERVER
#error "App requires server-only features disabled! Set BACNET_SVC_SERVER=0"
#endif

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_WRITE_ACCESS_DATA *Write_Access_Data;
/* needed to filter incoming messages */
static uint8_t Request_Invoke_ID = 0;
static BACNET_ADDRESS Target_Address;
/* needed for return value of main application */
static bool Error_Detected = false;
/* Used for verbose */
static bool Verbose = false;

static void MyWritePropertyMultipleErrorHandler(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    uint8_t service_choice,
    uint8_t *service_request,
    uint16_t service_len)
{
    int len = 0;
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };

    (void)service_choice;
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        len = wpm_error_ack_decode_apdu(service_request, service_len, &wp_data);
        if (len > 0) {
            printf(
                "BACnet Error: %s: %s\n",
                bactext_error_class_name((int)wp_data.error_class),
                bactext_error_code_name((int)wp_data.error_code));
            printf(
                "BACnet Error: %s %u: %s\n",
                bactext_object_type_name((int)wp_data.object_type),
                (unsigned)wp_data.object_instance,
                bactext_property_name((int)wp_data.object_property));
        }
        Error_Detected = true;
    }
}

static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    (void)server;
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf(
            "BACnet Abort: %s\n", bactext_abort_reason_name((int)abort_reason));
        Error_Detected = true;
    }
}

static void
MyRejectHandler(BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf(
            "BACnet Reject: %s\n",
            bactext_reject_reason_name((int)reject_reason));
        Error_Detected = true;
    }
}

static void
MyWritePropertyMultipleSimpleAckHandler(BACNET_ADDRESS *src, uint8_t invoke_id)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("\nWriteProperty Acknowledged!\n");
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
    /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE,
        MyWritePropertyMultipleSimpleAckHandler);
    /* handle any errors coming back */
    apdu_set_complex_error_handler(
        SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE,
        MyWritePropertyMultipleErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void cleanup(void)
{
    BACNET_WRITE_ACCESS_DATA *wpm_object;
    BACNET_WRITE_ACCESS_DATA *old_wpm_object;
    BACNET_PROPERTY_VALUE *wpm_property;
    BACNET_PROPERTY_VALUE *old_wpm_property;

    wpm_object = Write_Access_Data;
    old_wpm_object = wpm_object;
    while (wpm_object) {
        wpm_property = wpm_object->listOfProperties;
        while (wpm_property) {
            old_wpm_property = wpm_property;
            wpm_property = wpm_property->next;
            free(old_wpm_property);
        }
        old_wpm_object = wpm_object;
        wpm_object = wpm_object->next;
        free(old_wpm_object);
    }
}

static void print_usage(const char *filename)
{
    printf(
        "Usage: %s device-instance object-type object-instance "
        "property[index] priority tag value [property[index] priority tag "
        "value]\n",
        filename);
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf("Write one or more properties to one or more objects\n"
           "in a BACnet device.\n");
    printf("\n");
    printf("device-instance:\n"
           "BACnet Device Object Instance number that you are\n"
           "trying to communicate to.  This number will be used\n"
           "to try and bind with the device using Who-Is and\n"
           "I-Am services.  For example, if you were writing\n"
           "Device Object 123, the device-instance would be 123.\n");
    printf("\n");
    printf("object-type:\n"
           "The object type is object that you are writing. It\n"
           "can be defined either as the object-type name string\n"
           "as defined in the BACnet specification, or as the\n"
           "integer value of the enumeration BACNET_OBJECT_TYPE\n"
           "in bacenum.h. For example if you were writing Analog\n"
           "Output 2, the object-type would be analog-output or 1.\n");
    printf("\n");
    printf("object-instance:\n"
           "This is the object instance number of the object that\n"
           "you are writing.  For example, if you were writing\n"
           "Analog Output 2, the object-instance would be 2.\n");
    printf("\n");
    printf("property:\n"
           "The property is an integer value of the enumeration\n"
           "BACNET_PROPERTY_ID in bacenum.h.  It is the property\n"
           "you are writing.  For example, if you were writing the\n"
           "Present Value property, use 85 as the property.\n");
    printf("\n");
    printf("priority:\n"
           "This parameter is used for setting the priority of the\n"
           "write. If Priority 0 is given, no priority is sent.  The BACnet \n"
           "standard states that the value is written at the lowest \n"
           "priority (16) if the object property supports priorities\n"
           "when no priority is sent.\n");
    printf("\n");
    printf(
        "index:\n"
        "This integer parameter is the index number of an array.\n"
        "If the property is an array, individual elements can be written\n"
        "to if supported.  If this parameter is -1, the index is ignored.\n");
    printf("\n");
    printf(
        "tag:\n"
        "Tag is the integer value of the enumeration BACNET_APPLICATION_TAG \n"
        "in bacenum.h.  It is the data type of the value that you are\n"
        "writing.  For example, if you were writing a REAL value, you would \n"
        "use a tag of 4.\n"
        "Context tags are created using two tags in a row.  The context tag\n"
        "is preceded by a C.  Ctag tag. C2 4 creates a context 2 tagged "
        "REAL.\n");
    printf("Complex data use the property argument and a tag number -1 to\n"
           "lookup the appropriate internal application tag for the value.\n"
           "The complex data value argument varies in its construction.\n");
    printf("\n");
    printf(
        "value:\n"
        "The value is an ASCII representation of some type of data that you\n"
        "are writing.  It is encoded using the tag information provided.  For\n"
        "example, if you were writing a REAL value of 100.0, you would use \n"
        "100.0 as the value.\n");
    printf("\n");
    printf(
        "Here is a brief overview of BACnet property and tags:\n"
        "Certain properties are expected to be written with certain \n"
        "application tags, so you probably need to know which ones to use\n"
        "with each property of each object.  It is almost safe to say that\n"
        "given a property and an object and a table, the tag could be looked\n"
        "up automatically.  There may be a few exceptions to this, such as\n"
        "the Any property type in the schedule object and the Present Value\n"
        "accepting REAL, BOOLEAN, NULL, etc.\n");
    printf("Perhaps it would be simpler for the demo to use this\n"
           "kind of table - but this tool can also be used for negative\n"
           "testing by passing the wrong tag to validate that the server\n"
           "returns an error, reject, or abort message.\n");
    printf("\n");
    printf(
        "Example:\n"
        "If you want write a value of 100 to the Present-Value in\n"
        "Analog Output 44 and 45 of Device 123 at priority 16,\n"
        "send the following command:\n"
        "%s 123 1 44 85 16 4 100 1 45 85 16 4 100\n",
        filename);
}

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100; /* milliseconds */
    unsigned max_apdu = 0;
    int args_remaining = 0, tag_value_arg = 0, arg_sets = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    bool found = false;
    uint8_t buffer[MAX_PDU] = { 0 };
    BACNET_WRITE_ACCESS_DATA *wpm_object;
    BACNET_PROPERTY_VALUE *wpm_property;
    char *value_string = NULL;
    bool status = false;
    long property_tag;
    long priority;
    uint8_t context_tag = 0;
    unsigned object_type = 0;
    unsigned property_id = 0;
    unsigned property_array_index = 0;
    int scan_count = 0;
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
            printf("Copyright (C) 2017 by Steve Karg and others.\n"
                   "This is free software; see the source for copying "
                   "conditions.\n"
                   "There is NO warranty; not even for MERCHANTABILITY or\n"
                   "FITNESS FOR A PARTICULAR PURPOSE.\n");
            return 0;
        }
    }
    if (argc < 8) {
        print_usage(filename);
        return 0;
    }
    /* decode the command line parameters */
    Target_Device_Object_Instance = strtol(argv[1], NULL, 0);
    if (Target_Device_Object_Instance > BACNET_MAX_INSTANCE) {
        fprintf(
            stderr, "device-instance=%u - not greater than %u\n",
            Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
        return 1;
    }
    atexit(cleanup);
    Write_Access_Data = calloc(1, sizeof(BACNET_WRITE_ACCESS_DATA));
    wpm_object = Write_Access_Data;
    args_remaining = (argc - 2);
    arg_sets = 0;
    while (wpm_object) {
        tag_value_arg = 2 + (arg_sets * 6);
        if (bactext_object_type_strtol(argv[tag_value_arg], &object_type) ==
            false) {
            fprintf(
                stderr, "Error: object-type=%s invalid\n", argv[tag_value_arg]);
            return 1;
        }
        wpm_object->object_type = object_type;
        tag_value_arg++;
        args_remaining--;
        if (Verbose) {
            printf("object-type=%u\n", wpm_object->object_type);
        }
        if (args_remaining <= 0) {
            fprintf(stderr, "Error: not enough object property quadruples.\n");
            return 1;
        }
        if (wpm_object->object_type >= MAX_BACNET_OBJECT_TYPE) {
            fprintf(
                stderr, "object-type=%u - it must be less than %u\n",
                wpm_object->object_type, MAX_BACNET_OBJECT_TYPE);
            return 1;
        }
        wpm_object->object_instance = strtol(argv[tag_value_arg], NULL, 0);
        tag_value_arg++;
        args_remaining--;
        if (Verbose) {
            printf("object-instance=%u\n", wpm_object->object_instance);
        }
        if (args_remaining <= 0) {
            fprintf(stderr, "Error: not enough object property quadruples.\n");
            return 1;
        }
        if (wpm_object->object_instance > BACNET_MAX_INSTANCE) {
            fprintf(
                stderr, "object-instance=%u - not greater than %u\n",
                wpm_object->object_instance, BACNET_MAX_INSTANCE);
            return 1;
        }
        do {
            wpm_property = calloc(1, sizeof(BACNET_PROPERTY_VALUE));
            wpm_object->listOfProperties = wpm_property;
            if (wpm_property) {
                /* Property[index] */
                scan_count = sscanf(
                    argv[tag_value_arg], "%u[%u]", &property_id,
                    &property_array_index);
                tag_value_arg++;
                args_remaining--;
                if (scan_count > 0) {
                    wpm_property->propertyIdentifier = property_id;
                    if (Verbose) {
                        printf(
                            "property-identifier=%u, array-index=%u\n",
                            property_id, property_array_index);
                    }
                    if (wpm_property->propertyIdentifier >
                        MAX_BACNET_PROPERTY_ID) {
                        fprintf(
                            stderr, "property=%u - it must be less than %u\n",
                            wpm_property->propertyIdentifier,
                            MAX_BACNET_PROPERTY_ID + 1);
                        return 1;
                    }
                }
                if (scan_count > 1) {
                    wpm_property->propertyArrayIndex = property_array_index;
                } else {
                    wpm_property->propertyArrayIndex = BACNET_ARRAY_ALL;
                }
                if (args_remaining <= 0) {
                    fprintf(
                        stderr,
                        "Error: missing priority and tag value pair.\n");
                    return 1;
                }
                /* Priority */
                priority = strtol(argv[tag_value_arg], NULL, 0);
                if ((priority < BACNET_MIN_PRIORITY) ||
                    (priority > BACNET_MAX_PRIORITY)) {
                    priority = BACNET_NO_PRIORITY;
                }
                wpm_property->priority = priority;
                tag_value_arg++;
                args_remaining--;
                if (Verbose) {
                    printf("priority=%u\n", wpm_property->priority);
                }
                if (args_remaining <= 0) {
                    fprintf(stderr, "Error: missing tag value pair.\n");
                    return 1;
                }
                /* Tag + Value */
                /* special case for context tagged values */
                if (toupper(argv[tag_value_arg][0]) == 'C') {
                    context_tag =
                        (uint8_t)strtol(&argv[tag_value_arg][1], NULL, 0);
                    tag_value_arg++;
                    args_remaining--;
                    wpm_property->value.context_tag = context_tag;
                    wpm_property->value.context_specific = true;
                } else {
                    wpm_property->value.context_specific = false;
                }
                property_tag = strtol(argv[tag_value_arg], NULL, 0);
                tag_value_arg++;
                args_remaining--;
                if (args_remaining <= 0) {
                    fprintf(
                        stderr, "Error: missing value for tag-value pair\n");
                    return 1;
                }
                value_string = argv[tag_value_arg];
                tag_value_arg++;
                args_remaining--;
                if (Verbose) {
                    printf("tag=%ld value=%s\n", property_tag, value_string);
                }
                if (property_tag < 0) {
                    property_tag = bacapp_known_property_tag(
                        wpm_object->object_type,
                        wpm_property->propertyIdentifier);
                } else if (property_tag >= MAX_BACNET_APPLICATION_TAG) {
                    fprintf(
                        stderr, "Error: tag=%ld - it must be less than %u\n",
                        property_tag, MAX_BACNET_APPLICATION_TAG);
                    return 1;
                }
                if (property_tag >= 0) {
                    status = bacapp_parse_application_data(
                        property_tag, value_string, &wpm_property->value);
                    if (!status) {
                        /* FIXME: show the expected entry format for the tag */
                        fprintf(
                            stderr, "Error: unable to parse the tag value\n");
                        return 1;
                    }
                } else {
                    fprintf(
                        stderr,
                        "Error: parser for property %s is not implemented\n",
                        bactext_property_name(
                            wpm_property->propertyIdentifier));
                    return 1;
                }
                wpm_property->value.next = NULL;
                /* we only support a single property value */
                break;
            }
        } while (wpm_property);
        if (args_remaining) {
            arg_sets++;
            wpm_object->next = calloc(1, sizeof(BACNET_WRITE_ACCESS_DATA));
            wpm_object = wpm_object->next;
        } else {
            break;
        }
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    address_init();
    Init_Service_Handlers();
    dlenv_init();
    atexit(datalink_cleanup);
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = (apdu_timeout() / 1000) * apdu_retries();
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
        /* increment timer - exit if timed out */
        current_seconds = time(NULL);

        /* at least one second has passed */
        if (current_seconds != last_seconds) {
            tsm_timer_milliseconds(((current_seconds - last_seconds) * 1000));
            datalink_maintenance_timer(current_seconds - last_seconds);
        }
        if (Error_Detected) {
            break;
        }
        /* wait until the device is bound, or timeout and quit */
        if (!found) {
            found = address_bind_request(
                Target_Device_Object_Instance, &max_apdu, &Target_Address);
        }
        if (found) {
            if (Request_Invoke_ID == 0) {
                if (Verbose) {
                    printf(
                        "Sending WritePropertyMultiple to Device %u.\n",
                        Target_Device_Object_Instance);
                }
                Request_Invoke_ID = Send_Write_Property_Multiple_Request(
                    &buffer[0], sizeof(buffer), Target_Device_Object_Instance,
                    Write_Access_Data);
                if (Request_Invoke_ID == 0) {
                    fprintf(stderr, "\rError: failed to send request!\n");
                    break;
                }
            } else if (tsm_invoke_id_free(Request_Invoke_ID)) {
                break;
            } else if (tsm_invoke_id_failed(Request_Invoke_ID)) {
                fprintf(stderr, "\rError: TSM Timeout!\n");
                tsm_free_invoke_id(Request_Invoke_ID);
                Error_Detected = true;
                /* try again or abort? */
                break;
            }
        } else {
            /* increment timer - exit if timed out */
            elapsed_seconds += (current_seconds - last_seconds);
            if (elapsed_seconds > timeout_seconds) {
                printf("\rError: APDU Timeout!\n");
                Error_Detected = true;
                break;
            }
        }
        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }

        /* keep track of time for next check */
        last_seconds = current_seconds;
    }
    if (Error_Detected) {
        return 1;
    }

    return 0;
}
