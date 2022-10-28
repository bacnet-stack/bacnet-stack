/**
 * @file
 * @brief BACNet socket runloop function implementation.
 * @author Kirill Neznamov
 * @date September 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BSC__RUNLOOP__INCLUDED__
#define __BSC__RUNLOOP__INCLUDED__

#include "bacnet/datalink/bsc/bsc-retcodes.h"
#include "bacnet/datalink/bsc/bsc-conf.h"

#define BSC_MAX_CALLBACKS_NUM (10+BSC_CONF_HUB_CONNECTORS_NUM + 2*BSC_CONF_NODE_SWITCHES_NUM)

typedef struct BSC_RunLoop BSC_RUNLOOP;
BSC_RUNLOOP* bsc_runloop_init(void);
void bsc_runloop_deinit(BSC_RUNLOOP* runloop);

BSC_SC_RET bsc_runloop_start(void);
BSC_SC_RET bsc_runloop_reg(void* ctx,
						   void (*runloop_func)(void* ctx));
void bsc_runloop_schedule(void);
void bsc_runloop_unreg(void* ctx);
void bsc_runloop_stop(void);

#endif