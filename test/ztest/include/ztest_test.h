/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Zephyr testing framework _test.
 *
 * Modified from zephyr_v2.2.0 subsys/testsuite/ztest/src/ztest.c
 * because:
 *   1. This port will never be run in the Zephyr kernel.
 *      This repository is extended to be a Zephyr module for that.
 *   2. This port will not support multiple CPUs or toolchains.
 *
 * Modifications:
 *   a. Code conditionally compiled on the following CPP symbols were deleted
 *      (as they were kernel-specific):
 *        CONFIG_USERSPACE
 *        CONFIG_SMP
 *        KERNEL
 *   b. Inclusion of The following header files were removed as irrelevant.
 *        <app_memory/app_memdomain.h>
 *   c. syscall declarations and inclusions.
 */

#ifndef __ZTEST_TEST_H__
#define __ZTEST_TEST_H__


#ifdef __cplusplus
extern "C" {
#endif

struct unit_test {
	const char *name;
	void (*test)(void);
	void (*setup)(void);
	void (*teardown)(void);
	u32_t thread_options;
};

void z_ztest_run_test_suite(const char *name, struct unit_test *suite);

/**
 * @defgroup ztest_test Ztest testing macros
 * @ingroup ztest
 *
 * This module eases the testing process by providing helpful macros and other
 * testing structures.
 *
 * @{
 */

/**
 * @brief Fail the currently running test.
 *
 * This is the function called from failed assertions and the like. You
 * probably don't need to call it yourself.
 */
void ztest_test_fail(void);

/**
 * @brief Pass the currently running test.
 *
 * Normally a test passes just by returning without an assertion failure.
 * However, if the success case for your test involves a fatal fault,
 * you can call this function from k_sys_fatal_error_handler to indicate that
 * the test passed before aborting the thread.
 */
void ztest_test_pass(void);

/**
 * @brief Skip the current test.
 *
 */
void ztest_test_skip(void);

/**
 * @brief Do nothing, successfully.
 *
 * Unit test / setup function / teardown function that does
 * nothing, successfully. Can be used as a parameter to
 * ztest_unit_test_setup_teardown().
 */
static inline void unit_test_noop(void)
{
}

/**
 * @brief Define a test with setup and teardown functions
 *
 * This should be called as an argument to ztest_test_suite. The test will
 * be run in the following order: @a setup, @a fn, @a teardown.
 *
 * @param fn Main test function
 * @param setup Setup function
 * @param teardown Teardown function
 */

#define ztest_unit_test_setup_teardown(fn, setup, teardown) { \
		STRINGIFY(fn), fn, setup, teardown, 0 \
}

/**
 * @brief Define a user mode test with setup and teardown functions
 *
 * This should be called as an argument to ztest_test_suite. The test will
 * be run in the following order: @a setup, @a fn, @a teardown. ALL
 * test functions will be run in user mode, and only if CONFIG_USERSPACE
 * is enabled, otherwise this is the same as ztest_unit_test_setup_teardown().
 *
 * @param fn Main test function
 * @param setup Setup function
 * @param teardown Teardown function
 */

#define ztest_user_unit_test_setup_teardown(fn, setup, teardown) { \
		STRINGIFY(fn), fn, setup, teardown, K_USER \
}

/**
 * @brief Define a test function
 *
 * This should be called as an argument to ztest_test_suite.
 *
 * @param fn Test function
 */

#define ztest_unit_test(fn) \
	ztest_unit_test_setup_teardown(fn, unit_test_noop, unit_test_noop)

/**
 * @brief Define a test function that should run as a user thread
 *
 * This should be called as an argument to ztest_test_suite.
 * If CONFIG_USERSPACE is not enabled, this is functionally identical to
 * ztest_unit_test().
 *
 * @param fn Test function
 */

#define ztest_user_unit_test(fn) \
	ztest_user_unit_test_setup_teardown(fn, unit_test_noop, unit_test_noop)

/**
 * @brief Define a SMP-unsafe test function
 *
 * As ztest_unit_test(), but ensures all test code runs on only
 * one CPU when in SMP.
 *
 * @param fn Test function
 */
#define ztest_1cpu_unit_test(fn) ztest_unit_test(fn)

/**
 * @brief Define a SMP-unsafe test function that should run as a user thread
 *
 * As ztest_user_unit_test(), but ensures all test code runs on only
 * one CPU when in SMP.
 *
 * @param fn Test function
 */
#define ztest_1cpu_user_unit_test(fn) ztest_user_unit_test(fn)

/* definitions for use with testing application shared memory   */
#define ZTEST_DMEM
#define ZTEST_BMEM
#define ZTEST_SECTION	.data

/**
 * @brief Define a test suite
 *
 * This function should be called in the following fashion:
 * ```{.c}
 *      ztest_test_suite(test_suite_name,
 *              ztest_unit_test(test_function),
 *              ztest_unit_test(test_other_function)
 *      );
 *
 *      ztest_run_test_suite(test_suite_name);
 * ```
 *
 * @param suite Name of the testing suite
 */
#define ztest_test_suite(suite, ...) \
	static ZTEST_DMEM struct unit_test _##suite[] = { \
		__VA_ARGS__, { 0 } \
	}
/**
 * @brief Run the specified test suite.
 *
 * @param suite Test suite to run.
 */
#define ztest_run_test_suite(suite) \
	z_ztest_run_test_suite(#suite, _##suite)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ZTEST_ASSERT_H__ */
