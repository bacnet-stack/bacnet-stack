/**
 * @file
 * @brief module for common function for BACnet/SC implementation
 * @author Kirill Neznamov <kirill.neznamov@dsr-corporation.com>
 * @date Jule 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DATALINK_BSC_UTIL_H
#define BACNET_DATALINK_BSC_UTIL_H
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/bsc/bsc-node.h"
#include "bacnet/datalink/bsc/bsc-retcodes.h"
#include "bacnet/datalink/bsc/bsc-socket.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/datalink/bsc/websocket.h"
#include "bacnet/datetime.h"

BSC_SC_RET bsc_map_websocket_retcode(BSC_WEBSOCKET_RET ret);

void bsc_copy_vmac(BACNET_SC_VMAC_ADDRESS *dst, BACNET_SC_VMAC_ADDRESS *src);
void bsc_copy_uuid(BACNET_SC_UUID *dst, BACNET_SC_UUID *src);
char *bsc_vmac_to_string(BACNET_SC_VMAC_ADDRESS *vmac);
char *bsc_uuid_to_string(BACNET_SC_UUID *uuid);
bool bsc_node_conf_fill_from_netport(
    BSC_NODE_CONF *bsc_conf, BSC_NODE_EVENT_FUNC event_func);
void bsc_node_conf_cleanup(BSC_NODE_CONF *bsc_conf);
void bsc_copy_str(char *dst, const char *src, size_t dst_len);
void bsc_set_timestamp(BACNET_DATE_TIME *timestamp);
const char *bsc_return_code_to_string(BSC_SC_RET ret);
const char *bsc_socket_event_to_string(BSC_SOCKET_EVENT ev);
const char *bsc_socket_state_to_string(BSC_SOCKET_STATE state);
const char *bsc_websocket_return_to_string(BSC_WEBSOCKET_RET ret);
const char *bsc_websocket_event_to_string(BSC_WEBSOCKET_EVENT event);
const char *bsc_bvlc_message_type_to_string(BVLC_SC_MESSAGE_TYPE message);
const char *bsc_context_state_to_string(BSC_CTX_STATE state);

#endif
