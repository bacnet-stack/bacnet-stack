/**
 * @file
 * @brief mutex abstraction used in BACNet secure connect.
 * @author Kirill Neznamov
 * @date August 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __BSC__MUTEX__INCLUDED__
#define __BSC__MUTEX__INCLUDED__

#define BSC_MUTEX_DEBUG 0

struct BSC_Mutex;
typedef struct BSC_Mutex BSC_MUTEX;

BSC_MUTEX* bsc_mutex_init(void);
void bsc_mutex_deinit(BSC_MUTEX* mutex);
void bsc_mutex_lock(BSC_MUTEX* mutex);
void bsc_mutex_unlock(BSC_MUTEX* mutex);
void bsc_global_mutex_lock(void);
void bsc_global_mutex_unlock(void);

#if BSC_MUTEX_DEBUG == 1
void bsc_global_mutex_lock_dbg(char* file, int line);
void bsc_global_mutex_unlock_dbg(char* file, int line);
#define bsc_global_mutex_lock(...) bsc_global_mutex_lock_dbg(__FILE__, __LINE__)
#define bsc_global_mutex_unlock(...) bsc_global_mutex_unlock_dbg(__FILE__, __LINE__)
#endif

#endif