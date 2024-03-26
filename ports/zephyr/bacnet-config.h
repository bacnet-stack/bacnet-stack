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

/* some smaller defaults for microcontrollers */
#if !defined(MAX_TSM_TRANSACTIONS)
#define MAX_TSM_TRANSACTIONS 1
#endif
#if !defined(MAX_ADDRESS_CACHE)
#define MAX_ADDRESS_CACHE 1
#endif
#if !defined(MAX_CHARACTER_STRING_BYTES)
#define MAX_CHARACTER_STRING_BYTES 64
#endif
#if !defined(MAX_OCTET_STRING_BYTES)
#define MAX_OCTET_STRING_BYTES 64
#endif

/* K.6.6 BIBB - Network Management-BBMD Configuration-B (NM-BBMDC-B)*/
#if !defined(MAX_BBMD_ENTRIES)
#define MAX_BBMD_ENTRIES 5
#endif
#if !defined(MAX_FD_ENTRIES)
#define MAX_FD_ENTRIES 5
#endif

#endif
