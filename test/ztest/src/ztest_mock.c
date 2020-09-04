/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified from zephyr_v2.2.0 subsys/testsuite/ztest/src/ztest_mock.c
 * because:
 *   1. This port will never be run in the Zephyr kernel.
 *      This repository is extended to be a Zephyr module for that.
 *   2. This port will not support multiple CPUs or toolchains.
 *
 * Modifications:
 *   a. Code conditionally compiled on the following CPP symbols were deleted
 *      (as they were kernel-specific):
 *        KERNEL
 */

#include <ztest.h>
#include <zephyr/types.h>
#include <string.h>
#include <stdio.h>

struct parameter {
	struct parameter *next;
	const char *fn;
	const char *name;
	uintptr_t value;
};

#include <stdlib.h>
#include <stdarg.h>

static void free_parameter(struct parameter *param)
{
	free(param);
}

static struct parameter *alloc_parameter(void)
{
	struct parameter *param;

	param = calloc(1, sizeof(struct parameter));
	if (!param) {
		PRINT("Failed to allocate mock parameter\n");
		ztest_test_fail();
	}

	return param;
}

void z_init_mock(void)
{

}

void printk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void vprintk(const char *fmt, va_list ap)
{
	vprintf(fmt, ap);
}

static struct parameter *find_and_delete_value(struct parameter *param,
					       const char *fn,
					       const char *name)
{
	struct parameter *value;

	if (!param->next) {
		return NULL;
	}

	if (strcmp(param->next->name, name) || strcmp(param->next->fn, fn)) {
		return find_and_delete_value(param->next, fn, name);
	}

	value = param->next;
	param->next = param->next->next;
	value->next = NULL;

	return value;
}

static void insert_value(struct parameter *param, const char *fn,
			 const char *name, uintptr_t val)
{
	struct parameter *value;

	value = alloc_parameter();
	value->fn = fn;
	value->name = name;
	value->value = val;

	/* Seek to end of linked list to ensure correct discovery order in find_and_delete_value */
	while (param->next) {
		param = param->next;
	}

	/* Append to end of linked list */
	value->next = param->next;
	param->next = value;
}

static struct parameter parameter_list = { NULL, "", "", 0 };
static struct parameter return_value_list = { NULL, "", "", 0 };

void z_ztest_expect_value(const char *fn, const char *name, uintptr_t val)
{
	insert_value(&parameter_list, fn, name, val);
}

void z_ztest_check_expected_value(const char *fn, const char *name,
				 uintptr_t val)
{
	struct parameter *param;
	uintptr_t expected;

	param = find_and_delete_value(&parameter_list, fn, name);
	if (!param) {
		PRINT("Failed to find parameter %s for %s\n", name, fn);
		ztest_test_fail();
	}

	expected = param->value;
	free_parameter(param);

	if (expected != val) {
		/* We need to cast these values since the toolchain doesn't
		 * provide inttypes.h
		 */
		PRINT("%s received wrong value: Got %lu, expected %lu\n",
		      fn, (unsigned long)val, (unsigned long)expected);
		ztest_test_fail();
	}
}

void  z_ztest_returns_value(const char *fn, uintptr_t value)
{
	insert_value(&return_value_list, fn, "", value);
}


uintptr_t z_ztest_get_return_value(const char *fn)
{
	uintptr_t value;
	struct parameter *param = find_and_delete_value(&return_value_list,
							fn, "");

	if (!param) {
		PRINT("Failed to find return value for function %s\n", fn);
		ztest_test_fail();
	}

	value = param->value;
	free_parameter(param);

	return value;
}

static void free_param_list(struct parameter *param)
{
	struct parameter *next;

	while (param) {
		next = param->next;
		free_parameter(param);
		param = next;
	}
}

int z_cleanup_mock(void)
{
	int fail = 0;

	if (parameter_list.next) {
		fail = 1;
	}
	if (return_value_list.next) {
		fail = 2;
	}

	free_param_list(parameter_list.next);
	free_param_list(return_value_list.next);

	parameter_list.next = NULL;
	return_value_list.next = NULL;

	return fail;
}
