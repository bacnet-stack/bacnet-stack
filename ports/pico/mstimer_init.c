/**************************************************************************
 *
 * Copyright (C) 2025 Testimony Adams <adamstestimony@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/

#include "mstimer_init.h"

void systimer_init(void)
{
  //
}

unsigned long mstimer_now(void)
{
  return to_ms_since_boot(get_absolute_time());
}
