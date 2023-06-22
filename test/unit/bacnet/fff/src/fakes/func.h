/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

#include <zephyr/fff.h>

#define BACNET_STACK_TEST_FFF_FAKES_LIST(FAKE) \
    FAKE(function1)                            \
    FAKE(function2)                            \
    FAKE(function3)

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VOID_FUNC(function1);
DECLARE_FAKE_VALUE_FUNC(int, function2, uint8_t);
DECLARE_FAKE_VALUE_FUNC(int, function3, uint8_t, uint32_t*);

#ifdef __cplusplus
}
#endif
