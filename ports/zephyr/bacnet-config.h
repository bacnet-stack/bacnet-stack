/**
 * @file
 * @brief Port specific configuration for BACnet Stack for Zephyr OS
 * @author Steve Karg
 * @date 2024
 * @section LICENSE
 *
 * Copyright (C) 2024 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef BACNET_PORTS_ZEPHYR_BACNET_CONFIG_H
#define BACNET_PORTS_ZEPHYR_BACNET_CONFIG_H

#if ! defined BACNET_CONFIG_H || ! BACNET_CONFIG_H
#error bacnet-config.h included outside of BACNET_CONFIG_H control
#endif

/* provides platform specific define for ARRAY_SIZE */
#include <zephyr/sys/util.h>

#endif
