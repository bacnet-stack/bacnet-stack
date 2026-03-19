/**************************************************************************
 *
 * Copyright (C) 2025 Testimony Adams <adamstestimony@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/

#ifndef SYSTIMER_H
#define SYSTIMER_H

#include <stdint.h>
#include "pico/time.h"
#include "bacnet/basic/sys/mstimer.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
void systimer_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
