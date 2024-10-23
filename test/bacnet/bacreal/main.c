/**
 * @file
 * @author Steve Karg
 * @date April 2020
 * @brief Test file for a REAL and DOUBLE encode and decode
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
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
