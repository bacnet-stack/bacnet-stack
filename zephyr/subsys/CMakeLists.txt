# Copyright (c) 2020 Legrand North America, LLC.
# SPDX-License-Identifier: MIT

add_subdirectory_ifdef(CONFIG_BACNETSTACK_BACNET_SERVER server)
add_subdirectory(object)
