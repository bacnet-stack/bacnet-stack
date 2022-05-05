/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/bacstr.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test encode/decode API for strings
 */
static void testBitString(void)
{
    uint8_t bit = 0;
    int max_bit;
    BACNET_BIT_STRING bit_string;
    BACNET_BIT_STRING bit_string2;
    BACNET_BIT_STRING bit_string3;

    bitstring_init(&bit_string);
    /* verify initialization */
    zassert_equal(bitstring_bits_used(&bit_string), 0, NULL);
    for (bit = 0; bit < (MAX_BITSTRING_BYTES * 8); bit++) {
        zassert_false(bitstring_bit(&bit_string, bit), NULL);
    }

    /* test for true */
    for (bit = 0; bit < (MAX_BITSTRING_BYTES * 8); bit++) {
        bitstring_set_bit(&bit_string, bit, true);
        zassert_equal(bitstring_bits_used(&bit_string), (bit + 1), NULL);
        zassert_true(bitstring_bit(&bit_string, bit), NULL);
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
        bitstring_set_bit(&bit_string3, max_bit - 1,
            !bitstring_bit(&bit_string, max_bit - 1));
        zassert_false(bitstring_same(&bit_string, &bit_string2), NULL);
        zassert_false(bitstring_same(&bit_string, &bit_string3), NULL);
    }
}

/**
 * @brief Test encode/decode API for character strings
 */
static void testCharacterString(void)
{
    BACNET_CHARACTER_STRING bacnet_string;
    char *value = "Joshua,Mary,Anna,Christopher";
    char test_value[MAX_APDU] = "Patricia";
    char test_append_value[MAX_APDU] = " and the Kids";
    char test_append_string[MAX_APDU] = "";
    bool status = false;
    size_t length = 0;
    size_t test_length = 0;
    size_t i = 0;

    /* verify initialization */
    status = characterstring_init(&bacnet_string, CHARACTER_ANSI_X34, NULL, 0);
    zassert_true(status, NULL);
    zassert_equal(characterstring_length(&bacnet_string), 0, NULL);
    zassert_equal(characterstring_encoding(&bacnet_string), CHARACTER_ANSI_X34,
		  NULL);
    /* bounds check */
    status = characterstring_init(&bacnet_string, CHARACTER_ANSI_X34, NULL,
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
    value = characterstring_value(&bacnet_string);
    length = characterstring_length(&bacnet_string);
    zassert_equal(length, test_length, NULL);
    for (i = 0; i < test_length; i++) {
        zassert_equal(value[i], test_value[i], NULL);
    }
    test_length = strlen(test_append_value);
    status = characterstring_append(
        &bacnet_string, &test_append_value[0], test_length);
    strcat(test_append_string, test_value);
    strcat(test_append_string, test_append_value);
    test_length = strlen(test_append_string);
    zassert_true(status, NULL);
    length = characterstring_length(&bacnet_string);
    value = characterstring_value(&bacnet_string);
    zassert_equal(length, test_length, NULL);
    for (i = 0; i < test_length; i++) {
        zassert_equal(value[i], test_append_string[i], NULL);
    }
}

/**
 * @brief Test encode/decode API for octet strings
 */
static void testOctetString(void)
{
    BACNET_OCTET_STRING bacnet_string;
    uint8_t *value = NULL;
    uint8_t test_value[MAX_APDU] = "Patricia";
    uint8_t test_append_value[MAX_APDU] = " and the Kids";
    uint8_t test_append_string[MAX_APDU] = "";
    const char * hex_value_valid = "1234567890ABCDEF";
    const char * hex_value_skips = "12:34:56:78:90:AB:CD:EF";
    const char * hex_value_odd = "1234567890ABCDE";
    const char * hex_value_long = NULL;
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
    /* valid case - empty string */
    status = octetstring_init_ascii_hex(&bacnet_string, "");
    zassert_true(status, NULL);
    /* valid case - valid hex string */
    status = octetstring_init_ascii_hex(&bacnet_string, hex_value_valid);
    zassert_true(status, NULL);
    test_length = strlen(hex_value_valid)/2;
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
    memset(test_value, 'F', sizeof(test_value));
    hex_value_long = test_value;
    status = octetstring_init_ascii_hex(&bacnet_string, hex_value_long);
    zassert_false(status, NULL);
    /* invalid case - null arguments */
    status = octetstring_init_ascii_hex(&bacnet_string, NULL);
    zassert_false(status, NULL);
    status = octetstring_init_ascii_hex(NULL, hex_value_long);
    zassert_false(status, NULL);
    status = octetstring_init_ascii_hex(NULL, NULL);
    zassert_false(status, NULL);
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(bacstr_tests,
     ztest_unit_test(testBitString),
     ztest_unit_test(testCharacterString),
     ztest_unit_test(testOctetString)
     );

    ztest_run_test_suite(bacstr_tests);
}
