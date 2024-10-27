/**
 * @file
 * @brief Global websocket functions.
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

#include <windows.h>

void bsc_mutex_lock(volatile HANDLE *m);
void bsc_mutex_unlock(volatile HANDLE *m);
void bsc_websocket_global_lock(void);
void bsc_websocket_global_unlock(void);
void bsc_websocket_init_log(void);
#endif
