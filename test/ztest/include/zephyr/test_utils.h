/*  test_utils.h - TinyCrypt interface to common functions for tests */

/*
 *  Copyright (C) 2015 by Intel Corporation, All Rights Reserved.
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <zephyr/tc_util.h>
#include <tinycrypt/constants.h>

static inline void show_str(const char *label, const uint8_t *s, size_t len)
{
	uint32_t i;

	TC_PRINT("%s = ", label);
	for (i = 0U; i < (uint32_t)len; ++i) {
		TC_PRINT("%02x", s[i]);
	}
	TC_PRINT("\n");
}

static inline
void fatal(uint32_t testnum, const void *expected, size_t expectedlen,
	   const void *computed, size_t computedlen)
{
	TC_ERROR("\tTest #%d Failed!\n", testnum);
	show_str("\t\tExpected", expected, expectedlen);
	show_str("\t\tComputed  ", computed, computedlen);
	TC_PRINT("\n");
}

static inline
uint32_t check_result(uint32_t testnum, const void *expected,
		      size_t expectedlen, const void *computed,
		      size_t computedlen, uint32_t verbose)
{
	uint32_t result = TC_PASS;

	ARG_UNUSED(verbose);

        if (expectedlen != computedlen) {
		TC_ERROR("The length of the computed buffer (%zu)",
			 computedlen);
		TC_ERROR("does not match the expected length (%zu).",
			 expectedlen);
                result = TC_FAIL;
	} else {
		if (memcmp(computed, expected, computedlen) != 0) {
			fatal(testnum, expected, expectedlen,
			      computed, computedlen);
			result = TC_FAIL;
		}
	}

        return result;
}

#endif
