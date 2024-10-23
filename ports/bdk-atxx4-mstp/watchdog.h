/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef WATCHDOG_H
#define WATCHDOG_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void watchdog_reset(
        void);
    void watchdog_init(
        unsigned milliseconds);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
