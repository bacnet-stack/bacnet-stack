# CMake for BACnet settings library
#
# @author Steve Karg <skarg@users.sourceforge.net>
# @date May 2024
# @copyright SPDX-License-Identifier: MIT
zephyr_library(basic_device)

zephyr_library_include_directories(include)

zephyr_library_sources(
  bacnet-port.c
  bacnet-port-ipv4.c
  bacnet-port-ipv6.c
  bacnet.c
  device.c
  server.c
  )

  zephyr_library_sources_ifdef(CONFIG_BACNET_BASIC_DEVICE_SHELL
    bacnet-shell.c
  )
