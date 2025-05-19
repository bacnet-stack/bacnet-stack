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
#include "bacnet/bacdef.h"

/* Microcontroller related functionality */
#if !(                                                     \
    defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR) || \
    defined(UBASIC_SCRIPT_HAVE_PWM_CHANNELS) ||            \
    defined(UBASIC_SCRIPT_HAVE_GPIO) ||                    \
    defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS) ||         \
    defined(UBASIC_SCRIPT_HAVE_SLEEP) ||                   \
    defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS) ||         \
    defined(UBASIC_SCRIPT_HAVE_PRINT_TO_SERIAL) ||         \
    defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL) ||       \
    defined(UBASIC_SCRIPT_HAVE_ANALOG_READ) ||             \
    defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH) ||     \
    defined(UBASIC_SCRIPT_HAVE_BACNET) ||                  \
    defined(UBASIC_SCRIPT_HAVE_STRING_VARIABLES))
#define UBASIC_SCRIPT_HAVE_ALL
#endif

#if defined(UBASIC_SCRIPT_HAVE_ALL)
#define UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR
#define UBASIC_SCRIPT_HAVE_PWM_CHANNELS 4
#define UBASIC_SCRIPT_HAVE_GPIO
#define UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS 8
#define UBASIC_SCRIPT_HAVE_SLEEP
#define UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS
#define UBASIC_SCRIPT_HAVE_PRINT_TO_SERIAL
#define UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL
#define UBASIC_SCRIPT_HAVE_ANALOG_READ
#define UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH
#define UBASIC_SCRIPT_HAVE_BACNET
#define UBASIC_SCRIPT_HAVE_STRING_VARIABLES
#endif

/**
 *
 *   UBASIC-PLUS: Start
 *
 */

/* This many one-letter variables UBASIC supports */
#ifndef UBASIC_VARNUM_MAX
#define UBASIC_VARNUM_MAX 26
#endif

/* have numeric arrays and set their storage to this many UBASIC_VARIABLE_TYPE
 * entries
 */
#ifndef UBASIC_VARIABLE_TYPE_ARRAY
#define UBASIC_VARIABLE_TYPE_ARRAY 64
#endif

/* have strings and related functions */
#ifdef UBASIC_SCRIPT_HAVE_STRING_VARIABLES
#define UBASIC_VARIABLE_TYPE_STRING
#endif

/* default storage for all numeric values */
#ifndef UBASIC_VARIABLE_STORAGE_INT16
#undef UBASIC_VARIABLE_STORAGE_INT32
#define UBASIC_VARIABLE_STORAGE_INT32
#endif

/* defines the representation of floating point numbers as fixed points:
    this is to allow UBASIC to run on Cortex M0 processors which do not
    support Floating Point Arithmetic in hardware (they emulate it which
    consumes lots of memory) */
#ifndef UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10
#undef UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8
#define UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8
#endif

#if defined(UBASIC_VARIABLE_STORAGE_INT32)
#define UBASIC_VARIABLE_TYPE int32_t
#if defined(UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
#define FIXEDPT_BITS 32
#if defined(UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8)
#define FIXEDPT_WBITS 24
#elif defined(UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
#define FIXEDPT_WBITS 22
#else
#error "Only 24.8 and 22.10 floats are currently supported"
#endif
#include "fixedptc.h"
#endif
#elif defined(UBASIC_VARIABLE_STORAGE_INT16)
#define UBASIC_VARIABLE_TYPE int16_t
#if defined(UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
#error "Fixed Point Floats are Supported for 32bit Storage Only!"
#endif
#else
#error "Only INT32 and INT16 variable types are supported."
#endif

#ifndef UBASIC_STATEMENT_SIZE
#define UBASIC_STATEMENT_SIZE 64
#endif

#ifndef UBASIC_STRINGLEN_MAX
#define UBASIC_STRINGLEN_MAX 40
#endif

#ifndef UBASIC_LABEL_LEN_MAX
#define UBASIC_LABEL_LEN_MAX 10
#endif

#if defined(UBASIC_VARIABLE_TYPE_STRING)
#define UBASIC_STRING_BUFFER_LEN_MAX 256
#define UBASIC_STRING_VAR_LEN_MAX 26
#endif

#endif /* #ifndef _CONFIG_H_ */
