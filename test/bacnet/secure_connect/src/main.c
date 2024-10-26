/**
 * @file
 * @brief Unit test for BACnetSpecialEvent. This test also indirectly tests
 *  BACnetCalendarEntry
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @date Aug 2023
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <zephyr/ztest.h>
#include "bacnet/secure_connect.h"
#include "bacnet/datetime.h"

/**
 * @addtogroup bacnet_tests
 * @{
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnet_Secure_Connect_Tests, test_BACnet_Secure_Connect)
#else
static void test_BACnet_Secure_Connect(void)
#endif
{
    int apdu_len = 0, test_len = 0, null_len = 0, diff = 0;
    uint8_t apdu[MAX_APDU] = {0};
    BACNET_SC_HUB_CONNECTION_STATUS data = { 0 }, test_data;

    data.State = BACNET_SC_CONNECTION_STATE_CONNECTED;
    datetime_init_ascii(&data.Connect_Timestamp, "2023/08/01-12:00:00");
    datetime_init_ascii(&data.Disconnect_Timestamp, "2023/08/02-12:00:00");
    data.Error = ERROR_CODE_DEFAULT;
    data.Error_Details[0] = 0;

    null_len = bacapp_encode_SCHubConnection(NULL, &data);
    apdu_len = bacapp_encode_SCHubConnection(apdu, &data);
    zassert_true(null_len == apdu_len, NULL);

    test_len = bacapp_decode_SCHubConnection(apdu, apdu_len, &test_data);
    zassert_true(test_len == apdu_len, NULL);
    zassert_true(test_data.State == data.State, NULL);
    diff = datetime_compare(&test_data.Connect_Timestamp, &data.Connect_Timestamp);
    zassert_equal(diff, 0, NULL);
    diff = datetime_compare(&test_data.Disconnect_Timestamp, &data.Disconnect_Timestamp);
    zassert_equal(diff, 0, NULL);
    zassert_true(test_data.Error == data.Error, NULL);
    diff = strcmp(test_data.Error_Details, data.Error_Details);
    zassert_equal(diff, 0, NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(BACnet_Secure_Connect_Tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(BACnet_Secure_Connect_Tests,
        ztest_unit_test(test_BACnet_Secure_Connect));

    ztest_run_test_suite(BACnet_Secure_Connect_Tests);
}
#endif
