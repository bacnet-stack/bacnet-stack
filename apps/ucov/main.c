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

/* command line tool that sends a BACnet service */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* for time */
#include <errno.h>
#include "bacnet/bactext.h"
#include "bacnet/iam.h"
#include "bacnet/cov.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/basic/object/device.h"
#include "bacport.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/whois.h"
/* some demo stuff needed */
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/dlenv.h"

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
}

static void print_usage(char *filename)
{
    printf("Usage: %s pid device-id object-type object-instance "
        "time property tag value [priority] [index]\n", filename);
    printf("\n");
    printf("pid:\n"
        "Process Identifier for this broadcast.\n");
    printf("\n");
    printf("device-id:\n"
        "The Initiating BACnet Device Object Instance number.\n");
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
        "The monitored object instance number.\n");
    printf("\n");
    printf("time:\n"
        "The subscription time remaining is conveyed in seconds.\n");
    printf("\n");
    printf("property:\n"
        "The property of the object that you are reading. It\n"
        "can be defined either as the property name string as\n"
        "defined in the BACnet specification, or as an integer\n"
        "value of the enumeration BACNET_PROPERTY_ID in\n"
        "bacenum.h. For example, if you were reading the Present\n"
        "Value property, use present-value or 85 as the property.\n");
    printf("\n");
    printf("tag:\n"
        "Tag is the integer value of the enumeration BACNET_APPLICATION_TAG \n"
        "in bacenum.h.  It is the data type of the value that you are\n"
        "monitoring.  For example, if you were monitoring a REAL value,\n"
        "you would use a tag of 4.\n");
    printf("\n");
    printf("value:\n"
        "The value is an ASCII representation of some type of data that you\n"
        "are monitoring.  It is encoded using the tag information provided.\n"
        "For example, if you were writing a REAL value of 100.0,\n"
        "you would use 100.0 as the value.\n");
    printf("\n");
    printf("[priority]:\n"
        "This optional parameter is used for reporting the priority of the\n"
        "value. If no priority is given, none is sent, and the BACnet \n"
        "standard requires that the value is reported at the lowest \n"
        "priority (16) if the object property supports priorities.\n");
    printf("\n");
    printf("[index]\n"
        "This optional integer parameter is the index number of an array.\n"
        "If the property is an array, individual elements can be reported.\n");
    printf("\n");
    printf("Example:\n"
        "If you want generate an unconfirmed COV,\n"
        "you could send one of the following command:\n"
        "%s 1 2 analog-value 4 5 prevent-value 4 100.0\n"
        "%s 1 2 3 4 5 85 4 100.0\n"
        "where 1=pid, 2=device-id, 3=AV, 4=object-id, 5=time,\n"
        "85=Present-Value, 4=REAL, 100.0=value\n",
        filename, filename);

}

int main(int argc, char *argv[])
{
    char *value_string = NULL;
    bool status = false;
    BACNET_COV_DATA cov_data;
    BACNET_PROPERTY_VALUE value_list;
    uint8_t tag;
    unsigned object_type = 0;
    unsigned object_property = 0;

    if (argc < 7) {
        print_usage(filename_remove_path(argv[0]));
        return 0;
    }
    /* decode the command line parameters */
    cov_data.subscriberProcessIdentifier = strtol(argv[1], NULL, 0);
    cov_data.initiatingDeviceIdentifier = strtol(argv[2], NULL, 0);
    if (bactext_object_type_strtol(argv[3], &object_type) == false) {
        fprintf(stderr, "error: object-type=%s invalid\n", argv[3]);
        return 1;
    }
    cov_data.monitoredObjectIdentifier.type = (uint16_t)object_type;
    cov_data.monitoredObjectIdentifier.instance = strtol(argv[4], NULL, 0);
    cov_data.timeRemaining = strtol(argv[5], NULL, 0);
    cov_data.listOfValues = &value_list;
    value_list.next = NULL;
    if (bactext_property_strtol(argv[6], &object_property) == false) {
        fprintf(stderr, "property=%s invalid\n", argv[6]);
        return 1;
    }
    value_list.propertyIdentifier = object_property;
    tag = strtol(argv[7], NULL, 0);
    value_string = argv[8];
    /* optional priority */
    if (argc > 9) {
        value_list.priority = strtol(argv[9], NULL, 0);
    } else {
        value_list.priority = BACNET_NO_PRIORITY;
    }
    /* optional index */
    if (argc > 10) {
        value_list.propertyArrayIndex = strtol(argv[10], NULL, 0);
    } else {
        value_list.propertyArrayIndex = BACNET_ARRAY_ALL;
    }

    if (cov_data.initiatingDeviceIdentifier >= BACNET_MAX_INSTANCE) {
        fprintf(stderr, "device-instance=%u - it must be less than %u\n",
            cov_data.initiatingDeviceIdentifier, BACNET_MAX_INSTANCE);
        return 1;
    }
    if (cov_data.monitoredObjectIdentifier.type >= MAX_BACNET_OBJECT_TYPE) {
        fprintf(stderr, "object-type=%u - it must be less than %u\n",
            cov_data.monitoredObjectIdentifier.type, MAX_BACNET_OBJECT_TYPE);
        return 1;
    }
    if (cov_data.monitoredObjectIdentifier.instance > BACNET_MAX_INSTANCE) {
        fprintf(stderr, "object-instance=%u - it must be less than %u\n",
            cov_data.monitoredObjectIdentifier.instance,
            BACNET_MAX_INSTANCE + 1);
        return 1;
    }
    if (cov_data.listOfValues->propertyIdentifier > MAX_BACNET_PROPERTY_ID) {
        fprintf(stderr, "property-identifier=%u - it must be less than %u\n",
            cov_data.listOfValues->propertyIdentifier,
            MAX_BACNET_PROPERTY_ID + 1);
        return 1;
    }
    if (tag >= MAX_BACNET_APPLICATION_TAG) {
        fprintf(stderr, "tag=%u - it must be less than %u\n", tag,
            MAX_BACNET_APPLICATION_TAG);
        return 1;
    }
    status = bacapp_parse_application_data(
        tag, value_string, &cov_data.listOfValues->value);
    if (!status) {
        /* FIXME: show the expected entry format for the tag */
        fprintf(stderr, "unable to parse the tag value\n");
        return 1;
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    dlenv_init();
    atexit(datalink_cleanup);
    Send_UCOV_Notify(&Handler_Transmit_Buffer[0],
        sizeof(Handler_Transmit_Buffer), &cov_data);

    return 0;
}
