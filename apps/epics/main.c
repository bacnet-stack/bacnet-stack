/**
 * @file
 * @brief command line tool to generate EPICS-usable output acquired from
 * a BACnet device on the network.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* for time */
#if (__STDC_VERSION__ >= 199901L) && defined(__STDC_ISO_10646__)
#include <locale.h>
#endif
#include <assert.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/iam.h"
#include "bacnet/arf.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/apdu.h"
#include "bacnet/whois.h"
#include "bacnet/rp.h"
#include "bacnet/proplist.h"
#include "bacnet/property.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/basic/bbmd/h_bbmd.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacepics.h"

/* (Doxygen note: The next two lines pull all the following Javadoc
 *  into the BACEPICS module.) */
/** @addtogroup BACEPICS
 * @{ */

/** This is the souped-up version of the program for generating EPICS files.
 * It now:
 * 1) Prepends the heading information (supported services, etc)
 * 2) Determines some basic device properties for the header.
 * 3) Postpends the tail information to complete the EPICS file.
 */

typedef enum {
    RESP_FAILED = 0,
    RESP_SUCCESS = 1,
    RESP_FAILED_TO_DECODE = 2,
    RESP_ERROR_CODE = 3,
    RESP_REJECT_CODE = 4,
    RESP_ABORT_CODE = 5,
    RESP_WAITING = 6,
    RESP_TIMEOUT = 7,
    RESP_TSM_FAILED = 8
} RESPONSE_STATUS;

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };
static uint8_t Rx_RP_Data[sizeof(BACNET_READ_ACCESS_DATA)] = { 0 };
RESPONSE_STATUS Response_Status;

/* target information converted from command line */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_ADDRESS Target_Address;
/* the invoke id is needed to filter incoming messages */
static uint8_t Request_Invoke_ID = 0;
/* loopback address to talk to myself */
/* = { 6, { 127, 0, 0, 1, 0xBA, 0xC0, 0 }, 0 }; */
#if defined(BACDL_BIP)
/* If set, use this as the source port. */
static uint16_t My_BIP_Port = 0;
#endif
static bool Provided_Targ_MAC = false;

/* any errors are picked up in main loop */
static bool Error_Detected = false;
static uint16_t Last_Error_Class = 0;
static uint16_t Last_Error_Code = 0;
/* Counts errors we couldn't get around */
static uint16_t Error_Count = 0;
/* Assume device can do RPM, to start */
static EPICS_STATES myState = INITIAL_BINDING;

static struct mstimer APDU_Timer;

typedef struct BACnet_RPM_Service_Data_t {
    BACNET_CONFIRMED_SERVICE_ACK_DATA service_data;
    BACNET_READ_ACCESS_DATA *rpm_data;
} BACNET_RPM_SERVICE_DATA;
static BACNET_RPM_SERVICE_DATA Read_Property_Multiple_Data;

typedef struct Property_List {
    BACNET_PROPERTY_ID property;
    uint32_t exported;
} PROPERTY_LIST;

static void MyErrorHandler(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        /* error on the request */
        Response_Status = RESP_ERROR_CODE;
        if (error_code != ERROR_CODE_READ_ACCESS_DENIED) {
            Error_Detected = true;
            Last_Error_Class = error_class;
            Last_Error_Code = error_code;
        }
    }
}

static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    (void)server;
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        Response_Status = RESP_ABORT_CODE;
        fprintf(
            stderr, "BACnet Abort: %s\n",
            bactext_abort_reason_name((int)abort_reason));
    }
}

static void
MyRejectHandler(BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        Response_Status = RESP_REJECT_CODE;
        fprintf(
            stderr, "BACnet Reject: %s\n",
            bactext_reject_reason_name((int)reject_reason));
    }
}

static void MyReadPropertyAckHandler(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    int len = 0;
    BACNET_READ_ACCESS_DATA *rp_data;

    if (address_match(&Target_Address, src) &&
        (service_data->invoke_id == Request_Invoke_ID)) {
        rp_data = (BACNET_READ_ACCESS_DATA *)&Rx_RP_Data[0];
        if (rp_data) {
            len = rp_ack_fully_decode_service_request(
                service_request, service_len, rp_data);
            memmove(
                &Read_Property_Multiple_Data.service_data, service_data,
                sizeof(BACNET_CONFIRMED_SERVICE_ACK_DATA));
            if (len > 0) {
                Read_Property_Multiple_Data.rpm_data = rp_data;
                Response_Status = RESP_SUCCESS;
            } else { /* failed decode for some reason */
                Error_Detected = true;
                Response_Status = RESP_FAILED_TO_DECODE;
            }
        }
    }
}

/** Handler for a Simple ACK PDU.
 *
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param invoke_id [in] the invokeID from the rejected message
 */
static void
MyWritePropertySimpleAckHandler(BACNET_ADDRESS *src, uint8_t invoke_id)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        Response_Status = RESP_SUCCESS;
    }
}

static void MyWritePropertyErrorHandler(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        Response_Status = RESP_ERROR_CODE;
        fprintf(
            stderr, "BACnet Error: %s:%s\n",
            bactext_error_class_name((int)error_class),
            bactext_error_code_name((int)error_code));
    }
}

static void Init_Service_Handlers(void)
{
#if BAC_ROUTING
    uint32_t Object_Instance;
    BACNET_CHARACTER_STRING name_string;
#endif

    Device_Init(NULL);

#if BAC_ROUTING
    /* Put this client Device into the Routing table (first entry) */
    Object_Instance = Device_Object_Instance_Number();
    Device_Object_Name(Object_Instance, &name_string);
    Add_Routed_Device(Object_Instance, &name_string, Device_Description());
#endif

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
    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, MyReadPropertyAckHandler);
    /* handle the ack coming back */
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, MyWritePropertySimpleAckHandler);
    apdu_set_error_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, MyWritePropertyErrorHandler);
    /* handle any errors coming back */
    apdu_set_error_handler(SERVICE_CONFIRMED_READ_PROPERTY, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static const char *protocol_services_supported_text(size_t bit_index)
{
    bool is_confirmed = false;
    size_t text_index = 0;
    bool found = false;
    const char *services_text = "unknown";

    found =
        apdu_service_supported_to_index(bit_index, &text_index, &is_confirmed);
    if (found) {
        if (is_confirmed) {
            services_text = bactext_confirmed_service_name(text_index);
        } else {
            services_text = bactext_unconfirmed_service_name(text_index);
        }
    }

    return services_text;
}

/** Provide a nicer output for Supported Services and Object Types bitfields
 * and Date fields.
 * We have to override the library's normal bitfield print because the
 * EPICS format wants just T and F, and we want to provide (as comments)
 * the names of the active types.
 * These bitfields use opening and closing parentheses instead of braces.
 * We also limit the output to 4 bit fields per line.
 *
 * @param stream [in] Normally stdout
 * @param object_value [in] The structure holding this property's description
 *                          and value.
 * @return True if success.  Or otherwise.
 */

static void PrettyPrintPropertyValue(
    FILE *stream, BACNET_OBJECT_PROPERTY_VALUE *object_value)
{
    BACNET_APPLICATION_DATA_VALUE *value = NULL;
    size_t len = 0, i = 0;
    BACNET_PROPERTY_ID property = PROP_ALL;
    char short_month[4];

    value = object_value->value;
    property = object_value->object_property;
    if ((value != NULL) && (value->tag == BACNET_APPLICATION_TAG_BIT_STRING) &&
        ((property == PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED) ||
         (property == PROP_PROTOCOL_SERVICES_SUPPORTED))) {
        len = bitstring_bits_used(&value->type.Bit_String);
        fprintf(stream, "( \n        ");
        for (i = 0; i < len; i++) {
            fprintf(
                stream, "%s",
                bitstring_bit(&value->type.Bit_String, (uint8_t)i) ? "T" : "F");
            if (i < len - 1) {
                fprintf(stream, ",");
            } else {
                fprintf(stream, " ");
            }
            /* Tried with 8 per line, but with the comments, got way too long.
             */
            if ((i == (len - 1)) || ((i % 4) == 3)) { /* line break every 4 */
                fprintf(stream, "\n        ");
            }
        }
        fprintf(stream, ") \n");
    } else if ((value != NULL) && (value->tag == BACNET_APPLICATION_TAG_DATE)) {
        /* eg, property == PROP_LOCAL_DATE
         * VTS needs (3-Aug-2011,4) or (8/3/11,4), so we'll use the
         * clearer, international form. */
        snprintf(
            short_month, sizeof(short_month), "%s",
            bactext_month_name(value->type.Date.month));
        fprintf(
            stream, "(%u-%3s-%u, %u)", (unsigned)value->type.Date.day,
            short_month, (unsigned)value->type.Date.year,
            (unsigned)value->type.Date.wday);
    } else if (value != NULL) {
        assert(false); /* How did I get here?  Fix your code. */
        /* Meanwhile, a fallback plan */
        bacapp_print_value(stdout, object_value);
    } else {
        fprintf(stream, "? \n");
    }
}

static void wait_for_response(void)
{
    uint16_t pdu_len = 0;
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    unsigned long timeout_ms;

    timeout_ms = apdu_timeout();
    timeout_ms *= apdu_retries();
    Response_Status = RESP_WAITING;
    mstimer_restart(&APDU_Timer);
    while (mstimer_expired(&APDU_Timer) == false) {
        /* Process PDU if one comes in */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout_ms);
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        if (tsm_invoke_id_free(Request_Invoke_ID)) {
            /* Response received. Exit. */
            /* Response_Status is set it response handler */
            return;
        }
        if (tsm_invoke_id_failed(Request_Invoke_ID)) {
            /* TSM Timeout */
            tsm_free_invoke_id(Request_Invoke_ID);
            Response_Status = RESP_TIMEOUT;
            return;
        }
        if ((Response_Status == RESP_ABORT_CODE) ||
            (Response_Status == RESP_REJECT_CODE) ||
            (Response_Status == RESP_ERROR_CODE)) {
            return;
        }
    }
    /* TSM is stuck - free invoke id */
    tsm_free_invoke_id(Request_Invoke_ID);
    Response_Status = RESP_TSM_FAILED;
}

bool Attempt_To_Write(
    BACNET_OBJECT_TYPE object_type, BACNET_PROPERTY_ID property)
{
    if (object_type >= OBJECT_PROPRIETARY_MIN) {
        /* don't attempt to write to any properties in a proprietary object */
        return false;
    }
    if ((property >= PROP_PROPRIETARY_RANGE_MIN) &&
        (property <= PROP_PROPRIETARY_RANGE_MAX)) {
        /* don't attempt to write to any proprietary properties */
        return false;
    }

    switch (property) {
        case MAX_BACNET_PROPERTY_ID:
        case PROP_PRIORITY_ARRAY:
        case PROP_STATUS_FLAGS:
        case PROP_PROPERTY_LIST:
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_TYPE:
        case PROP_OBJECT_LIST:
        case PROP_COMMAND:
        case PROP_SYSTEM_STATUS:
        case PROP_VENDOR_NAME:
        case PROP_VENDOR_IDENTIFIER:
        case PROP_MODEL_NAME:
        case PROP_FIRMWARE_REVISION:
        case PROP_APPLICATION_SOFTWARE_VERSION:
        case PROP_PROTOCOL_VERSION:
        case PROP_PROTOCOL_REVISION:
        case PROP_PROTOCOL_SERVICES_SUPPORTED:
        case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
        case PROP_SEGMENTATION_SUPPORTED:
        case PROP_DATABASE_REVISION:
        case PROP_EVENT_TIME_STAMPS:
        case PROP_LOG_BUFFER:
            /* read-only or complex data types - don't attempt to write */
            return false;
        default:
            /* try to write everything else */
            return true;
    }
}

static void
print_value(FILE *stream, BACNET_OBJECT_PROPERTY_VALUE *object_value)
{
    switch (object_value->value->tag) {
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            fprintf(stdout, "X'");
            bacapp_print_value(stream, object_value);
            fprintf(stdout, "'");
            break;
        default:
            bacapp_print_value(stream, object_value);
            break;
    }
}

/** Print out the value(s) for one Property.
 * This function may be called repeatedly for one property if we are walking
 * through a list
 *
 * @param object_type [in] The BACnet Object type of this object.
 * @param object_instance [in] The ID number for this object.
 * @param rpm_property [in] Points to structure holding the Property,
 *                          Value, and Error information.
 */
static void PrintReadPropertyData(
    FILE *stream,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_REFERENCE *rpm_property)
{
    BACNET_OBJECT_PROPERTY_VALUE object_value; /* for bacapp printing */
    BACNET_APPLICATION_DATA_VALUE *value, *old_value;
    bool is_array = false;
    bool print_finished = false;
    uint32_t array_index;

    if (rpm_property == NULL) {
        fprintf(stream, "? \n");
        return;
    }
    value = rpm_property->value;
    if (value == NULL) {
        /* no value so print '?' */
        fprintf(stream, "? \n");
        return;
    }

    object_value.object_type = object_type;
    object_value.object_instance = object_instance;
    object_value.object_property = rpm_property->propertyIdentifier;
    object_value.array_index = rpm_property->propertyArrayIndex;
    object_value.value = value;

    if (property_list_bacnet_list_member(
            object_type, rpm_property->propertyIdentifier)) {
        /* property is a list - do not process or attempt to write */
        fprintf(stream, "?");
        if (rpm_property->value->tag == BACNET_APPLICATION_TAG_NULL) {
            /* empty list - changing the tag will attempt to write an empty list
             */
            rpm_property->value->tag = BACNET_APPLICATION_TAG_EMPTYLIST;
        }
        print_finished = true;
    }

    if (!print_finished) {
        switch (rpm_property->propertyIdentifier) {
            /* Don't attempt to decode and print the values for these properties
             */
            /* dynamic values, complex data types, or BTF expects something
             * specific */
            case PROP_PRIORITY_ARRAY:
            case PROP_DAYLIGHT_SAVINGS_STATUS:
            case PROP_LOCAL_TIME:
            case PROP_LOCAL_DATE:
            case PROP_RELIABILITY:
            case PROP_DATABASE_REVISION:
            case PROP_LAST_RESTORE_TIME:
            case PROP_CONFIGURATION_FILES:
            case PROP_EFFECTIVE_PERIOD:
            case PROP_WEEKLY_SCHEDULE:
            case PROP_RECORDS_SINCE_NOTIFICATION:
            case PROP_RECORD_COUNT:
            case PROP_TOTAL_RECORD_COUNT:
            case PROP_IPV6_DHCP_LEASE_TIME_REMAINING:
                fprintf(stream, "?");
                print_finished = true;
                break;

            case PROP_EXCEPTION_SCHEDULE:
                if (rpm_property->value->tag == BACNET_APPLICATION_TAG_NULL) {
                    /* empty list - changing the tag will attempt to write an
                     * empty list */
                    rpm_property->value->tag = BACNET_APPLICATION_TAG_EMPTYLIST;
                }
                fprintf(stream, "?");
                print_finished = true;
                break;

            case PROP_EVENT_TIME_STAMPS:
                /* BTF expects wierdness so give them it */
                fprintf(stream, "?");
                print_finished = true;
                break;

            case PROP_SETPOINT_REFERENCE:
                fprintf(stream, "?");
                print_finished = true;
                break;

            case PROP_OBJECT_PROPERTY_REFERENCE:
            case PROP_EVENT_ALGORITHM_INHIBIT_REF:
            case PROP_MANIPULATED_VARIABLE_REFERENCE:
            case PROP_CONTROLLED_VARIABLE_REFERENCE:
            case PROP_LOG_DEVICE_OBJECT_PROPERTY:
                /* BTF expects wierdness so give them it */
                fprintf(stream, "?");
                print_finished = true;
                break;

            case PROP_TIME_OF_DEVICE_RESTART:
                /* BTF expects wierdness so give them it */
                fprintf(stream, "?");
                print_finished = true;
                break;

            case PROP_FD_BBMD_ADDRESS:
                /* BTF expects wierdness so give them it */
                fprintf(stream, "?");
                print_finished = true;
                break;

            case PROP_CHANGE_OF_STATE_TIME:
            case PROP_TIME_OF_STATE_COUNT_RESET:
            case PROP_TIME_OF_ACTIVE_TIME_RESET:
            case PROP_MODIFICATION_DATE:
            case PROP_START_TIME:
            case PROP_STOP_TIME:
                /* BTF expects wierdness so give them it */
                fprintf(stream, "?");
                print_finished = true;
                break;

            case PROP_RESTART_NOTIFICATION_RECIPIENTS:
                /* BTF expects wierdness so give them it */
                fprintf(stream, "?");
                print_finished = true;
                break;

            case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
            case PROP_PROTOCOL_SERVICES_SUPPORTED:
                PrettyPrintPropertyValue(stream, &object_value);
                print_finished = true;
                break;

            case PROP_CURRENT_HEALTH:
                /* BTF expects wierdness so give them it */
                fprintf(stream, "?");
                print_finished = true;
                break;

            default:
                /* try to ready everything else */
                break;
        }
    }
    if (!print_finished) {
        if ((value != NULL) && (value->next != NULL)) {
            /* This is an array of values. */
            /* Normal array: open brace */
            fprintf(stream, "{ ");
            is_array = true; /* remember to close brace */
        }
        /* value(s) loop until there is no "next" ... */
        array_index = 0;
        while (value != NULL) {
            object_value.object_property = rpm_property->propertyIdentifier;
            object_value.array_index = rpm_property->propertyArrayIndex;
            object_value.value = value;
            if (property_list_bacnet_array_member(
                    object_type, rpm_property->propertyIdentifier)) {
                /* If the array is empty */
                if (value->tag == BACNET_APPLICATION_TAG_NULL) {
                    fprintf(stream, "?");
                    rpm_property->value->tag = BACNET_APPLICATION_TAG_EMPTYLIST;
                } else {
                    if (value->next == NULL) {
                        /* last entry in array */
                        print_value(stream, &object_value);
                        fprintf(stream, " }");
                    } else {
                        if (array_index == 0) {
                            /* first entry in array */
                            /* Normal array: open brace */
                            fprintf(stream, "\n        ");
                        } else {
                            /* entry in the middle of the array */
                        }
                        print_value(stream, &object_value);
                        fprintf(stream, ",\n        ");
                    }
                }
                array_index++;
            } else {
                print_value(stream, &object_value);
                if (value->next != NULL) {
                    /* there's more! */
                    fprintf(stdout, ",");
                } else {
                    if (is_array) {
                        /* Closing brace for this multi-valued array */
                        fprintf(stdout, " }");
                    }
                }
            }
            value = value->next; /* next or NULL */
        } /* End while loop */
    }
    if (Attempt_To_Write(
            object_value.object_type, object_value.object_property)) {
        /* attempt to write the received value back to the device */
        Response_Status = RESP_WAITING;
        Request_Invoke_ID = Send_Write_Property_Request(
            Target_Device_Object_Instance, object_value.object_type,
            object_value.object_instance, object_value.object_property,
            rpm_property->value, BACNET_NO_PRIORITY,
            rpm_property->propertyArrayIndex);
        wait_for_response();
        if (Response_Status == RESP_SUCCESS) {
            /* successfully wrote back what was read */
            fprintf(stdout, " W");
        }
    }
    fprintf(stdout, "\n");

    value = rpm_property->value;
    while (value != NULL) {
        old_value = value;
        value = value->next; /* next or NULL */
        free(old_value);
    } /* End while loop */
}

/** Print the property identifier name to stdout,
 *  handling the proprietary property numbers.
 * @param propertyIdentifier [in] The property identifier number.
 */
static void Print_Property_Identifier(FILE *stream, unsigned propertyIdentifier)
{
    if (bactext_property_name_proprietary(propertyIdentifier)) {
        fprintf(stream, "-- proprietary-%u", propertyIdentifier);
    } else {
        fprintf(stream, "%s", bactext_property_name(propertyIdentifier));
    }
}

static void print_usage(const char *filename)
{
    printf(
        "Usage: %s [-v] [-d] [-p sport] [-t target_mac [-n dnet]]"
        " device-instance\n",
        filename);
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    (void)filename;
    printf("Generates Full EPICS file, including Object and Property List\n");
    printf("device-instance:\n"
           "BACnet Device Object Instance number that you are\n"
           "trying to communicate to.  This number will be used\n"
           "to try and bind with the device using Who-Is and\n"
           "I-Am services.\n");
    printf("\n");
    printf("-v: show values instead of '?' \n");
    printf("-c: columns break for BACnetARRAY. Default is 0=always\n");
    printf("-d: show only device object properties\n");
    printf("-p: Use sport for \"my\" port.  0xBAC0 is default.\n");
    printf("    Allows you to communicate with a localhost target.\n");
    printf("-t: declare target's MAC instead of using Who-Is to bind to \n");
    printf("    device-instance. Format is \"C0:A8:00:18:BA:C0\"\n");
    printf("    Use \"7F:00:00:01:BA:C0\" for loopback testing \n");
    printf("-n: specify target's DNET if not local BACnet network  \n");
    printf("    or on routed Virtual Network \n");
    printf("\n");
    printf("To generate output directly to a .tpi file for VTS or BTF:\n");
    printf("$ bacepics 4194302 > epics-4194302.tpi \n");
}

static int CheckCommandLineArgs(int argc, char *argv[])
{
    int i;
    bool bFoundTarget = false;
    int argi = 0;
    const char *filename = NULL;

    filename = filename_remove_path(argv[0]);
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--help") == 0) {
            print_usage(filename);
            print_help(filename);
            exit(0);
        }
        if (strcmp(argv[argi], "--version") == 0) {
            printf("%s %s\n", filename, BACNET_VERSION_TEXT);
            printf("Copyright (C) 2014 by Steve Karg and others.\n"
                   "This is free software; see the source for copying "
                   "conditions.\n"
                   "There is NO warranty; not even for MERCHANTABILITY or\n"
                   "FITNESS FOR A PARTICULAR PURPOSE.\n");
            exit(0);
        }
    }
    if (argc < 2) {
        print_usage(filename);
        exit(0);
    }
    for (i = 1; i < argc; i++) {
        char *anArg = argv[i];
        if (anArg[0] == '-') {
            switch (anArg[1]) {
                case 'p':
                    if (++i < argc) {
#if defined(BACDL_BIP)
                        My_BIP_Port = (uint16_t)strtol(argv[i], NULL, 0);
                        /* Used strtol so sport can be either 0xBAC0 or 47808 */
#endif
                    }
                    break;
                default:
                    print_usage(filename);
                    exit(0);
                    break;
            }
        } else {
            /* decode the Target Device Instance parameter */
            Target_Device_Object_Instance = strtol(anArg, NULL, 0);
            if (Target_Device_Object_Instance > BACNET_MAX_INSTANCE) {
                fprintf(
                    stdout, "Error: device-instance=%u - not greater than %u\n",
                    Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
                print_usage(filename);
                exit(0);
            }
            bFoundTarget = true;
        }
    }
    if (!bFoundTarget) {
        fprintf(stdout, "Error: Must provide a device-instance \n\n");
        print_usage(filename);
        exit(0);
    }

    return 0; /* All OK if we reach here */
}

static RESPONSE_STATUS get_primitive_value(
    uint32_t device_instance,
    BACNET_OBJECT_ID object,
    BACNET_PROPERTY_ID property,
    uint32_t array_index,
    BACNET_APPLICATION_DATA_VALUE *value_ptr)
{
    uint8_t i;

    for (i = 0; i < apdu_retries(); i++) {
        Request_Invoke_ID = Send_Read_Property_Request(
            device_instance, object.type, object.instance, property,
            array_index);
        wait_for_response();
        if (Response_Status == RESP_SUCCESS) {
            *value_ptr =
                *Read_Property_Multiple_Data.rpm_data->listOfProperties->value;
            return Response_Status;
        }
    }
    /* failed to get a decodable response */
    return RESP_FAILED;
}

static void get_print_value(
    FILE *stream,
    uint32_t device_instance,
    BACNET_OBJECT_ID object,
    BACNET_PROPERTY_ID property,
    uint32_t array_index)
{
    BACNET_READ_ACCESS_DATA *rpm_data;
    uint8_t i;

    /* Print property name */
    fprintf(stream, "    ");
    if (bactext_property_name_proprietary(property)) {
        fprintf(stream, "-- proprietary-%u", property);
    } else {
        if (object.type >= OBJECT_PROPRIETARY_MIN &&
            object.type <= OBJECT_PROPRIETARY_MAX) {
            /* propriatary object */
            if (property != PROP_OBJECT_IDENTIFIER &&
                property != PROP_OBJECT_TYPE && property != PROP_OBJECT_NAME) {
                /* standard property, other than above, in a proprietary object
                 * - BTF wants them remmed out */
                fprintf(stream, "-- ");
            }
        }
        fprintf(stream, "%s", bactext_property_name(property));
    }

    fprintf(stream, ": ");

    if ((property >= PROP_PROPRIETARY_RANGE_MIN) &&
        (property <= PROP_PROPRIETARY_RANGE_MAX)) {
        /* proprietary property - print ? */
        fprintf(stream, "? \n");
        return;
    }

    /* get and print non-proprietary properties */
    /* read property value */
    for (i = 0; i < apdu_retries(); i++) {
        Request_Invoke_ID = Send_Read_Property_Request(
            device_instance, object.type, object.instance, property,
            array_index);
        wait_for_response();
        switch (Response_Status) {
            case RESP_SUCCESS:
                rpm_data = Read_Property_Multiple_Data.rpm_data;
                /* Print value or ? */
                PrintReadPropertyData(
                    stream, rpm_data->object_type, rpm_data->object_instance,
                    rpm_data->listOfProperties);
                /* valid response received - done */
                return;

            case RESP_ABORT_CODE:
            case RESP_REJECT_CODE:
            case RESP_ERROR_CODE:
                /* received a valid error, abort, or reject response - don't
                 * retry */
                fprintf(stream, "? \n");
                return;

            case RESP_FAILED_TO_DECODE:
                /* received a response this tool could not decode - add '?' and
                 * move on */
                fprintf(stream, "? \n");
                return;

            case RESP_TIMEOUT:
                /* timeout - try request again */
                break;

            case RESP_TSM_FAILED:
                /* TSM failed - try request again */
                break;

            default:
            case RESP_WAITING:
            case RESP_FAILED:
                /* BAD! wait_for_response() sets Response_Status to anything
                 * else! - try again */
                break;
        }
    }
    /* read failed for some reason */
    fprintf(stream, "? -- ERROR - IUT Failed to respond to request! \n");
}

static uint32_t print_header(FILE *stream, uint32_t device_instance)
{
    BACNET_OBJECT_PROPERTY_VALUE property_value;
    BACNET_APPLICATION_DATA_VALUE data_value;
    BACNET_OBJECT_ID device_object;
    uint32_t error = 0;
    RESPONSE_STATUS status;

    device_object.type = OBJECT_DEVICE;
    device_object.instance = device_instance;

    fprintf(stream, "PICS 0\n");
    printf("BACnet Protocol Implementation Conformance Statement\n");

    printf("--\n--\n");
    printf("-- Generated by BACnet Protocol Stack library EPICS tool\n");
    printf("-- BACnet/IP Interface for BACnet-stack Devices\n");
    printf("-- http://sourceforge.net/projects/bacnet/ \n");
    printf("-- Version %s\n", BACNET_VERSION_TEXT);
    printf("-- \n--\n\n");

    status = get_primitive_value(
        device_instance, device_object, PROP_VENDOR_NAME, BACNET_ARRAY_ALL,
        &data_value);

    if ((status == RESP_SUCCESS) && (data_value.type.Character_String.length)) {
        printf(
            "Vendor Name: \"%s\"\n",
            (char *)&data_value.type.Character_String.value);
    } else {
        printf("-- ERROR - failed to read VENDOR_NAME\n");
        printf("Vendor Name: \"your vendor name here\"\n");
        error++;
    }

    status = get_primitive_value(
        device_instance, device_object, PROP_MODEL_NAME, BACNET_ARRAY_ALL,
        &data_value);

    if ((status == RESP_SUCCESS) && (data_value.type.Character_String.length)) {
        printf(
            "Product Name: \"%s\"\n",
            (char *)&data_value.type.Character_String.value);
        printf(
            "Product Model Number: \"%s\"\n",
            (char *)&data_value.type.Character_String.value);
    } else {
        printf("-- ERROR - failed to read MODEL_NAME\n");
        printf("Product Name: \"your product name here\"\n");
        printf("Product Model Number: \"your model number here\"\n");
        error++;
    }

    status = get_primitive_value(
        device_instance, device_object, PROP_DESCRIPTION, BACNET_ARRAY_ALL,
        &data_value);
    if (status == RESP_SUCCESS) {
        printf(
            "Product Description: \"%s\"\n\n",
            (char *)&data_value.type.Character_String.value);
    } else {
        printf("Product Description: "
               "\"your product description here\"\n\n");
    }

    printf("--Use '--' to indicate unsupported Functionality.\n\n");

    printf("BIBBs Supported:\n");
    printf("{\n");

    printf("-- K.1 Data Sharing\n");
    printf(" DS-RP-B\n");
    printf("-- DS-RP-A\n");
    printf("-- DS-RPM-A\n");
    printf("-- DS-RPM-B\n");
    printf("-- DS-WP-A\n");
    printf("-- DS-WP-B\n");
    printf("-- DS-WPM-A\n");
    printf("-- DS-WPM-B\n");
    printf("-- DS-COV-A\n");
    printf("-- DS-COV-B\n");
    printf("-- DS-COVP-A\n");
    printf("-- DS-COVP-B\n");
    printf("-- DS-COVU-A\n");
    printf("-- DS-COVU-B\n");
    printf("-- DS-COVM-A\n");
    printf("-- DS-COVM-B\n");
    printf("-- DS-V-A\n");
    printf("-- DS-AV-A\n");
    printf("-- DS-M-A\n");
    printf("-- DS-AM-A\n");
    printf("-- DS-WG-A\n");
    printf("-- DS-WG-I-B\n");
    printf("-- DS-WG-E-B\n");
    printf("-- DS-VSI-B\n");
    printf("-- DS-LSV-A\n");
    printf("-- DS-LSAV-A\n");
    printf("-- DS-LSM-A\n");
    printf("-- DS-LSAM-A\n");
    printf("-- DS-ACV-A\n");
    printf("-- DS-ACAV-A\n");
    printf("-- DS-ACM-A\n");
    printf("-- DS-ACAM-A\n");
    printf("-- DS-ACUC-A\n");
    printf("-- DS-ACUC-B\n");
    printf("-- DS-ACSC-A\n");
    printf("-- DS-ACSC-B\n");
    printf("-- DS-ACAD-A\n");
    printf("-- DS-ACAD-B\n");
    printf("-- DS-ACCDI-A\n");
    printf("-- DS-ACCDI-B\n");
    printf("-- DS-LO-A\n");
    printf("-- DS-LOS-A\n");
    printf("-- DS-ALO-A\n");
    printf("-- DS-LO-B\n");
    printf("-- DS-BLO-B\n");
    printf("-- DS-LV-A\n");
    printf("-- DS-LAV-A\n");
    printf("-- DS-LM-A\n");
    printf("-- DS-LAM-A\n");
    printf("-- DS-EV-A\n");
    printf("-- DS-EAV-A\n");
    printf("-- DS-EM-A\n");
    printf("-- DS-EAM-A\n");

    printf("\n-- K.2 Alarm and Event\n");
    printf("-- AE-N-A\n");
    printf("-- AE-N-I-B\n");
    printf("-- AE-N-E-B\n");
    printf("-- AE-ACK-A\n");
    printf("-- AE-ACK-B\n");
    printf("-- AE-ASUM-A         -- deprecated BIBB\n");
    printf("-- AE-ASUM-B         -- deprecated BIBB\n");
    printf("-- AE-ESUM-A         -- deprecated BIBB\n");
    printf("-- AE-ESUM-B         -- deprecated BIBB\n");
    printf("-- AE-INFO-A         -- deprecated BIBB\n");
    printf("-- AE-INFO-B\n");
    printf("-- AE-LS-A\n");
    printf("-- AE-LS-B\n");
    printf("-- AE-VN-A\n");
    printf("-- AE-AVN-A\n");
    printf("-- AE-VM-A\n");
    printf("-- AE-AVM-A\n");
    printf("-- AE-AS-A\n");
    printf("-- AE-ELV-A\n");
    printf("-- AE-ELVM-A\n");
    printf("-- AE-EL-I-B\n");
    printf("-- AE-EL-E-B\n");
    printf("-- AE-NF-B\n");
    printf("-- AE-NF-I-B\n");
    printf("-- AE-CRL-B\n");
    printf("-- AE-TES-A\n");
    printf("-- AE-LSVN-A\n");
    printf("-- AE-LSAVN-A\n");
    printf("-- AE-LSVM-A\n");
    printf("-- AE-LSAVM-A\n");
    printf("-- AE-AC-A\n");
    printf("-- AE-AC-B\n");
    printf("-- AE-ACAVN-A\n");
    printf("-- AE-ACVM-A\n");
    printf("-- AE-ACAVM-A\n");
    printf("-- AE-EVN-A\n");
    printf("-- AE-EAVN-A\n");
    printf("-- AE-EVM-A\n");
    printf("-- AE-EAVM-A\n");

    printf("\n-- K.3 Scheduling\n");
    printf("-- SCHED-A            -- deprecated BIBB\n");
    printf("-- SCHED-I-B\n");
    printf("-- SCHED-E-B\n");
    printf("-- SCHED-R-B\n");
    printf("-- SCHED-AVM-A\n");
    printf("-- SCHED-VM-A\n");
    printf("-- SCHED-WS-A\n");
    printf("-- SCHED-WS-I-B\n");
    printf("-- SCHED-TMR-I-B\n");
    printf("-- SCHED-TMR-E-B\n");

    printf("\n-- K.4 Trending\n");
    printf("-- T-VMT-A            -- deprecated BIBB\n");
    printf("-- T-VMT-I-B\n");
    printf("-- T-VMT-E-B\n");
    printf("-- T-ATR-A\n");
    printf("-- T-ATR-B\n");
    printf("-- T-VMMV-A          -- deprecated BIBB\n");
    printf("-- T-VMMV-I-B\n");
    printf("-- T-VMMV-E-B\n");
    printf("-- T-AMVR-A\n");
    printf("-- T-AMVR-B\n");
    printf("-- T-V-A\n");
    printf("-- T-AVM-A\n");
    printf("-- T-A-A\n");

    printf("\n-- K.5 Device Management\n");
    printf("-- DM-DDB-A\n");
    printf("-- DM-DDB-B\n");
    printf("-- DM-DOB-A\n");
    printf("-- DM-DOB-B\n");
    printf("-- DM-DCC-A\n");
    printf("-- DM-DCC-B\n");
    printf("-- DM-TM-A\n");
    printf("-- DM-TM-B\n");
    printf("-- DM-TS-A\n");
    printf("-- DM-TS-B\n");
    printf("-- DM-UTC-A\n");
    printf("-- DM-UTC-B\n");
    printf("-- DM-RD-A\n");
    printf("-- DM-RD-B\n");
    printf("-- DM-BR-A\n");
    printf("-- DM-BR-B\n");
    printf("-- DM-R-A\n");
    printf("-- DM-R-B\n");
    printf("-- DM-LM-A\n");
    printf("-- DM-LM-B\n");
    printf("-- DM-OCD-A\n");
    printf("-- DM-OCD-B\n");
    printf("-- DM-VT-A\n");
    printf("-- DM-VT-B\n");
    printf("-- DM-ANM-A\n");
    printf("-- DM-ADM-A\n");
    printf("-- DM-ATS-A\n");
    printf("-- DM-MTS-A\n");
    printf("-- DM-SP-VM-A\n");
    printf("-- DM-SP-B\n");
    printf("-- DM-LOM-A\n");
    printf("-- DM-DDA-A\n");
    printf("-- DM-DDA-B\n");
    printf("-- DM-DAP-VM-A\n");
    printf("-- DM-DAP-B\n");
    printf("-- DM-TSDI-A\n");
    printf("-- DM-TSDE-A\n");

    printf("\n-- K.6 Network Management\n");
    printf("-- NM-CE-A\n");
    printf("-- NM-CE-B\n");
    printf("-- NM-RC-A\n");
    printf("-- NM-RC-B\n");
    printf("-- NM-BBMDC-A\n");
    printf("-- NM-BBMDC-B\n");
    printf("-- NM-FDR-A\n");
    printf("-- NM-SCH-B\n");
    printf("-- NM-SCDC-A\n");
    printf("-- NM-SCDC-B\n");
    printf("-- NM-CC-A\n");
    printf("-- NM-SCCM-A\n");

    printf("\n-- K.7 Gateway\n");
    printf("-- GW-VN-B\n");
    printf("-- GW-EO-B\n");

    printf("\n-- K.8 Audit Reporting\n");
    printf("-- AR-L-A\n");
    printf("-- AR-R-B\n");
    printf("-- AR-R-S-B\n");
    printf("-- AR-F-B\n");
    printf("-- AR-V-A\n");
    printf("-- AR-AVM-A\n");

    printf("\n-- k.9 Authentication and Authorization\n");
    printf("-- AA-DAC-A\n");
    printf("-- AA-SAC-A\n");
    printf("-- AA-AT-B\n");
    printf("-- AA-NAT-B\n");
    printf("-- AA-AS-B\n");
    printf("}\n\n");

    printf("BACnet Standard Application Services Supported:\n");
    printf("{\n");

    /* We have to process this bit string and determine which Object Types we
     * have, and show them
     */
    status = get_primitive_value(
        device_instance, device_object, PROP_PROTOCOL_SERVICES_SUPPORTED,
        BACNET_ARRAY_ALL, &data_value);
    if (status == RESP_SUCCESS) {
        int i, len = bitstring_bits_used(&data_value.type.Bit_String);
        printf("-- services reported by this device\n");
        for (i = 0; i < len; i++) {
            if (bitstring_bit(&data_value.type.Bit_String, (uint8_t)i)) {
                printf(
                    " %s\t\tInitiate Execute\n",
                    protocol_services_supported_text(i));
            } else {
                printf(
                    "-- %s\t\tInitiate Execute\n",
                    protocol_services_supported_text(i));
            }
        }
    } else {
        printf("-- ERROR - failed to read PROTOCOL_SERVICES_SUPPORTED\n");
        error++;
    }
    printf("}\n\n");

    printf("Standard Object Types Supported:\n");
    printf("{\n");

    /* We have to process this bit string and determine which Object Types we
     * have, and show them
     */
    status = get_primitive_value(
        device_instance, device_object, PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED,
        BACNET_ARRAY_ALL, &data_value);
    if (status == RESP_SUCCESS) {
        int i, len = bitstring_bits_used(&data_value.type.Bit_String);
        printf("-- objects reported by this device\n");
        for (i = 0; i < len; i++) {
            if (bitstring_bit(&data_value.type.Bit_String, (uint8_t)i)) {
                printf(
                    " %s\t\tCreateable Deleteable\n",
                    bactext_object_type_name_capitalized(i));
            } else {
                printf(
                    "-- %s\t\tCreateable Deleteable\n",
                    bactext_object_type_name_capitalized(i));
            }
        }
    } else {
        printf("-- ERROR - failed to read PROTOCOL_OBJECT_TYPES_SUPPORTED\n");
        error++;
    }
    printf("}\n\n");

    printf("Data Link Layer Option:\n");
    printf("{\n");
    printf("-- choose the data link options supported\n");
    printf("-- ISO 8802-3, 10BASE5\n");
    printf("-- ISO 8802-3, 10BASE2\n");
    printf("-- ISO 8802-3, 10BASET\n");
    printf("-- ISO 8802-3, fiber\n");
    printf("-- ARCNET, coax star\n");
    printf("-- ARCNET, coax bus\n");
    printf("-- ARCNET, twisted pair star \n");
    printf("-- ARCNET, twisted pair bus\n");
    printf("-- ARCNET, fiber star\n");
    printf("-- ARCNET, twisted pair, EIA-485, Baud rate(s): 156000\n");
    printf("-- MS/TP manager. Baud rate(s): 9600, 38400\n");
    printf("-- MS/TP subordinate. Baud rate(s): 9600, 38400\n");
    printf("-- Point-To-Point. EIA 232, Baud rate(s): 9600\n");
    printf("-- Point-To-Point. Modem, Baud rate(s): 9600\n");
    printf("-- Point-To-Point. Modem, Baud rate(s): 9600 to 115200\n");
    printf("-- BACnet/IP, 'DIX' Ethernet\n");
    printf("-- BACnet/IP, Other\n");
    printf("-- BACnet/IPv6, 'DIX' Ethernet\n");
    printf("-- BACnet/SC\n");
    printf("-- Zigbee\n");
    printf("-- Other\n");
    printf("}\n\n");

    printf("Character Sets Supported:\n");
    printf("{\n");
    printf("-- choose any character sets supported\n");
    printf("-- ANSI X3.4\n");
    printf("-- IBM/Microsoft DBCS\n");
    printf("-- JIS C 6226\n");
    printf("-- ISO 8859-1\n");
    printf("-- ISO 10646 (UCS-4)\n");
    printf("-- ISO 10646 (UCS2)\n");
    printf("-- ISO 10646 (UTF-8)\n");
    printf("}\n\n");

    printf("Special Functionality:\n");
    printf("{\n");

    printf(" Maximum APDU size in octets: ");
    status = get_primitive_value(
        device_instance, device_object, PROP_MAX_APDU_LENGTH_ACCEPTED,
        BACNET_ARRAY_ALL, &data_value);
    if (status == RESP_SUCCESS) {
        property_value.object_type = OBJECT_DEVICE;
        property_value.object_instance = 0;
        property_value.object_property = PROP_MAX_APDU_LENGTH_ACCEPTED;
        property_value.array_index = BACNET_ARRAY_ALL;
        property_value.value = &data_value;
        bacapp_print_value(stdout, &property_value);
        printf("\n");
    } else {
        printf("? -- ERROR - failed to read MAX_APDU_LENGTH_ACCEPTED");
        error++;
    }
    printf("-- Segmented Requests Supported, window size: ?\n");
    printf("-- Segmented Responses Supported, window size: ?\n");
    printf("-- Router\n");
    printf("-- BACnet/IP BBMD\n");
    printf("-- BACnet/IPV6 BBMD\n");
    printf("-- BACnet/SC Hub\n");
    printf("-- BACnet/SC Direct Connect\n");
    printf("}\n\n");

    printf("Default Property Value Restrictions:\n");
    printf("{\n");
    printf("  unsigned-integer: <minimum: 0; maximum: 4294967295>\n");
    printf("  signed-integer: <minimum: -2147483647; maximum: 2147483647>\n");
    printf(
        "  real: <minimum: -3.40282347E38; maximum: 3.40282347E38; resolution: "
        "1.0>\n");
    printf("  double: <minimum: 2.2250738585072016E-38; maximum: "
           "1.7976931348623157E38; resolution: 0.0001>\n");
    printf("  date: <minimum: 01-January-1970; maximum: 31-December-2038>\n");
    printf("  octet-string: <maximum length string: 122>\n");
    printf("  character-string: <maximum length string: 122>\n");
    printf("  list: <maximum length list: 10>\n");
    printf("  variable-length-array: <maximum length array: 10>\n");
    printf("}\n\n");

    printf("Fail Times:\n");
    printf("{\n");
    printf("  Notification Fail Time: 2\n");
    printf("  Internal Processing Fail Time: 0.5\n");
    printf("  Minimum ON/OFF Time: 5\n");
    printf("  Schedule Evaluation Fail Time: 1\n");
    printf("  External Command Fail Time: 1\n");
    printf("  Program Object State Change Fail Time: 2\n");
    printf("  Acknowledgement Fail Time: 2\n");
    printf("  Unconfirmed Response Fail Time: 1\n");
    printf("  Activate Changes Fail Time: 1\n");
    printf("  Auto Negotiation Fail Time: 1\n");
    printf("  Foreign Device Registration Fail Time: 1\n");
    printf("  Channel Write Fail Time: 1\n");
    printf("  Subordinate Proxy Confirm Interval: 1\n");
    printf("}\n\n");
    return (error);
}

static void get_print_object_list(
    FILE *stream, BACNET_OBJECT_ID object, uint32_t num_objects)
{
    BACNET_APPLICATION_DATA_VALUE data_value;
    uint32_t i;
    RESPONSE_STATUS status;

    /* Print property ID */
    fprintf(stream, "    ");
    Print_Property_Identifier(stream, PROP_OBJECT_LIST);
    fprintf(stream, ": {\n");

    for (i = 1; i <= num_objects; i++) {
        status = get_primitive_value(
            object.instance, object, PROP_OBJECT_LIST, i, &data_value);
        if (status == RESP_SUCCESS) {
            /* got an object id */
            if (data_value.type.Object_Id.type <
                BACNET_OBJECT_TYPE_RESERVED_MIN) {
                /* Print object type and instance for known object types */
                fprintf(
                    stream, "        (%s, %i)",
                    bactext_object_type_name(data_value.type.Object_Id.type),
                    data_value.type.Object_Id.instance);
            } else {
                /* print object type number and instance for unknown object
                 * types */
                fprintf(
                    stream, "        (%i, %i)", data_value.type.Object_Id.type,
                    data_value.type.Object_Id.instance);
            }
        } else {
            /* failed to read the property identifier - this entry will be
             * ignored */
            printf("-- ERROR - failed to read OBJECT_LIST entry = %i\n", i);
        }
        if (i == num_objects) {
            fprintf(stream, "\n");
        } else {
            fprintf(stream, ",\n");
        }
    }
    fprintf(stream, "    }\n");
}

static void print_property_list(
    FILE *stream,
    PROPERTY_LIST *prop_list,
    uint32_t num_properties,
    BACNET_OBJECT_TYPE type)
{
    uint32_t i;

    fprintf(stream, "    ");
    if (type >= OBJECT_PROPRIETARY_MIN && type <= OBJECT_PROPRIETARY_MAX) {
        /* propriatary object */
        /* standard property, other than above, in a proprietary object - BTF
         * wants them remmed out */
        fprintf(stream, "-- ");
    }
    Print_Property_Identifier(stream, PROP_PROPERTY_LIST);
    fprintf(stream, ": (");
    for (i = 0; i < num_properties; i++) {
        if (i == num_properties - 1) {
            fprintf(stream, "%i)\n", prop_list[i].property);
        } else {
            fprintf(stream, "%i,", prop_list[i].property);
        }
    }
}

static uint32_t get_build_EPICS_objects(FILE *stream, uint32_t device_instance)
{
    BACNET_OBJECT_ID device_object, object;
    uint32_t num_objects, num_properties;
    uint32_t i, j, k;
    BACNET_APPLICATION_DATA_VALUE data_value;
    PROPERTY_LIST prop_list[256];
    struct special_property_list_t special_property_list;
    uint32_t error = 0;
    RESPONSE_STATUS status;

    device_object.type = OBJECT_DEVICE;
    device_object.instance = device_instance;

    /* get number of objects */
    status = get_primitive_value(
        device_instance, device_object, PROP_OBJECT_LIST, 0, &data_value);
    if (status == RESP_SUCCESS) {
        /* got number of objects */
        num_objects = data_value.type.Unsigned_Int;
        printf("List of Objects in Test Device:\n");
        /* Print Opening brace, then kick off with the device Object */
        printf("{\n");

        /* get and print device object */
        object = device_object;

        status = get_primitive_value(
            object.instance, object, PROP_PROPERTY_LIST, 0, &data_value);
        if (status == RESP_SUCCESS) {
            /* got number of properties */
            printf("  {\n"); /* And opening brace for the first object */
            /* Since object-id, object-type, object-name are not
             * part of the property-list, print manually.*/
            get_print_value(
                stream, device_instance, object, PROP_OBJECT_IDENTIFIER,
                BACNET_ARRAY_ALL);
            get_print_value(
                stream, device_instance, object, PROP_OBJECT_NAME,
                BACNET_ARRAY_ALL);
            get_print_value(
                stream, device_instance, object, PROP_OBJECT_TYPE,
                BACNET_ARRAY_ALL);
            num_properties =
                min(data_value.type.Unsigned_Int, sizeof(prop_list));
            /* get and save list of property ids in this object in the IUT */
            for (j = 0; j < num_properties; j++) {
                status = get_primitive_value(
                    device_instance, object, PROP_PROPERTY_LIST, j + 1,
                    &data_value);
                if (status == RESP_SUCCESS) {
                    prop_list[j].property = data_value.type.Unsigned_Int;
                    prop_list[j].exported = 0;
                } else {
                    /* failed to read the property identifier - this
                     * entry will be ignored */
                    printf(
                        "-- ERROR - failed to read PROPERTY_LIST entry = %i\n",
                        j);
                    prop_list[j].property = MAX_BACNET_PROPERTY_ID;
                    prop_list[j].exported = 1;
                }
            }

            /* print out the required properties */
            property_list_special(object.type, &special_property_list);
            for (j = 0; j < special_property_list.Required.count; j++) {
                for (k = 0; k < num_properties; k++) {
                    if (special_property_list.Required.pList[j] ==
                        prop_list[k].property) {
                        if ((prop_list[k].property == PROP_PROPERTY_LIST) ||
                            (prop_list[k].property == PROP_OBJECT_LIST)) {
                            /* property and object lists are read later one
                             * element at a time */
                            prop_list[k].exported = 1;
                        } else {
                            /* read and print required property */
                            get_print_value(
                                stream, device_instance, object,
                                prop_list[k].property, BACNET_ARRAY_ALL);
                            prop_list[k].exported = 1;
                        }
                    }
                }
            }

            /* print object list and property list */
            get_print_object_list(stream, object, num_objects);
            print_property_list(
                stream, &prop_list[0], num_properties, object.type);

            /* print out the optional properties */
            for (j = 0; j < special_property_list.Optional.count; j++) {
                for (k = 0; k < num_properties; k++) {
                    if (special_property_list.Optional.pList[j] ==
                        prop_list[k].property) {
                        /* read and print optional property */
                        get_print_value(
                            stream, device_instance, object,
                            prop_list[k].property, BACNET_ARRAY_ALL);
                        prop_list[k].exported = 1;
                    }
                }
            }

            /* print out the other properties */
            for (j = 0; j < num_properties; j++) {
                if (prop_list[j].exported == 0) {
                    get_print_value(
                        stream, device_instance, object, prop_list[j].property,
                        BACNET_ARRAY_ALL);
                    prop_list[j].exported = 1;
                }
            }
            printf("  },\n"); /* And opening brace for the first object */
        } else {
            /* failed to get number of properties - move to next object */
            printf("-- ERROR - failed to read PROPERTY_LIST size for object\n");
            error++;
        }
        /* now get and print the rest of the objects */
        for (i = 1; i <= num_objects; i++) {
            /* get object ids from device object object_list */
            fprintf(stderr, "\rReading object %i of %i", i, num_objects);
            status = get_primitive_value(
                device_instance, device_object, PROP_OBJECT_LIST, i,
                &data_value);
            if (status == RESP_SUCCESS) {
                /* have an object id */
                object.type = data_value.type.Object_Id.type;
                object.instance = data_value.type.Object_Id.instance;
                if (object.type != OBJECT_DEVICE) {
                    /* get number of properties in object */
                    status = get_primitive_value(
                        device_instance, object, PROP_PROPERTY_LIST, 0,
                        &data_value);
                    if (status == RESP_SUCCESS) {
                        /* got number of properties */
                        printf("  {\n"); /* And opening brace for the first
                                            object */
                        /* Since object-id, object-type, object-name are not
                         * part of the property-list, print manually.*/
                        get_print_value(
                            stream, device_instance, object,
                            PROP_OBJECT_IDENTIFIER, BACNET_ARRAY_ALL);
                        get_print_value(
                            stream, device_instance, object, PROP_OBJECT_NAME,
                            BACNET_ARRAY_ALL);
                        get_print_value(
                            stream, device_instance, object, PROP_OBJECT_TYPE,
                            BACNET_ARRAY_ALL);
                        num_properties = min(
                            data_value.type.Unsigned_Int, sizeof(prop_list));
                        /* get and save list of property ids in this object in
                         * the IUT */
                        for (j = 0; j < num_properties; j++) {
                            status = get_primitive_value(
                                device_instance, object, PROP_PROPERTY_LIST,
                                j + 1, &data_value);
                            if (status == RESP_SUCCESS) {
                                prop_list[j].property =
                                    data_value.type.Unsigned_Int;
                                prop_list[j].exported = 0;
                            } else {
                                /* failed to read the property identifier - this
                                 * entry will be ignored */
                                printf(
                                    "-- ERROR - failed to read PROPERTY_LIST "
                                    "entry = %i\n",
                                    j);
                                prop_list[j].property = MAX_BACNET_PROPERTY_ID;
                                prop_list[j].exported = 1;
                            }
                        }
                        /* print out the required properties */
                        property_list_special(
                            object.type, &special_property_list);
                        for (j = 0; j < special_property_list.Required.count;
                             j++) {
                            for (k = 0; k < num_properties; k++) {
                                if (special_property_list.Required.pList[j] ==
                                    prop_list[k].property) {
                                    if (prop_list[k].property ==
                                        PROP_PROPERTY_LIST) {
                                        /* property list is read later one
                                         * element at a time */
                                        prop_list[k].exported = 1;
                                    } else {
                                        /* read and print required property */
                                        get_print_value(
                                            stream, device_instance, object,
                                            prop_list[k].property,
                                            BACNET_ARRAY_ALL);
                                        prop_list[k].exported = 1;
                                    }
                                }
                            }
                        }
                        /* print out the property list */
                        print_property_list(
                            stream, &prop_list[0], num_properties, object.type);
                        /* print out the optional properties */
                        for (j = 0; j < special_property_list.Optional.count;
                             j++) {
                            for (k = 0; k < num_properties; k++) {
                                if (special_property_list.Optional.pList[j] ==
                                    prop_list[k].property) {
                                    /* read and print optional property */
                                    get_print_value(
                                        stream, device_instance, object,
                                        prop_list[k].property,
                                        BACNET_ARRAY_ALL);
                                    prop_list[k].exported = 1;
                                }
                            }
                        }
                        /* print out the other properties */
                        for (j = 0; j < num_properties; j++) {
                            if (prop_list[j].exported == 0) {
                                get_print_value(
                                    stream, device_instance, object,
                                    prop_list[j].property, BACNET_ARRAY_ALL);
                                prop_list[j].exported = 1;
                            }
                        }
                        printf("  },\n"); /* And opening brace for the first
                                             object */
                    } else {
                        /* failed to get number of properties - move to next
                         * object */
                        printf(
                            "-- ERROR - failed to read PROPERTY_LIST size for "
                            "object\n");
                        error++;
                    }
                }
            } else {
                /* failed to get an object id from object list - move to next
                 * object */
                printf("-- ERROR - failed to read OBJECT_LIST entry = %i\n", i);
                error++;
            }
        }
    } else {
        /* failed to get size of object list */
        printf("-- ERROR - failed to read size of OBJECT_LIST\n");
        error++;
        return error;
    }
    printf("} \n");
    printf("End of BACnet Protocol Implementation Conformance Statement\n\n");
    return error;
}

/** Main function of the bacepics program.
 *
 * @see Device_Set_Object_Instance_Number, Keylist_Create, address_init,
 *      dlenv_init, address_bind_request, Send_WhoIs,
 *      tsm_timer_milliseconds, datalink_receive, npdu_handler,
 *      Send_Read_Property_Multiple_Request,
 *
 *
 * @param argc [in] Arg count.
 * @param argv [in] Takes one to four arguments, processed in
 * CheckCommandLineArgs().
 * @return 0 on success.
 */
int main(int argc, char *argv[])
{
    BACNET_ADDRESS src; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100; /* milliseconds */
    unsigned max_apdu = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    bool found = false;

    fprintf(stderr, "\n");
    fprintf(stderr, "EPICS Generator Version %s\n", "2.0.0");
    fprintf(
        stderr,
        "Copyright (C) 2014 by Steve Karg and others.\n"
        "This is free software; see the source for copying conditions.\n"
        "There is NO warranty; not even for MERCHANTABILITY or\n"
        "FITNESS FOR A PARTICULAR PURPOSE.\n");

    CheckCommandLineArgs(argc, argv); /* Won't return if there is an issue. */
    memset(&src, 0, sizeof(BACNET_ADDRESS));

    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
#if defined(BACDL_BIP)
    /* For BACnet/IP, we might have set a different port for "me", so
     * (eg) we could talk to a BACnet/IP device on our same interface.
     * My_BIP_Port will be non-zero in this case.
     */
    if (My_BIP_Port > 0) {
        bip_set_port(My_BIP_Port);
    }
#endif
    address_init();
    Init_Service_Handlers();
    dlenv_init();
#if (__STDC_VERSION__ >= 199901L) && defined(__STDC_ISO_10646__)
    /* Internationalized programs must call setlocale()
     * to initiate a specific language operation.
     * This can be done by calling setlocale() as follows.
     * If your native locale doesn't use UTF-8 encoding
     * you need to replace the empty string with a
     * locale like "en_US.utf8" which is the same as the string
     * used in the enviromental variable "LANG=en_US.UTF-8".
     */
    setlocale(LC_ALL, "");
#endif
    atexit(datalink_cleanup);

    /* configure the timeout values */
    current_seconds = time(NULL);
    timeout_seconds = (apdu_timeout() / 1000) * apdu_retries();
    mstimer_set(&APDU_Timer, (uint32_t)apdu_timeout());

#if defined(BACDL_BIP)
    if (My_BIP_Port > 0) {
        /* Set back to std BACnet/IP port */
        bip_set_port(0xBAC0);
    }
#endif
    Error_Count = 0;
    /* try to bind with the target device */
    found = address_bind_request(
        Target_Device_Object_Instance, &max_apdu, &Target_Address);
    if (!found) {
        if (Provided_Targ_MAC) {
            if (Target_Address.net > 0) {
                /* We specified a DNET; call Who-Is to find the full
                 * routed path to this Device */
                Send_WhoIs_Remote(
                    &Target_Address, Target_Device_Object_Instance,
                    Target_Device_Object_Instance);

            } else {
                /* Update by adding the MAC address */
                if (max_apdu == 0) {
                    max_apdu = MAX_APDU; /* Whatever set for this datalink. */
                }
                address_add_binding(
                    Target_Device_Object_Instance, max_apdu, &Target_Address);
            }
        } else {
            Send_WhoIs(
                Target_Device_Object_Instance, Target_Device_Object_Instance);
        }
    }
    myState = INITIAL_BINDING;
    do {
        /* increment timer - will exit if timed out */
        last_seconds = current_seconds;
        current_seconds = time(NULL);
        /* Has at least one second passed ? */
        if (current_seconds != last_seconds) {
            tsm_timer_milliseconds(
                (uint16_t)((current_seconds - last_seconds) * 1000));
            datalink_maintenance_timer(current_seconds - last_seconds);
        }
        /* OK to proceed; see what we are up to now */
        switch (myState) {
            case INITIAL_BINDING:
                /* returns 0 bytes on timeout */
                pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

                /* process; normally is some initial error */
                if (pdu_len) {
                    npdu_handler(&src, &Rx_Buf[0], pdu_len);
                }
                /* will wait until the device is bound, or timeout and quit */
                found = address_bind_request(
                    Target_Device_Object_Instance, &max_apdu, &Target_Address);
                if (!found) {
                    /* increment timer - exit if timed out */
                    elapsed_seconds += (current_seconds - last_seconds);
                    if (elapsed_seconds > timeout_seconds) {
                        fprintf(
                            stderr,
                            "\rError: Unable to bind to %u after waiting %ld "
                            "seconds.\n",
                            Target_Device_Object_Instance,
                            (long int)elapsed_seconds);
                        break;
                    }
                    /* else, loop back and try again */
                    continue;
                } else {
                    myState = BUILD_EPICS;
                }
                break;

            case BUILD_EPICS:
                Error_Count +=
                    print_header(stdout, Target_Device_Object_Instance);
                Error_Count += get_build_EPICS_objects(
                    stdout, Target_Device_Object_Instance);
                myState = EPICS_EXIT;
                break;

            default:
                assert(false); /* program error; fix this */
                break;
        }

        /* Check for timeouts */
        if (!found || (Request_Invoke_ID > 0)) {
            /* increment timer - exit if timed out */
            elapsed_seconds += (current_seconds - last_seconds);
            if (elapsed_seconds > timeout_seconds) {
                fprintf(
                    stderr, "\rError: APDU Timeout! (%lds)\n",
                    (long int)elapsed_seconds);
                break;
            }
        }

    } while (myState != EPICS_EXIT);

    if (Error_Count > 0) {
        fprintf(stdout, "\r-- Found %d Errors \n", Error_Count);
    }
    return 0;
}

/*@} */

/* End group BACEPICS */
