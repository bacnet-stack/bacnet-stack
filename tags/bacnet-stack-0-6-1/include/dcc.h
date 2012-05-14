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
/** @defgroup RDMS  Device and Network Management Service BIBBs
 * These device management BIBBs prescribe the BACnet capabilities required 
 * to interoperably perform the device management functions enumerated in 
 * 22.2.1.5 for the BACnet devices defined therein. 
 * 
 * The network management BIBBs prescribe the BACnet capabilities required to 
 * interoperably perform network management functions.
          *//** @defgroup DMDCC Device Management-Device Communication Control (DM-DCC)
 * @ingroup RDMS
 * 16.1 DeviceCommunicationControl Service <br>
 * The DeviceCommunicationControl service is used by a client BACnet-user to 
 * instruct a remote device to stop initiating and optionally stop responding 
 * to all APDUs (except DeviceCommunicationControl or, if supported, 
 * ReinitializeDevice) on the communication network or internetwork for a 
 * specified duration of time. This service is primarily used by a human operator
 * for diagnostic purposes. A password may be required from the client 
 * BACnet-user prior to executing the service. The time duration may be set to 
 * "indefinite," meaning communication must be re-enabled by a 
 * DeviceCommunicationControl or, if supported, ReinitializeDevice service, 
 * not by time.
          *//** @defgroup NMRC Network Management-Router Configuration (NM-RC)
 * @ingroup RDMS
 * The A device may query and change the configuration of routers and 
 * half-routers.
 * The B device responds to router management commands and must meet the 
 * requirements for BACnet Routers as stated in Clause 6.
 * 
 * 6.4 Network Layer Protocol Messages <br>
 * This subclause describes the format and purpose of the ten BACnet network 
 * layer protocol messages. These messages provide the basis for router 
 * auto-configuration, router table maintenance, and network layer congestion 
 * control.
 */
#endif
