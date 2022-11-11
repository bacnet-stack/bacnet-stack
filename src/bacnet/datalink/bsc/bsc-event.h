/**
 * @file
 * @brief event abstraction used in BACNet secure connect.
 * @author Kirill Neznamov
 * @date August 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BSC__EVENT__INCLUDED__
#define __BSC__EVENT__INCLUDED__

struct BSC_Event;
typedef struct BSC_Event BSC_EVENT;

BSC_EVENT* bsc_event_init(void);
void bsc_event_deinit(BSC_EVENT *ev);
void bsc_event_wait(BSC_EVENT *ev);
void bsc_event_signal(BSC_EVENT *ev);
void bsc_wait(int seconds);
#endif