/**
 * @file
 * @brief test GetAlarmSummary-ACK service handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdlib.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <bacnet/bacstr.h>
#include <bacnet/get_alarm_sum.h>
#include <bacnet/basic/service/h_gas_a.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

#define TEST_ENTRIES_MAX 4

static unsigned Callback_Count;
static unsigned Callback_Empty_Count;
static unsigned Callback_Error_Count;
static BACNET_ERROR_CODE Callback_Error_Code;
static BACNET_ADDRESS *Callback_Src;
static uint8_t Callback_Invoke_ID;
static BACNET_GET_ALARM_SUMMARY_DATA Callback_Data[TEST_ENTRIES_MAX];

static void test_callback(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    const BACNET_GET_ALARM_SUMMARY_DATA *data,
    BACNET_ERROR_CODE error_code)
{
    Callback_Src = src;
    Callback_Invoke_ID = invoke_id;
    if (error_code != ERROR_CODE_SUCCESS) {
        Callback_Error_Code = error_code;
        Callback_Error_Count++;
    } else if (!data) {
        Callback_Empty_Count++;
    } else {
        if (Callback_Count < TEST_ENTRIES_MAX) {
            Callback_Data[Callback_Count] = *data;
        }
        Callback_Count++;
    }
}

static void test_callback_reset(void)
{
    Callback_Count = 0;
    Callback_Empty_Count = 0;
    Callback_Error_Count = 0;
    Callback_Error_Code = ERROR_CODE_SUCCESS;
    Callback_Src = NULL;
    Callback_Invoke_ID = 0;
    memset(Callback_Data, 0, sizeof(Callback_Data));
}

static void test_entry_init(
    BACNET_GET_ALARM_SUMMARY_DATA *data,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_EVENT_STATE alarm_state)
{
    data->objectIdentifier.type = object_type;
    data->objectIdentifier.instance = object_instance;
    data->alarmState = alarm_state;
    bitstring_init(&data->acknowledgedTransitions);
    bitstring_set_bit(
        &data->acknowledgedTransitions, TRANSITION_TO_OFFNORMAL, false);
    bitstring_set_bit(
        &data->acknowledgedTransitions, TRANSITION_TO_FAULT, true);
    bitstring_set_bit(
        &data->acknowledgedTransitions, TRANSITION_TO_NORMAL, true);
}

static int test_entries_encode(
    uint8_t *apdu,
    size_t apdu_size,
    const BACNET_GET_ALARM_SUMMARY_DATA *data,
    unsigned count)
{
    int apdu_len = 0;
    unsigned i;

    for (i = 0; i < count; i++) {
        int len = get_alarm_summary_ack_encode_apdu_data(
            &apdu[apdu_len], apdu_size - apdu_len, &data[i]);
        zassert_true(len > 0, NULL);
        apdu_len += len;
    }

    return apdu_len;
}

static bool test_entry_same(
    const BACNET_GET_ALARM_SUMMARY_DATA *a,
    const BACNET_GET_ALARM_SUMMARY_DATA *b)
{
    return (a->objectIdentifier.type == b->objectIdentifier.type) &&
        (a->objectIdentifier.instance == b->objectIdentifier.instance) &&
        (a->alarmState == b->alarmState) &&
        bitstring_same(
               &a->acknowledgedTransitions, &b->acknowledgedTransitions);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_gas_a_tests, test_get_alarm_summary_ack_handler)
#else
static void test_get_alarm_summary_ack_handler(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    uint8_t *service_request = NULL;
    int apdu_len = 0;
    int first_len = 0;
    int second_len = 0;
    unsigned i;
    uint16_t service_len;
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_ACK_DATA service_data = { 0 };
    BACNET_GET_ALARM_SUMMARY_DATA data[3] = { 0 };

    test_entry_init(&data[0], OBJECT_ANALOG_INPUT, 1, EVENT_STATE_HIGH_LIMIT);
    test_entry_init(&data[1], OBJECT_BINARY_INPUT, 2, EVENT_STATE_OFFNORMAL);
    test_entry_init(
        &data[2], OBJECT_ANALOG_VALUE, BACNET_MAX_INSTANCE, EVENT_STATE_FAULT);
    apdu_len = test_entries_encode(apdu, sizeof(apdu), data, 3);
    service_data.invoke_id = 42;
    /* no callback registered */
    get_alarm_summary_ack_callback_set(NULL);
    test_callback_reset();
    get_alarm_summary_ack_handler(
        apdu, (uint16_t)apdu_len, &src, &service_data);
    zassert_equal(Callback_Count, 0, NULL);
    /* one callback per entry */
    get_alarm_summary_ack_callback_set(test_callback);
    test_callback_reset();
    get_alarm_summary_ack_handler(
        apdu, (uint16_t)apdu_len, &src, &service_data);
    zassert_equal(Callback_Count, 3, NULL);
    zassert_equal(Callback_Empty_Count, 0, NULL);
    zassert_equal(Callback_Error_Count, 0, NULL);
    zassert_equal(Callback_Src, &src, NULL);
    zassert_equal(Callback_Invoke_ID, 42, NULL);
    for (i = 0; i < 3; i++) {
        zassert_true(test_entry_same(&Callback_Data[i], &data[i]), NULL);
    }
    /* empty ACK is success without an entry */
    test_callback_reset();
    get_alarm_summary_ack_handler(apdu, 0, &src, &service_data);
    zassert_equal(Callback_Count, 0, NULL);
    zassert_equal(Callback_Empty_Count, 1, NULL);
    zassert_equal(Callback_Error_Count, 0, NULL);
    first_len = test_entries_encode(apdu, sizeof(apdu), data, 1);
    second_len = test_entries_encode(apdu, sizeof(apdu), data, 2);
    /* truncated: error is reported, and no entries are reported */
    for (service_len = 0; service_len < (uint16_t)apdu_len; service_len++) {
        /* exact-size buffer so an over-read is caught by sanitizers */
        service_request = malloc(service_len ? service_len : 1);
        zassert_not_null(service_request, NULL);
        if (!service_request) {
            return;
        }
        memcpy(service_request, apdu, service_len);
        test_callback_reset();
        get_alarm_summary_ack_handler(
            service_request, service_len, &src, &service_data);
        if (service_len == 0) {
            zassert_equal(Callback_Empty_Count, 1, NULL);
            zassert_equal(Callback_Error_Count, 0, NULL);
        } else if (
            (service_len == first_len) || (service_len == second_len) ||
            (service_len == apdu_len)) {
            /* truncated on an entry boundary is a valid shorter ACK */
            zassert_equal(
                Callback_Error_Count, 0, "service_len=%u", service_len);
        } else {
            zassert_equal(Callback_Count, 0, "service_len=%u", service_len);
            zassert_equal(
                Callback_Error_Count, 1, "service_len=%u", service_len);
            zassert_not_equal(Callback_Error_Code, ERROR_CODE_SUCCESS, NULL);
        }
        free(service_request);
    }
    /* malformed second entry: error is reported, and no entries */
    apdu_len = test_entries_encode(apdu, sizeof(apdu), data, 3);
    first_len = test_entries_encode(apdu, sizeof(apdu), data, 1);
    apdu[first_len] = 0xFF;
    test_callback_reset();
    get_alarm_summary_ack_handler(
        apdu, (uint16_t)apdu_len, &src, &service_data);
    zassert_equal(Callback_Count, 0, NULL);
    zassert_equal(Callback_Error_Count, 1, NULL);
    /* malformed first entry */
    memset(apdu, 0xFF, sizeof(apdu));
    test_callback_reset();
    get_alarm_summary_ack_handler(apdu, 10, &src, &service_data);
    zassert_equal(Callback_Count, 0, NULL);
    zassert_equal(Callback_Error_Count, 1, NULL);
    get_alarm_summary_ack_callback_set(NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_gas_a_tests, test_get_alarm_summary_ack_print_data)
#else
static void test_get_alarm_summary_ack_print_data(void)
#endif
{
    BACNET_GET_ALARM_SUMMARY_DATA data = { 0 };

    test_entry_init(&data, OBJECT_ANALOG_INPUT, 1, EVENT_STATE_HIGH_LIMIT);
    get_alarm_summary_ack_print_data(&data);
    get_alarm_summary_ack_print_data(NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(h_gas_a_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        h_gas_a_tests, ztest_unit_test(test_get_alarm_summary_ack_handler),
        ztest_unit_test(test_get_alarm_summary_ack_print_data));

    ztest_run_test_suite(h_gas_a_tests);
}
#endif
