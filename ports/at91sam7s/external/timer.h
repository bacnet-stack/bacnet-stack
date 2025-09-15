/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

extern volatile unsigned long Timer_Milliseconds;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void TimerInit(
        void);
    int Timer_Silence(
        void);
    void Timer_Silence_Reset(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
