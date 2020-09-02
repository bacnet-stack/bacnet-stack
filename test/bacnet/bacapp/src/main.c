/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/bacapp.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
/* generic - can be used by other unit tests
   returns true if matching or same, false if different */
static bool bacapp_same_value(BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_APPLICATION_DATA_VALUE *test_value)
{
    bool status = false; /*return value */

    /* does the tag match? */
    if (test_value->tag == value->tag)
        status = true;
    if (status) {
        /* second test for same-ness */
        status = false;
        /* does the value match? */
        switch (test_value->tag) {
#if defined(BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                status = true;
                break;
#endif
#if defined(BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                if (test_value->type.Boolean == value->type.Boolean)
                    status = true;
                break;
#endif
#if defined(BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                if (test_value->type.Unsigned_Int == value->type.Unsigned_Int)
                    status = true;
                break;
#endif
#if defined(BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                if (test_value->type.Signed_Int == value->type.Signed_Int)
                    status = true;
                break;
#endif
#if defined(BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                if (test_value->type.Real == value->type.Real)
                    status = true;
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                if (test_value->type.Double == value->type.Double)
                    status = true;
                break;
#endif
#if defined(BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                if (test_value->type.Enumerated == value->type.Enumerated)
                    status = true;
                break;
#endif
#if defined(BACAPP_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                if (datetime_compare_date(
                        &test_value->type.Date, &value->type.Date) == 0)
                    status = true;
                break;
#endif
#if defined(BACAPP_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                if (datetime_compare_time(
                        &test_value->type.Time, &value->type.Time) == 0)
                    status = true;
                break;
#endif
#if defined(BACAPP_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                if ((test_value->type.Object_Id.type ==
                        value->type.Object_Id.type) &&
                    (test_value->type.Object_Id.instance ==
                        value->type.Object_Id.instance)) {
                    status = true;
                }
                break;
#endif
#if defined(BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                status = characterstring_same(&value->type.Character_String,
                    &test_value->type.Character_String);
                break;
#endif
#if defined(BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                status = octetstring_value_same(
                    &value->type.Octet_String, &test_value->type.Octet_String);
                break;
#endif
#if defined(BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                status = bitstring_same(
                    &value->type.Bit_String, &test_value->type.Bit_String);
                break;
#endif
#if 0 /*TODO: Enable when lighting.c builds cleanly */
#if defined(BACAPP_LIGHTING_COMMAND)
            case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
                status = lighting_command_same(&value->type.Lighting_Command,
                    &test_value->type.Lighting_Command);
                break;
#endif
#endif /*TODO: */
            default:
                status = false;
                break;
        }
    }
    return status;
}

/**
 * @brief Test
 */
static void testBACnetApplicationData_Safe(void)
{
    int i;
    uint8_t apdu[MAX_APDU];
    int len = 0;
    int apdu_len;
    BACNET_APPLICATION_DATA_VALUE input_value[13];
    uint32_t len_segment[13];
    uint32_t single_length_segment;
    BACNET_APPLICATION_DATA_VALUE value;

    for (i = 0; i < 13; i++) {
        input_value[i].tag = (BACNET_APPLICATION_TAG)i;
        input_value[i].context_specific = 0;
        input_value[i].context_tag = 0;
        input_value[i].next = NULL;
        switch (input_value[i].tag) {
            case BACNET_APPLICATION_TAG_NULL:
                /* NULL: no data */
                break;

            case BACNET_APPLICATION_TAG_BOOLEAN:
                input_value[i].type.Boolean = true;
                break;

            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                input_value[i].type.Unsigned_Int = 0xDEADBEEF;
                break;

            case BACNET_APPLICATION_TAG_SIGNED_INT:
                input_value[i].type.Signed_Int = 0x00C0FFEE;
                break;
            case BACNET_APPLICATION_TAG_REAL:
                input_value[i].type.Real = 3.141592654f;
                break;
            case BACNET_APPLICATION_TAG_DOUBLE:
                input_value[i].type.Double = 2.32323232323;
                break;

            case BACNET_APPLICATION_TAG_OCTET_STRING: {
                uint8_t test_octet[5] = { "Karg" };
                octetstring_init(&input_value[i].type.Octet_String, test_octet,
                    sizeof(test_octet));
            } break;

            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                characterstring_init_ansi(
                    &input_value[i].type.Character_String, "Hello There!");
                break;

            case BACNET_APPLICATION_TAG_BIT_STRING:
                bitstring_init(&input_value[i].type.Bit_String);
                bitstring_set_bit(&input_value[i].type.Bit_String, 0, true);
                bitstring_set_bit(&input_value[i].type.Bit_String, 1, false);
                bitstring_set_bit(&input_value[i].type.Bit_String, 2, false);
                bitstring_set_bit(&input_value[i].type.Bit_String, 3, true);
                bitstring_set_bit(&input_value[i].type.Bit_String, 4, false);
                bitstring_set_bit(&input_value[i].type.Bit_String, 5, true);
                bitstring_set_bit(&input_value[i].type.Bit_String, 6, true);
                break;

            case BACNET_APPLICATION_TAG_ENUMERATED:
                input_value[i].type.Enumerated = 0x0BADF00D;
                break;

            case BACNET_APPLICATION_TAG_DATE:
                input_value[i].type.Date.day = 10;
                input_value[i].type.Date.month = 9;
                input_value[i].type.Date.wday = 3;
                input_value[i].type.Date.year = 1998;
                break;

            case BACNET_APPLICATION_TAG_TIME:
                input_value[i].type.Time.hour = 12;
                input_value[i].type.Time.hundredths = 56;
                input_value[i].type.Time.min = 20;
                input_value[i].type.Time.sec = 41;
                break;

            case BACNET_APPLICATION_TAG_OBJECT_ID:
                input_value[i].type.Object_Id.instance = 1234;
                input_value[i].type.Object_Id.type = 12;
                break;

            default:
                break;
        }
        single_length_segment = bacapp_encode_data(&apdu[len], &input_value[i]);
        ;
        zassert_true(single_length_segment > 0, NULL);
        /* len_segment is accumulated length */
        if (i == 0) {
            len_segment[i] = single_length_segment;
        } else {
            len_segment[i] = single_length_segment + len_segment[i - 1];
        }
        len = len_segment[i];
    }
    /*
     ** Start processing packets at processivly truncated lengths
     */

    for (apdu_len = len; apdu_len >= 0; apdu_len--) {
        bool status;
        bool expected_status;
        for (i = 0; i < 14; i++) {
            if (i == 13) {
                expected_status = false;
            } else {
                if (apdu_len < len_segment[i]) {
                    expected_status = false;
                } else {
                    expected_status = true;
                }
            }
            status = bacapp_decode_application_data_safe(
                i == 0 ? apdu : NULL, apdu_len, &value);

            zassert_equal(status, expected_status, NULL);
            if (status) {
                zassert_equal(value.tag, i, NULL);
                zassert_true(bacapp_same_value(&input_value[i], &value), NULL);
                zassert_true(!value.context_specific, NULL);
                zassert_is_null(value.next, NULL);
            } else {
                break;
            }
        }
    }
}

/**
 * @brief Test
 */
static void testBACnetApplicationDataLength(void)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0; /* total length of the apdu, return value */
    int test_len = 0; /* length of the data */
    uint8_t apdu[480] = { 0 };
    BACNET_TIME local_time;
    BACNET_DATE local_date;

    /* create some constructed data */
    /* 1. zero elements */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len = bacapp_data_len(
        &apdu[0], apdu_len, PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES);
    zassert_equal(test_len, len, NULL);

    /* 2. application tagged data, one element */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_application_unsigned(&apdu[apdu_len], 4194303);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len = bacapp_data_len(&apdu[0], apdu_len, PROP_OBJECT_IDENTIFIER);
    zassert_equal(test_len, len, NULL);

    /* 3. application tagged data, multiple elements */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_unsigned(&apdu[apdu_len], 1);
    test_len += len;
    apdu_len += len;
    len = encode_application_unsigned(&apdu[apdu_len], 42);
    test_len += len;
    apdu_len += len;
    len = encode_application_unsigned(&apdu[apdu_len], 91);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len = bacapp_data_len(&apdu[0], apdu_len, PROP_PRIORITY_ARRAY);
    zassert_equal(test_len, len, NULL);

    /* 4. complex datatype - one element */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    test_len += len;
    apdu_len += len;
    local_date.year = 2006; /* AD */
    local_date.month = 4; /* 1=Jan */
    local_date.day = 1; /* 1..31 */
    local_date.wday = 6; /* 1=Monday */
    len = encode_application_date(&apdu[apdu_len], &local_date);
    test_len += len;
    apdu_len += len;
    local_time.hour = 7;
    local_time.min = 0;
    local_time.sec = 3;
    local_time.hundredths = 1;
    len = encode_application_time(&apdu[apdu_len], &local_time);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len = bacapp_data_len(&apdu[0], apdu_len, PROP_START_TIME);
    zassert_equal(test_len, len, NULL);

    /* 5. complex datatype - multiple elements */

    /* 6. context tagged data, one element */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_context_unsigned(&apdu[apdu_len], 1, 91);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len = bacapp_data_len(&apdu[0], apdu_len, PROP_REQUESTED_SHED_LEVEL);
    zassert_equal(test_len, len, NULL);
}

/**
 * @brief Test
 */
static bool verifyBACnetApplicationDataValue(BACNET_APPLICATION_DATA_VALUE *value)
{
    uint8_t apdu[480] = { 0 };
    int apdu_len = 0;
    BACNET_APPLICATION_DATA_VALUE test_value;

    apdu_len = bacapp_encode_application_data(&apdu[0], value);
    bacapp_decode_application_data(&apdu[0], apdu_len, &test_value);

    return bacapp_same_value(value, &test_value);
}

/**
 * @brief Test
 */
static void testBACnetApplicationData(void)
{
    BACNET_APPLICATION_DATA_VALUE value;
    bool status = false;

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_NULL, NULL, &value);
    zassert_true(status, NULL);
    status = verifyBACnetApplicationDataValue(&value);
    zassert_true(status, NULL);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_BOOLEAN, "1", &value);
    zassert_true(status, NULL);
    zassert_true(value.type.Boolean, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_BOOLEAN, "0", &value);
    zassert_true(status, NULL);
    zassert_false(value.type.Boolean, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_UNSIGNED_INT, "0", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Unsigned_Int, 0, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_UNSIGNED_INT, "0xFFFF", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Unsigned_Int, 0xFFFF, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_UNSIGNED_INT, "0xFFFFFFFF", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Unsigned_Int, 0xFFFFFFFF, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_SIGNED_INT, "0", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Signed_Int, 0, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_SIGNED_INT, "-1", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Signed_Int, -1, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_SIGNED_INT, "32768", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Signed_Int, 32768, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_SIGNED_INT, "-32768", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Signed_Int, -32768, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_REAL, "0.0", &value);
    zassert_true(status, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_REAL, "-1.0", &value);
    zassert_true(status, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_REAL, "1.0", &value);
    zassert_true(status, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_REAL, "3.14159", &value);
    zassert_true(status, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_REAL, "-3.14159", &value);
    zassert_true(status, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_ENUMERATED, "0", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Enumerated, 0, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_ENUMERATED, "0xFFFF", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Enumerated, 0xFFFF, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_ENUMERATED, "0xFFFFFFFF", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Enumerated, 0xFFFFFFFF, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_DATE, "2005/5/22:1", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Date.year, 2005, NULL);
    zassert_equal(value.type.Date.month, 5, NULL);
    zassert_equal(value.type.Date.day, 22, NULL);
    zassert_equal(value.type.Date.wday, 1, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    /* Happy Valentines Day! */
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_DATE, "2007/2/14", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Date.year, 2007, NULL);
    zassert_equal(value.type.Date.month, 2, NULL);
    zassert_equal(value.type.Date.day, 14, NULL);
    zassert_equal(value.type.Date.wday, BACNET_WEEKDAY_WEDNESDAY, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    /* Wildcard Values */
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_DATE, "2155/255/255:255", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Date.year, 2155, NULL);
    zassert_equal(value.type.Date.month, 255, NULL);
    zassert_equal(value.type.Date.day, 255, NULL);
    zassert_equal(value.type.Date.wday, 255, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_TIME, "23:59:59.12", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Time.hour, 23, NULL);
    zassert_equal(value.type.Time.min, 59, NULL);
    zassert_equal(value.type.Time.sec, 59, NULL);
    zassert_equal(value.type.Time.hundredths, 12, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_TIME, "23:59:59", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Time.hour, 23, NULL);
    zassert_equal(value.type.Time.min, 59, NULL);
    zassert_equal(value.type.Time.sec, 59, NULL);
    zassert_equal(value.type.Time.hundredths, 0, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_TIME, "23:59", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Time.hour, 23, NULL);
    zassert_equal(value.type.Time.min, 59, NULL);
    zassert_equal(value.type.Time.sec, 0, NULL);
    zassert_equal(value.type.Time.hundredths, 0, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    /* Wildcard Values */
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_TIME, "255:255:255.255", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Time.hour, 255, NULL);
    zassert_equal(value.type.Time.min, 255, NULL);
    zassert_equal(value.type.Time.sec, 255, NULL);
    zassert_equal(value.type.Time.hundredths, 255, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_OBJECT_ID, "0:100", &value);
    zassert_true(status, NULL);
    zassert_equal(value.type.Object_Id.type, 0, NULL);
    zassert_equal(value.type.Object_Id.instance, 100, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_CHARACTER_STRING, "Karg!", &value);
    zassert_true(status, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    /* test empty string */
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_CHARACTER_STRING, "", &value);
    zassert_true(status, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_OCTET_STRING, "1234567890ABCDEF", &value);
    zassert_true(status, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_OCTET_STRING, "12-34-56-78-90-AB-CD-EF", &value);
    zassert_true(status, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_OCTET_STRING, "12 34 56 78 90 AB CD EF", &value);
    zassert_true(status, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);
    /* test empty string */
    status = bacapp_parse_application_data(
        BACNET_APPLICATION_TAG_OCTET_STRING, "", &value);
    zassert_true(status, NULL);
    zassert_true(verifyBACnetApplicationDataValue(&value), NULL);

    return;
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(bacapp_tests,
     ztest_unit_test(testBACnetApplicationData),
     ztest_unit_test(testBACnetApplicationDataLength),
     ztest_unit_test(testBACnetApplicationData_Safe)
     );

    ztest_run_test_suite(bacapp_tests);
}
