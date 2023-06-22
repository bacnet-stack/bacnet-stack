/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>

#include "fakes/func.h"


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

#define RESET_HISTORY_AND_FAKES()                                \
    BACNET_STACK_TEST_FFF_FAKES_LIST(RESET_FAKE) \
    FFF_RESET_HISTORY()

DEFINE_FFF_GLOBALS;

/*
 * Tests:
 */

static void testing_function(uint32_t *value)
{
    int k;

    function1();
    k = function2(4);
    zassert_equal(21, k, NULL);
    *value = 20;
    k = function3(10, value);
    zassert_equal(11, k, NULL);

    function1();
    k = function2(5);
    zassert_equal(22, k, NULL);
    *value = 20;
    k = function3(15, value);
    zassert_equal(12, k, NULL);

    function1();
}

static void test_ttt_sample(void)
{
    uint32_t value;

    int function2_ret_val[2] = { 21, 22};
    SET_RETURN_SEQ(function2, function2_ret_val, 2);

    int function3_ret_val[2] = { 11, 12};
    SET_RETURN_SEQ(function3, function3_ret_val, 2);
 
    testing_function(&value);


    zassert_equal(function1_fake.call_count, 3, NULL);
    zassert_equal(function2_fake.call_count, 2, NULL);
    zassert_equal(function3_fake.call_count, 2, NULL);

    zassert_equal(4, function2_fake.arg0_history[0], NULL);
    zassert_equal(5, function2_fake.arg0_history[1], NULL);

    zassert_equal(10, function3_fake.arg0_history[0], NULL);
    zassert_equal(&value, function3_fake.arg1_history[0], NULL);
    zassert_equal(15, function3_fake.arg0_history[1], NULL);
    zassert_equal(&value, function3_fake.arg1_history[1], NULL);

    zassert_equal(function2_ret_val[0], function2_fake.return_val_history[0], NULL);
    zassert_equal(function2_ret_val[1], function2_fake.return_val_history[1], NULL);
    zassert_equal(function3_ret_val[0], function3_fake.return_val_history[0], NULL);
    zassert_equal(function3_ret_val[1], function3_fake.return_val_history[1], NULL);
}

void test_main(void)
{
    ztest_test_suite(test_ttt,
        ztest_unit_test(test_ttt_sample));
    ztest_run_test_suite(test_ttt);
}
