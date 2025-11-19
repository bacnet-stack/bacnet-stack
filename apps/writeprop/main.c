/**
 * @file
 * @brief command line tool that uses BACnet WriteProperty service
 * message to write object property values to another device on
 * the network and prints an acknowledgment or error response of
 * this confirmed service request.  This is useful for testing
 * the WriteProperty service.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006-2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* for time */
#include <string.h>
#include <ctype.h> /* toupper */
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/iam.h"
#include "bacnet/arf.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/whois.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/dlenv.h"
#include "bacport.h"

#ifndef MAX_PROPERTY_VALUES
#define MAX_PROPERTY_VALUES 64
#endif

#if BACNET_SVC_SERVER
#error "App requires server-only features disabled! Set BACNET_SVC_SERVER=0"
#endif

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static uint32_t Target_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_OBJECT_TYPE Target_Object_Type = OBJECT_ANALOG_INPUT;
static BACNET_PROPERTY_ID Target_Object_Property = PROP_ACKED_TRANSITIONS;
/* array index value or BACNET_ARRAY_ALL */
static int32_t Target_Object_Property_Index = BACNET_ARRAY_ALL;
static BACNET_APPLICATION_DATA_VALUE
    Target_Object_Property_Value[MAX_PROPERTY_VALUES];

/* 0 if not set, 1..16 if set */
static uint8_t Target_Object_Property_Priority = 0;

/* needed to filter incoming messages */
static uint8_t Request_Invoke_ID = 0;
static BACNET_ADDRESS Target_Address;
/* needed for return value of main application */
static bool Error_Detected = false;

static void MyErrorHandler(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf(
            "BACnet Error: %s: %s\n",
            bactext_error_class_name((int)error_class),
            bactext_error_code_name((int)error_code));
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
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf(
            "BACnet Reject: %s\n",
            bactext_reject_reason_name((int)reject_reason));
        Error_Detected = true;
    }
}

static void
MyWritePropertySimpleAckHandler(BACNET_ADDRESS *src, uint8_t invoke_id)
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
    /* handle the ack coming back */
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, MyWritePropertySimpleAckHandler);
    /* handle any errors coming back */
    apdu_set_error_handler(SERVICE_CONFIRMED_WRITE_PROPERTY, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void print_usage(const char *filename)
{
    printf(
        "Usage: %s device-instance object-type object-instance "
        "property priority index tag value [tag value...]\n",
        filename);
}

static void print_help(const char *filename)
{
    printf(
        "device-instance:\n"
        "BACnet Device Object Instance number that you are trying to\n"
        "communicate to.  This number will be used to try and bind with\n"
        "the device using Who-Is and I-Am services.  For example, if you were\n"
        "writing to Device Object 123, the device-instance would be 123.\n");
    printf("\n");
    printf("object-type:\n"
           "The object type is object that you are writing. It\n"
           "can be defined either as the object-type name string\n"
           "as defined in the BACnet specification, or as the\n"
           "integer value of the enumeration BACNET_OBJECT_TYPE\n"
           "in bacenum.h. For example if you were writing Analog\n"
           "Output 2, the object-type would be analog-output or 1.\n");
    printf("\n");
    printf(
        "object-instance:\n"
        "This is the object instance number of the object that you are \n"
        "writing to.  For example, if you were writing to Analog Output 2, \n"
        "the object-instance would be 2.\n");
    printf("\n");
    printf("property:\n"
           "The property of the object that you are writing. It\n"
           "can be defined either as the property name string as\n"
           "defined in the BACnet specification, or as an integer\n"
           "value of the enumeration BACNET_PROPERTY_ID in\n"
           "bacenum.h. For example, if you were writing the Present\n"
           "Value property, use present-value or 85 as the property.\n");
    printf("\n");
    printf("priority:\n"
           "This parameter is used for setting the priority of the\n"
           "write. If Priority 0 is given, no priority is sent.  The BACnet \n"
           "standard states that the value is written at the lowest \n"
           "priority (16) if the object property supports priorities\n"
           "when no priority is sent.\n");
    printf("\n");
    printf(
        "index\n"
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
        "is preceded by a C, and followed by the application tag.\n"
        "Ctag atag. C2 4 creates a context 2 tagged REAL.\n");
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
        "Example:\n"
        "If you want send a value of 100 to the Present-Value in\n"
        "Analog Output 0 of Device 123 at priority 16,\n"
        "send the one of following commands:\n"
        "%s 123 analog-output 0 present-value 16 -1 4 100\n"
        "%s 123 1 0 85 16 -1 4 100\n",
        filename, filename);
    printf(
        "To send a relinquish command to the same object:\n"
        "%s 123 analog-output 0 present-value 16 -1 0 0\n"
        "%s 123 1 0 85 16 -1 0 0\n",
        filename, filename);
}

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100; /* milliseconds */
    unsigned max_apdu = 0;
    unsigned object_type = 0;
    unsigned object_property = 0;
    unsigned property_value_count = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    bool found = false;
    long dnet = -1;
    BACNET_MAC_ADDRESS mac = { 0 };
    BACNET_MAC_ADDRESS adr = { 0 };
    BACNET_ADDRESS dest = { 0 };
    bool specific_address = false;
    char *value_string;
    bool status = false;
    long property_tag;
    long priority;
    uint8_t context_tag = 0;
    int argi = 0;
    unsigned int target_args = 0;
    bool debug_enabled = false;
    const char *filename = NULL;

    filename = filename_remove_path(argv[0]);
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--help") == 0) {
            /* print help if requested */
            print_usage(filename);
            print_help(filename);
            return 0;
        }
        if (strcmp(argv[argi], "--version") == 0) {
            printf("%s %s\n", filename, BACNET_VERSION_TEXT);
            printf("Copyright (C) 2014 by Steve Karg\n"
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
                if ((dnet >= 0) && (dnet <= BACNET_BROADCAST_NETWORK)) {
                    specific_address = true;
                }
            }
        } else if (strcmp(argv[argi], "--dadr") == 0) {
            if (++argi < argc) {
                if (bacnet_address_mac_from_ascii(&adr, argv[argi])) {
                    specific_address = true;
                }
            }
        } else if (strcmp(argv[argi], "--debug") == 0) {
            debug_enabled = true;
        } else {
            if (target_args == 0) {
                Target_Device_Object_Instance = strtol(argv[argi], NULL, 0);
                if (Target_Device_Object_Instance > BACNET_MAX_INSTANCE) {
                    fprintf(
                        stderr, "device-instance=%u - not greater than %u\n",
                        Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
                    return 1;
                }
                target_args++;
            } else if (target_args == 1) {
                if (bactext_object_type_strtol(argv[argi], &object_type) ==
                    false) {
                    fprintf(stderr, "object-type=%s invalid\n", argv[argi]);
                    return 1;
                }
                Target_Object_Type = object_type;
                if (Target_Object_Type > MAX_BACNET_OBJECT_TYPE) {
                    fprintf(
                        stderr, "object-type=%u - it must be less than %u\n",
                        Target_Object_Type, MAX_BACNET_OBJECT_TYPE + 1);
                    return 1;
                }
                target_args++;
            } else if (target_args == 2) {
                Target_Object_Instance = strtol(argv[argi], NULL, 0);
                if (Target_Object_Instance > BACNET_MAX_INSTANCE) {
                    fprintf(
                        stderr, "object-instance=%u - not greater than %u\n",
                        Target_Object_Instance, BACNET_MAX_INSTANCE);
                    return 1;
                }
                target_args++;
            } else if (target_args == 3) {
                if (bactext_property_strtol(argv[argi], &object_property) ==
                    false) {
                    fprintf(stderr, "property=%s invalid\n", argv[argi]);
                    return 1;
                }
                Target_Object_Property = object_property;
                if (Target_Object_Property > MAX_BACNET_PROPERTY_ID) {
                    fprintf(
                        stderr, "property=%u - it must be less than %u\n",
                        Target_Object_Property, MAX_BACNET_PROPERTY_ID + 1);
                    return 1;
                }
                target_args++;
            } else if (target_args == 4) {
                priority = strtol(argv[argi], NULL, 0);
                if ((priority < BACNET_MIN_PRIORITY) ||
                    (priority > BACNET_MAX_PRIORITY)) {
                    priority = BACNET_NO_PRIORITY;
                }
                Target_Object_Property_Priority = priority;
                target_args++;
            } else if (target_args == 5) {
                Target_Object_Property_Index = strtol(argv[argi], NULL, 0);
                if (Target_Object_Property_Index == -1) {
                    Target_Object_Property_Index = BACNET_ARRAY_ALL;
                }
                target_args++;
            } else {
                unsigned i;
                for (i = 0; i < MAX_PROPERTY_VALUES; i++) {
                    /* special case for context tagged values */
                    if (toupper(argv[argi][0]) == 'C') {
                        context_tag = (uint8_t)strtol(&argv[argi][1], NULL, 0);
                        argi++;
                        Target_Object_Property_Value[i].context_tag =
                            context_tag;
                        Target_Object_Property_Value[i].context_specific = true;
                    } else {
                        Target_Object_Property_Value[i].context_specific =
                            false;
                    }
                    if (argi >= argc) {
                        fprintf(stderr, "Error: not enough tag-value pairs\n");
                        return 1;
                    }
                    property_tag = strtol(argv[argi], NULL, 0);
                    argi++;
                    if (argi >= argc) {
                        fprintf(stderr, "Error: not enough tag-value pairs\n");
                        return 1;
                    }
                    value_string = argv[argi];
                    argi++;
                    if (property_tag < 0) {
                        property_tag = bacapp_known_property_tag(
                            Target_Object_Type, Target_Object_Property);
                    } else if (property_tag >= MAX_BACNET_APPLICATION_TAG) {
                        fprintf(
                            stderr,
                            "Error: tag=%ld - it must be less than %u\n",
                            property_tag, MAX_BACNET_APPLICATION_TAG);
                        return 1;
                    }
                    if (property_tag >= 0) {
                        status = bacapp_parse_application_data(
                            property_tag, value_string,
                            &Target_Object_Property_Value[i]);
                        if (!status) {
                            /* FIXME: show the expected entry format for the tag
                             */
                            fprintf(
                                stderr,
                                "Error: unable to parse the tag value\n");
                            return 1;
                        }
                    } else {
                        fprintf(
                            stderr,
                            "Error: parser for property %s is not "
                            "implemented\n",
                            bactext_property_name(Target_Object_Property));
                        return 1;
                    }
                    if (debug_enabled) {
                        BACNET_OBJECT_PROPERTY_VALUE debug_property;
                        uint8_t debug_apdu[MAX_APDU];
                        int debug_i, debug_len;
                        fprintf(
                            stderr, "Writing: %s=",
                            bactext_application_tag_name(property_tag));
                        debug_property.value = &Target_Object_Property_Value[i];
                        debug_property.array_index =
                            Target_Object_Property_Index;
                        bacapp_print_value(stderr, &debug_property);
                        fprintf(stderr, "\n");
                        debug_len = bacapp_encode_application_data(
                            debug_apdu, &Target_Object_Property_Value[i]);
                        fprintf(stderr, "APDU[%d]=", debug_len);
                        for (debug_i = 0; debug_i < debug_len; debug_i++) {
                            fprintf(stderr, "%02x ", debug_apdu[debug_i]);
                        }
                        fprintf(stderr, "\n");
                    }
                    Target_Object_Property_Value[i].next = NULL;
                    property_value_count++;
                    if (i > 0) {
                        /* more value pairs - link another value */
                        Target_Object_Property_Value[i - 1].next =
                            &Target_Object_Property_Value[i];
                    }
                    if (argi >= argc) {
                        break;
                    }
                }
                if (argi < argc) {
                    fprintf(
                        stderr, "Error: Exceeded %d tag-value pairs.\n",
                        MAX_PROPERTY_VALUES);
                    return 1;
                }
            }
        }
    }
    if (property_value_count == 0) {
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
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = (apdu_timeout() / 1000) * apdu_retries();
    /* try to bind with the device */
    found = address_bind_request(
        Target_Device_Object_Instance, &max_apdu, &Target_Address);
    if (!found) {
        Send_WhoIs(
            Target_Device_Object_Instance, Target_Device_Object_Instance);
    }
    /* loop forever */
    for (;;) {
        /* increment timer - exit if timed out */
        current_seconds = time(NULL);

        /* at least one second has passed */
        if (current_seconds != last_seconds) {
            tsm_timer_milliseconds(
                (uint16_t)((current_seconds - last_seconds) * 1000));
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
                Request_Invoke_ID = Send_Write_Property_Request(
                    Target_Device_Object_Instance, Target_Object_Type,
                    Target_Object_Instance, Target_Object_Property,
                    &Target_Object_Property_Value[0],
                    Target_Object_Property_Priority,
                    Target_Object_Property_Index);
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
                Error_Detected = true;
                printf("\rError: APDU Timeout!\n");
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
