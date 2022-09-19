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
#include "bacnet/datalink/bsc/bsc-socket.h"

#ifndef BSC_CONF_MAX_CONTEXTS_NUM
#define BSC_MAX_CONTEXTS_NUM 10
#else
#define BSC_MAX_CONTEXTS_NUM BSC_CONF_MAX_CONTEXTS_NUM
#endif

BSC_SC_RET bsc_runloop_start(void);
BSC_SC_RET bsc_runloop_reg(BSC_SOCKET_CTX *ctx,
						   void (*runloop_func)(BSC_SOCKET_CTX *ctx));
void bsc_runloop_schedule(void);
void bsc_runloop_unreg(BSC_SOCKET_CTX *ctx);
void bsc_runloop_stop(void);

#endif