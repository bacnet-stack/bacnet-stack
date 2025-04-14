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
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Modified to support simple string variables and functions by David Mitchell
 * November 2008.
 * Changes and additions are marked 'string additions' throughout
 */

#include "config.h"
#include "tokenizer.h"

#define MAX_NUMLEN 8

struct keyword_token {
    const char *keyword;
    uint8_t token;
};

static const struct keyword_token keywords[] = {
#if defined(VARIABLE_TYPE_STRING)
    // new string-related statements and functions
    { "left$", TOKENIZER_LEFT$ },
    { "right$", TOKENIZER_RIGHT$ },
    { "mid$", TOKENIZER_MID$ },
    { "str$", TOKENIZER_STR$ },
    { "chr$", TOKENIZER_CHR$ },
    { "val", TOKENIZER_VAL },
    { "len", TOKENIZER_LEN },
    { "instr", TOKENIZER_INSTR },
    { "asc", TOKENIZER_ASC },
#endif
    // end of string additions
    { "let ", TOKENIZER_LET },
    { "println ", TOKENIZER_PRINTLN },
    { "print ", TOKENIZER_PRINT },
    { "if", TOKENIZER_IF },
    { "then", TOKENIZER_THEN },
    { "else", TOKENIZER_ELSE },
    { "endif", TOKENIZER_ENDIF },
#if defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS)
    { "toc", TOKENIZER_TOC },
#endif
#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
    { "input", TOKENIZER_INPUT },
#endif
    { "for ", TOKENIZER_FOR },
    { "to ", TOKENIZER_TO },
    { "next ", TOKENIZER_NEXT },
    { "step ", TOKENIZER_STEP },
    { "while", TOKENIZER_WHILE },
    { "endwhile", TOKENIZER_ENDWHILE },
    { "goto ", TOKENIZER_GOTO },
    { "gosub ", TOKENIZER_GOSUB },
    { "return", TOKENIZER_RETURN },
    { "end", TOKENIZER_END },
#if defined(UBASIC_SCRIPT_HAVE_SLEEP)
    { "sleep", TOKENIZER_SLEEP },
#endif
#if defined(VARIABLE_TYPE_ARRAY)
    { "dim ", TOKENIZER_DIM },
#endif
#if defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS)
    { "tic", TOKENIZER_TIC },
#endif
#if defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS)
    { "flag", TOKENIZER_HWE },
#endif
#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
    { "ran", TOKENIZER_RAN },
#endif
#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
    { "sqrt", TOKENIZER_SQRT },
    { "sin", TOKENIZER_SIN },
    { "cos", TOKENIZER_COS },
    { "tan", TOKENIZER_TAN },
    { "exp", TOKENIZER_EXP },
    { "ln", TOKENIZER_LN },
#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
    { "uniform", TOKENIZER_UNIFORM },
#endif
    { "abs", TOKENIZER_ABS },
    { "floor", TOKENIZER_FLOOR },
    { "ceil", TOKENIZER_CEIL },
    { "round", TOKENIZER_ROUND },
    { "pow", TOKENIZER_POWER },
    { "avgw", TOKENIZER_AVERAGEW },
#endif
#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
    { "pinmode", TOKENIZER_PINMODE },
    { "dread", TOKENIZER_DREAD },
    { "dwrite", TOKENIZER_DWRITE },
#endif
#ifdef UBASIC_SCRIPT_HAVE_PWM_CHANNELS
    { "awrite_conf", TOKENIZER_PWMCONF },
    { "awrite", TOKENIZER_PWM },
#endif
#if defined(UBASIC_SCRIPT_HAVE_ANALOG_READ)
    { "aread_conf", TOKENIZER_AREADCONF },
    { "aread", TOKENIZER_AREAD },
#endif
    { "hex ", TOKENIZER_PRINT_HEX },
    { "dec ", TOKENIZER_PRINT_DEC },
    { ":", TOKENIZER_COLON },
#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
    { "store", TOKENIZER_STORE },
    { "recall", TOKENIZER_RECALL },
#endif
#if defined(UBASIC_SCRIPT_HAVE_BACNET)
    { "bac_create", TOKENIZER_BACNET_CREATE_OBJECT },
    { "bac_read", TOKENIZER_BACNET_READ_PROPERTY },
    { "bac_write", TOKENIZER_BACNET_WRITE_PROPERTY },
#endif
    { "clear", TOKENIZER_CLEAR },
    { NULL, TOKENIZER_ERROR }
};

/*---------------------------------------------------------------------------*/
static uint8_t
singlechar_or_operator(struct tokenizer_data *tree, uint8_t *offset)
{
    if (offset) {
        *offset = 1;
    }

    if ((*tree->ptr == '\n') || (*tree->ptr == ';')) {
        return TOKENIZER_EOL;
    } else if (*tree->ptr == ',') {
        return TOKENIZER_COMMA;
    } else if (*tree->ptr == '+') {
        return TOKENIZER_PLUS;
    } else if (*tree->ptr == '-') {
        return TOKENIZER_MINUS;
    } else if (*tree->ptr == '&') {
        if (*(tree->ptr + 1) == '&') {
            if (offset) {
                *offset += 1;
            }
            return TOKENIZER_LAND;
        }
        return TOKENIZER_AND;
    } else if (*tree->ptr == '|') {
        if (*(tree->ptr + 1) == '|') {
            if (offset) {
                *offset += 1;
            }
            return TOKENIZER_LOR;
        }
        return TOKENIZER_OR;
    } else if (*tree->ptr == '*') {
        return TOKENIZER_ASTR;
    } else if (*tree->ptr == '!') {
        return TOKENIZER_LNOT;
    } else if (*tree->ptr == '~') {
        return TOKENIZER_NOT;
    } else if (*tree->ptr == '/') {
        return TOKENIZER_SLASH;
    } else if (*tree->ptr == '%') {
        return TOKENIZER_MOD;
    } else if (*tree->ptr == '(') {
        return TOKENIZER_LEFTPAREN;
    } else if (*tree->ptr == ')') {
        return TOKENIZER_RIGHTPAREN;
    } else if (*tree->ptr == '<') {
        if (tree->ptr[1] == '=') {
            if (offset) {
                *offset += 1;
            }
            return TOKENIZER_LE;
        } else if (tree->ptr[1] == '>') {
            if (offset) {
                *offset += 1;
            }
            return TOKENIZER_NE;
        }
        return TOKENIZER_LT;
    } else if (*tree->ptr == '>') {
        if (tree->ptr[1] == '=') {
            if (offset) {
                *offset += 1;
            }
            return TOKENIZER_GE;
        }
        return TOKENIZER_GT;
    } else if (*tree->ptr == '=') {
        if (tree->ptr[1] == '=') {
            if (offset) {
                *offset += 1;
            }
        }
        return TOKENIZER_EQ;
    }
    return 0;
}
/*---------------------------------------------------------------------------*/
static uint8_t tokenizer_next_token(struct tokenizer_data *tree)
{
    const struct keyword_token *kt;
    uint8_t i, j;

    // eat all whitespace
    while (*tree->ptr == ' ' || *tree->ptr == '\t' || *tree->ptr == '\r') {
        tree->ptr++;
    }

    if (*tree->ptr == 0) {
        return TOKENIZER_ENDOFINPUT;
    }

    uint8_t have_decdot = 0, i_dot = 0;
    if ((tree->ptr[0] == '0') &&
        ((tree->ptr[1] == 'x') || (tree->ptr[1] == 'X'))) {
        // is it HEX
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
            return TOKENIZER_INT;
        }
    } else if (
        (tree->ptr[0] == '0') &&
        ((tree->ptr[1] == 'b') || (tree->ptr[1] == 'B'))) {
        // is it BIN
        tree->nextptr = tree->ptr + 2;
        while (*tree->nextptr == '0' || *tree->nextptr == '1') {
            tree->nextptr++;
        }
        return TOKENIZER_INT;
    } else if (isdigit(*tree->ptr) || (*tree->ptr == '.')) {
        // is it
        //    FLOAT (digits with at most one decimal point)
        // or is it
        //    DEC (digits without decimal point which ends in d,D,L,l)
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
                    return TOKENIZER_ERROR;
                }
                continue;
            }
            if (*tree->nextptr == 'd' || *tree->nextptr == 'D' ||
                *tree->nextptr == 'l' || *tree->nextptr == 'L') {
                return TOKENIZER_INT;
            }

#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
            if (i_dot) {
                return TOKENIZER_FLOAT;
            }
#endif

            return TOKENIZER_NUMBER;
        }
    } else if ((j = singlechar_or_operator(tree, &i))) {
        tree->nextptr = tree->ptr + i;
        return j;
    }
#if defined(VARIABLE_TYPE_STRING)
    else if (
        (*tree->ptr == '"' || *tree->ptr == '\'') &&
        (*(tree->ptr - 1) != '\\')) {
        i = *tree->ptr;
        tree->nextptr = tree->ptr;
        do {
            ++tree->nextptr;
            if ((*tree->nextptr == '\0') || (*tree->nextptr == '\n') ||
                (*tree->nextptr == ';')) {
                return TOKENIZER_ERROR;
            }
        } while (*tree->nextptr != i || *(tree->nextptr - 1) == '\\');

        ++tree->nextptr;

        return TOKENIZER_STRING;
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
                return TOKENIZER_LABEL;
            }

            if (i == 1) {
#if defined(VARIABLE_TYPE_STRING)
                if (*(tree->ptr + 1) == '$') {
                    tree->nextptr++;
                    return TOKENIZER_STRINGVARIABLE;
                }
#endif

#if defined(VARIABLE_TYPE_ARRAY)
                if (*(tree->ptr + 1) == '@') {
                    tree->nextptr++;
                    return TOKENIZER_ARRAYVARIABLE;
                }
#endif
                return TOKENIZER_VARIABLE;
            }
            break;
        }
    }

    return TOKENIZER_ERROR;
}
/*---------------------------------------------------------------------------*/
#if defined(VARIABLE_TYPE_STRING)
int8_t tokenizer_stringlookahead(struct tokenizer_data *tree)
{
    // return 1 (true) if next 'defining' token is string not integer
    const char *saveptr = tree->ptr;
    const char *savenextptr = tree->nextptr;
    uint8_t token = tree->current_token;
    int8_t si = -1;

    while (si == -1) {
        if (token == TOKENIZER_EOL || token == TOKENIZER_ENDOFINPUT) {
            si = 0;
        } else if (
            token == TOKENIZER_NUMBER || token == TOKENIZER_VARIABLE ||
            token == TOKENIZER_FLOAT) {
            si = 0; // number or numeric var
        } else if (token == TOKENIZER_PLUS) {
            si = si;
        } else if (token == TOKENIZER_STRING) {
            si = 1;
        } else if (
            token >= TOKENIZER_STRINGVARIABLE && token <= TOKENIZER_CHR$) {
            si = 1;
        } else if (token > TOKENIZER_CHR$) {
            si = 0; // numeric function
        }

        token = tokenizer_next_token(tree);
    }
    tree->ptr = saveptr;
    tree->nextptr = savenextptr;
    return si;
}
#endif
/*---------------------------------------------------------------------------*/
void tokenizer_init(struct tokenizer_data *tree, const char *program)
{
    tree->ptr = program;
    tree->prog = program;
    tree->current_token = tokenizer_next_token(tree);
}
/*---------------------------------------------------------------------------*/
uint8_t tokenizer_token(struct tokenizer_data *tree)
{
    return tree->current_token;
}
/*---------------------------------------------------------------------------*/
void tokenizer_next(struct tokenizer_data *tree)
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

VARIABLE_TYPE tokenizer_num(struct tokenizer_data *tree)
{
    const char *c = tree->ptr;
    VARIABLE_TYPE rval = 0;

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

VARIABLE_TYPE tokenizer_int(struct tokenizer_data *tree)
{
    const char *c = tree->ptr;
    VARIABLE_TYPE rval = 0;
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
                rval += (*c - 87); // 87 = 'a' - 10
                c++;
                continue;
            }
            if ((*c >= 'A') && (*c <= 'F')) {
                rval <<= 4;
                rval += (*c - 55); // 55 = 'A' - 10
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

#if defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_24_8) || \
    defined(VARIABLE_TYPE_FLOAT_AS_FIXEDPT_22_10)
/*---------------------------------------------------------------------------*/
VARIABLE_TYPE tokenizer_float(struct tokenizer_data *tree)
{
    return str_fixedpt(
        tree->ptr, tree->nextptr - tree->ptr, FIXEDPT_FBITS >> 1);
}
#endif

#if defined(VARIABLE_TYPE_STRING)
/*---------------------------------------------------------------------------*/
void tokenizer_string(struct tokenizer_data *tree, char *dest, uint8_t len)
{
    const char *string_end;
    char quote_char;
    uint8_t string_len;

    if (tokenizer_token(tree) != TOKENIZER_STRING) {
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

void tokenizer_label(struct tokenizer_data *tree, char *dest, uint8_t len)
{
    const char *string_end = tree->nextptr;
    uint8_t string_len;

    if (tokenizer_token(tree) != TOKENIZER_LABEL) {
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
bool tokenizer_finished(struct tokenizer_data *tree)
{
    return ((*tree->ptr == 0) || (tree->current_token == TOKENIZER_ENDOFINPUT));
}

/*---------------------------------------------------------------------------*/
uint8_t tokenizer_variable_num(struct tokenizer_data *tree)
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

// uint16_t tokenizer_line_number(void)
// {
//   return current_line;
// }

uint16_t tokenizer_save_offset(struct tokenizer_data *tree)
{
    return (tree->ptr - tree->prog);
}

void tokenizer_jump_offset(struct tokenizer_data *tree, uint16_t offset)
{
    tree->ptr = (tree->prog + offset);
    tree->current_token = tokenizer_next_token(tree);
    while ((tree->current_token == TOKENIZER_EOL) &&
           !tokenizer_finished(tree)) {
        tokenizer_next(tree);
    }
    return;
}

const char *tokenizer_name(VARIABLE_TYPE token)
{
    const struct keyword_token *kt;

    for (kt = keywords; kt->keyword != NULL; ++kt) {
        if (kt->token == token) {
            return kt->keyword;
        }
    }

    return NULL;
}
