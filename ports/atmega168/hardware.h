/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef HARDWARE_H
#define HARDWARE_H

#if !defined(F_CPU)
    /* The processor clock frequency */
#define F_CPU 7372800UL
#endif

#if defined(__IAR_SYSTEMS_ICC__) || defined(__IAR_SYSTEMS_ASM__)
#include <ioavr.h>
#else
#if !defined(__AVR_ATmega328__)
#error Firmware is configured for ATmega328 only (-mmcu=atmega328)
#endif
#endif
#include "iar2gcc.h"
#include "bacnet/basic/sys/bits.h"

#endif
