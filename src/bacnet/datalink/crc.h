/**
 * @file
 * @brief BACnet MS/TP datalink CRC encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 * @defgroup DLMSTP BACnet MS/TP DataLink Network Layer
 * @ingroup DataLink
 */
#ifndef BACNET_CRC_H
#define BACNET_CRC_H

#include <stddef.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    uint8_t CRC_Calc_Header(
        uint8_t dataValue,
        uint8_t crcValue);
    BACNET_STACK_EXPORT
    uint16_t CRC_Calc_Data(
        uint8_t dataValue,
        uint16_t crcValue);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
