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
 * Modified to support fixed point arithmetic, and number of math and io and
 * hardware functions by Marijan Kostrun, January-February 2018.
 * uBasic-Plus Copyright (c) 2017-2018, M. Kostrun
 *
 * Modified to support multiple programs and use a structure to hold
 * each program state. 2025 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __UBASIC_H__
#define __UBASIC_H__
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include "config.h"
#include "platform.h"
#include "tokenizer.h"

/* define a status structure with bit fields */
typedef union {
    uint8_t byte;
    struct {
        uint8_t notInitialized : 1;
        uint8_t stringstackModified : 1;
        uint8_t bit2 : 1;
        uint8_t bit3 : 1;
        uint8_t bit4 : 1;
        uint8_t WaitForSerialInput : 1;
        uint8_t Error : 1;
        uint8_t isRunning : 1;
    } bit;
} UBASIC_STATUS;

#define MAX_FOR_STACK_DEPTH 4
struct ubasic_for_state {
    uint16_t line_after_for;
    uint8_t for_variable;
    VARIABLE_TYPE to;
    VARIABLE_TYPE step;
};

#define MAX_WHILE_STACK_DEPTH 4
struct ubasic_while_state {
    uint16_t line_while;
    int16_t line_after_endwhile;
};

#define MAX_GOSUB_STACK_DEPTH 10
#define MAX_IF_STACK_DEPTH 4

#define UBASIC_SERIAL_INPUT_MS 50

enum {
    UBASIC_RECALL_STORE_TYPE_VARIABLE = 0,
    UBASIC_RECALL_STORE_TYPE_STRING = 1,
    UBASIC_RECALL_STORE_TYPE_ARRAY = 2,
    UBASIC_RECALL_STORE_TYPE_MAX = 3
};

/**
 * A timer.
 *
 * This structure is used for declaring a timer. The timer must be set
 * with mstimer_set() before it can be used.
 */
struct ubasic_mstimer {
    uint32_t start;
    uint32_t interval;
};

struct ubasic_data {
    UBASIC_STATUS status;
    uint8_t input_how;
    struct tokenizer_data tree;

#if defined(VARIABLE_TYPE_ARRAY)
    VARIABLE_TYPE arrays_data[VARIABLE_TYPE_ARRAY];
    int16_t free_arrayptr;
    int16_t arrayvariable[MAX_VARNUM];
#endif
    const char *program_ptr;

    uint16_t gosub_stack[MAX_GOSUB_STACK_DEPTH];
    uint8_t gosub_stack_ptr;

    struct ubasic_for_state for_stack[MAX_FOR_STACK_DEPTH];
    uint8_t for_stack_ptr;

    int16_t if_stack[MAX_IF_STACK_DEPTH];
    uint8_t if_stack_ptr;

    struct ubasic_while_state while_stack[MAX_WHILE_STACK_DEPTH];
    uint8_t while_stack_ptr;

    VARIABLE_TYPE variables[MAX_VARNUM];

#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
    uint8_t varnum;
#endif

#if defined(VARIABLE_TYPE_STRING)
    char stringstack[MAX_BUFFERLEN];
    int16_t freebufptr;
    int16_t stringvariables[MAX_SVARNUM];
#endif

#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
    uint8_t input_varnum;
    uint8_t input_type;
    char statement[UBASIC_STATEMENT_SIZE];
#endif
#if defined(VARIABLE_TYPE_ARRAY)
    VARIABLE_TYPE input_array_index;
#endif
#if defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS)
    uint32_t tic_toc_timer[UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS];
#endif
#if defined(UBASIC_SCRIPT_HAVE_SLEEP)
    struct ubasic_mstimer input_wait_timer;
    struct ubasic_mstimer sleep_timer;
#endif

// API for hardware drivers
#if defined(UBASIC_SCRIPT_HAVE_PWM_CHANNELS)
    void (*pwm_config)(uint16_t psc, uint16_t per);
    void (*pwm_write)(uint8_t ch, int16_t dutycycle);
    int16_t (*pwm_read)(uint8_t ch);
#endif
#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
    void (*gpio_config)(uint8_t ch, int8_t mode, uint8_t freq);
    void (*gpio_write)(uint8_t ch, uint8_t pin_state);
    int8_t (*gpio_read)(uint8_t ch);
#endif
#if (                                              \
    defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS) || \
    defined(UBASIC_SCRIPT_HAVE_SLEEP) ||           \
    defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL))
    uint32_t (*mstimer_now)(void);
#endif
#if defined(UBASIC_SCRIPT_HAVE_ANALOG_READ)
    void (*adc_config)(uint8_t sampletime, uint8_t nreads);
    int16_t (*adc_read)(uint8_t channel);
#endif
#if defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS)
    int8_t (*hw_event)(uint8_t bit);
    void (*hw_event_clear)(uint8_t bit);
#endif
#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
    uint32_t (*random_uint32)(uint8_t size);
#endif
#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
    void (*flash_write)(
        uint8_t Name, uint8_t Vartype, uint8_t datalen_bytes, uint8_t *dataptr);
    void (*flash_read)(
        uint8_t Name, uint8_t Vartype, uint8_t *dataptr, uint8_t *datalen);
#endif
#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
    int (*ubasic_getc)(void);
#endif
#if defined(UBASIC_SCRIPT_PRINT_TO_SERIAL)
    void (*serial_write)(const char *buffer, uint16_t n);
#endif
#if defined(UBASIC_SCRIPT_HAVE_BACNET)
    void (*bacnet_create_object)(
        uint16_t object_type, uint32_t instance, char *object_name);
    void (*bacnet_write_property)(
        uint16_t object_type,
        uint32_t instance,
        uint32_t property_id,
        VARIABLE_TYPE value);
    VARIABLE_TYPE(*bacnet_read_property)
    (uint16_t object_type, uint32_t instance, uint32_t property_id);
#endif
};

void ubasic_load_program(struct ubasic_data *data, const char *program);
void ubasic_clear_variables(struct ubasic_data *data);
void ubasic_run_program(struct ubasic_data *data);
uint8_t ubasic_execute_statement(struct ubasic_data *data, char *statement);
uint8_t ubasic_finished(struct ubasic_data *data);

uint8_t ubasic_waiting_for_input(struct ubasic_data *data);
uint8_t ubasic_getline(struct ubasic_data *data, int ch);
int ubasic_printf(struct ubasic_data *data, const char *format, ...);
int ubasic_getc(struct ubasic_data *data);

VARIABLE_TYPE ubasic_get_variable(struct ubasic_data *data, char variable);
void ubasic_set_variable(
    struct ubasic_data *data, char variable, VARIABLE_TYPE value);

#if defined(VARIABLE_TYPE_ARRAY)
void ubasic_dim_arrayvariable(
    struct ubasic_data *data, char variable, int16_t size);
void ubasic_set_arrayvariable(
    struct ubasic_data *data, char variable, uint16_t idx, VARIABLE_TYPE value);
VARIABLE_TYPE
ubasic_get_arrayvariable(struct ubasic_data *data, char variable, uint16_t idx);
#endif

#if defined(VARIABLE_TYPE_STRING)
int16_t ubasic_get_stringvariable(struct ubasic_data *data, uint8_t varnum);
void ubasic_set_stringvariable(
    struct ubasic_data *data, uint8_t varnum, int16_t size);
#endif

/* API to interface and initialize the hardware drivers */
void ubasic_hardware_init(struct ubasic_data *data);

#endif /* __UBASIC_H__ */
