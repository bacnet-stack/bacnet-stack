/**
 * @file
 * @details Test the uBASIC implementation
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2025
 * @brief Platform libc and compiler abstraction layer
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <zephyr/ztest.h>
#include <bacnet/bacdef.h>
#include <bacnet/basic/program/ubasic/ubasic.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

static uint32_t Tick_Counter = 0;
static void tick_increment(void)
{
    Tick_Counter++;
}
static uint32_t tick_now(void)
{
    return Tick_Counter;
}

/**
 * @brief Write a buffer to the serial port
 * @param msg Pointer to the buffer to write
 * @param n Number of bytes to write
 */
static void serial_write(const char *msg, uint16_t n)
{
    printf("%.*s", n, msg);
    fflush(stdout);
}

/**
 * @brief Generate a random number
 * @param size Size of the random number in bits
 * @return Random number size-bits wide
 */
static uint32_t random_uint32(uint8_t size)
{
    uint32_t r = 0;

    for (int i = 0; i < size; i++) {
        r |= (1 << i);
    }

    return r;
}

static uint16_t Test_BACnet_Object_Type;
static uint32_t Test_BACnet_Object_Instance;
static uint32_t Test_BACnet_Object_Property_ID;
static VARIABLE_TYPE Test_BACnet_Object_Property_Value;
static char *Test_BACnet_Object_Name;

static void
bacnet_create_object(uint16_t object_type, uint32_t instance, char *object_name)
{
    Test_BACnet_Object_Type = object_type;
    Test_BACnet_Object_Instance = instance;
    Test_BACnet_Object_Name = object_name;
}

static void bacnet_write_property(
    uint16_t object_type,
    uint32_t instance,
    uint32_t property_id,
    VARIABLE_TYPE value)
{
    Test_BACnet_Object_Type = object_type;
    Test_BACnet_Object_Instance = instance;
    Test_BACnet_Object_Property_ID = property_id;
    Test_BACnet_Object_Property_Value = value;
}

static VARIABLE_TYPE bacnet_read_property(
    uint16_t object_type, uint32_t instance, uint32_t property_id)
{
    Test_BACnet_Object_Type = object_type;
    Test_BACnet_Object_Instance = instance;
    Test_BACnet_Object_Property_ID = property_id;
    return Test_BACnet_Object_Property_Value;
}

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ubasic_tests, test_ubasic_bacnet)
#else
static void test_ubasic_bacnet(void)
#endif
{
    struct ubasic_data data = { 0 };
    const char *program =
        /* program listing with either \0, \n, or ';' at the end of each line.
           note: indentation is not required */
        "println 'Demo - BACnet';"
        "bac_create(0, 1234, 'Object1');"
        "bac_write(0, 1234, 85, 42);"
        "a = bac_read(0, 1234, 85);"
        "println 'bac_read 0, 1234, 85 = ' a;"
        "end;";
    VARIABLE_TYPE value = 0;
    data.mstimer_now = tick_now;
    data.serial_write = serial_write;
    data.bacnet_create_object = bacnet_create_object;
    data.bacnet_write_property = bacnet_write_property;
    data.bacnet_read_property = bacnet_read_property;
    ubasic_load_program(&data, program);
    zassert_equal(data.status.bit.isRunning, 1, NULL);
    zassert_equal(data.status.bit.Error, 0, NULL);
    while (!ubasic_finished(&data)) {
        ubasic_run_program(&data);
        tick_increment();
    }
    zassert_equal(data.status.bit.Error, 0, NULL);
    /* check the final value of the bacnet read property */
    value = ubasic_get_variable(&data, 'a');
    zassert_equal(
        fixedpt_toint(value), 42, "bacnet read property value=%d",
        fixedpt_toint(value));
    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ubasic_tests, test_ubasic_math)
#else
static void test_ubasic_math(void)
#endif
{
    struct ubasic_data data = { 0 };
    const char *program =
        /* program listing with either \0, \n, or ';' at the end of each line.
           note: indentation is not required */
        "println 'Demo - Math';"
        "for i = 1 to 2;"
        "  j = i + 0.25 + 1/2;"
        "  k = sqrt(2*j) + ln(4*i) + cos(i+j) + sin(j);"
        "next i;"
        "println 'j=' j;"
        "println 'k=' k;"
        "dim r@(5);"
        "for i = 1 to 5;"
        "  r@(i) = ran;"
        "  println 'r[' i ']=' r@(i);"
        "next i;"
        "dim u@(5);"
        "a = 0;"
        "for i = 1 to 5;"
        "  u = uniform;"
        "  println 'u[' i ']=' u;"
        "  u@(i) = u;"
        "  a = avgw(u,a,5);"
        "next i;"
        "println 'uniform moving average = ' a;"
        "x = 1000 * uniform;"
        "f = floor(x);"
        "c = ceil(x);"
        "r = round(x);"
        "w = pow(x,3);"
        "println 'x=' x;"
        "println 'floor(x)=' f;"
        "println 'ceil(x)=' c;"
        "println 'round(x)=' r;"
        "println 'x^3=' w;"
        "end;";

    VARIABLE_TYPE value = 0, xvalue = 0, arrayvalue[5];
    unsigned i;
    int32_t value_int = 0;
    int32_t value_frac = 0;
    int32_t value_math = 0;

    data.mstimer_now = tick_now;
    data.serial_write = serial_write;
    data.random_uint32 = random_uint32;
    ubasic_load_program(&data, program);
    zassert_equal(data.status.bit.isRunning, 1, NULL);
    zassert_equal(data.status.bit.Error, 0, NULL);
    while (!ubasic_finished(&data)) {
        ubasic_run_program(&data);
        tick_increment();
    }
    zassert_equal(data.status.bit.Error, 0, NULL);
    /* check the final values of the math operations */
    value = ubasic_get_variable(&data, 'j');
    value_int = fixedpt_toint(value);
    value_frac = fixedpt_fracpart_floor_toint(value, 2);
    zassert_equal(value_int, 2, "int=%d", value_int);
    zassert_equal(value_frac, 75, "frac=%d", value_frac);
    value = ubasic_get_variable(&data, 'k');
    value_int = fixedpt_toint(value);
    value_frac = fixedpt_fracpart_floor_toint(value, 2);
    zassert_equal(value_int, 4, "int=%d", value_int);
    zassert_equal(value_frac, 83, "frac=%d", value_frac);
    for (i = 0; i < ARRAY_SIZE(arrayvalue); i++) {
        arrayvalue[i] = ubasic_get_arrayvariable(&data, 'r', 1 + i);
        value_int = fixedpt_toint(arrayvalue[i]);
        zassert_equal(value_int, 1, "ran[%u]=%d", 1 + i, value_int);
    }
    /* uniform random value */
    xvalue = ubasic_get_variable(&data, 'x');
    /* floor */
    value_math = fixedpt_floor_toint(xvalue);
    value = ubasic_get_variable(&data, 'f');
    value_int = fixedpt_toint(value);
    zassert_equal(
        value_int, value_math, "int=%d floor=%d", value_int, value_math);
    /* ceiling */
    value_math = fixedpt_ceil_toint(xvalue);
    value = ubasic_get_variable(&data, 'c');
    value_int = fixedpt_toint(value);
    zassert_equal(
        value_int, value_math, "int=%d ceil=%d", value_int, value_math);
    /* round */
    value_math = fixedpt_round_toint(xvalue);
    value = ubasic_get_variable(&data, 'r');
    value_int = fixedpt_toint(value);
    zassert_equal(
        value_int, value_math, "int=%d round=%d", value_int, value_math);

    return;
}

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ubasic_tests, test_ubasic)
#else
static void test_ubasic(void)
#endif
{
    struct ubasic_data data = { 0 };
    const char *program =
        /* program listing with either \0, \n, or ';' at the end of each line.
           note: indentation is not required */
        "println 'Demo - Flow';"
        "gosub l1;"
        "for i = 1 to 8;"
        "  for j = 1 to 9;"
        "    println 'i,j=',i,j;"
        "  next j;"
        "next i;"
        "println 'Demo 1 Completed';"
        "end;"
        ":l1 "
        "  println 'subroutine';"
        "return;";
    VARIABLE_TYPE value = 0;
    int32_t value_int = 0;

    ubasic_load_program(&data, program);
    zassert_equal(data.status.bit.isRunning, 1, NULL);
    zassert_equal(data.status.bit.Error, 0, NULL);
    while (!ubasic_finished(&data)) {
        ubasic_run_program(&data);
    }
    zassert_equal(data.status.bit.Error, 0, NULL);
    /* check the final value of i and j */
    value = ubasic_get_variable(&data, 'i');
    value_int = fixedpt_toint(value);
    zassert_equal(value_int, 9, NULL);
    value = ubasic_get_variable(&data, 'j');
    value_int = fixedpt_toint(value);
    zassert_equal(value_int, 10, NULL);

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(ubasic_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        ubasic_tests, ztest_unit_test(test_ubasic),
        ztest_unit_test(test_ubasic_math), ztest_unit_test(test_ubasic_bacnet));

    ztest_run_test_suite(ubasic_tests);
}
#endif
