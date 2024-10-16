/**
 * @file
 * @brief Unit test for WriteGroup service
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2024
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/write_group.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(write_group_tests, test_WriteGroup)
#else
static void test_WriteGroup(void)
#endif
{
    int apdu_len = 0;
    int len = 0;
    bool status;
    uint8_t apdu[480] = { 0 };
    BACNET_GROUP_CHANNEL_VALUE value = {
        /* initial test values for sunny day use-case */
        .channel = 0,
        .overriding_priority = 0,
        .value = { .tag = BACNET_APPLICATION_TAG_NULL },
        .next = NULL
    };
    BACNET_WRITE_GROUP_DATA data = {
        /* initial test values for sunny day use-case */
        .group_number = 1,
        .write_priority = 1,
        .change_list = NULL,
        .inhibit_delay = WRITE_GROUP_INHIBIT_DELAY_NONE,
        .next = NULL
    };
    BACNET_WRITE_GROUP_DATA test_data = { 0 };
    BACNET_GROUP_CHANNEL_VALUE test_value = { 0 };

    /* configure the data structures */
    data.change_list = &value;
    test_data.change_list = &test_value;
    /* codec test */
    len = bacnet_write_group_service_request_encode(
        &apdu[0], sizeof(apdu), &data);
    zassert_true(len > 0, NULL);
    apdu_len = len;
    len = bacnet_write_group_service_request_decode(
        &apdu[0], apdu_len, &test_data);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(len, apdu_len, NULL);
    status = bacnet_write_group_same(&data, &test_data);
    zassert_true(status, NULL);
    while (--apdu_len) {
        len = bacnet_write_group_service_request_decode(
            apdu, apdu_len, &test_data);
        zassert_true(len <= 0, NULL);
    }
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(write_group_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(write_group_tests, ztest_unit_test(test_WriteGroup));

    ztest_run_test_suite(write_group_tests);
}
#endif
