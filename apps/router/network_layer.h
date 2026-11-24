/**
 * @file
 * @author Andriy Sukhynyuk, Vasyl Tkhir, Andriy Ivasiv
 * @date 2012
 * @brief Network layer for BACnet routing
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef NETWORK_LAYER_H
#define NETWORK_LAYER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"
/* router utils */
#include "bacport.h"
#include "portthread.h"

int process_network_message(const BACMSG *msg, MSG_DATA *data, uint8_t **buff);

/* process_msg() result: *buff is Reject-Message-To-Network for the source.
 * data->pdu_len is that PDU's length and data->dest is the sender. */
#define ROUTER_MSG_REJECT (-4)

int process_msg(BACMSG *msg, MSG_DATA *data, uint8_t **buff);

bool is_network_msg(const BACMSG *msg);

int create_network_message(
    BACNET_NETWORK_MESSAGE_TYPE network_message_type,
    MSG_DATA *data,
    uint8_t **buff,
    void *val);

int send_network_message(
    BACNET_NETWORK_MESSAGE_TYPE network_message_type,
    MSG_DATA *data,
    uint8_t **buff,
    void *val);

void init_npdu(
    BACNET_NPDU_DATA *npdu_data,
    BACNET_NETWORK_MESSAGE_TYPE network_message_type,
    bool data_expecting_reply);

#endif /* end of NETWORK_LAYER_H */
