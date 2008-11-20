/**************************************************************************
*
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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "config.h"
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "apdu.h"
#include "npdu.h"
#include "abort.h"
/* special for this module */
#include "cov.h"
#include "bactext.h"

/* note: nothing is specified in BACnet about what to do with the
  information received from Unconfirmed COV Notifications. */
void handler_ucov_notification(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src)
{
    BACNET_COV_DATA cov_data;
    BACNET_PROPERTY_VALUE property_value;
    int len = 0;

    /* create linked list to store data if more 
        than one property value is expected */
    property_value.next = NULL;
    cov_data.listOfValues = &property_value;
#if PRINT_ENABLED
    fprintf(stderr, "UCOV: Received Notification!\n");
#endif
    /* decode the service request only */
    len = cov_notify_decode_service_request(
        service_request, service_len, &cov_data);
#if PRINT_ENABLED
    if (len > 0) {
        fprintf(stderr, "UCOV: PID=%u ",
            cov_data.subscriberProcessIdentifier);
        fprintf(stderr, "instance=%u ",
            cov_data.initiatingDeviceIdentifier);
        fprintf(stderr, "%s %u ",
            bactext_object_type_name(
                cov_data.monitoredObjectIdentifier.type),
            cov_data.monitoredObjectIdentifier.instance);
        fprintf(stderr, "time remaining=%u seconds ",
            cov_data.timeRemaining);
        fprintf(stderr, "%s ",
            bactext_property_name(property_value.propertyIdentifier));
        if (property_value.propertyArrayIndex != BACNET_ARRAY_ALL) {
            fprintf(stderr, "%u ",
                property_value.propertyArrayIndex);
        }
        fprintf(stderr, "\n");
    } else {
        fprintf(stderr, "UCOV: Unable to decode service request!\n");
    }
#endif
}
