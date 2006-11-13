/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2006 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#ifndef COV_H
#define COV_H

#include <stdint.h>
#include <stdbool.h>
#include "bacapp.h"

struct BACnet_Property_Value;
typedef struct BACnet_Property_Value {
    BACNET_PROPERTY_ID propertyIdentifier;
    unsigned propertyArrayIndex;
    BACNET_APPLICATION_DATA_VALUE value;
    uint8_t priority;
    /* simple linked list */
    struct BACnet_Property_Value *next;
} BACNET_PROPERTY_VALUE;

typedef struct BACnet_COV_Data {
    uint32_t subscriberProcessIdentifier;
    uint32_t initiatingDeviceIdentifier;
    BACNET_OBJECT_ID monitoredObjectIdentifier;
    unsigned timeRemaining;
    /* simple linked list of values */
    BACNET_PROPERTY_VALUE listOfValues;
} BACNET_COV_DATA;

typedef struct BACnet_Property_Reference {
  BACNET_PROPERTY_ID propertyIdentifier;
  unsigned propertyArrayIndex; /* optional */
} BACNET_PROPERTY_REFERENCE;

typedef struct BACnet_Subscribe_COV_Data {
    uint32_t subscriberProcessIdentifier;
    BACNET_OBJECT_ID monitoredObjectIdentifier;
    bool cancellationRequest; /* true if this is a cancellation request */
    bool issueConfirmedNotifications; /* optional */
    unsigned lifetime; /* optional */
    BACNET_PROPERTY_REFERENCE monitoredProperty;
    bool covIncrementPresent; /* true if present */
    float covIncrement; /* optional */
} BACNET_SUBSCRIBE_COV_DATA;

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

    int ucov_notify_encode_apdu(uint8_t * apdu, BACNET_COV_DATA * data);

    int ucov_notify_decode_apdu(uint8_t * apdu,
        unsigned apdu_len, BACNET_COV_DATA * data);

    int ucov_notify_send(uint8_t * buffer, BACNET_COV_DATA * data);

    int ccov_notify_encode_apdu(uint8_t * apdu,
        uint8_t invoke_id, BACNET_COV_DATA * data);

    int ccov_notify_decode_apdu(uint8_t * apdu,
        unsigned apdu_len, uint8_t * invoke_id, BACNET_COV_DATA * data);

    /* common for both confirmed and unconfirmed */
    int cov_notify_decode_service_request(uint8_t * apdu,
        unsigned apdu_len, BACNET_COV_DATA * data);

    int cov_subscribe_property_decode_service_request(uint8_t * apdu,
        unsigned apdu_len, BACNET_SUBSCRIBE_COV_DATA * data);

    int cov_subscribe_property_encode_adpu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_SUBSCRIBE_COV_DATA * data);

    int cov_subscribe_decode_service_request(uint8_t * apdu,
        unsigned apdu_len, BACNET_SUBSCRIBE_COV_DATA * data);

    int cov_subscribe_encode_adpu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_SUBSCRIBE_COV_DATA * data);


#ifdef TEST
#include "ctest.h"
    void testCOVNotify(Test * pTest);
    void testCOVSubscribeProperty(Test * pTest);
    void testCOVSubscribe(Test * pTest);
#endif

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif
