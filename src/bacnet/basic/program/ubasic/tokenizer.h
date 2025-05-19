/*
 * Copyright (c) 2006, Adam Dunkels
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 * Modified to support simple string variables and functions by David Mitchell
 * November 2008.
 * Changes and additions are marked 'string additions' throughout
 *
 * Modified to support Plus extension by Marijan Kostrun 2018.
 *
 * Added averagew function. Steve Karg <skarg@users.sourceforge.net> 2025
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _UBASIC_TOKENIZER_H__
#define _UBASIC_TOKENIZER_H__
#include <stdbool.h>
#include <stdint.h>
#include "config.h"

enum {
    /*0*/ UBASIC_TOKENIZER_ERROR,
    /*1*/ UBASIC_TOKENIZER_ENDOFINPUT,
    /*2*/ UBASIC_TOKENIZER_NUMBER,
#if defined(UBASIC_VARIABLE_TYPE_STRING)
    /*3*/ UBASIC_TOKENIZER_STRING,
#endif
    /*4*/ UBASIC_TOKENIZER_VARIABLE,
#if defined(UBASIC_VARIABLE_TYPE_STRING)
    /* string additions - must be here and in this order */
    /*5*/ UBASIC_TOKENIZER_STRINGVARIABLE,
    /*6*/ UBASIC_TOKENIZER_PRINT_STR,
    /*7*/ UBASIC_TOKENIZER_LEFT_STR,
    /*8*/ UBASIC_TOKENIZER_RIGHT_STR,
    /*9*/ UBASIC_TOKENIZER_MID_STR,
    /*10*/ UBASIC_TOKENIZER_STR_STR,
    /*11*/ UBASIC_TOKENIZER_CHR_STR,
    /*12*/ UBASIC_TOKENIZER_VAL,
    /*13*/ UBASIC_TOKENIZER_LEN,
    /*14*/ UBASIC_TOKENIZER_INSTR,
    /*15*/ UBASIC_TOKENIZER_ASC,
#endif
    /*16*/ UBASIC_TOKENIZER_LET,
    /*17*/ UBASIC_TOKENIZER_PRINTLN,
    /*18*/ UBASIC_TOKENIZER_PRINT,
    /*19*/ UBASIC_TOKENIZER_IF,
    /*20*/ UBASIC_TOKENIZER_THEN,
    /*21*/ UBASIC_TOKENIZER_ELSE,
    /*22*/ UBASIC_TOKENIZER_ENDIF,
    /*23*/ UBASIC_TOKENIZER_FOR,
    /*24*/ UBASIC_TOKENIZER_TO,
    /*25*/ UBASIC_TOKENIZER_NEXT,
    /*26*/ UBASIC_TOKENIZER_STEP,
    /*27*/ UBASIC_TOKENIZER_WHILE,
    /*28*/ UBASIC_TOKENIZER_ENDWHILE,
    /*29*/ UBASIC_TOKENIZER_GOTO,
    /*30*/ UBASIC_TOKENIZER_GOSUB,
    /*31*/ UBASIC_TOKENIZER_RETURN,
    /*32*/ UBASIC_TOKENIZER_END,
    /*33*/ UBASIC_TOKENIZER_COMMA,
    /*34*/ UBASIC_TOKENIZER_PLUS,
    /*35*/ UBASIC_TOKENIZER_MINUS,
    /*36*/ UBASIC_TOKENIZER_AND,
    /*37*/ UBASIC_TOKENIZER_OR,
    /*38*/ UBASIC_TOKENIZER_ASTR,
    /*39*/ UBASIC_TOKENIZER_SLASH,
    /*40*/ UBASIC_TOKENIZER_MOD,
    /*41*/ UBASIC_TOKENIZER_LEFTPAREN,
    /*42*/ UBASIC_TOKENIZER_RIGHTPAREN,
    /*43*/ UBASIC_TOKENIZER_LT,
    /*44*/ UBASIC_TOKENIZER_GT,
    /*45*/ UBASIC_TOKENIZER_EQ,
    /*46*/ UBASIC_TOKENIZER_EOL,
    /* */
    /* Plus : Start */
    /* */
    /*47*/ UBASIC_TOKENIZER_NE,
    /*48*/ UBASIC_TOKENIZER_GE,
    /*49*/ UBASIC_TOKENIZER_LE,
    /*50*/ UBASIC_TOKENIZER_LAND,
    /*51*/ UBASIC_TOKENIZER_LOR,
    /*52*/ UBASIC_TOKENIZER_LNOT,
    /*53*/ UBASIC_TOKENIZER_NOT,
    /*54*/ UBASIC_TOKENIZER_PRINT_HEX,
    /*55*/ UBASIC_TOKENIZER_PRINT_DEC,
#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
    /*56*/ UBASIC_TOKENIZER_INPUT,
#endif
#if defined(UBASIC_SCRIPT_HAVE_SLEEP)
    /*57*/ UBASIC_TOKENIZER_SLEEP,
#endif
#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
    /*58*/ UBASIC_TOKENIZER_PINMODE,
    /*59*/ UBASIC_TOKENIZER_DREAD,
    /*60*/ UBASIC_TOKENIZER_DWRITE,
#endif
#if defined(UBASIC_VARIABLE_TYPE_ARRAY)
    /*61*/ UBASIC_TOKENIZER_DIM,
    /*62*/ UBASIC_TOKENIZER_ARRAYVARIABLE,
#endif
#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
    /*63*/ UBASIC_TOKENIZER_RAN,
#if defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS)
    /*64*/ UBASIC_TOKENIZER_TIC,
    /*65*/ UBASIC_TOKENIZER_TOC,
#endif
#endif
#if defined(UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    /*66*/ UBASIC_TOKENIZER_INT,
    /*67*/ UBASIC_TOKENIZER_FLOAT,
    /*68*/ UBASIC_TOKENIZER_SQRT,
    /*69*/ UBASIC_TOKENIZER_SIN,
    /*70*/ UBASIC_TOKENIZER_COS,
    /*71*/ UBASIC_TOKENIZER_TAN,
    /*72*/ UBASIC_TOKENIZER_EXP,
    /*73*/ UBASIC_TOKENIZER_LN,
#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
    /*74*/ UBASIC_TOKENIZER_UNIFORM,
#endif
    /*75*/ UBASIC_TOKENIZER_ABS,
    /*76*/ UBASIC_TOKENIZER_FLOOR,
    /*77*/ UBASIC_TOKENIZER_CEIL,
    /*78*/ UBASIC_TOKENIZER_ROUND,
    /*79*/ UBASIC_TOKENIZER_POWER,
    /*80*/ UBASIC_TOKENIZER_AVERAGEW,
#endif
#if defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS)
    /*81*/ UBASIC_TOKENIZER_HWE,
#endif
#if defined(UBASIC_SCRIPT_HAVE_PWM_CHANNELS)
    /*82*/ UBASIC_TOKENIZER_PWMCONF,
    /*83*/ UBASIC_TOKENIZER_PWM,
#endif
#if defined(UBASIC_SCRIPT_HAVE_ANALOG_READ)
    /*84*/ UBASIC_TOKENIZER_AREADCONF,
    /*85*/ UBASIC_TOKENIZER_AREAD,
#endif
    /*86*/ UBASIC_TOKENIZER_LABEL,
    /*87*/ UBASIC_TOKENIZER_COLON,
#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
    /*88*/ UBASIC_TOKENIZER_STORE,
    /*89*/ UBASIC_TOKENIZER_RECALL,
#endif
#if defined(UBASIC_SCRIPT_HAVE_BACNET)
    /*90*/ UBASIC_TOKENIZER_BACNET_CREATE_OBJECT,
    /*91*/ UBASIC_TOKENIZER_BACNET_READ_PROPERTY,
    /*92*/ UBASIC_TOKENIZER_BACNET_WRITE_PROPERTY,
#endif
    /*93*/ UBASIC_TOKENIZER_CLEAR,
    /* */
    /* Plus: End */
    /* */
};

struct ubasic_tokenizer {
    const char *ptr;
    const char *nextptr;
    const char *prog;
    uint8_t current_token;
};

void tokenizer_init(struct ubasic_tokenizer *data, const char *program);
void tokenizer_next(struct ubasic_tokenizer *data);
uint8_t tokenizer_token(struct ubasic_tokenizer *data);
UBASIC_VARIABLE_TYPE tokenizer_num(struct ubasic_tokenizer *data);
UBASIC_VARIABLE_TYPE tokenizer_int(struct ubasic_tokenizer *data);

#ifdef FIXEDPT_FBITS
UBASIC_VARIABLE_TYPE tokenizer_float(struct ubasic_tokenizer *data);
#endif

uint8_t tokenizer_variable_num(struct ubasic_tokenizer *data);
bool tokenizer_finished(struct ubasic_tokenizer *data);

#if defined(UBASIC_VARIABLE_TYPE_STRING)
void tokenizer_string(struct ubasic_tokenizer *data, char *dest, uint8_t len);
int8_t tokenizer_stringlookahead(struct ubasic_tokenizer *data);
#endif

void tokenizer_label(struct ubasic_tokenizer *data, char *dest, uint8_t len);
uint16_t tokenizer_save_offset(struct ubasic_tokenizer *data);
void tokenizer_jump_offset(struct ubasic_tokenizer *data, uint16_t offset);

const char *tokenizer_name(UBASIC_VARIABLE_TYPE token);

#endif
