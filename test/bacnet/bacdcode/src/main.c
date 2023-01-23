/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ctype.h>  /* For isprint */
#include <zephyr/ztest.h>
#include <bacnet/bacdcode.h>

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

static void print_apdu(uint8_t *pBlock, uint32_t num)
{
    size_t lines = 0; /* number of lines to print */
    size_t line = 0; /* line of text counter */
    size_t last_line = 0; /* line on which the last text resided */
    unsigned long count = 0; /* address to print */
    unsigned int i = 0; /* counter */

    if (pBlock && num) {
        /* how many lines to print? */
        num--; /* adjust */
        lines = (num / 16) + 1;
        last_line = num % 16;

        /* create the line */
        for (line = 0; line < lines; line++) {
            /* start with the address */
            printf("%08lX: ", count);
            /* hex representation */
            for (i = 0; i < 16; i++) {
                if (((line == (lines - 1)) && (i <= last_line)) ||
                    (line != (lines - 1))) {
                    printf("%02X ", (unsigned)(0x00FF & pBlock[i]));
                } else {
                    printf("-- ");
                }
            }
            printf(" ");
            /* print the characters if valid */
            for (i = 0; i < 16; i++) {
                if (((line == (lines - 1)) && (i <= last_line)) ||
                    (line != (lines - 1))) {
                    if (isprint(pBlock[i])) {
                        printf("%c", pBlock[i]);
                    } else {
                        printf(".");
                    }
                } else {
                    printf(".");
                }
            }
            printf("\r\n");
            pBlock += 16;
            count += 16;
        }
    }

    return;
}

static void testBACDCodeTags(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    uint8_t tag_number = 0, test_tag_number = 0;
    int len = 0, test_len = 0;
    uint32_t value = 0, test_value = 0;

    for (tag_number = 0;; tag_number++) {
        len = encode_opening_tag(&apdu[0], tag_number);
        test_len = get_apdu_len(IS_EXTENDED_TAG_NUMBER(apdu[0]), 0);
        zassert_equal(len, test_len, NULL);
        test_len = encode_opening_tag(NULL, tag_number);
        zassert_equal(len, test_len, NULL);
        len = decode_tag_number_and_value(&apdu[0], &test_tag_number, &value);
        zassert_equal(value, 0, NULL);
        zassert_equal(len, test_len, NULL);
        zassert_equal(tag_number, test_tag_number, NULL);
        zassert_true(IS_OPENING_TAG(apdu[0]), NULL);
        zassert_false(IS_CLOSING_TAG(apdu[0]), NULL);
        len = encode_closing_tag(&apdu[0], tag_number);
        zassert_equal(len, test_len, NULL);
        test_len = encode_closing_tag(NULL, tag_number);
        zassert_equal(len, test_len, NULL);
        len = decode_tag_number_and_value(&apdu[0], &test_tag_number, &value);
        zassert_equal(len, test_len, NULL);
        zassert_equal(value, 0, NULL);
        zassert_equal(tag_number, test_tag_number, NULL);
        zassert_false(IS_OPENING_TAG(apdu[0]), NULL);
        zassert_true(IS_CLOSING_TAG(apdu[0]), NULL);
        /* test the len-value-type portion */
        for (value = 1;; value = value << 1) {
            len = encode_tag(&apdu[0], tag_number, false, value);
            len = decode_tag_number_and_value(
                &apdu[0], &test_tag_number, &test_value);
            zassert_equal(tag_number, test_tag_number, NULL);
            zassert_equal(value, test_value, NULL);
            test_len = get_apdu_len(IS_EXTENDED_TAG_NUMBER(apdu[0]), value);
            zassert_equal(len, test_len, NULL);
            /* stop at the the last value */
            if (value & BIT(31L)) {
                break;
            }
        }
        /* stop after the last tag number */
        if (tag_number == 255) {
            break;
        }
    }

    return;
}

static void testBACDCodeEnumerated(void)
{
    uint8_t array[5] = { 0 };
    uint8_t encoded_array[5] = { 0 };
    uint32_t value = 1;
    uint32_t decoded_value = 0;
    int i = 0, apdu_len = 0;
    int len = 0, null_len = 0;
    uint8_t apdu[MAX_APDU] = { 0 };
    uint8_t tag_number = 0;
    uint32_t len_value = 0;

    for (i = 0; i < 31; i++) {
        apdu_len = encode_application_enumerated(&array[0], value);
        null_len = encode_application_enumerated(NULL, value);
        len = decode_tag_number_and_value(&array[0], &tag_number, &len_value);
        len += decode_enumerated(&array[len], len_value, &decoded_value);
        zassert_equal(decoded_value, value, NULL);
        zassert_equal(tag_number, BACNET_APPLICATION_TAG_ENUMERATED, NULL);
        zassert_equal(len, apdu_len, NULL);
        zassert_equal(null_len, apdu_len, NULL);
        /* encode back the value */
        encode_application_enumerated(&encoded_array[0], decoded_value);
        zassert_equal(memcmp(&array[0], &encoded_array[0], sizeof(array)), 0, NULL);
        /* an enumerated will take up to 4 octects */
        /* plus a one octet for the tag */
        apdu_len = encode_application_enumerated(&apdu[0], value);
        len = decode_tag_number_and_value(&apdu[0], &tag_number, NULL);
        zassert_equal(len, 1, NULL);
        zassert_equal(tag_number, BACNET_APPLICATION_TAG_ENUMERATED, NULL);
        zassert_false(IS_CONTEXT_SPECIFIC(apdu[0]), NULL);
        /* context specific encoding */
        apdu_len = encode_context_enumerated(&apdu[0], 3, value);
        null_len = encode_context_enumerated(NULL, 3, value);
        zassert_true(IS_CONTEXT_SPECIFIC(apdu[0]), NULL);
        len = decode_tag_number_and_value(&apdu[0], &tag_number, NULL);
        zassert_equal(len, 1, NULL);
        zassert_equal(tag_number, 3, NULL);
        zassert_equal(null_len, apdu_len, NULL);
        /* test the interesting values */
        value = value << 1;
    }

    return;
}

static void testBACDCodeReal(void)
{
    uint8_t real_array[4] = { 0 };
    uint8_t encoded_array[4] = { 0 };
    float value = 42.123F;
    float decoded_value = 0.0F;
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, apdu_len = 0, null_len = 0;
    uint8_t tag_number = 0;
    uint32_t long_value = 0;

    encode_bacnet_real(value, &real_array[0]);
    decode_real(&real_array[0], &decoded_value);
    zassert_equal(decoded_value, value, NULL);
    encode_bacnet_real(value, &encoded_array[0]);
    zassert_equal(memcmp(&real_array, &encoded_array, sizeof(real_array)), 0, NULL);

    /* a real will take up 4 octects plus a one octet tag */
    apdu_len = encode_application_real(&apdu[0], value);
    null_len = encode_application_real(NULL, value);
    zassert_equal(apdu_len, 5, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    /* len tells us how many octets were used for encoding the value */
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &long_value);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_REAL, NULL);
    zassert_false(IS_CONTEXT_SPECIFIC(apdu[0]), NULL);
    zassert_equal(len, 1, NULL);
    zassert_equal(long_value, 4, NULL);
    decode_real(&apdu[len], &decoded_value);
    zassert_equal(decoded_value, value, NULL);

    return;
}

static void testBACDCodeDouble(void)
{
    uint8_t double_array[8] = { 0 };
    uint8_t encoded_array[8] = { 0 };
    double value = 42.123;
    double decoded_value = 0.0;
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, apdu_len = 0, null_len = 0;
    uint8_t tag_number = 0;
    uint32_t long_value = 0;

    encode_bacnet_double(value, &double_array[0]);
    decode_double(&double_array[0], &decoded_value);
    zassert_equal(decoded_value, value, NULL);
    encode_bacnet_double(value, &encoded_array[0]);
    zassert_equal(memcmp(&double_array, &encoded_array, sizeof(double_array)), 0, NULL);

    /* a real will take up 4 octects plus a one octet tag */
    apdu_len = encode_application_double(&apdu[0], value);
    null_len = encode_application_double(NULL, value);
    zassert_equal(apdu_len, 10, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    /* len tells us how many octets were used for encoding the value */
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &long_value);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_DOUBLE, NULL);
    zassert_false(IS_CONTEXT_SPECIFIC(apdu[0]), NULL);
    zassert_equal(len, 2, NULL);
    zassert_equal(long_value, 8, NULL);
    decode_double(&apdu[len], &decoded_value);
    zassert_equal(decoded_value, value, NULL);

    return;
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
    zassert_equal(decoded_value, value, NULL);
    if (decoded_value != value) {
        printf("value=%lu decoded_value=%lu\n", (unsigned long)value,
            (unsigned long)decoded_value);
        print_apdu(&array[0], sizeof(array));
    }
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

static void testBACDCodeUnsigned(void)
{
#ifdef UINT64_MAX
    const unsigned max_bits = 64;
#else
    const unsigned max_bits = 32;
#endif
    uint32_t value = 1;
    int i;

    for (i = 0; i < max_bits; i++) {
        verifyBACDCodeUnsignedValue(value - 1);
        verifyBACDCodeUnsignedValue(value);
        verifyBACDCodeUnsignedValue(value + 1);
        value |= (value << 1);
    }

    return;
}

static void testBACnetUnsigned(void)
{
    uint8_t apdu[32] = { 0 };
    BACNET_UNSIGNED_INTEGER value = 1, test_value = 0;
    int len = 0, test_len = 0, null_len = 0;
    unsigned i;
#ifdef UINT64_MAX
    const unsigned max_bits = 64;
#else
    const unsigned max_bits = 32;
#endif

    for (i = 0; i < max_bits; i++) {
        len = encode_bacnet_unsigned(&apdu[0], value);
        null_len = encode_bacnet_unsigned(NULL, value);
        test_len = decode_unsigned(&apdu[0], len, &test_value);
        zassert_equal(len, null_len, NULL);
        zassert_equal(len, test_len, NULL);
        zassert_equal(value, test_value, NULL);
        value |= (value << 1);
    }
}

static void testBACDCodeSignedValue(int32_t value)
{
    uint8_t array[5] = { 0 };
    uint8_t encoded_array[5] = { 0 };
    int32_t decoded_value = 0;
    int len = 0, null_len = 0;
    uint8_t apdu[MAX_APDU] = { 0 };
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    int diff = 0;

    len = encode_application_signed(&array[0], value);
    null_len = encode_application_signed(NULL, value);
    zassert_equal(null_len, len, NULL);
    len = decode_tag_number_and_value(&array[0], &tag_number, &len_value);
    len = decode_signed(&array[len], len_value, &decoded_value);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_SIGNED_INT, NULL);
    zassert_equal(decoded_value, value, NULL);
    if (decoded_value != value) {
        printf(
            "value=%ld decoded_value=%ld\n", (long)value, (long)decoded_value);
        print_apdu(&array[0], sizeof(array));
    }
    len = encode_application_signed(&encoded_array[0], decoded_value);
    null_len = encode_application_signed(NULL, decoded_value);
    zassert_equal(null_len, len, NULL);
    diff = memcmp(&array[0], &encoded_array[0], sizeof(array));
    zassert_equal(diff, 0, NULL);
    if (diff) {
        printf(
            "value=%ld decoded_value=%ld\n", (long)value, (long)decoded_value);
        print_apdu(&array[0], sizeof(array));
        print_apdu(&encoded_array[0], sizeof(array));
    }
    /* a signed int will take up to 4 octects */
    /* plus a one octet for the tag */
    len = encode_application_signed(&apdu[0], value);
    null_len = encode_application_signed(NULL, value);
    zassert_equal(null_len, len, NULL);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, NULL);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_SIGNED_INT, NULL);
    zassert_false(IS_CONTEXT_SPECIFIC(apdu[0]), NULL);

    return;
}

static void testBACDCodeSigned(void)
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

static void testBACnetSigned(void)
{
    uint8_t apdu[32] = { 0 };
    int32_t value = 0, test_value = 0;
    int len = 0, test_len = 0, null_len = 0;
    unsigned i = 0;

    value = -2147483647;
    for (i = 0; i < 32; i++) {
        len = encode_bacnet_signed(&apdu[0], value);
        null_len = encode_bacnet_signed(NULL, value);
        test_len = decode_signed(&apdu[0], len, &test_value);
        zassert_equal(len, null_len, NULL);
        zassert_equal(len, test_len, NULL);
        zassert_equal(value, test_value, NULL);
        value /= 2;
    }
    value = 2147483647;
    for (i = 0; i < 32; i++) {
        len = encode_bacnet_signed(&apdu[0], value);
        null_len = encode_bacnet_signed(NULL, value);
        test_len = decode_signed(&apdu[0], len, &test_value);
        zassert_equal(len, null_len, NULL);
        zassert_equal(len, test_len, NULL);
        zassert_equal(value, test_value, NULL);
        value /= 2;
    }
}

static void testBACDCodeOctetString(void)
{
    uint8_t array[MAX_APDU] = { 0 };
    uint8_t encoded_array[MAX_APDU] = { 0 };
    BACNET_OCTET_STRING octet_string;
    BACNET_OCTET_STRING test_octet_string;
    uint8_t test_value[MAX_APDU] = { "" };
    int i; /* for loop counter */
    int apdu_len = 0, len = 0, null_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    bool status = false;
    int diff = 0; /* for memcmp */

    status = octetstring_init(&octet_string, NULL, 0);
    zassert_true(status, NULL);
    apdu_len = encode_application_octet_string(&array[0], &octet_string);
    null_len = encode_application_octet_string(NULL, &octet_string);
    zassert_equal(apdu_len, null_len, NULL);
    len = decode_tag_number_and_value(&array[0], &tag_number, &len_value);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_OCTET_STRING, NULL);
    len += decode_octet_string(&array[len], len_value, &test_octet_string);
    zassert_equal(apdu_len, len, NULL);
    diff = memcmp(octetstring_value(&octet_string), &test_value[0],
        octetstring_length(&octet_string));
    zassert_equal(diff, 0, NULL);

    for (i = 0; i < (MAX_APDU - 6); i++) {
        test_value[i] = '0' + (i % 10);
        status = octetstring_init(&octet_string, test_value, i);
        zassert_true(status, NULL);
        apdu_len =
            encode_application_octet_string(&encoded_array[0], &octet_string);
        null_len = encode_application_octet_string(NULL, &octet_string);
        zassert_equal(apdu_len, null_len, NULL);
        len = decode_tag_number_and_value(
            &encoded_array[0], &tag_number, &len_value);
        zassert_equal(tag_number, BACNET_APPLICATION_TAG_OCTET_STRING, NULL);
        len += decode_octet_string(
            &encoded_array[len], len_value, &test_octet_string);
        if (apdu_len != len) {
            printf("test octet string=#%d\n", i);
        }
        zassert_equal(apdu_len, len, NULL);
        diff = memcmp(octetstring_value(&octet_string), &test_value[0],
            octetstring_length(&octet_string));
        if (diff) {
            printf("test octet string=#%d\n", i);
        }
        zassert_equal(diff, 0, NULL);
    }

    return;
}

static void testBACDCodeCharacterString(void)
{
    uint8_t array[MAX_APDU] = { 0 };
    uint8_t encoded_array[MAX_APDU] = { 0 };
    BACNET_CHARACTER_STRING char_string;
    BACNET_CHARACTER_STRING test_char_string;
    char test_value[MAX_APDU] = { "" };
    int i; /* for loop counter */
    int apdu_len = 0, len = 0, null_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    int diff = 0; /* for comparison */
    bool status = false;

    status = characterstring_init(&char_string, CHARACTER_ANSI_X34, NULL, 0);
    zassert_true(status, NULL);
    apdu_len = encode_application_character_string(&array[0], &char_string);
    null_len = encode_application_character_string(NULL, &char_string);
    zassert_equal(apdu_len, null_len, NULL);
    len = decode_tag_number_and_value(&array[0], &tag_number, &len_value);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    len += decode_character_string(&array[len], len_value, &test_char_string);
    zassert_equal(apdu_len, len, NULL);
    diff = memcmp(characterstring_value(&char_string), &test_value[0],
        characterstring_length(&char_string));
    zassert_equal(diff, 0, NULL);
    for (i = 0; i < MAX_CHARACTER_STRING_BYTES - 1; i++) {
        test_value[i] = 'S';
        test_value[i + 1] = '\0';
        status = characterstring_init_ansi(&char_string, test_value);
        zassert_true(status, NULL);
        apdu_len = encode_application_character_string(
            &encoded_array[0], &char_string);
        null_len = encode_application_character_string(NULL, &char_string);
        zassert_equal(apdu_len, null_len, NULL);
        len = decode_tag_number_and_value(
            &encoded_array[0], &tag_number, &len_value);
        zassert_equal(tag_number, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
        len += decode_character_string(
            &encoded_array[len], len_value, &test_char_string);
        if (apdu_len != len) {
            printf("test string=#%d apdu_len=%d len=%d\n", i, apdu_len, len);
        }
        zassert_equal(apdu_len, len, NULL);
        diff = memcmp(characterstring_value(&char_string), &test_value[0],
            characterstring_length(&char_string));
        if (diff) {
            printf("test string=#%d\n", i);
        }
        zassert_equal(diff, 0, NULL);
    }

    return;
}

static void testBACDCodeObject(void)
{
    uint8_t object_array[32] = { 0 };
    uint8_t encoded_array[32] = { 0 };
    BACNET_OBJECT_TYPE type = OBJECT_BINARY_INPUT;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_ANALOG_OUTPUT;
    uint32_t instance = 123;
    uint32_t decoded_instance = 0;
    int apdu_len = 0, len = 0, null_len = 0;
    uint8_t tag_number = 0;

    apdu_len = encode_bacnet_object_id(&encoded_array[0], type, instance);
    null_len = encode_bacnet_object_id(NULL, type, instance);
    zassert_equal(apdu_len, null_len, NULL);
    decode_object_id(&encoded_array[0], &decoded_type, &decoded_instance);
    zassert_equal(decoded_type, type, NULL);
    zassert_equal(decoded_instance, instance, NULL);
    encode_bacnet_object_id(&object_array[0], type, instance);
    zassert_equal(memcmp(&object_array[0], &encoded_array[0], sizeof(object_array)), 0, NULL);
    for (type = 0; type < 1024; type++) {
        for (instance = 0; instance <= BACNET_MAX_INSTANCE; instance += 1024) {
            /* test application encoded */
            len =
                encode_application_object_id(&encoded_array[0], type, instance);
            null_len = encode_application_object_id(NULL, type, instance);
            zassert_equal(len, null_len, NULL);
            zassert_true(len > 0, NULL);
            bacnet_object_id_application_decode(
                &encoded_array[0], len, &decoded_type, &decoded_instance);
            zassert_equal(decoded_type, type, NULL);
            zassert_equal(decoded_instance, instance, NULL);
            /* test context encoded */
            tag_number = 99;
            len = encode_context_object_id(
                &encoded_array[0], tag_number, type, instance);
            zassert_true(len > 0, NULL);
            len = decode_context_object_id(&encoded_array[0], tag_number,
                &decoded_type, &decoded_instance);
            zassert_true(len > 0, NULL);
            zassert_equal(decoded_type, type, NULL);
            zassert_equal(decoded_instance, instance, NULL);
            tag_number = 100;
            len = decode_context_object_id(&encoded_array[0], tag_number,
                &decoded_type, &decoded_instance);
            zassert_equal(len, BACNET_STATUS_ERROR, NULL);
        }
    }
    /* test context encoded */
    tag_number = 1;
    type = OBJECT_BINARY_INPUT;
    instance = 123;
    for (tag_number = 0; tag_number < 254; tag_number++) {
        len = encode_context_object_id(
            &encoded_array[0], tag_number, type, instance);
        zassert_true(len > 0, NULL);
        null_len = encode_context_object_id(NULL, tag_number, type, instance);
        zassert_equal(len, null_len, NULL);
        len = decode_context_object_id(
            &encoded_array[0], tag_number, &decoded_type, &decoded_instance);
        zassert_true(len > 0, NULL);
        zassert_equal(decoded_type, type, NULL);
        zassert_equal(decoded_instance, instance, NULL);
        len = decode_context_object_id(
            &encoded_array[0], 254, &decoded_type, &decoded_instance);
        zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    }

    return;
}

static void testBACDCodeMaxSegsApdu(void)
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

static void testBACDCodeBitString(void)
{
    uint8_t bit = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_BIT_STRING decoded_bit_string;
    uint8_t apdu[MAX_APDU] = { 0 };
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    int len = 0, null_len = 0;

    bitstring_init(&bit_string);
    /* verify initialization */
    zassert_equal(bitstring_bits_used(&bit_string), 0, NULL);
    for (bit = 0; bit < (MAX_BITSTRING_BYTES * 8); bit++) {
        zassert_false(bitstring_bit(&bit_string, bit), NULL);
    }
    /* test encode/decode -- true */
    for (bit = 0; bit < (MAX_BITSTRING_BYTES * 8); bit++) {
        bitstring_set_bit(&bit_string, bit, true);
        zassert_equal(bitstring_bits_used(&bit_string), (bit + 1), NULL);
        zassert_true(bitstring_bit(&bit_string, bit), NULL);
        /* encode */
        len = encode_application_bitstring(&apdu[0], &bit_string);
        null_len = encode_application_bitstring(NULL, &bit_string);
        zassert_equal(len, null_len, NULL);
        /* decode */
        len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
        zassert_equal(tag_number, BACNET_APPLICATION_TAG_BIT_STRING, NULL);
        len += decode_bitstring(&apdu[len], len_value, &decoded_bit_string);
        zassert_equal(bitstring_bits_used(&decoded_bit_string), (bit + 1), NULL);
        zassert_true(bitstring_bit(&decoded_bit_string, bit), NULL);
    }
    /* test encode/decode -- false */
    bitstring_init(&bit_string);
    for (bit = 0; bit < (MAX_BITSTRING_BYTES * 8); bit++) {
        bitstring_set_bit(&bit_string, bit, false);
        zassert_equal(bitstring_bits_used(&bit_string), (bit + 1), NULL);
        zassert_false(bitstring_bit(&bit_string, bit), NULL);
        /* encode */
        len = encode_application_bitstring(&apdu[0], &bit_string);
        null_len = encode_application_bitstring(NULL, &bit_string);
        zassert_equal(len, null_len, NULL);
        /* decode */
        len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
        zassert_equal(tag_number, BACNET_APPLICATION_TAG_BIT_STRING, NULL);
        len += decode_bitstring(&apdu[len], len_value, &decoded_bit_string);
        zassert_equal(bitstring_bits_used(&decoded_bit_string), (bit + 1), NULL);
        zassert_false(bitstring_bit(&decoded_bit_string, bit), NULL);
    }
}

static void testUnsignedContextDecodes(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int inLen = 0;
    int outLen = 0;
    int outLen2 = 0;
    uint8_t large_context_tag = 0xfe;
    BACNET_UNSIGNED_INTEGER in = 0xdeadbeef;
    BACNET_UNSIGNED_INTEGER out = 0;

    /* error, stack-overflow check */
    outLen2 = decode_context_unsigned(apdu, 9, &out);

#ifdef UINT64_MAX
    /* 64 bit number */
    in = 0xdeadbeefdeadbeef;
    inLen = encode_context_unsigned(apdu, 10, in);
    outLen = decode_context_unsigned(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    inLen = encode_context_unsigned(apdu, large_context_tag, in);
    outLen = decode_context_unsigned(apdu, large_context_tag, &out);
    outLen2 = decode_context_unsigned(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
#endif

    /* 32 bit number */
    in = 0xdeadbeef;
    inLen = encode_context_unsigned(apdu, 10, in);
    outLen = decode_context_unsigned(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    inLen = encode_context_unsigned(apdu, large_context_tag, in);
    outLen = decode_context_unsigned(apdu, large_context_tag, &out);
    outLen2 = decode_context_unsigned(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    /* 16 bit number */
    in = 0xdead;
    inLen = encode_context_unsigned(apdu, 10, in);
    outLen = decode_context_unsigned(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);

    inLen = encode_context_unsigned(apdu, large_context_tag, in);
    outLen = decode_context_unsigned(apdu, large_context_tag, &out);
    outLen2 = decode_context_unsigned(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    /* 8 bit number */
    in = 0xde;
    inLen = encode_context_unsigned(apdu, 10, in);
    outLen = decode_context_unsigned(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);

    inLen = encode_context_unsigned(apdu, large_context_tag, in);
    outLen = decode_context_unsigned(apdu, large_context_tag, &out);
    outLen2 = decode_context_unsigned(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    /* 4 bit number */
    in = 0xd;
    inLen = encode_context_unsigned(apdu, 10, in);
    outLen = decode_context_unsigned(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);

    inLen = encode_context_unsigned(apdu, large_context_tag, in);
    outLen = decode_context_unsigned(apdu, large_context_tag, &out);
    outLen2 = decode_context_unsigned(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    /* 2 bit number */
    in = 0x2;
    inLen = encode_context_unsigned(apdu, 10, in);
    outLen = decode_context_unsigned(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);

    inLen = encode_context_unsigned(apdu, large_context_tag, in);
    outLen = decode_context_unsigned(apdu, large_context_tag, &out);
    outLen2 = decode_context_unsigned(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
}

static void testSignedContextDecodes(void)
{
    uint8_t apdu[MAX_APDU];
    int inLen;
    int outLen;
    int outLen2;
    uint8_t large_context_tag = 0xfe;

    /* 32 bit number */
    int32_t in = 0xdeadbeef;
    int32_t out;

    outLen2 = decode_context_signed(apdu, 9, &out);

    in = 0xdeadbeef;
    inLen = encode_context_signed(apdu, 10, in);
    outLen = decode_context_signed(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    inLen = encode_context_signed(apdu, large_context_tag, in);
    outLen = decode_context_signed(apdu, large_context_tag, &out);
    outLen2 = decode_context_signed(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    /* 16 bit number */
    in = 0xdead;
    inLen = encode_context_signed(apdu, 10, in);
    outLen = decode_context_signed(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);

    inLen = encode_context_signed(apdu, large_context_tag, in);
    outLen = decode_context_signed(apdu, large_context_tag, &out);
    outLen2 = decode_context_signed(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    /* 8 bit number */
    in = 0xde;
    inLen = encode_context_signed(apdu, 10, in);
    outLen = decode_context_signed(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);

    inLen = encode_context_signed(apdu, large_context_tag, in);
    outLen = decode_context_signed(apdu, large_context_tag, &out);
    outLen2 = decode_context_signed(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    /* 4 bit number */
    in = 0xd;
    inLen = encode_context_signed(apdu, 10, in);
    outLen = decode_context_signed(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);

    inLen = encode_context_signed(apdu, large_context_tag, in);
    outLen = decode_context_signed(apdu, large_context_tag, &out);
    outLen2 = decode_context_signed(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    /* 2 bit number */
    in = 0x2;
    inLen = encode_context_signed(apdu, 10, in);
    outLen = decode_context_signed(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);

    inLen = encode_context_signed(apdu, large_context_tag, in);
    outLen = decode_context_signed(apdu, large_context_tag, &out);
    outLen2 = decode_context_signed(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
}

static void testEnumeratedContextDecodes(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int inLen = 0;
    int outLen = 0;
    int outLen2 = 0;
    uint8_t large_context_tag = 0xfe;

    /* 32 bit number */
    uint32_t in = 0xdeadbeef;
    uint32_t out = 0;

    outLen2 = decode_context_enumerated(apdu, 9, &out);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    in = 0xdeadbeef;
    inLen = encode_context_enumerated(apdu, 10, in);
    outLen = decode_context_enumerated(apdu, 10, &out);
    outLen2 = decode_context_enumerated(apdu, 9, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    inLen = encode_context_enumerated(apdu, large_context_tag, in);
    outLen = decode_context_enumerated(apdu, large_context_tag, &out);
    outLen2 = decode_context_enumerated(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    /* 16 bit number */
    in = 0xdead;
    inLen = encode_context_enumerated(apdu, 10, in);
    outLen = decode_context_enumerated(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);

    inLen = encode_context_enumerated(apdu, large_context_tag, in);
    outLen = decode_context_enumerated(apdu, large_context_tag, &out);
    outLen2 = decode_context_enumerated(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    /* 8 bit number */
    in = 0xde;
    inLen = encode_context_enumerated(apdu, 10, in);
    outLen = decode_context_enumerated(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);

    inLen = encode_context_enumerated(apdu, large_context_tag, in);
    outLen = decode_context_enumerated(apdu, large_context_tag, &out);
    outLen2 = decode_context_enumerated(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    /* 4 bit number */
    in = 0xd;
    inLen = encode_context_enumerated(apdu, 10, in);
    outLen = decode_context_enumerated(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);

    inLen = encode_context_enumerated(apdu, large_context_tag, in);
    outLen = decode_context_enumerated(apdu, large_context_tag, &out);
    outLen2 = decode_context_enumerated(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    /* 2 bit number */
    in = 0x2;
    inLen = encode_context_enumerated(apdu, 10, in);
    outLen = decode_context_enumerated(apdu, 10, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);

    inLen = encode_context_enumerated(apdu, large_context_tag, in);
    outLen = decode_context_enumerated(apdu, large_context_tag, &out);
    outLen2 = decode_context_enumerated(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
}

static void testFloatContextDecodes(void)
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
    outLen = decode_context_real(apdu, 10, &out);
    outLen2 = decode_context_real(apdu, 9, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    inLen = encode_context_real(apdu, large_context_tag, in);
    outLen = decode_context_real(apdu, large_context_tag, &out);
    outLen2 = decode_context_real(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    in = 0.0f;
    inLen = encode_context_real(apdu, 10, in);
    outLen = decode_context_real(apdu, 10, &out);
    outLen2 = decode_context_real(apdu, 9, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);

    inLen = encode_context_real(apdu, large_context_tag, in);
    outLen = decode_context_real(apdu, large_context_tag, &out);
    outLen2 = decode_context_real(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
}

static void testDoubleContextDecodes(void)
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
    outLen = decode_context_double(apdu, 10, &out);
    outLen2 = decode_context_double(apdu, 9, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    inLen = encode_context_double(apdu, large_context_tag, in);
    outLen = decode_context_double(apdu, large_context_tag, &out);
    outLen2 = decode_context_double(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    in = 0.0;
    inLen = encode_context_double(apdu, 10, in);
    outLen = decode_context_double(apdu, 10, &out);
    outLen2 = decode_context_double(apdu, 9, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);

    inLen = encode_context_double(apdu, large_context_tag, in);
    outLen = decode_context_double(apdu, large_context_tag, &out);
    outLen2 = decode_context_double(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in, out, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
}

static void testObjectIDContextDecodes(void)
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
    outLen = decode_context_object_id(apdu, 10, &out_type, &out_id);
    outLen2 = decode_context_object_id(apdu, 9, &out_type, &out_id);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in_type, out_type, NULL);
    zassert_equal(in_id, out_id, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);

    inLen = encode_context_object_id(apdu, large_context_tag, in_type, in_id);
    outLen =
        decode_context_object_id(apdu, large_context_tag, &out_type, &out_id);
    outLen2 = decode_context_object_id(
        apdu, large_context_tag - 1, &out_type, &out_id);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in_type, out_type, NULL);
    zassert_equal(in_id, out_id, NULL);
    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
}

static void testCharacterStringContextDecodes(void)
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

static void testBitStringContextDecodes(void)
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
    outLen = decode_context_bitstring(apdu, 10, &out);
    outLen2 = decode_context_bitstring(apdu, 9, &out);

    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.bits_used, out.bits_used, NULL);
    zassert_equal(memcmp(in.value, out.value, MAX_BITSTRING_BYTES), 0, NULL);

    inLen = encode_context_bitstring(apdu, large_context_tag, &in);
    outLen = decode_context_bitstring(apdu, large_context_tag, &out);
    outLen2 = decode_context_bitstring(apdu, large_context_tag - 1, &out);

    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.bits_used, out.bits_used, NULL);
    zassert_equal(memcmp(in.value, out.value, MAX_BITSTRING_BYTES), 0, NULL);
}

static void testOctetStringContextDecodes(void)
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
    outLen = decode_context_octet_string(apdu, 10, &out);
    outLen2 = decode_context_octet_string(apdu, 9, &out);

    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.length, out.length, NULL);
    zassert_true(octetstring_value_same(&in, &out), NULL);

    inLen = encode_context_octet_string(apdu, large_context_tag, &in);
    outLen = decode_context_octet_string(apdu, large_context_tag, &out);
    outLen2 = decode_context_octet_string(apdu, large_context_tag - 1, &out);

    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.length, out.length, NULL);
    zassert_true(octetstring_value_same(&in, &out), NULL);
}

static void testTimeContextDecodes(void)
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
    outLen = decode_context_bacnet_time(apdu, 10, &out);
    outLen2 = decode_context_bacnet_time(apdu, 9, &out);

    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.hour, out.hour, NULL);
    zassert_equal(in.hundredths, out.hundredths, NULL);
    zassert_equal(in.min, out.min, NULL);
    zassert_equal(in.sec, out.sec, NULL);

    inLen = encode_context_time(apdu, large_context_tag, &in);
    outLen = decode_context_bacnet_time(apdu, large_context_tag, &out);
    outLen2 = decode_context_bacnet_time(apdu, large_context_tag - 1, &out);

    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.hour, out.hour, NULL);
    zassert_equal(in.hundredths, out.hundredths, NULL);
    zassert_equal(in.min, out.min, NULL);
    zassert_equal(in.sec, out.sec, NULL);
}

static void testDateContextDecodes(void)
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
    outLen = decode_context_date(apdu, 10, &out);
    outLen2 = decode_context_date(apdu, 9, &out);

    zassert_equal(outLen2, BACNET_STATUS_ERROR, NULL);
    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.day, out.day, NULL);
    zassert_equal(in.month, out.month, NULL);
    zassert_equal(in.wday, out.wday, NULL);
    zassert_equal(in.year, out.year, NULL);

    /* Test large tags */
    inLen = encode_context_date(apdu, large_context_tag, &in);
    outLen = decode_context_date(apdu, large_context_tag, &out);
    outLen2 = decode_context_date(apdu, large_context_tag - 1, &out);

    zassert_equal(inLen, outLen, NULL);
    zassert_equal(in.day, out.day, NULL);
    zassert_equal(in.month, out.month, NULL);
    zassert_equal(in.wday, out.wday, NULL);
    zassert_equal(in.year, out.year, NULL);
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(bacdcode_tests,
     ztest_unit_test(testBACDCodeTags),
     ztest_unit_test(testBACDCodeReal),
     ztest_unit_test(testBACDCodeUnsigned),
     ztest_unit_test(testBACnetUnsigned),
     ztest_unit_test(testBACDCodeSigned),
     ztest_unit_test(testBACnetSigned),
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
     ztest_unit_test(testOctetStringContextDecodes),
     ztest_unit_test(testBACDCodeDouble)
     );

    ztest_run_test_suite(bacdcode_tests);
}
