/**
 * @file
 * @brief Implementation of websocket client interface.
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
#include <windows.h>
#include "websocket-mutex.h"

static HANDLE websocket_mutex = NULL;
static HANDLE websocket_dispatch_mutex = NULL;

static void bsc_init_mutex(volatile HANDLE* m)
{ 
  HANDLE p;
  if (*m == NULL) 
  { 
    p = CreateMutex(NULL, FALSE, NULL);
    if (InterlockedCompareExchangePointer((PVOID*)m, (PVOID)p, NULL) != NULL)
    {
      CloseHandle(p);
    }
  }
}

void bsc_mutex_lock(volatile HANDLE* m)
{
  bsc_init_mutex(m);
  WaitForSingleObject(*m, INFINITE);
}

void bsc_mutex_unlock(volatile HANDLE* m)
{
  ReleaseMutex(*m);
}

void bsc_websocket_global_lock(void)
{
  bsc_mutex_lock(&websocket_mutex);
}

void bsc_websocket_global_unlock(void)
{
  bsc_mutex_unlock(&websocket_mutex);
}

void bws_dispatch_lock(void)
{
  bsc_mutex_lock(&websocket_dispatch_mutex);
}

void bws_dispatch_unlock(void)
{
  bsc_mutex_unlock(&websocket_dispatch_mutex);
}
