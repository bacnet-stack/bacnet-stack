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

static int32_t ADC_Value[UINT8_MAX + 1];
static int32_t adc_read(uint8_t channel)
{
    return ADC_Value[channel];
}
static void adc_config(uint8_t sampletime, uint8_t nreads)
{
    (void)sampletime;
    (void)nreads;
}
static uint32_t Event_Mask;
static int8_t hw_event(uint8_t bit)
{
    if (bit < 32) {
        if (Event_Mask & (1UL << bit)) {
            return 1; // Event is set
        }
    }

    return 0; // Event is not set
}
static void hw_event_clear(uint8_t bit)
{
    if (bit < 32) {
        Event_Mask &= ~(1UL << bit);
    }
}
static uint8_t GPIO_Pin_State[UINT8_MAX + 1];
static void gpio_write(uint8_t ch, uint8_t pin_state)
{
    GPIO_Pin_State[ch] = pin_state;
}
static void gpio_config(uint8_t ch, int8_t mode, uint8_t freq)
{
    (void)ch;
    (void)mode;
    (void)freq;
}
static void pwm_config(uint16_t psc, uint16_t per)
{
    (void)psc;
    (void)per;
}

static int32_t Duty_Cycle[UINT8_MAX + 1];
static void pwm_write(uint8_t ch, int32_t dutycycle)
{
    Duty_Cycle[ch] = dutycycle;
}
static int32_t pwm_read(uint8_t ch)
{
    return Duty_Cycle[ch];
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

struct test_bacnet_object {
    uint16_t object_type;
    uint32_t object_instance;
    uint32_t property_id;
    VARIABLE_TYPE property_value;
    char *object_name;
};
static struct test_bacnet_object Test_BACnet_Object[5];

static void
bacnet_create_object(uint16_t object_type, uint32_t instance, char *object_name)
{
    if (instance < ARRAY_SIZE(Test_BACnet_Object)) {
        Test_BACnet_Object[instance].object_type = object_type;
        Test_BACnet_Object[instance].object_instance = instance;
        Test_BACnet_Object[instance].object_name = strdup(object_name);
    }
}

static void bacnet_write_property(
    uint16_t object_type,
    uint32_t instance,
    uint32_t property_id,
    VARIABLE_TYPE value)
{
    if (instance < ARRAY_SIZE(Test_BACnet_Object)) {
        Test_BACnet_Object[instance].object_type = object_type;
        Test_BACnet_Object[instance].object_instance = instance;
        Test_BACnet_Object[instance].property_id = property_id;
        Test_BACnet_Object[instance].property_value = value;
    }
}

static VARIABLE_TYPE bacnet_read_property(
    uint16_t object_type, uint32_t instance, uint32_t property_id)
{
    if (instance < ARRAY_SIZE(Test_BACnet_Object)) {
        if (Test_BACnet_Object[instance].object_type == object_type &&
            Test_BACnet_Object[instance].property_id == property_id) {
            return Test_BACnet_Object[instance].property_value;
        }
    }
    return 0;
}

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ubasic_tests, test_ubasic_gpio)
#else
static void test_ubasic_gpio(void)
#endif
{
    struct ubasic_data data = { 0 };
    const char *program =
        /* program listing with either \0, \n, or ';' at the end of each line.
           note: indentation is not required */
        "println 'Demo - GPIO & ADC';"
        "pinmode(0xc0,-1,0);"
        "pinmode(0xc1,-1,0);"
        "pinmode(0xc2,-1,0);"
        "pinmode(0xc3,-1,0);"
        "for j = 0 to 2;"
        "  dwrite(0xc0,(j % 2));"
        "  dwrite(0xc1,(j % 2));"
        "  dwrite(0xc2,(j % 2));"
        "  dwrite(0xc3,(j % 2));"
        "  sleep(0.5);"
        "next j;"
        "aread_conf(7,16);"
        "aread_conf(7,17);"
        "a = 4096 / 2;"
        "z = 4096 / 2;"
        "s = 5;"
        "for i = 1 to s;"
        "  x = aread(16);"
        "  y = aread(17);"
        "  println 'VREF,TEMP=', x, y;"
        "  a = avgw(x,a,s);"
        "  z = avgw(y,z,s);"
        "next i;"
        "println 'average x y=', a, z;"
        "end;";
    VARIABLE_TYPE value = 0;
    data.mstimer_now = tick_now;
    data.serial_write = serial_write;
    data.gpio_config = gpio_config;
    data.gpio_write = gpio_write;
    data.adc_config = adc_config;
    data.adc_read = adc_read;
    data.pwm_config = pwm_config;
    data.pwm_write = pwm_write;
    data.pwm_read = pwm_read;
    data.hw_event = hw_event;
    data.hw_event_clear = hw_event_clear;
    ADC_Value[16] = 2048;
    ADC_Value[17] = 2048;
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
        fixedpt_toint(value), 2048, "a value=%d", fixedpt_toint(value));
    value = ubasic_get_variable(&data, 'z');
    zassert_equal(
        fixedpt_toint(value), 2048, "z value=%d", fixedpt_toint(value));
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
        "bac_create(0, 1, 'Object1');"
        "bac_create(0, 2, 'Object2');"
        "bac_create(0, 3, 'Object3');"
        "bac_create(0, 4, 'Object4');"
        "bac_write(0, 1, 85, 42);"
        "a = bac_read(0, 1, 85);"
        "println 'bac_read 0, 1, 85 = ' a;"
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
    zassert_equal(
        Test_BACnet_Object[1].object_type, 0, "bacnet object type=%d",
        Test_BACnet_Object[1].object_type);
    zassert_equal(
        Test_BACnet_Object[1].object_instance, 1, "bacnet object instance=%d",
        Test_BACnet_Object[1].object_instance);
    zassert_equal(
        Test_BACnet_Object[1].property_id, 85, "bacnet object property ID=%d",
        Test_BACnet_Object[1].property_id);
    zassert_equal(
        strcmp(Test_BACnet_Object[1].object_name, "Object1"), 0,
        "bacnet object name=%s", Test_BACnet_Object[1].object_name);
    zassert_equal(
        strcmp(Test_BACnet_Object[2].object_name, "Object2"), 0,
        "bacnet object name=%s", Test_BACnet_Object[2].object_name);
    zassert_equal(
        strcmp(Test_BACnet_Object[3].object_name, "Object3"), 0,
        "bacnet object name=%s", Test_BACnet_Object[3].object_name);
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
        ztest_unit_test(test_ubasic_math), ztest_unit_test(test_ubasic_bacnet),
        ztest_unit_test(test_ubasic_gpio));

    ztest_run_test_suite(ubasic_tests);
}
#endif
