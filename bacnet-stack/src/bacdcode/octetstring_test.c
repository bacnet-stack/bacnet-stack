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

void testOctetString(Test * pTest)
{
    BACNET_OCTET_STRING bacnet_string;
    uint8_t *value = NULL;
    uint8_t test_value[MAX_APDU] = "Patricia";
    uint8_t test_append_value[MAX_APDU] = " and the Kids";
    uint8_t test_append_string[MAX_APDU] = "";
    bool status = false;
    size_t length = 0;
    size_t test_length = 0;
    size_t i = 0;

    /* verify initialization */
    status = octetstring_init(&bacnet_string, NULL, 0);
    ct_test(pTest, status == true);
    ct_test(pTest, octetstring_length(&bacnet_string) == 0);
    value = octetstring_value(&bacnet_string);
    for (i = 0; i < octetstring_capacity(&bacnet_string); i++) {
        ct_test(pTest, value[i] == 0);
    }
    /* bounds check */
    status = octetstring_init(&bacnet_string, NULL,
        octetstring_capacity(&bacnet_string) + 1);
    ct_test(pTest, status == false);
    status = octetstring_init(&bacnet_string, NULL,
        octetstring_capacity(&bacnet_string));
    ct_test(pTest, status == true);
    status =
        octetstring_truncate(&bacnet_string,
        octetstring_capacity(&bacnet_string) + 1);
    ct_test(pTest, status == false);
    status =
        octetstring_truncate(&bacnet_string,
        octetstring_capacity(&bacnet_string));
    ct_test(pTest, status == true);

    test_length = strlen((char *) test_value);
    status = octetstring_init(&bacnet_string, &test_value[0], test_length);
    ct_test(pTest, status == true);
    length = octetstring_length(&bacnet_string);
    value = octetstring_value(&bacnet_string);
    ct_test(pTest, length == test_length);
    for (i = 0; i < test_length; i++) {
        ct_test(pTest, value[i] == test_value[i]);
    }

    test_length = strlen((char *) test_append_value);
    status = octetstring_append(&bacnet_string,
        &test_append_value[0], test_length);
    strcat((char *) test_append_string, (char *) test_value);
    strcat((char *) test_append_string, (char *) test_append_value);
    test_length = strlen((char *) test_append_string);
    ct_test(pTest, status == true);
    length = octetstring_length(&bacnet_string);
    value = octetstring_value(&bacnet_string);
    ct_test(pTest, length == test_length);
    for (i = 0; i < test_length; i++) {
        ct_test(pTest, value[i] == test_append_string[i]);
    }
}
