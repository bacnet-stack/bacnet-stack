/**
 * @file
 * @brief crossplatform event abstraction used in BACNet secure connect.
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

/**
 * @brief  bsc_event_init() allocates and initializes auto-reset
 *         event object to non-signalled state. An event object
 *         can be set to signalled state by the call of
 *         bsc_event_signal(). When the state of the event object
 *         is signaled, it remains signaled until last thread is
 *         released which was blocked on bsc_event_wait() or
 *         bsc_event_timed_wait() calls. When writing the code user
 *         must always remember the following statements:
 *         1. It is guaranteed that all currently waiting threads will be
 *            unblocked by the call of bsc_event_signal().
 *         2. If user has called bsc_event_wait() or bsc_event_timedwait()
 *            from thread N after the call of bsc_event_signal() but
 *            when the state of the corresponed event object is still
 *            signalled, it is not guaranteed that thread N will be
 *            unblocked. Thread N may be stay blocked or may unblock
 *            depending on multitasking context of the OS.
 *
 * @return handle of event object if function succeeded, otherwise
 *         returns NULL.
 */

BSC_EVENT *bsc_event_init(void);

/**
 * @brief  bsc_event_deinit() deinitialize auto-reset
 *         event object. If a user calls that function while
 *         some threads are waiting the specified event object ev
 *         the behaviour is undefined.
 *
 * @param  ev - handle to previously initialized event object.
 */

void bsc_event_deinit(BSC_EVENT *ev);

/**
 * @brief  bsc_wait() suspends the execution of the current thread
 *                    for the specified amount of seconds.
 *
 * @param  seconds - time to sleep in seconds.
 */

void bsc_wait(int seconds);

/**
 * @brief  bsc_event_wait() suspends the execution of the current thread
 *                          until corresponded event object becomes signalled.
 *
 * @param  ev - handle to previously initialized event object.
 */

void bsc_event_wait(BSC_EVENT *ev);

/**
 * @brief  bsc_event_timedwait() suspends the execution of the current thread
 *                          until corresponded event object becomes signalled
 *                          or ms_timeout becomes elapsed.
 *
 * @param  ev - handle to previously initialized event object.
 * @param  ms_timeout - timeout in milliseconds
 * @return true if the corresponded event was signalled.
 *         false if ms_timeout was elapsed but event ev is still in
 *               non signalled state.
 */

bool bsc_event_timedwait(BSC_EVENT *ev, unsigned int ms_timeout);

/**
 * @brief  bsc_event_signal() function sets state of corresponded event object
 *                            to signalled state.
 *
 * @param  ev - handle to previously initialized event object.
 */

void bsc_event_signal(BSC_EVENT *ev);

#endif
