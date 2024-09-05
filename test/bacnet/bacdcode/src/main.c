/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */
#include <math.h>
#include <ctype.h> /* For isprint */
#include <zephyr/ztest.h>
#include <bacnet/bacdcode.h>
#include <memory.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test ...
 */
static int get_apdu_len(bool extended_tag, uint32_t value)
{
    int test_len = 1;

    if (extended_tag) {
        test_len++;
    }
    if (value <= 4) {
        test_len += 0; /* do nothing... */
    } else if (value <= 253) {
        test_len += 1;
    } else if (value <= 65535) {
        test_len += 3;
    } else {
        test_len += 5;
    }

    return test_len;
}

static void test_bacnet_tag_codec(
    uint8_t tag_number,
    bool context_specific,
    bool opening,
    bool closing,
    uint32_t len_value_type)
{
    uint8_t apdu[BACNET_TAG_SIZE] = { 0 };
    BACNET_TAG tag = { 0 };
    int len = 0, test_len = 0, null_len = 0, tag_len = 0;
    uint32_t tag_len_value_type;
    bool status;

    if (opening) {
        null_len = encode_opening_tag(NULL, tag_number);
        len = encode_opening_tag(apdu, tag_number);
    } else if (closing) {
        null_len = encode_closing_tag(NULL, tag_number);
        len = encode_closing_tag(apdu, tag_number);
    } else {
        null_len =
            encode_tag(NULL, tag_number, context_specific, len_value_type);
        len = encode_tag(apdu, tag_number, context_specific, len_value_type);
    }
    zassert_equal(len, null_len, NULL);
    test_len = bacnet_tag_decode(apdu, sizeof(apdu), &tag);
    zassert_equal(len, test_len, NULL);
    zassert_equal(tag.number, tag_number, NULL);
    if (context_specific) {
        zassert_true(tag.context, NULL);
        zassert_false(tag.application, NULL);
        zassert_false(tag.closing, NULL);
        zassert_false(tag.opening, NULL);
        status = bacnet_is_context_tag_number(
            apdu, sizeof(apdu), tag_number, &tag_len, &tag_len_value_type);
        zassert_true(status, NULL);
        zassert_equal(tag_len, test_len, NULL);
        zassert_equal(tag_len_value_type, len_value_type, NULL);
    } else if (opening) {
        //zassert_false(tag.context, NULL);
        zassert_false(tag.application, NULL);
        zassert_false(tag.closing, NULL);
        zassert_true(tag.opening, NULL);
        status = bacnet_is_opening_tag_number(
            apdu, sizeof(apdu), tag_number, &tag_len);
        zassert_true(status, NULL);
        zassert_equal(tag_len, test_len, NULL);
    } else if (closing) {
        //zassert_false(tag.context, NULL);
        zassert_false(tag.application, NULL);
        zassert_true(tag.closing, NULL);
        zassert_false(tag.opening, NULL);
        status = bacnet_is_closing_tag_number(
            apdu, sizeof(apdu), tag_number, &tag_len);
        zassert_true(status, NULL);
        zassert_equal(tag_len, test_len, NULL);
    } else {
        zassert_false(tag.context, NULL);
        zassert_true(tag.application, NULL);
        zassert_false(tag.closing, NULL);
        zassert_false(tag.opening, NULL);
        status = bacnet_is_context_tag_number(
            apdu, sizeof(apdu), tag_number, &tag_len, &tag_len_value_type);
        zassert_false(status, NULL);
    }
    while (len) {
        len--;
        test_len = bacnet_tag_decode(apdu, len, &tag);
        zassert_equal(test_len, 0, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, test_BACnet_Tag_Codec)
#else
static void testBACnetTagCodec(void)
#endif
{
    bool context_specific = false;
    unsigned bit_i, bit_j;
    uint8_t tag_number;
    uint32_t len_value_type;
    bool opening, closing;

    tag_number = 0;
    for (bit_i = 0; bit_i < 8; bit_i++) {
        len_value_type = 0;
        for (bit_j = 0; bit_j < 32; bit_j++) {
            opening = false;
            closing = false;
            test_bacnet_tag_codec(
                tag_number, context_specific, opening, closing, len_value_type);
            context_specific = true;
            test_bacnet_tag_codec(
                tag_number, context_specific, opening, closing, len_value_type);
            context_specific = false;
            opening = true;
            test_bacnet_tag_codec(
                tag_number, context_specific, opening, closing, len_value_type);
            opening = false;
            closing = true;
            test_bacnet_tag_codec(
                tag_number, context_specific, opening, closing, len_value_type);
            len_value_type = BIT(bit_j);
        }
        tag_number = BIT(bit_i);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACDCodeTags)
#else
static void testBACDCodeTags(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    uint8_t tag_number = 0, test_tag_number = 0;
    int len = 0, test_len = 0, tag_len = 0;
    uint32_t value = 0, test_value = 0, tag_len_value_type = 0;
    unsigned i = 0, j = 0;
    BACNET_TAG tag = { 0 };
    bool status = false;

    for (j = 0; j < 8; j++) {
        len = encode_opening_tag(&apdu[0], tag_number);
        test_len = get_apdu_len(IS_EXTENDED_TAG_NUMBER(apdu[0]), 0);
        zassert_equal(len, test_len, NULL);
        test_len = encode_opening_tag(NULL, tag_number);
        zassert_equal(len, test_len, NULL);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
        len = decode_tag_number_and_value(&apdu[0], &test_tag_number, &value);
        zassert_equal(value, 0, NULL);
        zassert_equal(len, test_len, NULL);
        zassert_equal(tag_number, test_tag_number, NULL);
#endif
        zassert_true(IS_OPENING_TAG(apdu[0]), NULL);
        zassert_false(IS_CLOSING_TAG(apdu[0]), NULL);
        test_len = bacnet_tag_decode(apdu, sizeof(apdu), &tag);
        zassert_true(test_len > 0, NULL);
        zassert_true(tag.opening, NULL);
        zassert_false(tag.closing, NULL);
        len = encode_closing_tag(&apdu[0], tag_number);
        zassert_equal(len, test_len, NULL);
        test_len = encode_closing_tag(NULL, tag_number);
        zassert_equal(len, test_len, NULL);
        test_len = bacnet_tag_decode(apdu, sizeof(apdu), &tag);
        zassert_true(test_len > 0, NULL);
        zassert_false(tag.opening, NULL);
        zassert_true(tag.closing, NULL);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
        len = decode_tag_number_and_value(&apdu[0], &test_tag_number, &value);
        zassert_equal(len, test_len, NULL);
        zassert_equal(value, 0, NULL);
        zassert_equal(tag_number, test_tag_number, NULL);
#endif
        zassert_false(IS_OPENING_TAG(apdu[0]), NULL);
        zassert_true(IS_CLOSING_TAG(apdu[0]), NULL);
        /* test the len-value-type portion */
        value = 0;
        for (i = 0; i < 32; i++) {
            len = encode_tag(&apdu[0], tag_number, false, value);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
            len = decode_tag_number_and_value(
                &apdu[0], &test_tag_number, &test_value);
            zassert_equal(tag_number, test_tag_number, NULL);
            zassert_equal(value, test_value, NULL);
            test_len = get_apdu_len(IS_EXTENDED_TAG_NUMBER(apdu[0]), value);
            zassert_equal(len, test_len, NULL);
#endif
            test_len = bacnet_tag_decode(apdu, sizeof(apdu), &tag);
            zassert_equal(len, test_len, NULL);
            zassert_equal(tag.number, tag_number, NULL);
            zassert_false(tag.context, NULL);
            zassert_true(tag.application, NULL);
            status = bacnet_is_context_tag_number(
                apdu, sizeof(apdu), tag_number, &tag_len, &tag_len_value_type);
            zassert_false(status, NULL);
            zassert_false(tag.closing, NULL);
            zassert_false(tag.opening, NULL);
            /* next value */
            value = BIT(i);
        }
        /* next tag number */
        tag_number = BIT(i);
    }

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACnetTagEncoder)
#else
static void testBACnetTagEncoder(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    unsigned i = 0, j = 0, k = 0;
    BACNET_TAG tag = { 0 }, test_tag = { 0 };
    int len, test_len, null_len;

    tag.application = true;
    tag.opening = false;
    tag.closing = false;
    tag.context = false;
    tag.len_value_type = 0;
    tag.number = 0;
    for (k = 0; k < 2; k++) {
        for (j = 0; j < 8; j++) {
            for (i = 0; i < 32; i++) {
                null_len = bacnet_tag_encode(NULL, sizeof(apdu), &tag);
                len = bacnet_tag_encode(&apdu[0], sizeof(apdu), &tag);
                zassert_equal(len, null_len, NULL);
                test_len = bacnet_tag_decode(apdu, len, &test_tag);
                zassert_equal(len, test_len, NULL);
                zassert_equal(tag.number, test_tag.number, NULL);
                zassert_equal(tag.application, test_tag.application, NULL);
                zassert_equal(tag.context, test_tag.context, NULL);
                zassert_equal(tag.closing, test_tag.closing, NULL);
                zassert_equal(tag.opening, test_tag.opening, NULL);
                tag.len_value_type = BIT(i);
            }
            tag.number = BIT(i);
        }
        tag.context = true;
        tag.application = false;
        tag.len_value_type = 0;
        tag.number = 0;
    }
    tag.opening = true;
    tag.closing = false;
    tag.application = false;
    tag.context = false;
    tag.len_value_type = 0;
    tag.number = 0;
    for (k = 0; k < 2; k++) {
        for (j = 0; j < 8; j++) {
            null_len = bacnet_tag_encode(NULL, sizeof(apdu), &tag);
            len = bacnet_tag_encode(&apdu[0], sizeof(apdu), &tag);
            zassert_equal(len, null_len, NULL);
            test_len = bacnet_tag_decode(apdu, len, &test_tag);
            zassert_equal(len, test_len, NULL);
            zassert_equal(tag.number, test_tag.number, NULL);
            zassert_equal(tag.application, test_tag.application, NULL);
            zassert_equal(tag.context, test_tag.context, NULL);
            zassert_equal(tag.closing, test_tag.closing, NULL);
            zassert_equal(tag.opening, test_tag.opening, NULL);
            tag.number = BIT(i);
        }
        tag.number = 0;
        tag.opening = false;
        tag.closing = true;
    }
    tag.number = BIT(7);
    tag.len_value_type = BIT(31);
    tag.opening = false;
    tag.closing = false;
    tag.application = true;
    tag.context = false;
    len = bacnet_tag_encode(&apdu[0], sizeof(apdu), &tag);
    while (--len) {
        test_len = bacnet_tag_decode(apdu, len, &test_tag);
        zassert_equal(test_len, 0, NULL);
    }
    null_len = bacnet_tag_encode(NULL, sizeof(apdu), &tag);
    while (--null_len) {
        test_len = bacnet_tag_encode(&apdu[0], null_len, &tag);
        zassert_equal(test_len, 0, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACDCodeEnumerated)
#else
static void testBACDCodeEnumerated(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    uint32_t value = 0;
    uint32_t decoded_value = 0;
    int i = 0, apdu_len = 0;
    int len = 0, null_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    BACNET_TAG tag = { 0 };

    for (i = 0; i < 32; i++) {
        apdu_len = encode_application_enumerated(&apdu[0], value);
        null_len = encode_application_enumerated(NULL, value);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
        len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
        len += decode_enumerated(&apdu[len], len_value, &decoded_value);
        zassert_equal(decoded_value, value, NULL);
        zassert_equal(tag_number, BACNET_APPLICATION_TAG_ENUMERATED, NULL);
        zassert_equal(len, apdu_len, NULL);
        zassert_equal(null_len, apdu_len, NULL);
#endif
        len = bacnet_enumerated_application_decode(
            &apdu[0], apdu_len, &decoded_value);
        zassert_equal(decoded_value, value, NULL);
        zassert_equal(len, apdu_len, NULL);
        zassert_equal(null_len, apdu_len, NULL);
        len = bacnet_tag_decode(apdu, sizeof(apdu), &tag);
        zassert_true(len > 0, NULL);
        zassert_equal(tag.number, BACNET_APPLICATION_TAG_ENUMERATED, NULL);
        zassert_true(tag.application, NULL);
        zassert_false(tag.context, NULL);
        zassert_false(tag.closing, NULL);
        zassert_false(tag.opening, NULL);
        /* context specific encoding */
        apdu_len = encode_context_enumerated(&apdu[0], 3, value);
        null_len = encode_context_enumerated(NULL, 3, value);
        len = bacnet_tag_decode(apdu, sizeof(apdu), &tag);
        zassert_true(tag.context, NULL);
        zassert_false(tag.application, NULL);
        zassert_false(tag.closing, NULL);
        zassert_false(tag.opening, NULL);
        zassert_equal(tag.number, 3, NULL);
        zassert_equal(null_len, apdu_len, NULL);
        /* test the interesting values */
        value = BIT(i);
    }
    apdu_len = bacnet_enumerated_application_encode(apdu, sizeof(apdu), value);
    null_len = bacnet_enumerated_application_encode(NULL, sizeof(apdu), value);
    zassert_equal(apdu_len, null_len, NULL);
    len = bacnet_enumerated_application_decode(apdu, apdu_len, &decoded_value);
    zassert_equal(apdu_len, len, "len=%d apdu_len=%d", len, apdu_len);
    zassert_equal(decoded_value, value, NULL);
    while (--apdu_len) {
        len = bacnet_enumerated_application_encode(apdu, apdu_len, value);
        zassert_equal(len, 0, NULL);
    }

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACDCodeReal)
#else
static void testBACDCodeReal(void)
#endif
{
    uint8_t real_apdu[4] = { 0 };
    uint8_t encoded_apdu[4] = { 0 };
    float value = 42.123F;
    float decoded_value = 0.0F;
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, apdu_len = 0, null_len = 0, tag_len = 0;
    uint8_t tag_number = 0;
    BACNET_TAG tag = { 0 };

#if defined(BACNET_STACK_DEPRECATED_DISABLE)
    encode_bacnet_real(value, &real_apdu[0]);
    decode_real(&real_apdu[0], &decoded_value);
    zassert_false(islessgreater(decoded_value, value), NULL);
    encode_bacnet_real(value, &encoded_apdu[0]);
    zassert_equal(
        memcmp(&real_apdu, &encoded_apdu, sizeof(real_apdu)), 0, NULL);
#endif
    /* a real will take up 4 octects plus a one octet tag */
    apdu_len = encode_application_real(&apdu[0], value);
    null_len = encode_application_real(NULL, value);
    zassert_equal(apdu_len, 5, NULL);
    zassert_equal(apdu_len, null_len, NULL);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
    /* len tells us how many octets were used for encoding the value */
    uint32_t long_value = 0;
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &long_value);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_REAL, NULL);
    zassert_false(IS_CONTEXT_SPECIFIC(apdu[0]), NULL);
    zassert_equal(len, 1, NULL);
    zassert_equal(long_value, 4, NULL);
    decode_real(&apdu[len], &decoded_value);
    zassert_false(islessgreater(decoded_value, value), NULL);
#endif
    null_len = bacnet_real_application_decode(apdu, apdu_len, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    len = bacnet_real_application_decode(apdu, apdu_len, &decoded_value);
    zassert_equal(apdu_len, len, NULL);
    tag_len = bacnet_tag_decode(apdu, apdu_len, &tag);
    zassert_true(tag_len > 0, NULL);
    zassert_equal(tag.number, BACNET_APPLICATION_TAG_REAL, NULL);
    zassert_true(tag.application, NULL);
    zassert_false(tag.context, NULL);
    zassert_false(islessgreater(decoded_value, value), NULL);
    while (apdu_len) {
        apdu_len--;
        len = bacnet_real_application_decode(apdu, apdu_len, NULL);
        zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    }
    apdu_len = bacnet_real_application_encode(apdu, sizeof(apdu), value);
    null_len = bacnet_real_application_encode(NULL, sizeof(apdu), value);
    zassert_equal(apdu_len, null_len, NULL);
    len = bacnet_real_application_decode(apdu, apdu_len, &decoded_value);
    zassert_equal(apdu_len, len, "len=%d apdu_len=%d", len, apdu_len);
    zassert_false(islessgreater(decoded_value, value), NULL);
    while (--apdu_len) {
        len = bacnet_real_application_encode(apdu, apdu_len, value);
        zassert_equal(len, 0, NULL);
    }

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACDCodeDouble)
#else
static void testBACDCodeDouble(void)
#endif
{
    uint8_t double_apdu[8] = { 0 };
    uint8_t encoded_apdu[8] = { 0 };
    double value = 42.123;
    double decoded_value = 0.0;
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, apdu_len = 0, null_len = 0, tag_len = 0;
    BACNET_TAG tag = { 0 };

#if defined(BACNET_STACK_DEPRECATED_DISABLE)
    encode_bacnet_double(value, &double_apdu[0]);
    decode_double(&double_apdu[0], &decoded_value);
    zassert_false(islessgreater(decoded_value, value), NULL);
    encode_bacnet_double(value, &encoded_apdu[0]);
    zassert_equal(
        memcmp(&double_apdu, &encoded_apdu, sizeof(double_apdu)), 0, NULL);
#endif
    /* a double will take up 8 octects plus a one octet tag */
    apdu_len = encode_application_double(&apdu[0], value);
    null_len = encode_application_double(NULL, value);
    zassert_equal(apdu_len, 10, NULL);
    zassert_equal(apdu_len, null_len, NULL);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
    uint8_t tag_number = 0;
    uint32_t long_value = 0;
    /* len tells us how many octets were used for encoding the value */
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &long_value);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_DOUBLE, NULL);
    zassert_false(IS_CONTEXT_SPECIFIC(apdu[0]), NULL);
    zassert_equal(len, 2, NULL);
    zassert_equal(long_value, 8, NULL);
    decode_double(&apdu[len], &decoded_value);
#endif
    null_len = bacnet_double_application_decode(apdu, apdu_len, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    len = bacnet_double_application_decode(apdu, apdu_len, &decoded_value);
    zassert_equal(apdu_len, len, NULL);
    tag_len = bacnet_tag_decode(apdu, apdu_len, &tag);
    zassert_true(tag_len > 0, NULL);
    zassert_equal(tag.number, BACNET_APPLICATION_TAG_DOUBLE, NULL);
    zassert_true(tag.application, NULL);
    zassert_false(tag.context, NULL);
    zassert_false(tag.closing, NULL);
    zassert_false(tag.opening, NULL);
    zassert_false(islessgreater(decoded_value, value), NULL);
    while (apdu_len) {
        apdu_len--;
        len = bacnet_double_application_decode(apdu, apdu_len, NULL);
        zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    }
    apdu_len = bacnet_double_application_encode(apdu, sizeof(apdu), value);
    null_len = bacnet_double_application_encode(NULL, sizeof(apdu), value);
    zassert_equal(apdu_len, null_len, NULL);
    len = bacnet_double_application_decode(apdu, apdu_len, &decoded_value);
    zassert_equal(apdu_len, len, "len=%d apdu_len=%d", len, apdu_len);
    zassert_false(islessgreater(decoded_value, value), NULL);
    while (--apdu_len) {
        len = bacnet_double_application_encode(apdu, apdu_len, value);
        zassert_equal(len, 0, NULL);
    }

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACnetDateDecodes)
#else
static void testBACnetDateDecodes(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    uint8_t sample[10] = { 0xa4, 0xff, 0xff, 0xff, 0xff };
    int len, test_len, null_len, apdu_len;
    BACNET_DATE value;
    BACNET_DATE test_value;

    value.day = 3;
    value.month = 10;
    value.wday = 5;
    value.year = 1945;
    len = encode_application_date(apdu, &value);
    test_len = decode_application_date(apdu, &test_value);
    zassert_equal(len, test_len, NULL);
    zassert_equal(value.day, test_value.day, NULL);
    zassert_equal(value.month, test_value.month, NULL);
    zassert_equal(value.wday, test_value.wday, NULL);
    zassert_equal(value.year, test_value.year, NULL);

    memset(&test_value, 0, sizeof(test_value));
    test_len = decode_application_date(sample, &test_value);
    /* try decoding sample data captured from a bacnet device - all wildcards */
    zassert_equal(5, test_len, NULL);
    zassert_equal(0xff, test_value.day, NULL);
    zassert_equal(0xff, test_value.month, NULL);
    zassert_equal(0xff, test_value.wday, NULL);
    zassert_equal(2155, test_value.year, NULL);
    /* test new API for APDU size checking and NULL behavior */
    apdu_len = bacnet_date_application_encode(apdu, sizeof(apdu), &value);
    null_len = bacnet_date_application_encode(NULL, sizeof(apdu), &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_date_application_decode(apdu, apdu_len, &test_value);
    zassert_equal(
        apdu_len, test_len, "test_len=%d apdu_len=%d", test_len, apdu_len);
    zassert_equal(value.day, test_value.day, NULL);
    zassert_equal(value.month, test_value.month, NULL);
    zassert_equal(value.wday, test_value.wday, NULL);
    zassert_equal(value.year, test_value.year, NULL);
    while (--test_len) {
        len = bacnet_date_application_decode(apdu, test_len, &test_value);
        zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    }
    while (--apdu_len) {
        len = bacnet_date_application_encode(apdu, apdu_len, &value);
        zassert_equal(len, 0, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACnetDateRangeDecodes)
#else
static void testBACnetDateRangeDecodes(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    uint8_t sample[10] = { 0xa4, 0xff, 0xff, 0xff, 0xff,
                           0xa4, 0xff, 0xff, 0xff, 0xff };
    int len;
    int null_len;
    int test_len;

    BACNET_DATE_RANGE data;
    BACNET_DATE_RANGE test_data;

    memset(&test_data, 0, sizeof(test_data));

    data.startdate.day = 3;
    data.startdate.month = 10;
    data.startdate.wday = 5;
    data.startdate.year = 1945;

    data.enddate.day = 24;
    data.enddate.month = 8;
    data.enddate.wday = 4;
    data.enddate.year = 2023;

    len = bacnet_daterange_encode(apdu, &data);
    null_len = bacnet_daterange_encode(NULL, &data);
    zassert_equal(len, null_len, NULL);

    test_len = bacnet_daterange_decode(apdu, len, &test_data);
    zassert_equal(len, test_len, NULL);
    zassert_equal(data.startdate.day, test_data.startdate.day, NULL);
    zassert_equal(data.startdate.month, test_data.startdate.month, NULL);
    zassert_equal(data.startdate.wday, test_data.startdate.wday, NULL);
    zassert_equal(data.startdate.year, test_data.startdate.year, NULL);

    zassert_equal(data.enddate.day, test_data.enddate.day, NULL);
    zassert_equal(data.enddate.month, test_data.enddate.month, NULL);
    zassert_equal(data.enddate.wday, test_data.enddate.wday, NULL);
    zassert_equal(data.enddate.year, test_data.enddate.year, NULL);

    memset(&test_data, 0, sizeof(test_data));
    test_len = bacnet_daterange_decode(sample, len, &test_data);

    /* try decoding sample data captured from a bacnet device - all wildcards */
    zassert_equal(10, test_len, NULL);
    zassert_equal(0xff, test_data.startdate.day, NULL);
    zassert_equal(0xff, test_data.startdate.month, NULL);
    zassert_equal(0xff, test_data.startdate.wday, NULL);
    zassert_equal(2155, test_data.startdate.year, NULL);

    zassert_equal(0xff, test_data.enddate.day, NULL);
    zassert_equal(0xff, test_data.enddate.month, NULL);
    zassert_equal(0xff, test_data.enddate.wday, NULL);
    zassert_equal(2155, test_data.enddate.year, NULL);
}

static void verifyBACDCodeUnsignedValue(BACNET_UNSIGNED_INTEGER value)
{
    uint8_t array[5] = { 0 };
    uint8_t encoded_array[5] = { 0 };
    BACNET_UNSIGNED_INTEGER decoded_value = 0;
    int len = 0, null_len = 0;
    uint8_t apdu[MAX_APDU] = { 0 };
    uint8_t tag_number = 0;
    uint32_t len_value = 0;

    len_value = encode_application_unsigned(&array[0], value);
    len = decode_tag_number_and_value(&array[0], &tag_number, &len_value);
    len = decode_unsigned(&array[len], len_value, &decoded_value);
    zassert_equal(
        decoded_value, value, "value=%lu decoded_value=%lu\n",
        (unsigned long)value, (unsigned long)decoded_value);
    encode_application_unsigned(&encoded_array[0], decoded_value);
    zassert_equal(memcmp(&array[0], &encoded_array[0], sizeof(array)), 0, NULL);
    /* an unsigned will take up to 4 octects */
    /* plus a one octet for the tag */
    len = encode_application_unsigned(&apdu[0], value);
    null_len = encode_application_unsigned(NULL, value);
    zassert_equal(len, null_len, NULL);
    /* apdu_len varies... */
    len = decode_tag_number_and_value(&apdu[0], &tag_number, NULL);
    zassert_equal(len, 1, NULL);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_UNSIGNED_INT, NULL);
    zassert_false(IS_CONTEXT_SPECIFIC(apdu[0]), NULL);
}

static void test_bacnet_unsigned_value_codec(BACNET_UNSIGNED_INTEGER value)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_UNSIGNED_INTEGER decoded_value = 0;
    int null_len = 0, test_len = 0, apdu_len = 0, tag_len = 0;
    BACNET_TAG tag = { 0 };

    null_len = encode_application_unsigned(NULL, value);
    apdu_len = encode_application_unsigned(apdu, value);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_true(apdu_len > 0, NULL);
    null_len = bacnet_unsigned_application_decode(apdu, apdu_len, NULL);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    test_len =
        bacnet_unsigned_application_decode(apdu, apdu_len, &decoded_value);
    zassert_equal(apdu_len, test_len, NULL);
    tag_len = bacnet_tag_decode(apdu, apdu_len, &tag);
    zassert_true(tag_len > 0, NULL);
    zassert_equal(tag.number, BACNET_APPLICATION_TAG_UNSIGNED_INT, NULL);
    zassert_true(tag.application, NULL);
    zassert_false(tag.context, NULL);
    zassert_false(tag.closing, NULL);
    zassert_false(tag.opening, NULL);
    zassert_equal(decoded_value, value, NULL);
    while (apdu_len) {
        apdu_len--;
        test_len = bacnet_unsigned_application_decode(apdu, apdu_len, NULL);
        zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    }
    apdu_len = bacnet_unsigned_application_encode(apdu, sizeof(apdu), value);
    null_len = bacnet_unsigned_application_encode(NULL, sizeof(apdu), value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len =
        bacnet_unsigned_application_decode(apdu, apdu_len, &decoded_value);
    zassert_equal(
        apdu_len, test_len, "test_len=%d apdu_len=%d", test_len, apdu_len);
    zassert_equal(decoded_value, value, NULL);
    while (--apdu_len) {
        test_len = bacnet_unsigned_application_encode(apdu, apdu_len, value);
        zassert_equal(test_len, 0, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACDCodeUnsigned)
#else
static void testBACDCodeUnsigned(void)
#endif
{
#ifdef UINT64_MAX
    const unsigned max_bits = 64;
#else
    const unsigned max_bits = 32;
#endif
    uint32_t value;
    int i;

    for (i = 0; i < max_bits; i++) {
        value = BIT(i);
        verifyBACDCodeUnsignedValue(value - 1);
        verifyBACDCodeUnsignedValue(value);
        verifyBACDCodeUnsignedValue(value + 1);

        test_bacnet_unsigned_value_codec(value - 1);
        test_bacnet_unsigned_value_codec(value);
        test_bacnet_unsigned_value_codec(value + 1);
    }

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACnetUnsigned)
#else
static void testBACnetUnsigned(void)
#endif
{
    uint8_t apdu[32] = { 0 };
    BACNET_UNSIGNED_INTEGER value = 0, test_value = 0;
    int len_value = 0, apdu_len = 0, test_len = 0, null_len = 0;
    unsigned i;
#ifdef UINT64_MAX
    const unsigned max_bits = 64;
#else
    const unsigned max_bits = 32;
#endif

    for (i = 0; i < max_bits; i++) {
        value = BIT(i);
        apdu_len = encode_bacnet_unsigned(&apdu[0], value);
        null_len = encode_bacnet_unsigned(NULL, value);
        zassert_equal(apdu_len, null_len, NULL);
        len_value = apdu_len;
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
        test_len = decode_unsigned(&apdu[0], len_value, &test_value);
        zassert_equal(apdu_len, test_len, NULL);
        zassert_equal(value, test_value, NULL);
#endif
        null_len = bacnet_unsigned_decode(apdu, apdu_len, len_value, NULL);
        zassert_equal(
            apdu_len, null_len, "apdu_len=%d null_len=%d value=%u", apdu_len,
            null_len, value);
        test_len =
            bacnet_unsigned_decode(apdu, apdu_len, len_value, &test_value);
        zassert_equal(apdu_len, test_len, NULL);
        while (apdu_len) {
            apdu_len--;
            test_len = bacnet_unsigned_decode(apdu, apdu_len, len_value, NULL);
            zassert_equal(test_len, 0, NULL);
        }
    }
}

static void testBACDCodeSignedValue(int32_t value)
{
    int32_t decoded_value = 0;
    int len = 0, null_len = 0, tag_len = 0, test_len = 0, apdu_len = 0;
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_TAG tag = { 0 };

    len = encode_application_signed(&apdu[0], value);
    null_len = encode_application_signed(NULL, value);
    zassert_equal(null_len, len, NULL);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    len = decode_signed(&apdu[len], len_value, &decoded_value);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_SIGNED_INT, NULL);
    zassert_equal(
        decoded_value, value, "value=%ld decoded_value=%ld\n", (long)value,
        (long)decoded_value);
#endif
    len = encode_application_signed(&apdu[0], value);
    null_len = encode_application_signed(NULL, value);
    zassert_equal(null_len, len, NULL);
    zassert_true(len > 0, NULL);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
    tag_len = decode_tag_number_and_value(&apdu[0], &tag_number, NULL);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_SIGNED_INT, NULL);
    zassert_false(IS_CONTEXT_SPECIFIC(apdu[0]), NULL);
    zassert_true(tag_len > 0, NULL);
#endif
    tag_len = bacnet_tag_decode(apdu, len, &tag);
    zassert_true(tag_len > 0, NULL);
    zassert_equal(tag.number, BACNET_APPLICATION_TAG_SIGNED_INT, NULL);
    zassert_true(tag.application, NULL);
    zassert_false(tag.context, NULL);
    zassert_false(tag.closing, NULL);
    zassert_false(tag.opening, NULL);
    test_len = bacnet_signed_application_decode(apdu, len, &decoded_value);
    null_len = bacnet_signed_application_decode(apdu, len, NULL);
    zassert_equal(
        null_len, len, "test_len=%d null_len=%d len=%d", test_len, null_len,
        len);
    zassert_equal(
        decoded_value, value, "value=%ld decoded_value=%ld", value,
        decoded_value);
    while (len) {
        len--;
        test_len = bacnet_signed_application_decode(apdu, len, NULL);
        zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    }
    apdu_len = bacnet_signed_application_encode(apdu, sizeof(apdu), value);
    null_len = bacnet_signed_application_encode(NULL, sizeof(apdu), value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_signed_application_decode(apdu, apdu_len, &decoded_value);
    zassert_equal(
        apdu_len, test_len, "test_len=%d apdu_len=%d", test_len, apdu_len);
    zassert_equal(decoded_value, value, NULL);
    while (--apdu_len) {
        test_len = bacnet_signed_application_encode(apdu, apdu_len, value);
        zassert_equal(test_len, 0, NULL);
    }

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACDCodeSigned)
#else
static void testBACDCodeSigned(void)
#endif
{
    int value = 1;
    int i = 0;

    for (i = 0; i < 32; i++) {
        testBACDCodeSignedValue(value - 1);
        testBACDCodeSignedValue(value);
        testBACDCodeSignedValue(value + 1);
        value = value << 1;
    }

    testBACDCodeSignedValue(-1);
    value = -2;
    for (i = 0; i < 32; i++) {
        testBACDCodeSignedValue(value - 1);
        testBACDCodeSignedValue(value);
        testBACDCodeSignedValue(value + 1);
        value = value << 1;
    }

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACnetSigned)
#else
static void testBACnetSigned(void)
#endif
{
    uint8_t apdu[32] = { 0 };
    int32_t value = 0, test_value = 0;
    int len = 0, test_len = 0, null_len = 0;
    unsigned i = 0;

    value = -2147483647;
    for (i = 0; i < 32; i++) {
        len = encode_bacnet_signed(&apdu[0], value);
        null_len = encode_bacnet_signed(NULL, value);
        zassert_equal(len, null_len, NULL);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
        test_len = decode_signed(&apdu[0], len, &test_value);
        zassert_equal(len, test_len, NULL);
        zassert_equal(value, test_value, NULL);
#endif
        test_len = bacnet_signed_decode(apdu, len, len, &test_value);
        zassert_equal(len, test_len, NULL);
        zassert_equal(value, test_value, NULL);
        /* next value */
        value /= 2;
    }
    value = 2147483647;
    for (i = 0; i < 32; i++) {
        len = encode_bacnet_signed(&apdu[0], value);
        null_len = encode_bacnet_signed(NULL, value);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
        test_len = decode_signed(&apdu[0], len, &test_value);
        zassert_equal(len, null_len, NULL);
        zassert_equal(len, test_len, NULL);
        zassert_equal(value, test_value, NULL);
#endif
        test_len = bacnet_signed_decode(apdu, len, len, &test_value);
        zassert_equal(len, test_len, NULL);
        zassert_equal(value, test_value, NULL);
        /* next value */
        value /= 2;
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACDCodeOctetString)
#else
static void testBACDCodeOctetString(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_OCTET_STRING value;
    BACNET_OCTET_STRING test_value;
    uint8_t test_apdu[MAX_APDU] = { "" };
    int i; /* for loop counter */
    int apdu_len = 0, len = 0, null_len = 0, test_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    bool status = false;
    int diff = 0; /* for memcmp */

    status = octetstring_init(&value, NULL, 0);
    zassert_true(status, NULL);
    apdu_len = encode_application_octet_string(&apdu[0], &value);
    null_len = encode_application_octet_string(NULL, &value);
    zassert_equal(apdu_len, null_len, NULL);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_OCTET_STRING, NULL);
    len += decode_octet_string(&apdu[len], len_value, &test_value);
    zassert_equal(apdu_len, len, NULL);
    diff = memcmp(
        octetstring_value(&value), &test_apdu[0], octetstring_length(&value));
    zassert_equal(diff, 0, NULL);
#endif
    for (i = 0; i < (MAX_APDU - 6); i++) {
        test_apdu[i] = '0' + (i % 10);
        status = octetstring_init(&value, test_apdu, i);
        zassert_true(status, NULL);
        apdu_len = encode_application_octet_string(apdu, &value);
        null_len = encode_application_octet_string(NULL, &value);
        zassert_equal(apdu_len, null_len, NULL);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
        len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
        zassert_equal(tag_number, BACNET_APPLICATION_TAG_OCTET_STRING, NULL);
        len += decode_octet_string(&apdu[len], len_value, &test_value);
        zassert_equal(apdu_len, len, "test octet string=#%d\n", i);
        diff = memcmp(
            octetstring_value(&value), &test_apdu[0],
            octetstring_length(&value));
        zassert_equal(diff, 0, "test octet string=#%d\n", i);
#endif
        test_len =
            bacnet_octet_string_application_decode(apdu, apdu_len, &test_value);
        zassert_equal(
            apdu_len, test_len, "apdu_len=%d test_len=%d i=%d", apdu_len,
            test_len, i);
        zassert_true(octetstring_value_same(&value, &test_value), NULL);
    }
    apdu_len =
        bacnet_octet_string_application_encode(apdu, sizeof(apdu), &value);
    null_len =
        bacnet_octet_string_application_encode(NULL, sizeof(apdu), &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len =
        bacnet_octet_string_application_decode(apdu, apdu_len, &test_value);
    zassert_equal(
        apdu_len, test_len, "test_len=%d apdu_len=%d", test_len, apdu_len);
    zassert_true(octetstring_value_same(&value, &test_value), NULL);
    while (--test_len) {
        len = bacnet_octet_string_application_decode(apdu, test_len, NULL);
        zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    }
    while (--apdu_len) {
        len = bacnet_octet_string_application_encode(apdu, apdu_len, &value);
        zassert_equal(len, 0, NULL);
    }

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACDCodeCharacterString)
#else
static void testBACDCodeCharacterString(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    uint8_t encoded_apdu[MAX_APDU] = { 0 };
    BACNET_CHARACTER_STRING value;
    BACNET_CHARACTER_STRING test_value;
    char test_name[MAX_APDU] = { "" };
    int i; /* for loop counter */
    int apdu_len = 0, len = 0, null_len = 0, tag_len, test_len;
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    int diff = 0; /* for comparison */
    bool status = false;
    BACNET_TAG tag = { 0 };

    status = characterstring_init(&value, CHARACTER_ANSI_X34, NULL, 0);
    zassert_true(status, NULL);
    apdu_len = encode_application_character_string(&apdu[0], &value);
    null_len = encode_application_character_string(NULL, &value);
    zassert_equal(apdu_len, null_len, NULL);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    len += decode_character_string(&apdu[len], len_value, &test_value);
    zassert_equal(apdu_len, len, NULL);
    diff = memcmp(
        characterstring_value(&value), &test_name[0],
        characterstring_length(&value));
    zassert_equal(diff, 0, NULL);
#endif
    for (i = 0; i < MAX_CHARACTER_STRING_BYTES - 1; i++) {
        test_name[i] = 'S';
        test_name[i + 1] = '\0';
        status = characterstring_init_ansi(&value, test_name);
        zassert_true(status, NULL);
        apdu_len =
            encode_application_character_string(&encoded_apdu[0], &value);
        null_len = encode_application_character_string(NULL, &value);
        zassert_equal(apdu_len, null_len, NULL);
        len = bacnet_character_string_application_decode(
            encoded_apdu, apdu_len, &test_value);
        zassert_equal(len, apdu_len, NULL);
        tag_len = bacnet_tag_decode(encoded_apdu, apdu_len, &tag);
        zassert_true(tag_len > 0, NULL);
        zassert_equal(
            tag.number, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
        zassert_true(tag.application, NULL);
        zassert_false(tag.context, NULL);
        zassert_false(tag.opening, NULL);
        zassert_false(tag.closing, NULL);
        if (apdu_len != len) {
            printf("test string=#%d apdu_len=%d len=%d\n", i, apdu_len, len);
        }
        zassert_equal(apdu_len, len, NULL);
        diff = memcmp(
            characterstring_value(&value), &test_name[0],
            characterstring_length(&value));
        if (diff) {
            printf("test string=#%d\n", i);
        }
        zassert_equal(diff, 0, NULL);
    }
    apdu_len =
        bacnet_character_string_application_encode(apdu, sizeof(apdu), &value);
    null_len =
        bacnet_character_string_application_encode(NULL, sizeof(apdu), &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len =
        bacnet_character_string_application_decode(apdu, apdu_len, &test_value);
    zassert_equal(
        apdu_len, test_len, "test_len=%d apdu_len=%d", test_len, apdu_len);
    zassert_true(characterstring_same(&value, &test_value), NULL);
    while (--test_len) {
        len = bacnet_character_string_application_decode(apdu, test_len, NULL);
        zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    }
    while (--apdu_len) {
        len =
            bacnet_character_string_application_encode(apdu, apdu_len, &value);
        zassert_equal(len, 0, NULL);
    }

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACDCodeObject)
#else
static void testBACDCodeObject(void)
#endif
{
    uint8_t object_apdu[32] = { 0 };
    uint8_t encoded_apdu[32] = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_OBJECT_TYPE type = OBJECT_BINARY_INPUT;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_ANALOG_OUTPUT;
    uint32_t instance = 123;
    uint32_t decoded_instance = 0;
    int apdu_len = 0, len = 0, null_len = 0, test_len = 0;
    uint8_t tag_number = 0;

    apdu_len = encode_bacnet_object_id(&encoded_apdu[0], type, instance);
    null_len = encode_bacnet_object_id(NULL, type, instance);
    zassert_equal(apdu_len, null_len, NULL);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
    decode_object_id(&encoded_apdu[0], &decoded_type, &decoded_instance);
    zassert_equal(decoded_type, type, NULL);
    zassert_equal(decoded_instance, instance, NULL);
#endif
    encode_bacnet_object_id(&object_apdu[0], type, instance);
    zassert_equal(
        memcmp(&object_apdu[0], &encoded_apdu[0], sizeof(object_apdu)), 0,
        NULL);
    for (type = 0; type < 1024; type++) {
        for (instance = 0; instance <= BACNET_MAX_INSTANCE; instance += 1024) {
            /* test application encoded */
            len =
                encode_application_object_id(&encoded_apdu[0], type, instance);
            null_len = encode_application_object_id(NULL, type, instance);
            zassert_equal(len, null_len, NULL);
            zassert_true(len > 0, NULL);
            bacnet_object_id_application_decode(
                &encoded_apdu[0], len, &decoded_type, &decoded_instance);
            zassert_equal(decoded_type, type, NULL);
            zassert_equal(decoded_instance, instance, NULL);
            /* test context encoded */
            tag_number = 99;
            len = encode_context_object_id(
                &encoded_apdu[0], tag_number, type, instance);
            zassert_true(len > 0, NULL);
            len = bacnet_object_id_context_decode(
                &encoded_apdu[0], len, tag_number, &decoded_type,
                &decoded_instance);
            zassert_true(len > 0, NULL);
            zassert_equal(decoded_type, type, NULL);
            zassert_equal(decoded_instance, instance, NULL);
            tag_number = 100;
            len = bacnet_object_id_context_decode(
                &encoded_apdu[0], len, tag_number, &decoded_type,
                &decoded_instance);
            zassert_equal(len, 0, NULL);
        }
    }
    /* validate application API codec and APDU size too short */
    type = 1023;
    instance = BACNET_MAX_INSTANCE;
    apdu_len =
        bacnet_object_id_application_encode(apdu, sizeof(apdu), type, instance);
    null_len =
        bacnet_object_id_application_encode(NULL, sizeof(apdu), type, instance);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_object_id_application_decode(
        apdu, apdu_len, &decoded_type, &decoded_instance);
    zassert_equal(
        apdu_len, test_len, "test_len=%d apdu_len=%d", test_len, apdu_len);
    zassert_equal(decoded_type, type, NULL);
    zassert_equal(decoded_instance, instance, NULL);
    while (--test_len) {
        len = bacnet_object_id_application_decode(
            apdu, test_len, &decoded_type, &decoded_instance);
        zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    }
    while (--apdu_len) {
        len =
            bacnet_object_id_application_encode(apdu, apdu_len, type, instance);
        zassert_equal(len, 0, NULL);
    }
    /* test context encoded */
    type = OBJECT_BINARY_INPUT;
    instance = 123;
    for (tag_number = 0; tag_number < 254; tag_number++) {
        len = encode_context_object_id(
            &encoded_apdu[0], tag_number, type, instance);
        zassert_true(len > 0, NULL);
        null_len = encode_context_object_id(NULL, tag_number, type, instance);
        zassert_equal(len, null_len, NULL);
        len = bacnet_object_id_context_decode(
            &encoded_apdu[0], null_len, tag_number, &decoded_type,
            &decoded_instance);
        zassert_true(len > 0, NULL);
        zassert_equal(decoded_type, type, NULL);
        zassert_equal(decoded_instance, instance, NULL);
        len = bacnet_object_id_context_decode(
            &encoded_apdu[0], null_len, 254, &decoded_type, &decoded_instance);
        zassert_equal(len, 0, NULL);
    }

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACDCodeMaxSegsApdu)
#else
static void testBACDCodeMaxSegsApdu(void)
#endif
{
    int max_segs[8] = { 0, 2, 4, 8, 16, 32, 64, 65 };
    int max_apdu[6] = { 50, 128, 206, 480, 1024, 1476 };
    int i = 0;
    int j = 0;
    uint8_t octet = 0;

    /* test */
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 6; j++) {
            octet = encode_max_segs_max_apdu(max_segs[i], max_apdu[j]);
            zassert_equal(max_segs[i], decode_max_segs(octet), NULL);
            zassert_equal(max_apdu[j], decode_max_apdu(octet), NULL);
        }
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBACDCodeBitString)
#else
static void testBACDCodeBitString(void)
#endif
{
    uint8_t bit = 0;
    BACNET_BIT_STRING value;
    BACNET_BIT_STRING test_value;
    uint8_t apdu[MAX_APDU] = { 0 };
    int len, null_len, apdu_len, test_len;
    BACNET_TAG tag = { 0 };

    bitstring_init(&value);
    /* verify initialization */
    zassert_equal(bitstring_bits_used(&value), 0, NULL);
    for (bit = 0; bit < (MAX_BITSTRING_BYTES * 8); bit++) {
        zassert_false(bitstring_bit(&value, bit), NULL);
    }
    /* test encode/decode -- true */
    for (bit = 0; bit < (MAX_BITSTRING_BYTES * 8); bit++) {
        bitstring_set_bit(&value, bit, true);
        zassert_equal(bitstring_bits_used(&value), (bit + 1), NULL);
        zassert_true(bitstring_bit(&value, bit), NULL);
        /* encode */
        len = encode_application_bitstring(&apdu[0], &value);
        null_len = encode_application_bitstring(NULL, &value);
        zassert_equal(len, null_len, NULL);
        /* decode */
        len = bacnet_bitstring_application_decode(apdu, null_len, &test_value);
        zassert_equal(bitstring_bits_used(&test_value), (bit + 1), NULL);
        zassert_true(bitstring_bit(&test_value, bit), NULL);
        len = bacnet_tag_decode(apdu, len, &tag);
        zassert_true(len > 0, NULL);
        zassert_equal(tag.number, BACNET_APPLICATION_TAG_BIT_STRING, NULL);
    }
    /* test encode/decode -- false */
    bitstring_init(&value);
    for (bit = 0; bit < (MAX_BITSTRING_BYTES * 8); bit++) {
        bitstring_set_bit(&value, bit, false);
        zassert_equal(bitstring_bits_used(&value), (bit + 1), NULL);
        zassert_false(bitstring_bit(&value, bit), NULL);
        /* encode */
        len = bacnet_bitstring_application_encode(apdu, sizeof(apdu), &value);
        null_len =
            bacnet_bitstring_application_encode(NULL, sizeof(apdu), &value);
        zassert_equal(len, null_len, NULL);
        /* decode */
        len = bacnet_bitstring_application_decode(apdu, null_len, &test_value);
        len = bacnet_tag_decode(apdu, len, &tag);
        zassert_true(len > 0, NULL);
        zassert_equal(tag.number, BACNET_APPLICATION_TAG_BIT_STRING, NULL);
        zassert_equal(bitstring_bits_used(&test_value), (bit + 1), NULL);
        zassert_false(bitstring_bit(&test_value, bit), NULL);
    }
    /* test APDU size limits */
    apdu_len = bacnet_bitstring_application_encode(apdu, sizeof(apdu), &value);
    null_len = bacnet_bitstring_application_encode(NULL, sizeof(apdu), &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_bitstring_application_decode(apdu, apdu_len, &test_value);
    zassert_equal(
        apdu_len, test_len, "test_len=%d apdu_len=%d", test_len, apdu_len);
    zassert_true(bitstring_same(&value, &test_value), NULL);
    while (--test_len) {
        len = bacnet_bitstring_application_decode(apdu, test_len, NULL);
        zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    }
    while (--apdu_len) {
        len = bacnet_bitstring_application_encode(apdu, apdu_len, &value);
        zassert_equal(len, 0, NULL);
    }
}

static void
test_unsigned_context_codec(BACNET_UNSIGNED_INTEGER value, uint8_t context_tag)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0, null_len = 0, match_len = 0;
    BACNET_UNSIGNED_INTEGER decoded_value = 0;

    null_len = encode_context_unsigned(NULL, context_tag, value);
    len = encode_context_unsigned(apdu, context_tag, value);
    zassert_equal(null_len, len, NULL);
    zassert_true(len > 0, NULL);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
    test_len = decode_context_unsigned(apdu, context_tag, &decoded_value);
    zassert_equal(test_len, len, NULL);
    zassert_equal(value, decoded_value, NULL);
    match_len = decode_context_unsigned(apdu, context_tag - 1, &decoded_value);
    zassert_equal(match_len, BACNET_STATUS_ERROR, NULL);
#endif
    null_len =
        bacnet_unsigned_context_decode(apdu, sizeof(apdu), context_tag, NULL);
    test_len = bacnet_unsigned_context_decode(
        apdu, sizeof(apdu), context_tag, &decoded_value);
    zassert_equal(null_len, test_len, NULL);
    zassert_equal(test_len, len, NULL);
    zassert_equal(value, decoded_value, NULL);
    match_len = bacnet_unsigned_context_decode(
        apdu, sizeof(apdu), context_tag - 1, &decoded_value);
    zassert_equal(match_len, 0, NULL);
    while (len) {
        len--;
        test_len = bacnet_unsigned_context_decode(apdu, len, context_tag, NULL);
        zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testUnsignedContextDecodes)
#else
static void testUnsignedContextDecodes(void)
#endif
{
    uint8_t context_tag = 0;
    BACNET_UNSIGNED_INTEGER value = 0;
    unsigned i, j;

    for (i = 0; i < 64; i++) {
        value = BIT(i);
        for (j = 0; j < 8; j++) {
            context_tag = BIT(j);
            test_unsigned_context_codec(value, context_tag);
        }
    }
}

static void test_signed_context_codec(int32_t value, uint8_t context_tag)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0, null_len = 0, match_len = 0;
    int32_t decoded_value = 0;

    null_len = encode_context_signed(NULL, context_tag, value);
    len = encode_context_signed(apdu, context_tag, value);
    zassert_equal(null_len, len, NULL);
    zassert_true(len > 0, NULL);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
    test_len = decode_context_signed(apdu, context_tag, &decoded_value);
    zassert_equal(test_len, len, NULL);
    zassert_equal(value, decoded_value, NULL);
    match_len = decode_context_signed(apdu, context_tag - 1, &decoded_value);
    zassert_equal(match_len, BACNET_STATUS_ERROR, NULL);
#endif
    null_len =
        bacnet_signed_context_decode(apdu, sizeof(apdu), context_tag, NULL);
    test_len = bacnet_signed_context_decode(
        apdu, sizeof(apdu), context_tag, &decoded_value);
    zassert_equal(null_len, test_len, NULL);
    zassert_equal(test_len, len, NULL);
    zassert_equal(value, decoded_value, NULL);
    match_len = bacnet_signed_context_decode(
        apdu, sizeof(apdu), context_tag - 1, &decoded_value);
    zassert_equal(match_len, 0, NULL);
    while (len) {
        len--;
        test_len = bacnet_signed_context_decode(apdu, len, context_tag, NULL);
        zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testSignedContextDecodes)
#else
static void testSignedContextDecodes(void)
#endif
{
    uint8_t context_tag = 0;
    int32_t value = 0;
    unsigned i, j;

    for (i = 0; i < 32; i++) {
        value = BIT(i);
        for (j = 0; j < 8; j++) {
            context_tag = BIT(j);
            test_signed_context_codec(value, context_tag);
        }
        value = BIT(i) | BIT(31);
        for (j = 0; j < 8; j++) {
            context_tag = BIT(j);
            test_signed_context_codec(value, context_tag);
        }
    }
}

static void test_enumerated_context_codec(uint32_t value, uint8_t context_tag)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0, null_len = 0, match_len = 0;
    uint32_t decoded_value = 0;

    null_len = encode_context_enumerated(NULL, context_tag, value);
    len = encode_context_enumerated(apdu, context_tag, value);
    zassert_equal(null_len, len, NULL);
    zassert_true(len > 0, NULL);
#if defined(BACNET_STACK_DEPRECATED_DISABLE)
    test_len = decode_context_enumerated(apdu, context_tag, &decoded_value);
    zassert_equal(test_len, len, NULL);
    zassert_equal(value, decoded_value, NULL);
    match_len =
        decode_context_enumerated(apdu, context_tag - 1, &decoded_value);
    zassert_equal(match_len, BACNET_STATUS_ERROR, NULL);
#endif
    null_len =
        bacnet_enumerated_context_decode(apdu, sizeof(apdu), context_tag, NULL);
    test_len = bacnet_enumerated_context_decode(
        apdu, sizeof(apdu), context_tag, &decoded_value);
    zassert_equal(null_len, test_len, NULL);
    zassert_equal(test_len, len, NULL);
    zassert_equal(value, decoded_value, NULL);
    match_len = bacnet_enumerated_context_decode(
        apdu, sizeof(apdu), context_tag - 1, &decoded_value);
    zassert_equal(match_len, 0, NULL);
    while (len) {
        len--;
        test_len =
            bacnet_enumerated_context_decode(apdu, len, context_tag, NULL);
        zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testEnumeratedContextDecodes)
#else
static void testEnumeratedContextDecodes(void)
#endif
{
    uint8_t context_tag = 10;

    /* 32-bit value */
    uint32_t value = 0xdeadbeef;
    test_enumerated_context_codec(value, context_tag);
    context_tag = 0xfe;
    test_enumerated_context_codec(value, context_tag);

    /* 16 bit value */
    value = 0xdead;
    context_tag = 10;
    test_enumerated_context_codec(value, context_tag);
    context_tag = 0xfe;
    test_enumerated_context_codec(value, context_tag);

    /* 8 bit number */
    value = 0xde;
    context_tag = 10;
    test_enumerated_context_codec(value, context_tag);
    context_tag = 0xfe;
    test_enumerated_context_codec(value, context_tag);

    /* 4 bit number */
    value = 0xd;
    context_tag = 10;
    test_enumerated_context_codec(value, context_tag);
    context_tag = 0xfe;
    test_enumerated_context_codec(value, context_tag);

    /* 2 bit number */
    value = 0x2;
    context_tag = 10;
    test_enumerated_context_codec(value, context_tag);
    context_tag = 0xfe;
    test_enumerated_context_codec(value, context_tag);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testFloatContextDecodes)
#else
static void testFloatContextDecodes(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    int inLen;
    int outLen;
    int outLen2;
    uint8_t large_context_tag = 0xfe;

    /* 32 bit number */
    float in;
    float out;

    in = 0.1234f;
    inLen = encode_context_real(apdu, 10, in);
    outLen = bacnet_real_context_decode(apdu, inLen, 10, &out);
    outLen2 = bacnet_real_context_decode(apdu, inLen, 9, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_false(islessgreater(in, out), NULL);
    zassert_equal(outLen2, 0, NULL);

    inLen = encode_context_real(apdu, large_context_tag, in);
    outLen = bacnet_real_context_decode(apdu, inLen, large_context_tag, &out);
    outLen2 =
        bacnet_real_context_decode(apdu, inLen, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_false(islessgreater(in, out), NULL);
    zassert_equal(outLen2, 0, NULL);

    in = 0.0f;
    inLen = encode_context_real(apdu, 10, in);
    outLen = bacnet_real_context_decode(apdu, inLen, 10, &out);
    outLen2 = bacnet_real_context_decode(apdu, inLen, 9, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_false(islessgreater(in, out), NULL);

    inLen = encode_context_real(apdu, large_context_tag, in);
    outLen = bacnet_real_context_decode(apdu, inLen, large_context_tag, &out);
    outLen2 =
        bacnet_real_context_decode(apdu, inLen, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_false(islessgreater(in, out), NULL);
    zassert_equal(outLen2, 0, NULL);
    while (inLen) {
        inLen--;
        outLen =
            bacnet_real_context_decode(apdu, inLen, large_context_tag, &out);
        zassert_equal(outLen, BACNET_STATUS_ERROR, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testDoubleContextDecodes)
#else
static void testDoubleContextDecodes(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    int inLen;
    int outLen;
    int outLen2;
    uint8_t large_context_tag = 0xfe;

    /* 64 bit number */
    double in;
    double out;

    in = 0.1234;
    inLen = encode_context_double(apdu, 10, in);
    outLen = bacnet_double_context_decode(apdu, inLen, 10, &out);
    outLen2 = bacnet_double_context_decode(apdu, inLen, 9, &out);
    zassert_equal(inLen, outLen, NULL);
    zassert_false(islessgreater(in, out), NULL);
    zassert_equal(outLen2, 0, NULL);

    inLen = encode_context_double(apdu, large_context_tag, in);
    outLen = bacnet_double_context_decode(apdu, inLen, large_context_tag, &out);
    outLen2 =
        bacnet_double_context_decode(apdu, inLen, large_context_tag - 1, &out);
    zassert_equal(inLen, outLen, NULL);
    zassert_false(islessgreater(in, out), NULL);
    zassert_equal(outLen2, 0, NULL);

    in = 0.0;
    inLen = encode_context_double(apdu, 10, in);
    outLen = bacnet_double_context_decode(apdu, inLen, 10, &out);
    outLen2 = bacnet_double_context_decode(apdu, inLen, 9, &out);
    zassert_equal(inLen, outLen, NULL);
    zassert_false(islessgreater(in, out), NULL);
    zassert_equal(outLen2, 0, NULL);

    inLen = encode_context_double(apdu, large_context_tag, in);
    outLen = bacnet_double_context_decode(apdu, inLen, large_context_tag, &out);
    outLen2 =
        bacnet_double_context_decode(apdu, inLen, large_context_tag - 1, &out);
    zassert_equal(inLen, outLen, NULL);
    zassert_false(islessgreater(in, out), NULL);
    zassert_equal(outLen2, 0, NULL);
    while (inLen) {
        inLen--;
        outLen =
            bacnet_double_context_decode(apdu, inLen, large_context_tag, &out);
        zassert_equal(outLen, BACNET_STATUS_ERROR, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testObjectIDContextDecodes)
#else
static void testObjectIDContextDecodes(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    int inLen;
    int outLen;
    int outLen2;
    uint8_t large_context_tag = 0xfe;
    /* 32 bit number */
    BACNET_OBJECT_TYPE in_type;
    uint32_t in_id;
    BACNET_OBJECT_TYPE out_type;
    uint32_t out_id;

    in_type = 0xde;
    in_id = 0xbeef;

    inLen = encode_context_object_id(apdu, 10, in_type, in_id);
    outLen =
        bacnet_object_id_context_decode(apdu, inLen, 10, &out_type, &out_id);
    outLen2 =
        bacnet_object_id_context_decode(apdu, inLen, 9, &out_type, &out_id);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in_type, out_type, NULL);
    zassert_equal(in_id, out_id, NULL);
    zassert_equal(outLen2, 0, NULL);

    inLen = encode_context_object_id(apdu, large_context_tag, in_type, in_id);
    outLen = bacnet_object_id_context_decode(
        apdu, inLen, large_context_tag, &out_type, &out_id);
    outLen2 = bacnet_object_id_context_decode(
        apdu, inLen, large_context_tag - 1, &out_type, &out_id);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in_type, out_type, NULL);
    zassert_equal(in_id, out_id, NULL);
    zassert_equal(outLen2, 0, NULL);
    while (inLen) {
        inLen--;
        outLen = bacnet_object_id_context_decode(
            apdu, inLen, large_context_tag, &out_type, &out_id);
        zassert_equal(outLen, BACNET_STATUS_ERROR, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testCharacterStringContextDecodes)
#else
static void testCharacterStringContextDecodes(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    int inLen;
    int outLen;
    int outLen2;
    uint8_t large_context_tag = 0xfe;

    BACNET_CHARACTER_STRING in;
    BACNET_CHARACTER_STRING out;

    characterstring_init_ansi(&in, "This is a test");

    inLen = encode_context_character_string(apdu, 10, &in);
    outLen = decode_context_character_string(apdu, 10, &out);
    outLen2 = decode_context_character_string(apdu, 9, &out);

    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.length, out.length, NULL);
    zassert_equal(in.encoding, out.encoding, NULL);
    zassert_equal(strcmp(in.value, out.value), 0, NULL);

    inLen = encode_context_character_string(apdu, large_context_tag, &in);
    outLen = decode_context_character_string(apdu, large_context_tag, &out);
    outLen2 =
        decode_context_character_string(apdu, large_context_tag - 1, &out);

    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.length, out.length, NULL);
    zassert_equal(in.encoding, out.encoding, NULL);
    zassert_equal(strcmp(in.value, out.value), 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testBitStringContextDecodes)
#else
static void testBitStringContextDecodes(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    int inLen;
    int outLen;
    int outLen2;
    uint8_t large_context_tag = 0xfe;

    BACNET_BIT_STRING in;
    BACNET_BIT_STRING out;

    bitstring_init(&in);
    bitstring_set_bit(&in, 1, true);
    bitstring_set_bit(&in, 3, true);
    bitstring_set_bit(&in, 6, true);
    bitstring_set_bit(&in, 10, false);
    bitstring_set_bit(&in, 11, true);
    bitstring_set_bit(&in, 12, false);

    inLen = encode_context_bitstring(apdu, 10, &in);
    outLen = bacnet_bitstring_context_decode(apdu, inLen, 10, &out);
    outLen2 = bacnet_bitstring_context_decode(apdu, inLen, 9, &out);
    zassert_equal(outLen2, 0, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.bits_used, out.bits_used, NULL);
    zassert_equal(memcmp(in.value, out.value, MAX_BITSTRING_BYTES), 0, NULL);
    while (inLen) {
        inLen--;
        outLen = bacnet_bitstring_context_decode(apdu, inLen, 10, &out);
        zassert_equal(outLen, BACNET_STATUS_ERROR, NULL);
    }

    inLen = encode_context_bitstring(apdu, large_context_tag, &in);
    outLen =
        bacnet_bitstring_context_decode(apdu, inLen, large_context_tag, &out);
    outLen2 = bacnet_bitstring_context_decode(
        apdu, inLen, large_context_tag - 1, &out);
    zassert_equal(outLen2, 0, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.bits_used, out.bits_used, NULL);
    zassert_equal(memcmp(in.value, out.value, MAX_BITSTRING_BYTES), 0, NULL);
    while (inLen) {
        inLen--;
        outLen = bacnet_bitstring_context_decode(
            apdu, inLen, large_context_tag, &out);
        zassert_equal(
            outLen, BACNET_STATUS_ERROR, "inLen=%d outLen=%d", inLen, outLen);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testOctetStringContextDecodes)
#else
static void testOctetStringContextDecodes(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    int inLen;
    int outLen;
    int outLen2;
    uint8_t large_context_tag = 0xfe;

    BACNET_OCTET_STRING in;
    BACNET_OCTET_STRING out;

    uint8_t initData[] = { 0xde, 0xad, 0xbe, 0xef };

    octetstring_init(&in, initData, sizeof(initData));

    inLen = encode_context_octet_string(apdu, 10, &in);
    outLen = bacnet_octet_string_context_decode(apdu, inLen, 10, &out);
    outLen2 = bacnet_octet_string_context_decode(apdu, inLen, 9, &out);

    zassert_equal(outLen2, 0, NULL);
    zassert_equal(inLen, outLen, "inLen=%d outLen=%d", inLen, outLen);
    zassert_equal(in.length, out.length, NULL);
    zassert_true(octetstring_value_same(&in, &out), NULL);

    inLen = encode_context_octet_string(apdu, large_context_tag, &in);
    outLen = bacnet_octet_string_context_decode(
        apdu, inLen, large_context_tag, &out);
    outLen2 = bacnet_octet_string_context_decode(
        apdu, inLen, large_context_tag - 1, &out);

    zassert_equal(outLen2, 0, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.length, out.length, NULL);
    zassert_true(octetstring_value_same(&in, &out), NULL);
    while (inLen) {
        inLen--;
        outLen2 = bacnet_octet_string_context_decode(
            apdu, inLen, large_context_tag, &out);
        zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testTimeContextDecodes)
#else
static void testTimeContextDecodes(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    int inLen;
    int outLen;
    int outLen2;
    uint8_t large_context_tag = 0xfe;

    BACNET_TIME in;
    BACNET_TIME out;

    in.hour = 10;
    in.hundredths = 20;
    in.min = 30;
    in.sec = 40;

    inLen = encode_context_time(apdu, 10, &in);
    outLen = bacnet_time_context_decode(apdu, inLen, 10, &out);
    outLen2 = bacnet_time_context_decode(apdu, inLen, 9, &out);

    zassert_equal(outLen2, 0, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.hour, out.hour, NULL);
    zassert_equal(in.hundredths, out.hundredths, NULL);
    zassert_equal(in.min, out.min, NULL);
    zassert_equal(in.sec, out.sec, NULL);

    inLen = encode_context_time(apdu, large_context_tag, &in);
    outLen = bacnet_time_context_decode(apdu, inLen, large_context_tag, &out);
    outLen2 =
        bacnet_time_context_decode(apdu, inLen, large_context_tag - 1, &out);

    zassert_equal(outLen2, 0, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.hour, out.hour, NULL);
    zassert_equal(in.hundredths, out.hundredths, NULL);
    zassert_equal(in.min, out.min, NULL);
    zassert_equal(in.sec, out.sec, NULL);

    while (inLen) {
        inLen--;
        outLen2 =
            bacnet_time_context_decode(apdu, inLen, large_context_tag, &out);
        zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testDateContextDecodes)
#else
static void testDateContextDecodes(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    int inLen;
    int outLen;
    int outLen2;
    uint8_t large_context_tag = 0xfe;

    BACNET_DATE in;
    BACNET_DATE out;

    in.day = 3;
    in.month = 10;
    in.wday = 5;
    in.year = 1945;

    inLen = encode_context_date(apdu, 10, &in);
    outLen = bacnet_date_context_decode(apdu, inLen, 10, &out);
    outLen2 = bacnet_date_context_decode(apdu, inLen, 9, &out);

    zassert_equal(outLen2, 0, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.day, out.day, NULL);
    zassert_equal(in.month, out.month, NULL);
    zassert_equal(in.wday, out.wday, NULL);
    zassert_equal(in.year, out.year, NULL);

    /* Test large tags */
    inLen = encode_context_date(apdu, large_context_tag, &in);
    outLen = bacnet_date_context_decode(apdu, inLen, large_context_tag, &out);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.day, out.day, NULL);
    zassert_equal(in.month, out.month, NULL);
    zassert_equal(in.wday, out.wday, NULL);
    zassert_equal(in.year, out.year, NULL);
    /* incorrect tag */
    outLen2 =
        bacnet_date_context_decode(apdu, inLen, large_context_tag - 1, &out);
    zassert_equal(outLen2, 0, NULL);
    /* short APDU */
    while (inLen) {
        inLen--;
        outLen2 =
            bacnet_date_context_decode(apdu, inLen, large_context_tag, &out);
        zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, testDateRangeContextDecodes)
#else
static void testDateRangeContextDecodes(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    int len;
    int null_len;
    int test_len;

    BACNET_DATE_RANGE data;
    BACNET_DATE_RANGE test_data;
    memset(&data, 0, sizeof(data));
    memset(&test_data, 0, sizeof(test_data));

    data.startdate.day = 3;
    data.startdate.month = 10;
    data.startdate.wday = 5;
    data.startdate.year = 1945;

    data.enddate.day = 24;
    data.enddate.month = 8;
    data.enddate.wday = 4;
    data.enddate.year = 2023;

    len = bacnet_daterange_context_encode(apdu, 10, &data);
    null_len = bacnet_daterange_context_encode(NULL, 10, &data);
    zassert_equal(len, null_len, NULL);
    test_len = bacnet_daterange_context_decode(apdu, len, 10, &test_data);
    zassert_equal(len, test_len, NULL);
    zassert_equal(data.startdate.day, test_data.startdate.day, NULL);
    zassert_equal(data.startdate.month, test_data.startdate.month, NULL);
    zassert_equal(data.startdate.wday, test_data.startdate.wday, NULL);
    zassert_equal(data.startdate.year, test_data.startdate.year, NULL);
    zassert_equal(data.enddate.day, test_data.enddate.day, NULL);
    zassert_equal(data.enddate.month, test_data.enddate.month, NULL);
    zassert_equal(data.enddate.wday, test_data.enddate.wday, NULL);
    zassert_equal(data.enddate.year, test_data.enddate.year, NULL);
    /* incorrect tag number */
    test_len = bacnet_daterange_context_decode(apdu, len, 9, &test_data);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
}

/**
 * @brief Encode a BACnetARRAY property element; a function template
 * @param object_instance [in] BACnet network port object instance number
 * @param apdu_index [in] apdu index requested:
 *    0 to N for individual apdu members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
static int bacnet_apdu_property_element_encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX apdu_index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;

    if (apdu_index < 1) {
        apdu_len =
            encode_application_object_id(apdu, OBJECT_DEVICE, object_instance);
    }

    return apdu_len;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacdcode_tests, test_bacnet_array_encode)
#else
static void test_bacnet_array_encode(void)
#endif
{
    uint32_t object_instance = 0;
    BACNET_ARRAY_INDEX apdu_index = 0;
    BACNET_UNSIGNED_INTEGER apdu_size = 1;
    uint8_t apdu[480] = { 0 };
    int apdu_len, len;
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    BACNET_UNSIGNED_INTEGER decoded_value = 0;
    BACNET_TAG tag = { 0 };

    /* element zero returns the apdu size */
    apdu_len = bacnet_array_encode(
        object_instance, apdu_index, bacnet_apdu_property_element_encode,
        apdu_size, apdu, sizeof(apdu));
    zassert_true(apdu_len > 0, NULL);
    len = bacnet_tag_decode(apdu, apdu_len, &tag);
    zassert_true(len > 0, NULL);
    zassert_equal(tag.number, BACNET_APPLICATION_TAG_UNSIGNED_INT, NULL);
    len = bacnet_unsigned_decode(
        &apdu[len], apdu_len - len, tag.len_value_type, &decoded_value);
    zassert_equal(decoded_value, apdu_size, NULL);
    /* element zero - APDU too small */
    apdu_len = bacnet_array_encode(
        object_instance, apdu_index, bacnet_apdu_property_element_encode,
        apdu_size, apdu, 1);
    zassert_true(apdu_len == BACNET_STATUS_ABORT, NULL);
    /* element 1 returns the first element */
    apdu_index = 1;
    apdu_len = bacnet_array_encode(
        object_instance, apdu_index, bacnet_apdu_property_element_encode,
        apdu_size, apdu, sizeof(apdu));
    zassert_true(apdu_len > 0, NULL);
    len = decode_tag_number_and_value(apdu, &tag_number, &len_value);
    zassert_true(len > 0, NULL);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_OBJECT_ID, NULL);
    /* element 1 - APDU too small */
    apdu_len = bacnet_array_encode(
        object_instance, apdu_index, bacnet_apdu_property_element_encode,
        apdu_size, apdu, 1);
    zassert_true(apdu_len == BACNET_STATUS_ABORT, NULL);
    /* element 2, in this test case, returns an error */
    apdu_index = 2;
    apdu_len = bacnet_array_encode(
        object_instance, apdu_index, bacnet_apdu_property_element_encode,
        apdu_size, apdu, sizeof(apdu));
    zassert_true(apdu_len < 0, NULL);
    /* ALL - fits in APDU */
    apdu_index = BACNET_ARRAY_ALL;
    apdu_len = bacnet_array_encode(
        object_instance, apdu_index, bacnet_apdu_property_element_encode,
        apdu_size, apdu, sizeof(apdu));
    zassert_true(apdu_len == 5, "len=%d", apdu_len);
    /* ALL - APDU too small */
    apdu_len = bacnet_array_encode(
        object_instance, apdu_index, bacnet_apdu_property_element_encode,
        apdu_size, apdu, 4);
    zassert_true(apdu_len == BACNET_STATUS_ABORT, NULL);
}

/**
 * @}
 */
/* clang-format off */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacdcode_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bacdcode_tests, ztest_unit_test(testBACnetTagEncoder),
        ztest_unit_test(testBACnetTagCodec),
        ztest_unit_test(testBACDCodeTags),
        ztest_unit_test(testBACDCodeReal),
        ztest_unit_test(testBACDCodeUnsigned),
        ztest_unit_test(testBACnetUnsigned),
        ztest_unit_test(testBACDCodeSigned),
        ztest_unit_test(testBACnetSigned),
        ztest_unit_test(testBACnetDateDecodes),
        ztest_unit_test(testBACnetDateRangeDecodes),
        ztest_unit_test(testBACDCodeEnumerated),
        ztest_unit_test(testBACDCodeOctetString),
        ztest_unit_test(testBACDCodeCharacterString),
        ztest_unit_test(testBACDCodeObject),
        ztest_unit_test(testBACDCodeMaxSegsApdu),
        ztest_unit_test(testBACDCodeBitString),
        ztest_unit_test(testUnsignedContextDecodes),
        ztest_unit_test(testSignedContextDecodes),
        ztest_unit_test(testEnumeratedContextDecodes),
        ztest_unit_test(testCharacterStringContextDecodes),
        ztest_unit_test(testFloatContextDecodes),
        ztest_unit_test(testDoubleContextDecodes),
        ztest_unit_test(testObjectIDContextDecodes),
        ztest_unit_test(testBitStringContextDecodes),
        ztest_unit_test(testTimeContextDecodes),
        ztest_unit_test(testDateContextDecodes),
        ztest_unit_test(testDateRangeContextDecodes),
        ztest_unit_test(testOctetStringContextDecodes),
        ztest_unit_test(testBACDCodeDouble),
        ztest_unit_test(test_bacnet_array_encode));

    ztest_run_test_suite(bacdcode_tests);
}
#endif
/* clang-format on */
