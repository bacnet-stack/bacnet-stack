/**
 * @file
 * @brief Stub functions for unit test of a BACnet object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date January 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/basic/sys/mstimer.h"

/**
 * @brief Get the current time in milliseconds
 * @return milliseconds
 */
unsigned long mstimer_now(void)
{
    return ztest_get_return_value();
}
