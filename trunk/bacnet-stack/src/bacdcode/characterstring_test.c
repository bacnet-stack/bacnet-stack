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
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testCharacterString(Test * pTest)
{
    BACNET_CHARACTER_STRING bacnet_string;
    char *value = "Joshua,Mary,Anna,Christopher";
    char test_value[MAX_APDU] = "Patricia";
    char test_append_value[MAX_APDU] = " and the Kids";
    char test_append_string[MAX_APDU] = "";
    bool status = false;
    size_t length = 0;
    size_t test_length = 0;
    size_t i = 0;

    /* verify initialization */
    status =
        characterstring_init(&bacnet_string, CHARACTER_ANSI_X34, NULL, 0);
    ct_test(pTest, status == true);
    ct_test(pTest, characterstring_length(&bacnet_string) == 0);
    ct_test(pTest,
        characterstring_encoding(&bacnet_string) == CHARACTER_ANSI_X34);
    /* bounds check */
    status = characterstring_init(&bacnet_string,
        CHARACTER_ANSI_X34,
        NULL, characterstring_capacity(&bacnet_string) + 1);
    ct_test(pTest, status == false);
    status =
        characterstring_truncate(&bacnet_string,
        characterstring_capacity(&bacnet_string) + 1);
    ct_test(pTest, status == false);
    status =
        characterstring_truncate(&bacnet_string,
        characterstring_capacity(&bacnet_string));
    ct_test(pTest, status == true);

    test_length = strlen(test_value);
    status = characterstring_init(&bacnet_string,
        CHARACTER_ANSI_X34, &test_value[0], test_length);
    ct_test(pTest, status == true);
    value = characterstring_value(&bacnet_string);
    length = characterstring_length(&bacnet_string);
    ct_test(pTest, length == test_length);
    for (i = 0; i < test_length; i++) {
        ct_test(pTest, value[i] == test_value[i]);
    }
    test_length = strlen(test_append_value);
    status = characterstring_append(&bacnet_string,
        &test_append_value[0], test_length);
    strcat(test_append_string, test_value);
    strcat(test_append_string, test_append_value);
    test_length = strlen(test_append_string);
    ct_test(pTest, status == true);
    length = characterstring_length(&bacnet_string);
    value = characterstring_value(&bacnet_string);
    ct_test(pTest, length == test_length);
    for (i = 0; i < test_length; i++) {
        ct_test(pTest, value[i] == test_append_string[i]);
    }
}
