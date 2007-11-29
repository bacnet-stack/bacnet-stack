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
#ifndef DCC_H
#define DCC_H

#include <stdint.h>
#include <stdbool.h>
#include "bacenum.h"
#include "bacstr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* return the status */
    BACNET_COMMUNICATION_ENABLE_DISABLE dcc_enable_status(
        void);
    bool dcc_communication_enabled(
        void);
    bool dcc_communication_disabled(
        void);
    bool dcc_communication_initiation_disabled(
        void);
/* return the time */
    uint32_t dcc_duration_seconds(
        void);
/* called every second or so.  If more than one second,
  then seconds should be the number of seconds to tick away */
    void dcc_timer_seconds(
        uint32_t seconds);
/* setup the communication values */
    bool dcc_set_status_duration(
        BACNET_COMMUNICATION_ENABLE_DISABLE status,
        uint16_t minutes);

/* encode service */
    int dcc_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        uint16_t timeDuration,  /* 0=optional */
        BACNET_COMMUNICATION_ENABLE_DISABLE enable_disable,
        BACNET_CHARACTER_STRING * password);    /* NULL=optional */

/* decode the service request only */
    int dcc_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        uint16_t * timeDuration,
        BACNET_COMMUNICATION_ENABLE_DISABLE * enable_disable,
        BACNET_CHARACTER_STRING * password);

#ifdef TEST
#include "ctest.h"
    int dcc_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_len,
        uint8_t * invoke_id,
        uint16_t * timeDuration,
        BACNET_COMMUNICATION_ENABLE_DISABLE * enable_disable,
        BACNET_CHARACTER_STRING * password);

    void test_DeviceCommunicationControl(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
