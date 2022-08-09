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

typedef enum {
    BSC_CONN_STATE_IDLE = 0,
    BSC_CONN_STATE_AWAITING_WEBSOCKET = 1,
    BSC_CONN_STATE_AWAITING_REQUEST = 2,
    BSC_CONN_STATE_AWAITING_ACCEPT = 3,
    BSC_CONN_STATE_CONNECTED = 4,
    BSC_CONN_STATE_DISCONNECTING = 5
} BSC_CONN_STATE;

typedef enum {
    BSC_PEER_INITIATOR = 1,
    BSC_PEER_ACCEPTOR = 2
} BSC_CONN_PEER_TYPE;

struct BSC_Connection {
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

    BSC_CONN_PEER_TYPE peer_type;
    unsigned long heartbeat_seconds_elapsed;
    uint16_t expected_connect_accept_message_id;
    uint16_t expected_disconnect_message_id;
    uint16_t expected_heartbeat_message_id;
};