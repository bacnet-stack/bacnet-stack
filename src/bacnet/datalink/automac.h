/**
 * @file
 * @brief BACnet MS/TP Auto MAC address functionality
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2011
 * @copyright SPDX-License-Identifier: MIT
 * @defgroup DLMSTP BACnet MS/TP DataLink Network Layer
 * @ingroup DataLink
 */
#ifndef BACNET_AUTOMAC_H
#define BACNET_AUTOMAC_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/* starting number available for AutoMAC */
#define MSTP_MAC_SLOTS_OFFSET 32
/* total number of slots */
#define MSTP_MAC_SLOTS_MAX 128

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void automac_init(void);

bool automac_free_address_valid(uint8_t mac);
uint8_t automac_free_address_count(void);
uint8_t automac_free_address_mac(uint8_t count);
uint8_t automac_free_address_random(void);
void automac_pfm_set(uint8_t mac);
void automac_token_set(uint8_t mac);
void automac_emitter_set(uint8_t mac);
uint8_t automac_next_station(uint8_t mac);
uint8_t automac_address(void);
void automac_address_set(uint8_t mac);
void automac_address_init(void);
uint16_t automac_time_slot(void);
bool automac_pfm_cycle_complete(void);
bool automac_enabled(void);
void automac_enabled_set(bool status);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
