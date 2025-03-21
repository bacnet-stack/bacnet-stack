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
#include <math.h>
#include <zephyr/ztest.h>
#include <bacnet/baclog.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

static void test_bacnet_log_record_datum(BACNET_LOG_RECORD *value)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_LOG_RECORD test_value = { 0 };
    int apdu_len = 0, null_len = 0, test_len = 0;

    value->status_flags = 0;
    null_len = bacnet_log_record_encode(NULL, sizeof(apdu), value);
    apdu_len = bacnet_log_record_encode(apdu, sizeof(apdu), value);
    zassert_equal(apdu_len, null_len, NULL);
    null_len = bacnet_log_record_decode(apdu, apdu_len, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_log_record_decode(apdu, apdu_len, &test_value);
    zassert_equal(apdu_len, test_len, "datum=%u", value->tag);
    zassert_true(bacnet_log_record_same(value, &test_value), NULL);

    value->status_flags = 0x0F;
    null_len = bacnet_log_record_encode(NULL, sizeof(apdu), value);
    apdu_len = bacnet_log_record_encode(apdu, sizeof(apdu), value);
    zassert_equal(apdu_len, null_len, NULL);
    null_len = bacnet_log_record_decode(apdu, apdu_len, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_log_record_decode(apdu, apdu_len, &test_value);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(bacnet_log_record_same(value, &test_value), NULL);

    BIT_SET(value->status_flags, 7);
    null_len = bacnet_log_record_encode(NULL, sizeof(apdu), value);
    apdu_len = bacnet_log_record_encode(apdu, sizeof(apdu), value);
    zassert_equal(apdu_len, null_len, NULL);
    null_len = bacnet_log_record_decode(apdu, apdu_len, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_log_record_decode(apdu, apdu_len, &test_value);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(bacnet_log_record_same(value, &test_value), NULL);

    /* decoding, some negative tests */
    test_len = bacnet_log_record_decode(NULL, apdu_len, &test_value);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    test_len = bacnet_log_record_decode(apdu, 0, &test_value);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
}

uint8_t Test_APDU[MAX_APDU];
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_log_tests, test_bacnet_log_record)
#else
static void test_bacnet_log_record(void)
#endif
{
    BACNET_LOG_RECORD value = { 0 }, test_value = { 0 };
    bool status = false;

    /* common */
    datetime_init_ascii(&value.timestamp, "2021/12/31-23:59:59.99");

    /* value type = null */
    status = bacnet_log_record_datum_from_ascii(&value, "null");
    zassert_true(status, NULL);
    zassert_equal(value.tag, BACNET_LOG_DATUM_NULL, NULL);
    test_bacnet_log_record_datum(&value);

    /* value type = boolean */
    status = bacnet_log_record_datum_from_ascii(&value, "true");
    zassert_true(status, NULL);
    zassert_equal(value.tag, BACNET_LOG_DATUM_BOOLEAN, NULL);
    zassert_equal(value.log_datum.boolean_value, true, NULL);
    test_bacnet_log_record_datum(&value);

    /* value type = unsigned */
    status = bacnet_log_record_datum_from_ascii(&value, "1234");
    zassert_true(status, NULL);
    zassert_equal(value.tag, BACNET_LOG_DATUM_UNSIGNED, NULL);
    zassert_equal(value.log_datum.unsigned_value, 1234, NULL);
    test_bacnet_log_record_datum(&value);

    /* value type = signed */
    status = bacnet_log_record_datum_from_ascii(&value, "-1234");
    zassert_true(status, NULL);
    zassert_equal(value.tag, BACNET_LOG_DATUM_SIGNED, NULL);
    zassert_equal(value.log_datum.integer_value, -1234, NULL);
    test_bacnet_log_record_datum(&value);

    /* value type = REAL */
    status = bacnet_log_record_datum_from_ascii(&value, "3.14159");
    zassert_true(status, NULL);
    zassert_equal(value.tag, BACNET_LOG_DATUM_REAL, NULL);
    status = islessgreater(value.log_datum.real_value, 3.14159f);
    zassert_false(status, NULL);
    test_bacnet_log_record_datum(&value);

    /* value type = ENUMERATED */
    value.tag = BACNET_LOG_DATUM_ENUMERATED;
    value.log_datum.enumerated_value = 1234;
    test_bacnet_log_record_datum(&value);

    status = bacnet_log_record_same(&value, NULL);
    zassert_false(status, NULL);
    status = bacnet_log_record_same(NULL, &value);
    zassert_false(status, NULL);
    value.tag = 255;
    test_value.tag = 255;
    status = bacnet_log_record_same(&value, &test_value);
    zassert_false(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacnet_log_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(bacnet_log_tests, ztest_unit_test(test_bacnet_log_record));

    ztest_run_test_suite(bacnet_log_tests);
}
#endif
