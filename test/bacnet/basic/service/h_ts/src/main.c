/**
 * @file
 * @brief Unit tests for the TimeSynchronization service handler
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/ztest.h>
#include "bacnet/bacaddr.h"
#include "bacnet/bacdest.h"
#include "bacnet/datetime.h"
#include "bacnet/timesync.h"
#include "bacnet/basic/service/h_ts.h"
#if defined(BACNET_ADDRESS_CACHE)
#include "bacnet/basic/binding/address.h"
#endif

#ifndef MAX_TIME_SYNC_RECIPIENTS
#define MAX_TIME_SYNC_RECIPIENTS 16
#endif

extern unsigned Test_Local_Sync_Count;
extern unsigned Test_UTC_Sync_Count;
extern BACNET_DATE_TIME Test_Local_Sync_Time;
extern BACNET_DATE_TIME Test_UTC_Sync_Time;
extern BACNET_ADDRESS Test_Local_Sync_Address;
extern BACNET_ADDRESS Test_UTC_Sync_Address;
extern void test_datetime_local_set(
    const BACNET_DATE_TIME *date_time, int16_t utc_offset, bool dst_active);
extern void
test_device_time_sync_set(uint32_t interval, bool align, uint32_t offset);

static int append_recipient(
    uint8_t *buffer, int buffer_length, const BACNET_RECIPIENT *recipient)
{
    int len = 0;

    len = bacnet_recipient_encode(&buffer[buffer_length], recipient);
    if (len > 0) {
        buffer_length += len;
    }

    return buffer_length;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_ts_tests, test_handler_timesync_recipient_lists)
#else
static void test_handler_timesync_recipient_lists(void)
#endif
{
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    BACNET_RECIPIENT recipient = { 0 };
    BACNET_RECIPIENT_LIST decoded[MAX_TIME_SYNC_RECIPIENTS] = { 0 };
    BACNET_ADDRESS address = { 0 };
    uint8_t encoded[MAX_APDU] = { 0 };
    int encoded_length = 0;
    int application_length = 0;
    unsigned index = 0;

    handler_timesync_init();
    wp_data.array_index = BACNET_ARRAY_ALL;
    recipient.tag = BACNET_RECIPIENT_TAG_DEVICE;
    recipient.type.device.type = OBJECT_DEVICE;
    recipient.type.device.instance = 1234;
    application_length =
        append_recipient(wp_data.application_data, 0, &recipient);
    address.net = 42;
    address.len = 2;
    address.adr[0] = 0x12;
    address.adr[1] = 0x34;
    recipient.tag = BACNET_RECIPIENT_TAG_ADDRESS;
    recipient.type.address = address;
    application_length = append_recipient(
        wp_data.application_data, application_length, &recipient);
    wp_data.application_data_len = application_length;
#if defined(BACNET_ADDRESS_CACHE)
    zassert_true(handler_timesync_recipient_write(&wp_data), NULL);
    encoded_length =
        handler_timesync_encode_recipients(encoded, sizeof(encoded));
    zassert_true(encoded_length > 0, NULL);
    bacnet_recipient_list_link_array(decoded, MAX_TIME_SYNC_RECIPIENTS);
    zassert_equal(
        timesync_decode_timesync_recipients(
            encoded, (unsigned)encoded_length, decoded),
        encoded_length, NULL);
    zassert_equal(decoded[0].recipient.tag, BACNET_RECIPIENT_TAG_DEVICE, NULL);
    zassert_equal(decoded[0].recipient.type.device.instance, 1234, NULL);
    zassert_equal(decoded[1].recipient.tag, BACNET_RECIPIENT_TAG_ADDRESS, NULL);
    zassert_true(
        bacnet_address_same(&decoded[1].recipient.type.address, &address),
        NULL);
#else
    zassert_false(handler_timesync_recipient_write(&wp_data), NULL);
    zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(
        wp_data.error_code, ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED,
        NULL);
    zassert_equal(
        handler_timesync_encode_recipients(encoded, sizeof(encoded)), 0, NULL);
    application_length =
        append_recipient(wp_data.application_data, 0, &recipient);
    wp_data.application_data_len = application_length;
    zassert_true(handler_timesync_recipient_write(&wp_data), NULL);
    encoded_length =
        handler_timesync_encode_recipients(encoded, sizeof(encoded));
    zassert_true(encoded_length > 0, NULL);
    bacnet_recipient_list_link_array(decoded, MAX_TIME_SYNC_RECIPIENTS);
    zassert_equal(
        timesync_decode_timesync_recipients(
            encoded, (unsigned)encoded_length, decoded),
        encoded_length, NULL);
    zassert_equal(decoded[0].recipient.tag, BACNET_RECIPIENT_TAG_ADDRESS, NULL);
    zassert_true(
        bacnet_address_same(&decoded[0].recipient.type.address, &address),
        NULL);
#endif

    wp_data.application_data_len = 0;
    zassert_true(handler_timesync_utc_recipient_write(&wp_data), NULL);
    zassert_equal(
        handler_timesync_encode_utc_recipients(encoded, sizeof(encoded)), 0,
        NULL);

    wp_data.application_data[0] = 0xFF;
    wp_data.application_data_len = 1;
    zassert_false(handler_timesync_recipient_write(&wp_data), NULL);
    wp_data.application_data_len = -1;
    zassert_false(handler_timesync_recipient_write(&wp_data), NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_INVALID_DATA_TYPE, NULL);
    wp_data.application_data_len = MAX_APDU + 1;
    zassert_false(handler_timesync_recipient_write(&wp_data), NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_INVALID_DATA_TYPE, NULL);
    wp_data.application_data_len = 1;
    wp_data.application_data[0] = 0xFF;
    zassert_false(handler_timesync_recipient_write(&wp_data), NULL);
    wp_data.array_index = 1;
    wp_data.application_data_len = 0;
    zassert_false(handler_timesync_utc_recipient_write(&wp_data), NULL);
    zassert_equal(
        wp_data.error_code, ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY, NULL);

    handler_timesync_init();
    wp_data.array_index = BACNET_ARRAY_ALL;
    application_length = 0;
    recipient.tag = BACNET_RECIPIENT_TAG_ADDRESS;
    for (index = 0; index < MAX_TIME_SYNC_RECIPIENTS; index++) {
        address.net = (uint16_t)index;
        recipient.type.address = address;
        application_length = append_recipient(
            wp_data.application_data, application_length, &recipient);
    }
    wp_data.application_data_len = application_length;
    zassert_true(handler_timesync_recipient_write(&wp_data), NULL);
    encoded_length =
        handler_timesync_encode_recipients(encoded, sizeof(encoded));
    zassert_true(encoded_length > 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_ts_tests, test_handler_timesync_sends_and_alignment)
#else
static void test_handler_timesync_sends_and_alignment(void)
#endif
{
    BACNET_DATE_TIME current = { 0 };
    BACNET_ADDRESS address = { 0 };
    BACNET_RECIPIENT recipient = { 0 };
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    unsigned sends = 0;

    handler_timesync_init();
#if defined(BACNET_ADDRESS_CACHE)
    address_init();
#endif
    Test_Local_Sync_Count = 0;
    Test_UTC_Sync_Count = 0;
    address.net = 1;
    address.len = 1;
    address.adr[0] = 1;
    len = 0;
#if defined(BACNET_ADDRESS_CACHE)
    recipient.tag = BACNET_RECIPIENT_TAG_DEVICE;
    recipient.type.device.type = OBJECT_DEVICE;
    recipient.type.device.instance = 1234;
    len = bacnet_recipient_encode(apdu, &recipient);
#endif
    recipient.tag = BACNET_RECIPIENT_TAG_ADDRESS;
    recipient.type.address = address;
    len += bacnet_recipient_encode(&apdu[len], &recipient);
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.application_data_len = len;
    (void)memcpy(wp_data.application_data, apdu, (size_t)len);
    zassert_true(handler_timesync_recipient_write(&wp_data), NULL);
    zassert_true(handler_timesync_utc_recipient_write(&wp_data), NULL);

    datetime_set_values(&current, 2030, 1, 1, 12, 1, 0, 0);
    test_datetime_local_set(&current, 300, false);
    test_device_time_sync_set(30, true, 1);
    handler_timesync_task(&current);
    zassert_equal(Test_Local_Sync_Count, 1, NULL);
    zassert_equal(Test_UTC_Sync_Count, 1, NULL);
    zassert_true(bacnet_address_same(&Test_Local_Sync_Address, &address), NULL);
    zassert_true(bacnet_address_same(&Test_UTC_Sync_Address, &address), NULL);
    zassert_equal(Test_UTC_Sync_Time.time.hour, 17, NULL);
    zassert_equal(Test_UTC_Sync_Time.time.min, 1, NULL);

    datetime_set_values(&current, 2030, 1, 1, 12, 30, 0, 0);
    test_datetime_local_set(&current, 300, false);
    handler_timesync_task(&current);
    zassert_equal(Test_Local_Sync_Count, 1, NULL);
    datetime_set_values(&current, 2030, 1, 1, 12, 31, 0, 0);
    test_datetime_local_set(&current, 300, false);
    handler_timesync_task(&current);
    zassert_equal(Test_Local_Sync_Count, 2, NULL);

    test_device_time_sync_set(15, true, 0);
    datetime_set_values(&current, 2032, 1, 1, 10, 59, 0, 0);
    test_datetime_local_set(&current, 300, false);
    handler_timesync_task(&current);
    sends = Test_Local_Sync_Count;
    datetime_set_values(&current, 2032, 1, 1, 10, 59, 59, 0);
    test_datetime_local_set(&current, 300, false);
    handler_timesync_task(&current);
    zassert_equal(Test_Local_Sync_Count, sends, NULL);
    datetime_set_values(&current, 2032, 1, 1, 11, 0, 0, 0);
    test_datetime_local_set(&current, 300, false);
    handler_timesync_task(&current);
    zassert_equal(Test_Local_Sync_Count, sends + 1, NULL);

    test_device_time_sync_set(1440, true, 1);
    datetime_set_values(&current, 2033, 1, 1, 23, 59, 0, 0);
    test_datetime_local_set(&current, 300, false);
    handler_timesync_task(&current);
    sends = Test_Local_Sync_Count;
    datetime_set_values(&current, 2033, 1, 2, 0, 0, 0, 0);
    test_datetime_local_set(&current, 300, false);
    handler_timesync_task(&current);
    zassert_equal(Test_Local_Sync_Count, sends, NULL);
    datetime_set_values(&current, 2033, 1, 2, 0, 1, 0, 0);
    test_datetime_local_set(&current, 300, false);
    handler_timesync_task(&current);
    zassert_equal(Test_Local_Sync_Count, sends + 1, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_ts_tests, test_handler_timesync_device_recipients)
#else
static void test_handler_timesync_device_recipients(void)
#endif
{
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    BACNET_RECIPIENT recipient = { 0 };
    BACNET_DATE_TIME current = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;

    handler_timesync_init();
    Test_Local_Sync_Count = 0;
    Test_UTC_Sync_Count = 0;
    wp_data.array_index = BACNET_ARRAY_ALL;
    recipient.tag = BACNET_RECIPIENT_TAG_DEVICE;
    recipient.type.device.type = OBJECT_DEVICE;
    recipient.type.device.instance = 4321;
    len = bacnet_recipient_encode(apdu, &recipient);
#if defined(BACNET_ADDRESS_CACHE)
    {
        BACNET_RECIPIENT_LIST decoded[MAX_TIME_SYNC_RECIPIENTS] = { 0 };
        BACNET_ADDRESS direct = { 0 };
        BACNET_ADDRESS bound = { 0 };
        uint8_t encoded[MAX_APDU] = { 0 };
        int encoded_length = 0;
        unsigned local_sends = 0;

        direct.net = 9;
        direct.len = 1;
        direct.adr[0] = 9;
        recipient.tag = BACNET_RECIPIENT_TAG_ADDRESS;
        recipient.type.address = direct;
        len += bacnet_recipient_encode(&apdu[len], &recipient);
        wp_data.application_data_len = len;
        (void)memcpy(wp_data.application_data, apdu, (size_t)len);
        zassert_true(handler_timesync_recipient_write(&wp_data), NULL);

        recipient.tag = BACNET_RECIPIENT_TAG_DEVICE;
        recipient.type.device.type = OBJECT_DEVICE;
        recipient.type.device.instance = 4321;
        len = bacnet_recipient_encode(apdu, &recipient);
        wp_data.application_data_len = len;
        (void)memcpy(wp_data.application_data, apdu, (size_t)len);
        zassert_true(handler_timesync_utc_recipient_write(&wp_data), NULL);
        encoded_length =
            handler_timesync_encode_utc_recipients(encoded, sizeof(encoded));
        zassert_true(encoded_length > 0, NULL);
        bacnet_recipient_list_link_array(decoded, MAX_TIME_SYNC_RECIPIENTS);
        zassert_equal(
            timesync_decode_timesync_recipients(
                encoded, (unsigned)encoded_length, decoded),
            encoded_length, NULL);
        zassert_equal(
            decoded[0].recipient.tag, BACNET_RECIPIENT_TAG_DEVICE, NULL);
        zassert_equal(decoded[0].recipient.type.device.instance, 4321, NULL);

        address_init();
        datetime_set_values(&current, 2040, 6, 1, 8, 0, 0, 0);
        test_datetime_local_set(&current, 0, false);
        test_device_time_sync_set(60, false, 0);
        handler_timesync_task(&current);
        zassert_equal(Test_Local_Sync_Count, 1, NULL);
        zassert_equal(Test_UTC_Sync_Count, 0, NULL);
        zassert_true(
            bacnet_address_same(&Test_Local_Sync_Address, &direct), NULL);

        bound.net = 4;
        bound.mac_len = 1;
        bound.mac[0] = 0x44;
        address_add(4321, 480, &bound);
        local_sends = Test_Local_Sync_Count;
        datetime_set_values(&current, 2040, 6, 1, 9, 0, 0, 0);
        test_datetime_local_set(&current, 0, false);
        handler_timesync_task(&current);
        zassert_equal(Test_Local_Sync_Count, local_sends + 2, NULL);
        zassert_equal(Test_UTC_Sync_Count, 1, NULL);
        zassert_true(
            bacnet_address_same(&Test_Local_Sync_Address, &direct), NULL);
        zassert_true(bacnet_address_same(&Test_UTC_Sync_Address, &bound), NULL);
    }
#else
    wp_data.application_data_len = len;
    (void)memcpy(wp_data.application_data, apdu, (size_t)len);
    zassert_false(handler_timesync_recipient_write(&wp_data), NULL);
    zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(
        wp_data.error_code, ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED,
        NULL);
    zassert_false(handler_timesync_utc_recipient_write(&wp_data), NULL);
    datetime_set_values(&current, 2040, 6, 1, 8, 0, 0, 0);
    test_datetime_local_set(&current, 0, false);
    test_device_time_sync_set(60, false, 0);
    handler_timesync_task(&current);
    zassert_equal(Test_Local_Sync_Count, 0, NULL);
    zassert_equal(Test_UTC_Sync_Count, 0, NULL);
    zassert_equal(
        handler_timesync_encode_recipients(apdu, sizeof(apdu)), 0, NULL);
    zassert_equal(
        handler_timesync_encode_utc_recipients(apdu, sizeof(apdu)), 0, NULL);
#endif
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(h_ts_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        h_ts_tests, ztest_unit_test(test_handler_timesync_recipient_lists),
        ztest_unit_test(test_handler_timesync_sends_and_alignment),
        ztest_unit_test(test_handler_timesync_device_recipients));
    ztest_run_test_suite(h_ts_tests);
}
#endif
