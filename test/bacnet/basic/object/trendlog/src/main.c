/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/trendlog.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void test_Trend_Log_ReadProperty(void)
{
    unsigned count = 0;
    uint32_t object_instance = 0;
    bool status = false;
    const int32_t known_fail_property_list[] = { -1 };

    Trend_Log_Init();
    count = Trend_Log_Count();
    zassert_true(count > 0, NULL);
    object_instance = Trend_Log_Index_To_Instance(0);
    status = Trend_Log_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_TRENDLOG, object_instance, Trend_Log_Property_Lists,
        Trend_Log_Read_Property, Trend_Log_Write_Property,
        known_fail_property_list);
}

static void test_Trend_Log_ReadRange_BySequence(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_READ_RANGE_DATA pRequest = { 0 };
    int len = 0;
    uint32_t object_instance = 0;
    int log_index = 0;
    uint32_t i;
    uint32_t total_records = 0;
    uint32_t first_seq = 0;

    Trend_Log_Init();
    object_instance = Trend_Log_Index_To_Instance(0);
    log_index = Trend_Log_Instance_To_Index(object_instance);

    /* Populate the log with some entries to make sure it's not empty and we
     * know the state */
    for (i = 0; i < 100; i++) {
        TL_Insert_Status_Rec(log_index, LOG_STATUS_LOG_INTERRUPTED, true);
    }

    /* Get the current state of the log */
    total_records = Trend_Log_Total_Record_Count(object_instance);
    zassert_true(total_records > 0, "Failed to get total record count");
    /* The first sequence number currently in the log buffer */
    first_seq = total_records - Trend_Log_Record_Count(object_instance) + 1;
    zassert_true(first_seq > 0, "Failed to get first sequence number");

    /* Verify BY_SEQUENCE with positive count within range */
    pRequest.object_type = OBJECT_TRENDLOG;
    pRequest.object_instance = object_instance;
    pRequest.object_property = PROP_LOG_BUFFER;
    pRequest.array_index = BACNET_ARRAY_ALL;
    pRequest.RequestType = RR_BY_SEQUENCE;
    pRequest.Range.RefSeqNum = first_seq;
    pRequest.Count = 10;
    pRequest.Overhead = 0;

    len = rr_trend_log_encode(apdu, &pRequest);
    zassert_true(len > 0, "Encoding failed for positive count");
    zassert_equal(pRequest.ItemCount, 10, "Expected 10 items");
    zassert_equal(
        pRequest.FirstSequence, first_seq, "Expected FirstSequence %u, got %u",
        (unsigned)first_seq, (unsigned)pRequest.FirstSequence);

    /* Verify BY_SEQUENCE with positive count and truncation at end of log */
    pRequest.Range.RefSeqNum = total_records - 5;
    pRequest.Count = 10;
    len = rr_trend_log_encode(apdu, &pRequest);
    zassert_true(len > 0, "Encoding failed");
    zassert_equal(pRequest.ItemCount, 6, "Expected 6 items (truncated at end)");
    zassert_equal(pRequest.FirstSequence, total_records - 5, "Expected FirstSequence %u, got %u",
        (unsigned)(total_records - 5), (unsigned)pRequest.FirstSequence);
    zassert_true(bitstring_bit(&pRequest.ResultFlags, RESULT_FLAG_LAST_ITEM), "Expected LAST_ITEM flag");

    /* Verify BY_SEQUENCE with negative count and truncation at beginning of log */
    pRequest.Range.RefSeqNum = first_seq + 5;
    pRequest.Count = -10;
    len = rr_trend_log_encode(apdu, &pRequest);
    zassert_true(len > 0, "Encoding failed");
    zassert_equal(pRequest.ItemCount, 6, "Expected 6 items (truncated at start)");
    zassert_equal(pRequest.FirstSequence, first_seq, "Expected FirstSequence %u (start of log), got %u",
        (unsigned)first_seq, (unsigned)pRequest.FirstSequence);
    zassert_true(bitstring_bit(&pRequest.ResultFlags, RESULT_FLAG_FIRST_ITEM), "Expected FIRST_ITEM flag");

    /* Verify BY_SEQUENCE with negative count and APDU capacity truncation (THE FIX) */
    /* Requesting everything from the end. This should definitely trigger
     * truncation. */
    pRequest.ItemCount = 0;
    pRequest.Range.RefSeqNum = total_records;
    pRequest.Count =
        -(int32_t)total_records; /* Try to get more than will fit */

    len = rr_trend_log_encode(apdu, &pRequest);
    zassert_true(len > 0, "Encoding failed for negative count");
    zassert_true(
        pRequest.ItemCount < total_records, "Expected truncated ItemCount");
    zassert_true(
        bitstring_bit(&pRequest.ResultFlags, RESULT_FLAG_MORE_ITEMS),
        "Expected MORE_ITEMS flag");

    /* THE FIX: FirstSequence should now correctly reflect the shifted uiBegin.
       uiBegin = uiEnd - max_fit + 1.
       In the code, uiEnd is RefSeqNum (total_records).
       So FirstSequence should be total_records - ItemCount + 1 */
    zassert_equal(
        pRequest.FirstSequence, total_records - pRequest.ItemCount + 1,
        "FirstSequence %u does not match calculated start %u",
        (unsigned)pRequest.FirstSequence,
        (unsigned)(total_records - pRequest.ItemCount + 1));
}
/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(
        trendlog_tests, ztest_unit_test(test_Trend_Log_ReadProperty),
        ztest_unit_test(test_Trend_Log_ReadRange_BySequence));

    ztest_run_test_suite(trendlog_tests);
}
