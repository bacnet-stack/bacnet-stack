/*-
 * Copyright (c) 2017-18, Marijan Kostrun <mkostrun@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

//
// Undef it all
//

/* Storage and arithmetics */
#undef VARIABLE_STORAGE_INT16
#undef VARIABLE_STORAGE_INT32
#undef VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8
#undef VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10
#undef VARIABLE_TYPE_STRING
#undef VARIABLE_TYPE_ARRAY
#undef UBASIC_SCRIPT_HAVE_DEMO_SCRIPTS

/* Microcontroller related functionality */
#undef UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR
#undef UBASIC_SCRIPT_HAVE_PWM_CHANNELS
#undef UBASIC_SCRIPT_HAVE_GPIO
#undef UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS
#undef UBASIC_SCRIPT_HAVE_SLEEP
#undef UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS
#undef UBASIC_SCRIPT_PRINT_TO_SERIAL
#undef UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL
#undef UBASIC_SCRIPT_HAVE_ANALOG_READ
#undef UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH
#undef UBASIC_SCRIPT_HAVE_BACNET

/**
 *
 *   UBASIC-PLUS: Start
 *
 */
/* default storage for all numeric values */
#define VARIABLE_STORAGE_INT32

/* defines the representation of floating point numbers as fixed points:
    this is to allow UBASIC to run on Cortex M0 processors which do not
    support Floating Point Arithmetic in hardware (they emulate it which
    consumes lots of memory) */
#define VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8

/* This many one-letter variables UBASIC supports */
#define MAX_VARNUM 26

/* have numeric arrays and set their storage to this many VARIABLE_TYPE entries
 */
#define VARIABLE_TYPE_ARRAY 64

/* have strings and related functions */
#define VARIABLE_TYPE_STRING

/* can go to sleep: leave UBASIC for other stuff while waiting for timer to
 * expire */
#define UBASIC_SCRIPT_HAVE_SLEEP

/* have microcontroller support for PWM: specify how many channels */
#define UBASIC_SCRIPT_HAVE_PWM_CHANNELS (4)

/* have internal timer channels available through rlab-like toc(ch) functions */
#define UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS (8)

/* support for random number generator by micro-controller */
#define UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR

/* support for direct access to pin inputs and ooutputs */
#define UBASIC_SCRIPT_HAVE_GPIO_CHANNELS

/* support flags in BASIC that change on hardware events:
    for STM32F0XX nucleo and discovery boards
   source of the events is push-button
*/
#define UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS

/* have a standard print to serial console function */
#define UBASIC_SCRIPT_PRINT_TO_SERIAL

/* how is input function supported ? */
#define UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL

/* support for analog inputs */
#define UBASIC_SCRIPT_HAVE_ANALOG_READ

/* support for BACnet objects and ReadProperty and WriteProperty (internal) */
#define UBASIC_SCRIPT_HAVE_BACNET

/* Demo scripts are huge. Do we need them? */
#define UBASIC_SCRIPT_HAVE_DEMO_SCRIPTS

/* support for storing/recalling variables in/from flash memory */
#define UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH
/**
 *
 *   UBASIC-PLUS: End
 *
 */

/*
 * Selectively load header files based on the ocnfiguration above.
 * Remember MC Hammer:
 *                      CAN'T TOUCH THIS!
 *
 */
#if defined(VARIABLE_STORAGE_INT32)

#define VARIABLE_TYPE int32_t

#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)

#define FIXEDPT_BITS 32

#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8)
#define FIXEDPT_WBITS 24
#elif defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
#define FIXEDPT_WBITS 22
#else
#error "Only 24.8 and 22.10 floats are currently supported"
#endif

#include "fixedptc.h"

#endif

#elif defined(VARIABLE_STORAGE_INT16)

#define VARIABLE_TYPE int16_t
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
#error "Fixed Point Floats are Supported for 32bit Storage Only!"
#endif

#else

#error "Only INT32 and INT16 variable types are supported."

#endif

#define UBASIC_STATEMENT_SIZE (64)

#define MAX_STRINGLEN 40
#define MAX_LABEL_LEN 10

#if defined(VARIABLE_TYPE_STRING)
#define MAX_STRINGVARLEN 64
#define MAX_BUFFERLEN 256
#define MAX_SVARNUM 26
#endif

#endif /* #ifndef _CONFIG_H_ */
