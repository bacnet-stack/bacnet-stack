# Copyright (c) 2020 Legrand North America, LLC.
# SPDX-License-Identifier: MIT
zephyr_sources(bacnet_shell.c)

add_subdirectory_ifdef(CONFIG_BACNETSTACK_BACNET_SETTINGS bacnet_settings)
add_subdirectory_ifdef(CONFIG_BACNETSTACK_BASIC_DEVICE basic_device)
add_subdirectory_ifdef(CONFIG_BACNETSTACK_BACNET_SERVER server)
add_subdirectory_ifdef(CONFIG_BACNETSTACK_BACNET_OBJECT object)
