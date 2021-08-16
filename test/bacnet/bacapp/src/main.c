/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <stdint.h>
#include <string.h>
#include <ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/bacapp.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

static void test_bacapp_decode_application_data(void)
{
    uint8_t apdu[128] = { 0 };
    //unsigned max_apdu_len = sizeof(apdu);
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    zassert_equal(bacapp_decode_application_data(NULL, sizeof(apdu), &value), 0, NULL);
    zassert_equal(bacapp_decode_application_data(apdu, 0, &value), 0, NULL);
    zassert_equal(bacapp_decode_application_data(apdu, sizeof(apdu), NULL), 0, NULL);
}


static void test_bacapp_decode_data_len(void)
{
    uint8_t apdu[3] = { 0 };
    uint32_t len_value_type = 0;
    int expected_value = 0;

    zassert_equal(bacapp_decode_data_len(NULL, BACNET_APPLICATION_TAG_NULL, sizeof(apdu)), 0, NULL);
    zassert_equal(bacapp_decode_data_len(apdu, UINT8_MAX, sizeof(apdu)), 0, NULL);

    expected_value = (int) (~0U >> 1);  /* INT_MAX is not universally defined */
    zassert_equal(bacapp_decode_data_len(apdu, BACNET_APPLICATION_TAG_UNSIGNED_INT, UINT32_MAX), expected_value, NULL);

    zassert_equal(bacapp_decode_data_len(apdu, BACNET_APPLICATION_TAG_NULL, sizeof(apdu)), 0, NULL);
    zassert_equal(bacapp_decode_data_len(apdu, BACNET_APPLICATION_TAG_BOOLEAN, sizeof(apdu)), 0, NULL);

    len_value_type = INT32_MAX - 1;
    expected_value = (int) len_value_type;
    zassert_equal(bacapp_decode_data_len(apdu, BACNET_APPLICATION_TAG_UNSIGNED_INT, len_value_type), expected_value, NULL);

    len_value_type = INT32_MAX - 2;
    expected_value = (int) len_value_type;
    zassert_equal(bacapp_decode_data_len(apdu, BACNET_APPLICATION_TAG_SIGNED_INT, len_value_type), expected_value, NULL);

    len_value_type = INT32_MAX - 5;
    expected_value = (int) len_value_type;
    zassert_equal(bacapp_decode_data_len(apdu, BACNET_APPLICATION_TAG_REAL, len_value_type), expected_value, NULL);

    len_value_type = INT32_MAX - 9;
    expected_value = (int) len_value_type;
    zassert_equal(bacapp_decode_data_len(apdu, BACNET_APPLICATION_TAG_DOUBLE, len_value_type), expected_value, NULL);

    len_value_type = INT32_MAX - 13;
    expected_value = (int) len_value_type;
    zassert_equal(bacapp_decode_data_len(apdu, BACNET_APPLICATION_TAG_OCTET_STRING, len_value_type), expected_value, NULL);

    len_value_type = INT32_MAX - 17;
    expected_value = (int) len_value_type;
    zassert_equal(bacapp_decode_data_len(apdu, BACNET_APPLICATION_TAG_CHARACTER_STRING, len_value_type), expected_value, NULL);

    len_value_type = INT32_MAX - 19;
    expected_value = (int) len_value_type;
    zassert_equal(bacapp_decode_data_len(apdu, BACNET_APPLICATION_TAG_BIT_STRING, len_value_type), expected_value, NULL);

    len_value_type = INT32_MAX - 23;
    expected_value = (int) len_value_type;
    zassert_equal(bacapp_decode_data_len(apdu, BACNET_APPLICATION_TAG_ENUMERATED, len_value_type), expected_value, NULL);

    len_value_type = INT32_MAX - 29;
    expected_value = (int) len_value_type;
    zassert_equal(bacapp_decode_data_len(apdu, BACNET_APPLICATION_TAG_DATE, len_value_type), expected_value, NULL);

    len_value_type = INT32_MAX - 31;
    expected_value = (int) len_value_type;
    zassert_equal(bacapp_decode_data_len(apdu, BACNET_APPLICATION_TAG_TIME, len_value_type), expected_value, NULL);

    len_value_type = INT32_MAX - 37;
    expected_value = (int) len_value_type;
    zassert_equal(bacapp_decode_data_len(apdu, BACNET_APPLICATION_TAG_OBJECT_ID, len_value_type), expected_value, NULL);
}

static void test_bacapp_copy(void)
{
    int i = 0;

    BACNET_APPLICATION_DATA_VALUE src_value = { 0 };
    BACNET_APPLICATION_DATA_VALUE dest_value = { 0 };

    zassert_false(bacapp_copy(NULL, &src_value), NULL);
    zassert_false(bacapp_copy(&dest_value, NULL), NULL);

    memset(&src_value, 0xAA, sizeof(src_value));
    memset(&dest_value, 0, sizeof(dest_value));
    zassert_false(bacapp_copy(&dest_value, &src_value), NULL);
    zassert_equal(dest_value.tag, src_value.tag, NULL);
    zassert_equal(dest_value.next, src_value.next, NULL);

    const BACNET_APPLICATION_TAG tags[] = {
            BACNET_APPLICATION_TAG_NULL,
        #if defined(BACAPP_BOOLEAN)
            BACNET_APPLICATION_TAG_BOOLEAN,
        #endif
        #if defined(BACAPP_UNSIGNED)
            BACNET_APPLICATION_TAG_UNSIGNED_INT,
        #endif
        #if defined(BACAPP_SIGNED)
            BACNET_APPLICATION_TAG_SIGNED_INT,
        #endif
        #if defined(BACAPP_REAL)
            BACNET_APPLICATION_TAG_REAL,
        #endif
        #if defined(BACAPP_DOUBLE)
            BACNET_APPLICATION_TAG_DOUBLE,
        #endif
        #if defined(BACAPP_OCTET_STRING)
            BACNET_APPLICATION_TAG_OCTET_STRING,
        #endif
        #if defined(BACAPP_CHARACTER_STRING)
            BACNET_APPLICATION_TAG_CHARACTER_STRING,
        #endif
        #if defined(BACAPP_BIT_STRING)
            BACNET_APPLICATION_TAG_BIT_STRING,
        #endif
        #if defined(BACAPP_ENUMERATED)
            BACNET_APPLICATION_TAG_ENUMERATED,
        #endif
        #if defined(BACAPP_DATE)
            BACNET_APPLICATION_TAG_DATE,
        #endif
        #if defined(BACAPP_TIME)
            BACNET_APPLICATION_TAG_TIME,
        #endif
        #if defined(BACAPP_OBJECT_ID)
            BACNET_APPLICATION_TAG_OBJECT_ID,
        #endif
        #if defined(BACAPP_LIGHTING_COMMAND)
            BACNET_APPLICATION_TAG_LIGHTING_COMMAND,
        #endif
    };
    for (i = 0; i < sizeof(tags)/sizeof(tags[0]); ++i) {
        BACNET_APPLICATION_TAG tag = tags[i];
        bool expected_result = true;

        #if ! defined(BACAPP_NULL)
        if (tag == BACNET_APPLICATION_TAG_NULL) {
            expected_result = false;
        }
        #endif

        memset(&src_value, 0, sizeof(src_value));
        memset(&dest_value, 0xBB, sizeof(dest_value));
        src_value.tag = tag;
        src_value.next = (struct BACnet_Application_Data_Value *)(((uint32_t)tags[i]) << 8);
        zassert_equal(bacapp_copy(&dest_value, &src_value), expected_result, NULL);
        zassert_true(bacapp_same_value(&dest_value, &src_value), NULL);
        zassert_equal(dest_value.next, src_value.next, NULL);
    }
}

static void test_bacapp_value_list_init(void)
{
    BACNET_APPLICATION_DATA_VALUE value[2] = { { 0 } };
    size_t max_count = 0;
    size_t count = 0;

    /* Verify NULL ptr is properly handled */
    bacapp_value_list_init(NULL, 1);

    /* Verify zero length is properly handled */
    value[0] = value[1]; /* Struct copy */
    bacapp_value_list_init(&value[0], 0);
    zassert_equal(memcmp(&value[0], &value[1], sizeof(value[1])), 0, NULL);
    /* Verify one structure is initialized correctly */
    for (max_count = 1; max_count < sizeof(value)/sizeof(value[0]); ++max_count) {
        memset(value, 0, sizeof(value));
        max_count = 1;
        bacapp_value_list_init(&value[0], max_count);

        for (count = 0; count < max_count; ++count) {
            zassert_equal(value[count].tag, BACNET_APPLICATION_TAG_NULL, NULL);
            zassert_equal(value[count].context_specific, 0, NULL);
            zassert_equal(value[count].context_tag, 0, NULL);
            zassert_equal(value[count].next, ((count + 1 >= max_count) ? NULL : &value[count + 1]), NULL);
        }
    }
}

static void test_bacapp_property_value_list_init(void)
{
    BACNET_PROPERTY_VALUE value[2] = { { 0 } };
    size_t max_count = 0;
    size_t count = 0;

    /* Verify NULL ptr is properly handled */
    bacapp_property_value_list_init(NULL, 1);

    /* Verify zero length is properly handled */
    value[0] = value[1]; /* Struct copy */
    bacapp_property_value_list_init(&value[0], 0);
    zassert_equal(memcmp(&value[0], &value[1], sizeof(value[1])), 0, NULL);

    /* Verify one structure is initialized correctly */
    for (max_count = 1; max_count < sizeof(value)/sizeof(value[0]); ++max_count) {
        memset(value, 0, sizeof(value));
        max_count = 1;
        bacapp_property_value_list_init(&value[0], max_count);

        for (count = 0; count < max_count; ++count) {
            zassert_equal(value[count].propertyIdentifier, MAX_BACNET_PROPERTY_ID, NULL);
            zassert_equal(value[count].propertyArrayIndex, BACNET_ARRAY_ALL, NULL);
            zassert_equal(value[count].priority, BACNET_NO_PRIORITY, NULL);
            zassert_equal(value[count].next, ((count + 1 >= max_count) ? NULL : &value[count + 1]), NULL);
        }
    }
}

static void test_bacapp_same_value(void)
{
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_APPLICATION_DATA_VALUE test_value = { 0 };

    zassert_false(bacapp_same_value(NULL, &test_value), NULL);
    zassert_false(bacapp_same_value(&value, NULL), NULL);

    value.tag = ~test_value.tag;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);

    test_value.tag = BACNET_APPLICATION_TAG_NULL;
    value.tag = test_value.tag;
#if defined(BACAPP_NULL)
    zassert_true(bacapp_same_value(&value, &test_value), NULL);
#else
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif


    test_value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    value.tag = test_value.tag;
#if defined(BACAPP_BOOLEAN)
    zassert_true(bacapp_same_value(&value, &test_value), NULL);
#else
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif
    value.type.Boolean = !test_value.type.Boolean;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);


    memset(&test_value, 0, sizeof(test_value));
    test_value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value = test_value;  /* Struct copy */
#if defined(BACAPP_UNSIGNED)
    zassert_true(bacapp_same_value(&value, &test_value), NULL);
#else
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif
    value.type.Unsigned_Int = ~test_value.type.Unsigned_Int;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);


    memset(&test_value, 0, sizeof(test_value));
    test_value.tag = BACNET_APPLICATION_TAG_SIGNED_INT;
    value = test_value;  /* Struct copy */
#if defined(BACAPP_SIGNED)
    zassert_true(bacapp_same_value(&value, &test_value), NULL);
#else
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif
    value.type.Signed_Int = test_value.type.Signed_Int + 1;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);


    memset(&test_value, 0, sizeof(test_value));
    test_value.tag = BACNET_APPLICATION_TAG_REAL;
    value = test_value;  /* Struct copy */
#if defined(BACAPP_REAL)
    zassert_true(bacapp_same_value(&value, &test_value), NULL);
#else
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif
    value.type.Real = test_value.type.Real + 1.0f;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);


    memset(&test_value, 0, sizeof(test_value));
    test_value.tag = BACNET_APPLICATION_TAG_DOUBLE;
    value = test_value;  /* Struct copy */
#if defined(BACAPP_DOUBLE)
    zassert_true(bacapp_same_value(&value, &test_value), NULL);
#else
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif
    value.type.Double = test_value.type.Double + 1.0;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);


    memset(&test_value, 0, sizeof(test_value));
    test_value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
    value = test_value;  /* Struct copy */
#if defined(BACAPP_ENUMERATED)
    zassert_true(bacapp_same_value(&value, &test_value), NULL);
#else
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif
    value.type.Enumerated = test_value.type.Enumerated + 1;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);


    memset(&test_value, 0, sizeof(test_value));
    test_value.tag = BACNET_APPLICATION_TAG_DATE;
    value = test_value;  /* Struct copy */
#if defined(BACAPP_DATE)
    zassert_true(bacapp_same_value(&value, &test_value), NULL);
#else
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif

    value = test_value;  /* Struct copy */
    value.type.Date.day = test_value.type.Date.day + 1;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);

#if 0 /*REVISIT: wday is not compared! */
    value = test_value;  /* Struct copy */
    value.type.Date.wday = test_value.type.Date.wday + 1;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif

    value = test_value;  /* Struct copy */
    value.type.Date.month = test_value.type.Date.month + 1;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);

    value = test_value;  /* Struct copy */
    value.type.Date.year = test_value.type.Date.year + 1;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);


    memset(&test_value, 0, sizeof(test_value));
    test_value.tag = BACNET_APPLICATION_TAG_TIME;
    value = test_value;  /* Struct copy */
#if defined(BACAPP_TIME)
    zassert_true(bacapp_same_value(&value, &test_value), NULL);
#else
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif

    value = test_value;  /* Struct copy */
    value.type.Time.hour = test_value.type.Time.hour + 1;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);

    value = test_value;  /* Struct copy */
    value.type.Time.min = test_value.type.Time.min + 1;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);

    value = test_value;  /* Struct copy */
    value.type.Time.sec = test_value.type.Time.sec + 1;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);

    value = test_value;  /* Struct copy */
    value.type.Time.hundredths = test_value.type.Time.hundredths + 1;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);


    memset(&test_value, 0, sizeof(test_value));
    test_value.tag = BACNET_APPLICATION_TAG_OBJECT_ID;
    value = test_value;  /* Struct copy */
#if defined(BACAPP_OBJECT_ID)
    zassert_true(bacapp_same_value(&value, &test_value), NULL);
#else
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif

    value = test_value;  /* Struct copy */
    value.type.Object_Id.type = test_value.type.Object_Id.type + 1;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);

    value = test_value;  /* Struct copy */
    value.type.Object_Id.instance = test_value.type.Object_Id.instance + 1;
    zassert_false(bacapp_same_value(&value, &test_value), NULL);


    memset(&test_value, 0, sizeof(test_value));
    test_value.tag = BACNET_APPLICATION_TAG_CHARACTER_STRING;
    value = test_value;  /* Struct copy */
#if defined(BACAPP_CHARACTER_STRING)
    zassert_true(bacapp_same_value(&value, &test_value), NULL);
#else
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif
    //TODO: Verify .type.Character_String value compared


    memset(&test_value, 0, sizeof(test_value));
    test_value.tag = BACNET_APPLICATION_TAG_OCTET_STRING;
    value = test_value;  /* Struct copy */
#if defined(BACAPP_OCTET_STRING)
    zassert_true(bacapp_same_value(&value, &test_value), NULL);
#else
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif
    //TODO: Verify .type.Octet_String value compared


    memset(&test_value, 0, sizeof(test_value));
    test_value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
    value = test_value;  /* Struct copy */
#if defined(BACAPP_BIT_STRING)
    zassert_true(bacapp_same_value(&value, &test_value), NULL);
#else
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif
    //TODO: Verify .type.Bit_String value compared


    memset(&test_value, 0, sizeof(test_value));
    test_value.tag = BACNET_APPLICATION_TAG_LIGHTING_COMMAND;
    value = test_value;  /* Struct copy */
#if defined(BACAPP_LIGHTING_COMMAND)
    zassert_true(bacapp_same_value(&value, &test_value), NULL);
#else
    zassert_false(bacapp_same_value(&value, &test_value), NULL);
#endif
    //TODO: Verify .type.Lighting_Command value compared
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
     ztest_unit_test(test_bacapp_decode_application_data),
     ztest_unit_test(test_bacapp_decode_data_len),
     ztest_unit_test(test_bacapp_copy),
     ztest_unit_test(test_bacapp_value_list_init),
     ztest_unit_test(test_bacapp_property_value_list_init),
     ztest_unit_test(test_bacapp_same_value),
     ztest_unit_test(testBACnetApplicationData),
     ztest_unit_test(testBACnetApplicationDataLength),
     ztest_unit_test(testBACnetApplicationData_Safe)
     );

    ztest_run_test_suite(bacapp_tests);
}
