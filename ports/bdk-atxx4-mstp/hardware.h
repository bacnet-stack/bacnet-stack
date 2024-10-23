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
#define F_CPU 18432000UL
#endif

/* IAR compiler specific configuration */
#if defined(__ICCAVR__)
#if defined(__ATmega644P__)
#include <iom644p.h>
#endif
#if defined(__ATmega1284P__)
#include <iom1284p.h>
#endif
#endif

/* AVR-GCC compiler specific configuration */
#if defined(__GNUC__)
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#if defined(__AVR_ATmega644P__)
/* defined for ATmega644p */
#elif defined(__AVR_ATmega1284P__)
/* defined for ATmega1284p */
#else
#error For ATmega644P or ATmega1284p only (-mmcu=atmega644p -mmcu=atmega1284p)
#endif
#endif

#if defined (__CROSSWORKS_AVR)
#include <avr.h>
#if (__TARGET_PROCESSOR != ATmega644P)
#error Firmware is configured for ATmega644P only
#endif
#endif

#include "iar2gcc.h"
#include "bacnet/basic/sys/bits.h"

/* SEEPROM is 24LC128 */
/*#define SEEPROM_PAGE_SIZE 64 */
/*#define SEEPROM_WORD_ADDRESS_16BIT 1 */
/* SEEPROM is 24C16 */
#ifndef SEEPROM_PAGE_SIZE
#define SEEPROM_PAGE_SIZE 16
#endif
#ifndef SEEPROM_WORD_ADDRESS_16BIT
#define SEEPROM_WORD_ADDRESS_16BIT 0
#endif

/* Serial EEPROM address */
#define SEEPROM_I2C_ADDRESS 0xA0
/* Serial EEPROM clocking speed - usually 100000 or 400000 */
#define SEEPROM_I2C_CLOCK 400000L
/* Serial EEPROM max write cycle in milliseconds as defined by datasheet */
#define SEEPROM_WRITE_CYCLE 5

#define LED_2 2
#define LED_3 3
#define LED_4 1
#define LED_5 0
#define MAX_LEDS 4

#endif
