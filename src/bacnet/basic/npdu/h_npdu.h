/**
 * @file
 * @brief Header file for a basic NPDU handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2019
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_NPDU_HANDLER_H
#define BACNET_BASIC_NPDU_HANDLER_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void npdu_handler(BACNET_ADDRESS *src, uint8_t *pdu, uint16_t pdu_len);

BACNET_STACK_EXPORT
uint16_t npdu_network_number(void);
BACNET_STACK_EXPORT
void npdu_network_number_set(uint16_t net);
BACNET_STACK_EXPORT
int npdu_send_network_number_is(
    BACNET_ADDRESS *dst, uint16_t net, uint8_t status);
BACNET_STACK_EXPORT
int npdu_send_what_is_network_number(BACNET_ADDRESS *dst);

BACNET_STACK_EXPORT
void npdu_handler_cleanup(void);
BACNET_STACK_EXPORT
void npdu_handler_init(uint16_t bip_net, uint16_t mstp_net);
BACNET_STACK_EXPORT
void npdu_router_handler(
    uint16_t snet, BACNET_ADDRESS *src, uint8_t *pdu, uint16_t pdu_len);
BACNET_STACK_EXPORT
int npdu_router_send_pdu(
    uint16_t dnet,
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned int pdu_len);
BACNET_STACK_EXPORT
void npdu_router_get_my_address(uint16_t dnet, BACNET_ADDRESS *my_address);
BACNET_STACK_EXPORT
int npdu_send_reject_message_to_network(BACNET_ADDRESS *dst, uint16_t net);

/* I Am Router To Network function */
typedef void (*i_am_router_to_network_function)(
    BACNET_ADDRESS *src, uint16_t network);

BACNET_STACK_EXPORT
void npdu_set_i_am_router_to_network_handler(
    i_am_router_to_network_function pFunction);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
