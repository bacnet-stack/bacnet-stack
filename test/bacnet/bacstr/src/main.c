/**
 * @file
 * @brief test BACnet CharacterString, BitString, and OctetString APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacstr.h>

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
    uint8_t bit = 0;
    int max_bit;
    BACNET_BIT_STRING bit_string;
    BACNET_BIT_STRING bit_string2;
    BACNET_BIT_STRING bit_string3;
    bool status = false;

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
        bitstring_set_bit(
            &bit_string3, max_bit - 1,
            !bitstring_bit(&bit_string, max_bit - 1));
        zassert_false(bitstring_same(&bit_string, &bit_string2), NULL);
        zassert_false(bitstring_same(&bit_string, &bit_string3), NULL);
    }
    status = bitstring_init_ascii(&bit_string, "1111000010100101");
    zassert_true(status, NULL);
    status = bitstring_init_ascii(&bit_string2, "1111000010100111");
    zassert_true(status, NULL);
    status = bitstring_same(&bit_string, &bit_string2);
    zassert_false(status, NULL);
    zassert_equal(
        bitstring_bits_capacity(&bit_string), (MAX_BITSTRING_BYTES * 8), NULL);
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
    BACNET_CHARACTER_STRING bacnet_string;
    const char *value = "Joshua,Mary,Anna,Christopher";
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
    zassert_equal(
        characterstring_encoding(&bacnet_string), CHARACTER_ANSI_X34, NULL);
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
    /* init from valid ASCII string */
    status = characterstring_init_ansi(&bacnet_string, value);
    zassert_true(status, NULL);
    /* check for valid string */
    status = characterstring_valid(&bacnet_string);
    zassert_true(status, NULL);
    /* check for same string */
    status = characterstring_ansi_same(&bacnet_string, value);
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
    status = characterstring_ansi_same(NULL, value);
    zassert_false(status, NULL);
    status = characterstring_printable(NULL);
    zassert_false(status, NULL);
    /* null arguments that succeed */
    status = characterstring_init_ansi(&bacnet_string, NULL);
    zassert_true(status, NULL);
    status = characterstring_ansi_same(&bacnet_string, NULL);
    zassert_true(status, NULL);
    /* alternate API for init and copy */
    status =
        characterstring_init_ansi_safe(&bacnet_string, value, strlen(value));
    status = characterstring_ansi_copy(
        test_append_string, sizeof(test_append_string), &bacnet_string);
    zassert_equal(strncmp(value, test_append_string, strlen(value)), 0, NULL);
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
        ztest_unit_test(testCharacterString), ztest_unit_test(testOctetString));

    ztest_run_test_suite(bacstr_tests);
}
#endif
