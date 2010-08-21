/*************************************************************************
* Copyright (C) 2008 Steve Karg <skarg@users.sourceforge.net>
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
#include <errno.h>
#include <time.h>       /* for time */

#define PRINT_ENABLED 1

#include "bacdef.h"
#include "config.h"
#include "bactext.h"
#include "bacerror.h"
#include "iam.h"
#include "arf.h"
#include "tsm.h"
#include "address.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "net.h"
#include "datalink.h"
#include "whois.h"
#include "bacnet-session.h"
#include "handlers-data.h"
#include "session.h"
/* some demo stuff needed */
#include "rpm.h"
#include "filename.h"
#include "handlers.h"
#include "client.h"
#include "txbuf.h"
#include "dlenv.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_READ_ACCESS_DATA *Read_Access_Data;
/* needed to filter incoming messages */
static uint8_t Request_Invoke_ID = 0;
static BACNET_ADDRESS Target_Address;
/* needed for return value of main application */
static bool Error_Detected = false;

static void MyErrorHandler(
    struct bacnet_session_object *sess,
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("BACnet Error: %s: %s\r\n",
            bactext_error_class_name((int) error_class),
            bactext_error_code_name((int) error_code));
        Error_Detected = true;
    }
}

void MyAbortHandler(
    struct bacnet_session_object *sess,
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t abort_reason,
    bool server)
{
    (void) server;
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("BACnet Abort: %s\r\n",
            bactext_abort_reason_name((int) abort_reason));
        Error_Detected = true;
    }
}

void MyRejectHandler(
    struct bacnet_session_object *sess,
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("BACnet Reject: %s\r\n",
            bactext_reject_reason_name((int) reject_reason));
        Error_Detected = true;
    }
}

/** Handler for a ReadPropertyMultiple ACK.
 * @ingroup DSRPM
 * For each read property, print out the ACK'd data,
 * and free the request data items from linked property list.
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
void My_Read_Property_Multiple_Ack_Handler(
    struct bacnet_session_object *sess,
    uint8_t * service_request,
    uint32_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data)
{
    int len = 0;
    BACNET_READ_ACCESS_DATA *rpm_data;
    BACNET_READ_ACCESS_DATA *old_rpm_data;
    BACNET_PROPERTY_REFERENCE *rpm_property;
    BACNET_PROPERTY_REFERENCE *old_rpm_property;
    BACNET_APPLICATION_DATA_VALUE *value;
    BACNET_APPLICATION_DATA_VALUE *old_value;

    if (address_match(&Target_Address, src) &&
        (service_data->invoke_id == Request_Invoke_ID)) {
        rpm_data = calloc(1, sizeof(BACNET_READ_ACCESS_DATA));
        if (rpm_data) {
            len =
                rpm_ack_decode_service_request(service_request, service_len,
                rpm_data);
        }
        if (len > 0) {
            while (rpm_data) {
                rpm_ack_print_data(rpm_data);
                rpm_property = rpm_data->listOfProperties;
                while (rpm_property) {
                    value = rpm_property->value;
                    while (value) {
                        old_value = value;
                        value = value->next;
                        free(old_value);
                    }
                    old_rpm_property = rpm_property;
                    rpm_property = rpm_property->next;
                    free(old_rpm_property);
                }
                old_rpm_data = rpm_data;
                rpm_data = rpm_data->next;
                free(old_rpm_data);
            }
        } else {
            fprintf(stderr, "RPM Ack Malformed! Freeing memory...\n");
            while (rpm_data) {
                rpm_property = rpm_data->listOfProperties;
                while (rpm_property) {
                    value = rpm_property->value;
                    while (value) {
                        old_value = value;
                        value = value->next;
                        free(old_value);
                    }
                    old_rpm_property = rpm_property;
                    rpm_property = rpm_property->next;
                    free(old_rpm_property);
                }
                old_rpm_data = rpm_data;
                rpm_data = rpm_data->next;
                free(old_rpm_data);
            }
        }
    }
}

static void Init_Service_Handlers(
    struct bacnet_session_object *sess)
{
    Device_Init(sess);
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(sess, SERVICE_UNCONFIRMED_WHO_IS,
        handler_who_is);
    /* handle i-am to support binding to other devices */
    apdu_set_unconfirmed_handler(sess, SERVICE_UNCONFIRMED_I_AM,
        handler_i_am_bind);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(sess,
        handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(sess, SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_ack_handler(sess, SERVICE_CONFIRMED_READ_PROP_MULTIPLE,
        My_Read_Property_Multiple_Ack_Handler);
    /* handle any errors coming back */
    apdu_set_error_handler(sess, SERVICE_CONFIRMED_READ_PROPERTY,
        MyErrorHandler);
    apdu_set_abort_handler(sess, MyAbortHandler);
    apdu_set_reject_handler(sess, MyRejectHandler);
}

void cleanup(
    void)
{
    BACNET_READ_ACCESS_DATA *rpm_object;
    BACNET_READ_ACCESS_DATA *old_rpm_object;
    BACNET_PROPERTY_REFERENCE *rpm_property;
    BACNET_PROPERTY_REFERENCE *old_rpm_property;

    rpm_object = Read_Access_Data;
    old_rpm_object = rpm_object;
    while (rpm_object) {
        rpm_property = rpm_object->listOfProperties;
        while (rpm_property) {
            old_rpm_property = rpm_property;
            rpm_property = rpm_property->next;
            free(old_rpm_property);
        }
        old_rpm_object = rpm_object;
        rpm_object = rpm_object->next;
        free(old_rpm_object);
    }
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
    unsigned max_apdu = 0;
    uint8_t segmentation = 0;
    int args_remaining = 0, tag_value_arg = 0, arg_sets = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    bool found = false;
    uint8_t buffer[MAX_PDU] = {
        0
    };
    BACNET_READ_ACCESS_DATA *rpm_object;
    BACNET_PROPERTY_REFERENCE *rpm_property;
    struct bacnet_session_object *sess = NULL;


    if (argc < 5) {
        printf("Usage: %s device-instance object-type object-instance "
            "property index [object-type ...]\r\n",
            filename_remove_path(argv[0]));
        if ((argc > 1) && (strcmp(argv[1], "--help") == 0)) {
            printf("device-instance:\r\n"
                "BACnet Device Object Instance number that you are\r\n"
                "trying to communicate to.  This number will be used\r\n"
                "to try and bind with the device using Who-Is and\r\n"
                "I-Am services.  For example, if you were reading\r\n"
                "Device Object 123, the device-instance would be 123.\r\n"
                "\r\nobject-type:\r\n"
                "The object type is the integer value of the enumeration\r\n"
                "BACNET_OBJECT_TYPE in bacenum.h.  It is the object\r\n"
                "that you are reading.  For example if you were\r\n"
                "reading Analog Output 2, the object-type would be 1.\r\n"
                "\r\nobject-instance:\r\n"
                "This is the object instance number of the object that\r\n"
                "you are reading.  For example, if you were reading\r\n"
                "Analog Output 2, the object-instance would be 2.\r\n"
                "\r\nproperty:\r\n"
                "The property is an integer value of the enumeration\r\n"
                "BACNET_PROPERTY_ID in bacenum.h.  It is the property\r\n"
                "you are reading.  For example, if you were reading the\r\n"
                "Present Value property, use 85 as the property.\r\n"
                "\r\nindex:\r\n"
                "This integer parameter is the index number of an array.\r\n"
                "If the property is an array, individual elements can\r\n"
                "be read.  If this parameter is missing and the property\r\n"
                "is an array, the entire array will be read.\r\n"
                "\r\nExample:\r\n" "If you want read the ALL property in\r\n"
                "Device object 123, you would use the following command:\r\n"
                "%s 123 8 123 8 -1\r\n"
                "If you want read the OPTIONAL property in\r\n"
                "Device object 123, you would use the following command:\r\n"
                "%s 123 8 123 80 -1\r\n"
                "If you want read the REQUIRED property in\r\n"
                "Device object 123, you would use the following command:\r\n"
                "%s 123 8 123 105 -1\r\n", filename_remove_path(argv[0]),
                filename_remove_path(argv[0]), filename_remove_path(argv[0]));
        }
        return 0;
    }
    /* decode the command line parameters */
    Target_Device_Object_Instance = strtol(argv[1], NULL, 0);
    if (Target_Device_Object_Instance >= BACNET_MAX_INSTANCE) {
        fprintf(stderr, "device-instance=%u - it must be less than %u\r\n",
            Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
        return 1;
    }
    atexit(cleanup);
    Read_Access_Data = calloc(1, sizeof(BACNET_READ_ACCESS_DATA));
    rpm_object = Read_Access_Data;
    args_remaining = (argc - 2);
    arg_sets = 0;
    while (rpm_object) {
        tag_value_arg = 2 + (arg_sets * 4);
        rpm_object->object_type = strtol(argv[tag_value_arg], NULL, 0);
        tag_value_arg++;
        args_remaining--;
        if (args_remaining <= 0) {
            fprintf(stderr, "Error: not enough object property quads.\r\n");
            return 1;
        }
        if (rpm_object->object_type >= MAX_BACNET_OBJECT_TYPE) {
            fprintf(stderr, "object-type=%u - it must be less than %u\r\n",
                rpm_object->object_type, MAX_BACNET_OBJECT_TYPE);
            return 1;
        }
        rpm_object->object_instance = strtol(argv[tag_value_arg], NULL, 0);
        tag_value_arg++;
        args_remaining--;
        if (args_remaining <= 0) {
            fprintf(stderr, "Error: not enough object property quads.\r\n");
            return 1;
        }
        if (rpm_object->object_instance > BACNET_MAX_INSTANCE) {
            fprintf(stderr, "object-instance=%u - it must be less than %u\r\n",
                rpm_object->object_instance, BACNET_MAX_INSTANCE + 1);
            return 1;
        }
        rpm_property = calloc(1, sizeof(BACNET_PROPERTY_REFERENCE));
        rpm_object->listOfProperties = rpm_property;
        if (rpm_property) {
            rpm_property->propertyIdentifier =
                strtol(argv[tag_value_arg], NULL, 0);
            /* used up another arg */
            tag_value_arg++;
            args_remaining--;
            if (args_remaining <= 0) {
                fprintf(stderr,
                    "Error: not enough object property quads.\r\n");
                return 1;
            }
            if (rpm_property->propertyIdentifier > MAX_BACNET_PROPERTY_ID) {
                fprintf(stderr, "property=%u - it must be less than %u\r\n",
                    rpm_property->propertyIdentifier,
                    MAX_BACNET_PROPERTY_ID + 1);
                return 1;
            }
            rpm_property->propertyArrayIndex =
                strtol(argv[tag_value_arg], NULL, 0);
            /* note: we are only loading one property for now */
            rpm_property->next = NULL;
            /* used up another arg */
            tag_value_arg++;
            args_remaining--;
        }
        if (args_remaining) {
            arg_sets++;
            rpm_object->next = calloc(1, sizeof(BACNET_READ_ACCESS_DATA));
            rpm_object = rpm_object->next;
        } else {
            break;
        }
    }
    /* setup my info */
    sess = create_bacnet_session();
    Device_Set_Object_Instance_Number(sess, BACNET_MAX_INSTANCE);
    address_init(sess);
    Init_Service_Handlers(sess);
    dlenv_init(sess);
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = (apdu_timeout(sess) / 1000) * apdu_retries(sess);
    /* try to bind with the device */
    found =
        address_bind_request(sess, Target_Device_Object_Instance, &max_apdu,
        &segmentation, &Target_Address);
    if (!found) {
        Send_WhoIs(sess, Target_Device_Object_Instance,
            Target_Device_Object_Instance);
    }
    /* loop forever */
    for (;;) {
        /* increment timer - exit if timed out */
        current_seconds = time(NULL);

        /* at least one second has passed */
        if (current_seconds != last_seconds)
            tsm_timer_milliseconds(sess,
                ((current_seconds - last_seconds) * 1000));
        if (Error_Detected)
            break;
        /* wait until the device is bound, or timeout and quit */
        if (!found) {
            found =
                address_bind_request(sess, Target_Device_Object_Instance,
                &max_apdu, &segmentation, &Target_Address);
        }
        if (found) {
            if (Request_Invoke_ID == 0) {
                Request_Invoke_ID =
                    Send_Read_Property_Multiple_Request(sess, NULL, &buffer[0],
                    sizeof(buffer), Target_Device_Object_Instance,
                    Read_Access_Data);
            } else if (tsm_invoke_id_free(sess, Request_Invoke_ID))
                break;
            else if (tsm_invoke_id_failed(sess, Request_Invoke_ID)) {
                fprintf(stderr, "\rError: TSM Timeout!\r\n");
                tsm_free_invoke_id_check(sess, Request_Invoke_ID,
                    &Target_Address, true);
                Error_Detected = true;
                /* try again or abort? */
                break;
            }
        } else {
            /* increment timer - exit if timed out */
            elapsed_seconds += (current_seconds - last_seconds);
            if (elapsed_seconds > timeout_seconds) {
                printf("\rError: APDU Timeout!\r\n");
                Error_Detected = true;
                break;
            }
        }

        /* returns 0 bytes on timeout */
        pdu_len =
            sess->datalink_receive(sess, &src, &Rx_Buf[0], MAX_MPDU, timeout);

        /* process */
        if (pdu_len) {
            npdu_handler(sess, &src, &Rx_Buf[0], pdu_len);
        }

        /* keep track of time for next check */
        last_seconds = current_seconds;
    }
    /* perform memory desallocation */
    bacnet_destroy_session(sess);


    if (Error_Detected)
        return 1;
    return 0;
}
