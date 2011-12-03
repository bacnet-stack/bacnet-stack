/**************************************************************************
*
* Copyright (C) 2011 Steve Karg <skarg@users.sourceforge.net>
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
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "hardware.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacstr.h"
#include "nvdata.h"
#include "seeprom.h"
#include "device.h"
#include "bname.h"

/* Basic UTF-8 manipulation routines
  by Jeff Bezanson
  placed in the public domain Fall 2005 */
static const char trailingBytesForUTF8[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4,
        4, 4, 4, 5, 5, 5, 5
};

/* based on the valid_utf8 routine from the PCRE library by Philip Hazel
   length is in bytes, since without knowing whether the string is valid
   it's hard to know how many characters there are! */
static int utf8_isvalid(
    const char *str,
    int length)
{
    const unsigned char *p, *pend = (unsigned char *) str + length;
    unsigned char c;
    int ab;

    for (p = (unsigned char *) str; p < pend; p++) {
        c = *p;
        /* null in middle of string */
        if (c == 0) {
            return 0;
        }
        /* ASCII character */
        if (c < 128) {
            continue;
        }
        if ((c & 0xc0) != 0xc0) {
            return 0;
        }
        ab = trailingBytesForUTF8[c];
        if (length < ab) {
            return 0;
        }
        length -= ab;

        p++;
        /* Check top bits in the second byte */
        if ((*p & 0xc0) != 0x80) {
            return 0;
        }
        /* Check for overlong sequences for each different length */
        switch (ab) {
                /* Check for xx00 000x */
            case 1:
                if ((c & 0x3e) == 0)
                    return 0;
                continue;       /* We know there aren't any more bytes to check */

                /* Check for 1110 0000, xx0x xxxx */
            case 2:
                if (c == 0xe0 && (*p & 0x20) == 0)
                    return 0;
                break;

                /* Check for 1111 0000, xx00 xxxx */
            case 3:
                if (c == 0xf0 && (*p & 0x30) == 0)
                    return 0;
                break;

                /* Check for 1111 1000, xx00 0xxx */
            case 4:
                if (c == 0xf8 && (*p & 0x38) == 0)
                    return 0;
                break;

                /* Check for leading 0xfe or 0xff,
                   and then for 1111 1100, xx00 00xx */
            case 5:
                if (c == 0xfe || c == 0xff || (c == 0xfc && (*p & 0x3c) == 0))
                    return 0;
                break;
        }

        /* Check for valid bytes after the 2nd, if any; all must start 10 */
        while (--ab > 0) {
            if ((*(++p) & 0xc0) != 0x80)
                return 0;
        }
    }

    return 1;
}

static bool bacnet_name_isvalid(
    uint8_t encoding,
    uint8_t length,
    char *str)
{
    bool valid = false;

    if ((encoding < MAX_CHARACTER_STRING_ENCODING) &&
        (length <= NV_EEPROM_NAME_SIZE)) {
        if (encoding == CHARACTER_ANSI_X34) {
            if (utf8_isvalid(str, length)) {
                valid = true;
            }
        } else {
            valid = true;
        }
    }

    return valid;
}

bool bacnet_name_set(
    uint16_t offset,
    BACNET_CHARACTER_STRING * char_string)
{
    uint8_t encoding = 0;
    uint8_t length = 0;
    char *str = NULL;

    length = characterstring_length(char_string);
    encoding = characterstring_encoding(char_string);
    str = characterstring_value(char_string);
    if (bacnet_name_isvalid(encoding, length, str)) {
        seeprom_bytes_write(NV_EEPROM_NAME_LENGTH(offset), &length, 1);
        seeprom_bytes_write(NV_EEPROM_NAME_ENCODING(offset), &encoding, 1);
        seeprom_bytes_write(NV_EEPROM_NAME_STRING(offset), (uint8_t *) str,
            length);
        return true;
    }

    return false;
}

bool bacnet_name_write(
    uint16_t offset,
    BACNET_CHARACTER_STRING * char_string,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    bool status = false;
    size_t length = 0;
    uint8_t encoding = 0;

    length = characterstring_length(char_string);

    if (length < 1) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
    } else if (length <= NV_EEPROM_NAME_SIZE) {
        encoding = characterstring_encoding(char_string);
        if (encoding < MAX_CHARACTER_STRING_ENCODING) {
            if (Device_Valid_Object_Name(char_string, NULL, NULL)) {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_DUPLICATE_NAME;
            } else {
                status = bacnet_name_set(offset, char_string);
                if (status) {
                    Device_Inc_Database_Revision();
                } else {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED;
        }
    } else {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
    }

    return status;
}

/* no required minumum length or duplicate checking */
bool bacnet_name_write_other(
    uint16_t offset,
    BACNET_CHARACTER_STRING * char_string,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    bool status = false;
    size_t length = 0;
    uint8_t encoding = 0;

    length = characterstring_length(char_string);

    if (length <= NV_EEPROM_NAME_SIZE) {
        encoding = characterstring_encoding(char_string);
        if (encoding < MAX_CHARACTER_STRING_ENCODING) {
            status = bacnet_name_set(offset, char_string);
            if (!status) {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED;
        }
    } else {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
    }

    return status;
}

void bacnet_name_init(
    uint16_t offset,
    BACNET_CHARACTER_STRING * char_string,
    char *default_string)
{
    characterstring_init_ansi(char_string, default_string);
    (void) bacnet_name_set(offset, char_string);
}

void bacnet_name(
    uint16_t offset,
    BACNET_CHARACTER_STRING * char_string,
    char *default_string)
{
    uint8_t encoding = 0;
    uint8_t length = 0;
    char name[NV_EEPROM_NAME_SIZE + 1] = "";

    seeprom_bytes_read(NV_EEPROM_NAME_ENCODING(offset), &encoding, 1);
    seeprom_bytes_read(NV_EEPROM_NAME_LENGTH(offset), &length, 1);
    seeprom_bytes_read(NV_EEPROM_NAME_STRING(offset), (uint8_t *) & name,
        NV_EEPROM_NAME_SIZE);
    if (bacnet_name_isvalid(encoding, length, name)) {
        characterstring_init(char_string, encoding, &name[0], length);
    } else if (default_string) {
        bacnet_name_init(offset, char_string, default_string);
    }
}
