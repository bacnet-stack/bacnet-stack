/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bacnet/bacerror.h"

#include <errno.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>

#include "fakes/bacdcode.h"

#if 0
#include <zephyr/fff_extensions.h>
#else

/* The helpful macro below is from a Zephyr v3.3.0+ PR currently under review.
 * Once it is in a Zephyr release, then it can be provided via #include
 * rather than declared locally.
 */
#ifndef RETURN_HANDLED_CONTEXT
#define RETURN_HANDLED_CONTEXT(                                        \
    FUNCNAME, CONTEXTTYPE, RESULTFIELD, CONTEXTPTRNAME, HANDLERBODY)   \
    if (FUNCNAME##_fake.return_val_seq_len) {                          \
        CONTEXTTYPE *const contexts = CONTAINER_OF(                    \
            FUNCNAME##_fake.return_val_seq, CONTEXTTYPE, RESULTFIELD); \
        size_t const seq_idx = (FUNCNAME##_fake.return_val_seq_idx <   \
                                   FUNCNAME##_fake.return_val_seq_len) \
            ? FUNCNAME##_fake.return_val_seq_idx++                     \
            : FUNCNAME##_fake.return_val_seq_idx - 1;                  \
        CONTEXTTYPE *const CONTEXTPTRNAME = &contexts[seq_idx];        \
        HANDLERBODY;                                                   \
    }                                                                  \
    return FUNCNAME##_fake.return_val
#endif /* RETURN_HANDLED_CONTEXT */

#endif

#define RESET_HISTORY_AND_FAKES()                                \
    BACNET_STACK_TEST_BACNET_BACDCODE_FFF_FAKES_LIST(RESET_FAKE) \
    FFF_RESET_HISTORY()

DEFINE_FFF_GLOBALS;

/*
 * Custom Fakes:
 */
struct encode_application_enumerated_custom_fake_context {
    uint8_t *const apdu_expected;
    uint32_t value_expected;

    /* Written to client by custom fake */
    const uint8_t *const encoded_enumerated;
    int const encoded_enumerated_len;

    int result;
};

static int encode_application_enumerated_custom_fake(
    uint8_t *apdu, uint32_t enumerated)
{
    RETURN_HANDLED_CONTEXT(encode_application_enumerated,
        struct encode_application_enumerated_custom_fake_context,
        result, /* return field name in _fake_context struct */
        context, /* Name of context ptr variable used below */
        {
            if (context != NULL) {
                if (context->result == 0) {
                    if (apdu != NULL) {
                        memcpy(apdu, context->encoded_enumerated,
                            context->encoded_enumerated_len);
                    }
                }

                return context->result;
            }

            return encode_application_enumerated_fake.return_val;
        });
}

struct decode_tag_number_and_value_custom_fake_context {
    uint8_t *const apdu_expected;

    /* Written to client by custom fake */
    uint8_t const tag_number;
    uint32_t const value;

    int result;
};

static int decode_tag_number_and_value_custom_fake(
    uint8_t *apdu, uint8_t *tag_number, uint32_t *value)
{
    RETURN_HANDLED_CONTEXT(decode_tag_number_and_value,
        struct decode_tag_number_and_value_custom_fake_context,
        result, /* return field name in _fake_context struct */
        context, /* Name of context ptr variable used below */
        {
            if (context != NULL) {
                if (context->result > 0) {
                    if (tag_number != NULL)
                        *tag_number = context->tag_number;
                    if (value != NULL)
                        *value = context->value;
                }

                return context->result;
            }

            return decode_tag_number_and_value_fake.return_val;
        });
}

struct decode_enumerated_custom_fake_context {
    uint8_t *const apdu_expected;

    /* Written to client by custom fake */
    uint32_t const value;

    int result;
};

static int decode_enumerated_custom_fake(
    uint8_t *apdu, uint32_t len_value, uint32_t *value)
{
    RETURN_HANDLED_CONTEXT(decode_enumerated,
        struct decode_enumerated_custom_fake_context,
        result, /* return field name in _fake_context struct */
        context, /* Name of context ptr variable used below */
        {
            if (context != NULL) {
                if (context->result > 0) {
                    if (value != NULL)
                        *value = context->value;
                }

                return context->result;
            }

            return decode_enumerated_fake.return_val;
        });
}

/*
 * Tests:
 */

static void test_bacerror_encode_apdu(void)
{
    uint8_t test_apdu[32] = { 0 };

    struct test_case {
        const char *description_oneliner;

        uint8_t apdu_prefill;

        uint8_t *apdu;
        uint8_t invoke_id;
        BACNET_CONFIRMED_SERVICE service;
        BACNET_ERROR_CLASS error_class;
        BACNET_ERROR_CODE error_code;

        void **expected_call_history;

        /* Last FFF sequence entry is reused for excess calls.
         * Have an extra entry that returns a distinct failure (-E2BIG)
         *
         * Expect one less call than _len, or 0 if sequence ptr is NULL
         *
         * Configure to return -E2BIG if excess calls.
         */
        int encode_application_enumerated_custom_fake_contexts_len;
        struct encode_application_enumerated_custom_fake_context
            *encode_application_enumerated_custom_fake_contexts;

        int result_expected;
    } const test_cases[] = {
        {
            .description_oneliner = "Call with NULL apdu",

            .apdu = NULL,

            .expected_call_history =
                (void *[]) {
                    NULL, /* mark end of array */
                },

            .result_expected = 0, /* zero total length of APDU */
        },
        {
            .description_oneliner =
                "Call with valid apdu, return negative apdu_len",

            .apdu_prefill = 0x55U, /* arbitrary */

            .apdu = &test_apdu[0],
            .invoke_id = 0xFEU,
            .service = MAX_BACNET_CONFIRMED_SERVICE,
            .error_class = ERROR_CLASS_PROPRIETARY_FIRST,
            .error_code = ERROR_CODE_PROPRIETARY_LAST,

            .expected_call_history =
                (void *[]) {
                    encode_application_enumerated,
                    encode_application_enumerated, NULL, /* mark end of array */
                },

            .encode_application_enumerated_custom_fake_contexts_len = 3,
            .encode_application_enumerated_custom_fake_contexts =
                (struct encode_application_enumerated_custom_fake_context[]) {
                    {
                        .apdu_expected = &test_apdu[3],
                        .value_expected = ERROR_CLASS_PROPRIETARY_FIRST,
                        .result = -2,
                    },
                    {
                        .apdu_expected = &test_apdu[1],
                        .value_expected = ERROR_CODE_PROPRIETARY_LAST,
                        .result = -1,
                    },
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .result_expected = 0, /* zero total length of APDU */
        },
        {
            .description_oneliner =
                "Call with valid apdu, return positive apdu_len",

            .apdu_prefill = 0xABU, /* arbitrary */

            .apdu = &test_apdu[0],
            .invoke_id = 0xFEU,
            .service = SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM,
            .error_class = ERROR_CLASS_PROPRIETARY_LAST,
            .error_code = ERROR_CODE_PROPRIETARY_FIRST,

            .expected_call_history =
                (void *[]) {
                    encode_application_enumerated,
                    encode_application_enumerated, NULL, /* mark end of array */
                },

            .encode_application_enumerated_custom_fake_contexts_len = 3,
            .encode_application_enumerated_custom_fake_contexts =
                (struct encode_application_enumerated_custom_fake_context[]) {
                    {
                        .apdu_expected = &test_apdu[3],
                        .value_expected = ERROR_CLASS_PROPRIETARY_LAST,
                        .result = 4,
                    },
                    {
                        .apdu_expected = &test_apdu[7],
                        .value_expected = ERROR_CODE_PROPRIETARY_FIRST,
                        .result = 5,
                    },
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .result_expected = 3 + 4 + 5, /* total length of APDU */
        },
    };

    for (int i = 0; i < ARRAY_SIZE(test_cases); ++i) {
        const struct test_case *const tc = &test_cases[i];

        printk("Checking test_cases[%i]: %s\n", i,
            (tc->description_oneliner != NULL) ? tc->description_oneliner : "");

        /*
         * Set up pre-conditions
         */
        RESET_HISTORY_AND_FAKES();

        /* NOTE: Point to the return type field in the first returns struct.
         *       This custom_fake:
         *     - uses *_fake.return_val_seq and CONTAINER_OF()
         *         to determine the beginning of the array of structures.
         *     - uses *_fake.return_val_seq_id to index into
         *         the array of structures.
         *       This overloading is to allow the return_val_seq to
         *       also contain call-specific output parameters to be
         *       applied by the custom_fake.
         */
        encode_application_enumerated_fake.return_val =
            -E2BIG; /* for excessive calls */
        SET_RETURN_SEQ(encode_application_enumerated,
            &tc->encode_application_enumerated_custom_fake_contexts[0].result,
            tc->encode_application_enumerated_custom_fake_contexts_len);
        encode_application_enumerated_fake.custom_fake =
            encode_application_enumerated_custom_fake;

        memset(test_apdu, tc->apdu_prefill, sizeof(test_apdu));

        /*
         * Call code_under_test
         */
        int result = bacerror_encode_apdu(tc->apdu, tc->invoke_id, tc->service,
            tc->error_class, tc->error_code);

        /*
         * Verify expected behavior of code_under_test:
         *   - call history, args per call
         *   - results
         *   - outputs
         */
        if (tc->expected_call_history != NULL) {
            for (int j = 0; j < fff.call_history_idx; ++j) {
                zassert_equal(
                    fff.call_history[j], tc->expected_call_history[j], NULL);
            }
            zassert_is_null(
                tc->expected_call_history[fff.call_history_idx], NULL);
        } else {
            zassert_equal(fff.call_history_idx, 0, NULL);
        }

        const int encode_application_enumerated_fake_call_count_expected =
            (tc->encode_application_enumerated_custom_fake_contexts == NULL)
            ? 0
            : tc->encode_application_enumerated_custom_fake_contexts_len - 1;

        zassert_equal(encode_application_enumerated_fake.call_count,
            encode_application_enumerated_fake_call_count_expected, NULL);
        for (int j = 0;
             j < encode_application_enumerated_fake_call_count_expected; ++j) {
            zassert_equal(encode_application_enumerated_fake.arg0_history[j],
                tc->encode_application_enumerated_custom_fake_contexts[j]
                    .apdu_expected,
                NULL);
            zassert_equal(encode_application_enumerated_fake.arg1_history[j],
                tc->encode_application_enumerated_custom_fake_contexts[j]
                    .value_expected,
                NULL);
        }

        if (tc->apdu != NULL) {
            zassert_equal(test_apdu[0], PDU_TYPE_ERROR, NULL);
            zassert_equal(test_apdu[1], tc->invoke_id, NULL);
            zassert_equal(test_apdu[2], tc->service, NULL);

            /* Verify remaining APDU bytes were not modified by code under test
             */
            for (int j = 3; j < sizeof(test_apdu); ++j) {
                zassert_equal(test_apdu[j], tc->apdu_prefill, NULL);
            }
        }

        zassert_equal(result, tc->result_expected, NULL);
    }
}

static void test_bacerror_decode_error_class_and_code(void)
{
#if !BACNET_SVC_SERVER
    uint8_t test_apdu[32] = { 0 };

    uint8_t test_invoke_id = 0; /* bacerror_decode_service_request */
    BACNET_CONFIRMED_SERVICE test_service =
        0; /* bacerror_decode_service_request */
    BACNET_ERROR_CLASS test_error_class = 0;
    BACNET_ERROR_CODE test_error_code = 0;

    struct test_case {
        const char *description_oneliner;

        BACNET_ERROR_CLASS error_class_prefill;
        BACNET_ERROR_CODE error_code_prefill;

        bool call_bacerror_decode_service_request;

        uint8_t *apdu;
        unsigned apdu_len;
        uint8_t *invoke_id; /* bacerror_decode_service_request */
        BACNET_CONFIRMED_SERVICE *service; /* bacerror_decode_service_request */
        BACNET_ERROR_CLASS *error_class;
        BACNET_ERROR_CODE *error_code;

        void **expected_call_history;

        /* Last FFF sequence entry is reused for excess calls.
         * Have an extra entry that returns a distinct failure (-E2BIG)
         *
         * Expect one less call than _len, or 0 if sequence ptr is NULL
         *
         * Configure to return -E2BIG if excess calls.
         */
        int decode_tag_number_and_value_custom_fake_contexts_len;
        struct decode_tag_number_and_value_custom_fake_context
            *decode_tag_number_and_value_custom_fake_contexts;

        int decode_enumerated_custom_fake_contexts_len;
        struct decode_enumerated_custom_fake_context
            *decode_enumerated_custom_fake_contexts;

        uint8_t invoke_id_expected; /* bacerror_decode_service_request */
        BACNET_CONFIRMED_SERVICE
        service_expected; /* bacerror_decode_service_request */
        BACNET_ERROR_CLASS error_class_expected;
        BACNET_ERROR_CODE error_code_expected;

        int result_expected; /* zero is error, >0 is len consumed */
    } const test_cases[] = {
        {
            .description_oneliner = "Handle apdu ref of NULL",

            .apdu = NULL,
            .apdu_len = 1,
            .error_class = &test_error_class,
            .error_class_prefill = 1U,
            .error_code = &test_error_code,
            .error_code_prefill = 0xFFFFU,

            .expected_call_history =
                (void *[]) {
                    NULL, /* mark end of array */
                },

            .result_expected = 0, /* zero total length of APDU consumed */
        },
        {
            .description_oneliner = "Handle apdu_len of zero (0)",

            .apdu =
                (uint8_t[]) {
                    0,
                },
            .apdu_len = 0,
            .error_class = &test_error_class,
            .error_class_prefill = 1U,
            .error_code = &test_error_code,
            .error_code_prefill = 0xFFFFU,

            .expected_call_history =
                (void *[]) {
                    NULL, /* mark end of array */
                },

            .result_expected = 0, /* zero total length of APDU consumed */
        },
        {
            .description_oneliner = "Handle invalid first tag",

            .apdu =
                (uint8_t[]) {
                    0,
                },
            .apdu_len = 1,
            .error_class = &test_error_class,
            .error_class_prefill = 1U,
            .error_code = &test_error_code,
            .error_code_prefill = 0xFFFFU,

            .expected_call_history =
                (void *[]) {
                    bacnet_enumerated_application_decode, NULL, /* mark end of array */
                },

            .decode_tag_number_and_value_custom_fake_contexts_len = 2,
            .decode_tag_number_and_value_custom_fake_contexts =
                (struct decode_tag_number_and_value_custom_fake_context[]) {
                    {
                        .apdu_expected = &test_apdu[0],
                        .tag_number =
                            BACNET_APPLICATION_TAG_ENUMERATED - 1, /* err */
                        .result = 1,
                    },
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .result_expected = 0, /* zero total length of APDU consumed */
        },
        {
            .description_oneliner = "Handle invalid second tag",

            .apdu =
                (uint8_t[]) {
                    0,
                },
            .apdu_len = 1,
            .error_class = &test_error_class,
            .error_class_prefill = 1U,
            .error_code = &test_error_code,
            .error_code_prefill = 0xFFFFU,

            .expected_call_history =
                (void *[]) {
                    decode_tag_number_and_value, decode_enumerated,
                    decode_tag_number_and_value, NULL, /* mark end of array */
                },

            .decode_tag_number_and_value_custom_fake_contexts_len = 3,
            .decode_tag_number_and_value_custom_fake_contexts =
                (struct decode_tag_number_and_value_custom_fake_context[]) {
                    {
                        .apdu_expected = &test_apdu[0],

                        .tag_number = BACNET_APPLICATION_TAG_ENUMERATED,
                        .value = 42, /* arbitrary */
                        .result = 1,
                    },
                    {
                        .apdu_expected = &test_apdu[1 + 1],

                        .tag_number =
                            BACNET_APPLICATION_TAG_ENUMERATED + 1, /* err */
                        .value = 24, /* arbitrary */
                        .result = 1,
                    },
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .decode_enumerated_custom_fake_contexts_len = 2,
            .decode_enumerated_custom_fake_contexts =
                (struct decode_enumerated_custom_fake_context[]) {
                    {
                        .apdu_expected = &test_apdu[1],

                        .value = 3, /* arbitrary */
                        .result = 1,
                    },
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .result_expected = 0, /* length of APDU consumed */
        },
        {
            .description_oneliner = "Handle valid inputs",

            .apdu =
                (uint8_t[]) {
                    0,
                },
            .apdu_len = 1,
            .error_class = &test_error_class,
            .error_class_prefill = 1U,
            .error_code = &test_error_code,
            .error_code_prefill = 0xFFFFU,

            .expected_call_history =
                (void *[]) {
                    decode_tag_number_and_value, decode_enumerated,
                    decode_tag_number_and_value, decode_enumerated,
                    NULL, /* mark end of array */
                },

            .decode_tag_number_and_value_custom_fake_contexts_len = 3,
            .decode_tag_number_and_value_custom_fake_contexts =
                (struct decode_tag_number_and_value_custom_fake_context[]) {
                    {
                        .apdu_expected = &test_apdu[0],

                        .tag_number = BACNET_APPLICATION_TAG_ENUMERATED,
                        .value = 42, /* arbitrary */
                        .result = 1,
                    },
                    {
                        .apdu_expected = &test_apdu[1 + 2],

                        .tag_number = BACNET_APPLICATION_TAG_ENUMERATED,
                        .value = 24, /* arbitrary */
                        .result = 3,
                    },
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .decode_enumerated_custom_fake_contexts_len = 3,
            .decode_enumerated_custom_fake_contexts =
                (struct decode_enumerated_custom_fake_context[]) {
                    {
                        .apdu_expected = &test_apdu[1],

                        .value = 3, /* error_class, arbitrary */
                        .result = 2,
                    },
                    {
                        .apdu_expected = &test_apdu[1 + 2 + 3],

                        .value = 7, /* error_code, arbitrary */
                        .result = 4,
                    },
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .error_class_expected = 3,
            .error_code_expected = 7,

            .result_expected = 1 + 2 + 3 + 4, /* length of APDU consumed */
        },
        {
            .description_oneliner = "decode_service_request, apdu ref NULL",

            .call_bacerror_decode_service_request = true,

            .apdu = NULL,
            .apdu_len = 3,
            .invoke_id = &test_invoke_id,
            .service = &test_service,
            .error_class = &test_error_class,
            .error_class_prefill = 1U,
            .error_code = &test_error_code,
            .error_code_prefill = 0xFFFFU,

            .expected_call_history =
                (void *[]) {
                    NULL, /* mark end of array */
                },

            .decode_tag_number_and_value_custom_fake_contexts_len = 1,
            .decode_tag_number_and_value_custom_fake_contexts =
                (struct decode_tag_number_and_value_custom_fake_context[]) {
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .decode_enumerated_custom_fake_contexts_len = 1,
            .decode_enumerated_custom_fake_contexts =
                (struct decode_enumerated_custom_fake_context[]) {
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .invoke_id_expected = 0xFEU, /* apdu[0] */
            .service_expected = 0xFDU, /* apdu[1] */
            .error_class_expected = 3,
            .error_code_expected = 7,

            .result_expected = 0, /* length of APDU consumed */
        },
        {
            .description_oneliner =
                "decode_service_request invalid inputs, apdu_len = 0",

            .call_bacerror_decode_service_request = true,

            .apdu =
                (uint8_t[]) {
                    0xBEU, /* apdu[0]: invoke_id */
                    0xBDU, /* apdu[1]: service */
                    0,
                },
            .apdu_len = 0,
            .invoke_id = &test_invoke_id,
            .service = &test_service,
            .error_class = &test_error_class,
            .error_class_prefill = 1U,
            .error_code = &test_error_code,
            .error_code_prefill = 0xFFFFU,

            .expected_call_history =
                (void *[]) {
                    NULL, /* mark end of array */
                },

            .decode_tag_number_and_value_custom_fake_contexts_len = 1,
            .decode_tag_number_and_value_custom_fake_contexts =
                (struct decode_tag_number_and_value_custom_fake_context[]) {
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .decode_enumerated_custom_fake_contexts_len = 1,
            .decode_enumerated_custom_fake_contexts =
                (struct decode_enumerated_custom_fake_context[]) {
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .invoke_id_expected = 0xFEU, /* apdu[0] */
            .service_expected = 0xFDU, /* apdu[1] */
            .error_class_expected = 3,
            .error_code_expected = 7,

            .result_expected = 0, /* length of APDU consumed */
        },
        {
            .description_oneliner =
                "decode_service_request invalid inputs, apdu_len = 2",

            .call_bacerror_decode_service_request = true,

            .apdu =
                (uint8_t[]) {
                    0xBEU, /* apdu[0]: invoke_id */
                    0xBDU, /* apdu[1]: service */
                    0,
                },
            .apdu_len = 2,
            .invoke_id = &test_invoke_id,
            .service = &test_service,
            .error_class = &test_error_class,
            .error_class_prefill = 1U,
            .error_code = &test_error_code,
            .error_code_prefill = 0xFFFFU,

            .expected_call_history =
                (void *[]) {
                    NULL, /* mark end of array */
                },

            .decode_tag_number_and_value_custom_fake_contexts_len = 1,
            .decode_tag_number_and_value_custom_fake_contexts =
                (struct decode_tag_number_and_value_custom_fake_context[]) {
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .decode_enumerated_custom_fake_contexts_len = 1,
            .decode_enumerated_custom_fake_contexts =
                (struct decode_enumerated_custom_fake_context[]) {
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .invoke_id_expected = 0xFEU, /* apdu[0] */
            .service_expected = 0xFDU, /* apdu[1] */
            .error_class_expected = 3,
            .error_code_expected = 7,

            .result_expected = 0, /* length of APDU consumed */
        },
        {
            .description_oneliner = "decode_service_request valid inputs",

            .call_bacerror_decode_service_request = true,

            .apdu =
                (uint8_t[]) {
                    0xFEU, /* apdu[0]: invoke_id */
                    0xFDU, /* apdu[1]: service */
                    0,
                },
            .apdu_len = 3,
            .invoke_id = &test_invoke_id,
            .service = &test_service,
            .error_class = &test_error_class,
            .error_class_prefill = 1U,
            .error_code = &test_error_code,
            .error_code_prefill = 0xFFFFU,

            .expected_call_history =
                (void *[]) {
                    decode_tag_number_and_value, decode_enumerated,
                    decode_tag_number_and_value, decode_enumerated,
                    NULL, /* mark end of array */
                },

            .decode_tag_number_and_value_custom_fake_contexts_len = 3,
            .decode_tag_number_and_value_custom_fake_contexts =
                (struct decode_tag_number_and_value_custom_fake_context[]) {
                    {
                        .apdu_expected = &test_apdu[2 + 0],

                        .tag_number = BACNET_APPLICATION_TAG_ENUMERATED,
                        .value = 142, /* arbitrary */
                        .result = 1,
                    },
                    {
                        .apdu_expected = &test_apdu[2 + 1 + 2],

                        .tag_number = BACNET_APPLICATION_TAG_ENUMERATED,
                        .value = 124, /* arbitrary */
                        .result = 3,
                    },
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .decode_enumerated_custom_fake_contexts_len = 3,
            .decode_enumerated_custom_fake_contexts =
                (struct decode_enumerated_custom_fake_context[]) {
                    {
                        .apdu_expected = &test_apdu[2 + 1],

                        .value = 3, /* error_class, arbitrary */
                        .result = 2,
                    },
                    {
                        .apdu_expected = &test_apdu[2 + 1 + 2 + 3],

                        .value = 7, /* error_code, arbitrary */
                        .result = 4,
                    },
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .invoke_id_expected = 0xFEU, /* apdu[0] */
            .service_expected = 0xFDU, /* apdu[1] */
            .error_class_expected = 3,
            .error_code_expected = 7,

            .result_expected = 2 + 1 + 2 + 3 + 4, /* length of APDU consumed */
        },
        {
            .description_oneliner =
                "decode_service_request valid inputs, invoke_id is NULL",

            .call_bacerror_decode_service_request = true,

            .apdu =
                (uint8_t[]) {
                    0xFEU, /* apdu[0]: invoke_id */
                    0xFDU, /* apdu[1]: service */
                    0,
                },
            .apdu_len = 3,
            .invoke_id = NULL,
            .service = &test_service,
            .error_class = &test_error_class,
            .error_class_prefill = 1U,
            .error_code = &test_error_code,
            .error_code_prefill = 0xFFFFU,

            .expected_call_history =
                (void *[]) {
                    decode_tag_number_and_value, decode_enumerated,
                    decode_tag_number_and_value, decode_enumerated,
                    NULL, /* mark end of array */
                },

            .decode_tag_number_and_value_custom_fake_contexts_len = 3,
            .decode_tag_number_and_value_custom_fake_contexts =
                (struct decode_tag_number_and_value_custom_fake_context[]) {
                    {
                        .apdu_expected = &test_apdu[2 + 0],

                        .tag_number = BACNET_APPLICATION_TAG_ENUMERATED,
                        .value = 142, /* arbitrary */
                        .result = 1,
                    },
                    {
                        .apdu_expected = &test_apdu[2 + 1 + 2],

                        .tag_number = BACNET_APPLICATION_TAG_ENUMERATED,
                        .value = 124, /* arbitrary */
                        .result = 3,
                    },
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .decode_enumerated_custom_fake_contexts_len = 3,
            .decode_enumerated_custom_fake_contexts =
                (struct decode_enumerated_custom_fake_context[]) {
                    {
                        .apdu_expected = &test_apdu[2 + 1],

                        .value = 3, /* error_class, arbitrary */
                        .result = 2,
                    },
                    {
                        .apdu_expected = &test_apdu[2 + 1 + 2 + 3],

                        .value = 7, /* error_code, arbitrary */
                        .result = 4,
                    },
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .invoke_id_expected = 0xFEU, /* apdu[0] */
            .service_expected = 0xFDU, /* apdu[1] */
            .error_class_expected = 3,
            .error_code_expected = 7,

            .result_expected = 2 + 1 + 2 + 3 + 4, /* length of APDU consumed */
        },
        {
            .description_oneliner =
                "decode_service_request valid inputs, service is NULL",

            .call_bacerror_decode_service_request = true,

            .apdu =
                (uint8_t[]) {
                    0xFCU, /* apdu[0]: invoke_id */
                    0xFBU, /* apdu[1]: service */
                    0,
                },
            .apdu_len = 3,
            .invoke_id = &test_invoke_id,
            .service = NULL,
            .error_class = &test_error_class,
            .error_class_prefill = 1U,
            .error_code = &test_error_code,
            .error_code_prefill = 0xFFFFU,

            .expected_call_history =
                (void *[]) {
                    decode_tag_number_and_value, decode_enumerated,
                    decode_tag_number_and_value, decode_enumerated,
                    NULL, /* mark end of array */
                },

            .decode_tag_number_and_value_custom_fake_contexts_len = 3,
            .decode_tag_number_and_value_custom_fake_contexts =
                (struct decode_tag_number_and_value_custom_fake_context[]) {
                    {
                        .apdu_expected = &test_apdu[2 + 0],

                        .tag_number = BACNET_APPLICATION_TAG_ENUMERATED,
                        .value = 142, /* arbitrary */
                        .result = 1,
                    },
                    {
                        .apdu_expected = &test_apdu[2 + 1 + 2],

                        .tag_number = BACNET_APPLICATION_TAG_ENUMERATED,
                        .value = 124, /* arbitrary */
                        .result = 3,
                    },
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .decode_enumerated_custom_fake_contexts_len = 3,
            .decode_enumerated_custom_fake_contexts =
                (struct decode_enumerated_custom_fake_context[]) {
                    {
                        .apdu_expected = &test_apdu[2 + 1],

                        .value = 3, /* error_class, arbitrary */
                        .result = 2,
                    },
                    {
                        .apdu_expected = &test_apdu[2 + 1 + 2 + 3],

                        .value = 7, /* error_code, arbitrary */
                        .result = 4,
                    },
                    {
                        .result = -E2BIG, /* for excessive calls */
                    },
                },

            .invoke_id_expected = 0xFCU, /* apdu[0] */
            .service_expected = 0xFBU, /* apdu[1] */
            .error_class_expected = 3,
            .error_code_expected = 7,

            .result_expected = 2 + 1 + 2 + 3 + 4, /* length of APDU consumed */
        },
    };

    for (int i = 0; i < ARRAY_SIZE(test_cases); ++i) {
        const struct test_case *const tc = &test_cases[i];

        printk("Checking test_cases[%i]: %s\n", i,
            (tc->description_oneliner != NULL) ? tc->description_oneliner : "");

        /*
         * Set up pre-conditions
         */
        RESET_HISTORY_AND_FAKES();

        /* NOTE: Point to the return type field in the first returns struct.
         *       This custom_fake:
         *     - uses *_fake.return_val_seq and CONTAINER_OF()
         *         to determine the beginning of the array of structures.
         *     - uses *_fake.return_val_seq_id to index into
         *         the array of structures.
         *       This overloading is to allow the return_val_seq to
         *       also contain call-specific output parameters to be
         *       applied by the custom_fake.
         */
        decode_tag_number_and_value_fake.return_val =
            -E2BIG; /* for excessive calls */
        SET_RETURN_SEQ(decode_tag_number_and_value,
            &tc->decode_tag_number_and_value_custom_fake_contexts[0].result,
            tc->decode_tag_number_and_value_custom_fake_contexts_len);
        decode_tag_number_and_value_fake.custom_fake =
            decode_tag_number_and_value_custom_fake;

        decode_enumerated_fake.return_val = -E2BIG; /* for excessive calls */
        SET_RETURN_SEQ(decode_enumerated,
            &tc->decode_enumerated_custom_fake_contexts[0].result,
            tc->decode_enumerated_custom_fake_contexts_len);
        decode_enumerated_fake.custom_fake = decode_enumerated_custom_fake;

        memset(test_apdu, 0, sizeof(test_apdu));
        if (tc->apdu != NULL) {
            memcpy(test_apdu, tc->apdu, MIN(tc->apdu_len, sizeof(test_apdu)));
        }
        test_invoke_id =
            ~tc->invoke_id_expected; /* bacerror_decode_service_request */
        test_service =
            ~tc->service_expected; /* bacerror_decode_service_request */
        test_error_class = tc->error_class_prefill;
        test_error_code = tc->error_code_prefill;

        /*
         * Call code_under_test
         */
        int result = -1;
        if (tc->call_bacerror_decode_service_request) {
            result = bacerror_decode_service_request(
                ((tc->apdu != NULL) ? test_apdu : NULL), tc->apdu_len,
                ((tc->invoke_id) ? &test_invoke_id : NULL),
                ((tc->service) ? &test_service : NULL), tc->error_class,
                tc->error_code);
        } else {
            result = bacerror_decode_error_class_and_code(
                ((tc->apdu != NULL) ? test_apdu : NULL), tc->apdu_len,
                tc->error_class, tc->error_code);
        }

        /*
         * Verify expected behavior of code_under_test:
         *   - call history, args per call
         *   - results
         *   - outputs
         */
        if (tc->expected_call_history != NULL) {
            for (int j = 0; j < fff.call_history_idx; ++j) {
                zassert_equal(
                    fff.call_history[j], tc->expected_call_history[j], NULL);
            }
            zassert_is_null(
                tc->expected_call_history[fff.call_history_idx], NULL);
        } else {
            zassert_equal(fff.call_history_idx, 0, NULL);
        }

        const int decode_tag_number_and_value_fake_call_count_expected =
            (tc->decode_tag_number_and_value_custom_fake_contexts == NULL)
            ? 0
            : tc->decode_tag_number_and_value_custom_fake_contexts_len - 1;

        zassert_equal(decode_tag_number_and_value_fake.call_count,
            decode_tag_number_and_value_fake_call_count_expected, NULL);
        for (int j = 0;
             j < decode_tag_number_and_value_fake_call_count_expected; ++j) {
            zassert_equal(decode_tag_number_and_value_fake.arg0_history[j],
                tc->decode_tag_number_and_value_custom_fake_contexts[j]
                    .apdu_expected,
                NULL);
            zassert_not_null(
                decode_tag_number_and_value_fake.arg1_history[j], NULL);
            zassert_not_null(
                decode_tag_number_and_value_fake.arg2_history[j], NULL);
        }

        const int decode_enumerated_fake_call_count_expected =
            (tc->decode_enumerated_custom_fake_contexts == NULL)
            ? 0
            : tc->decode_enumerated_custom_fake_contexts_len - 1;

        zassert_equal(decode_enumerated_fake.call_count,
            decode_enumerated_fake_call_count_expected, NULL);
        for (int j = 0; j < decode_enumerated_fake_call_count_expected; ++j) {
            zassert_equal(decode_enumerated_fake.arg0_history[j],
                tc->decode_enumerated_custom_fake_contexts[j].apdu_expected,
                NULL);
            zassert_equal(decode_enumerated_fake.arg1_history[j],
                tc->decode_tag_number_and_value_custom_fake_contexts[j].value,
                NULL);
            zassert_not_null(decode_enumerated_fake.arg2_history[j], NULL);
        }

        if ((tc->apdu != NULL) && tc->result_expected > 0) {
            if (tc->call_bacerror_decode_service_request) {
                if (tc->invoke_id != NULL) {
                    zassert_equal(test_invoke_id, tc->invoke_id_expected, NULL);
                }

                if (tc->service != NULL) {
                    zassert_equal(test_service, tc->service_expected, NULL);
                }
            }

            if (tc->error_class != NULL) {
                zassert_equal(test_error_class, tc->error_class_expected, NULL);
            }
            if (tc->error_code != NULL) {
                zassert_equal(test_error_code, tc->error_code_expected, NULL);
            }
        } else {
#if 0 /* NOTE: outputs are only valid if result > 0 */
            zassert_equal(test_error_class, tc->error_class_prefill, NULL);
            zassert_equal(test_error_code, tc->error_code_prefill, NULL);
#endif
        }

        zassert_equal(result, tc->result_expected, NULL);
    }
#else
    ztest_test_skip();
#endif
}

void test_main(void)
{
    ztest_test_suite(bacnet_bacerror,
        ztest_unit_test(test_bacerror_encode_apdu),
        ztest_unit_test(test_bacerror_decode_error_class_and_code));
    ztest_run_test_suite(bacnet_bacerror);
}
