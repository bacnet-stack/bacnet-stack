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

/* returns false if the string exceeds capacity
   initialize by using length=0 */
bool octetstring_init(BACNET_OCTET_STRING * octet_string,
    uint8_t * value, size_t length)
{
    bool status = false;        /* return value */
    size_t i;                   /* counter */

    if (octet_string) {
        octet_string->length = 0;
        if (length <= sizeof(octet_string->value)) {
            if (value) {
                for (i = 0; i < length; i++) {
                    octet_string->value[octet_string->length] = value[i];
                    octet_string->length++;
                }
            } else {
                for (i = 0; i < sizeof(octet_string->value); i++) {
                    octet_string->value[i] = 0;
                }
            }
            status = true;
        }
    }

    return status;
}

bool octetstring_copy(BACNET_OCTET_STRING * dest,
    BACNET_OCTET_STRING * src)
{
    return octetstring_init(dest,
        octetstring_value(src), octetstring_length(src));
}

/* returns false if the string exceeds capacity */
bool octetstring_append(BACNET_OCTET_STRING * octet_string,
    uint8_t * value, size_t length)
{
    size_t i;                   /* counter */
    bool status = false;        /* return value */

    if (octet_string) {
        if ((length + octet_string->length) <= sizeof(octet_string->value)) {
            for (i = 0; i < length; i++) {
                octet_string->value[octet_string->length] = value[i];
                octet_string->length++;
            }
            status = true;
        }
    }

    return status;
}

/* This function sets a new length without changing the value.
   If length exceeds capacity, no modification happens and
   function returns false.  */
bool octetstring_truncate(BACNET_OCTET_STRING * octet_string,
    size_t length)
{
    bool status = false;        /* return value */

    if (octet_string) {
        if (length <= sizeof(octet_string->value)) {
            octet_string->length = length;
            status = true;
        }
    }

    return status;
}

/* returns the length.  Returns the value in parameter. */
uint8_t *octetstring_value(BACNET_OCTET_STRING * octet_string)
{
    uint8_t *value = NULL;

    if (octet_string) {
        value = octet_string->value;
    }

    return value;
}

/* returns the length. */
size_t octetstring_length(BACNET_OCTET_STRING * octet_string)
{
    size_t length = 0;

    if (octet_string) {
        /* FIXME: validate length is within bounds? */
        length = octet_string->length;
    }

    return length;
}

/* returns the length. */
size_t octetstring_capacity(BACNET_OCTET_STRING * octet_string)
{
    size_t length = 0;

    if (octet_string) {
        /* FIXME: validate length is within bounds? */
        length = sizeof(octet_string->value);
    }

    return length;
}
