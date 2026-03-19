/**************************************************************************
 *
 * Copyright (C) 2025 Testimony Adams <adamstestimony@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/

#ifndef DLENV_H
#define DLENV_H
#include <stdint.h>
#include <stdbool.h>

#include "bacnet/bacdef.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstp.h"
#include "bacnet/basic/sys/mstimer.h"

#include "rs485.h"

#define BACNET_MSTP_MAX_MASTER      5
#define BACNET_MSTP_MAX_INFO_FRAMES 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

bool pico_dlenv_init(uint8_t mac_address);
void dlenv_maintenance_timer(uint16_t seconds);
void dlenv_cleanup(void);

// TO BE IMPROVED
extern volatile bool g_mstp_have_token;
extern volatile uint8_t g_last_frame_type;
extern volatile uint8_t g_last_src;
extern volatile uint8_t g_last_dst;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif