/**
 * @file
 * @brief test BACnet CharacterString, BitString, and OctetString APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <zephyr/ztest.h>
#include <bacnet/bacstr.h>

/**
 * @brief compare two double precision floating points to 3 decimal places
 * @param x1 - first comparison value
 * @param x2 - second comparison value
 * @return true if the value is the same to 3 decimal points
 */
static bool is_float_equal(double x1, double x2)
{
    return fabs(x1 - x2) < 0.001;
}

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test encode/decode API for strings
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacstr_tests, testBitString)
#else
static void testBitString(void)
#endif
{
    uint8_t bit = 0, octet = 0, octet_index = 0;
    uint8_t bits_used = 0, test_bits_used = 0, bits_unused = 0;
    int max_bit;
    BACNET_BIT_STRING bit_string;
    BACNET_BIT_STRING bit_string2;
    BACNET_BIT_STRING bit_string3;
    bool status = false;
    uint8_t bytes = 0;

    bitstring_init(&bit_string);
    /* verify initialization */
    zassert_equal(bitstring_bits_used(&bit_string), 0, NULL);
    for (bit = 0; bit < (MAX_BITSTRING_BYTES * 8); bit++) {
        zassert_false(bitstring_bit(&bit_string, bit), NULL);
    }
    zassert_equal(bitstring_bytes_used(&bit_string), 0, NULL);
    /* test for true */
    for (bit = 0; bit < (MAX_BITSTRING_BYTES * 8); bit++) {
        bitstring_set_bit(&bit_string, bit, true);
        bits_used = bitstring_bits_used(&bit_string);
        zassert_equal(bits_used, (bit + 1), NULL);
        zassert_true(bitstring_bit(&bit_string, bit), NULL);
        zassert_true(bitstring_bits_used_set(&bit_string, (bit + 1)), NULL);
        zassert_equal(bitstring_bits_used(&bit_string), (bit + 1), NULL);
        bytes = bitstring_bytes_used(&bit_string);
        zassert_true(bytes > 0, "bytes=%u", bit, bytes);
        /* manipulate the bitstring per octet */
        octet_index = bytes - 1;
        octet = bitstring_octet(&bit_string, octet_index);
        zassert_true(octet > 0, "octet=0x%02X byte=%u", octet, octet_index);
        zassert_true(
            bitstring_set_octet(&bit_string, octet_index, octet), NULL);
        /* manipulate the bitstring bits used based on the last set octet */
        bits_unused = 8 - (bits_used - (octet_index * 8));
        zassert_true(
            bitstring_set_bits_used(&bit_string, bytes, bits_unused), NULL);
        test_bits_used = bitstring_bits_used(&bit_string);
        zassert_equal(
            bits_used, test_bits_used,
            "bits_used=%u bits_unused=%u test_bits_used=%u", bits_used,
            bits_unused, test_bits_used);
    }
    /* test for false */
    bitstring_init(&bit_string);
    for (bit = 0; bit < (MAX_BITSTRING_BYTES * 8); bit++) {
        bitstring_set_bit(&bit_string, bit, false);
        zassert_equal(bitstring_bits_used(&bit_string), (bit + 1), NULL);
        zassert_false(bitstring_bit(&bit_string, bit), NULL);
    }

    /* test for compare equals */
    for (max_bit = 0; max_bit < (MAX_BITSTRING_BYTES * 8); max_bit++) {
        bitstring_init(&bit_string);
        bitstring_init(&bit_string2);
        bitstring_set_bit(&bit_string, bit, true);
        bitstring_set_bit(&bit_string2, bit, true);
        zassert_true(bitstring_same(&bit_string, &bit_string2), NULL);
    }
    /* test for compare not equals */
    for (max_bit = 1; max_bit < (MAX_BITSTRING_BYTES * 8); max_bit++) {
        bitstring_init(&bit_string);
        bitstring_init(&bit_string2);
        bitstring_init(&bit_string3);
        /* Set the first bit of bit_string2 and the last bit of bit_string3 to
         * be different */
        bitstring_set_bit(&bit_string2, 0, !bitstring_bit(&bit_string, 0));
        bitstring_set_bit(
            &bit_string3, max_bit - 1,
            !bitstring_bit(&bit_string, max_bit - 1));
        zassert_false(bitstring_same(&bit_string, &bit_string2), NULL);
        zassert_false(bitstring_same(&bit_string, &bit_string3), NULL);
    }
    status = bitstring_init_ascii(&bit_string, "1111000010100101");
    zassert_true(status, NULL);
    status = bitstring_init_ascii(&bit_string2, "1110000010101111");
    zassert_true(status, NULL);
    status = bitstring_same(&bit_string, &bit_string2);
    zassert_false(status, NULL);
    status = bitstring_copy(&bit_string2, &bit_string);
    zassert_true(status, NULL);
    status = bitstring_same(&bit_string, &bit_string2);
    zassert_true(status, NULL);
    zassert_equal(
        bitstring_bits_capacity(&bit_string), (MAX_BITSTRING_BYTES * 8), NULL);
    zassert_equal(bitstring_bits_capacity(NULL), 0, NULL);
}

/**
 * @brief Test encode/decode API for character strings
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacstr_tests, testCharacterString)
#else
static void testCharacterString(void)
#endif
{
    BACNET_CHARACTER_STRING bacnet_string = { 0 }, bacnet_string2 = { 0 };
    const char *value = "Joshua,Mary,Anna,Christopher";
    const char *utf8_value = "Joshua😍Mary😍Anna😍Christopher";
    const char *result = NULL;
    char test_value[MAX_APDU] = "Patricia";
    char test_append_value[MAX_APDU] = " and the Kids";
    char test_append_string[MAX_APDU] = "";
    char test_string[MAX_APDU] = { 0 };
    bool status = false;
    size_t length = 0;
    size_t test_length = 0;
    size_t i = 0;

    /* verify UTF8 initialization */
    status = characterstring_init(&bacnet_string, CHARACTER_UTF8, NULL, 0);
    zassert_true(status, NULL);
    zassert_equal(characterstring_length(&bacnet_string), 0, NULL);
    zassert_equal(
        characterstring_encoding(&bacnet_string), CHARACTER_UTF8, NULL);
    /* verify ANSI initialization */
    status = characterstring_init(&bacnet_string, CHARACTER_ANSI_X34, NULL, 0);
    zassert_true(status, NULL);
    zassert_equal(characterstring_length(&bacnet_string), 0, NULL);
    zassert_equal(
        characterstring_encoding(&bacnet_string), CHARACTER_ANSI_X34, NULL);

    /* empty string is the same as NULL */
    status = characterstring_same(&bacnet_string, NULL);
    zassert_true(status, NULL);
    status = characterstring_same(NULL, &bacnet_string);
    zassert_true(status, NULL);
    /* bounds check */
    status = characterstring_init(
        &bacnet_string, CHARACTER_ANSI_X34, NULL,
        characterstring_capacity(&bacnet_string) + 1);
    zassert_false(status, NULL);
    status = characterstring_truncate(
        &bacnet_string, characterstring_capacity(&bacnet_string) + 1);
    zassert_false(status, NULL);
    status = characterstring_truncate(
        &bacnet_string, characterstring_capacity(&bacnet_string));
    zassert_true(status, NULL);

    test_length = strlen(test_value);
    status = characterstring_init(
        &bacnet_string, CHARACTER_ANSI_X34, &test_value[0], test_length);
    zassert_true(status, NULL);
    result = characterstring_value(&bacnet_string);
    length = characterstring_length(&bacnet_string);
    zassert_equal(length, test_length, NULL);
    for (i = 0; i < test_length; i++) {
        zassert_equal(result[i], test_value[i], NULL);
    }
    test_length = characterstring_copy_value(
        test_string, sizeof(test_string), &bacnet_string);
    zassert_equal(length, test_length, NULL);

    test_length = strlen(test_append_value);
    status = characterstring_append(
        &bacnet_string, &test_append_value[0], test_length);
    strcat(test_append_string, test_value);
    strcat(test_append_string, test_append_value);
    test_length = strlen(test_append_string);
    zassert_true(status, NULL);
    length = characterstring_length(&bacnet_string);
    result = characterstring_value(&bacnet_string);
    zassert_equal(length, test_length, NULL);
    for (i = 0; i < test_length; i++) {
        zassert_equal(result[i], test_append_string[i], NULL);
    }
    /* init from valid ASCII string */
    status = characterstring_init_ansi(&bacnet_string, value);
    zassert_true(status, NULL);
    /* check for valid string */
    status = characterstring_valid(&bacnet_string);
    zassert_true(status, NULL);
    /* check for same string */
    status = characterstring_ansi_same(&bacnet_string, value);
    zassert_true(status, NULL);
    status = characterstring_copy(&bacnet_string, &bacnet_string2);
    zassert_true(status, NULL);
    status = characterstring_same(&bacnet_string, &bacnet_string2);
    zassert_true(status, NULL);

    /* set the encoding */
    status = characterstring_set_encoding(&bacnet_string, CHARACTER_ANSI_X34);
    zassert_true(status, NULL);
    /* validate that string is printable */
    status = characterstring_printable(&bacnet_string);
    zassert_true(status, NULL);
    /* pass NULL arguments */
    status = characterstring_init_ansi(NULL, value);
    zassert_false(status, NULL);
    status = characterstring_valid(NULL);
    zassert_false(status, NULL);
    status = characterstring_printable(NULL);
    zassert_false(status, NULL);
    status = characterstring_copy(&bacnet_string, NULL);
    zassert_false(status, NULL);
    status = characterstring_copy(NULL, &bacnet_string);
    zassert_false(status, NULL);
    /* null arguments that succeed */
    status = characterstring_init_ansi(&bacnet_string, NULL);
    zassert_true(status, NULL);
    status = characterstring_ansi_same(&bacnet_string, NULL);
    zassert_true(status, NULL);
    status = characterstring_ansi_same(NULL, "");
    zassert_true(status, NULL);
    /* alternate API for init and copy */
    status =
        characterstring_init_ansi_safe(&bacnet_string, value, strlen(value));
    status = characterstring_ansi_copy(
        test_append_string, sizeof(test_append_string), &bacnet_string);
    zassert_equal(strncmp(value, test_append_string, strlen(value)), 0, NULL);
    /* UTF-8 specific */
    status = characterstring_init_ansi(&bacnet_string, value);
    zassert_true(status, NULL);
    length = characterstring_length(&bacnet_string);
    status = characterstring_init_ansi(&bacnet_string, utf8_value);
    zassert_true(status, NULL);
    zassert_equal(
        characterstring_encoding(&bacnet_string), CHARACTER_UTF8, NULL);
    status = characterstring_valid(&bacnet_string);
    zassert_true(status, NULL);
    test_length = characterstring_utf8_length(&bacnet_string);
    zassert_equal(
        length, test_length, "value=\"%s\" length=%d, test_length=%d", value,
        length, test_length);
    status = characterstring_printable(&bacnet_string);
    zassert_false(status, NULL);
}

/**
 * @brief Test encode/decode API for octet strings
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacstr_tests, testOctetString)
#else
static void testOctetString(void)
#endif
{
    BACNET_OCTET_STRING bacnet_string;
    BACNET_OCTET_STRING bacnet_string_twin;
    uint8_t *value = NULL;
    uint8_t test_value[MAX_APDU] = "Patricia";
    uint8_t test_value_twin[MAX_APDU] = "PATRICIA";
    uint8_t test_append_value[MAX_APDU] = " and the Kids";
    uint8_t test_append_string[MAX_APDU] = "";
    const char *hex_value_valid = "1234567890ABCDEF";
    const char *hex_value_skips = "12:34:56:78:90:AB:CD:EF";
    const char *hex_value_odd = "1234567890ABCDE";
    char hex_value_long[MAX_APDU + MAX_APDU] = "";
    uint8_t apdu[MAX_APDU] = { 0 };
    bool status = false;
    size_t length = 0;
    size_t test_length = 0;
    size_t i = 0;

    /* verify initialization */
    status = octetstring_init(&bacnet_string, NULL, 0);
    zassert_true(status, NULL);
    zassert_equal(octetstring_length(&bacnet_string), 0, NULL);
    value = octetstring_value(&bacnet_string);
    for (i = 0; i < octetstring_capacity(&bacnet_string); i++) {
        zassert_equal(value[i], 0, NULL);
    }
    /* bounds check */
    status = octetstring_init(
        &bacnet_string, NULL, octetstring_capacity(&bacnet_string) + 1);
    zassert_false(status, NULL);
    status = octetstring_init(
        &bacnet_string, NULL, octetstring_capacity(&bacnet_string));
    zassert_true(status, NULL);
    status = octetstring_truncate(
        &bacnet_string, octetstring_capacity(&bacnet_string) + 1);
    zassert_false(status, NULL);
    status = octetstring_truncate(
        &bacnet_string, octetstring_capacity(&bacnet_string));
    zassert_true(status, NULL);

    test_length = strlen((char *)test_value);
    status = octetstring_init(&bacnet_string, &test_value[0], test_length);
    zassert_true(status, NULL);
    length = octetstring_length(&bacnet_string);
    value = octetstring_value(&bacnet_string);
    zassert_equal(length, test_length, NULL);
    for (i = 0; i < test_length; i++) {
        zassert_equal(value[i], test_value[i], NULL);
    }

    test_length = strlen((char *)test_append_value);
    status =
        octetstring_append(&bacnet_string, &test_append_value[0], test_length);
    strcat((char *)test_append_string, (char *)test_value);
    strcat((char *)test_append_string, (char *)test_append_value);
    test_length = strlen((char *)test_append_string);
    zassert_true(status, NULL);
    length = octetstring_length(&bacnet_string);
    value = octetstring_value(&bacnet_string);
    zassert_equal(length, test_length, NULL);
    for (i = 0; i < test_length; i++) {
        zassert_equal(value[i], test_append_string[i], NULL);
    }
    /* twins, almost */
    test_length = strlen((char *)test_value);
    status = octetstring_init(&bacnet_string, &test_value[0], test_length);
    zassert_true(status, NULL);
    test_length = strlen((char *)test_value_twin);
    status =
        octetstring_init(&bacnet_string_twin, &test_value_twin[0], test_length);
    zassert_true(status, NULL);
    status = octetstring_value_same(&bacnet_string, &bacnet_string_twin);
    zassert_false(status, NULL);
    /* null argument */
    status = octetstring_value_same(NULL, &bacnet_string_twin);
    zassert_false(status, NULL);
    status = octetstring_value_same(&bacnet_string, NULL);
    zassert_false(status, NULL);
    status = octetstring_value_same(NULL, NULL);
    zassert_false(status, NULL);
    /* self-healing length too long */
    bacnet_string.length = MAX_OCTET_STRING_BYTES + 1;
    length = octetstring_length(&bacnet_string);
    zassert_equal(length, MAX_OCTET_STRING_BYTES, NULL);
    /* valid case - empty string */
    status = octetstring_init_ascii_hex(&bacnet_string, "");
    zassert_true(status, NULL);
    /* valid case - valid hex string */
    status = octetstring_init_ascii_hex(&bacnet_string, hex_value_valid);
    zassert_true(status, NULL);
    test_length = strlen(hex_value_valid) / 2;
    length = octetstring_length(&bacnet_string);
    zassert_equal(length, test_length, NULL);
    /* valid case - with non-hex characters interspersed */
    status = octetstring_init_ascii_hex(&bacnet_string, hex_value_skips);
    zassert_true(status, NULL);
    length = octetstring_length(&bacnet_string);
    zassert_equal(length, test_length, NULL);
    /* invalid case - not enough pairs */
    status = octetstring_init_ascii_hex(&bacnet_string, hex_value_odd);
    zassert_false(status, NULL);
    /* invalid case - too long */
    memset(hex_value_long, 'F', sizeof(hex_value_long));
    hex_value_long[sizeof(hex_value_long) - 1] = 0;
    status = octetstring_init_ascii_hex(&bacnet_string, hex_value_long);
    zassert_false(status, NULL);
    /* invalid case - null arguments */
    status = octetstring_init_ascii_hex(&bacnet_string, NULL);
    zassert_false(status, NULL);
    status = octetstring_init_ascii_hex(NULL, hex_value_long);
    zassert_false(status, NULL);
    status = octetstring_init_ascii_hex(NULL, NULL);
    zassert_false(status, NULL);
    /* copy value */
    test_length = strlen((char *)test_value);
    status = octetstring_init(&bacnet_string, &test_value[0], test_length);
    zassert_true(status, NULL);
    length = octetstring_copy_value(apdu, sizeof(apdu), &bacnet_string);
    zassert_equal(length, test_length, NULL);
    /* test the buffer is too small */
    while (test_length) {
        test_length--;
        length = octetstring_copy_value(apdu, test_length, &bacnet_string);
        zassert_equal(
            length, 0, "test_length=%u length=%u", test_length, length);
    }
    /* copy */
    test_length = strlen((char *)test_value);
    status = octetstring_init(&bacnet_string, &test_value[0], test_length);
    zassert_true(status, NULL);
    status = octetstring_copy(&bacnet_string_twin, &bacnet_string);
    zassert_true(status, NULL);
    status = octetstring_value_same(&bacnet_string_twin, &bacnet_string);
    zassert_true(status, NULL);
}

/**
 * @brief Test encode/decode API for bacnet_stricmp
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacstr_tests, test_bacnet_stricmp)
#else
static void test_bacnet_stricmp(void)
#endif
{
    int rv;
    const char *name_a = "Patricia", *test_name_a = "patricia";
    const char *name_b = "CamelCase", *test_name_b = "CAMELCASE";

    rv = bacnet_stricmp(name_a, test_name_a);
    zassert_equal(rv, 0, NULL);
    rv = bacnet_stricmp(name_b, test_name_b);
    zassert_equal(rv, 0, NULL);
    rv = bacnet_stricmp(name_a, name_b);
    zassert_not_equal(rv, 0, NULL);
    rv = bacnet_stricmp(test_name_a, test_name_b);
    zassert_not_equal(rv, 0, NULL);
    rv = bacnet_stricmp(NULL, test_name_b);
    zassert_not_equal(rv, 0, NULL);
    rv = bacnet_stricmp(test_name_a, NULL);
    zassert_not_equal(rv, 0, NULL);
    /* case sensitive */
    rv = bacnet_strcmp(name_a, name_a);
    zassert_equal(rv, 0, NULL);
    rv = bacnet_strcmp(name_a, test_name_a);
    zassert_not_equal(rv, 0, NULL);
    rv = bacnet_strcmp(test_name_a, test_name_b);
    zassert_not_equal(rv, 0, NULL);
    rv = bacnet_strcmp(NULL, test_name_b);
    zassert_not_equal(rv, 0, NULL);
    rv = bacnet_strcmp(test_name_a, NULL);
    zassert_not_equal(rv, 0, NULL);
}

/**
 * @brief Test encode/decode API for bacnet_strnicmp
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacstr_tests, test_bacnet_strnicmp)
#else
static void test_bacnet_strnicmp(void)
#endif
{
    int rv, len;
    const char *name_a = "Patricia", *test_name_a = "patricia";
    const char *name_b = "CamelCase", *test_name_b = "CAMELCASE";

    /* case sensitive */
    rv = bacnet_strncmp(name_a, name_a, strlen(name_a));
    zassert_equal(rv, 0, NULL);
    rv = bacnet_strncmp(name_a, test_name_a, strlen(name_a));
    zassert_not_equal(rv, 0, NULL);
    rv = bacnet_strncmp(test_name_a, test_name_b, strlen(test_name_a));
    zassert_not_equal(rv, 0, NULL);
    rv = bacnet_strncmp(NULL, test_name_b, strlen(test_name_b));
    zassert_not_equal(rv, 0, NULL);
    rv = bacnet_strncmp(test_name_a, NULL, strlen(test_name_a));
    zassert_not_equal(rv, 0, NULL);
    /* case insensitive */
    rv = bacnet_strnicmp(name_a, test_name_a, strlen(name_a));
    zassert_equal(rv, 0, NULL);
    rv = bacnet_strnicmp(name_b, test_name_b, strlen(name_b));
    zassert_equal(rv, 0, NULL);
    rv = bacnet_strnicmp(name_a, name_b, strlen(name_a));
    zassert_not_equal(rv, 0, NULL);
    rv = bacnet_strnicmp(test_name_a, test_name_b, strlen(test_name_a));
    zassert_not_equal(rv, 0, NULL);
    rv = bacnet_strnicmp(NULL, test_name_b, strlen(test_name_b));
    zassert_not_equal(rv, 0, NULL);
    rv = bacnet_strnicmp(test_name_a, NULL, strlen(test_name_a));
    zassert_not_equal(rv, 0, NULL);
    /* shrink the test space */
    len = strlen(name_a);
    while (len >= 0) {
        len--;
        rv = bacnet_strnicmp(name_a, test_name_a, len);
        zassert_equal(rv, 0, NULL);
    }
}
/**
 * @brief Test encode/decode API for bacnet_stricmp
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacstr_tests, test_bacnet_strnlen)
#else
static void test_bacnet_strnlen(void)
#endif
{
    size_t len, test_len;
    const char *test_name = "Patricia";

    len = strlen(test_name);
    while (len) {
        test_len = bacnet_strnlen(test_name, len);
        zassert_equal(len, test_len, NULL);
        len--;
    }
    len = strlen(test_name);
    test_len = bacnet_strnlen(test_name, 512);
    zassert_equal(len, test_len, "len=%u test_len=%d", len, test_len);
}

/**
 * @brief Test encode/decode API for bacnet_strto. functions
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacstr_tests, test_bacnet_strto)
#else
static void test_bacnet_strto(void)
#endif
{
    bool status;
    const char *empty_string = "";
    const char *extra_text_string = "123yyx";
    const char *test_unsigned_long_string = "1234567890";
    unsigned long unsigned_long_value, test_unsigned_long_value = 1234567890;
    const char *test_long_string = "-1234567890";
    long long_value, test_long_value = -1234567890;
    const char *test_float_positive_string = "1.23";
    float float_value, test_float_value = 1.23f;
    double double_value, test_double_value = 1.23;
    long double long_double_value, test_long_double_value = 1.23L;
    const char *test_float_negative_string = "-1.23";
    float float_negative_value, test_float_negative_value = -1.23f;
    double double_negative_value, test_double_negative_value = -1.23;
    long double long_double_negative_value,
        test_long_double_negative_value = -1.23L;
    char buffer[80] = "", *ascii_result = NULL;

    /* unsigned long */
    status = bacnet_strtoul(test_unsigned_long_string, &unsigned_long_value);
    zassert_true(status, NULL);
    zassert_equal(unsigned_long_value, test_unsigned_long_value, NULL);
    status = bacnet_strtoul(empty_string, &unsigned_long_value);
    zassert_false(status, NULL);
    status = bacnet_strtoul(extra_text_string, &unsigned_long_value);
    zassert_false(status, NULL);
    ascii_result = bacnet_ultoa(unsigned_long_value, buffer, sizeof(buffer));
    zassert_equal(bacnet_strcmp(buffer, test_unsigned_long_string), 0, NULL);
    zassert_equal(ascii_result, buffer, NULL);
    ascii_result = bacnet_utoa(unsigned_long_value, buffer, sizeof(buffer));
    zassert_equal(bacnet_strcmp(buffer, test_unsigned_long_string), 0, NULL);
    zassert_equal(ascii_result, buffer, NULL);
    /* long */
    status = bacnet_strtol(test_long_string, &long_value);
    zassert_true(status, NULL);
    zassert_equal(long_value, test_long_value, NULL);
    status = bacnet_strtol(empty_string, &long_value);
    zassert_false(status, NULL);
    status = bacnet_strtol(extra_text_string, &long_value);
    zassert_false(status, NULL);
    ascii_result = bacnet_ltoa(long_value, buffer, sizeof(buffer));
    zassert_equal(bacnet_strcmp(buffer, test_long_string), 0, NULL);
    zassert_equal(ascii_result, buffer, NULL);
    ascii_result = bacnet_itoa(long_value, buffer, sizeof(buffer));
    zassert_equal(bacnet_strcmp(buffer, test_long_string), 0, NULL);
    zassert_equal(ascii_result, buffer, NULL);
    /* single precision */
    status = bacnet_strtof(test_float_positive_string, &float_value);
    zassert_true(status, NULL);
    zassert_true(is_float_equal(float_value, test_float_value), NULL);
    status = bacnet_strtof(test_float_negative_string, &float_negative_value);
    zassert_true(status, NULL);
    zassert_true(
        is_float_equal(float_negative_value, test_float_negative_value), NULL);
    status = bacnet_strtof(empty_string, &float_value);
    zassert_false(status, NULL);
    status = bacnet_strtof(extra_text_string, &float_value);
    zassert_false(status, NULL);
    /* double precision */
    status = bacnet_strtod(test_float_positive_string, &double_value);
    zassert_true(status, NULL);
    zassert_true(is_float_equal(double_value, test_double_value), NULL);
    status = bacnet_strtod(test_float_negative_string, &double_negative_value);
    zassert_true(status, NULL);
    zassert_true(
        is_float_equal(double_negative_value, test_double_negative_value),
        NULL);
    status = bacnet_strtod(empty_string, &double_value);
    zassert_false(status, NULL);
    status = bacnet_strtod(extra_text_string, &double_value);
    zassert_false(status, NULL);
    ascii_result =
        bacnet_dtoa(double_negative_value, buffer, sizeof(buffer), 2);
    zassert_equal(bacnet_strcmp(buffer, test_float_negative_string), 0, NULL);
    zassert_equal(ascii_result, buffer, NULL);
    /* long double precision */
    status = bacnet_strtold(test_float_positive_string, &long_double_value);
    zassert_true(status, NULL);
    zassert_true(
        is_float_equal(long_double_value, test_long_double_value), NULL);
    status =
        bacnet_strtold(test_float_negative_string, &long_double_negative_value);
    zassert_true(status, NULL);
    zassert_true(
        is_float_equal(
            long_double_negative_value, test_long_double_negative_value),
        NULL);
    status = bacnet_strtold(empty_string, &long_double_value);
    zassert_false(status, NULL);
    status = bacnet_strtold(extra_text_string, &long_double_value);
    zassert_false(status, NULL);
}

/**
 * @brief Test encode/decode API for bacnet_string_to_x functions
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacstr_tests, test_bacnet_string_to_x)
#else
static void test_bacnet_string_to_x(void)
#endif
{
    bool status;
    const char *empty_string = "";
    const char *extra_text_string = "123yyx";
    const char *test_uint8_t_string = "123";
    const char *test_uint16_t_string = "12345";
    const char *test_uint32_t_string = "1234567890";
    const char *test_int32_t_string = "-1234567890";
    const char *test_true_string = "true";
    const char *test_false_string = "false";
    const char *test_active_string = "active";
    const char *test_inactive_string = "inactive";
    const char *test_true_numeric_string = "1";
    const char *test_false_numeric_string = "0";
    const char *test_unsigned_string = "1234567890";
    const char *test_ascii_string = "abcdefghijklmnopqrstuvwxyz";
    uint8_t uint8_t_value, test_uint8_t_value = 123;
    uint16_t uint16_t_value, test_uint16_t_value = 12345;
    uint32_t uint32_t_value, test_uint32_t_value = 1234567890;
    int32_t int32_t_value, test_int32_t_value = -1234567890;
    BACNET_UNSIGNED_INTEGER bacnet_unsigned_integer,
        test_bacnet_unsigned_integer = 1234567890;
    bool bool_value, test_true_value = true, test_false_value = false;
    char ascii_string[80] = "", *ascii_string_result = NULL;

    /* uint8_t */
    status = bacnet_string_to_uint8(test_uint8_t_string, &uint8_t_value);
    zassert_true(status, NULL);
    zassert_equal(uint8_t_value, test_uint8_t_value, NULL);
    status = bacnet_string_to_uint8(empty_string, &uint8_t_value);
    zassert_false(status, NULL);
    status = bacnet_string_to_uint8(extra_text_string, &uint8_t_value);
    zassert_false(status, NULL);
    /* uint16_t */
    status = bacnet_string_to_uint16(test_uint16_t_string, &uint16_t_value);
    zassert_true(status, NULL);
    zassert_equal(uint16_t_value, test_uint16_t_value, NULL);
    status = bacnet_string_to_uint16(empty_string, &uint16_t_value);
    zassert_false(status, NULL);
    status = bacnet_string_to_uint16(extra_text_string, &uint16_t_value);
    zassert_false(status, NULL);
    /* uint32_t */
    status = bacnet_string_to_uint32(test_uint32_t_string, &uint32_t_value);
    zassert_true(status, NULL);
    zassert_equal(uint32_t_value, test_uint32_t_value, NULL);
    status = bacnet_string_to_uint32(empty_string, &uint32_t_value);
    zassert_false(status, NULL);
    status = bacnet_string_to_uint32(extra_text_string, &uint32_t_value);
    zassert_false(status, NULL);
    /* int32_t */
    status = bacnet_string_to_int32(test_int32_t_string, &int32_t_value);
    zassert_true(status, NULL);
    zassert_equal(int32_t_value, test_int32_t_value, NULL);
    status = bacnet_string_to_int32(empty_string, &int32_t_value);
    zassert_false(status, NULL);
    /* bool */
    status = bacnet_string_to_bool(test_true_string, &bool_value);
    zassert_true(status, NULL);
    zassert_equal(bool_value, test_true_value, NULL);
    status = bacnet_string_to_bool(test_false_string, &bool_value);
    zassert_true(status, NULL);
    zassert_equal(bool_value, test_false_value, NULL);
    status = bacnet_string_to_bool(empty_string, &bool_value);
    zassert_false(status, NULL);
    status = bacnet_string_to_bool(extra_text_string, &bool_value);
    zassert_false(status, NULL);
    /* active/inactive */
    status = bacnet_string_to_bool(test_active_string, &bool_value);
    zassert_true(status, NULL);
    zassert_equal(bool_value, test_true_value, NULL);
    status = bacnet_string_to_bool(test_inactive_string, &bool_value);
    zassert_true(status, NULL);
    zassert_equal(bool_value, test_false_value, NULL);
    status = bacnet_string_to_bool(empty_string, &bool_value);
    zassert_false(status, NULL);
    /* 0/1 */
    status = bacnet_string_to_bool(test_true_numeric_string, &bool_value);
    zassert_true(status, NULL);
    zassert_equal(bool_value, test_true_value, NULL);
    status = bacnet_string_to_bool(test_false_numeric_string, &bool_value);
    zassert_true(status, NULL);
    zassert_equal(bool_value, test_false_value, NULL);
    /* bacnet_unsigned_integer */
    status = bacnet_string_to_unsigned(
        test_unsigned_string, &bacnet_unsigned_integer);
    zassert_true(status, NULL);
    zassert_equal(bacnet_unsigned_integer, test_bacnet_unsigned_integer, NULL);
    status = bacnet_string_to_unsigned(empty_string, &bacnet_unsigned_integer);
    zassert_false(status, NULL);
    status =
        bacnet_string_to_unsigned(extra_text_string, &bacnet_unsigned_integer);
    zassert_false(status, NULL);
    /* ascii string */
    ascii_string_result = bacnet_snprintf_to_ascii(
        ascii_string, sizeof(ascii_string), "%s", test_ascii_string);
    zassert_equal(
        bacnet_strcmp(ascii_string_result, test_ascii_string), 0, NULL);
}

/**
 * @brief Test encode/decode API for bacnet trim functions
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacstr_tests, test_bacnet_string_trim)
#else
static void test_bacnet_string_trim(void)
#endif
{
    char trim_left[80] = "    abcdefg", *trim_left_result = NULL;
    char trim_right[80] = "abcdefg    ", *trim_right_result = NULL;
    char trim_both[80] = "   abcdefg   ", *trim_both_result = NULL;
    char *trim_test_value = "abcdefg";
    char *empty_string = "";

    trim_left_result = bacnet_ltrim(trim_left, " ");
    trim_right_result = bacnet_rtrim(trim_right, " ");
    trim_both_result = bacnet_trim(trim_both, " ");
    zassert_equal(bacnet_strcmp(trim_left_result, trim_test_value), 0, NULL);
    zassert_equal(bacnet_strcmp(trim_right_result, trim_test_value), 0, NULL);
    zassert_equal(bacnet_strcmp(trim_both_result, trim_test_value), 0, NULL);
    trim_left_result = bacnet_ltrim(empty_string, " ");
    trim_right_result = bacnet_rtrim(empty_string, " ");
    trim_both_result = bacnet_trim(empty_string, " ");
    zassert_equal(bacnet_strcmp(trim_left_result, empty_string), 0, NULL);
    zassert_equal(bacnet_strcmp(trim_right_result, empty_string), 0, NULL);
    zassert_equal(bacnet_strcmp(trim_both_result, empty_string), 0, NULL);
}

/**
 * @brief Test bacnet_stptok string tokenizer
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacstr_tests, test_bacnet_stptok)
#else
static void test_bacnet_stptok(void)
#endif
{
    char *pCmd = "I Love You\r\n";
    char token[80] = "";

    pCmd = bacnet_stptok(pCmd, token, sizeof(token), " \r\n");

    zassert_true(bacnet_strcmp(token, "I") == 0, NULL);
    zassert_true(bacnet_strcmp(pCmd, "Love You\r\n") == 0, NULL);

    pCmd = bacnet_stptok(pCmd, token, sizeof(token), " \r\n");

    zassert_true(bacnet_strcmp(token, "Love") == 0, NULL);
    zassert_true(bacnet_strcmp(pCmd, "You\r\n") == 0, NULL);

    pCmd = bacnet_stptok(pCmd, token, sizeof(token), " \r\n");

    zassert_true(bacnet_strcmp(token, "You") == 0, NULL);
    zassert_true(pCmd == NULL, NULL);
}

/**
 * @brief Test encode/decode API for bacnet snprintf and shift functions
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacstr_tests, test_bacnet_snprintf)
#else
static void test_bacnet_snprintf(void)
#endif
{
    int buf_len = 0, str_len, null_len = 0, test_null_len = 0;
    int i;
    char str[30] = "";
    const char *null_string = "REALLY BIG NULL STRING";
    const char *one_char_string = "1";
    const char *two_char_string = "12";

    /* one char */
    buf_len = 0;
    str_len = 1;
    buf_len = bacnet_snprintf(str, str_len, buf_len, "%s", one_char_string);
    zassert_equal(buf_len, str_len, "buf_len=%d str_len=%d", buf_len, str_len);
    /* two char */
    buf_len = 0;
    str_len = 2;
    buf_len = bacnet_snprintf(str, str_len, buf_len, "%s", two_char_string);
    zassert_equal(buf_len, str_len, "buf_len=%d str_len=%d", buf_len, str_len);
    buf_len = bacnet_snprintf(str, str_len, buf_len, "%s", two_char_string);
    zassert_equal(buf_len, str_len, "buf_len=%d str_len=%d", buf_len, str_len);
    /* large chars */
    buf_len = 0;
    str_len = sizeof(str);
    for (i = 0; i < 5; i++) {
        /* appending formatted strings */
        buf_len = bacnet_snprintf(str, str_len, buf_len, "{");
        buf_len =
            bacnet_snprintf(str, str_len, buf_len, "REALLY BIG STRING BASS");
        buf_len = bacnet_snprintf(str, str_len, buf_len, "}");
        /* appending to a NULL string for length */
        null_len = bacnet_snprintf(NULL, 0, null_len, null_string);
        test_null_len += strlen(null_string);
    }
    zassert_equal(buf_len, str_len, "buf_len=%d str_len=%d", buf_len, str_len);
    zassert_equal(
        str[buf_len - 1], 0, "str[%d]=%c", buf_len - 1, str[buf_len - 1]);
    zassert_equal(null_len, test_null_len, "null_len=%d", null_len);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacstr_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bacstr_tests, ztest_unit_test(testBitString),
        ztest_unit_test(testCharacterString), ztest_unit_test(testOctetString),
        ztest_unit_test(test_bacnet_stricmp),
        ztest_unit_test(test_bacnet_strnicmp),
        ztest_unit_test(test_bacnet_strnlen),
        ztest_unit_test(test_bacnet_strto),
        ztest_unit_test(test_bacnet_string_to_x),
        ztest_unit_test(test_bacnet_string_trim),
        ztest_unit_test(test_bacnet_stptok),
        ztest_unit_test(test_bacnet_snprintf));
    ztest_run_test_suite(bacstr_tests);
}
#endif
