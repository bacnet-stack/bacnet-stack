/**
 * @file
 * @author Steve Karg
 * @date April 2020
 * @brief Test file for a REAL and DOUBLE encode and decode
 *
 * @section LICENSE
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
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include "bacnet/bacreal.h"
#include "ctest.h"

static void test_setup(void)
{
}

static void test_cleanup(void)
{
}

void testBACreal(Test *pTest)
{
    float real_value = 3.14159F, test_real_value = 0.0;
    uint8_t apdu[50] = { 0 };
    int len = 0, test_len = 0;

    test_setup();
    len = encode_bacnet_real(real_value, &apdu[0]);
    ct_test(pTest, len == 4);
    test_len = decode_real(&apdu[0], &test_real_value);
    ct_test(pTest, test_len == len);
    ct_test(pTest, test_real_value == real_value);
    test_cleanup();
}

void testBACdouble(Test *pTest)
{
    double double_value = 3.1415927, test_double_value = 0.0;
    uint8_t apdu[50] = { 0 };
    int len = 0, test_len = 0;

    test_setup();
    len = encode_bacnet_double(double_value, &apdu[0]);
    ct_test(pTest, len == 8);
    test_len = decode_double(&apdu[0], &test_double_value);
    ct_test(pTest, test_len == len);
    ct_test(pTest, test_double_value == double_value);
    test_cleanup();
}

int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACreal", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testBACreal);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACdouble);
    assert(rc);

    /* configure output */
    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void)ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
