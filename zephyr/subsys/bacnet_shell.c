/**
 * @file
 * @brief The BACnet shell commands for debugging and testing
 * @author Greg Shue <greg.shue@outlook.com>
 * @date May 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/shell/shell.h>

SHELL_SUBCMD_SET_CREATE(sub_bacnet_cmds, (bacnet));

SHELL_CMD_REGISTER(bacnet, &sub_bacnet_cmds, "BACnet module", NULL);
