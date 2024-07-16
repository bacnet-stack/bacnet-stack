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
#include <bacnet_settings/bacnet_storage.h>

/**
 * @brief Get or set a string using BACnet storage subsystem
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_key(BACNET_STORAGE_KEY *key, const struct shell *sh, size_t argc,
		   char **argv)
{
	uint16_t object_type;
	uint32_t object_instance;
	uint32_t property_id = 77;
	uint32_t array_index = BACNET_STORAGE_ARRAY_INDEX_NONE;
	long value = 0;

	if (argc < 3) {
		shell_error(
			sh,
			"Usage: %s <object-type> <instance> <property> [value]",
			argv[0]);
		return -EINVAL;
	}
	value = strtoul(argv[1], NULL, 0);
	if ((value < 0) || (value >= UINT16_MAX)) {
		shell_error(sh, "Invalid object-type: %s. Must be 0-65535.",
			    argv[1]);
		return -EINVAL;
	}
	object_type = (uint16_t)value;
	value = strtoul(argv[2], NULL, 0);
	if (value > 4194303) {
		shell_error(sh,
			    "Invalid object-instance: %s. Must be 0-4194303.",
			    argv[2]);
		return -EINVAL;
	}
	object_instance = (uint32_t)value;
	value = strtoul(argv[3], NULL, 0);
	if (value > UINT32_MAX) {
		shell_error(sh, "Invalid property: %s. Must be 0-4294967295.",
			    argv[3]);
		return -EINVAL;
	}
	property_id = (uint32_t)value;
	/* setup the storage key */
	bacnet_storage_key_init(key, object_type, object_instance, property_id,
				array_index);

	return 0;
}

/**
 * @brief Get or set a string using BACnet storage subsystem
 * @param sh Shell
 * @param argc Number of arguments
 * @param argv Argument list
 * @return 0 on success, negative on failure
 */
static int cmd_string(const struct shell *sh, size_t argc, char **argv)
{
	char key_name[BACNET_STORAGE_KEY_SIZE_MAX + 1] = { 0 };
	uint8_t data[BACNET_STORAGE_VALUE_SIZE_MAX + 1] = { 0 };
	BACNET_STORAGE_KEY key = { 0 };
	size_t arg_len = 0;
	int rc;

	rc = cmd_key(&key, sh, argc, argv);
	if (rc < 0) {
		return rc;
	}
	/* convert the key to a string for the shell */
	(void)bacnet_storage_key_encode(key_name, sizeof(key_name), &key);
	if (argc > 4) {
		arg_len = strlen(argv[4]);
		rc = bacnet_storage_set(&key, argv[4], arg_len);
		if (rc == 0) {
			shell_print(sh, "Set %s = %s", key_name, argv[4]);
		} else {
			shell_error(sh, "Unable to set %s = %s", key_name,
				    argv[4]);
			return -EINVAL;
		}
	} else {
		rc = bacnet_storage_get(&key, data, sizeof(data));
		if (rc < 0) {
			shell_error(sh, "Unable to get %s", key_name);
			return -EINVAL;
		}
		shell_print(sh, "Get %s = %s", key_name, data);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_bacnet_settings_cmds,
			       SHELL_CMD(string, NULL,
					 "get or set BACnet storage string",
					 cmd_string),
			       SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((bacnet), settings, &sub_bacnet_settings_cmds,
		 "BACnet settings commands", NULL, 1, 0);
