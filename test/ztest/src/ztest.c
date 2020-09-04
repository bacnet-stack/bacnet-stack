/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified from zephyr_v2.2.0 subsys/testsuite/ztest/src/ztest.c
 * because:
 *   1. This port will never be run in the Zephyr kernel.
 *      This repository is extended to be a Zephyr module for that.
 *   2. This port will not support multiple CPUs or toolchains.
 *
 * Modifications:
 *   a. Deleted code conditionally compiled on the following CPP symbols:
 *      (as they were kernel-specific):
 *        CONFIG_USERSPACE
 *        KERNEL
 *   b. Removed irrelevant inclusion of the following header files:
 *        <app_memory/app_memdomain.h>
 *        <power/reboot.h>
 *   c. Addition of test_skip functionality missing from non-kernel paths.
 */

#include <ztest.h>
#include <stdio.h>

/* ZTEST_DMEM and ZTEST_BMEM are used for the application shared memory test  */

ZTEST_DMEM enum {
	TEST_PHASE_SETUP,
	TEST_PHASE_TEST,
	TEST_PHASE_TEARDOWN,
	TEST_PHASE_FRAMEWORK
} phase = TEST_PHASE_FRAMEWORK;

static ZTEST_BMEM int test_status;

static int cleanup_test(struct unit_test *test)
{
	int ret = TC_PASS;
	int mock_status;

	mock_status = z_cleanup_mock();

	if (!ret && mock_status == 1) {
		PRINT("Test %s failed: Unused mock parameter values\n",
		      test->name);
		ret = TC_FAIL;
	} else if (!ret && mock_status == 2) {
		PRINT("Test %s failed: Unused mock return values\n",
		      test->name);
		ret = TC_FAIL;
	}

	return ret;
}

static void run_test_functions(struct unit_test *test)
{
	phase = TEST_PHASE_SETUP;
	test->setup();
	phase = TEST_PHASE_TEST;
	test->test();
}

#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#define FAIL_FAST 0

static jmp_buf test_fail;
static jmp_buf test_pass;
static jmp_buf test_skip;
static jmp_buf stack_fail;

void ztest_test_fail(void)
{
	raise(SIGABRT);
}

void ztest_test_pass(void)
{
	longjmp(test_pass, 1);
}

void ztest_test_skip(void)
{
	longjmp(test_skip, 1);
}

static void handle_signal(int sig)
{
	static const char *const phase_str[] = {
		"setup",
		"unit test",
		"teardown",
	};

	PRINT("    %s", strsignal(sig));
	switch (phase) {
	case TEST_PHASE_SETUP:
	case TEST_PHASE_TEST:
	case TEST_PHASE_TEARDOWN:
		PRINT(" at %s function\n", phase_str[phase]);
		longjmp(test_fail, 1);
	case TEST_PHASE_FRAMEWORK:
		PRINT("\n");
		longjmp(stack_fail, 1);
	}
}

static void init_testing(void)
{
	signal(SIGABRT, handle_signal);
	signal(SIGSEGV, handle_signal);

	if (setjmp(stack_fail)) {
		PRINT("Test suite crashed.");
		exit(1);
	}
}

static int run_test(struct unit_test *test)
{
	int ret = TC_PASS;

	TC_START(test->name);

	if (setjmp(test_fail)) {
		ret = TC_FAIL;
		goto out;
	}

	if (setjmp(test_pass)) {
		ret = TC_PASS;
		goto out;
	}

	if (setjmp(test_skip)) {
		ret = TC_SKIP;
		goto out;
	}

	run_test_functions(test);
out:
	ret |= cleanup_test(test);
	Z_TC_END_RESULT(ret, test->name);

	return ret;
}

void z_ztest_run_test_suite(const char *name, struct unit_test *suite)
{
	int fail = 0;

	if (test_status < 0) {
		return;
	}

	init_testing();

	PRINT("Running test suite %s\n", name);
	PRINT_LINE;
	while (suite->test) {
		fail += run_test(suite);
		suite++;

		if (fail && FAIL_FAST) {
			break;
		}
	}
	if (fail) {
		TC_PRINT("Test suite %s failed.\n", name);
	} else {
		TC_PRINT("Test suite %s succeeded\n", name);
	}

	test_status = (test_status || fail) ? 1 : 0;
}

void end_report(void)
{
	if (test_status) {
		TC_END_REPORT(TC_FAIL);
	} else {
		TC_END_REPORT(TC_PASS);
	}
}

int main(void)
{
	z_init_mock();
	test_main();
	end_report();

	return test_status;
}
