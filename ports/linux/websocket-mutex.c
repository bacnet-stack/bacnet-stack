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

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "websocket-mutex.h"

static pthread_mutex_t websocket_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static pthread_mutex_t websocket_dispatch_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

#ifndef BSC_DEBUG_WEBSOCKET_MUTEX_ENABLED

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
  printf("bsc_websocket_global_lock_dbg() >>> %s:%d lock_cnt %d tid = %ld\n", f, line, websocket_mutex_cnt, pthread_self());
  websocket_mutex_cnt++;
  fflush(stdout);
  pthread_mutex_lock(&websocket_mutex);
  printf("bsc_websocket_global_lock_dbg() <<< lock_cnt %d tid = %ld\n", websocket_mutex_cnt, pthread_self());
  fflush(stdout);
}

void bsc_websocket_global_unlock_dbg(char *f, int line)
{
  printf("bsc_websocket_global_unlock_dbg() >>> %s:%d lock_cnt %d tid = %ld\n", f, line, websocket_mutex_cnt, pthread_self());
  websocket_mutex_cnt--;
  fflush(stdout);
  pthread_mutex_unlock(&websocket_mutex);
  printf("bsc_websocket_global_unlock_dbg() <<< lock_cnt %d tid = %ld\n", websocket_mutex_cnt, pthread_self());
  fflush(stdout);
}

void bws_dispatch_lock_dbg(char *f, int line)
{
  printf("bws_dispatch_lock_dbg() >>> %s:%d lock_cnt %d tid = %ld\n", f, line, websocket_dispatch_mutex_cnt, pthread_self());
  websocket_dispatch_mutex_cnt++;
  fflush(stdout);
  pthread_mutex_lock(&websocket_dispatch_mutex);
  printf("bws_dispatch_lock_dbg() <<< lock_cnt %d tid = %ld\n", websocket_dispatch_mutex_cnt, pthread_self());
  fflush(stdout);
}

void bws_dispatch_unlock_dbg(char *f, int line)
{
  printf("bws_dispatch_unlock_dbg() >>> %s:%d lock_cnt %d  tid = %ld\n", f, line, websocket_dispatch_mutex_cnt, pthread_self());
  websocket_dispatch_mutex_cnt--;
  fflush(stdout);
  pthread_mutex_unlock(&websocket_dispatch_mutex);
  printf("bws_dispatch_unlock_dbg() <<< lock_cnt %d tid = %ld\n", websocket_dispatch_mutex_cnt, pthread_self());
  fflush(stdout);
}

#endif
