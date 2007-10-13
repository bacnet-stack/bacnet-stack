/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2004 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>             /* for strlen */
#include "bacstr.h"
#include "bits.h"

#define CHARACTER_STRING_CAPACITY (MAX_CHARACTER_STRING_BYTES - 1)
/* returns false if the string exceeds capacity
   initialize by using length=0 */
bool characterstring_init(BACNET_CHARACTER_STRING * char_string,
    uint8_t encoding, char *value, size_t length)
{
    bool status = false;        /* return value */
    size_t i;                   /* counter */

    if (char_string) {
        char_string->length = 0;
        char_string->encoding = encoding;
        /* save a byte at the end for NULL -
           note: assumes printable characters */
        if (length <= CHARACTER_STRING_CAPACITY) {
            if (value) {
                for (i = 0; i < MAX_CHARACTER_STRING_BYTES; i++) {
                    if (i < length) {
                        char_string->value[char_string->length] = value[i];
                        char_string->length++;
                    } else
                        char_string->value[i] = 0;
                }
            } else {
                for (i = 0; i < MAX_CHARACTER_STRING_BYTES; i++) {
                    char_string->value[i] = 0;
                }
            }
            status = true;
        }
    }

    return status;
}

bool characterstring_init_ansi(BACNET_CHARACTER_STRING * char_string,
    char *value)
{
    return characterstring_init(char_string,
        CHARACTER_ANSI_X34, value, value ? strlen(value) : 0);
}

bool characterstring_copy(BACNET_CHARACTER_STRING * dest,
    BACNET_CHARACTER_STRING * src)
{
    return characterstring_init(dest,
        characterstring_encoding(src),
        characterstring_value(src), characterstring_length(src));
}

bool characterstring_same(BACNET_CHARACTER_STRING * dest,
    BACNET_CHARACTER_STRING * src)
{
    size_t i;                   /* counter */
    bool same_status = false;

    if (src && dest) {
        if ((src->length == dest->length) &&
            (src->encoding == dest->encoding)) {
            same_status = true;
            for (i = 0; i < src->length; i++) {
                if (src->value[i] != dest->value[i]) {
                    same_status = false;
                    break;
                }
            }
        }
    } else if (src) {
        if (src->length == 0)
            same_status = true;
    } else if (dest) {
        if (dest->length == 0)
            same_status = true;
    }

    return same_status;
}

bool characterstring_ansi_same(BACNET_CHARACTER_STRING * dest,
    const char *src)
{
    size_t i;                   /* counter */
    bool same_status = false;

    if (src && dest) {
        if ((dest->length == strlen(src)) &&
            (dest->encoding == CHARACTER_ANSI_X34)) {
            same_status = true;
            for (i = 0; i < dest->length; i++) {
                if (src[i] != dest->value[i]) {
                    same_status = false;
                    break;
                }
            }
        }
    }
    /* NULL matches an empty string in our world */
    else if (src) {
        if (strlen(src) == 0)
            same_status = true;
    } else if (dest) {
        if (dest->length == 0)
            same_status = true;
    }

    return same_status;
}

/* returns false if the string exceeds capacity */
bool characterstring_append(BACNET_CHARACTER_STRING * char_string,
    char *value, size_t length)
{
    size_t i;                   /* counter */
    bool status = false;        /* return value */

    if (char_string) {
        if ((length + char_string->length) <= CHARACTER_STRING_CAPACITY) {
            for (i = 0; i < length; i++) {
                char_string->value[char_string->length] = value[i];
                char_string->length++;
            }
            status = true;
        }
    }

    return status;
}

/* This function sets a new length without changing the value.
   If length exceeds capacity, no modification happens and
   function returns false.  */
bool characterstring_truncate(BACNET_CHARACTER_STRING * char_string,
    size_t length)
{
    bool status = false;        /* return value */

    if (char_string) {
        if (length <= CHARACTER_STRING_CAPACITY) {
            char_string->length = length;
            status = true;
        }
    }

    return status;
}

/* Returns the value. */
char *characterstring_value(BACNET_CHARACTER_STRING * char_string)
{
    char *value = NULL;

    if (char_string) {
        value = char_string->value;
    }

    return value;
}

/* returns the length. */
size_t characterstring_length(BACNET_CHARACTER_STRING * char_string)
{
    size_t length = 0;

    if (char_string) {
        /* FIXME: validate length is within bounds? */
        length = char_string->length;
    }

    return length;
}

size_t characterstring_capacity(BACNET_CHARACTER_STRING * char_string)
{
    size_t length = 0;

    if (char_string) {
        length = CHARACTER_STRING_CAPACITY;
    }

    return length;
}

/* returns the encoding. */
uint8_t characterstring_encoding(BACNET_CHARACTER_STRING * char_string)
{
    uint8_t encoding = 0;

    if (char_string) {
        encoding = char_string->encoding;
    }

    return encoding;
}
