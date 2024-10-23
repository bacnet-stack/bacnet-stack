/**
 * @file
 * @brief Enumerations to text for BACnet Master-Slave Twisted Pair (MS/TP)
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 * @defgroup DLMSTP BACnet MS/TP DataLink Network Layer
 * @ingroup DataLink
 */
#include <stdio.h>
#include "bacnet/datalink/mstp.h"
#include "bacnet/indtext.h"
#include "bacnet/bacenum.h"
#include "bacnet/datalink/mstptext.h"

/** @file mstptext.c  Text mapping functions for BACnet MS/TP */

static INDTEXT_DATA mstp_receive_state_text[] = {
    { MSTP_RECEIVE_STATE_IDLE, "IDLE" },
    { MSTP_RECEIVE_STATE_PREAMBLE, "PREAMBLE" },
    { MSTP_RECEIVE_STATE_HEADER, "HEADER" },
    { MSTP_RECEIVE_STATE_DATA, "DATA" },
    { 0, NULL }
};

const char *mstptext_receive_state(unsigned index)
{
    return indtext_by_index_default(mstp_receive_state_text, index, "unknown");
}

static INDTEXT_DATA mstp_master_state_text[] = {
    { MSTP_MASTER_STATE_INITIALIZE, "INITIALIZE" },
    { MSTP_MASTER_STATE_IDLE, "IDLE" },
    { MSTP_MASTER_STATE_USE_TOKEN, "USE_TOKEN" },
    { MSTP_MASTER_STATE_WAIT_FOR_REPLY, "WAIT_FOR_REPLY" },
    { MSTP_MASTER_STATE_DONE_WITH_TOKEN, "DONE_WITH_TOKEN" },
    { MSTP_MASTER_STATE_PASS_TOKEN, "PASS_TOKEN" },
    { MSTP_MASTER_STATE_NO_TOKEN, "NO_TOKEN" },
    { MSTP_MASTER_STATE_POLL_FOR_MASTER, "POLL_FOR_MASTER" },
    { MSTP_MASTER_STATE_ANSWER_DATA_REQUEST, "ANSWER_DATA_REQUEST" },
    { 0, NULL }
};

const char *mstptext_master_state(unsigned index)
{
    return indtext_by_index_default(mstp_master_state_text, index, "unknown");
}

static INDTEXT_DATA mstp_frame_type_text[] = {
    { FRAME_TYPE_TOKEN, "TOKEN" },
    { FRAME_TYPE_POLL_FOR_MASTER, "POLL_FOR_MASTER" },
    { FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER, "REPLY_TO_POLL_FOR_MASTER" },
    { FRAME_TYPE_TEST_REQUEST, "TEST_REQUEST" },
    { FRAME_TYPE_TEST_RESPONSE, "TEST_RESPONSE" },
    { FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY, "BACNET_DATA_EXPECTING_REPLY" },
    { FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY,
      "BACNET_DATA_NOT_EXPECTING_REPLY" },
    { FRAME_TYPE_REPLY_POSTPONED, "REPLY_POSTPONED" },
    { FRAME_TYPE_BACNET_EXTENDED_DATA_EXPECTING_REPLY,
      "BACNET_EXTENDED_DATA_EXPECTING_REPLY" },
    { FRAME_TYPE_BACNET_EXTENDED_DATA_NOT_EXPECTING_REPLY,
      "BACNET_EXTENDED_DATA_NOT_EXPECTING_REPLY" },
    { FRAME_TYPE_IPV6_ENCAPSULATION, "IPV6_ENCAPSULATION" },
    { 0, NULL }
};

const char *mstptext_frame_type(unsigned index)
{
    return indtext_by_index_split_default(
        mstp_frame_type_text, index, FRAME_TYPE_PROPRIETARY_MIN, "UNKNOWN",
        "PROPRIETARY");
}

static INDTEXT_DATA mstp_zero_config_state_text[] = {
    { MSTP_ZERO_CONFIG_STATE_INIT, "INIT" },
    { MSTP_ZERO_CONFIG_STATE_IDLE, "IDLE" },
    { MSTP_ZERO_CONFIG_STATE_LURK, "LURK" },
    { MSTP_ZERO_CONFIG_STATE_CLAIM, "CLAIM" },
    { MSTP_ZERO_CONFIG_STATE_CONFIRM, "CONFIRM" },
    { MSTP_ZERO_CONFIG_STATE_USE, "USE" },
    { 0, NULL }
};

const char *mstptext_zero_config_state(unsigned index)
{
    return indtext_by_index_default(
        mstp_zero_config_state_text, index, "unknown");
}
