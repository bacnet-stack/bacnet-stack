/**
 * @file
 * @brief Millisecond timer library header file.
 * @details The mstimer library provides functions for setting, resetting and
 * restarting timers, and for checking if a timer has expired. An
 * application must "manually" check if its timers have expired; this
 * is not done automatically.
 *
 * A timer is declared as a \c struct \c mstimer and all access to the
 * timer is made by a pointer to the declared timer.
 *
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2009
 * @note Adapted from the Contiki operating system.
 * Original Authors: Adam Dunkels <adam@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 * @copyright SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/basic/sys/mstimer.h"

/* callback data head of list */
static struct mstimer_callback_data_t *Callback_Head;

/**
 * Handles an interrupt from a hardware millisecond timer
 */
void mstimer_callback_handler(void)
{
    struct mstimer_callback_data_t *cb;

    cb = (struct mstimer_callback_data_t *)Callback_Head;
    while (cb) {
        if (mstimer_expired(&cb->timer)) {
            cb->callback();
            if (mstimer_interval(&cb->timer) > 0) {
                mstimer_reset(&cb->timer);
            }
        }
        cb = cb->next;
    }
}

/**
 * Configures and enables a repeating callback function
 *
 * @param new_cb - pointer to #mstimer_callback_data_t
 * @param callback - pointer to a #timer_callback_function function
 * @param milliseconds - how often to call the function
 */
void mstimer_callback(struct mstimer_callback_data_t *new_cb,
    mstimer_callback_function callback,
    unsigned long milliseconds)
{
    struct mstimer_callback_data_t *cb;

    if (new_cb) {
        new_cb->callback = callback;
        mstimer_set(&new_cb->timer, milliseconds);
    }
    if (Callback_Head) {
        cb = (struct mstimer_callback_data_t *)Callback_Head;
        while (cb) {
            if (!cb->next) {
                cb->next = new_cb;
                break;
            } else {
                cb = cb->next;
            }
        }
    } else {
        Callback_Head = new_cb;
    }
}

/**
 * @brief Set a timer for a time sometime in the future
 *
 * This function is used to set a timer for a time sometime in the
 * future. The function mstimer_expired() will evaluate to true after
 * the timer has expired.
 *
 * @param t A pointer to the timer
 * @param interval The interval before the timer expires.
 */
void mstimer_set(struct mstimer *t, unsigned long interval)
{
    t->interval = interval;
    t->start = mstimer_now();
}

/**
 * @brief Reset the timer with the same interval.
 *
 * This function resets the timer with the same interval that was
 * given to the mstimer_set() function. The start point of the interval
 * is the exact time that the timer last expired. Therefore, this
 * function will cause the timer to be stable over time, unlike the
 * mstimer_restart() function.
 *
 * @param t A pointer to the timer.
 * @sa mstimer_restart()
 */
void mstimer_reset(struct mstimer *t)
{
    t->start += t->interval;
}

/**
 * @brief Restart the timer from the current point in time
 *
 * This function restarts a timer with the same interval that was
 * given to the mstimer_set() function. The timer will start at the
 * current time.
 *
 * @note A periodic timer will drift if this function is used to reset
 * it. For periodic timers, use the mstimer_reset() function instead.
 * @param t A pointer to the timer.
 * @sa mstimer_reset()
 */
void mstimer_restart(struct mstimer *t)
{
    t->start = mstimer_now();
}

/**
 * @brief Check if a timer has expired.
 *
 * This function tests if a timer has expired and returns true or
 * false depending on its status.
 *
 * @param t A pointer to the timer
 * @return Non-zero if the timer has expired, zero otherwise.
 */
int mstimer_expired(struct mstimer *t)
{
    if (t->interval) {
        return ((unsigned long)((mstimer_now()) - (t->start + t->interval)) <
            ((unsigned long)(~((unsigned long)0)) >> 1));
    }

    return 0;
}
/*---------------------------------------------------------------------------*/

/**
 * @brief Expire the timer from the current point in time
 *
 * This function expires a timer with the same interval that was
 * given to the mstimer_set() function.
 *
 * @param t A pointer to the timer.
 * @sa mstimer_reset()
 */
void mstimer_expire(struct mstimer *t)
{
    t->start -= t->interval;
}

/**
 * The time until the timer expires
 *
 * This function returns the time until the timer expires.
 *
 * @param t A pointer to the timer
 *
 * @return The time until the timer expires
 *
 */
unsigned long mstimer_remaining(struct mstimer *t)
{
    return t->start + t->interval - mstimer_now();
}

/**
 * The time elapsed since the timer started
 *
 * This function returns the time elapsed.
 *
 * @param t A pointer to the timer
 *
 * @return The time elapsed since the last start of the timer
 *
 */
unsigned long mstimer_elapsed(struct mstimer *t)
{
    return mstimer_now() - t->start;
}

/**
 * The value of the interval
 *
 * This function returns the interval value
 *
 * @param t A pointer to the timer
 *
 * @return The interval value
 *
 */
unsigned long mstimer_interval(struct mstimer *t)
{
    return t->interval;
}
