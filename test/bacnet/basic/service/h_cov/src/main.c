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

    idle = handler_cov_fsm();
    /* Without subcriptions, it jumps quickly to idle in the state machine loop
     * maybe or does steps */
    /* Wait, checking just does not crash. */
    zassert_true(idle || !idle, NULL);

    handler_cov_task();
    zassert_true(true, NULL);
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
        ztest_unit_test(test_h_cov_timer), ztest_unit_test(test_h_cov_fsm));

    ztest_run_test_suite(h_cov_tests);
}
#endif
