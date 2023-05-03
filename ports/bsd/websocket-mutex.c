/**
 * @file
 * @brief Implementation of global websocket mutex lock/unlock functions.
 * @author Kirill Neznamov
 * @date May 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "websocket-global.h"

static pthread_mutex_t websocket_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
static pthread_mutex_t websocket_dispatch_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;

#if (BSC_DEBUG_WEBSOCKET_MUTEX_ENABLED != 1)

void bsc_websocket_global_lock(void)
{
  pthread_mutex_lock(&websocket_mutex);
}

void bsc_websocket_global_unlock(void)
{
  pthread_mutex_unlock(&websocket_mutex);
}

void bws_dispatch_lock(void)
{
  pthread_mutex_lock(&websocket_dispatch_mutex);
}

void bws_dispatch_unlock(void)
{
  pthread_mutex_unlock(&websocket_dispatch_mutex);
}

#else
static int volatile websocket_mutex_cnt = 0;
static int volatile websocket_dispatch_mutex_cnt = 0;

void bsc_websocket_global_lock_dbg(char *f, int line)
{
  printf("bsc_websocket_global_lock_dbg() >>> %s:%d lock_cnt %d\n", f, line, websocket_mutex_cnt);
  websocket_mutex_cnt++;
  fflush(stdout);
  pthread_mutex_lock(&websocket_mutex);
  printf("bsc_websocket_global_lock_dbg() <<< lock_cnt %d\n", websocket_mutex_cnt);
  fflush(stdout);
}

void bsc_websocket_global_unlock_dbg(char *f, int line)
{
  printf("bsc_websocket_global_unlock_dbg() >>> %s:%d lock_cnt %d\n", f, line, websocket_mutex_cnt);
  websocket_mutex_cnt--;
  fflush(stdout);
  pthread_mutex_unlock(&websocket_mutex);
  printf("bsc_websocket_global_unlock_dbg() <<< lock_cnt %d\n", websocket_mutex_cnt);
  fflush(stdout);
}

void bws_dispatch_lock_dbg(char *f, int line)
{
  printf("bws_dispatch_lock_dbg() >>> %s:%d lock_cnt %d\n", f, line, websocket_dispatch_mutex_cnt);
  websocket_dispatch_mutex_cnt++;
  fflush(stdout);
  pthread_mutex_lock(&websocket_dispatch_mutex);
  printf("bws_dispatch_lock_dbg() <<< lock_cnt %d\n", websocket_dispatch_mutex_cnt);
  fflush(stdout);
}

void bws_dispatch_unlock_dbg(char *f, int line)
{
  printf("bws_dispatch_unlock_dbg() >>> %s:%d lock_cnt %d\n", f, line, websocket_dispatch_mutex_cnt);
  websocket_dispatch_mutex_cnt--;
  fflush(stdout);
  pthread_mutex_unlock(&websocket_dispatch_mutex);
  printf("bws_dispatch_unlock_dbg() <<< lock_cnt %d\n", websocket_dispatch_mutex_cnt);
  fflush(stdout);
}

#endif
