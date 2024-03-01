/**
 * @file
 * @author Steve Karg
 * @date 2009
 * @brief Millisecond timer library header file.
 *
 * @section DESCRIPTION
 *
 * The mstimer library provides functions for setting, resetting and
 * restarting timers, and for checking if a timer has expired. An
 * application must "manually" check if its timers have expired; this
 * is not done automatically.
 *
 * A timer is declared as a \c struct \c mstimer and all access to the
 * timer is made by a pointer to the declared timer.
 *
 * Adapted from the Contiki operating system.
 * Original Authors: Adam Dunkels <adam@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 */
#ifndef MSTIMER_H_
#define MSTIMER_H_

#include "bacnet/bacnet_stack_exports.h"

/**
 * A timer.
 *
 * This structure is used for declaring a timer. The timer must be set
 * with mstimer_set() before it can be used.
 *
 * \hideinitializer
 */
struct mstimer {
  unsigned long start;
  unsigned long interval;
};

/* optional callback function form */
typedef void (*mstimer_callback_function) (void);
/* optional callback data structure */
struct mstimer_callback_data_t;
struct mstimer_callback_data_t {
    struct mstimer timer;
    mstimer_callback_function callback;
    struct mstimer_callback_data_t *next;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void mstimer_set(struct mstimer *t, unsigned long interval);
BACNET_STACK_EXPORT
void mstimer_reset(struct mstimer *t);
BACNET_STACK_EXPORT
void mstimer_restart(struct mstimer *t);
BACNET_STACK_EXPORT
int mstimer_expired(struct mstimer *t);
BACNET_STACK_EXPORT
void mstimer_expire(struct mstimer *t);
BACNET_STACK_EXPORT
unsigned long mstimer_remaining(struct mstimer *t);
BACNET_STACK_EXPORT
unsigned long mstimer_elapsed(struct mstimer *t);
BACNET_STACK_EXPORT
unsigned long mstimer_interval(struct mstimer *t);
/* optional callback timer support for embedded systems */
BACNET_STACK_EXPORT
void mstimer_callback(
    struct mstimer_callback_data_t *cb,
    mstimer_callback_function callback,
    unsigned long milliseconds);
BACNET_STACK_EXPORT
void mstimer_callback_handler(void);
/* HAL implementation */
BACNET_STACK_EXPORT
unsigned long mstimer_now(void);
BACNET_STACK_EXPORT
void mstimer_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
