/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_H_
#define ZEPHYR_INCLUDE_ZEPHYR_H_

/*
 * Applications can identify whether they are built for Zephyr by
 * macro below. (It may be already defined by a makefile or toolchain.)
 */
#ifndef __ZEPHYR__
#define __ZEPHYR__
#endif

//GAS: rm? #include <kernel.h>

//GAS: add?
#define Z_STRINGIFY(x) #x
#define STRINGIFY(s) Z_STRINGIFY(s)

#endif /* ZEPHYR_INCLUDE_ZEPHYR_H_ */
