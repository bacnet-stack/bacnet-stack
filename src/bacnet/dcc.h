/**
 * @file
 * @brief BACnet DeviceCommunicationControl encode and decode functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DEVICE_COMMUNICATION_CONTROL_H
#define BACNET_DEVICE_COMMUNICATION_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacstr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* return the status */
    BACNET_STACK_EXPORT
    BACNET_COMMUNICATION_ENABLE_DISABLE dcc_enable_status(
        void);
    BACNET_STACK_EXPORT
    bool dcc_communication_enabled(
        void);
    BACNET_STACK_EXPORT
    bool dcc_communication_disabled(
        void);
    BACNET_STACK_EXPORT
    bool dcc_communication_initiation_disabled(
        void);
/* return the time */
    BACNET_STACK_EXPORT
    uint32_t dcc_duration_seconds(
        void);
/* called every second or so.  If more than one second,
  then seconds should be the number of seconds to tick away */
    BACNET_STACK_EXPORT
    void dcc_timer_seconds(
        uint32_t seconds);
/* setup the communication values */
    BACNET_STACK_EXPORT
    bool dcc_set_status_duration(
        BACNET_COMMUNICATION_ENABLE_DISABLE status,
        uint16_t minutes);

    BACNET_STACK_EXPORT
    int dcc_apdu_encode(uint8_t *apdu,
        uint16_t timeDuration,
        BACNET_COMMUNICATION_ENABLE_DISABLE enable_disable,
        const BACNET_CHARACTER_STRING *password);
    BACNET_STACK_EXPORT
    size_t dcc_service_request_encode(uint8_t *apdu,
        size_t apdu_size,
        uint16_t timeDuration,
        BACNET_COMMUNICATION_ENABLE_DISABLE enable_disable,
        const BACNET_CHARACTER_STRING *password);
    BACNET_STACK_EXPORT
    int dcc_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        uint16_t timeDuration,
        BACNET_COMMUNICATION_ENABLE_DISABLE enable_disable,
        const BACNET_CHARACTER_STRING * password);

    BACNET_STACK_EXPORT
    int dcc_decode_service_request(
        const uint8_t * apdu,
        unsigned apdu_len,
        uint16_t * timeDuration,
        BACNET_COMMUNICATION_ENABLE_DISABLE * enable_disable,
        BACNET_CHARACTER_STRING * password);

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
 */

/** @defgroup DMDCC Device Management-Device Communication Control (DM-DCC)
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
 */

/** @defgroup NMRC Network Management-Router Configuration (NM-RC)
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
