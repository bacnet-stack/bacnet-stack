/**
 * @file
 * @brief Header file for a basic SubscribeCOV request handler, FSM, & Task
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_SERVICE_HANDLER_COV_SUBSCRIBE_H
#define BACNET_BASIC_SERVICE_HANDLER_COV_SUBSCRIBE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void handler_cov_subscribe(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data);
BACNET_STACK_EXPORT
bool handler_cov_fsm(bool);
BACNET_STACK_EXPORT
void handler_cov_task(void);
BACNET_STACK_EXPORT
void handler_cov_timer_seconds(uint32_t elapsed_seconds);
BACNET_STACK_EXPORT
void handler_cov_init(void);
BACNET_STACK_EXPORT
int handler_cov_encode_subscriptions(uint8_t *apdu, int max_apdu);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
