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

#define BSC_RUNLOOP_CALLBACKS_NUM (10+BSC_CONF_HUB_CONNECTORS_NUM + 2*BSC_CONF_NODE_SWITCHES_NUM)
#define BSC_RUNLOOP_LOCAL_NUM BSC_CONF_NODES_NUM

typedef struct BSC_RunLoop BSC_RUNLOOP;
BSC_RUNLOOP* bsc_local_runloop_alloc(void);
void bsc_local_runloop_free(BSC_RUNLOOP* runloop);

BSC_SC_RET bsc_runloop_start(BSC_RUNLOOP* runloop);
BSC_SC_RET bsc_runloop_reg(BSC_RUNLOOP* runloop,
						   void* ctx,
						   void (*runloop_callback_func)(void* ctx));
void bsc_runloop_schedule(BSC_RUNLOOP* runloop);
void bsc_runloop_unreg(BSC_RUNLOOP* runloop, void* ctx);
void bsc_runloop_stop(BSC_RUNLOOP* runloop);
BSC_RUNLOOP* bsc_global_runloop(void);
#endif
