/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic Abort message send
*
* @section LICENSE
*
* SPDX-License-Identifier: MIT
*/
#ifndef SEND_ABORT_TO_NETWORK_H
#define SEND_ABORT_TO_NETWORK_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int Send_Abort_To_Network(
        uint8_t * buffer,
        BACNET_ADDRESS *dest,
        uint8_t invoke_id,
        BACNET_ABORT_REASON reason,
        bool server);

    BACNET_STACK_EXPORT
    int abort_encode_pdu(
        uint8_t * buffer,
        BACNET_ADDRESS * dest,
        BACNET_ADDRESS * src,
        BACNET_NPDU_DATA * npdu_data,
        uint8_t invoke_id,
        BACNET_ABORT_REASON reason,
        bool server);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
