/**
 * @file
 * @brief Unit test for the linked-list BACnetDailySchedule codec
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/ztest.h>
#include "bacnet/dailyschedule.h"

typedef struct BACnetDailyScheduleStoreContext {
    BACNET_TIME_VALUE entries[4];
    size_t count;
} BACNET_DAILY_SCHEDULE_STORE_CONTEXT;

static bool
bacnet_dailyschedule_store_entry(const BACNET_TIME_VALUE *time_value, void *ctx)
{
    BACNET_DAILY_SCHEDULE_STORE_CONTEXT *store = ctx;

    if (!time_value || !store) {
        return false;
    }
    if (store->count >= (sizeof(store->entries) / sizeof(store->entries[0]))) {
        return false;
    }
    store->entries[store->count] = *time_value;
    store->count++;

    return true;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetDailySchedule_tests, test_BACnetDailyScheduleListEncodeDecode)
#else
static void test_BACnetDailyScheduleListEncodeDecode(void)
#endif
{
    BACNET_DAILY_SCHEDULE_ENTRY in[3] = { 0 };
    BACNET_DAILY_SCHEDULE_STORE_CONTEXT out = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t tag_number = 0;
    bool status = false;

    in[0].Time_Value.Time = (BACNET_TIME) { .hour = 5, .min = 30 };
    in[0].Time_Value.Value = (BACNET_PRIMITIVE_DATA_VALUE) {
        .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
        .type.Unsigned_Int = 123,
    };
    in[1].Time_Value.Time = (BACNET_TIME) { .hour = 15, .min = 0 };
    in[1].Time_Value.Value = (BACNET_PRIMITIVE_DATA_VALUE) {
        .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
        .type.Unsigned_Int = 456,
    };
    in[2].Time_Value.Time = (BACNET_TIME) { .hour = 23, .min = 59 };
    in[2].Time_Value.Value = (BACNET_PRIMITIVE_DATA_VALUE) {
        .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
        .type.Unsigned_Int = 789,
    };
    in[0].next = &in[1];
    in[1].next = &in[2];

    /* NULL apdu returns length only */
    len = bacnet_dailyschedule_list_context_encode(NULL, tag_number, &in[0]);
    zassert_true(len > 0, NULL);

    apdu_len =
        bacnet_dailyschedule_list_context_encode(apdu, tag_number, &in[0]);
    zassert_equal(apdu_len, len, NULL);

    /* decode without a store_fn just measures the length */
    len = bacnet_dailyschedule_list_context_decode(
        apdu, apdu_len, tag_number, NULL, NULL);
    zassert_equal(len, apdu_len, NULL);

    len = bacnet_dailyschedule_list_context_decode(
        apdu, apdu_len, tag_number, bacnet_dailyschedule_store_entry, &out);
    zassert_equal(len, apdu_len, NULL);
    zassert_equal(out.count, 3, NULL);
    zassert_true(
        bacnet_time_value_same(&in[0].Time_Value, &out.entries[0]), NULL);
    zassert_true(
        bacnet_time_value_same(&in[1].Time_Value, &out.entries[1]), NULL);
    zassert_true(
        bacnet_time_value_same(&in[2].Time_Value, &out.entries[2]), NULL);

    /* negative testing - the tag differs */
    len = bacnet_dailyschedule_list_context_decode(
        apdu, apdu_len, tag_number + 1, NULL, NULL);
    zassert_true(len < 0, NULL);

    /* empty list still encodes/decodes as an empty sequence */
    apdu_len = bacnet_dailyschedule_list_context_encode(apdu, tag_number, NULL);
    zassert_true(apdu_len > 0, NULL);
    memset(&out, 0, sizeof(out));
    len = bacnet_dailyschedule_list_context_decode(
        apdu, apdu_len, tag_number, bacnet_dailyschedule_store_entry, &out);
    zassert_equal(len, apdu_len, NULL);
    zassert_equal(out.count, 0, NULL);

    /* list-wide comparison */
    status = bacnet_dailyschedule_list_same(&in[0], &in[0]);
    zassert_true(status, NULL);
    status = bacnet_dailyschedule_list_same(&in[0], &in[1]);
    zassert_false(status, NULL);
    status = bacnet_dailyschedule_list_same(NULL, NULL);
    zassert_true(status, NULL);
    status = bacnet_dailyschedule_list_same(&in[0], NULL);
    zassert_false(status, NULL);
}

/**
 * @brief Test the fixed-array BACnetDailySchedule context encode/decode API,
 *  including a NULL destination which shall return the same length as a
 *  non-NULL decode.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetDailySchedule_tests, test_BACnetDailyScheduleArrayEncodeDecode)
#else
static void test_BACnetDailyScheduleArrayEncodeDecode(void)
#endif
{
    BACNET_DAILY_SCHEDULE in = { 0 };
    BACNET_DAILY_SCHEDULE out = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    uint8_t tag_number = 2;
    int len, null_len, test_len;

    in.TV_Count = 2;
    in.Time_Values[0].Time = (BACNET_TIME) { .hour = 12, .min = 30 };
    in.Time_Values[0].Value = (BACNET_PRIMITIVE_DATA_VALUE) {
        .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
        .type.Unsigned_Int = 15,
    };
    in.Time_Values[1].Time = (BACNET_TIME) { .hour = 16, .min = 1 };
    in.Time_Values[1].Value = (BACNET_PRIMITIVE_DATA_VALUE) {
        .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
        .type.Unsigned_Int = 0,
    };

    len = bacnet_dailyschedule_context_encode(apdu, tag_number, &in);
    zassert_true(len > 0, NULL);

    /* NULL day only measures the length */
    null_len = bacnet_dailyschedule_context_decode(apdu, len, tag_number, NULL);
    zassert_equal(len, null_len, NULL);

    test_len = bacnet_dailyschedule_context_decode(apdu, len, tag_number, &out);
    zassert_equal(len, test_len, NULL);
    zassert_true(bacnet_dailyschedule_same(&in, &out), NULL);

    /* NULL apdu is rejected */
    zassert_equal(
        bacnet_dailyschedule_context_decode(NULL, len, tag_number, &out),
        BACNET_STATUS_ERROR, NULL);
}

/**
 * @brief A list longer than the caller's bounded store shall abort the
 *  decode with BACNET_STATUS_ERROR, and store_fn shall never be called
 *  beyond the store's capacity.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(
    BACnetDailySchedule_tests, test_BACnetDailyScheduleListDecodeStoreOverflow)
#else
static void test_BACnetDailyScheduleListDecodeStoreOverflow(void)
#endif
{
    /* one more entry than BACNET_DAILY_SCHEDULE_STORE_CONTEXT can hold */
    BACNET_DAILY_SCHEDULE_ENTRY in[5] = { 0 };
    BACNET_DAILY_SCHEDULE_STORE_CONTEXT out = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    int apdu_len, len;
    uint8_t tag_number = 0;
    unsigned i;

    for (i = 0; i < ARRAY_SIZE(in); i++) {
        in[i].Time_Value.Time = (BACNET_TIME) { .hour = (uint8_t)i, .min = 0 };
        in[i].Time_Value.Value = (BACNET_PRIMITIVE_DATA_VALUE) {
            .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
            .type.Unsigned_Int = i,
        };
        if (i > 0) {
            in[i - 1].next = &in[i];
        }
    }
    apdu_len =
        bacnet_dailyschedule_list_context_encode(apdu, tag_number, &in[0]);
    zassert_true(apdu_len > 0, NULL);

    len = bacnet_dailyschedule_list_context_decode(
        apdu, apdu_len, tag_number, bacnet_dailyschedule_store_entry, &out);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    /* store_fn stopped at capacity - no overrun into adjacent memory */
    zassert_equal(
        out.count, (sizeof(out.entries) / sizeof(out.entries[0])), NULL);
    for (i = 0; i < out.count; i++) {
        zassert_true(
            bacnet_time_value_same(&in[i].Time_Value, &out.entries[i]), NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(BACnetDailySchedule_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        BACnetDailySchedule_tests,
        ztest_unit_test(test_BACnetDailyScheduleListEncodeDecode),
        ztest_unit_test(test_BACnetDailyScheduleArrayEncodeDecode),
        ztest_unit_test(test_BACnetDailyScheduleListDecodeStoreOverflow));

    ztest_run_test_suite(BACnetDailySchedule_tests);
}
#endif
