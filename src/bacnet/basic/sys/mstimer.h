/**
 * @file
 * @brief API for millisecond timer library
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2009
 * @copyright SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef BACNET_SYS_MSTIMER_H_
#define BACNET_SYS_MSTIMER_H_

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

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
int mstimer_expired(const struct mstimer *t);
BACNET_STACK_EXPORT
void mstimer_expire(struct mstimer *t);
BACNET_STACK_EXPORT
unsigned long mstimer_remaining(const struct mstimer *t);
BACNET_STACK_EXPORT
unsigned long mstimer_elapsed(const struct mstimer *t);
BACNET_STACK_EXPORT
unsigned long mstimer_interval(const struct mstimer *t);
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
