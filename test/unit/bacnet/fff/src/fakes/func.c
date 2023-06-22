/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include "func.h"

DEFINE_FAKE_VOID_FUNC(function1);
DEFINE_FAKE_VALUE_FUNC(int, function2, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, function3, uint8_t, uint32_t*);
