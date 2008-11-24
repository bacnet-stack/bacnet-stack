/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2008 John Minack

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

#ifndef _BAC_PROP_STATES_H_
#define _BAC_PROP_STATES_H_

#include "bacenum.h"
#include <stdint.h>
#include <stdbool.h>
#include "bacapp.h"
#include <time.h>
#include "timestamp.h"

typedef enum {
    BOOLEAN_VALUE,
    BINARY_VALUE,
    EVENT_TYPE,
    POLARITY,
    PROGRAM_CHANGE,
    PROGRAM_STATE,
    REASON_FOR_HALT,
    RELIABILITY,
    STATE,
    SYSTEM_STATUS,
    UNITS,
    UNSIGNED_VALUE,
    LIFE_SAFETY_MODE,
    LIFE_SAFETY_STATE,
} BACNET_PROPERTY_STATE_TYPE;

typedef struct {
    BACNET_PROPERTY_STATE_TYPE tag;
    union {
        bool booleanValue;
        BACNET_BINARY_PV binaryValue;
        BACNET_EVENT_TYPE eventType;
        BACNET_POLARITY polarity;
        BACNET_PROGRAM_REQUEST programChange;
        BACNET_PROGRAM_STATE programState;
        BACNET_PROGRAM_ERROR programError;
        BACNET_RELIABILITY reliability;
        BACNET_EVENT_STATE state;
        BACNET_DEVICE_STATUS systemStatus;
        BACNET_ENGINEERING_UNITS units;
        uint32_t unsignedValue;
        BACNET_LIFE_SAFETY_MODE lifeSafetyMode;
        BACNET_LIFE_SAFETY_STATE lifeSafetyState;
    } state;
} BACNET_PROPERTY_STATE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    int bacapp_decode_property_state(
        uint8_t * apdu,
        BACNET_PROPERTY_STATE * value);

    int bacapp_decode_context_property_state(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_PROPERTY_STATE * value);

    int bacapp_encode_property_state(
        uint8_t * apdu,
        BACNET_PROPERTY_STATE * value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif // _BAC_PROP_STATES_H_
