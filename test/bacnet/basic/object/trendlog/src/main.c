/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */

#include <string.h>
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
    const int known_fail_property_list[] = { -1 };

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

static void test_Trend_Log_ReadRange_ByPosition(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_READ_RANGE_DATA pRequest = { 0 };
    int len = 0;
    uint32_t object_instance = 0;
    int log_index = 0;
    uint32_t i;

    Trend_Log_Init();
    object_instance = Trend_Log_Index_To_Instance(0);
    log_index = Trend_Log_Instance_To_Index(object_instance);
    for (i = 0; i < 20; i++) {
        TL_Insert_Status_Rec(log_index, LOG_STATUS_LOG_INTERRUPTED, true);
    }
    pRequest.object_type = OBJECT_TRENDLOG;
    pRequest.object_instance = object_instance;
    pRequest.object_property = PROP_LOG_BUFFER;
    pRequest.array_index = BACNET_ARRAY_ALL;
    pRequest.RequestType = RR_BY_POSITION;
    pRequest.Overhead = 0;

    /* positions 1..5 */
    pRequest.Range.RefIndex = 1;
    pRequest.Count = 5;
    len = rr_trend_log_encode(apdu, &pRequest);
    zassert_true(len > 0, "Encoding failed for RefIndex=1");
    zassert_equal(pRequest.ItemCount, 5, "Expected 5 items");

    /* position 0 is not a valid list position: nothing should be returned
       (previously this read the log entry before the array, i.e.
       Logs[log_index][-1], disclosing adjacent memory) */
    memset(&pRequest.ResultFlags, 0, sizeof(pRequest.ResultFlags));
    pRequest.ItemCount = 0;
    pRequest.Range.RefIndex = 0;
    pRequest.Count = 5;
    len = rr_trend_log_encode(apdu, &pRequest);
    zassert_equal(pRequest.ItemCount, 0, "Expected 0 items for RefIndex=0");
}

/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(
        trendlog_tests, ztest_unit_test(test_Trend_Log_ReadProperty),
        ztest_unit_test(test_Trend_Log_ReadRange_ByPosition));

    ztest_run_test_suite(trendlog_tests);
}
