/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/
#ifndef HARDWARE_H
#define HARDWARE_H

#if !defined(F_CPU)
    /* The processor clock frequency */
#define F_CPU 18430000UL
#endif

#if defined(__ICCAVR__)
#include <iom644p.h>
#endif

#if defined(__GNUC__)
#if !defined(__AVR_ATmega644P__)
#error Firmware is configured for ATmega644P only (-mmcu=atmega644p)
#endif
    /* GCC specific configuration */
#include <avr/wdt.h>
#endif

#include "iar2gcc.h"
#include "bits.h"

/* SEEPROM is 24LC128 */
/*#define SEEPROM_PAGE_SIZE 64 */
/*#define SEEPROM_WORD_ADDRESS_16BIT 1 */
/* SEEPROM is 24C16 */
#define SEEPROM_PAGE_SIZE 16
#define SEEPROM_WORD_ADDRESS_16BIT 0

#endif
