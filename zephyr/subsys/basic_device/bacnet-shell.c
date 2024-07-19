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
/* BACnet objects API */
#include "bacnet/basic/object/device.h"
/* storage */
#include "bacnet_settings/bacnet_settings.h"
#include "basic_device/bacnet.h"

/**
 * @brief List all BACnet objects in this device
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_objects(const struct shell *sh, size_t argc, char **argv)
{
    int count;
    BACNET_OBJECT_TYPE object_type;
    uint32_t instance;
    uint32_t array_index;
    bool found;

    (void)argc;
    (void)argv;
    shell_print(sh, "List of BACnet Objects: [{");
    count = Device_Object_List_Count();
    for (array_index = 0; array_index < count; array_index++) {
        found =
            Device_Object_List_Identifier(array_index, &object_type, &instance);
        if (found) {
            shell_print(sh, "  \"%s-%u\"%c",
                        bactext_object_type_name(object_type), instance,
                        (array_index == count - 1) ? ' ' : ',');
        }
    }
    shell_print(sh, "}] -- %d objects found", count);

    return 0;
}

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
                bacnet_uptime_seconds());

    return 0;
}

/**
 * @brief Print BACnet packet statistics
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_packets(const struct shell *sh, size_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    shell_print(sh, "BACnet thread packets received: %ld",
                bacnet_packet_count());

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    subcmd_bacnet,
    SHELL_CMD(objects, NULL, "list of BACnet objects", cmd_objects),
    SHELL_CMD(uptime, NULL, "BACnet task uptime", cmd_uptime),
    SHELL_CMD(packets, NULL, "BACnet task packet stats", cmd_packets),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(bacnet, &subcmd_bacnet, "BACnet commands", NULL);
