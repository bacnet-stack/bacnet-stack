/**
 * @file
 * @brief Tests for the router System V message queue ABI
 *
 * SPDX-License-Identifier: MIT
 */
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/msg.h>

#include "msgqueue.h"

#define SYSV_MESSAGE_OVERFLOW_BYTES \
    ((sizeof(long) > sizeof(MSGTYPE)) ? (sizeof(long) - sizeof(MSGTYPE)) : 1U)

typedef struct {
    long type;
    MSGBOX_ID origin;
    MSGSUBTYPE subtype;
    void *data;
} SYSV_MESSAGE;

typedef struct {
    SYSV_MESSAGE message;
    uint8_t trailing[SYSV_MESSAGE_OVERFLOW_BYTES];
} OVERSIZED_SYSV_MESSAGE;

typedef struct {
    BACMSG message;
    uint8_t canary[SYSV_MESSAGE_OVERFLOW_BYTES];
} GUARDED_BACMSG;

static size_t sysv_payload_size(void)
{
    return sizeof(SYSV_MESSAGE) - sizeof(long);
}

static void test_sysv_layout(void)
{
    BACMSG message = { 0 };

    assert(offsetof(BACMSG, type) == 0);
    assert(sizeof(message.type) == sizeof(long));
}

static void test_wrapper_send_to_raw_receive(void)
{
    MSGBOX_ID queue = create_msgbox();
    BACMSG sent = { 0 };
    SYSV_MESSAGE received = { 0 };
    ssize_t received_size;

    assert(queue != INVALID_MSGBOX_ID);
    sent.type = DATA;
    sent.origin = 42;
    sent.subtype = CHG_IP;
    sent.data = &sent;

    assert(send_to_msgbox(queue, &sent));
    received_size =
        msgrcv(queue, &received, sysv_payload_size(), 0, IPC_NOWAIT);
    assert(received_size == (ssize_t)sysv_payload_size());
    assert(received.type == DATA);
    assert(received.origin == sent.origin);
    assert(received.subtype == sent.subtype);
    assert(received.data == sent.data);

    del_msgbox(queue);
}

static void test_raw_send_to_wrapper_receive(void)
{
    MSGBOX_ID queue = create_msgbox();
    SYSV_MESSAGE sent = { 0 };
    BACMSG received = { 0 };

    assert(queue != INVALID_MSGBOX_ID);
    sent.type = SERVICE;
    sent.origin = 84;
    sent.subtype = CHG_MAC;
    sent.data = &sent;

    assert(msgsnd(queue, &sent, sysv_payload_size(), 0) == 0);
    assert(recv_from_msgbox(queue, &received, IPC_NOWAIT) == &received);
    assert(received.type == SERVICE);
    assert(received.origin == sent.origin);
    assert(received.subtype == sent.subtype);
    assert(received.data == sent.data);

    del_msgbox(queue);
}

static void test_oversized_raw_message_is_not_received(void)
{
    if (sizeof(long) <= sizeof(MSGTYPE)) {
        return;
    }

    MSGBOX_ID queue = create_msgbox();
    OVERSIZED_SYSV_MESSAGE sent = { 0 };
    GUARDED_BACMSG guarded = { 0 };

    assert(queue != INVALID_MSGBOX_ID);
    sent.message.type = SERVICE;
    sent.message.origin = 126;
    sent.message.subtype = CHG_MAC;
    sent.message.data = &sent;
    memset(sent.trailing, 0x5A, sizeof(sent.trailing));
    memset(guarded.canary, 0xA5, sizeof(guarded.canary));

    assert(
        msgsnd(queue, &sent, sysv_payload_size() + sizeof(sent.trailing), 0) ==
        0);
    errno = 0;
    assert(recv_from_msgbox(queue, &guarded.message, IPC_NOWAIT) == NULL);
    assert(errno == E2BIG);
    for (size_t i = 0; i < sizeof(guarded.canary); i++) {
        assert(guarded.canary[i] == 0xA5);
    }

    del_msgbox(queue);
}

int main(void)
{
    test_wrapper_send_to_raw_receive();
    test_raw_send_to_wrapper_receive();
    test_oversized_raw_message_is_not_received();
    test_sysv_layout();

    printf("router msgqueue SysV ABI tests passed\n");
    return 0;
}
