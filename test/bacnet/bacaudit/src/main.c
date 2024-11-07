/**
 * @file
 * @brief Unit test for for BACnetAuditNotification and BACnetAuditLogRecord
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date November 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <bacnet/bacaudit.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_audit_tests, test_bacnet_audit_value)
#else
static void test_bacnet_audit_value(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_AUDIT_VALUE value = { 0 }, test_value = { 0 };
    int apdu_len = 0, null_len = 0, test_len = 0;

    null_len = bacnet_audit_value_encode(NULL, &value);
    apdu_len = bacnet_audit_value_encode(apdu, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_audit_value_decode(apdu, apdu_len, &value);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(bacnet_audit_value_same(&value, &test_value), NULL);

    /* decoding, some negative tests */
    test_len = bacnet_audit_value_decode(NULL, apdu_len, &test_value);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    test_len = bacnet_audit_value_decode(apdu, 0, &test_value);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    test_len = bacnet_audit_value_decode(apdu, apdu_len, NULL);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_audit_tests, test_bacnet_audit_log_notification)
#else
static void test_bacnet_audit_log_notification(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_AUDIT_NOTIFICATION value = { 0 }, test_value = { 0 };
    int apdu_len = 0, null_len = 0, test_len = 0;

    null_len = bacnet_audit_log_notification_encode(NULL, &value);
    apdu_len = bacnet_audit_log_notification_encode(apdu, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_audit_log_notification_decode(apdu, apdu_len, &value);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(bacnet_audit_log_notification_same(&value, &test_value), NULL);

    /* decoding, some negative tests */
    test_len =
        bacnet_audit_log_notification_decode(NULL, apdu_len, &test_value);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    test_len = bacnet_audit_log_notification_decode(apdu, 0, &test_value);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    test_len = bacnet_audit_log_notification_decode(apdu, apdu_len, NULL);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_audit_tests, test_bacnet_audit_log_record)
#else
static void test_bacnet_audit_log_record(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_AUDIT_LOG_RECORD value = { 0 }, test_value = { 0 };
    int apdu_len = 0, null_len = 0, test_len = 0;

    value.tag = AUDIT_LOG_DATUM_TAG_STATUS;
    null_len = bacnet_audit_log_record_encode(NULL, &value);
    apdu_len = bacnet_audit_log_record_encode(apdu, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_audit_log_record_decode(apdu, apdu_len, &value);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(bacnet_audit_log_record_same(&value, &test_value), NULL);

    /* decoding, some negative tests */
    test_len = bacnet_audit_log_record_decode(NULL, apdu_len, &test_value);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    test_len = bacnet_audit_log_record_decode(apdu, 0, &test_value);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    test_len = bacnet_audit_log_record_decode(apdu, apdu_len, NULL);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacnet_audit_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bacnet_audit_tests, ztest_unit_test(test_bacnet_audit_log_record),
        ztest_unit_test(test_bacnet_audit_log_notification),
        ztest_unit_test(test_bacnet_audit_value));

    ztest_run_test_suite(bacnet_audit_tests);
}
#endif
