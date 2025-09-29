/**
 * @file
 * @brief mock for COV functions
 * @author Steve Karg
 * @date September 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdef.h>
#include <bacnet/basic/services.h>

int handler_cov_encode_subscriptions(uint8_t *apdu, int max_apdu)
{
    (void)apdu;
    (void)max_apdu;
    return 0;
}
