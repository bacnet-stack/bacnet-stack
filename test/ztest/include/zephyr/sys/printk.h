/* printk.h - low-level debug output */

/*
 * Copyright (c) 2010-2012, 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified from zephyr_v2.2.0 include/sys/printk.h
 * because:
 *   1. This port will never be run in the Zephyr kernel.
 *      This repository is extended to be a Zephyr module for that.
 *
 * Modifications:
 *   a. Code conditionally compiled on the following CPP symbols were deleted
 *      (as they were kernel-specific):
 *        CONFIG_PRINTK
 *        KERNEL
 *   b. Inclusion of The following header files were removed as irrelevant.
 *        <toolchain.h>
 *        <inttypes.h>
 */
#ifndef ZEPHYR_INCLUDE_SYS_PRINTK_H_
#define ZEPHYR_INCLUDE_SYS_PRINTK_H_

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @brief Print kernel debugging message.
 *
 * This routine prints a kernel debugging message to the system console.
 * Output is send immediately, without any mutual exclusion or buffering.
 *
 * A basic set of conversion specifier characters are supported:
 *   - signed decimal: \%d, \%i
 *   - unsigned decimal: \%u
 *   - unsigned hexadecimal: \%x (\%X is treated as \%x)
 *   - pointer: \%p
 *   - string: \%s
 *   - character: \%c
 *   - percent: \%\%
 *
 * Field width (with or without leading zeroes) is supported.
 * Length attributes h, hh, l, ll and z are supported. However, integral
 * values with %lld and %lli are only printed if they fit in a long
 * otherwise 'ERR' is printed. Full 64-bit values may be printed with %llx.
 * Flags and precision attributes are not supported.
 *
 * @param fmt Format string.
 * @param ... Optional list of format arguments.
 *
 * @return N/A
 */

extern void printk(const char *fmt, ...);
extern void vprintk(const char *fmt, va_list ap);

//GAS: useful? #warning "z_vprintk is not supported in this port of ztest"
static inline void z_vprintk(int (*out)(int f, void *c), void *ctx,
					 const char *fmt, va_list ap)
{
	(void)(out);
	(void)(ctx);
	(void)(fmt);
	(void)(ap);
}

#ifdef __cplusplus
}
#endif

#endif
