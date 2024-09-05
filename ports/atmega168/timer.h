/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef TIMER_H
#define TIMER_H

extern volatile uint8_t Timer_Milliseconds;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void Timer_Initialize(
        void);
    uint16_t Timer_Silence(
        void);
    void Timer_Silence_Reset(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
