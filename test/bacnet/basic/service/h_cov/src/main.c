/**
 * @file
 * @brief test BACnet COV service handler
 * @date 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdef.h>
#include <bacnet/cov.h>
#include <bacnet/basic/services.h>
#include <bacnet/basic/tsm/tsm.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/* Device_ mock functions are in bacnet/basic/object/test/device_mock.c */

/* the COV FSM visits one subscription slot for each call, and the
   slots are sequenced through the MARK, CLEAR, FREE, and SEND states */
#define H_COV_FSM_CYCLE_STEPS (8 * 128)

/**
 * @brief Run the COV FSM until the confirmed notification uses invoke_id
 * @param invoke_id - the expected invoke ID of the notification
 * @return true if the notification was sent using invoke_id
 */
static bool h_cov_fsm_send_confirmed(uint8_t invoke_id)
{
    unsigned i;

    for (i = 0; i < H_COV_FSM_CYCLE_STEPS; i++) {
        handler_cov_fsm();
        if (!tsm_invoke_id_free(invoke_id)) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Subscribe for confirmed COV, send a notification with invoke_id,
 *  receive its SimpleACK, then let another transaction reuse invoke_id.
 * @param cov_data - subscription parameters
 * @param src - subscriber address
 * @param invoke_id - invoke ID to use and then reuse
 */
static void h_cov_invoke_id_reuse_setup(
    BACNET_SUBSCRIBE_COV_DATA *cov_data, BACNET_ADDRESS *src, uint8_t invoke_id)
{
    uint8_t service_request[128] = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint8_t apdu[4] = { 0 };
    int len;
    uint8_t reused_id;

    handler_cov_init();
    zassert_true(tsm_invoke_id_free(invoke_id), NULL);
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), cov_data);
    zassert_true(len > 0, NULL);
    handler_cov_subscribe(service_request, len, src, &service_data);
    /* 1. send a confirmed COV notification using invoke_id */
    tsm_invokeID_set(invoke_id);
    zassert_true(h_cov_fsm_send_confirmed(invoke_id), NULL);
    /* 2. the SimpleACK is received, and apdu_handler() frees the ID */
    tsm_free_invoke_id(invoke_id);
    zassert_true(tsm_invoke_id_free(invoke_id), NULL);
    /* 3. another transaction reuses the same invoke ID */
    tsm_invokeID_set(invoke_id);
    reused_id = tsm_next_free_invokeID();
    zassert_equal(reused_id, invoke_id, NULL);
    tsm_set_confirmed_unsegmented_transaction(
        reused_id, src, &npdu_data, apdu, sizeof(apdu));
    zassert_false(tsm_invoke_id_free(invoke_id), NULL);
    zassert_false(tsm_invoke_id_failed(invoke_id), NULL);
}

/**
 * @brief Test that a released and reused invoke ID is not freed by COV
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_invoke_id_reuse)
#else
static void test_h_cov_invoke_id_reuse(void)
#endif
{
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };
    const uint8_t invoke_id = 200;
    unsigned i;
    int len;

    src.mac_len = 1;
    src.mac[0] = 42;
    cov_data.subscriberProcessIdentifier = 99;
    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    cov_data.monitoredObjectIdentifier.instance = 1;
    cov_data.issueConfirmedNotifications = true;
    cov_data.lifetime = 60;

    /* 4a. renew the old subscription */
    cov_data.cancellationRequest = false;
    h_cov_invoke_id_reuse_setup(&cov_data, &src, invoke_id);
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);
    zassert_false(tsm_invoke_id_free(invoke_id), NULL);
    /* the COV FSM must not free or wait on the other transaction */
    for (i = 0; i < H_COV_FSM_CYCLE_STEPS; i++) {
        handler_cov_fsm();
    }
    zassert_false(tsm_invoke_id_free(invoke_id), NULL);
    zassert_false(tsm_invoke_id_failed(invoke_id), NULL);
    tsm_free_invoke_id(invoke_id);

    /* 4b. cancel the old subscription */
    h_cov_invoke_id_reuse_setup(&cov_data, &src, invoke_id);
    cov_data.cancellationRequest = true;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);
    zassert_false(tsm_invoke_id_free(invoke_id), NULL);
    tsm_free_invoke_id(invoke_id);
    cov_data.cancellationRequest = false;

    /* 4c. expire the old subscription */
    h_cov_invoke_id_reuse_setup(&cov_data, &src, invoke_id);
    handler_cov_timer_seconds(cov_data.lifetime + 1);
    zassert_false(tsm_invoke_id_free(invoke_id), NULL);
    tsm_free_invoke_id(invoke_id);

    handler_cov_init();
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(h_cov_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(h_cov_tests, ztest_unit_test(test_h_cov_invoke_id_reuse));

    ztest_run_test_suite(h_cov_tests);
}
#endif
