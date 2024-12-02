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
    int apdu_len = 0, null_len = 0, test_len = 0, tag_len = 0, value_len = 0;
    uint8_t tag_number = 1;
    bool status = false;

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

    /* value type = boolean */
    value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    value.type.Boolean = true;
    null_len = bacnet_audit_value_encode(NULL, &value);
    apdu_len = bacnet_audit_value_encode(apdu, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_audit_value_decode(apdu, apdu_len, &test_value);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(bacnet_audit_value_same(&value, &test_value), NULL);

    /* value type = unsigned */
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = 1234;
    null_len = bacnet_audit_value_encode(NULL, &value);
    apdu_len = bacnet_audit_value_encode(apdu, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_audit_value_decode(apdu, apdu_len, &test_value);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(bacnet_audit_value_same(&value, &test_value), NULL);

    /* value type = signed */
    value.tag = BACNET_APPLICATION_TAG_SIGNED_INT;
    value.type.Signed_Int = -1234;
    null_len = bacnet_audit_value_encode(NULL, &value);
    apdu_len = bacnet_audit_value_encode(apdu, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_audit_value_decode(apdu, apdu_len, &test_value);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(bacnet_audit_value_same(&value, &test_value), NULL);

    /* value type = REAL */
    value.tag = BACNET_APPLICATION_TAG_REAL;
    value.type.Real = 3.14159;
    null_len = bacnet_audit_value_encode(NULL, &value);
    apdu_len = bacnet_audit_value_encode(apdu, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_audit_value_decode(apdu, apdu_len, &test_value);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(bacnet_audit_value_same(&value, &test_value), NULL);

    /* value type = DOUBLE */
#if defined(BACAPP_DOUBLE)
    value.tag = BACNET_APPLICATION_TAG_DOUBLE;
    value.type.Double = 3.14159;
    null_len = bacnet_audit_value_encode(NULL, &value);
    apdu_len = bacnet_audit_value_encode(apdu, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_audit_value_decode(apdu, apdu_len, &test_value);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(bacnet_audit_value_same(&value, &test_value), NULL);
#endif

    /* value type = ENUMERATED */
    value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
    value.type.Enumerated = 1234;
    null_len = bacnet_audit_value_encode(NULL, &value);
    apdu_len = bacnet_audit_value_encode(apdu, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_audit_value_decode(apdu, apdu_len, &test_value);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(bacnet_audit_value_same(&value, &test_value), NULL);

    /* negative tests */
    null_len = bacnet_audit_value_encode(NULL, NULL);
    zassert_equal(null_len, 0, NULL);
    value.tag = 255;
    null_len = bacnet_audit_value_encode(NULL, &value);
    zassert_equal(null_len, 0, NULL);

    /* context encoded */
    value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    value.type.Boolean = true;
    null_len = bacnet_audit_value_context_encode(NULL, tag_number, &value);
    apdu_len = bacnet_audit_value_context_encode(apdu, tag_number, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = 0;
    status = bacnet_is_opening_tag_number(apdu, apdu_len, tag_number, &tag_len);
    zassert_true(status, NULL);
    test_len += tag_len;
    value_len = bacnet_audit_value_decode(&apdu[test_len], apdu_len-test_len, &test_value);
    test_len += value_len;
    status = bacnet_is_closing_tag_number(&apdu[test_len], apdu_len-test_len, tag_number, &tag_len);
    zassert_true(status, NULL);
    test_len += tag_len;
    zassert_equal(apdu_len, test_len, NULL);
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
    bool status = false;

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
    zassert_equal(test_len, apdu_len, NULL);
    status = bacnet_audit_log_record_same(&value, NULL);
    zassert_false(status, NULL);
    status = bacnet_audit_log_record_same(NULL, &value);
    zassert_false(status, NULL);
    value.tag = 255;
    test_value.tag = 255;
    status = bacnet_audit_log_record_same(&value, &test_value);
    zassert_false(status, NULL);

    /* record type = notification */
    value.tag = AUDIT_LOG_DATUM_TAG_NOTIFICATION;
    value.datum.notification.operation = AUDIT_OPERATION_DEVICE_RESET;
    value.datum.notification.source_device.tag = BACNET_RECIPIENT_TAG_DEVICE;
    value.datum.notification.source_device.type.device.instance = 1234;
    value.datum.notification.source_device.type.device.type = OBJECT_DEVICE;
    value.datum.notification.target_device.tag = BACNET_RECIPIENT_TAG_DEVICE;
    value.datum.notification.target_device.type.device.instance = 5678;
    value.datum.notification.target_device.type.device.type = OBJECT_DEVICE;
    null_len = bacnet_audit_log_record_encode(NULL, &value);
    apdu_len = bacnet_audit_log_record_encode(apdu, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_audit_log_record_decode(apdu, apdu_len, &test_value);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(bacnet_audit_log_record_same(&value, &test_value), NULL);

    /* record type = time-change */
    value.tag = AUDIT_LOG_DATUM_TAG_TIME_CHANGE;
    value.datum.time_change = 3.14159;
    null_len = bacnet_audit_log_record_encode(NULL, &value);
    apdu_len = bacnet_audit_log_record_encode(apdu, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_audit_log_record_decode(apdu, apdu_len, &test_value);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(bacnet_audit_log_record_same(&value, &test_value), NULL);
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