# Copyright (c) 2024 Legrand North America, LLC.
# SPDX-License-Identifier: MIT
# Kconfig - Subsystem configuration options
zephyr_library(bacnet_settings)

zephyr_library_include_directories(include)

zephyr_library_sources(
  bacnet_storage.c
  bacnet_settings.c
  )

zephyr_library_sources_ifdef(CONFIG_BACNET_SETTINGS_SHELL
  bacnet_shell.c
  )
