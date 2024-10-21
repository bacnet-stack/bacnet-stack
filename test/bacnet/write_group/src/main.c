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

#define WRITE_GROUP_CHANNEL_LIST_MAX 8

/**
 * @addtogroup bacnet_tests
 * @{
 */

static void test_WriteGroup_Postive(BACNET_WRITE_GROUP_DATA *data)
{
    int len = 0;
    int apdu_len = 0;
    bool status;
    uint8_t apdu[480] = { 0 };
    BACNET_WRITE_GROUP_DATA test_data = { 0 };

    /* positive test */
    len =
        bacnet_write_group_service_request_encode(&apdu[0], sizeof(apdu), data);
    zassert_true(len > 0, NULL);
    apdu_len = len;
    len = bacnet_write_group_service_request_decode(
        &apdu[0], apdu_len, &test_data);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(len, apdu_len, NULL);
    status = bacnet_write_group_same(data, &test_data);
    zassert_true(status, NULL);
    /* negative decoding test */
    while (--apdu_len) {
        len = bacnet_write_group_service_request_decode(
            apdu, apdu_len, &test_data);
        /* inhibit delay is an optional element at the end of the
           construction and the APDU is valid without it */
        if (data->inhibit_delay == WRITE_GROUP_INHIBIT_DELAY_NONE) {
            zassert_true(len <= 0, NULL);
        }
    }
    /* negative encoding test */
    len =
        bacnet_write_group_service_request_encode(&apdu[0], sizeof(apdu), data);
    zassert_true(len > 0, NULL);
    apdu_len = len;
    while (--apdu_len) {
        len =
            bacnet_write_group_service_request_encode(&apdu[0], apdu_len, data);
        zassert_equal(len, 0, NULL);
    }
}

/**
 * @addtogroup bacnet_tests
 * @{
 */

static void test_WriteGroup_Negative(BACNET_WRITE_GROUP_DATA *data)
{
    int len = 0;
    int apdu_len = 0;
    uint8_t apdu[480] = { 0 };
    BACNET_WRITE_GROUP_DATA test_data = { 0 };

    len =
        bacnet_write_group_service_request_encode(&apdu[0], sizeof(apdu), data);
    zassert_true(len > 0, NULL);
    apdu_len = len;
    len = bacnet_write_group_service_request_decode(
        &apdu[0], apdu_len, &test_data);
    zassert_true(len < 0, NULL);
}

/**
 * @brief callback for WriteGroup-Request iterator
 * @param data [in] The contents of the WriteGroup-Request message
 * @param change_list_index [in] The index of the current value in the change
 * list
 * @param change_list [in] The current value in the change list
 */
static void test_WriteGroup_Iterate_Value(
    BACNET_WRITE_GROUP_DATA *data,
    uint32_t change_list_index,
    BACNET_GROUP_CHANNEL_VALUE *change_list)
{
    BACNET_GROUP_CHANNEL_VALUE *value = NULL;

    value = bacnet_write_group_channel_value_element(data, change_list_index);
    zassert_not_null(value, NULL);
    zassert_true(bacnet_group_channel_value_same(value, change_list), NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(write_group_tests, test_WriteGroup_Iterate)
#else
static void test_WriteGroup_Iterate(void)
#endif
{
    int len, test_len;
    uint8_t apdu[480] = { 0 };
    BACNET_WRITE_GROUP_DATA data = {
        /* initial test values for sunny day use-case */
        .group_number = 1,
        .write_priority = BACNET_MIN_PRIORITY,
        .change_list = { 0 },
        .inhibit_delay = WRITE_GROUP_INHIBIT_DELAY_NONE,
        .next = NULL
    };
    BACNET_GROUP_CHANNEL_VALUE value[WRITE_GROUP_CHANNEL_LIST_MAX] = { 0 };
    BACNET_GROUP_CHANNEL_VALUE *value_element = NULL;
    unsigned index = 0;

    /* link the array of change-list to our data */
    bacnet_write_group_channel_value_link_array(
        &data, value, ARRAY_SIZE(value));
    /* note: array + head */
    for (index = 0; index < ARRAY_SIZE(value) + 1; index++) {
        value_element = bacnet_write_group_channel_value_element(&data, index);
        zassert_not_null(value_element, NULL);
        /* set some predictable values to be tested */
        value_element->channel = index;
        value_element->overriding_priority = index;
        value_element->value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
        value_element->value.type.Unsigned_Int = index;
    }
    len = bacnet_write_group_service_request_encode(
        &apdu[0], sizeof(apdu), &data);
    zassert_true(len > 0, NULL);
    test_len = bacnet_write_group_service_request_decode_iterate(
        &apdu[0], len, &data, test_WriteGroup_Iterate_Value);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(write_group_tests, test_WriteGroup)
#else
static void test_WriteGroup(void)
#endif
{
    int len = 0;
    uint8_t apdu[480] = { 0 };
    BACNET_WRITE_GROUP_DATA data = {
        /* initial test values for sunny day use-case */
        .group_number = 1,
        .write_priority = BACNET_MIN_PRIORITY,
        .change_list = { 0 },
        .inhibit_delay = WRITE_GROUP_INHIBIT_DELAY_NONE,
        .next = NULL
    };

    /* negative test */
    len =
        bacnet_write_group_service_request_encode(&apdu[0], sizeof(apdu), NULL);
    zassert_false(len > 0, NULL);
    /* positive tests */
    data.inhibit_delay = WRITE_GROUP_INHIBIT_DELAY_TRUE;
    test_WriteGroup_Postive(&data);
    data.inhibit_delay = WRITE_GROUP_INHIBIT_DELAY_FALSE;
    test_WriteGroup_Postive(&data);
    data.inhibit_delay = WRITE_GROUP_INHIBIT_DELAY_NONE;
    test_WriteGroup_Postive(&data);
    data.write_priority = 0;
    test_WriteGroup_Negative(&data);
    data.write_priority = BACNET_MAX_PRIORITY + 1;
    test_WriteGroup_Negative(&data);
    data.write_priority = BACNET_MAX_PRIORITY;
    test_WriteGroup_Postive(&data);
    data.group_number = 0;
    test_WriteGroup_Negative(&data);
    data.group_number = 1;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(write_group_tests, test_WriteGroup_Same)
#else
static void test_WriteGroup_Same(void)
#endif
{
    bool status;
    BACNET_WRITE_GROUP_DATA data = {
        /* initial test values for sunny day use-case */
        .group_number = 1,
        .write_priority = BACNET_MIN_PRIORITY,
        .change_list = { 0 },
        .inhibit_delay = WRITE_GROUP_INHIBIT_DELAY_NONE,
        .next = NULL
    };
    BACNET_WRITE_GROUP_DATA test_data = { 0 };

    status = bacnet_write_group_same(&data, &test_data);
    zassert_false(status, NULL);
    status = bacnet_write_group_copy(NULL, &data);
    zassert_false(status, NULL);
    status = bacnet_write_group_copy(&test_data, NULL);
    zassert_false(status, NULL);

    status = bacnet_write_group_copy(&test_data, &data);
    zassert_true(status, NULL);
    status = bacnet_write_group_same(&data, &test_data);
    zassert_true(status, NULL);
    status = bacnet_write_group_same(NULL, &test_data);
    zassert_false(status, NULL);
    status = bacnet_write_group_same(&data, NULL);
    zassert_false(status, NULL);

    status = bacnet_write_group_copy(&test_data, &data);
    test_data.group_number = 0;
    status = bacnet_write_group_same(&data, &test_data);
    zassert_false(status, NULL);

    status = bacnet_write_group_copy(&test_data, &data);
    test_data.write_priority = BACNET_MAX_PRIORITY;
    status = bacnet_write_group_same(&data, &test_data);
    zassert_false(status, NULL);

    status = bacnet_write_group_copy(&test_data, &data);
    test_data.inhibit_delay = WRITE_GROUP_INHIBIT_DELAY_TRUE;
    status = bacnet_write_group_same(&data, &test_data);
    zassert_false(status, NULL);

    status = bacnet_write_group_copy(&test_data, &data);
    test_data.change_list.channel = 1;
    status = bacnet_write_group_same(&data, &test_data);
    zassert_false(status, NULL);

    status = bacnet_write_group_copy(&test_data, &data);
    test_data.change_list.overriding_priority = 1;
    status = bacnet_write_group_same(&data, &test_data);
    zassert_false(status, NULL);

    status = bacnet_write_group_copy(&test_data, &data);
    test_data.change_list.value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    test_data.change_list.value.type.Boolean = true;
    status = bacnet_write_group_same(&data, &test_data);
    zassert_false(status, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(write_group_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        write_group_tests, ztest_unit_test(test_WriteGroup),
        ztest_unit_test(test_WriteGroup_Same),
        ztest_unit_test(test_WriteGroup_Iterate));

    ztest_run_test_suite(write_group_tests);
}
#endif
