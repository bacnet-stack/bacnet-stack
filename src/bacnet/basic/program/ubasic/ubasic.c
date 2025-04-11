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
 * Modified to support multiple programs and use a structure to hold
 * each program state. 2025 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Includes: Generic
 * ----------------------------------------------------------*/
#include "config.h"
#include "ubasic.h"
#include "tokenizer.h"

#if defined(VARIABLE_TYPE_STRING)
static int16_t sexpr(struct ubasic_data *data);
static int16_t scpy(struct ubasic_data *data, char *);
static int16_t sconcat(struct ubasic_data *data, char *, char *);
static int16_t sleft(struct ubasic_data *data, char *, int16_t);
static int16_t sright(struct ubasic_data *data, char *, int16_t);
static int16_t smid(struct ubasic_data *data, char *, int16_t, int16_t);
static int16_t sstr(struct ubasic_data *data, VARIABLE_TYPE j);
static int16_t schr(struct ubasic_data *data, VARIABLE_TYPE j);
static uint8_t sinstr(uint16_t, char *, char *);
static char *strptr(struct ubasic_data *data, int16_t size)
{
    return ((char *)((data->stringstack) + (size) + 1));
}
#endif
static VARIABLE_TYPE relation(struct ubasic_data *data);
static void numbered_line_statement(struct ubasic_data *data);
static void statement(struct ubasic_data *data);
#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
static VARIABLE_TYPE recall_statement(struct ubasic_data *data);
#endif

/*---------------------------------------------------------------------------*/
#if (                                              \
    defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS) || \
    defined(UBASIC_SCRIPT_HAVE_SLEEP) ||           \
    defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL))
/**
 * @brief Calculates the elapsed time since the given start time.
 * @param start The start time.
 * @param now The current time.
 * @return The elapsed time, in milliseconds.
 */
static uint32_t mstimer_since(uint32_t start, uint32_t now)
{
    return now - start;
}

/**
 * @brief Set a timer for a time sometime in the future
 *
 * This function is used to set a timer for a time sometime in the
 * future. The function mstimer_remaining() will determine how much time
 * is left until the timer expires.
 *
 * @param t A pointer to the timer
 * @param interval The interval before the timer expires.
 * @param now The current time.
 */
static void
mstimer_set(struct ubasic_mstimer *t, uint32_t interval, uint32_t now)
{
    t->interval = interval;
    t->start = now;
}

/**
 * @brief Check if a timer has expired.
 *
 * This function tests if a timer has expired and returns true or
 * false depending on its status.
 *
 * @param t A pointer to the timer
 * @param now The current time.
 * @return Non-zero if the timer has expired, zero otherwise.
 */
static int mstimer_expired(const struct ubasic_mstimer *t, uint32_t now)
{
    return (
        (now - (t->start + t->interval)) < ((uint32_t)(~((uint32_t)0)) >> 1));
}

/**
 * @brief The amount of time until the timer expires
 * @param t A pointer to the timer
 * @return The time until the timer expires
 */
static uint32_t mstimer_remaining(const struct ubasic_mstimer *t, uint32_t now)
{
    if (t->interval) {
        if (!mstimer_expired(t, now)) {
            return ((t->start + t->interval) - now);
        }
    }

    return 0;
}
#endif

/*---------------------------------------------------------------------------*/
#if defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS)
static void timer_tic(struct ubasic_data *data, uint8_t ch)
{
    if (ch > UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS) {
        return;
    }
    if ((data->mstimer_now)) {
        data->tic_toc_timer[ch] = data->mstimer_now();
    }
}

static int32_t timer_toc(struct ubasic_data *data, uint8_t ch)
{
    uint32_t elapsed;
    if ((ch > UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS) || (!data->mstimer_now)) {
        return 0;
    }
    elapsed = mstimer_since(data->tic_toc_timer[ch], data->mstimer_now());
    if (elapsed > INT32_MAX) {
        return INT32_MAX;
    }
    return (int32_t)elapsed;
}
#endif

/*---------------------------------------------------------------------------*/
#if defined(UBASIC_SCRIPT_HAVE_SLEEP)
static void mstimer_sleep(struct ubasic_data *data, int32_t ms)
{
    if (data->mstimer_now) {
        mstimer_set(&data->sleep_timer, ms, data->mstimer_now());
    }
}

static int32_t mstimer_sleeping(struct ubasic_data *data)
{
    int32_t ms;
    if (data->mstimer_now) {
        ms =
            (int32_t)mstimer_remaining(&data->sleep_timer, data->mstimer_now());
        if (ms == 0) {
            /* automatic disable */
            data->sleep_timer.interval = 0;
        }
    } else {
        ms = 0;
    }
    return ms;
}
#endif

/*---------------------------------------------------------------------------*/
#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
static void mstimer_input_wait(struct ubasic_data *data, int32_t ms)
{
    if (data->mstimer_now) {
        mstimer_set(&data->input_wait_timer, ms, data->mstimer_now());
    }
}

static int32_t mstimer_input_remaining(struct ubasic_data *data)
{
    int32_t ms;
    if (data->mstimer_now) {
        ms = (int32_t)mstimer_remaining(
            &data->input_wait_timer, data->mstimer_now());
        if (ms == 0) {
            /* automatic disable */
            data->input_wait_timer.interval = 0;
        }
    } else {
        ms = 0;
    }
    return ms;
}
#endif

/*---------------------------------------------------------------------------*/
#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
static void
gpio_config(struct ubasic_data *data, uint8_t ch, int8_t mode, uint8_t freq)
{
    if (data->gpio_config) {
        data->gpio_config(ch, mode, freq);
    }
}
static void gpio_write(struct ubasic_data *data, uint8_t ch, uint8_t pin_state)
{
    if (data->gpio_write) {
        data->gpio_write(ch, pin_state);
    }
}
static int8_t gpio_read(struct ubasic_data *data, uint8_t ch)
{
    if (data->gpio_read) {
        return data->gpio_read(ch);
    }
    return 0;
}
#endif

/*---------------------------------------------------------------------------*/
#if defined(UBASIC_SCRIPT_HAVE_ANALOG_READ)
static void
adc_config(struct ubasic_data *data, uint8_t sampletime, uint8_t nreads)
{
    if (data->adc_config) {
        data->adc_config(sampletime, nreads);
    }
}
static int16_t adc_read(struct ubasic_data *data, uint8_t channel)
{
    if (data->adc_read) {
        return data->adc_read(channel);
    }
    return 0;
}
#endif

/*---------------------------------------------------------------------------*/
#if defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS)
static int8_t hw_event(struct ubasic_data *data, uint8_t bit)
{
    if (data->hw_event) {
        return data->hw_event(bit);
    }
    return 0;
}
static void hw_event_clear(struct ubasic_data *data, uint8_t bit)
{
    if (data->hw_event_clear) {
        data->hw_event_clear(bit);
    }
}
#endif

/*---------------------------------------------------------------------------*/
#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
static uint32_t random_uint32(struct ubasic_data *data, uint8_t size)
{
    if (data->random_uint32) {
        return data->random_uint32(size);
    }
    return 0;
}
#endif

/*---------------------------------------------------------------------------*/
#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
static void flash_write(
    struct ubasic_data *data,
    uint8_t Name,
    uint8_t Vartype,
    uint8_t datalen_bytes,
    uint8_t *dataptr)
{
    if (data->flash_write) {
        data->flash_write(Name, Vartype, datalen_bytes, dataptr);
    }
}
static void flash_read(
    struct ubasic_data *data,
    uint8_t Name,
    uint8_t Vartype,
    uint8_t *dataptr,
    uint8_t *datalen)
{
    if (data->flash_read) {
        data->flash_read(Name, Vartype, dataptr, datalen);
    }
}
#endif
/*---------------------------------------------------------------------------*/
#if defined(UBASIC_SCRIPT_PRINT_TO_SERIAL)
static void
serial_write(struct ubasic_data *data, const char *buffer, uint16_t n)
{
    if (data->serial_write) {
        data->serial_write(buffer, n);
    }
}
#endif
static void serial_write_string(struct ubasic_data *data, const char *msg)
{
#if defined(UBASIC_SCRIPT_PRINT_TO_SERIAL)
    serial_write(data, msg, strlen(msg));
#endif
}
/**
 * @brief Print with a printf string
 * @param format - printf format string
 * @param ... - variable arguments
 * @note This function is only available if
 * PRINT_ENABLED is non-zero
 * @return number of characters printed
 */
int ubasic_printf(struct ubasic_data *data, const char *format, ...)
{
    int length = 0;
    char buffer[256];
    va_list ap;

    va_start(ap, format);
    length = vsnprintf(buffer, sizeof(buffer), format, ap);
#if defined(UBASIC_SCRIPT_PRINT_TO_SERIAL)
    serial_write(data, buffer, length);
#endif
    va_end(ap);

    return length;
}

int ubasic_getc(struct ubasic_data *data)
{
#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
    if (data->ubasic_getc) {
        return data->ubasic_getc();
    }
#endif
    return EOF;
}

/*---------------------------------------------------------------------------*/
void ubasic_clear_variables(struct ubasic_data *data)
{
    int16_t i;
    for (i = 0; i < MAX_VARNUM; i++) {
        data->variables[i] = 0;
#if defined(VARIABLE_TYPE_ARRAY)
        data->arrayvariable[i] = -1;
#endif
    }

#if defined(VARIABLE_TYPE_ARRAY)
    data->free_arrayptr = 0;
    for (i = 0; i < VARIABLE_TYPE_ARRAY; i++) {
        data->arrays_data[i] = 0;
    }
#endif

#if defined(VARIABLE_TYPE_STRING)
    data->freebufptr = 0;
    for (i = 0; i < MAX_SVARNUM; i++) {
        data->stringvariables[i] = -1;
    }
#endif
}

/*---------------------------------------------------------------------------*/
void ubasic_load_program(struct ubasic_data *data, const char *program)
{
    data->for_stack_ptr = data->gosub_stack_ptr = 0;
    if (data->status.bit.notInitialized) {
        ubasic_clear_variables(data);
    }
    data->status.byte = 0x00;
    if (program) {
        data->program_ptr = program;
        tokenizer_init(&data->tree, program);
        data->status.bit.isRunning = 1;
    }
}
/*---------------------------------------------------------------------------*/
static void token_error_print(struct ubasic_data *data, VARIABLE_TYPE token)
{
    char msg[32];
    const char *name;
    struct tokenizer_data *tree = &data->tree;

    serial_write_string(data, "Err");
    name = tokenizer_name(token);
    if (name) {
        snprintf(msg, sizeof(msg), "[%s]:", name);
    } else {
        snprintf(msg, sizeof(msg), "[%u]:", (unsigned)token);
    }
    serial_write_string(data, msg);
    serial_write_string(data, tree->ptr - 1);
    serial_write_string(data, "\n");
}

/*---------------------------------------------------------------------------*/
static uint8_t accept(struct ubasic_data *data, VARIABLE_TYPE token)
{
    struct tokenizer_data *tree = &data->tree;

    if (token != tokenizer_token(tree)) {
        token_error_print(data, token);
        return 1;
    }

    tokenizer_next(tree);
    return 0;
}

/*---------------------------------------------------------------------------*/
static void accept_cr(struct tokenizer_data *tree)
{
    while ((tokenizer_token(tree) != TOKENIZER_EOL) &&
           (tokenizer_token(tree) != TOKENIZER_ERROR) &&
           (tokenizer_token(tree) != TOKENIZER_ENDOFINPUT)) {
        tokenizer_next(tree);
    }

    if (tokenizer_token(tree) == TOKENIZER_EOL) {
        tokenizer_next(tree);
    }
}

#if defined(VARIABLE_TYPE_STRING)

/*---------------------------------------------------------------------------*/
static uint8_t string_space_check(struct ubasic_data *data, uint16_t l)
{
    // returns true if not enough room for new string
    uint8_t i;
    i = ((MAX_BUFFERLEN - data->freebufptr) <= (l + 2)); // +2 to play it safe
    if (i) {
        data->status.bit.isRunning = 0;
        data->status.bit.Error = 1;
    }
    return i;
}

/*---------------------------------------------------------------------------*/
static void clear_stringstack(struct ubasic_data *data)
{
    // if (!status.bit.stringstackModified )
    // return;

    data->status.bit.stringstackModified = 0;

    int16_t bottom = 0;
    int16_t len = 0;

    // find bottom of the stringstack skip allocated stringstack space
    while (*(data->stringstack + bottom) != 0) {
        bottom += strlen(strptr(data, bottom)) + 2;
        if (data->freebufptr == bottom) {
            return;
        }
    }

    int16_t top = bottom;
    do {
        len = strlen(strptr(data, top)) + 2;

        if (*(data->stringstack + top) > 0) {
            // moving stuff down
            for (uint8_t i = 0; i <= len; i++) {
                *(data->stringstack + bottom + i) =
                    *(data->stringstack + top + i);
            }

            // update variable reference from top to bottom
            data->stringvariables[*(data->stringstack + bottom) - 1] = bottom;

            bottom += len;
        }
        top += len;
    } while (top < data->freebufptr);
    data->freebufptr = bottom;

    return;
}
/*---------------------------------------------------------------------------*/
// copy s1 at the end of stringstack and add a header
static int16_t scpy(struct ubasic_data *data, char *s1)
{
    if (!s1) {
        return (-1);
    }

    int16_t bp = data->freebufptr;
    int16_t l = strlen(s1);

    if (!l) {
        return (-1);
    }

    if (string_space_check(data, l)) {
        return (-1);
    }

    data->status.bit.stringstackModified = 1;

    *(data->stringstack + bp) = 0;
    memcpy(data->stringstack + bp + 1, s1, l + 1);

    data->freebufptr = bp + l + 2;

    return bp;
}

/*---------------------------------------------------------------------------*/
// return the concatenation of s1 and s2 in a string at the end
// of the stringbuffer
static int16_t sconcat(struct ubasic_data *data, char *s1, char *s2)
{
    int16_t l1 = strlen(s1), l2 = strlen(s2);

    if (string_space_check(data, l1 + l2)) {
        return (-1);
    }

    int16_t rp = scpy(data, s1);
    data->freebufptr -= 2; // last char in s1, will be overwritten by s2 header
    int16_t fp = data->freebufptr;
    char dummy = *(data->stringstack + fp);
    scpy(data, s2);
    *(data->stringstack + fp) = dummy; // overwrite s2 header
    return (rp);
}
/*---------------------------------------------------------------------------*/
static int16_t sleft(
    struct ubasic_data *data,
    char *s1,
    int16_t l) // return the left l chars of s1
{
    int16_t bp = data->freebufptr;
    int16_t rp = bp;

    if (l < 1) {
        return (-1);
    }

    if (string_space_check(data, l)) {
        return (-1);
    }

    data->status.bit.stringstackModified = 1;

    if (strlen(s1) <= l) {
        return scpy(data, s1);
    } else {
        // write header
        *(data->stringstack + bp) = 0;
        bp++;
        memcpy(data->stringstack + bp, s1, l);
        bp += l;
        *(data->stringstack + bp) = 0;
        data->freebufptr = bp + 1;
    }

    return rp;
}
/*---------------------------------------------------------------------------*/
static int16_t sright(
    struct ubasic_data *data,
    char *s1,
    int16_t l) // return the right l chars of s1
{
    int16_t j = strlen(s1);

    if (l < 1) {
        return (-1);
    }

    if (j <= l) {
        l = j;
    }

    if (string_space_check(data, l)) {
        return (-1);
    }

    return scpy(data, s1 + j - l);
}

/*---------------------------------------------------------------------------*/
static int16_t smid(
    struct ubasic_data *data,
    char *s1,
    int16_t l1,
    int16_t l2) // return the l2 chars of s1 starting at offset l1
{
    int16_t bp = data->freebufptr;
    int16_t rp = bp;
    int16_t i, j = strlen(s1);

    if (l2 < 1 || l1 > j) {
        return (-1);
    }

    if (string_space_check(data, l2)) {
        return (-1);
    }

    if (l2 > j - l1) {
        l2 = j - l1;
    }

    data->status.bit.stringstackModified = 1;

    *(data->stringstack + bp) = 0;
    bp++;

    for (i = l1; i < l1 + l2; i++) {
        *(data->stringstack + bp) = *(s1 + l1 - 1);
        bp++;
        s1++;
    }

    *(data->stringstack + bp) = '\0';
    data->freebufptr = bp + 1;
    return rp;
}
/*---------------------------------------------------------------------------*/
static int16_t sstr(
    struct ubasic_data *data,
    VARIABLE_TYPE j) // return the integer j as a string
{
    int16_t bp = data->freebufptr;
    int16_t rp = bp;

    if (string_space_check(data, 10)) {
        return (-1);
    }

    data->status.bit.stringstackModified = 1;

    *(data->stringstack + bp) = 0;
    bp++;

    sprintf((char *)(data->stringstack + bp), "%ld", (long)j);

    data->freebufptr = bp + strlen((char *)(data->stringstack + bp)) + 1;

    return rp;
}
/*---------------------------------------------------------------------------*/
static int16_t schr(
    struct ubasic_data *data,
    VARIABLE_TYPE j) // return the character whose ASCII code is j
{
    int16_t bp = data->freebufptr;
    int16_t rp = bp;

    if (string_space_check(data, 1)) {
        return (-1);
    }

    data->status.bit.stringstackModified = 1;

    *(data->stringstack + bp) = 0;
    bp++;

    *(data->stringstack + bp) = j;
    bp++;

    *(data->stringstack + bp) = 0;
    bp++;

    data->freebufptr = bp;
    return rp;
}
/*---------------------------------------------------------------------------*/
static uint8_t
sinstr(uint16_t j, char *s, char *s1) // return the position of s1 in s (or 0)
{
    char *p;
    p = strstr(s + j, s1);
    if (p == NULL) {
        return 0;
    }
    return (p - s + 1);
}

/*---------------------------------------------------------------------------*/
static int16_t sfactor(struct ubasic_data *data)
{
    // string form of factor
    int16_t r = 0, s = 0;
    char tmpstring[MAX_STRINGLEN];
    VARIABLE_TYPE i, j;
    struct tokenizer_data *tree = &data->tree;

    switch (tokenizer_token(tree)) {
        case TOKENIZER_LEFTPAREN:
            accept(data, TOKENIZER_LEFTPAREN);
            r = sexpr(data);
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

        case TOKENIZER_STRING:
            tokenizer_string(tree, tmpstring, MAX_STRINGLEN);
            r = scpy(data, tmpstring);
            accept(data, TOKENIZER_STRING);
            break;

        case TOKENIZER_LEFT$:
            accept(data, TOKENIZER_LEFT$);
            accept(data, TOKENIZER_LEFTPAREN);
            s = sexpr(data);
            accept(data, TOKENIZER_COMMA);
            i = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            i = fixedpt_toint(i);
#endif
            r = sleft(data, strptr(data, s), i);
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

        case TOKENIZER_RIGHT$:
            accept(data, TOKENIZER_RIGHT$);
            accept(data, TOKENIZER_LEFTPAREN);
            s = sexpr(data);
            accept(data, TOKENIZER_COMMA);
            i = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            i = fixedpt_toint(i);
#endif
            r = sright(data, strptr(data, s), i);
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

        case TOKENIZER_MID$:
            accept(data, TOKENIZER_MID$);
            accept(data, TOKENIZER_LEFTPAREN);
            s = sexpr(data);
            accept(data, TOKENIZER_COMMA);
            i = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            i = fixedpt_toint(i);
#endif
            if (tokenizer_token(tree) == TOKENIZER_COMMA) {
                accept(data, TOKENIZER_COMMA);
                j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
                j = fixedpt_toint(j);
#endif
            } else {
                j = 999; // ensure we get all of it
            }
            r = smid(data, strptr(data, s), i, j);
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

        case TOKENIZER_STR$:
            accept(data, TOKENIZER_STR$);
            j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            j = fixedpt_toint(j);
#endif
            r = sstr(data, j);
            break;

        case TOKENIZER_CHR$:
            accept(data, TOKENIZER_CHR$);
            j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            j = fixedpt_toint(j);
#endif
            if (j < 0 || j > 255) {
                j = 0;
            }
            r = schr(data, j);
            break;

        default:
            r = ubasic_get_stringvariable(data, tokenizer_variable_num(tree));
            accept(data, TOKENIZER_STRINGVARIABLE);
    }

    return r;
}

/*---------------------------------------------------------------------------*/

static int16_t sexpr(struct ubasic_data *data) // string form of expr
{
    int16_t s1, s2;
    s1 = sfactor(data);
    uint8_t op = tokenizer_token(&data->tree);
    while (op == TOKENIZER_PLUS) {
        tokenizer_next(&data->tree);
        s2 = sfactor(data);
        s1 = sconcat(data, strptr(data, s1), strptr(data, s2));
        op = tokenizer_token(&data->tree);
    }
    return s1;
}
/*---------------------------------------------------------------------------*/
static uint8_t slogexpr(struct ubasic_data *data) // string logical expression
{
    int16_t s1, s2;
    uint8_t r = 0;
    s1 = sexpr(data);
    uint8_t op = tokenizer_token(&data->tree);
    tokenizer_next(&data->tree);
    if (op == TOKENIZER_EQ) {
        s2 = sexpr(data);
        r = (strcmp(strptr(data, s1), strptr(data, s2)) == 0);
    }
    return r;
}
// end of string additions
#endif

/*---------------------------------------------------------------------------*/
static VARIABLE_TYPE varfactor(struct ubasic_data *data)
{
    VARIABLE_TYPE r;
    struct tokenizer_data *tree = &data->tree;

    r = ubasic_get_varnum(data, tokenizer_variable_num(tree));
    accept(data, TOKENIZER_VARIABLE);
    return r;
}
/*---------------------------------------------------------------------------*/
static VARIABLE_TYPE factor(struct ubasic_data *data)
{
    VARIABLE_TYPE r;
    VARIABLE_TYPE i, j, k;
#if defined(VARIABLE_TYPE_ARRAY)
    uint8_t varnum;
#endif
#if defined(VARIABLE_TYPE_STRING)
    int16_t s, s1;
#endif
    struct tokenizer_data *tree = &data->tree;

    switch (tokenizer_token(tree)) {
#if defined(VARIABLE_TYPE_STRING)
        case TOKENIZER_LEN:
            accept(data, TOKENIZER_LEN);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            r = fixedpt_fromint(strlen(strptr(data, sexpr(data))));
#else
            r = strlen(strptr(data, sexpr(data)));
#endif
            break;

        case TOKENIZER_VAL:
            accept(data, TOKENIZER_VAL);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            s1 = sexpr(data);
            r = str_fixedpt(strptr(data, s1), strlen(strptr(data, s1)), 3);
#else
            r = atoi(strptr(sexpr()));
#endif
            break;

        case TOKENIZER_ASC:
            accept(data, TOKENIZER_ASC);
            s = sexpr(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            r = fixedpt_fromint(*strptr(data, s));
#else
            r = *strptr(data, s);
#endif
            break;

        case TOKENIZER_INSTR:
            accept(data, TOKENIZER_INSTR);
            accept(data, TOKENIZER_LEFTPAREN);
            j = 1;
            if (tokenizer_token(tree) == TOKENIZER_NUMBER) {
                j = tokenizer_num(tree);
                accept(data, TOKENIZER_NUMBER);
                accept(data, TOKENIZER_COMMA);
            }
            if (j < 1) {
                return 0;
            }
            s = sexpr(data);
            accept(data, TOKENIZER_COMMA);
            s1 = sexpr(data);
            accept(data, TOKENIZER_RIGHTPAREN);
            r = sinstr(j, strptr(data, s), strptr(data, s1));
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            r = fixedpt_fromint(r);
#endif
            break;
#endif

        case TOKENIZER_MINUS:
            accept(data, TOKENIZER_MINUS);
            r = -factor(data);
            break;

        case TOKENIZER_LNOT:
            accept(data, TOKENIZER_LNOT);
            r = !relation(data);
            break;

        case TOKENIZER_NOT:
            accept(data, TOKENIZER_LNOT);
            r = ~relation(data);
            break;

#if defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS)
        case TOKENIZER_TOC:
            accept(data, TOKENIZER_TOC);
            accept(data, TOKENIZER_LEFTPAREN);
            r = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            r = fixedpt_toint(r);
#endif
            r = timer_toc(data, r);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            r = fixedpt_fromint(r);
            accept(data, TOKENIZER_RIGHTPAREN);
#endif
            break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS)
        case TOKENIZER_HWE:
            accept(data, TOKENIZER_HWE);
            accept(data, TOKENIZER_LEFTPAREN);
            r = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            r = fixedpt_toint(r);
#endif
            if (r) {
                if (hw_event(data, r - 1)) {
                    hw_event_clear(data, r - 1);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
                    r = FIXEDPT_ONE;
#else
                    r = 1;
#endif
                } else {
                    r = 0;
                }
            }
            accept(data, TOKENIZER_RIGHTPAREN);
            break;
#endif /* #if defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS) */

#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
        case TOKENIZER_RAN:
            accept(data, TOKENIZER_RAN);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            r = random_uint32(data, FIXEDPT_WBITS);
            r = fixedpt_fromint(r);
#else
            r = random_uint32(data, 32);
#endif
            r = fixedpt_abs(r);
            break;
#endif

        case TOKENIZER_ABS:
            accept(data, TOKENIZER_ABS);
            accept(data, TOKENIZER_LEFTPAREN);
            r = relation(data);
            r = fixedpt_abs(r);
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
        case TOKENIZER_AVERAGEW:
            accept(data, TOKENIZER_AVERAGEW);
            accept(data, TOKENIZER_LEFTPAREN);
            // latest_reading
            i = relation(data);
            accept(data, TOKENIZER_COMMA);
            // previous_average
            j = relation(data);
            accept(data, TOKENIZER_COMMA);
            // nsamples
            k = relation(data);
            r = fixedpt_averagew(i, j, k);
            accept(data, TOKENIZER_RIGHTPAREN);
            break;
        case TOKENIZER_POWER:
            accept(data, TOKENIZER_POWER);
            accept(data, TOKENIZER_LEFTPAREN);
            // argument:
            i = relation(data);
            accept(data, TOKENIZER_COMMA);
            // exponent
            j = relation(data);
            r = fixedpt_pow(i, j);
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

        case TOKENIZER_FLOAT:
            r = tokenizer_float(tree); /* 24.8 decimal number */
            accept(data, TOKENIZER_FLOAT);
            break;

        case TOKENIZER_SQRT:
            accept(data, TOKENIZER_SQRT);
            accept(data, TOKENIZER_LEFTPAREN);
            r = fixedpt_sqrt(relation(data));
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

        case TOKENIZER_SIN:
            accept(data, TOKENIZER_SIN);
            accept(data, TOKENIZER_LEFTPAREN);
            r = fixedpt_sin(relation(data));
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

        case TOKENIZER_COS:
            accept(data, TOKENIZER_COS);
            accept(data, TOKENIZER_LEFTPAREN);
            r = fixedpt_cos(relation(data));
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

        case TOKENIZER_TAN:
            accept(data, TOKENIZER_TAN);
            accept(data, TOKENIZER_LEFTPAREN);
            r = fixedpt_tan(relation(data));
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

        case TOKENIZER_EXP:
            accept(data, TOKENIZER_EXP);
            accept(data, TOKENIZER_LEFTPAREN);
            r = fixedpt_exp(relation(data));
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

        case TOKENIZER_LN:
            accept(data, TOKENIZER_LN);
            accept(data, TOKENIZER_LEFTPAREN);
            r = fixedpt_ln(relation(data));
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
        case TOKENIZER_UNIFORM:
            accept(data, TOKENIZER_UNIFORM);
            r = random_uint32(data, FIXEDPT_FBITS) & FIXEDPT_FMASK;
            break;
#endif

        case TOKENIZER_FLOOR:
            accept(data, TOKENIZER_FLOOR);
            accept(data, TOKENIZER_LEFTPAREN);
            r = relation(data);
            r = fixedpt_floor(r);
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

        case TOKENIZER_CEIL:
            accept(data, TOKENIZER_CEIL);
            accept(data, TOKENIZER_LEFTPAREN);
            r = relation(data);
            r = fixedpt_ceil(r);
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

        case TOKENIZER_ROUND:
            accept(data, TOKENIZER_ROUND);
            accept(data, TOKENIZER_LEFTPAREN);
            r = relation(data);
            r = fixedpt_round(r);
            accept(data, TOKENIZER_RIGHTPAREN);
            break;
#endif /* #if defined(VARIABLE_TYPE_FLOAT_AS ... */

        case TOKENIZER_INT:
            r = tokenizer_int(tree);
            accept(data, TOKENIZER_INT);
            break;

        case TOKENIZER_NUMBER:
            r = tokenizer_num(tree);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            r = fixedpt_fromint(r);
#endif
            accept(data, TOKENIZER_NUMBER);
            break;

#ifdef UBASIC_SCRIPT_HAVE_PWM_CHANNELS
        case TOKENIZER_PWM:
            accept(data, TOKENIZER_PWM);
            accept(data, TOKENIZER_LEFTPAREN);
            // single argument: channel
            j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            j = fixedpt_toint(j);
#endif
            if (j < 1 || j > UBASIC_SCRIPT_HAVE_PWM_CHANNELS ||
                !data->pwm_read) {
                r = -1;
            } else {
                r = data->pwm_read(j - 1);
            }
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            r = fixedpt_fromint(r);
#endif
            accept(data, TOKENIZER_RIGHTPAREN);
            break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_ANALOG_READ)
        case TOKENIZER_AREAD:
            accept(data, TOKENIZER_AREAD);
            accept(data, TOKENIZER_LEFTPAREN);
            // single argument: channel as hex value
            j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            j = fixedpt_toint(j);
#endif
            r = adc_read(data, j);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            r = fixedpt_fromint(r);
#endif
            accept(data, TOKENIZER_RIGHTPAREN);
            break;
#endif

        case TOKENIZER_LEFTPAREN:
            accept(data, TOKENIZER_LEFTPAREN);
            r = relation(data);
            accept(data, TOKENIZER_RIGHTPAREN);
            break;

#if defined(VARIABLE_TYPE_ARRAY)
        case TOKENIZER_ARRAYVARIABLE:
            varnum = tokenizer_variable_num(tree);
            accept(data, TOKENIZER_ARRAYVARIABLE);
            j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            j = fixedpt_toint(j);
#endif
            r = ubasic_get_arrayvariable(data, varnum, (uint16_t)j);
            break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
        case TOKENIZER_DREAD:
            accept(data, TOKENIZER_LEFTPAREN);
            r = relation(data);
            r = gpio_read(data, r);
            accept(data, TOKENIZER_RIGHTPAREN);
            break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
        case TOKENIZER_RECALL:
            r = recall_statement(data);
            break;
#endif

        default:
            r = varfactor(data);
            break;
    }

    return r;
}

/*---------------------------------------------------------------------------*/

static VARIABLE_TYPE term(struct ubasic_data *data)
{
    VARIABLE_TYPE f1, f2;
    struct tokenizer_data *tree = &data->tree;

#if defined(VARIABLE_TYPE_STRING)
    if (tokenizer_stringlookahead(tree)) {
        f1 = slogexpr(data);
    } else
#endif
    {
        f1 = factor(data);
        VARIABLE_TYPE op = tokenizer_token(tree);
        while (op == TOKENIZER_ASTR || op == TOKENIZER_SLASH ||
               op == TOKENIZER_MOD) {
            tokenizer_next(tree);
            f2 = factor(data);
            switch (op) {
                case TOKENIZER_ASTR:
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
                    f1 = fixedpt_xmul(f1, f2);
#else
                    f1 = f1 * f2;
#endif
                    break;

                case TOKENIZER_SLASH:
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
                    f1 = fixedpt_xdiv(f1, f2);
#else
                    f1 = f1 / f2;
#endif
                    break;

                case TOKENIZER_MOD:
                    f1 = f1 % f2;
                    break;
                default:
                    break;
            }
            op = tokenizer_token(tree);
        }
    }
    return f1;
}
/*---------------------------------------------------------------------------*/
static VARIABLE_TYPE relation(struct ubasic_data *data)
{
    VARIABLE_TYPE r1, r2;

    r1 = (VARIABLE_TYPE)term(data);

    VARIABLE_TYPE op = tokenizer_token(&data->tree);

    while (op == TOKENIZER_LT || op == TOKENIZER_LE || op == TOKENIZER_GT ||
           op == TOKENIZER_GE || op == TOKENIZER_EQ || op == TOKENIZER_NE ||
           op == TOKENIZER_LAND || op == TOKENIZER_LOR ||
           op == TOKENIZER_PLUS || op == TOKENIZER_MINUS ||
           op == TOKENIZER_AND || op == TOKENIZER_OR) {
        tokenizer_next(&data->tree);
        r2 = (VARIABLE_TYPE)term(data);

        switch (op) {
            case TOKENIZER_LE:
                r1 = (r1 <= r2);
                break;

            case TOKENIZER_LT:
                r1 = (r1 < r2);
                break;

            case TOKENIZER_GT:
                r1 = (r1 > r2);
                break;

            case TOKENIZER_GE:
                r1 = (r1 >= r2);
                break;

            case TOKENIZER_EQ:
                r1 = (r1 == r2);
                break;

            case TOKENIZER_NE:
                r1 = (r1 != r2);
                break;

            case TOKENIZER_LAND:
                r1 = (r1 && r2);
                break;

            case TOKENIZER_LOR:
                r1 = (r1 || r2);
                break;

            case TOKENIZER_PLUS:
                r1 = r1 + r2;
                break;

            case TOKENIZER_MINUS:
                r1 = r1 - r2;
                break;

            case TOKENIZER_AND:
                r1 = ((int32_t)r1) & ((int32_t)r2);
                break;

            case TOKENIZER_OR:
                r1 = ((int32_t)r1) | ((int32_t)r2);
                break;

            default:
                break;
        }
        op = tokenizer_token(&data->tree);
    }

    return r1;
}

// TODO: error handling?
static uint8_t jump_label(struct ubasic_data *data, char *label)
{
    char currLabel[MAX_LABEL_LEN] = { '\0' };
    struct tokenizer_data *tree = &data->tree;

    tokenizer_init(tree, data->program_ptr);

    while (tokenizer_token(tree) != TOKENIZER_ENDOFINPUT) {
        tokenizer_next(tree);

        if (tokenizer_token(tree) == TOKENIZER_COLON) {
            tokenizer_next(tree);
            if (tokenizer_token(tree) == TOKENIZER_LABEL) {
                tokenizer_label(tree, currLabel, sizeof(currLabel));
                if (strcmp(label, currLabel) == 0) {
                    accept(data, TOKENIZER_LABEL);
                    return 1;
                }
            }
        }
    }

    if (tokenizer_token(tree) == TOKENIZER_ENDOFINPUT) {
        return 0;
    }

    return 1;
}

static void gosub_statement(struct ubasic_data *data)
{
    char tmpstring[MAX_STRINGLEN];
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_GOSUB);
    if (tokenizer_token(tree) == TOKENIZER_LABEL) {
        // copy label
        tokenizer_label(tree, tmpstring, MAX_STRINGLEN);
        tokenizer_next(tree);

        // check for the end of line
        while (tokenizer_token(tree) == TOKENIZER_EOL) {
            tokenizer_next(tree);
        }
        //     accept_cr(tree);

        if (data->gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH) {
            /*    tokenizer_line_number_inc();*/
            // gosub_stack[gosub_stack_ptr] = tokenizer_line_number();
            data->gosub_stack[data->gosub_stack_ptr] =
                tokenizer_save_offset(tree);
            data->gosub_stack_ptr++;
            jump_label(data, tmpstring);
            return;
        }
    }

    token_error_print(data, TOKENIZER_GOSUB);
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 1;
}

static void return_statement(struct ubasic_data *data)
{
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_RETURN);
    if (data->gosub_stack_ptr > 0) {
        data->gosub_stack_ptr--;
        // jump_line(gosub_stack[gosub_stack_ptr]);
        tokenizer_jump_offset(tree, data->gosub_stack[data->gosub_stack_ptr]);
        return;
    }
    token_error_print(data, TOKENIZER_RETURN);
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 1;
}

static void goto_statement(struct ubasic_data *data)
{
    char tmpstring[MAX_STRINGLEN];
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_GOTO);

    if (tokenizer_token(tree) == TOKENIZER_LABEL) {
        tokenizer_label(tree, tmpstring, sizeof(tmpstring));
        tokenizer_next(tree);
        jump_label(data, tmpstring);
        return;
    }

    token_error_print(data, TOKENIZER_GOTO);
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 1;
}

/*---------------------------------------------------------------------------*/
#ifdef UBASIC_SCRIPT_HAVE_PWM_CHANNELS
static void pwm_statement(struct ubasic_data *data)
{
    VARIABLE_TYPE j, r;
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_PWM);

    accept(data, TOKENIZER_LEFTPAREN);

    // first argument: channel
    j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    j = fixedpt_toint(j);
#endif
    if (j < 1 || j > 4) {
        return;
    }

    accept(data, TOKENIZER_COMMA);

    // second argument: value
    r = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_toint(r);
#endif
    accept(data, TOKENIZER_RIGHTPAREN);

    if (j >= 1 && j <= UBASIC_SCRIPT_HAVE_PWM_CHANNELS) {
        if (data->pwm_write) {
            data->pwm_write(j - 1, r);
        }
    }

    accept_cr(tree);
    return;
}

static void pwmconf_statement(struct ubasic_data *data)
{
    VARIABLE_TYPE j, r;
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_PWMCONF);
    accept(data, TOKENIZER_LEFTPAREN);
    // first argument: prescaler 0...
    j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    j = fixedpt_toint(j);
#endif
    if (j < 0) {
        j = 0;
    }

    accept(data, TOKENIZER_COMMA);
    r = relation(data);
    // second argument: period
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_toint(r);
#endif
    if (data->pwm_config) {
        data->pwm_config(j, r);
    }
    r = 0;
    accept(data, TOKENIZER_RIGHTPAREN);
    accept_cr(tree);
}

#endif

#if defined(UBASIC_SCRIPT_HAVE_ANALOG_READ)
static void areadconf_statement(struct ubasic_data *data)
{
    VARIABLE_TYPE j, r;
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_AREADCONF);
    accept(data, TOKENIZER_LEFTPAREN);
    // first argument: sampletime 0...7 on STM32
    j = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    j = fixedpt_toint(j);
#endif
    if (j < 0) {
        j = 0;
    }
    if (j > 7) {
        j = 7;
    }
    accept(data, TOKENIZER_COMMA);
    r = relation(data);
    // second argument: number of analog sample to average from
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_toint(r);
#endif
    adc_config(data, j, r);
    accept(data, TOKENIZER_RIGHTPAREN);
    accept_cr(tree);
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
static void pinmode_statement(struct ubasic_data *data)
{
    VARIABLE_TYPE i, j, r;
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_PINMODE);
    accept(data, TOKENIZER_LEFTPAREN);

    // channel - should be entered as 0x..
    i = relation(data);
    if (i < 0xa0 || i > 0xff) {
        return;
    }

    accept(data, TOKENIZER_COMMA);

    // mode
    j = relation(data);

#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    j = fixedpt_toint(j);
#endif

    if (j < -2) {
        j = -1;
    }
    if (j > 2) {
        j = 0;
    }

    accept(data, TOKENIZER_COMMA);

    // speed
    r = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_toint(r);
#endif
    if (r < 0 || r > 2) {
        r = 0;
    }

    accept(data, TOKENIZER_RIGHTPAREN);

    gpio_config(data, (uint8_t)i, (int8_t)j, (int8_t)r);

    accept_cr(tree);

    return;
}

static void dwrite_statemet(struct ubasic_data *data)
{
    VARIABLE_TYPE j, r;
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_DWRITE);
    accept(data, TOKENIZER_LEFTPAREN);
    j = relation(data);
    accept(data, TOKENIZER_COMMA);
    r = relation(data);
    if (r) {
        r = 0x01;
    }
    accept(data, TOKENIZER_RIGHTPAREN);
    gpio_write(data, j, r);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    r = fixedpt_fromint(r);
#endif
    accept_cr(tree);

    return;
}

#endif /* UBASIC_SCRIPT_HAVE_ANALOG_READ */

static void print_statement(struct ubasic_data *data, uint8_t println)
{
    uint8_t print_how = 0; /*0-xp, 1-hex, 2-oct, 3-dec, 4-bin*/
    char tmpstring[MAX_STRINGLEN];
    struct tokenizer_data *tree = &data->tree;

    // string additions
    if (println) {
        accept(data, TOKENIZER_PRINTLN);
    } else {
        accept(data, TOKENIZER_PRINT);
    }
    do {
        if (tokenizer_token(tree) == TOKENIZER_PRINT_HEX) {
            tokenizer_next(tree);
            print_how = 1;
        } else if (tokenizer_token(tree) == TOKENIZER_PRINT_DEC) {
            tokenizer_next(tree);
            print_how = 2;
        }
#if defined(VARIABLE_TYPE_STRING)
        if (tokenizer_token(tree) == TOKENIZER_STRING) {
            tokenizer_string(tree, tmpstring, MAX_STRINGLEN);
            tokenizer_next(tree);
        } else
#endif
            if (tokenizer_token(tree) == TOKENIZER_COMMA) {
            sprintf(tmpstring, " ");
            tokenizer_next(tree);
        } else {
#if defined(VARIABLE_TYPE_STRING)
            if (tokenizer_stringlookahead(tree)) {
                sprintf(tmpstring, "%s", strptr(data, sexpr(data)));
            } else
#endif
            {
                if (print_how == 1) {
                    sprintf(tmpstring, "%lx", (unsigned long)relation(data));
                } else if (print_how == 2) {
                    sprintf(tmpstring, "%ld", (long)relation(data));
                } else {
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
                    fixedpt_str(relation(data), tmpstring, FIXEDPT_FBITS / 3);
#else
                    sprintf(tmpstring, "%ld", relation(data));
#endif
                }
            }
            // end of string additions
        }
        serial_write_string(data, tmpstring);
    } while (tokenizer_token(tree) != TOKENIZER_EOL &&
             tokenizer_token(tree) != TOKENIZER_ENDOFINPUT);

    // printf("\n");
    if (println) {
        serial_write_string(data, "\n");
    }

    accept_cr(tree);
}
/*---------------------------------------------------------------------------*/
static void endif_statement(struct ubasic_data *data)
{
    if (data->if_stack_ptr > 0) {
        accept(data, TOKENIZER_ENDIF);
        accept(data, TOKENIZER_EOL);
        data->if_stack_ptr--;
        return;
    }
    token_error_print(data, TOKENIZER_IF);
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 1;
    return;
}

static void if_statement(struct ubasic_data *data)
{
    int8_t else_cntr, endif_cntr, f_nt, f_sl;
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_IF);

    VARIABLE_TYPE r = relation(data);

    if (accept(data, TOKENIZER_THEN)) {
        token_error_print(data, TOKENIZER_IF);
        data->status.bit.isRunning = 0;
        data->status.bit.Error = 1;
        return;
    }

    if (tokenizer_token(tree) == TOKENIZER_EOL) {
        // Multi-line IF-Statement
        // CR after then -> multiline IF-Statement
        if (data->if_stack_ptr < MAX_IF_STACK_DEPTH) {
            data->if_stack[data->if_stack_ptr] = r;
            data->if_stack_ptr++;
        } else {
            token_error_print(data, TOKENIZER_IF);
            data->status.bit.isRunning = 0;
            data->status.bit.Error = 1;
            return;
        }
        accept(data, TOKENIZER_EOL);
        if (r) {
            return;
        } else {
            else_cntr = endif_cntr =
                0; // number of else/endif possible in current nesting
            f_nt = f_sl =
                0; // f_nt flag for additional next token, f_fs flag single line

            while (((tokenizer_token(tree) != TOKENIZER_ELSE &&
                     tokenizer_token(tree) != TOKENIZER_ENDIF) ||
                    else_cntr || endif_cntr) &&
                   tokenizer_token(tree) != TOKENIZER_ENDOFINPUT) {
                f_nt = 0;

                // nested if
                if (tokenizer_token(tree) == TOKENIZER_IF) {
                    else_cntr += 1;
                    endif_cntr += 1;
                    f_sl = 0;
                }

                if (tokenizer_token(tree) == TOKENIZER_THEN) {
                    f_nt = 1;
                    tokenizer_next(tree);
                    if (tokenizer_token(tree) != TOKENIZER_EOL) {
                        f_sl = 1;
                    }
                }

                if (tokenizer_token(tree) == TOKENIZER_ELSE) {
                    else_cntr--;
                    if (else_cntr < 0) {
                        token_error_print(data, TOKENIZER_IF);
                        data->status.bit.isRunning = 0;
                        data->status.bit.Error = 1;
                        return;
                    }
                }

                if (!f_sl && (tokenizer_token(tree) == TOKENIZER_ENDIF)) {
                    endif_cntr--;
                    if (endif_cntr != else_cntr) {
                        else_cntr--;
                    }
                } else {
                    if (f_sl && (tokenizer_token(tree) == TOKENIZER_EOL)) {
                        f_sl = 0;
                        endif_cntr--;
                        if (endif_cntr != else_cntr) {
                            else_cntr--;
                        }
                    } else {
                        if (tokenizer_token(tree) == TOKENIZER_ENDIF) {
                            token_error_print(data, TOKENIZER_IF);
                            data->status.bit.isRunning = 0;
                            data->status.bit.Error = 1;
                            return;
                        }
                    }
                }
                if (!f_nt) {
                    tokenizer_next(tree);
                }
            }

            if (tokenizer_token(tree) == TOKENIZER_ELSE) {
                return;
            }
        }
        endif_statement(data);
    } else {
        // Single-line IF-Statement
        if (r) {
            statement(data);
        } else {
            do {
                tokenizer_next(tree);
            } while (tokenizer_token(tree) != TOKENIZER_ELSE &&
                     tokenizer_token(tree) != TOKENIZER_EOL &&
                     tokenizer_token(tree) != TOKENIZER_ENDOFINPUT);
            if (tokenizer_token(tree) == TOKENIZER_ELSE) {
                accept(data, TOKENIZER_ELSE);
                tokenizer_next(tree);
                statement(data);
            } else {
                accept_cr(tree);
            }
        }
    }
}

static void else_statement(struct ubasic_data *data)
{
    VARIABLE_TYPE r = 0;
    uint8_t endif_cntr, f_nt;
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_ELSE);

    if (data->if_stack_ptr > 0) {
        r = data->if_stack[data->if_stack_ptr - 1];
    } else {
        token_error_print(data, TOKENIZER_ELSE);
        data->status.bit.isRunning = 0;
        data->status.bit.Error = 1;
        return;
    }
    if (tokenizer_token(tree) == TOKENIZER_EOL) {
        accept(data, TOKENIZER_EOL);
        if (!r) {
            return;
        } else {
            endif_cntr = 0;
            while (((tokenizer_token(tree) != TOKENIZER_ENDIF) || endif_cntr) &&
                   tokenizer_token(tree) != TOKENIZER_ENDOFINPUT) {
                f_nt = 0;
                if (tokenizer_token(tree) == TOKENIZER_IF) {
                    endif_cntr += 1;
                }
                if (tokenizer_token(tree) == TOKENIZER_THEN) {
                    tokenizer_next(tree);
                    // then followed by CR -> multi line
                    if (tokenizer_token(tree) == TOKENIZER_EOL) {
                        f_nt = 1;
                    } else { // single line
                        endif_cntr--;
                        while (tokenizer_token(tree) != TOKENIZER_ENDIF &&
                               tokenizer_token(tree) != TOKENIZER_EOL &&
                               tokenizer_token(tree) != TOKENIZER_ENDOFINPUT) {
                            tokenizer_next(tree);
                        }
                        if (tokenizer_token(tree) == TOKENIZER_ENDIF) {
                            token_error_print(data, TOKENIZER_ELSE);
                            data->status.bit.isRunning = 0;
                            data->status.bit.Error = 1;
                            return;
                        }
                    }
                }
                if (tokenizer_token(tree) == TOKENIZER_ENDIF) {
                    endif_cntr--;
                }
                if (!f_nt) {
                    tokenizer_next(tree);
                }
            }
        }
        endif_statement(data);
        return;
    }
    token_error_print(data, TOKENIZER_ELSE);
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 1;
    return;
}

/*---------------------------------------------------------------------------*/
static void let_statement(struct ubasic_data *data)
{
    uint8_t varnum;
    struct tokenizer_data *tree = &data->tree;

    if (tokenizer_token(tree) == TOKENIZER_VARIABLE) {
        varnum = tokenizer_variable_num(tree);
        accept(data, TOKENIZER_VARIABLE);
        if (!accept(data, TOKENIZER_EQ)) {
            ubasic_set_varnum(data, varnum, relation(data));
        }
    }
#if defined(VARIABLE_TYPE_STRING)
    // string additions here
    else if (tokenizer_token(tree) == TOKENIZER_STRINGVARIABLE) {
        varnum = tokenizer_variable_num(tree);
        accept(data, TOKENIZER_STRINGVARIABLE);
        if (!accept(data, TOKENIZER_EQ)) {
            ubasic_set_stringvariable(data, varnum, sexpr(data));
        }
    }
    // end of string additions
#endif
#if defined(VARIABLE_TYPE_ARRAY)
    else if (tokenizer_token(tree) == TOKENIZER_ARRAYVARIABLE) {
        varnum = tokenizer_variable_num(tree);
        accept(data, TOKENIZER_ARRAYVARIABLE);
        accept(data, TOKENIZER_LEFTPAREN);
        VARIABLE_TYPE idx = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
        idx = fixedpt_toint(idx);
#endif
        accept(data, TOKENIZER_RIGHTPAREN);
        if (!accept(data, TOKENIZER_EQ)) {
            ubasic_set_arrayvariable(
                data, varnum, (uint16_t)idx, relation(data));
        }
    }
#endif

    accept_cr(tree);
}

#if defined(VARIABLE_TYPE_ARRAY)
static void dim_statement(struct ubasic_data *data)
{
    VARIABLE_TYPE size = 0;
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_DIM);
    uint8_t varnum = tokenizer_variable_num(tree);
    accept(data, TOKENIZER_ARRAYVARIABLE);

    //   accept(data, TOKENIZER_LEFTPAREN);
    size = relation(data);

#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    size = fixedpt_toint(size);
#endif

    ubasic_dim_arrayvariable(data, varnum, size);

    //   accept(data, TOKENIZER_RIGHTPAREN);
    accept_cr(tree);

    // end of array additions
}
#endif

/*---------------------------------------------------------------------------*/
static void next_statement(struct ubasic_data *data)
{
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_NEXT);
    uint8_t var = tokenizer_variable_num(tree);
    accept(data, TOKENIZER_VARIABLE);
    if (data->for_stack_ptr > 0 &&
        var == data->for_stack[data->for_stack_ptr - 1].for_variable) {
        VARIABLE_TYPE value = ubasic_get_varnum(data, var) +
            data->for_stack[data->for_stack_ptr - 1].step;
        ubasic_set_varnum(data, var, value);

        if (((data->for_stack[data->for_stack_ptr - 1].step > 0) &&
             (value <= data->for_stack[data->for_stack_ptr - 1].to)) ||
            ((data->for_stack[data->for_stack_ptr - 1].step < 0) &&
             (value >= data->for_stack[data->for_stack_ptr - 1].to))) {
            // jump_line(for_stack[for_stack_ptr - 1].line_after_for);
            tokenizer_jump_offset(
                tree, data->for_stack[data->for_stack_ptr - 1].line_after_for);
        } else {
            data->for_stack_ptr--;
            accept_cr(tree);
        }
        return;
    }

    token_error_print(data, TOKENIZER_FOR);
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 1;
}

/*---------------------------------------------------------------------------*/

static void for_statement(struct ubasic_data *data)
{
    uint8_t for_variable;
    VARIABLE_TYPE to;
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_FOR);
    for_variable = tokenizer_variable_num(tree);
    accept(data, TOKENIZER_VARIABLE);
    accept(data, TOKENIZER_EQ);
    ubasic_set_varnum(data, for_variable, relation(data));
    accept(data, TOKENIZER_TO);
    to = relation(data);

#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    VARIABLE_TYPE step = FIXEDPT_ONE;
#else
    VARIABLE_TYPE step = 1;
#endif
    if (tokenizer_token(tree) == TOKENIZER_STEP) {
        accept(data, TOKENIZER_STEP);
        step = relation(data);
    }
    accept_cr(tree);

    if (data->for_stack_ptr < MAX_FOR_STACK_DEPTH) {
        // for_stack[for_stack_ptr].line_after_for = tokenizer_line_number();
        data->for_stack[data->for_stack_ptr].line_after_for =
            tokenizer_save_offset(tree);
        data->for_stack[data->for_stack_ptr].for_variable = for_variable;
        data->for_stack[data->for_stack_ptr].to = to;
        data->for_stack[data->for_stack_ptr].step = step;
        data->for_stack_ptr++;
        return;
    }

    token_error_print(data, TOKENIZER_FOR);
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 1;
}

/*---------------------------------------------------------------------------*/

static void end_statement(struct ubasic_data *data)
{
    accept(data, TOKENIZER_END);
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 0;
}

#if defined(UBASIC_SCRIPT_HAVE_SLEEP)
static void sleep_statement(struct ubasic_data *data)
{
    VARIABLE_TYPE r;
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_SLEEP);
    VARIABLE_TYPE f = relation(data);
    if (f > 0) {
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
        r = fixedpt_toint(f * 1000);
#else
        r = (uint32_t)f;
#endif
    } else {
        r = 0;
    }
    mstimer_sleep(data, r);

    accept_cr(tree);
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS)
static void tic_statement(struct ubasic_data *data)
{
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_TIC);
    accept(data, TOKENIZER_LEFTPAREN);
    VARIABLE_TYPE f = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    f = fixedpt_toint(f);
#endif
    timer_tic(data, f);
    accept(data, TOKENIZER_RIGHTPAREN);
    accept_cr(tree);
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
static void input_statement_wait(struct ubasic_data *data)
{
    struct tokenizer_data *tree = &data->tree;

    data->input_how = 0;
    accept(data, TOKENIZER_INPUT);
    if (tokenizer_token(tree) == TOKENIZER_PRINT_HEX) {
        tokenizer_next(tree);
        data->input_how = 1;
    } else if (tokenizer_token(tree) == TOKENIZER_PRINT_DEC) {
        tokenizer_next(tree);
        data->input_how = 2;
    }

    if (tokenizer_token(tree) == TOKENIZER_VARIABLE) {
        data->input_varnum = tokenizer_variable_num(tree);
        accept(data, TOKENIZER_VARIABLE);
        data->input_type = 0;
    }
#if defined(VARIABLE_TYPE_STRING)
    // string additions here
    else if (tokenizer_token(tree) == TOKENIZER_STRINGVARIABLE) {
        data->input_varnum = tokenizer_variable_num(tree);
        accept(data, TOKENIZER_STRINGVARIABLE);
        data->input_type = 1;
    }
// end of string additions
#endif
#if defined(VARIABLE_TYPE_ARRAY)
    else if (tokenizer_token(tree) == TOKENIZER_ARRAYVARIABLE) {
        data->input_varnum = tokenizer_variable_num(tree);
        accept(data, TOKENIZER_ARRAYVARIABLE);

        accept(data, TOKENIZER_LEFTPAREN);
        data->input_array_index = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
        data->input_array_index = fixedpt_toint(data->input_array_index);
#endif
        accept(data, TOKENIZER_RIGHTPAREN);
        data->input_type = 2;
    }
#endif

    // get next token:
    //    CR
    // or
    //    , timeout
    if (tokenizer_token(tree) == TOKENIZER_COMMA) {
        accept(data, TOKENIZER_COMMA);
        VARIABLE_TYPE r = relation(data);
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
        r = fixedpt_toint(r);
#endif
        if (r > 0) {
            mstimer_input_wait(data, r);
        }
    }

    accept_cr(tree);

    data->status.bit.WaitForSerialInput = 1;
}

static void serial_getline_completed(struct ubasic_data *data)
{
    // process if something has been received.
    // otherwise leave the variable content unchanged.
    if (strlen(data->statement) > 0) {
        if ((data->input_type == 0)
#if defined(VARIABLE_TYPE_ARRAY)
            || (data->input_type == 2)
#endif
        ) {
            VARIABLE_TYPE r;
            if ((data->input_how == 1) || (data->input_how == 2)) {
                r = atoi(data->statement);
            } else {
                // process number
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
                r = str_fixedpt(
                    data->statement, MAX_STRINGLEN, FIXEDPT_FBITS >> 1);
#else
                r = atoi(data->statement);
#endif
            }

            if (data->input_type == 0) {
                ubasic_set_varnum(data, data->input_varnum, r);
            }
#if defined(VARIABLE_TYPE_ARRAY)
            else if (data->input_type == 2) {
                ubasic_set_arrayvariable(
                    data, data->input_varnum, data->input_array_index, r);
            }
#endif
        }
#if defined(VARIABLE_TYPE_STRING)
        else if (data->input_type == 1) {
            ubasic_set_stringvariable(
                data, data->input_varnum, scpy(data, data->statement));
        }
#endif
    }
    memset(data->statement, 0, sizeof(data->statement));
    data->status.bit.WaitForSerialInput = 0;
}
#endif

/*---------------------------------------------------------------------------*/
static void while_statement(struct ubasic_data *data)
{
    VARIABLE_TYPE r;
    int8_t while_cntr;
    uint16_t while_offset;
    struct tokenizer_data *tree = &data->tree;

    // this is where we jump to after 'endwhile'
    while_offset = tokenizer_save_offset(tree);
    accept(data, TOKENIZER_WHILE);
    if (data->while_stack_ptr == MAX_WHILE_STACK_DEPTH) {
        token_error_print(data, TOKENIZER_WHILE);
        data->status.bit.isRunning = 0;
        data->status.bit.Error = 1;
    }
    // this makes sure that the jump to the same while is ignored
    if ((data->while_stack_ptr == 0) ||
        ((data->while_stack_ptr > 0) &&
         (data->while_stack[data->while_stack_ptr - 1].line_while !=
          while_offset))) {
        data->while_stack[data->while_stack_ptr].line_while = while_offset;
        // we don't know it yet
        data->while_stack[data->while_stack_ptr].line_after_endwhile = -1;
        data->while_stack_ptr++;
    }

    r = relation(data);

    if (data->while_stack_ptr == 0) {
        token_error_print(data, TOKENIZER_WHILE);
        data->status.bit.isRunning = 0;
        data->status.bit.Error = 1;
        return;
    }

    if (r) {
        accept_cr(tree);
        return;
    }

    if (data->while_stack[data->while_stack_ptr - 1].line_after_endwhile > 0) {
        // we have traversed while loop once to its end already.
        // thus we know where the loop ends. we just use that to jump there.
        tokenizer_jump_offset(
            tree,
            data->while_stack[data->while_stack_ptr - 1].line_after_endwhile);
    } else {
        // first time the loop is entered the condition is not satisfied,
        // so we gobble the lines until we reach the matching endwhile
        while_cntr = 0;
        while ((tokenizer_token(tree) != TOKENIZER_ENDWHILE || while_cntr) &&
               (tokenizer_token(tree) != TOKENIZER_ENDOFINPUT)) {
            if (tokenizer_token(tree) == TOKENIZER_WHILE) {
                while_cntr += 1;
            }
            if (tokenizer_token(tree) == TOKENIZER_ENDWHILE) {
                while_cntr -= 1;
            }
            tokenizer_next(tree);
        }
        data->while_stack_ptr--;
        accept(data, TOKENIZER_ENDWHILE);
        accept(data, TOKENIZER_EOL);
    }

    return;
}
/*---------------------------------------------------------------------------*/
static void endwhile_statement(struct ubasic_data *data)
{
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_ENDWHILE);
    if (data->while_stack_ptr > 0) {
        // jump_line(while_stack[while_stack_ptr-1]);
        if (data->while_stack[data->while_stack_ptr - 1].line_after_endwhile ==
            -1) {
            data->while_stack[data->while_stack_ptr - 1].line_after_endwhile =
                tokenizer_save_offset(tree);
        }
        tokenizer_jump_offset(
            tree, data->while_stack[data->while_stack_ptr - 1].line_while);
        return;
    }
    token_error_print(data, TOKENIZER_FOR);
    data->status.bit.isRunning = 0;
    data->status.bit.Error = 1;
}
/*---------------------------------------------------------------------------*/

#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)

static VARIABLE_TYPE recall_statement(struct ubasic_data *data)
{
    VARIABLE_TYPE rval = 0;
    uint8_t *dataptr;
    uint8_t *datalen;
    uint8_t var_type;
    struct tokenizer_data *tree = &data->tree;

    accept(data, TOKENIZER_RECALL);
    accept(data, TOKENIZER_LEFTPAREN);
    if (tokenizer_token(tree) == TOKENIZER_VARIABLE) {
        var_type = UBASIC_RECALL_STORE_TYPE_VARIABLE;
        data->varnum = tokenizer_variable_num(tree);
        accept(data, TOKENIZER_VARIABLE);
        dataptr = (uint8_t *)&data->variables[data->varnum];
        datalen = (uint8_t *)&rval;
        flash_read(data, data->varnum, var_type, dataptr, datalen);
        rval >>= 2;
    }
#if defined(VARIABLE_TYPE_STRING)
    else if (tokenizer_token(tree) == TOKENIZER_STRINGVARIABLE) {
        var_type = UBASIC_RECALL_STORE_TYPE_STRING;
        data->varnum = tokenizer_variable_num(tree);
        accept(data, TOKENIZER_STRINGVARIABLE);
        char dummy_s[MAX_STRINGLEN] = { 0 };
        dataptr = (uint8_t *)dummy_s;
        datalen = (uint8_t *)&rval;
        flash_read(data, data->varnum, var_type, dataptr, datalen);
        if (rval > 0) {
            ubasic_set_stringvariable(
                data, data->varnum, scpy(data, (char *)dummy_s));
        }

        clear_stringstack(data);
    }
#endif
#if defined(VARIABLE_TYPE_ARRAY)
    else if (tokenizer_token(tree) == TOKENIZER_ARRAYVARIABLE) {
        var_type = UBASIC_RECALL_STORE_TYPE_ARRAY;
        data->varnum = tokenizer_variable_num(tree);
        accept(data, TOKENIZER_ARRAYVARIABLE);
        VARIABLE_TYPE dummy_a[VARIABLE_TYPE_ARRAY + 1];
        dataptr = (uint8_t *)dummy_a;
        datalen = (uint8_t *)&rval;
        flash_read(data, data->varnum, 2, dataptr, datalen);
        if (rval > 0) {
            rval >>= 2;
            ubasic_dim_arrayvariable(data, data->varnum, rval);
            for (uint8_t i = 0; i < rval; i++) {
                ubasic_set_arrayvariable(data, data->varnum, i + 1, dummy_a[i]);
            }
        }
    }
#endif
    accept(data, TOKENIZER_RIGHTPAREN);
    return rval;
}

static void store_statement(struct ubasic_data *data)
{
    struct tokenizer_data *tree = &data->tree;
    uint8_t *dataptr;
    uint8_t datalen;
    uint8_t var_type;

    accept(data, TOKENIZER_STORE);
    accept(data, TOKENIZER_LEFTPAREN);

    if (tokenizer_token(tree) == TOKENIZER_VARIABLE) {
        var_type = UBASIC_RECALL_STORE_TYPE_VARIABLE;
        data->varnum = tokenizer_variable_num(tree);
        accept(data, TOKENIZER_VARIABLE);
        dataptr = (uint8_t *)&data->variables[data->varnum];
        flash_write(data, data->varnum, var_type, 4, dataptr);
    }
#if defined(VARIABLE_TYPE_STRING)
    else if (tokenizer_token(tree) == TOKENIZER_STRINGVARIABLE) {
        var_type = UBASIC_RECALL_STORE_TYPE_STRING;
        data->varnum = tokenizer_variable_num(tree);
        accept(data, TOKENIZER_STRINGVARIABLE);
        dataptr = (uint8_t *)strptr(data, data->stringvariables[data->varnum]);
        datalen = strlen((char *)dataptr);
        flash_write(data, data->varnum, var_type, datalen, dataptr);
    }
#endif
#if defined(VARIABLE_TYPE_ARRAY)
    else if (tokenizer_token(tree) == TOKENIZER_ARRAYVARIABLE) {
        var_type = UBASIC_RECALL_STORE_TYPE_ARRAY;
        data->varnum = tokenizer_variable_num(tree);
        accept(data, TOKENIZER_ARRAYVARIABLE);
        datalen = 4 *
            (data->arrays_data[data->arrayvariable[data->varnum]] & 0x0000ffff);
        dataptr =
            (uint8_t *)&data->arrays_data[data->arrayvariable[data->varnum]];
        flash_write(data, data->varnum, var_type, datalen, dataptr);
    }
#endif
    accept(data, TOKENIZER_RIGHTPAREN);
    accept_cr(tree);
}
/*---------------------------------------------------------------------------*/
#endif

/*---------------------------------------------------------------------------*/
static void statement(struct ubasic_data *data)
{
    struct tokenizer_data *tree = &data->tree;
    uint8_t println = 0;
    VARIABLE_TYPE token;

    if (data->status.bit.Error) {
        return;
    }
    token = tokenizer_token(tree);
    switch (token) {
        case TOKENIZER_EOL:
            accept(data, TOKENIZER_EOL);
            break;

        case TOKENIZER_PRINTLN:
            println = 1;
        case TOKENIZER_PRINT:
            print_statement(data, println);
            break;

        case TOKENIZER_IF:
            if_statement(data);
            break;

        case TOKENIZER_ELSE:
            else_statement(data);
            break;

        case TOKENIZER_ENDIF:
            endif_statement(data);
            break;

        case TOKENIZER_GOTO:
            goto_statement(data);
            break;

        case TOKENIZER_GOSUB:
            gosub_statement(data);
            break;

        case TOKENIZER_RETURN:
            return_statement(data);
            break;

        case TOKENIZER_FOR:
            for_statement(data);
            break;

        case TOKENIZER_NEXT:
            next_statement(data);
            break;

        case TOKENIZER_WHILE:
            while_statement(data);
            break;

        case TOKENIZER_ENDWHILE:
            endwhile_statement(data);
            break;

        case TOKENIZER_END:
            end_statement(data);
            break;

        case TOKENIZER_LET:
            accept(data, TOKENIZER_LET); /* Fall through: Nothing to do! */
        case TOKENIZER_VARIABLE:
#if defined(VARIABLE_TYPE_STRING)
        // string addition
        case TOKENIZER_STRINGVARIABLE:
            // end of string addition
#endif
#if defined(VARIABLE_TYPE_ARRAY)
        case TOKENIZER_ARRAYVARIABLE:
#endif
            let_statement(data);
            break;

#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
        case TOKENIZER_INPUT:
            input_statement_wait(data);
            break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_SLEEP)
        case TOKENIZER_SLEEP:
            sleep_statement(data);
            break;
#endif

#if defined(VARIABLE_TYPE_ARRAY)
        case TOKENIZER_DIM:
            dim_statement(data);
            break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS)
        case TOKENIZER_TIC:
            tic_statement(data);
            break;
#endif

#ifdef UBASIC_SCRIPT_HAVE_PWM_CHANNELS
        case TOKENIZER_PWM:
            pwm_statement(data);
            break;

        case TOKENIZER_PWMCONF:
            pwmconf_statement(data);
            break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_ANALOG_READ)
        case TOKENIZER_AREADCONF:
            areadconf_statement(data);
            break;
#endif

#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
        case TOKENIZER_PINMODE:
            pinmode_statement(data);
            break;

        case TOKENIZER_DWRITE:
            dwrite_statemet(data);
            break;
#endif /* UBASIC_SCRIPT_HAVE_GPIO_CHANNELS */

#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
        case TOKENIZER_STORE:
            store_statement(data);
            break;

        case TOKENIZER_RECALL:
            recall_statement(data);
            break;
#endif /* UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH */

        case TOKENIZER_CLEAR:
            ubasic_clear_variables(data);
            accept_cr(tree);
            break;

        default:
            token_error_print(data, token);
            data->status.bit.isRunning = 0;
            data->status.bit.Error = 1;
    }
}
/*---------------------------------------------------------------------------*/
static void numbered_line_statement(struct ubasic_data *data)
{
    struct tokenizer_data *tree = &data->tree;

    while (1) {
        if (tokenizer_token(tree) == TOKENIZER_COLON) {
            accept(data, TOKENIZER_COLON);
            if (accept(data, TOKENIZER_LABEL)) {
                return;
            }
            continue;
        }
        break;
    }
    statement(data);

    return;
}
/*---------------------------------------------------------------------------*/
static bool ubasic_program_finished(struct ubasic_data *data)
{
    struct tokenizer_data *tree = &data->tree;

    if (data->status.bit.isRunning) {
        return tokenizer_finished(tree);
    }

    return (tokenizer_finished(tree) || (data->status.bit.Error));
}

/*---------------------------------------------------------------------------*/
void ubasic_run_program(struct ubasic_data *data)
{
    if (data->status.bit.isRunning == 0) {
        return;
    }
    if (data->status.bit.Error == 1) {
        return;
    }
#if defined(UBASIC_SCRIPT_HAVE_SLEEP)
    if (mstimer_sleeping(data) > 0) {
        return;
    }
#endif
#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
    if (data->status.bit.WaitForSerialInput) {
        if (!ubasic_getline(data, ubasic_getc(data))) {
            if (mstimer_input_remaining(data) > 0) {
                return;
            }
        }
        serial_getline_completed(data);
    }
#endif
#if defined(VARIABLE_TYPE_STRING)
    // string additions
    clear_stringstack(data);
    // end of string additions
#endif
    if (ubasic_program_finished(data)) {
        return;
    }
    numbered_line_statement(data);
}

/*---------------------------------------------------------------------------*/
uint8_t ubasic_execute_statement(struct ubasic_data *data, char *stmt)
{
    struct tokenizer_data *tree = &data->tree;
    data->status.byte = 0;

    data->program_ptr = stmt;
    data->for_stack_ptr = data->gosub_stack_ptr = 0;
    tokenizer_init(tree, stmt);
    do {
#if defined(VARIABLE_TYPE_STRING)
        clear_stringstack(data);
#endif

        statement(data);

        if (data->status.bit.Error) {
            break;
        }

#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
        while (data->status.bit.WaitForSerialInput) {
            if (!ubasic_getline(data, ubasic_getc(data))) {
                if (mstimer_input_remaining(data) > 0) {
                    continue;
                }
            }
            serial_getline_completed(data);
        }
#endif

#if defined(UBASIC_SCRIPT_HAVE_SLEEP)
        while (mstimer_sleeping(data) > 0) {
            /* FIXME: maybe just a return until the sleep is over? */
        }
#endif

    } while (!tokenizer_finished(tree));

    return data->status.byte;
}

/*---------------------------------------------------------------------------*/
uint8_t ubasic_waiting_for_input(struct ubasic_data *data)
{
    return (data->status.bit.WaitForSerialInput);
}

/**
 * @brief Append a character to a line of text, if there is space
 * @param buffer - buffer that recieves the appended character
 * @param buffer_len - sizeof the buffer
 * @param ch - character to append
 * @return 1 if character was appended, 0 if not
 */
static int line_append_char(char *buffer, unsigned buffer_len, char ch)
{
    unsigned len = 0;
    bool status = false;

    len = strlen(buffer);
    if (len < (buffer_len - 1)) {
        buffer[len] = ch;
        buffer[len + 1] = 0;
        status = true;
    }

    return status;
}

/**
 * @brief Remove the trailing character from a line of text
 * @param buffer - buffer to remove the trailing character
 * @param buffer_len - sizeof the buffer
 * @return 1 if character was removed, 0 if not
 */
static int line_remove_char(char *buffer, size_t buffer_len)
{
    unsigned len = 0;
    int status = 0;

    len = strlen(buffer);
    if ((len > 0) && (len < (buffer_len - 1))) {
        buffer[len - 1] = 0;
        status = 1;
    }

    return status;
}

/**
 * @brief Non-blocking serial getline task
 * @param data - ubasic data structure
 * @return 1 if statement is complete and ready to process, 0 if not
 */
uint8_t ubasic_getline(struct ubasic_data *data, int ch)
{
    uint8_t eol = 0;

    if (ch == EOF) {
        return 0;
    }
    switch (ch) {
        case '\a':
        case '\f':
        case '\t':
        case '\r':
        case '\v':
            /* ignored characters */
            break;
        case 0x1B:
            /* escape */
            /* clear buffer */
            data->statement[0] = 0;
            eol = 1;
            break;
        case '\b':
            /* console backspace */
            line_remove_char(data->statement, sizeof(data->statement));
            break;
        case 0x7F:
            /* DEL */
            line_remove_char(data->statement, sizeof(data->statement));
            break;
        case '\n':
            /* enter */
            eol = 1;
            break;
        default:
            /* all the rest of the characters */
            if (line_append_char(
                    data->statement, sizeof(data->statement), ch)) {
            } else {
                eol = 1;
            }
            break;
    }

    return eol;
}

uint8_t ubasic_finished(struct ubasic_data *data)
{
    return (ubasic_program_finished(data) || data->status.bit.isRunning == 0);
}

/*---------------------------------------------------------------------------*/

void ubasic_set_varnum(
    struct ubasic_data *data, uint8_t varnum, VARIABLE_TYPE value)
{
    if (varnum < MAX_VARNUM) {
        data->variables[varnum] = value;
    }
}

void ubasic_set_variable(
    struct ubasic_data *data, char variable, VARIABLE_TYPE value)
{
    if (isupper(variable)) {
        uint8_t varnum = variable - 'A';
        ubasic_set_varnum(data, varnum, value);
    } else if (islower(variable)) {
        uint8_t varnum = variable - 'a';
        ubasic_set_varnum(data, varnum, value);
    }
}

/*---------------------------------------------------------------------------*/

VARIABLE_TYPE ubasic_get_varnum(struct ubasic_data *data, uint8_t varnum)
{
    if (varnum < MAX_VARNUM) {
        return data->variables[varnum];
    }
    return 0;
}

VARIABLE_TYPE ubasic_get_variable(struct ubasic_data *data, char variable)
{
    if (isupper(variable)) {
        uint8_t varnum = variable - 'A';
        return ubasic_get_varnum(data, varnum);
    } else if (islower(variable)) {
        uint8_t varnum = variable - 'a';
        return ubasic_get_varnum(data, varnum);
    }
    return 0;
}

#if defined(VARIABLE_TYPE_STRING)
//
// string additions
//
/*---------------------------------------------------------------------------*/
void ubasic_set_stringvariable(
    struct ubasic_data *data, uint8_t svarnum, int16_t svalue)
{
    if (svarnum < MAX_SVARNUM) {
        // was it previously allocated?
        if (data->stringvariables[svarnum] > -1) {
            *(data->stringstack + data->stringvariables[svarnum]) = 0;
        }

        data->stringvariables[svarnum] = svalue;
        if (svalue > -1) {
            *(data->stringstack + svalue) = svarnum + 1;
        }
#if defined(UBASIC_DEBUG_STRINGVARIABLES)
        serial_write_string(data, "set_stringvar:");
        char msg[12];
        sprintf(msg, "[%d]", stringvariables[svarnum]);
        serial_write_string(data, msg);
        serial_write_string(data, strptr(stringvariables[svarnum]));
        serial_write_string(data, "\n");
#endif
    }
}

/*---------------------------------------------------------------------------*/

int16_t ubasic_get_stringvariable(struct ubasic_data *data, uint8_t varnum)
{
    if (varnum < MAX_SVARNUM) {
#if defined(UBASIC_DEBUG_STRINGVARIABLES)
        serial_write_string(data, "get_stringvar:");
        char msg[12];
        sprintf(msg, "[%d]", stringvariables[varnum]);
        serial_write_string(data, msg);
        serial_write_string(data, strptr(stringvariables[varnum]));
        serial_write_string(data, "\n");
#endif

        return data->stringvariables[varnum];
    }

    return (-1);
}
//
// end of string additions
//
#endif

#if defined(VARIABLE_TYPE_ARRAY)
//
// array additions: works only for VARIABLE_TYPE 32bit
//  array storage:
//    1st entry:   [ 31:16 , 15:0]
//                  varnum   size
//    entries 2 through size+1 are the array elements
//  could work for 16bit values as well
/*---------------------------------------------------------------------------*/
void ubasic_dim_arrayvariable(
    struct ubasic_data *data, uint8_t varnum, int16_t newsize)
{
    int16_t oldsize;
    int16_t current_location;

    if (varnum >= MAX_VARNUM) {
        return;
    }
_attach_at_the_end:
    current_location = data->arrayvariable[varnum];
    if (current_location == -1) {
        /* does the array fit in the available memory? */
        if ((data->free_arrayptr + newsize + 1) < VARIABLE_TYPE_ARRAY) {
            current_location = data->free_arrayptr;
            data->arrayvariable[varnum] = current_location;
            data->arrays_data[current_location] = (varnum << 16) | newsize;
            data->free_arrayptr += newsize + 1;
            return;
        }
        return; /* failed to allocate*/
    } else {
        oldsize = data->arrays_data[current_location];
    }
    /* if size of the array is the same as earlier allocated then do nothing */
    if (oldsize == newsize) {
        return;
    }
    /* if this is the last array in arrays_data, just modify the boundary */
    if (current_location + oldsize + 1 == data->free_arrayptr) {
        if ((data->free_arrayptr - current_location + newsize) <
            VARIABLE_TYPE_ARRAY) {
            data->arrays_data[current_location] = (varnum << 16) | newsize;
            data->free_arrayptr += (newsize - oldsize);
            data->arrays_data[data->free_arrayptr] = 0;
            return;
        }
        /* failed to allocate memory */
        data->arrayvariable[varnum] = -1;
        return;
    }
    /* Array has been allocated before. It is not the last array */
    /* Thus we have to go over all arrays above the current location, and shift
     * them down */
    data->arrayvariable[varnum] = -1;
    int16_t next_location;
    uint16_t mov_size, mov_varnum;
    next_location = current_location + oldsize + 1;
    do {
        mov_varnum = (data->arrays_data[next_location] >> 16);
        mov_size = data->arrays_data[next_location];

        for (uint8_t i = 0; i <= mov_size; i++) {
            data->arrays_data[current_location + i] =
                data->arrays_data[next_location + i];
            data->arrays_data[next_location + i] = 0;
        }
        data->arrayvariable[mov_varnum] = current_location;
        next_location = next_location + mov_size + 1;
        current_location = current_location + mov_size + 1;
        data->arrays_data[current_location] = 0;
    } while (data->arrays_data[next_location] > 0);
    data->free_arrayptr = current_location;
    /** now the array should be added to the end of the list:
        if there is space do it! */
    goto _attach_at_the_end;
}

void ubasic_set_arrayvariable(
    struct ubasic_data *data, uint8_t varnum, uint16_t idx, VARIABLE_TYPE value)
{
    int16_t array;

    if (varnum >= MAX_VARNUM) {
        return;
    }
    array = data->arrayvariable[varnum];
    if (array < 0) {
        return;
    }
    if (array >= VARIABLE_TYPE_ARRAY) {
        return;
    }
    uint16_t size = (uint16_t)data->arrays_data[array];
    if ((size < idx) || (idx < 1)) {
        return;
    }

    data->arrays_data[array + idx] = value;
}

VARIABLE_TYPE
ubasic_get_arrayvariable(struct ubasic_data *data, uint8_t varnum, uint16_t idx)
{
    int16_t array;

    if (varnum >= MAX_VARNUM) {
        return -1;
    }
    array = data->arrayvariable[varnum];
    if (array < 0) {
        return -1;
    }
    if (array >= VARIABLE_TYPE_ARRAY) {
        return -1;
    }
    uint16_t size = (uint16_t)data->arrays_data[array];
    if ((size < idx) || (idx < 1)) {
        return -1;
    }
    return (VARIABLE_TYPE)data->arrays_data[array + idx];
}
#endif
/*---------------------------------------------------------------------------*/
