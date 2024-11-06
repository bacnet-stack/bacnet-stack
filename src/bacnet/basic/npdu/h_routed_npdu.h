/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2019
 * @brief Header file for a basic routing NPDU handler
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_NPDU_ROUTED_HANDLER_H
#define BACNET_BASIC_NPDU_ROUTED_HANDLER_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void routing_npdu_handler(
    BACNET_ADDRESS *src, int *DNET_list, uint8_t *pdu, uint16_t pdu_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
