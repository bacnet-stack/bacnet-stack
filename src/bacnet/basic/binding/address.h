/**
 * @file
 * @brief API for basic BACnet address binding table
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_ADDRESS_H
#define BACNET_BASIC_ADDRESS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacaddr.h"
#include "bacnet/readrange.h"

/* refactored utility functions - see bacaddr.c module */
#define address_mac_init(m, a, l) bacnet_address_mac_init(m, a, l)
#define address_mac_from_ascii(m, a) bacnet_address_mac_from_ascii(m, a)
#define address_match(d, s) bacnet_address_same(d, s)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void address_init(void);

BACNET_STACK_EXPORT
void address_init_partial(void);

BACNET_STACK_EXPORT
void address_add(
    uint32_t device_id, unsigned max_apdu, const BACNET_ADDRESS *src);

BACNET_STACK_EXPORT
void address_remove_device(uint32_t device_id);

BACNET_STACK_EXPORT
bool address_get_by_device(
    uint32_t device_id, unsigned *max_apdu, BACNET_ADDRESS *src);

BACNET_STACK_EXPORT
bool address_segment_get_by_device(
    uint32_t device_id,
    unsigned *max_apdu,
    BACNET_ADDRESS *src,
    uint8_t *segmentation,
    uint16_t *maxsegments);

BACNET_STACK_EXPORT
bool address_get_by_index(
    unsigned index,
    uint32_t *device_id,
    unsigned *max_apdu,
    BACNET_ADDRESS *src);

BACNET_STACK_EXPORT
bool address_device_get_by_index(
    unsigned index,
    uint32_t *device_id,
    uint32_t *device_ttl,
    unsigned *max_apdu,
    BACNET_ADDRESS *src);

BACNET_STACK_EXPORT
bool address_get_device_id(const BACNET_ADDRESS *src, uint32_t *device_id);

BACNET_STACK_EXPORT
unsigned address_count(void);

BACNET_STACK_EXPORT
bool address_bind_request(
    uint32_t device_id, unsigned *max_apdu, BACNET_ADDRESS *src);

BACNET_STACK_EXPORT
bool address_device_bind_request(
    uint32_t device_id,
    uint32_t *device_ttl,
    unsigned *max_apdu,
    BACNET_ADDRESS *src);

BACNET_STACK_EXPORT
void address_add_binding(
    uint32_t device_id, unsigned max_apdu, const BACNET_ADDRESS *src);

BACNET_STACK_EXPORT
int address_list_encode(uint8_t *apdu, unsigned apdu_len);

BACNET_STACK_EXPORT
int rr_address_list_encode(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest);

BACNET_STACK_EXPORT
void address_set_device_TTL(
    uint32_t device_id, uint32_t TimeOut, bool StaticFlag);

BACNET_STACK_EXPORT
void address_cache_timer(uint16_t uSeconds);

BACNET_STACK_EXPORT
void address_protected_entry_index_set(uint32_t top_protected_entry_index);
BACNET_STACK_EXPORT
void address_own_device_id_set(uint32_t own_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
