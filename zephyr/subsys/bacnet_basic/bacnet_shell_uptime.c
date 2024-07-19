/**
 * @file
 * @brief The BACnet shell commands for debugging and testing
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <zephyr/shell/shell.h>
/* BACnet definitions */
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bactext.h"
#include "bacnet/bacapp.h"
/* Basic BACnet */
#include "bacnet_basic/bacnet_basic.h"

/**
 * @brief Print BACnet uptime statistics
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_uptime(const struct shell *sh, size_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    shell_print(sh, "BACnet thread uptimeseconds: %ld",
                bacnet_basic_uptime_seconds());

    return 0;
}

SHELL_SUBCMD_ADD((bacnet), uptime, NULL,  "BACnet task uptime", cmd_uptime,
		 1, 0);
