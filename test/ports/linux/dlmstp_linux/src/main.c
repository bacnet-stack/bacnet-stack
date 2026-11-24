/* SPDX-License-Identifier: MIT */
/**
 * @file
 * @brief Regression tests for ports/linux/dlmstp.c defects:
 *  - B3: dlmstp_receive() ignored max_pdu and could overflow the
 *    caller's buffer.
 *  - B4: a master MAC address above DEFAULT_MAX_MASTER made the MS/TP
 *    thread busy-spin without ever checking for a shutdown request,
 *    so dlmstp_cleanup() blocked forever in pthread_join().
 *  - B5: a packet already queued before dlmstp_receive() was called
 *    was only returned after the full timeout elapsed.
 *  - dlmstp_send_pdu_queue_empty() and dlmstp_send_pdu_queue_full()
 *    were declared in dlmstp.h but not defined.
 */
#include <zephyr/ztest.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <bacnet/bacdef.h>
#include <bacnet/bacaddr.h>
#include <bacnet/npdu.h>
#include <bacnet/datalink/mstp.h>
#include <bacnet/datalink/dlmstp.h>

/**
 * @brief B3 - dlmstp_receive() must never copy more than max_pdu bytes
 *  into the caller's buffer. When the queued packet exceeds max_pdu it
 *  must be discarded: return 0 and leave the caller's buffer untouched,
 *  rather than silently truncating it.
 */
static void test_dlmstp_receive_respects_max_pdu(void)
{
    struct mstp_port_struct_t mstp_port = { 0 };
    uint8_t input[64];
    struct {
        uint8_t buf[8];
        uint8_t guard;
    } out;
    uint8_t expected_buf[8];
    BACNET_ADDRESS src = { 0 };
    uint16_t pdu_len;
    unsigned i;

    dlmstp_set_mac_address(10);
    zassert_true(dlmstp_init(NULL), NULL);

    for (i = 0; i < sizeof(input); i++) {
        input[i] = (uint8_t)(i + 1);
    }
    memset(&out, 0, sizeof(out));
    out.guard = 0x5A;
    memcpy(expected_buf, out.buf, sizeof(expected_buf));

    mstp_port.InputBuffer = input;
    mstp_port.DataLength = sizeof(input);
    mstp_port.SourceAddress = 5;
    zassert_true(MSTP_Put_Receive(&mstp_port) > 0, NULL);

    pdu_len = dlmstp_receive(&src, out.buf, sizeof(out.buf), 0);

    zassert_equal(
        pdu_len, 0, "oversized NPDU must be discarded, not truncated");
    zassert_equal(
        memcmp(out.buf, expected_buf, sizeof(out.buf)), 0,
        "dlmstp_receive() must not modify the caller's buffer when the "
        "queued PDU exceeds max_pdu");
    zassert_equal(
        out.guard, 0x5A, "dlmstp_receive() overran the caller's buffer");

    dlmstp_cleanup();
}

/**
 * @brief B5 - if a packet is already queued before dlmstp_receive() is
 *  called with a non-zero timeout, it must be returned immediately
 *  instead of waiting for the full timeout to elapse.
 */
static void test_dlmstp_receive_no_wait_when_already_queued(void)
{
    struct mstp_port_struct_t mstp_port = { 0 };
    uint8_t input[4] = { 1, 2, 3, 4 };
    uint8_t out[4];
    BACNET_ADDRESS src = { 0 };
    struct timespec start, end;
    double elapsed_ms;
    uint16_t pdu_len;

    dlmstp_set_mac_address(11);
    zassert_true(dlmstp_init(NULL), NULL);

    mstp_port.InputBuffer = input;
    mstp_port.DataLength = sizeof(input);
    mstp_port.SourceAddress = 6;
    zassert_true(MSTP_Put_Receive(&mstp_port) > 0, NULL);

    clock_gettime(CLOCK_MONOTONIC, &start);
    pdu_len = dlmstp_receive(&src, out, sizeof(out), 500);
    clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_ms = (double)(end.tv_sec - start.tv_sec) * 1000.0 +
        (double)(end.tv_nsec - start.tv_nsec) / 1.0e6;

    zassert_equal(pdu_len, sizeof(input), NULL);
    zassert_true(
        elapsed_ms < 100.0,
        "dlmstp_receive() waited for the timeout instead of returning "
        "the already-queued packet immediately");

    dlmstp_cleanup();
}

struct cleanup_thread_arg {
    volatile bool done;
};

static void *cleanup_thread_func(void *arg)
{
    struct cleanup_thread_arg *a = (struct cleanup_thread_arg *)arg;

    dlmstp_cleanup();
    a->done = true;

    return NULL;
}

/**
 * @brief B4 - an out-of-range master MAC address (above
 *  DEFAULT_MAX_MASTER) must not cause the MS/TP thread to busy-spin
 *  without checking for a shutdown request. dlmstp_cleanup() must
 *  return in a bounded time.
 * @note if this regresses, the cleanup thread spawned below is left
 *  stuck forever inside pthread_join(); the timed join guards this
 *  test (and the test process) from hanging.
 */
static void test_dlmstp_high_mac_cleanup_does_not_hang(void)
{
    pthread_t tid;
    struct cleanup_thread_arg arg = { false };
    struct timespec deadline;
    int rv;

    dlmstp_set_mac_address(200); /* > DEFAULT_MAX_MASTER (127) */
    zassert_true(dlmstp_init(NULL), NULL);

    rv = pthread_create(&tid, NULL, cleanup_thread_func, &arg);
    zassert_equal(rv, 0, NULL);

    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += 3;
    rv = pthread_timedjoin_np(tid, NULL, &deadline);

    zassert_equal(
        rv, 0,
        "dlmstp_cleanup() did not return within 3 seconds - the MS/TP "
        "thread is busy-spinning without checking for shutdown");
    zassert_true(arg.done, NULL);
}

/**
 * @brief dlmstp_send_pdu_queue_empty() and dlmstp_send_pdu_queue_full()
 *  must report the PDUs that dlmstp_send_pdu() has queued and that
 *  the MS/TP thread has not sent yet.
 * @note a master MAC address above DEFAULT_MAX_MASTER keeps the MS/TP
 *  thread from running the master node state machine, so the thread
 *  does not take a PDU out of the queue during the test.
 */
static void test_dlmstp_send_pdu_queue_status(void)
{
    BACNET_ADDRESS dest = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint8_t pdu[4] = { 1, 2, 3, 4 };
    uint8_t oversized_pdu[DLMSTP_MPDU_MAX + 1] = { 0 };
    unsigned queued = 0;

    dlmstp_set_mac_address(200); /* > DEFAULT_MAX_MASTER (127) */
    zassert_true(dlmstp_init(NULL), NULL);
    dest.mac_len = 1;
    dest.mac[0] = 5;

    zassert_true(dlmstp_send_pdu_queue_empty(), NULL);
    zassert_false(dlmstp_send_pdu_queue_full(), NULL);
    zassert_equal(
        dlmstp_send_pdu(
            &dest, &npdu_data, oversized_pdu, sizeof(oversized_pdu)),
        0, NULL);
    zassert_true(
        dlmstp_send_pdu_queue_empty(), "oversized PDU must not be queued");

    zassert_equal(
        dlmstp_send_pdu(&dest, &npdu_data, pdu, sizeof(pdu)), sizeof(pdu),
        NULL);
    zassert_false(dlmstp_send_pdu_queue_empty(), NULL);
    zassert_false(dlmstp_send_pdu_queue_full(), NULL);

    while ((queued < 4096) &&
           (dlmstp_send_pdu(&dest, &npdu_data, pdu, sizeof(pdu)) > 0)) {
        queued++;
    }
    zassert_true(queued > 0, NULL);
    zassert_true(queued < 4096, "the PDU queue never refused a PDU");
    zassert_false(dlmstp_send_pdu_queue_empty(), NULL);
    zassert_true(dlmstp_send_pdu_queue_full(), NULL);

    dlmstp_cleanup();
    zassert_true(dlmstp_init(NULL), NULL);
    zassert_true(
        dlmstp_send_pdu_queue_empty(),
        "dlmstp_init() must start with an empty PDU queue");
    zassert_false(dlmstp_send_pdu_queue_full(), NULL);

    dlmstp_cleanup();
}

void test_main(void)
{
    ztest_test_suite(
        dlmstp_test, ztest_unit_test(test_dlmstp_receive_respects_max_pdu),
        ztest_unit_test(test_dlmstp_receive_no_wait_when_already_queued),
        ztest_unit_test(test_dlmstp_high_mac_cleanup_does_not_hang),
        ztest_unit_test(test_dlmstp_send_pdu_queue_status));
    ztest_run_test_suite(dlmstp_test);
}
