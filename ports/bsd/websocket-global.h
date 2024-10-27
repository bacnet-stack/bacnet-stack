/**
 * @file
 * @brief Global websocket functions
 * @author Kirill Neznamov
 * @date May 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __BSC_WEBSOCKET_MUTEX_INCLUDED__
#define __BSC_WEBSOCKET_MUTEX_INCLUDED__

#ifndef BSC_DEBUG_WEBSOCKET_MUTEX_ENABLED
#define BSC_DEBUG_WEBSOCKET_MUTEX_ENABLED 0
#endif

#if (BSC_DEBUG_WEBSOCKET_MUTEX_ENABLED != 1)
void bsc_websocket_global_lock(void);
void bsc_websocket_global_unlock(void);
#else
void bsc_websocket_global_lock_dbg(char *f, int line);
void bsc_websocket_global_unlock_dbg(char *f, int line);
#define bsc_websocket_global_lock() bsc_websocket_global_lock_dbg(__FILE__, __LINE__);
#define bsc_websocket_global_unlock() bsc_websocket_global_unlock_dbg(__FILE__, __LINE__);
#endif

void bsc_websocket_init_log(void);

#endif
