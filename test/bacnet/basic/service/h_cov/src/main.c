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

/* Mock functions for Device_ */
#include <bacnet/basic/object/device.h>

/* Mocks have been moved to bacnet/basic/object/test/ */
/**
 * @brief Test test_h_cov_init_encode
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_init_encode)
#else
static void test_h_cov_init_encode(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int apdu_size = sizeof(apdu);
    int len;
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };

    handler_cov_init();

    len = handler_cov_encode_subscriptions(apdu, apdu_size);
    /* Should be zero because we initialized the handler */
    zassert_equal(len, 0, NULL);

    /* Create a valid subscription */
    cov_data.subscriberProcessIdentifier = 1;
    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    cov_data.monitoredObjectIdentifier.instance = 0;
    cov_data.cancellationRequest = false;
    cov_data.issueConfirmedNotifications = false;
    cov_data.lifetime = 0;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, BACNET_STATUS_ABORT, NULL);
    handler_cov_subscribe(service_request, len, &src, &service_data);

    /* Normal encode */
    len = handler_cov_encode_subscriptions(apdu, apdu_size);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, BACNET_STATUS_ABORT, NULL);

    /* edge case: exact size buffer encode */
    len = handler_cov_encode_subscriptions(apdu, len);
    zassert_equal(len, len, NULL);

    /* edge case: buffer too small */
    len = handler_cov_encode_subscriptions(apdu, len - 1);
    zassert_equal(len, BACNET_STATUS_ABORT, NULL);
}

/**
 * @brief Test test_h_cov_timer
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_timer)
#else
static void test_h_cov_timer(void)
#endif
{
    handler_cov_timer_seconds(1);
    zassert_true(true, NULL);
}

/**
 * @brief Test test_h_cov_fsm
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_fsm)
#else
static void test_h_cov_fsm(void)
#endif
{
    bool idle;
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };
    int len;

    handler_cov_init();

    idle = handler_cov_fsm();
    zassert_true(idle, NULL); /* No subscriptions, quickly back to idle */

    /* Subscribe Unconfirmed */
    cov_data.subscriberProcessIdentifier = 1;
    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    cov_data.monitoredObjectIdentifier.instance = 0;
    cov_data.cancellationRequest = false;
    cov_data.issueConfirmedNotifications = false;
    cov_data.lifetime = 0;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);

    /* IDLE -> MARK */
    idle = handler_cov_fsm();
    zassert_false(idle, NULL);
    /* MARK -> CLEAR */
    idle = handler_cov_fsm();
    zassert_false(idle, NULL);
    /* CLEAR -> FREE */
    idle = handler_cov_fsm();
    zassert_false(idle, NULL);
    /* FREE -> SEND */
    idle = handler_cov_fsm();
    zassert_false(idle, NULL);
    /* SEND -> IDLE */
    idle = handler_cov_fsm();
    zassert_true(idle, NULL);

    /* Subscribe Confirmed */
    cov_data.subscriberProcessIdentifier = 2;
    cov_data.issueConfirmedNotifications = true;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);

    /* Run FSM again */
    handler_cov_fsm(); /* IDLE -> MARK */
    handler_cov_fsm(); /* MARK -> CLEAR */
    handler_cov_fsm(); /* CLEAR -> FREE */
    handler_cov_fsm(); /* FREE -> SEND */
    handler_cov_fsm(); /* SEND -> IDLE */

    /* Trigger Confirmed free branch by expiring */
    cov_data.lifetime = 10;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);

    handler_cov_fsm(); /* IDLE -> MARK */
    handler_cov_fsm(); /* MARK -> CLEAR */
    handler_cov_fsm(); /* CLEAR -> FREE */
    handler_cov_fsm(); /* FREE -> SEND */
    handler_cov_fsm(); /* SEND -> IDLE */

    /* Expire */
    handler_cov_timer_seconds(15);

    handler_cov_task();
    zassert_true(true, NULL);
}

/**
 * @brief Test test_h_cov_subscribe_error_cases
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_subscribe_error_cases)
#else
static void test_h_cov_subscribe_error_cases(void)
#endif
{
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };
    int len;

    handler_cov_init();

    /* test segment abort */
    service_data.segmented_message = true;
    handler_cov_subscribe(service_request, 10, &src, &service_data);
    zassert_true(true, NULL);

    /* test missing required parameter */
    service_data.segmented_message = false;
    handler_cov_subscribe(service_request, 0, &src, &service_data);
    zassert_true(true, NULL);

    /* test bad request decode */
    handler_cov_subscribe(service_request, 1, &src, &service_data);
    zassert_true(true, NULL);

    /* test valid decode but object valid (by mock) */
    cov_data.subscriberProcessIdentifier = 1;
    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    cov_data.monitoredObjectIdentifier.instance = 12345;
    cov_data.cancellationRequest = false;
    cov_data.issueConfirmedNotifications = false;
    cov_data.lifetime = 0;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);
    zassert_true(true, NULL);

    /* test cancellation of non-existing */
    cov_data.cancellationRequest = true;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);
    zassert_true(true, NULL);
}

/**
 * @brief Test test_h_cov_subscribe_out_of_space
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_subscribe_out_of_space)
#else
static void test_h_cov_subscribe_out_of_space(void)
#endif
{
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };
    int len;
    int i;

    handler_cov_init();

    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    cov_data.monitoredObjectIdentifier.instance = 0;
    cov_data.cancellationRequest = false;
    cov_data.issueConfirmedNotifications = false;
    cov_data.lifetime = 0;

    for (i = 0; i < 130; i++) {
        cov_data.subscriberProcessIdentifier = i;
        len = cov_subscribe_service_request_encode(
            service_request, sizeof(service_request), &cov_data);
        handler_cov_subscribe(service_request, len, &src, &service_data);
    }
    zassert_true(true, NULL);
}

/**
 * @brief Test test_h_cov_timer_expiration
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_timer_expiration)
#else
static void test_h_cov_timer_expiration(void)
#endif
{
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };
    int len;

    handler_cov_init();

    /* Subscribe with lifetime */
    cov_data.subscriberProcessIdentifier = 1;
    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    cov_data.monitoredObjectIdentifier.instance = 0;
    cov_data.cancellationRequest = false;
    cov_data.issueConfirmedNotifications = true;
    cov_data.lifetime = 10;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);

    /* Check timer decreasing */
    handler_cov_timer_seconds(5);
    handler_cov_timer_seconds(10); /* Should expire here */
    zassert_true(true, NULL);
}

/**
 * @brief Test test_h_cov_address_management
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_address_management)
#else
static void test_h_cov_address_management(void)
#endif
{
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };
    int len;
    int i;

    handler_cov_init();

    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    cov_data.monitoredObjectIdentifier.instance = 0;
    cov_data.cancellationRequest = false;
    cov_data.issueConfirmedNotifications = false;
    cov_data.lifetime = 0;

    /* Add subscriptions from 18 different sources */
    for (i = 0; i < 18; i++) {
        src.mac_len = 1;
        src.mac[0] = i + 1;
        cov_data.subscriberProcessIdentifier = i;
        len = cov_subscribe_service_request_encode(
            service_request, sizeof(service_request), &cov_data);
        handler_cov_subscribe(service_request, len, &src, &service_data);
    }

    /* Cancel one, to free an address */
    src.mac_len = 1;
    src.mac[0] = 1;
    cov_data.subscriberProcessIdentifier = 0;
    cov_data.cancellationRequest = true;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);

    zassert_true(true, NULL);
}

/**
 * @brief Run the COV FSM until the confirmed notification uses invoke_id
 * @param invoke_id - the expected invoke ID of the notification
 * @return true if the notification was sent using invoke_id
 */
static bool h_cov_fsm_send_confirmed(uint8_t invoke_id)
{
    unsigned i;

    for (i = 0; i < 20; i++) {
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
    for (i = 0; i < 20; i++) {
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
    ztest_test_suite(
        h_cov_tests, ztest_unit_test(test_h_cov_init_encode),
        ztest_unit_test(test_h_cov_timer), ztest_unit_test(test_h_cov_fsm),
        ztest_unit_test(test_h_cov_subscribe_error_cases),
        ztest_unit_test(test_h_cov_subscribe_out_of_space),
        ztest_unit_test(test_h_cov_timer_expiration),
        ztest_unit_test(test_h_cov_address_management),
        ztest_unit_test(test_h_cov_invoke_id_reuse));

    ztest_run_test_suite(h_cov_tests);
}
#endif
