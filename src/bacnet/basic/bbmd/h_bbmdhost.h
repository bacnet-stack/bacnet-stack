/**
 * @file
 * @brief BBMD Host Name handling for BACnet/IPv4
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#ifndef BACNET_BASIC_BBMD_H_HOST_H
#define BACNET_BASIC_BBMD_H_HOST_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/hostnport.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Callback to query the hostname to IP address and update the address
 * @param host [in,out] The host and port data
 */
typedef void (*bbmdhost_lookup_callback)(BACNET_HOST_N_PORT_MINIMAL *host);

BACNET_STACK_EXPORT
void bbmdhost_init(void);

BACNET_STACK_EXPORT
void bbmdhost_cleanup(void);

BACNET_STACK_EXPORT
int bbmdhost_add(BACNET_CHARACTER_STRING *name);

BACNET_STACK_EXPORT
int bbmdhost_count(void);
BACNET_STACK_EXPORT
bool bbmdhost_data(int index, BACNET_HOST_ADDRESS_PAIR *data);

BACNET_STACK_EXPORT
bool bbmdhost_data_resolved(int index, BACNET_HOST_N_PORT_MINIMAL *host);

BACNET_STACK_EXPORT
void bbmdhost_lookup_update(bbmdhost_lookup_callback callback);

BACNET_STACK_EXPORT
bool bbmdhost_ip_address(
    int index, uint8_t *ip_address, uint8_t *ip_address_len);

BACNET_STACK_EXPORT
bool bbmdhost_hostname_ip_address(
    const char *hostname, uint8_t *ip_address, uint8_t *ip_address_len);

BACNET_STACK_EXPORT
bool bbmdhost_same(
    BACNET_HOST_ADDRESS_PAIR *dst, BACNET_HOST_ADDRESS_PAIR *src);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
