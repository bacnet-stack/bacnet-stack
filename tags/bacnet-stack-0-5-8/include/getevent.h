/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2009 Steve Karg

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
#ifndef GETEVENT_H
#define GETEVENT_H

#include <stdint.h>
#include <stdbool.h>
#include "bacdef.h"
#include "bacenum.h"
#include "timestamp.h"
#include "event.h"

struct BACnet_Get_Event_Information_Data;
typedef struct BACnet_Get_Event_Information_Data {
    BACNET_OBJECT_ID objectIdentifier;
    BACNET_EVENT_STATE eventState;
    BACNET_BIT_STRING acknowledgedTransitions;
    BACNET_TIMESTAMP eventTimeStamps[3];
    BACNET_NOTIFY_TYPE notifyType;
    BACNET_BIT_STRING eventEnable;
    uint32_t eventPriorities[3];
    struct BACnet_Get_Event_Information_Data *next;
} BACNET_GET_EVENT_INFORMATION_DATA;

/* return 0 if no active event at this index
   return -1 if end of list 
   return +1 if active event */
typedef int (
    *get_event_info_function) (
    unsigned index,
    BACNET_GET_EVENT_INFORMATION_DATA * getevent_data);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    int getevent_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_OBJECT_ID * lastReceivedObjectIdentifier);

    int getevent_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_OBJECT_ID * object_id);

    int getevent_ack_encode_apdu_init(
        uint8_t * apdu,
        size_t max_apdu,
        uint8_t invoke_id);

    int getevent_ack_encode_apdu_data(
        uint8_t * apdu,
        size_t max_apdu,
        BACNET_GET_EVENT_INFORMATION_DATA * get_event_data);

    int getevent_ack_encode_apdu_end(
        uint8_t * apdu,
        size_t max_apdu,
        bool moreEvents);

    int getevent_ack_decode_service_request(
        uint8_t * apdu,
        int apdu_len,   /* total length of the apdu */
        BACNET_GET_EVENT_INFORMATION_DATA * get_event_data,
        bool * moreEvents);

#ifdef TEST
#include "ctest.h"
    int getevent_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_len,
        uint8_t * invoke_id,
        BACNET_OBJECT_ID * lastReceivedObjectIdentifier);

    int getevent_ack_decode_apdu(
        uint8_t * apdu,
        int apdu_len,   /* total length of the apdu */
        uint8_t * invoke_id,
        BACNET_GET_EVENT_INFORMATION_DATA * get_event_data,
        bool * moreEvents);

    void testGetEventInformationAck(
        Test * pTest);

    void testGetEventInformation(
        Test * pTest);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
