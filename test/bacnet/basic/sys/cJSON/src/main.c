/**
 * @file
 * @brief test the trimmed cJSON parser
 * @details A focused subset of upstream DaveGamble/cJSON's tests
 * (tests/parse_number.c, tests/parse_string.c, tests/parse_array.c,
 * tests/parse_object.c, tests/misc_tests.c and tests/parse_with_opts.c),
 * adapted to the API this module actually exposes (parse + read-only
 * accessors; no printing, no cJSON_Utils patch/merge support), plus
 * regression tests for the parser hardening applied to this file
 * (nesting-depth limit, invalid/truncated \\u escapes, non-finite
 * numbers).
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <bacnet/basic/sys/cJSON.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/* build a string of `depth` '[' followed by `depth` ']', e.g. depth=3
 * gives "[[[]]]" - used to probe the parser's nesting-depth limit. */
static char *build_nested_array(int depth)
{
    char *buf = (char *)malloc((size_t)(2 * depth) + 1);
    int i;

    zassert_not_null(buf, NULL);
    for (i = 0; i < depth; i++) {
        buf[i] = '[';
        buf[depth + i] = ']';
    }
    buf[2 * depth] = '\0';

    return buf;
}

/**
 * @brief literals: null, true, false
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cJSON_tests, testCJSON_ParseLiterals)
#else
static void testCJSON_ParseLiterals(void)
#endif
{
    cJSON *item;

    item = cJSON_Parse("null");
    zassert_not_null(item, NULL);
    zassert_true(cJSON_IsNull(item), NULL);
    cJSON_Delete(item);

    item = cJSON_Parse("true");
    zassert_not_null(item, NULL);
    zassert_true(cJSON_IsTrue(item), NULL);
    zassert_true(cJSON_IsBool(item), NULL);
    cJSON_Delete(item);

    item = cJSON_Parse("false");
    zassert_not_null(item, NULL);
    zassert_true(cJSON_IsFalse(item), NULL);
    zassert_true(cJSON_IsBool(item), NULL);
    cJSON_Delete(item);
}

/**
 * @brief numbers: integers, negatives, fractions, exponents
 * (subset of upstream tests/parse_number.c)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cJSON_tests, testCJSON_ParseNumbers)
#else
static void testCJSON_ParseNumbers(void)
#endif
{
    cJSON *item;

    item = cJSON_Parse("0");
    zassert_not_null(item, NULL);
    zassert_true(cJSON_IsNumber(item), NULL);
    zassert_within(item->valuedouble, 0.0, 0.0001, NULL);
    cJSON_Delete(item);

    item = cJSON_Parse("1");
    zassert_not_null(item, NULL);
    zassert_within(item->valuedouble, 1.0, 0.0001, NULL);
    cJSON_Delete(item);

    item = cJSON_Parse("-128");
    zassert_not_null(item, NULL);
    zassert_within(item->valuedouble, -128.0, 0.0001, NULL);
    zassert_equal(item->valueint, -128, NULL);
    cJSON_Delete(item);

    item = cJSON_Parse("3.1415");
    zassert_not_null(item, NULL);
    zassert_within(item->valuedouble, 3.1415, 0.0001, NULL);
    cJSON_Delete(item);

    item = cJSON_Parse("1e2");
    zassert_not_null(item, NULL);
    zassert_within(item->valuedouble, 100.0, 0.0001, NULL);
    cJSON_Delete(item);

    item = cJSON_Parse("-1.5e-2");
    zassert_not_null(item, NULL);
    zassert_within(item->valuedouble, -0.015, 0.00001, NULL);
    cJSON_Delete(item);
}

/**
 * @brief valueint saturates instead of wrapping for out-of-int-range
 * finite numbers, rather than invoking the undefined behaviour of
 * casting an out-of-range double straight to int.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cJSON_tests, testCJSON_NumberValueintSaturates)
#else
static void testCJSON_NumberValueintSaturates(void)
#endif
{
    cJSON *item;

    item = cJSON_Parse("1e15");
    zassert_not_null(item, NULL);
    zassert_true(cJSON_IsNumber(item), NULL);
    zassert_equal(item->valueint, INT_MAX, NULL);
    cJSON_Delete(item);

    item = cJSON_Parse("-1e15");
    zassert_not_null(item, NULL);
    zassert_equal(item->valueint, INT_MIN, NULL);
    cJSON_Delete(item);
}

/**
 * @brief a number literal that overflows to +/-Infinity is rejected
 * by the parser instead of being silently stored as a non-finite
 * cJSON_Number (regression test for the parser hardening).
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cJSON_tests, testCJSON_NumberOverflowRejected)
#else
static void testCJSON_NumberOverflowRejected(void)
#endif
{
    cJSON *item;

    item = cJSON_Parse("1e400");
    zassert_is_null(item, NULL);

    item = cJSON_Parse("-1e400");
    zassert_is_null(item, NULL);
}

/**
 * @brief strings: escapes and \\u unicode escapes, including a
 * surrogate pair (subset of upstream tests/parse_string.c)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cJSON_tests, testCJSON_ParseStrings)
#else
static void testCJSON_ParseStrings(void)
#endif
{
    cJSON *item;

    item = cJSON_Parse("\"Hello, World!\"");
    zassert_not_null(item, NULL);
    zassert_true(cJSON_IsString(item), NULL);
    zassert_equal(strcmp(item->valuestring, "Hello, World!"), 0, NULL);
    cJSON_Delete(item);

    item = cJSON_Parse("\"Hello\\nWorld\\t!\"");
    zassert_not_null(item, NULL);
    zassert_equal(strcmp(item->valuestring, "Hello\nWorld\t!"), 0, NULL);
    cJSON_Delete(item);

    /* \u0041 \u0042 \u0043 -> "ABC" */
    item = cJSON_Parse("\"\\u0041\\u0042\\u0043\"");
    zassert_not_null(item, NULL);
    zassert_equal(strcmp(item->valuestring, "ABC"), 0, NULL);
    cJSON_Delete(item);

    /* surrogate pair for U+1D11E (MUSICAL SYMBOL G CLEF),
     * UTF-8 encoded as F0 9D 84 9E */
    item = cJSON_Parse("\"\\uD834\\uDD1E\"");
    zassert_not_null(item, NULL);
    zassert_equal(strlen(item->valuestring), 4, NULL);
    zassert_mem_equal(item->valuestring, "\xF0\x9D\x84\x9E", 4, NULL);
    cJSON_Delete(item);
}

/**
 * @brief an invalid \\u escape (non-hex digits) is rejected rather
 * than silently decoding as U+0000 (regression test for the parser
 * hardening).
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cJSON_tests, testCJSON_InvalidUnicodeEscapeRejected)
#else
static void testCJSON_InvalidUnicodeEscapeRejected(void)
#endif
{
    cJSON *item;

    item = cJSON_Parse("\"\\uZZZZ\"");
    zassert_is_null(item, NULL);
}

/**
 * @brief a \\u escape truncated by the end of the input buffer (fewer
 * than 4 hex digits before the NUL terminator) must not read past the
 * end of the buffer, and must fail the parse rather than silently
 * treating it as a valid escape (regression test for the parser
 * hardening).
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cJSON_tests, testCJSON_TruncatedUnicodeEscapeRejected)
#else
static void testCJSON_TruncatedUnicodeEscapeRejected(void)
#endif
{
    cJSON *item;

    /* only 2 of 4 hex digits present before the string (and the
     * buffer) ends */
    item = cJSON_Parse("\"\\u12");
    zassert_is_null(item, NULL);

    item = cJSON_Parse("\"\\u");
    zassert_is_null(item, NULL);

    item = cJSON_Parse("\"\\u1");
    zassert_is_null(item, NULL);
}

/**
 * @brief arrays: empty, flat, nested, and index/size accessors
 * (subset of upstream tests/parse_array.c)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cJSON_tests, testCJSON_ParseArray)
#else
static void testCJSON_ParseArray(void)
#endif
{
    cJSON *item, *element;

    item = cJSON_Parse("[]");
    zassert_not_null(item, NULL);
    zassert_true(cJSON_IsArray(item), NULL);
    zassert_equal(cJSON_GetArraySize(item), 0, NULL);
    cJSON_Delete(item);

    item = cJSON_Parse("[1, 2, 3]");
    zassert_not_null(item, NULL);
    zassert_equal(cJSON_GetArraySize(item), 3, NULL);
    element = cJSON_GetArrayItem(item, 0);
    zassert_not_null(element, NULL);
    zassert_within(element->valuedouble, 1.0, 0.0001, NULL);
    element = cJSON_GetArrayItem(item, 2);
    zassert_not_null(element, NULL);
    zassert_within(element->valuedouble, 3.0, 0.0001, NULL);
    /* out of range */
    zassert_is_null(cJSON_GetArrayItem(item, 3), NULL);
    cJSON_Delete(item);

    item = cJSON_Parse("[[1, 2], [3, 4]]");
    zassert_not_null(item, NULL);
    zassert_equal(cJSON_GetArraySize(item), 2, NULL);
    element = cJSON_GetArrayItem(item, 1);
    zassert_not_null(element, NULL);
    zassert_true(cJSON_IsArray(element), NULL);
    zassert_equal(cJSON_GetArraySize(element), 2, NULL);
    cJSON_Delete(item);
}

/**
 * @brief objects: simple, nested, and key lookup (subset of upstream
 * tests/parse_object.c)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cJSON_tests, testCJSON_ParseObject)
#else
static void testCJSON_ParseObject(void)
#endif
{
    cJSON *item, *value;

    item = cJSON_Parse("{}");
    zassert_not_null(item, NULL);
    zassert_true(cJSON_IsObject(item), NULL);
    cJSON_Delete(item);

    item = cJSON_Parse("{\"name\":\"Joshua\",\"age\":21,\"ok\":true}");
    zassert_not_null(item, NULL);

    value = cJSON_GetObjectItem(item, "name");
    zassert_not_null(value, NULL);
    zassert_true(cJSON_IsString(value), NULL);
    zassert_equal(strcmp(value->valuestring, "Joshua"), 0, NULL);

    value = cJSON_GetObjectItem(item, "age");
    zassert_not_null(value, NULL);
    zassert_within(value->valuedouble, 21.0, 0.0001, NULL);

    value = cJSON_GetObjectItem(item, "ok");
    zassert_not_null(value, NULL);
    zassert_true(cJSON_IsTrue(value), NULL);

    /* missing key */
    zassert_is_null(cJSON_GetObjectItem(item, "missing"), NULL);
    cJSON_Delete(item);

    item = cJSON_Parse("{\"outer\":{\"inner\":42}}");
    zassert_not_null(item, NULL);
    value = cJSON_GetObjectItem(item, "outer");
    zassert_not_null(value, NULL);
    zassert_true(cJSON_IsObject(value), NULL);
    value = cJSON_GetObjectItem(value, "inner");
    zassert_not_null(value, NULL);
    zassert_within(value->valuedouble, 42.0, 0.0001, NULL);
    cJSON_Delete(item);
}

/**
 * @brief malformed input is rejected without crashing (subset of
 * upstream tests/parse_with_opts.c / misc_tests.c)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cJSON_tests, testCJSON_ParseInvalidInput)
#else
static void testCJSON_ParseInvalidInput(void)
#endif
{
    zassert_is_null(cJSON_Parse(""), NULL);
    zassert_is_null(cJSON_Parse("   "), NULL);
    zassert_is_null(cJSON_Parse("{"), NULL);
    zassert_is_null(cJSON_Parse("["), NULL);
    zassert_is_null(cJSON_Parse("{\"a\":}"), NULL);
    zassert_is_null(cJSON_Parse("{\"a\":1"), NULL);
    zassert_is_null(cJSON_Parse("[1, 2,"), NULL);
    zassert_is_null(cJSON_Parse("nul"), NULL);
    /* NULL input must not crash */
    zassert_is_null(cJSON_Parse(NULL), NULL);
}

/**
 * @brief documents known parser leniency (not a memory-safety issue):
 * a string missing its closing quote is accepted, truncated at the
 * end of the input buffer, rather than rejected. This mirrors
 * upstream cJSON's own documented leniency around malformed input;
 * it is called out here so a future tightening of the grammar has a
 * test to update rather than silently changing behaviour.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cJSON_tests, testCJSON_UnterminatedStringLeniency)
#else
static void testCJSON_UnterminatedStringLeniency(void)
#endif
{
    cJSON *item = cJSON_Parse("\"unterminated");

    zassert_not_null(item, NULL);
    zassert_true(cJSON_IsString(item), NULL);
    zassert_equal(strcmp(item->valuestring, "unterminated"), 0, NULL);
    cJSON_Delete(item);
}

/**
 * @brief deeply nested arrays are rejected before they can exhaust the
 * call stack, while moderate nesting still parses correctly
 * (regression test for the parser hardening).
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cJSON_tests, testCJSON_NestingLimit)
#else
static void testCJSON_NestingLimit(void)
#endif
{
    char *json;
    cJSON *item;

    /* well within any sane limit: must parse successfully */
    json = build_nested_array(50);
    item = cJSON_Parse(json);
    zassert_not_null(item, NULL);
    cJSON_Delete(item);
    free(json);

    /* far beyond the parser's nesting-depth limit: must be rejected,
     * not crash with a stack overflow */
    json = build_nested_array(5000);
    item = cJSON_Parse(json);
    zassert_is_null(item, NULL);
    free(json);
}

/**
 * @brief cJSON_GetErrorPtr() reflects a failed parse
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cJSON_tests, testCJSON_GetErrorPtr)
#else
static void testCJSON_GetErrorPtr(void)
#endif
{
    const char *json = "{\"a\":}";
    cJSON *item;

    item = cJSON_Parse(json);
    zassert_is_null(item, NULL);
    zassert_not_null(cJSON_GetErrorPtr(), NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(cJSON_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        cJSON_tests, ztest_unit_test(testCJSON_ParseLiterals),
        ztest_unit_test(testCJSON_ParseNumbers),
        ztest_unit_test(testCJSON_NumberValueintSaturates),
        ztest_unit_test(testCJSON_NumberOverflowRejected),
        ztest_unit_test(testCJSON_ParseStrings),
        ztest_unit_test(testCJSON_InvalidUnicodeEscapeRejected),
        ztest_unit_test(testCJSON_TruncatedUnicodeEscapeRejected),
        ztest_unit_test(testCJSON_ParseArray),
        ztest_unit_test(testCJSON_ParseObject),
        ztest_unit_test(testCJSON_ParseInvalidInput),
        ztest_unit_test(testCJSON_UnterminatedStringLeniency),
        ztest_unit_test(testCJSON_NestingLimit),
        ztest_unit_test(testCJSON_GetErrorPtr));

    ztest_run_test_suite(cJSON_tests);
}
#endif
