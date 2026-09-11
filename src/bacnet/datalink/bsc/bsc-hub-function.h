/**
 * @file
 * @brief BACnet secure connect hub function API.
 *        In general, user should not use that API directly,
 *        BACnet/SC datalink API should be used.
 * @author Kirill Neznamov <kirill.neznamov@dsr-corporation.com>
 * @date July 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DATALINK_BSC_HUB_FUNCTION_H
#define BACNET_DATALINK_BSC_HUB_FUNCTION_H
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/datalink/bsc/bsc-retcodes.h"
#include "bacnet/datalink/bsc/bsc-socket.h"
#include "bacnet/basic/object/sc_netport.h"

typedef void *BSC_HUB_FUNCTION_HANDLE;

typedef enum {
    BSC_HUBF_EVENT_STARTED = 1,
    BSC_HUBF_EVENT_STOPPED = 2,
    BSC_HUBF_EVENT_ERROR_DUPLICATED_VMAC = 3
} BSC_HUB_FUNCTION_EVENT;

typedef void (*BSC_HUB_EVENT_FUNC)(
    BSC_HUB_FUNCTION_EVENT ev, BSC_HUB_FUNCTION_HANDLE h, void *user_arg);

BACNET_STACK_EXPORT
BSC_SC_RET bsc_hub_function_start(
    uint8_t *ca_cert_chain,
    size_t ca_cert_chain_size,
    uint8_t *cert_chain,
    size_t cert_chain_size,
    uint8_t *key,
    size_t key_size,
    int port,
    char *iface,
    const BACNET_SC_UUID *local_uuid,
    BACNET_SC_VMAC_ADDRESS *local_vmac,
    uint16_t max_local_bvlc_len,
    uint16_t max_local_npdu_len,
    unsigned int connect_timeout_s,
    unsigned int heartbeat_timeout_s,
    unsigned int disconnect_timeout_s,
    BSC_HUB_EVENT_FUNC event_func,
    void *user_arg,
    BSC_HUB_FUNCTION_HANDLE *h);

/**
 * @brief bsc_hub_function_set_identity_policy() configures an opt-in,
 * disabled-by-default local authorization policy that binds a
 * Connect-Request's claimed Device UUID/VMAC to the peer certificate
 * identity presented during the TLS handshake (see
 * BSC_CERT_IDENTITY_ENTRY). When entries_num is 0 (the default), no
 * identity binding is enforced and the hub function follows the AB.7.4
 * default behavior of accepting any Connect-Request whose certificate
 * passes standard chain validation, regardless of claimed UUID/VMAC or
 * certificate SAN content. When entries_num is non-zero, a
 * Connect-Request is rejected unless its peer certificate carries a
 * "bacnet://<instance>" SAN URI matching an entry whose uuid (and vmac,
 * if vmac_required) equals the claimed values.
 *
 * @param h - hub function handle returned by bsc_hub_function_start().
 * @param entries - pointer to an array of policy entries. The caller
 *                  retains ownership; the array must remain valid for as
 *                  long as it is set on the hub function.
 * @param entries_num - number of entries in the array, or 0 to disable.
 *
 * @return BSC_SC_SUCCESS on success, BSC_SC_BAD_PARAM for invalid
 *         parameters.
 */
BACNET_STACK_EXPORT
BSC_SC_RET bsc_hub_function_set_identity_policy(
    BSC_HUB_FUNCTION_HANDLE h,
    BSC_CERT_IDENTITY_ENTRY *entries,
    size_t entries_num);

BACNET_STACK_EXPORT
void bsc_hub_function_stop(BSC_HUB_FUNCTION_HANDLE h);

BACNET_STACK_EXPORT
bool bsc_hub_function_stopped(BSC_HUB_FUNCTION_HANDLE h);

BACNET_STACK_EXPORT
bool bsc_hub_function_started(BSC_HUB_FUNCTION_HANDLE h);

#endif
