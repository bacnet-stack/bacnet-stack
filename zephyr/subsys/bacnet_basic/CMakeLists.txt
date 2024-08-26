# CMake for BACnet settings library
#
# @author Steve Karg <skarg@users.sourceforge.net>
# @date May 2024
# @copyright SPDX-License-Identifier: MIT
zephyr_library(bacnet_basic)

zephyr_library_include_directories(include)

zephyr_library_sources(
  bacnet_port.c
  bacnet_port_ipv4.c
  bacnet_port_ipv6.c
  bacnet_basic.c
  device.c
  server.c
  )

  zephyr_library_sources_ifdef(CONFIG_BACNET_BASIC_DEVICE_SHELL
    bacnet_shell_objects.c
    bacnet_shell_packets.c
    bacnet_shell_uptime.c
  )
