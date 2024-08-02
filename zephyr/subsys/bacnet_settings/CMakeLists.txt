# CMake for BACnet settings library
#
# @author Steve Karg <skarg@users.sourceforge.net>
# @date May 2024
# @copyright SPDX-License-Identifier: MIT
zephyr_library(bacnet_settings)

zephyr_library_include_directories(include)

zephyr_library_sources(
  bacnet_storage.c
  bacnet_settings.c
  )

zephyr_library_sources_ifdef(CONFIG_BACNET_SETTINGS_SHELL
  bacnet_shell.c
  )
