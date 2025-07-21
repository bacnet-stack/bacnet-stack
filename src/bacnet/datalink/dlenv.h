/**
 * @file
 * @brief Environment variables used for the BACnet command line tools
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2009
 * @copyright SPDX-License-Identifier: MIT
 * @ingroup DataLink
 */
#ifndef BACNET_DLENV_H
#define BACNET_DLENV_H
#include <stddef.h>
#include <stdint.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/datalink/bvlc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void dlenv_init(void);

BACNET_STACK_EXPORT
void dlenv_debug_disable(void);

BACNET_STACK_EXPORT
void dlenv_debug_enable(void);

BACNET_STACK_EXPORT
int dlenv_register_as_foreign_device(void);

BACNET_STACK_EXPORT
void dlenv_network_port_init(void);

BACNET_STACK_EXPORT
void dlenv_maintenance_timer(uint16_t elapsed_seconds);

BACNET_STACK_EXPORT
void dlenv_bbmd_address_set(const BACNET_IP_ADDRESS *address);

BACNET_STACK_EXPORT
void dlenv_bbmd_ttl_set(uint16_t ttl_secs);

BACNET_STACK_EXPORT
int dlenv_bbmd_result(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
