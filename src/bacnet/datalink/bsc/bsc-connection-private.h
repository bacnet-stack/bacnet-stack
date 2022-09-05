/**
 * @file
 * @brief BACNet secure connect API.
 * @author Kirill Neznamov
 * @date July 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __BSC__CONNECTION__PRIVATE__INCLUDED__
#define __BSC__CONNECTION__PRIVATE__INCLUDED__

typedef enum {
    BSC_CONN_STATE_IDLE = 0,
    BSC_CONN_STATE_AWAITING_WEBSOCKET = 1,
    BSC_CONN_STATE_AWAITING_REQUEST = 2,
    BSC_CONN_STATE_AWAITING_ACCEPT = 3,
    BSC_CONN_STATE_CONNECTED = 4,
    BSC_CONN_STATE_DISCONNECTING = 5
} BSC_CONN_STATE;


struct BSC_Connection {
    BSC_CONNECTION_CTX *ctx;
    BSC_CONNECTION *next;
    BSC_CONNECTION *last;
    BACNET_WEBSOCKET_HANDLE wh;
    BSC_CONN_STATE state;
    unsigned long time_stamp;
    BACNET_SC_VMAC_ADDRESS vmac; // VMAC address of the requesting node.
    BACNET_SC_UUID uuid;
    uint16_t message_id;
    
    // Regarding max_bvlc_len and max_npdu_len:
    // These are the datalink limits and are passed up the stack to let
    // the application layer know one of the several numbers that go into computing
    // how big an NPDU/APDU can be.

    uint16_t max_bvlc_len; // remote peer max bvlc len
    uint16_t max_npdu_len; // remote peer max npdu len

    unsigned long heartbeat_seconds_elapsed;
    uint16_t expected_connect_accept_message_id;
    uint16_t expected_disconnect_message_id;
    uint16_t expected_heartbeat_message_id;
};

struct BSC_ContextCFG
{
    BSC_CTX_TYPE type;
    BACNET_WEBSOCKET_PROTOCOL proto;
    uint16_t port;
    uint8_t *ca_cert_chain;
    size_t ca_cert_chain_size;
    uint8_t *cert_chain;
    size_t cert_chain_size;
    uint8_t *priv_key;
    size_t priv_key_size;
    BACNET_SC_VMAC_ADDRESS local_vmac;
    BACNET_SC_UUID local_uuid;
    uint16_t max_bvlc_len; // local peer max bvlc len
    uint16_t max_ndpu_len; // local peer max npdu len

    // According AB.6.2 BACnet/SC Connection Establishment and Termination
    // recommended default value for establishing of connection 10 seconds
    unsigned long connect_timeout_s;
    unsigned long disconnect_timeout_s;

    // According 12.56.Y10 SC_Heartbeat_Timeout
    // (http://www.bacnet.org/Addenda/Add-135-2020cc.pdf) the recommended default
    // value is 300 seconds.
    unsigned long heartbeat_timeout_s;
};

struct BSC_ConnectionContextFuncs {
    BSC_CONNECTION* (*find_connection_for_vmac)(BACNET_SC_VMAC_ADDRESS *vmac);
    BSC_CONNECTION* (*find_connection_for_uuid)(BACNET_SC_UUID *uuid);
};

struct BSC_ConnectionContext {
    BSC_CONNECTION *head;
    BSC_CONNECTION *tail;
    BSC_CONNECTION_CTX_FUNCS* funcs;
    BSC_CONTEXT_CFG *cfg;
};

#endif