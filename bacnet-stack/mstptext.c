#include <stdio.h>
#include "mstp.h"
#include "indtext.h"
#include "bacenum.h"

static INDTEXT_DATA mstp_receive_state_text[] = {
    {MSTP_RECEIVE_STATE_IDLE, "IDLE"},
    {MSTP_RECEIVE_STATE_PREAMBLE, "PREAMBLE"},
    {MSTP_RECEIVE_STATE_HEADER, "HEADER"},
    {MSTP_RECEIVE_STATE_HEADER_CRC, "HEADER_CRC"},
    {MSTP_RECEIVE_STATE_DATA, "DATA"},
    {0, NULL}
};

const char *mstptext_receive_state(int index)
{
    return indtext_by_index_default(mstp_receive_state_text,
        index, "unknown");
}

static INDTEXT_DATA mstp_master_state_text[] = {
    {MSTP_MASTER_STATE_INITIALIZE, "INITIALIZE"},
    {MSTP_MASTER_STATE_IDLE, "IDLE"},
    {MSTP_MASTER_STATE_USE_TOKEN, "USE_TOKEN"},
    {MSTP_MASTER_STATE_WAIT_FOR_REPLY, "WAIT_FOR_REPLY"},
    {MSTP_MASTER_STATE_DONE_WITH_TOKEN, "DONE_WITH_TOKEN"},
    {MSTP_MASTER_STATE_PASS_TOKEN, "PASS_TOKEN"},
    {MSTP_MASTER_STATE_NO_TOKEN, "NO_TOKEN"},
    {MSTP_MASTER_STATE_POLL_FOR_MASTER, "POLL_FOR_MASTER"},
    {MSTP_MASTER_STATE_ANSWER_DATA_REQUEST, "ANSWER_DATA_REQUEST"},
    {0, NULL}
};

const char *mstptext_master_state(int index)
{
    return indtext_by_index_default(mstp_master_state_text,
        index, "unknown");
}

static INDTEXT_DATA mstp_frame_type_text[] = {
    {FRAME_TYPE_TOKEN, "TOKEN"},
    {FRAME_TYPE_POLL_FOR_MASTER, "POLL_FOR_MASTER"},
    {FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER, "REPLY_TO_POLL_FOR_MASTER"},
    {FRAME_TYPE_TEST_REQUEST, "TEST_REQUEST"},
    {FRAME_TYPE_TEST_RESPONSE, "TEST_RESPONSE"},
    {FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY, "BACNET_DATA_EXPECTING_REPLY"},
    {FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY, "BACNET_DATA_NOT_EXPECTING_REPLY"},
    {FRAME_TYPE_REPLY_POSTPONED, "REPLY_POSTPONED"},
    {0, NULL}
};

const char *mstptext_frame_type(int index)
{
    return indtext_by_index_split_default(mstp_frame_type_text,
        index,
        FRAME_TYPE_PROPRIETARY_MIN,
        "UNKNOWN", "PROPRIETARY");
}
