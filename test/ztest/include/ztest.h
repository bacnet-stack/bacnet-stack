/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified from zephyr_v2.2.0 subsys/testsuite/ztest/include/ztest.h
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

/**
 * @file
 *
 * @brief Zephyr testing suite
 */

/**
 * @brief Zephyr Tests
 * @defgroup all_tests Zephyr Tests
 * @{
 * @}
 */

#ifndef __ZTEST_H__
#define __ZTEST_H__

/**
 * @defgroup ztest Zephyr testing suite
 */

#if !defined(CONFIG_ZTEST) && !defined(ZTEST_UNITTEST)
#error "You need to add CONFIG_ZTEST to your config file."
#endif

#define CONFIG_STDOUT_CONSOLE 1
#define CONFIG_ZTEST_ASSERT_VERBOSE 1
#define CONFIG_ZTEST_MOCKING
#define CONFIG_NUM_COOP_PRIORITIES 16
#define CONFIG_COOP_ENABLED 1
#define CONFIG_PREEMPT_ENABLED 1
#define CONFIG_MP_NUM_CPUS 1
#define CONFIG_SYS_CLOCK_TICKS_PER_SEC 100
#define CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC 10000000
/* FIXME: Properly integrate with Zephyr's arch specific code */
#define CONFIG_X86 1
#define CONFIG_PRINTK 1
#ifdef __cplusplus
extern "C" {
#endif
struct esf;
typedef struct esf z_arch_esf_t;
#ifdef __cplusplus
}
#endif

#include <sys/printk.h>
#define PRINT printk

#include <zephyr.h>

#include <ztest_assert.h>
#include <ztest_mock.h>
#include <ztest_test.h>
#include <tc_util.h>

#ifdef __cplusplus
extern "C" {
#endif

void test_main(void);

#ifdef __cplusplus
}
#endif

#endif /* __ZTEST_H__ */
