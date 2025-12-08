/**
 * @file
 * @brief Unit test for BACnetChannelValue.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <zephyr/ztest.h>
#include "bacnet/bacdcode.h"
#include "bacnet/bactext.h"
#include "bacnet/channel_value.h"

/**
 * @addtogroup bacnet_tests
 * @{
 */

/** @brief try decoding a real sample from Siemens, captured with wireshark */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetChannelValue_Tests, test_BACnetChannelValue)
#else
static void test_BACnetChannelValue(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;
    bool status;
    BACNET_CHANNEL_VALUE *value;
    BACNET_CHANNEL_VALUE case_value[] = {
        { .tag = BACNET_APPLICATION_TAG_NULL, .type.Real = 0.0f, .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_BOOLEAN,
          .type.Boolean = false,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
          .type.Unsigned_Int = 0xDEADBEEF,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_SIGNED_INT,
          .type.Signed_Int = 0x00C0FFEE,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_REAL,
          .type.Real = 3.141592654f,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_DOUBLE,
          .type.Double = 2.32323232323,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_ENUMERATED,
          .type.Enumerated = 0x0BADF00D,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_OCTET_STRING,
          .type.Octet_String = { .length = 4,
                                 .value = { 0xDE, 0xAD, 0xBE, 0xEF } },
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_CHARACTER_STRING,
          .type.Character_String = { .encoding = CHARACTER_UTF8,
                                     .length = 11,
                                     .value = "Hello World" },
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_BIT_STRING,
          .type.Bit_String = { .bits_used = 10, .value = { 0xFF, 0x03 } },
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_DATE,
          .type.Date = { .year = 2024, .month = 10, .day = 31, .wday = 4 },
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_TIME,
          .type.Time = { .hour = 13, .min = 45, .sec = 30, .hundredths = 50 },
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_OBJECT_ID,
          .type.Object_Id = { .type = OBJECT_ANALOG_INPUT, .instance = 12345 },
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_LIGHTING_COMMAND,
          .type.Lighting_Command.operation = BACNET_LIGHTS_NONE,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_COLOR_COMMAND,
          .type.Color_Command.operation = BACNET_COLOR_OPERATION_NONE,
          .next = NULL },
        { .tag = BACNET_APPLICATION_TAG_XY_COLOR,
          .type.XY_Color.x_coordinate = 0.0f,
          .type.XY_Color.y_coordinate = 0.0f,
          .next = NULL },
    };
    struct ascii_channel_value {
        const char *string;
        BACNET_APPLICATION_TAG tag;
    } ascii_values[] = {
        { "NULL", BACNET_APPLICATION_TAG_NULL },
        { "FALSE", BACNET_APPLICATION_TAG_BOOLEAN },
        { "TRUE", BACNET_APPLICATION_TAG_BOOLEAN },
        { "1234567890", BACNET_APPLICATION_TAG_UNSIGNED_INT },
        { "-1234567890", BACNET_APPLICATION_TAG_SIGNED_INT },
        { "3.141592654", BACNET_APPLICATION_TAG_REAL },
        { "-3.141592654", BACNET_APPLICATION_TAG_REAL },
        { "F1.21", BACNET_APPLICATION_TAG_REAL },
        { "f1.21", BACNET_APPLICATION_TAG_REAL },
        { "D1.21", BACNET_APPLICATION_TAG_DOUBLE },
        { "d1.21", BACNET_APPLICATION_TAG_DOUBLE },
        { "L0", BACNET_APPLICATION_TAG_LIGHTING_COMMAND },
        { "l0", BACNET_APPLICATION_TAG_LIGHTING_COMMAND },
        { "C0", BACNET_APPLICATION_TAG_COLOR_COMMAND },
        { "c0", BACNET_APPLICATION_TAG_COLOR_COMMAND },
        { "X0.0,0.0", BACNET_APPLICATION_TAG_XY_COLOR },
        { "x0.0,0.0", BACNET_APPLICATION_TAG_XY_COLOR },
    };
    size_t i;
    BACNET_CHANNEL_VALUE test_value = { 0 };
    BACNET_APPLICATION_TAG test_tag, expected_tag;
    BACNET_TAG tag = { 0 };
    BACNET_CHANNEL_VALUE coercion_value = { 0 };
    struct channel_value_coercion_values {
        BACNET_CHANNEL_VALUE value;
        BACNET_APPLICATION_TAG tag;
        BACNET_APPLICATION_TAG expected_tag;
    } coercion_values[] = {
        /* add valid coercion test cases here */
        { { .tag = BACNET_APPLICATION_TAG_NULL },
          BACNET_APPLICATION_TAG_NULL,
          BACNET_APPLICATION_TAG_NULL },
        { { .tag = BACNET_APPLICATION_TAG_NULL },
          BACNET_APPLICATION_TAG_BOOLEAN,
          BACNET_APPLICATION_TAG_NULL },
        /* Boolean */
        { { .tag = BACNET_APPLICATION_TAG_BOOLEAN, .type.Boolean = false },
          BACNET_APPLICATION_TAG_NULL,
          BACNET_APPLICATION_TAG_NULL },
        { { .tag = BACNET_APPLICATION_TAG_BOOLEAN, .type.Boolean = false },
          BACNET_APPLICATION_TAG_UNSIGNED_INT,
          BACNET_APPLICATION_TAG_UNSIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_BOOLEAN, .type.Boolean = true },
          BACNET_APPLICATION_TAG_UNSIGNED_INT,
          BACNET_APPLICATION_TAG_UNSIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_BOOLEAN, .type.Boolean = false },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_SIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_BOOLEAN, .type.Boolean = true },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_SIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_BOOLEAN, .type.Boolean = false },
          BACNET_APPLICATION_TAG_REAL,
          BACNET_APPLICATION_TAG_REAL },
        { { .tag = BACNET_APPLICATION_TAG_BOOLEAN, .type.Boolean = true },
          BACNET_APPLICATION_TAG_REAL,
          BACNET_APPLICATION_TAG_REAL },
        { { .tag = BACNET_APPLICATION_TAG_BOOLEAN, .type.Boolean = false },
          BACNET_APPLICATION_TAG_DOUBLE,
          BACNET_APPLICATION_TAG_DOUBLE },
        { { .tag = BACNET_APPLICATION_TAG_BOOLEAN, .type.Boolean = true },
          BACNET_APPLICATION_TAG_DOUBLE,
          BACNET_APPLICATION_TAG_DOUBLE },
        { { .tag = BACNET_APPLICATION_TAG_BOOLEAN, .type.Boolean = false },
          BACNET_APPLICATION_TAG_ENUMERATED,
          BACNET_APPLICATION_TAG_ENUMERATED },
        { { .tag = BACNET_APPLICATION_TAG_BOOLEAN, .type.Boolean = true },
          BACNET_APPLICATION_TAG_ENUMERATED,
          BACNET_APPLICATION_TAG_ENUMERATED },
        /* Unsigned Integer */
        { { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
            .type.Unsigned_Int = 0 },
          BACNET_APPLICATION_TAG_NULL,
          BACNET_APPLICATION_TAG_NULL },
        { { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
            .type.Unsigned_Int = 0 },
          BACNET_APPLICATION_TAG_BOOLEAN,
          BACNET_APPLICATION_TAG_BOOLEAN },
        { { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
            .type.Unsigned_Int = 1 },
          BACNET_APPLICATION_TAG_BOOLEAN,
          BACNET_APPLICATION_TAG_BOOLEAN },
        { { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
            .type.Unsigned_Int = 0 },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_SIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
            .type.Unsigned_Int = 0 },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_SIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
            .type.Unsigned_Int = 2147483647UL },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_SIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
            .type.Unsigned_Int = 2147483647UL + 1UL },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        { { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
            .type.Unsigned_Int = 9999999UL },
          BACNET_APPLICATION_TAG_REAL,
          BACNET_APPLICATION_TAG_REAL },
        { { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
            .type.Unsigned_Int = 9999999UL + 1UL },
          BACNET_APPLICATION_TAG_REAL,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        { { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
            .type.Unsigned_Int = 1UL },
          BACNET_APPLICATION_TAG_DOUBLE,
          BACNET_APPLICATION_TAG_DOUBLE },
        { { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
            .type.Unsigned_Int = 1UL },
          BACNET_APPLICATION_TAG_ENUMERATED,
          BACNET_APPLICATION_TAG_ENUMERATED },
        { { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
            .type.Unsigned_Int = 1UL },
          BACNET_APPLICATION_TAG_OBJECT_ID,
          BACNET_APPLICATION_TAG_OBJECT_ID },
        /* Signed Integer */
        { { .tag = BACNET_APPLICATION_TAG_SIGNED_INT, .type.Signed_Int = 0 },
          BACNET_APPLICATION_TAG_NULL,
          BACNET_APPLICATION_TAG_NULL },
        { { .tag = BACNET_APPLICATION_TAG_SIGNED_INT, .type.Signed_Int = 0 },
          BACNET_APPLICATION_TAG_BOOLEAN,
          BACNET_APPLICATION_TAG_BOOLEAN },
        { { .tag = BACNET_APPLICATION_TAG_SIGNED_INT, .type.Signed_Int = 1 },
          BACNET_APPLICATION_TAG_BOOLEAN,
          BACNET_APPLICATION_TAG_BOOLEAN },
        { { .tag = BACNET_APPLICATION_TAG_SIGNED_INT, .type.Signed_Int = 0 },
          BACNET_APPLICATION_TAG_UNSIGNED_INT,
          BACNET_APPLICATION_TAG_UNSIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_SIGNED_INT,
            .type.Signed_Int = 2147483647 },
          BACNET_APPLICATION_TAG_UNSIGNED_INT,
          BACNET_APPLICATION_TAG_UNSIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_SIGNED_INT, .type.Signed_Int = -1L },
          BACNET_APPLICATION_TAG_UNSIGNED_INT,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        { { .tag = BACNET_APPLICATION_TAG_SIGNED_INT,
            .type.Signed_Int = 9999999L },
          BACNET_APPLICATION_TAG_REAL,
          BACNET_APPLICATION_TAG_REAL },
        { { .tag = BACNET_APPLICATION_TAG_SIGNED_INT,
            .type.Signed_Int = 9999999L + 1L },
          BACNET_APPLICATION_TAG_REAL,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        { { .tag = BACNET_APPLICATION_TAG_SIGNED_INT, .type.Signed_Int = 1UL },
          BACNET_APPLICATION_TAG_DOUBLE,
          BACNET_APPLICATION_TAG_DOUBLE },
        { { .tag = BACNET_APPLICATION_TAG_SIGNED_INT, .type.Signed_Int = 1UL },
          BACNET_APPLICATION_TAG_ENUMERATED,
          BACNET_APPLICATION_TAG_ENUMERATED },
        { { .tag = BACNET_APPLICATION_TAG_SIGNED_INT, .type.Signed_Int = 1UL },
          BACNET_APPLICATION_TAG_OBJECT_ID,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        /* REAL */
        { { .tag = BACNET_APPLICATION_TAG_REAL, .type.Real = 0.0f },
          BACNET_APPLICATION_TAG_NULL,
          BACNET_APPLICATION_TAG_NULL },
        { { .tag = BACNET_APPLICATION_TAG_REAL, .type.Real = 0.0f },
          BACNET_APPLICATION_TAG_BOOLEAN,
          BACNET_APPLICATION_TAG_BOOLEAN },
        { { .tag = BACNET_APPLICATION_TAG_REAL, .type.Real = 1.0f },
          BACNET_APPLICATION_TAG_BOOLEAN,
          BACNET_APPLICATION_TAG_BOOLEAN },
        { { .tag = BACNET_APPLICATION_TAG_REAL, .type.Real = 0.0f },
          BACNET_APPLICATION_TAG_UNSIGNED_INT,
          BACNET_APPLICATION_TAG_UNSIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_REAL, .type.Real = -1.0f },
          BACNET_APPLICATION_TAG_UNSIGNED_INT,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        { { .tag = BACNET_APPLICATION_TAG_REAL, .type.Real = 9999999.0f },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_SIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_REAL, .type.Real = -1.0f },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_SIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_REAL,
            .type.Real = 214783000.0F + 9999.0f },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        { { .tag = BACNET_APPLICATION_TAG_REAL, .type.Real = 1.0f },
          BACNET_APPLICATION_TAG_DOUBLE,
          BACNET_APPLICATION_TAG_DOUBLE },
        { { .tag = BACNET_APPLICATION_TAG_REAL, .type.Real = 1.0f },
          BACNET_APPLICATION_TAG_ENUMERATED,
          BACNET_APPLICATION_TAG_ENUMERATED },
        { { .tag = BACNET_APPLICATION_TAG_REAL, .type.Real = -1.0f },
          BACNET_APPLICATION_TAG_ENUMERATED,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        { { .tag = BACNET_APPLICATION_TAG_REAL, .type.Real = 1.0f },
          BACNET_APPLICATION_TAG_OBJECT_ID,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        /* Double */
        { { .tag = BACNET_APPLICATION_TAG_DOUBLE, .type.Double = 0.0 },
          BACNET_APPLICATION_TAG_NULL,
          BACNET_APPLICATION_TAG_NULL },
        { { .tag = BACNET_APPLICATION_TAG_DOUBLE, .type.Double = 0.0 },
          BACNET_APPLICATION_TAG_BOOLEAN,
          BACNET_APPLICATION_TAG_BOOLEAN },
        { { .tag = BACNET_APPLICATION_TAG_DOUBLE, .type.Double = 1.0 },
          BACNET_APPLICATION_TAG_BOOLEAN,
          BACNET_APPLICATION_TAG_BOOLEAN },
        { { .tag = BACNET_APPLICATION_TAG_DOUBLE, .type.Double = 0.0 },
          BACNET_APPLICATION_TAG_UNSIGNED_INT,
          BACNET_APPLICATION_TAG_UNSIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_DOUBLE, .type.Double = -1.0 },
          BACNET_APPLICATION_TAG_UNSIGNED_INT,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        { { .tag = BACNET_APPLICATION_TAG_DOUBLE, .type.Double = 9999999.0 },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_SIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_DOUBLE, .type.Double = -1.0 },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_SIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_DOUBLE,
            .type.Double = 214783000.0 + 9999.0 },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        { { .tag = BACNET_APPLICATION_TAG_DOUBLE, .type.Double = 1.0 },
          BACNET_APPLICATION_TAG_REAL,
          BACNET_APPLICATION_TAG_REAL },
        { { .tag = BACNET_APPLICATION_TAG_DOUBLE, .type.Double = 3.4E+40 },
          BACNET_APPLICATION_TAG_REAL,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        { { .tag = BACNET_APPLICATION_TAG_DOUBLE, .type.Double = 1.0 },
          BACNET_APPLICATION_TAG_ENUMERATED,
          BACNET_APPLICATION_TAG_ENUMERATED },
        { { .tag = BACNET_APPLICATION_TAG_DOUBLE, .type.Double = -1.0 },
          BACNET_APPLICATION_TAG_ENUMERATED,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        { { .tag = BACNET_APPLICATION_TAG_DOUBLE, .type.Double = 1.0 },
          BACNET_APPLICATION_TAG_OBJECT_ID,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        /* Enumerated */
        { { .tag = BACNET_APPLICATION_TAG_ENUMERATED, .type.Unsigned_Int = 0 },
          BACNET_APPLICATION_TAG_NULL,
          BACNET_APPLICATION_TAG_NULL },
        { { .tag = BACNET_APPLICATION_TAG_ENUMERATED, .type.Unsigned_Int = 0 },
          BACNET_APPLICATION_TAG_BOOLEAN,
          BACNET_APPLICATION_TAG_BOOLEAN },
        { { .tag = BACNET_APPLICATION_TAG_ENUMERATED, .type.Unsigned_Int = 1 },
          BACNET_APPLICATION_TAG_BOOLEAN,
          BACNET_APPLICATION_TAG_BOOLEAN },
        { { .tag = BACNET_APPLICATION_TAG_ENUMERATED, .type.Unsigned_Int = 0 },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_SIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_ENUMERATED, .type.Unsigned_Int = 0 },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_SIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_ENUMERATED,
            .type.Unsigned_Int = 2147483647UL },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_SIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_ENUMERATED,
            .type.Unsigned_Int = 2147483647UL + 1UL },
          BACNET_APPLICATION_TAG_SIGNED_INT,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        { { .tag = BACNET_APPLICATION_TAG_ENUMERATED,
            .type.Unsigned_Int = 9999999UL },
          BACNET_APPLICATION_TAG_REAL,
          BACNET_APPLICATION_TAG_REAL },
        { { .tag = BACNET_APPLICATION_TAG_ENUMERATED,
            .type.Unsigned_Int = 9999999UL + 1UL },
          BACNET_APPLICATION_TAG_REAL,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        { { .tag = BACNET_APPLICATION_TAG_ENUMERATED,
            .type.Unsigned_Int = 1UL },
          BACNET_APPLICATION_TAG_DOUBLE,
          BACNET_APPLICATION_TAG_DOUBLE },
        { { .tag = BACNET_APPLICATION_TAG_ENUMERATED,
            .type.Unsigned_Int = 1UL },
          BACNET_APPLICATION_TAG_UNSIGNED_INT,
          BACNET_APPLICATION_TAG_UNSIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_ENUMERATED,
            .type.Unsigned_Int = 1UL },
          BACNET_APPLICATION_TAG_OBJECT_ID,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        /* DATE */
        { { .tag = BACNET_APPLICATION_TAG_DATE },
          BACNET_APPLICATION_TAG_NULL,
          BACNET_APPLICATION_TAG_NULL },
        { { .tag = BACNET_APPLICATION_TAG_DATE },
          BACNET_APPLICATION_TAG_BOOLEAN,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        /* TIME */
        { { .tag = BACNET_APPLICATION_TAG_TIME },
          BACNET_APPLICATION_TAG_NULL,
          BACNET_APPLICATION_TAG_NULL },
        { { .tag = BACNET_APPLICATION_TAG_TIME },
          BACNET_APPLICATION_TAG_BOOLEAN,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        /* Object Identifier */
        { { .tag = BACNET_APPLICATION_TAG_OBJECT_ID,
            .type.Object_Id = { .type = OBJECT_DEVICE, .instance = 12345 } },
          BACNET_APPLICATION_TAG_NULL,
          BACNET_APPLICATION_TAG_NULL },
        { { .tag = BACNET_APPLICATION_TAG_OBJECT_ID,
            .type.Object_Id = { .type = OBJECT_LOOP, .instance = 12345 } },
          BACNET_APPLICATION_TAG_UNSIGNED_INT,
          BACNET_APPLICATION_TAG_UNSIGNED_INT },
        { { .tag = BACNET_APPLICATION_TAG_OBJECT_ID,
            .type.Object_Id = { .type = OBJECT_LOOP, .instance = 12345 } },
          BACNET_APPLICATION_TAG_BOOLEAN,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
        /* negative testing */
        { { .tag = BACNET_APPLICATION_TAG_EMPTYLIST },
          BACNET_APPLICATION_TAG_EMPTYLIST,
          BACNET_APPLICATION_TAG_RESERVE_MAX },
    };

    bacnet_channel_value_link_array(case_value, ARRAY_SIZE(case_value));
    value = &case_value[0];
    while (value) {
        /* no coercion path */
        null_len = bacnet_channel_value_encode(NULL, sizeof(apdu), value);
        if (value->tag != BACNET_APPLICATION_TAG_NULL) {
            zassert_not_equal(null_len, 0, NULL);
        }
        apdu_len = bacnet_channel_value_encode(apdu, sizeof(apdu), value);
        zassert_equal(
            apdu_len, null_len, "value->tag: %s len=%d null_len=%d",
            bactext_application_tag_name(value->tag), apdu_len, null_len);
        test_len = bacnet_channel_value_decode(apdu, apdu_len, &test_value);
        zassert_not_equal(
            test_len, BACNET_STATUS_ERROR, "value->tag: %s test_len=%d",
            bactext_application_tag_name(value->tag), test_len);
        zassert_equal(test_len, apdu_len, NULL);
        zassert_equal(
            value->tag, test_value.tag, "value->tag: %s test_tag=%s",
            bactext_application_tag_name(value->tag),
            bactext_application_tag_name(test_value.tag));
        status = bacnet_channel_value_same(value, &test_value);
        zassert_true(
            status, "decode: different: %s",
            bactext_application_tag_name(value->tag));
        status = bacnet_channel_value_copy(NULL, value);
        zassert_false(status, NULL);
        status = bacnet_channel_value_copy(&test_value, NULL);
        zassert_false(status, NULL);
        status = bacnet_channel_value_copy(&test_value, value);
        zassert_true(
            status, "copy: failed: %s",
            bactext_application_tag_name(value->tag));
        status = bacnet_channel_value_same(value, &test_value);
        zassert_true(
            status, "copy: different: %s",
            bactext_application_tag_name(value->tag));
        /* coercion path */
        null_len = bacnet_channel_value_coerce_data_encode(
            NULL, sizeof(apdu), value, value->tag);
        if (value->tag != BACNET_APPLICATION_TAG_NULL) {
            zassert_not_equal(null_len, 0, NULL);
        }
        apdu_len = bacnet_channel_value_coerce_data_encode(
            apdu, sizeof(apdu), value, value->tag);
        zassert_equal(
            apdu_len, null_len, "value->tag: %s len=%d null_len=%d",
            bactext_application_tag_name(value->tag), apdu_len, null_len);
        test_len = bacnet_channel_value_decode(NULL, apdu_len, &test_value);
        zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
        test_len = bacnet_channel_value_decode(apdu, apdu_len, NULL);
        zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
        test_len = bacnet_channel_value_decode(apdu, apdu_len, &test_value);
        zassert_not_equal(
            test_len, BACNET_STATUS_ERROR, "value->tag: %s test_len=%d",
            bactext_application_tag_name(value->tag), test_len);
        zassert_equal(test_len, apdu_len, NULL);
        zassert_equal(
            value->tag, test_value.tag, "value->tag: %s test_tag=%s",
            bactext_application_tag_name(value->tag),
            bactext_application_tag_name(test_value.tag));
        status = bacnet_channel_value_same(value, &test_value);
        zassert_true(
            status, "decode: different: %s",
            bactext_application_tag_name(value->tag));
        /* next test case */
        value = value->next;
    }
    apdu_len = bacnet_channel_value_coerce_data_encode(
        apdu, sizeof(apdu), NULL, BACNET_APPLICATION_TAG_NULL);
    zassert_equal(apdu_len, BACNET_STATUS_ERROR, NULL);
    for (i = 0; i < ARRAY_SIZE(coercion_values); i++) {
        bacnet_channel_value_copy(&coercion_value, &coercion_values[i].value);
        test_tag = coercion_values[i].tag;
        expected_tag = coercion_values[i].expected_tag;
        /* coercion path */
        apdu_len = bacnet_channel_value_coerce_data_encode(
            apdu, sizeof(apdu), &coercion_value, test_tag);
        if (apdu_len != BACNET_STATUS_ERROR) {
            test_len = bacnet_tag_decode(apdu, apdu_len, &tag);
            zassert_not_equal(
                test_len, 0, "tag decode failed len=%d", test_len);
            zassert_true(tag.application, "tag is not application");
            zassert_equal(
                tag.number, expected_tag,
                "value->tag: %s coerce-to: %s expected=%s apdu=%s len=%d",
                bactext_application_tag_name(coercion_value.tag),
                bactext_application_tag_name(test_tag),
                bactext_application_tag_name(expected_tag),
                bactext_application_tag_name(tag.number), apdu_len);
        } else {
            zassert_equal(
                expected_tag, BACNET_APPLICATION_TAG_RESERVE_MAX,
                "value->tag: %s coerce-to: %s len=%d",
                bactext_application_tag_name(coercion_value.tag),
                bactext_application_tag_name(test_tag), apdu_len);
        }
    }
    status = bacnet_channel_value_from_ascii(NULL, NULL);
    zassert_false(status, NULL);
    for (i = 0; i < ARRAY_SIZE(ascii_values); i++) {
        status = bacnet_channel_value_from_ascii(
            &test_value, ascii_values[i].string);
        zassert_true(status, "from_ascii: failed: %s", ascii_values[i].string);
        zassert_equal(
            test_value.tag, ascii_values[i].tag, "from_ascii: %s",
            ascii_values[i].string);
    }
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(BACnetChannelValue_Tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        BACnetChannelValue_Tests, ztest_unit_test(test_BACnetChannelValue));

    ztest_run_test_suite(BACnetChannelValue_Tests);
}
#endif
