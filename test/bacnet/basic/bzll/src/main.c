/**
 * @file
 * @brief Test file for a basic BACnet Zigbee Link Layer (BZLL)
 * @author Steve Karg
 * @date July 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h> /* for standard i/o, like printing */
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <string.h> /* for memcpy */
#include <assert.h>
#include <string.h>
/* support file */
#include "support.h"
/* Test files */
#include "test_bzllvmac.h"
#include "test_h_bzll.h"

/**
 * Main function to execute the test
 *
 * @return 0 on success, non-zero on failure
 */
int main(void)
{
    test_bzllvmac();
    test_h_bzll();

    SUPPORT_PRINT("Success\n");
    return 0;
}
