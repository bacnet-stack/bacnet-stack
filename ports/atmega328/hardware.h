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
#error Set F_CPU in the Makefile
#endif

/* IAR compiler specific configuration */
#if defined(__IAR_SYSTEMS_ICC__) || defined(__IAR_SYSTEMS_ASM__)
#include <ioavr.h>
#include <iom328p.h>
#endif

/* AVR-GCC compiler specific configuration */
#if defined(__GNUC__)
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#endif

#if !defined(__AVR_ATmega328P__)
#error Firmware is configured for ATmega328P only (-mmcu=atmega328p)
#endif

#include "iar2gcc.h"
#include "bacnet/basic/sys/bits.h"

#endif
