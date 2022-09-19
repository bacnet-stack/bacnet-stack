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

struct BSC_Mutex;
typedef struct BSC_Mutex BSC_MUTEX;

BSC_MUTEX* bsc_mutex_init(void);
void bsc_mutex_deinit(BSC_MUTEX* mutex);
void bsc_mutex_lock(BSC_MUTEX* mutex);
void bsc_mutex_unlock(BSC_MUTEX* mutex);

void bsc_global_mutex_lock(void);
void bsc_global_mutex_unlock(void);
#endif