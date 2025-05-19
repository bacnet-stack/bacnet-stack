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
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"
#include "tokenizer.h"

struct keyword_token {
    const char *keyword;
    uint8_t token;
};

static const struct keyword_token keywords[] = {
#if defined(UBASIC_VARIABLE_TYPE_STRING)
    /* new string-related statements and functions */
    { "left$", UBASIC_TOKENIZER_LEFT_STR },
    { "right$", UBASIC_TOKENIZER_RIGHT_STR },
    { "mid$", UBASIC_TOKENIZER_MID_STR },
    { "str$", UBASIC_TOKENIZER_STR_STR },
    { "chr$", UBASIC_TOKENIZER_CHR_STR },
    { "val", UBASIC_TOKENIZER_VAL },
    { "len", UBASIC_TOKENIZER_LEN },
    { "instr", UBASIC_TOKENIZER_INSTR },
    { "asc", UBASIC_TOKENIZER_ASC },
#endif
    /* end of string additions */
    { "let ", UBASIC_TOKENIZER_LET },
    { "println ", UBASIC_TOKENIZER_PRINTLN },
    { "print ", UBASIC_TOKENIZER_PRINT },
    { "if", UBASIC_TOKENIZER_IF },
    { "then", UBASIC_TOKENIZER_THEN },
    { "else", UBASIC_TOKENIZER_ELSE },
    { "endif", UBASIC_TOKENIZER_ENDIF },
#if defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS)
    { "toc", UBASIC_TOKENIZER_TOC },
#endif
#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
    { "input", UBASIC_TOKENIZER_INPUT },
#endif
    { "for ", UBASIC_TOKENIZER_FOR },
    { "to ", UBASIC_TOKENIZER_TO },
    { "next ", UBASIC_TOKENIZER_NEXT },
    { "step ", UBASIC_TOKENIZER_STEP },
    { "while", UBASIC_TOKENIZER_WHILE },
    { "endwhile", UBASIC_TOKENIZER_ENDWHILE },
    { "goto ", UBASIC_TOKENIZER_GOTO },
    { "gosub ", UBASIC_TOKENIZER_GOSUB },
    { "return", UBASIC_TOKENIZER_RETURN },
    { "end", UBASIC_TOKENIZER_END },
#if defined(UBASIC_SCRIPT_HAVE_SLEEP)
    { "sleep", UBASIC_TOKENIZER_SLEEP },
#endif
#if defined(UBASIC_VARIABLE_TYPE_ARRAY)
    { "dim ", UBASIC_TOKENIZER_DIM },
#endif
#if defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS)
    { "tic", UBASIC_TOKENIZER_TIC },
#endif
#if defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS)
    { "flag", UBASIC_TOKENIZER_HWE },
#endif
#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
    { "ran", UBASIC_TOKENIZER_RAN },
#endif
#if defined(UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    { "sqrt", UBASIC_TOKENIZER_SQRT },
    { "sin", UBASIC_TOKENIZER_SIN },
    { "cos", UBASIC_TOKENIZER_COS },
    { "tan", UBASIC_TOKENIZER_TAN },
    { "exp", UBASIC_TOKENIZER_EXP },
    { "ln", UBASIC_TOKENIZER_LN },
#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
    { "uniform", UBASIC_TOKENIZER_UNIFORM },
#endif
    { "abs", UBASIC_TOKENIZER_ABS },
    { "floor", UBASIC_TOKENIZER_FLOOR },
    { "ceil", UBASIC_TOKENIZER_CEIL },
    { "round", UBASIC_TOKENIZER_ROUND },
    { "pow", UBASIC_TOKENIZER_POWER },
    { "avgw", UBASIC_TOKENIZER_AVERAGEW },
#endif
#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
    { "pinmode", UBASIC_TOKENIZER_PINMODE },
    { "dread", UBASIC_TOKENIZER_DREAD },
    { "dwrite", UBASIC_TOKENIZER_DWRITE },
#endif
#ifdef UBASIC_SCRIPT_HAVE_PWM_CHANNELS
    { "awrite_conf", UBASIC_TOKENIZER_PWMCONF },
    { "awrite", UBASIC_TOKENIZER_PWM },
#endif
#if defined(UBASIC_SCRIPT_HAVE_ANALOG_READ)
    { "aread_conf", UBASIC_TOKENIZER_AREADCONF },
    { "aread", UBASIC_TOKENIZER_AREAD },
#endif
    { "hex ", UBASIC_TOKENIZER_PRINT_HEX },
    { "dec ", UBASIC_TOKENIZER_PRINT_DEC },
    { ":", UBASIC_TOKENIZER_COLON },
#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
    { "store", UBASIC_TOKENIZER_STORE },
    { "recall", UBASIC_TOKENIZER_RECALL },
#endif
#if defined(UBASIC_SCRIPT_HAVE_BACNET)
    { "bac_create", UBASIC_TOKENIZER_BACNET_CREATE_OBJECT },
    { "bac_read", UBASIC_TOKENIZER_BACNET_READ_PROPERTY },
    { "bac_write", UBASIC_TOKENIZER_BACNET_WRITE_PROPERTY },
#endif
    { "clear", UBASIC_TOKENIZER_CLEAR },
    { NULL, UBASIC_TOKENIZER_ERROR }
};

/*---------------------------------------------------------------------------*/
static uint8_t
singlechar_or_operator(struct ubasic_tokenizer *tree, uint8_t *offset)
{
    if (offset) {
        *offset = 1;
    }

    if ((*tree->ptr == '\n') || (*tree->ptr == ';')) {
        return UBASIC_TOKENIZER_EOL;
    } else if (*tree->ptr == ',') {
        return UBASIC_TOKENIZER_COMMA;
    } else if (*tree->ptr == '+') {
        return UBASIC_TOKENIZER_PLUS;
    } else if (*tree->ptr == '-') {
        return UBASIC_TOKENIZER_MINUS;
    } else if (*tree->ptr == '&') {
        if (*(tree->ptr + 1) == '&') {
            if (offset) {
                *offset += 1;
            }
            return UBASIC_TOKENIZER_LAND;
        }
        return UBASIC_TOKENIZER_AND;
    } else if (*tree->ptr == '|') {
        if (*(tree->ptr + 1) == '|') {
            if (offset) {
                *offset += 1;
            }
            return UBASIC_TOKENIZER_LOR;
        }
        return UBASIC_TOKENIZER_OR;
    } else if (*tree->ptr == '*') {
        return UBASIC_TOKENIZER_ASTR;
    } else if (*tree->ptr == '!') {
        return UBASIC_TOKENIZER_LNOT;
    } else if (*tree->ptr == '~') {
        return UBASIC_TOKENIZER_NOT;
    } else if (*tree->ptr == '/') {
        return UBASIC_TOKENIZER_SLASH;
    } else if (*tree->ptr == '%') {
        return UBASIC_TOKENIZER_MOD;
    } else if (*tree->ptr == '(') {
        return UBASIC_TOKENIZER_LEFTPAREN;
    } else if (*tree->ptr == ')') {
        return UBASIC_TOKENIZER_RIGHTPAREN;
    } else if (*tree->ptr == '<') {
        if (tree->ptr[1] == '=') {
            if (offset) {
                *offset += 1;
            }
            return UBASIC_TOKENIZER_LE;
        } else if (tree->ptr[1] == '>') {
            if (offset) {
                *offset += 1;
            }
            return UBASIC_TOKENIZER_NE;
        }
        return UBASIC_TOKENIZER_LT;
    } else if (*tree->ptr == '>') {
        if (tree->ptr[1] == '=') {
            if (offset) {
                *offset += 1;
            }
            return UBASIC_TOKENIZER_GE;
        }
        return UBASIC_TOKENIZER_GT;
    } else if (*tree->ptr == '=') {
        if (tree->ptr[1] == '=') {
            if (offset) {
                *offset += 1;
            }
        }
        return UBASIC_TOKENIZER_EQ;
    }
    return 0;
}
/*---------------------------------------------------------------------------*/
static uint8_t tokenizer_next_token(struct ubasic_tokenizer *tree)
{
    const struct keyword_token *kt;
    uint8_t i, j;

    /* eat all whitespace */
    while (*tree->ptr == ' ' || *tree->ptr == '\t' || *tree->ptr == '\r') {
        tree->ptr++;
    }

    if (*tree->ptr == 0) {
        return UBASIC_TOKENIZER_ENDOFINPUT;
    }

    uint8_t have_decdot = 0, i_dot = 0;
    if ((tree->ptr[0] == '0') &&
        ((tree->ptr[1] == 'x') || (tree->ptr[1] == 'X'))) {
        /* is it HEX */
        tree->nextptr = tree->ptr + 2;
        while (1) {
            if (*tree->nextptr >= '0' && *tree->nextptr <= '9') {
                tree->nextptr++;
                continue;
            }
            if ((*tree->nextptr >= 'a') && (*tree->nextptr <= 'f')) {
                tree->nextptr++;
                continue;
            }
            if ((*tree->nextptr >= 'A') && (*tree->nextptr <= 'F')) {
                tree->nextptr++;
                continue;
            }
            return UBASIC_TOKENIZER_INT;
        }
    } else if (
        (tree->ptr[0] == '0') &&
        ((tree->ptr[1] == 'b') || (tree->ptr[1] == 'B'))) {
        /* is it BIN */
        tree->nextptr = tree->ptr + 2;
        while (*tree->nextptr == '0' || *tree->nextptr == '1') {
            tree->nextptr++;
        }
        return UBASIC_TOKENIZER_INT;
    } else if (isdigit(*tree->ptr) || (*tree->ptr == '.')) {
        /* is it FLOAT (digits with at most one decimal point) */
        /* is it DEC (digits without decimal point which ends in d,D,L,l) */
        tree->nextptr = tree->ptr;
        have_decdot = 0;
        i_dot = 0;
        while (1) {
            if (*tree->nextptr >= '0' && *tree->nextptr <= '9') {
                tree->nextptr++;
                if (have_decdot) {
                    i_dot++;
                }
                continue;
            }
            if (*tree->nextptr == '.') {
                tree->nextptr++;
                have_decdot++;
                if (have_decdot > 1) {
                    return UBASIC_TOKENIZER_ERROR;
                }
                continue;
            }
            if (*tree->nextptr == 'd' || *tree->nextptr == 'D' ||
                *tree->nextptr == 'l' || *tree->nextptr == 'L') {
                return UBASIC_TOKENIZER_INT;
            }

#if defined(UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            if (i_dot) {
                return UBASIC_TOKENIZER_FLOAT;
            }
#endif

            return UBASIC_TOKENIZER_NUMBER;
        }
    } else if ((j = singlechar_or_operator(tree, &i))) {
        tree->nextptr = tree->ptr + i;
        return j;
    }
#if defined(UBASIC_VARIABLE_TYPE_STRING)
    else if (
        (*tree->ptr == '"' || *tree->ptr == '\'') &&
        (*(tree->ptr - 1) != '\\')) {
        i = *tree->ptr;
        tree->nextptr = tree->ptr;
        do {
            ++tree->nextptr;
            if ((*tree->nextptr == '\0') || (*tree->nextptr == '\n') ||
                (*tree->nextptr == ';')) {
                return UBASIC_TOKENIZER_ERROR;
            }
        } while (*tree->nextptr != i || *(tree->nextptr - 1) == '\\');

        ++tree->nextptr;

        return UBASIC_TOKENIZER_STRING;
    }
#endif
    else {
        /* Check for keywords: */
        for (kt = keywords; kt->keyword != NULL; ++kt) {
            if (strncmp(tree->ptr, kt->keyword, strlen(kt->keyword)) == 0) {
                tree->nextptr = tree->ptr + strlen(kt->keyword);
                return kt->token;
            }
        }
    }

    /**
     * what is left after this point we call a label as long as
     * it starts with "_" or a..z
     * and contains only digits and letters
     */
    i = 0;
    j = 0;
    if (*tree->ptr == '_' || (*tree->ptr >= 'a' && *tree->ptr <= 'z') ||
        (*tree->ptr >= 'A' && *tree->ptr <= 'Z')) {
        tree->nextptr = tree->ptr;
        while (1) {
            if (*tree->nextptr == '_') {
                j++;
                tree->nextptr++;
                continue;
            }
            if ((*tree->nextptr >= '0') && (*tree->nextptr <= '9')) {
                i++;
                tree->nextptr++;
                continue;
            }
            if ((*tree->nextptr >= 'a') && (*tree->nextptr <= 'z')) {
                i++;
                tree->nextptr++;
                continue;
            }
            if ((*tree->nextptr >= 'A') && (*tree->nextptr <= 'Z')) {
                i++;
                tree->nextptr++;
                continue;
            }

            if (j > 0 || i > 1) {
                return UBASIC_TOKENIZER_LABEL;
            }

            if (i == 1) {
#if defined(UBASIC_VARIABLE_TYPE_STRING)
                if (*(tree->ptr + 1) == '$') {
                    tree->nextptr++;
                    return UBASIC_TOKENIZER_STRINGVARIABLE;
                }
#endif

#if defined(UBASIC_VARIABLE_TYPE_ARRAY)
                if (*(tree->ptr + 1) == '@') {
                    tree->nextptr++;
                    return UBASIC_TOKENIZER_ARRAYVARIABLE;
                }
#endif
                return UBASIC_TOKENIZER_VARIABLE;
            }
            break;
        }
    }

    return UBASIC_TOKENIZER_ERROR;
}
/*---------------------------------------------------------------------------*/
#if defined(UBASIC_VARIABLE_TYPE_STRING)
int8_t tokenizer_stringlookahead(struct ubasic_tokenizer *tree)
{
    /* return 1 (true) if next 'defining' token is string not integer */
    const char *saveptr = tree->ptr;
    const char *savenextptr = tree->nextptr;
    uint8_t token = tree->current_token;
    int8_t si = -1;

    while (si == -1) {
        if (token == UBASIC_TOKENIZER_EOL ||
            token == UBASIC_TOKENIZER_ENDOFINPUT) {
            si = 0;
        } else if (
            token == UBASIC_TOKENIZER_NUMBER ||
            token == UBASIC_TOKENIZER_VARIABLE ||
            token == UBASIC_TOKENIZER_FLOAT) {
            si = 0; /* number or numeric var */
        } else if (token == UBASIC_TOKENIZER_PLUS) {
            /* do nothing */
        } else if (token == UBASIC_TOKENIZER_STRING) {
            si = 1;
        } else if (
            token >= UBASIC_TOKENIZER_STRINGVARIABLE &&
            token <= UBASIC_TOKENIZER_CHR_STR) {
            si = 1;
        } else if (token > UBASIC_TOKENIZER_CHR_STR) {
            si = 0; /* numeric function */
        }

        token = tokenizer_next_token(tree);
    }
    tree->ptr = saveptr;
    tree->nextptr = savenextptr;
    return si;
}
#endif
/*---------------------------------------------------------------------------*/
void tokenizer_init(struct ubasic_tokenizer *tree, const char *program)
{
    tree->ptr = program;
    tree->prog = program;
    tree->current_token = tokenizer_next_token(tree);
}
/*---------------------------------------------------------------------------*/
uint8_t tokenizer_token(struct ubasic_tokenizer *tree)
{
    return tree->current_token;
}
/*---------------------------------------------------------------------------*/
void tokenizer_next(struct ubasic_tokenizer *tree)
{
    if (tokenizer_finished(tree)) {
        return;
    }

    tree->ptr = tree->nextptr;

    while (*tree->ptr == ' ') {
        ++tree->ptr;
    }

    tree->current_token = tokenizer_next_token(tree);
    return;
}
/*---------------------------------------------------------------------------*/

UBASIC_VARIABLE_TYPE tokenizer_num(struct ubasic_tokenizer *tree)
{
    const char *c = tree->ptr;
    UBASIC_VARIABLE_TYPE rval = 0;

    while (1) {
        if (*c < '0' || *c > '9') {
            break;
        }

        rval *= 10;
        rval += (*c - '0');
        c++;
    }

    return rval;
}

UBASIC_VARIABLE_TYPE tokenizer_int(struct ubasic_tokenizer *tree)
{
    const char *c = tree->ptr;
    UBASIC_VARIABLE_TYPE rval = 0;
    if ((*c == '0') && (*(c + 1) == 'x' || *(c + 1) == 'X')) {
        c += 2;
        while (1) {
            if (*c >= '0' && *c <= '9') {
                rval <<= 4;
                rval += (*c - '0');
                c++;
                continue;
            }
            if ((*c >= 'a') && (*c <= 'f')) {
                rval <<= 4;
                rval += (*c - 87); /* 87 = 'a' - 10 */
                c++;
                continue;
            }
            if ((*c >= 'A') && (*c <= 'F')) {
                rval <<= 4;
                rval += (*c - 55); /* 55 = 'A' - 10 */
                c++;
                continue;
            }
            break;
        }
        return rval;
    }
    if ((*c == '0') && (*(c + 1) == 'b' || *(c + 1) == 'B')) {
        c += 2;
        while (1) {
            if (*c == '0' || *c == '1') {
                rval <<= 1;
                rval += (*c - '0');
                c++;
                continue;
            }
            break;
        }
        return rval;
    }

    return tokenizer_num(tree);
}

#if defined(UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(UBASIC_VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
/*---------------------------------------------------------------------------*/
UBASIC_VARIABLE_TYPE tokenizer_float(struct ubasic_tokenizer *tree)
{
    return str_fixedpt(
        tree->ptr, tree->nextptr - tree->ptr, FIXEDPT_FBITS >> 1);
}
#endif

#if defined(UBASIC_VARIABLE_TYPE_STRING)
/*---------------------------------------------------------------------------*/
void tokenizer_string(struct ubasic_tokenizer *tree, char *dest, uint8_t len)
{
    const char *string_end;
    char quote_char;
    uint8_t string_len;

    if (tokenizer_token(tree) != UBASIC_TOKENIZER_STRING) {
        return;
    }
    quote_char = *tree->ptr;

    /** figure out the quote used for strings
     * ignore escaped string-quotes
     */
    string_end = tree->ptr;
    do {
        string_end++;

        string_end = strchr(string_end, quote_char);
        if (string_end == NULL) {
            return;
        }
    } while (*(string_end - 1) == '\\');

    string_len = string_end - tree->ptr - 1;
    if (len < string_len) {
        string_len = len;
    }
    memcpy(dest, tree->ptr + 1, string_len);
    dest[string_len] = 0;
    return;
}
#endif

void tokenizer_label(struct ubasic_tokenizer *tree, char *dest, uint8_t len)
{
    const char *string_end = tree->nextptr;
    uint8_t string_len;

    if (tokenizer_token(tree) != UBASIC_TOKENIZER_LABEL) {
        return;
    }

    for (string_len = 0; string_len < string_end - tree->ptr; string_len++) {
        if ((*(tree->ptr + string_len) == '_') ||
            ((*(tree->ptr + string_len) >= '0') &&
             (*(tree->ptr + string_len) <= '9')) ||
            ((*(tree->ptr + string_len) >= 'A') &&
             (*(tree->ptr + string_len) <= 'Z')) ||
            ((*(tree->ptr + string_len) >= 'a') &&
             (*(tree->ptr + string_len) <= 'z'))) {
            continue;
        }
        break;
    }
    if (string_len > len) {
        string_len = len;
    }
    memcpy(dest, tree->ptr, string_len);
    dest[string_len] = 0;
}

/*---------------------------------------------------------------------------*/
bool tokenizer_finished(struct ubasic_tokenizer *tree)
{
    return (
        (*tree->ptr == 0) ||
        (tree->current_token == UBASIC_TOKENIZER_ENDOFINPUT));
}

/*---------------------------------------------------------------------------*/
uint8_t tokenizer_variable_num(struct ubasic_tokenizer *tree)
{
    if ((*tree->ptr >= 'a' && *tree->ptr <= 'z')) {
        return (((uint8_t)*tree->ptr) - 'a');
    }

    if ((*tree->ptr >= 'A' && *tree->ptr <= 'Z')) {
        return (((uint8_t)*tree->ptr) - 'A');
    }

    return 0xff;
}
/*---------------------------------------------------------------------------*/

uint16_t tokenizer_save_offset(struct ubasic_tokenizer *tree)
{
    return (tree->ptr - tree->prog);
}

void tokenizer_jump_offset(struct ubasic_tokenizer *tree, uint16_t offset)
{
    tree->ptr = (tree->prog + offset);
    tree->current_token = tokenizer_next_token(tree);
    while ((tree->current_token == UBASIC_TOKENIZER_EOL) &&
           !tokenizer_finished(tree)) {
        tokenizer_next(tree);
    }
    return;
}

const char *tokenizer_name(UBASIC_VARIABLE_TYPE token)
{
    const struct keyword_token *kt;

    for (kt = keywords; kt->keyword != NULL; ++kt) {
        if (kt->token == token) {
            return kt->keyword;
        }
    }

    return NULL;
}
