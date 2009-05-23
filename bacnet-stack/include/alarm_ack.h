/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2009 John Minack

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

#ifndef ALARM_ACK_H_
#define ALARM_ACK_H_

#include "bacenum.h"
#include <stdint.h>
#include <stdbool.h>
#include "bacapp.h"
#include "timestamp.h"

typedef struct  {
    uint32_t                ackProcessIdentifier;
    BACNET_OBJECT_ID        eventObjectIdentifier;
    BACNET_EVENT_TYPE       eventTypeAcked;
    BACNET_TIMESTAMP        eventTimeStamp;
    BACNET_CHARACTER_STRING ackSource;       
	BACNET_TIMESTAMP        ackTimeStamp;
} BACNET_ALARM_ACK_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************
**
** Creates a Alarm Acknowledge APDU
**
****************************************************/
    int alarm_ack_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_ALARM_ACK_DATA * data);

/***************************************************
**
** Encodes the service data part of Alarm Acknowledge
**
****************************************************/
    int alarm_ack_encode_service_request(
        uint8_t * apdu,
        BACNET_ALARM_ACK_DATA * data);

/***************************************************
**
** Decodes the service data part of Alarm Acknowledge
**
****************************************************/
    int alarm_ack_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_ALARM_ACK_DATA * data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ALARM_ACK_H_ */



