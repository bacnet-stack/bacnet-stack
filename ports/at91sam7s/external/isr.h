/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/

#ifndef ISR_H
#define ISR_H

#include <stdint.h>

#if defined(__ICCARM__)
#include <intrinsics.h>
#define isr_enable() __enable_interrupt()
#define isr_disable() __disable_interrupt()
#define __get_cpsr __get_CPSR
#define __set_cpsr __set_CPSR
#endif
#if defined(__GNUC__)
#define isr_enable() enableIRQ();enableFIQ();
#define isr_disable() disableFIQ();disableIRQ();
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    unsigned disableIRQ(
        void);

    unsigned restoreIRQ(
        unsigned oldCPSR);

    unsigned enableIRQ(
        void);

    unsigned disableFIQ(
        void);

    unsigned restoreFIQ(
        unsigned oldCPSR);

    unsigned enableFIQ(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
