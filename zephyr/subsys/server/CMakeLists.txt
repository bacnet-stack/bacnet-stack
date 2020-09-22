# Copyright (c) 2020 Legrand North America, LLC.
# SPDX-License-Identifier: MIT

zephyr_include_directories(include)

zephyr_interface_library_named(bac_server)

zephyr_library()
zephyr_library_sources(server.c)

zephyr_library_link_libraries(bac_server)
