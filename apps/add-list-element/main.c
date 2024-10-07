/**
 * @file
 * @brief Application to send a BACnet AddListElement service request
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date January 2023
 *
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#define PRINT_ENABLED 0
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
#include "bacnet/list_element.h"
#include "bacnet/whois.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/basic/object/device.h"

#if BACNET_SVC_SERVER
#error "App requires server-only features disabled! Set BACNET_SVC_SERVER=0"
#endif

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };
/* target device data for the request */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_OBJECT_TYPE Target_Object_Type;
static uint32_t Target_Object_Instance;
static BACNET_PROPERTY_ID Target_Object_Property;
static BACNET_ARRAY_INDEX Target_Object_Array_Index;
static BACNET_APPLICATION_DATA_VALUE Target_Object_Value;
/* needed to filter incoming messages */
static uint8_t Request_Invoke_ID = 0;
static BACNET_ADDRESS Target_Address;
/* needed for return value of main application */
static bool Error_Detected = false;
/* Used for verbose */
static bool Verbose = false;

static void MyAddListElementErrorHandler(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    uint8_t service_choice,
    uint8_t *service_request,
    uint16_t service_len)
{
    int len = 0;
    BACNET_LIST_ELEMENT_DATA list_element = { 0 };

    (void)service_choice;
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        len = list_element_error_ack_decode(
            service_request, service_len, &list_element);
        if (len > 0) {
            printf(
                "BACnet Error: %s: %s [first-failed=%u]\n",
                bactext_error_class_name((int)list_element.error_class),
                bactext_error_code_name((int)list_element.error_code),
                (unsigned)list_element.first_failed_element_number);
        }
        Error_Detected = true;
    }
}

static void
MyAddListElementSimpleAckHandler(BACNET_ADDRESS *src, uint8_t invoke_id)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("AddListElement Acknowledged!\n");
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
        SERVICE_CONFIRMED_ADD_LIST_ELEMENT, MyAddListElementSimpleAckHandler);
    apdu_set_complex_error_handler(
        SERVICE_CONFIRMED_ADD_LIST_ELEMENT, MyAddListElementErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void print_usage(const char *filename)
{
    printf(
        "Usage: %s device-instance object-type object-instance "
        "property array-index tag value\n",
        filename);
    printf("       [--dnet][--dadr][--mac]\n");
    printf("       [--version][--help][--verbose]\n");
}

static void print_help(const char *filename)
{
    printf("Add a BACnetLIST element to a property of an object\n"
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
           "The object type is object that you are reading. It\n"
           "can be defined either as the object-type name string\n"
           "as defined in the BACnet specification, or as the\n"
           "integer value of the enumeration BACNET_OBJECT_TYPE\n"
           "in bacenum.h. For example if you were reading Analog\n"
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
    printf(
        "array-index:\n"
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
        "is preceded by a C.  Ctag tag. C2 4 creates a context 2 tagged REAL.\n"
        "Complex data uses a tag of -1, and relies on the property\n"
        "to choose the appropriate parser for the value(s).\n");
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
    printf("\n");
    printf(
        "Example:\n"
        "If you want to AddListElement to the Recipient-List property in\n"
        "Notification Class 1 of Device 123, send the following command:\n"
        "%s 123 15 1 102 -1 4 100\n",
        filename);
}

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100; /* milliseconds */
    unsigned max_apdu = 0;
    int tag_value_arg = 0;
    struct mstimer apdu_timer;
    struct mstimer maintenance_timer;
    bool found = false;
    char *value_string = NULL;
    bool status = false;
    unsigned context_tag = 0;
    BACNET_APPLICATION_DATA_VALUE *application_value = NULL;
    unsigned object_type = 0;
    unsigned object_instance = 0;
    unsigned property_id = 0;
    unsigned property_array_index = 0;
    long property_tag = 0;
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
                dnet = strtol(argv[argi], NULL, 0);
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
                object_instance = strtoul(argv[argi], NULL, 0);
                if (object_instance > BACNET_MAX_INSTANCE) {
                    fprintf(
                        stderr, "device-instance=%u - not greater than %u\n",
                        object_instance, BACNET_MAX_INSTANCE);
                    return 1;
                }
                Target_Device_Object_Instance = object_instance;
                target_args++;
            } else if (target_args == 1) {
                if (bactext_object_type_strtol(argv[argi], &object_type) ==
                    false) {
                    fprintf(stderr, "object-type=%s invalid\n", argv[argi]);
                    return 1;
                }
                Target_Object_Type = object_type;
                target_args++;
            } else if (target_args == 2) {
                object_instance = strtoul(argv[argi], NULL, 0);
                if (object_instance > BACNET_MAX_INSTANCE) {
                    fprintf(
                        stderr, "device-instance=%u - not greater than %u\n",
                        Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
                    return 1;
                }
                Target_Object_Instance = object_instance;
                if (Verbose) {
                    printf("Instance=%u=%s\n", object_instance, argv[argi]);
                }
                target_args++;
            } else if (target_args == 3) {
                if (bactext_property_strtol(argv[argi], &property_id) ==
                    false) {
                    fprintf(stderr, "property=%s invalid\n", argv[argi]);
                    return 1;
                }
                Target_Object_Property = property_id;
                if (Verbose) {
                    printf("Property=%u=%s\n", property_id, argv[argi]);
                }
                target_args++;
            } else if (target_args == 4) {
                property_array_index = strtol(argv[argi], NULL, 0);
                Target_Object_Array_Index = property_array_index;
                if (Verbose) {
                    printf(
                        "Array_Index=%i=%s\n", property_array_index,
                        argv[argi]);
                }
                target_args++;
                tag_value_arg = 0;
                application_value = &Target_Object_Value;
            } else if (target_args == 5) {
                /* Tag + Value or Cn + Tag + Value */
                if (tag_value_arg == 0) {
                    /* special case for context tagged values */
                    if (toupper(argv[argi][0]) == 'C') {
                        context_tag = strtoul(&argv[target_args][1], NULL, 0);
                        application_value->context_tag = context_tag;
                        application_value->context_specific = true;
                        argi++;
                    } else {
                        application_value->context_specific = false;
                    }
                    /* application tag */
                    property_tag = strtol(argv[argi], NULL, 0);
                    if (Verbose) {
                        printf("tag=%ld\n", property_tag);
                    }
                    tag_value_arg++;
                } else if (tag_value_arg == 1) {
                    value_string = argv[argi];
                    if (Verbose) {
                        printf(
                            "tag=%ld value=%s\n", property_tag, value_string);
                    }
                    if (property_tag < 0) {
                        /* find the application tag for internal properties */
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
                        /* note: this will also set the tag to property_tag */
                        status = bacapp_parse_application_data(
                            property_tag, value_string, application_value);
                        if (!status) {
                            fprintf(
                                stderr,
                                "Error: unable to parse the tag value\n");
                            return 1;
                        }
                    } else {
                        /* FIXME: show the expected entry format for the tag */
                        fprintf(
                            stderr,
                            "Error: unable to parse the known property"
                            " \"%s\"\r\n",
                            value_string);
                        return 1;
                    }
                    /* do the next tag+value */
                    tag_value_arg = 0;
                    /* we only support a single property value */
                    application_value->next = NULL;
                    break;
                }
            }
        }
    }
    if (target_args != 5) {
        print_usage(filename);
        return 0;
    }
    if (tag_value_arg != 0) {
        fprintf(
            stderr, "Error: invalid tag+value pairs (%i).\n", tag_value_arg);
        return 1;
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
                        "Sending AddListElement to Device %u.\n",
                        Target_Device_Object_Instance);
                }
                Request_Invoke_ID = Send_Add_List_Element_Request(
                    Target_Device_Object_Instance, Target_Object_Type,
                    Target_Object_Instance, Target_Object_Property,
                    &Target_Object_Value, Target_Object_Array_Index);
            } else if (tsm_invoke_id_free(Request_Invoke_ID)) {
                break;
            } else if (tsm_invoke_id_failed(Request_Invoke_ID)) {
                fprintf(stderr, "\rError: TSM Timeout!\n");
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
            printf("\rError: APDU Timeout!\n");
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
