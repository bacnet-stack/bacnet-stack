/**
 * @file
 * @brief test millisecond timer API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @copyright SPDX-License-Identifier: MIT
 */
#include <limits.h>
#include <zephyr/ztest.h>
#include <bacnet/basic/sys/mstimer.h>

static unsigned long Fake_Milliseconds;
/* Added to Fake_Milliseconds on every mstimer_now() sample. */
static unsigned long Now_Advance;
static unsigned Callback_Fast_Count;
static unsigned Callback_Slow_Count;
static unsigned Callback_Idle_Count;
static struct mstimer_callback_data_t Callback_Fast;
static struct mstimer_callback_data_t Callback_Slow;
static struct mstimer_callback_data_t Callback_Idle;

/**
 * @brief Get the current time in milliseconds
 * @return milliseconds
 */
unsigned long mstimer_now(void)
{
    Fake_Milliseconds += Now_Advance;

    return Fake_Milliseconds;
}

/**
 * @brief Fast repeating callback
 */
static void callback_fast(void)
{
    Callback_Fast_Count++;
}

/**
 * @brief Slow repeating callback
 */
static void callback_slow(void)
{
    Callback_Slow_Count++;
}

/**
 * @brief Zero-interval callback
 */
static void callback_idle(void)
{
    Callback_Idle_Count++;
}

/**
 * @brief Half of the unsigned range used by mstimer_expired()
 * @return half of the unsigned long range
 */
static unsigned long expired_half_range(void)
{
    return ((unsigned long)(~((unsigned long)0))) >> 1;
}

/**
 * @brief Test mstimer_set() and mstimer_interval()
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(mstimer_tests, test_mstimer_set)
#else
static void test_mstimer_set(void)
#endif
{
    struct mstimer timer = { 0 };

    Now_Advance = 0;
    Fake_Milliseconds = 1000;
    mstimer_set(&timer, 5000);
    zassert_equal(mstimer_interval(&timer), 5000, NULL);
    zassert_equal(timer.start, 1000, NULL);
    zassert_equal(mstimer_elapsed(&timer), 0, NULL);
    zassert_equal(mstimer_remaining(&timer), 5000, NULL);
    zassert_false(mstimer_expired(&timer), NULL);

    /* A zero interval does not expire. */
    mstimer_set(&timer, 0);
    Fake_Milliseconds = 5000;
    zassert_equal(mstimer_interval(&timer), 0, NULL);
    zassert_equal(timer.start, 1000, NULL);
    zassert_equal(mstimer_elapsed(&timer), 4000, NULL);
    zassert_false(mstimer_expired(&timer), NULL);

    /* mstimer_set() stores the same sample that mstimer_now() returns. */
    Now_Advance = 5;
    mstimer_set(&timer, 250);
    zassert_equal(timer.start, Fake_Milliseconds, NULL);
    zassert_equal(mstimer_interval(&timer), 250, NULL);
    Now_Advance = 0;
}

/**
 * @brief Test elapsed time, remaining time, and expiry
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(mstimer_tests, test_mstimer_elapsed_remaining_expired)
#else
static void test_mstimer_elapsed_remaining_expired(void)
#endif
{
    struct mstimer timer = { 0 };
    unsigned long half = 0;

    Now_Advance = 0;
    Fake_Milliseconds = 1000;
    mstimer_set(&timer, 5000);
    Fake_Milliseconds = 5999;
    zassert_equal(mstimer_elapsed(&timer), 4999, NULL);
    zassert_equal(mstimer_remaining(&timer), 1, NULL);
    zassert_false(mstimer_expired(&timer), NULL);

    Fake_Milliseconds = 6000;
    zassert_equal(mstimer_elapsed(&timer), 5000, NULL);
    zassert_equal(mstimer_remaining(&timer), 0, NULL);
    zassert_true(mstimer_expired(&timer), NULL);

    Fake_Milliseconds = 6100;
    zassert_equal(mstimer_elapsed(&timer), 5100, NULL);
    zassert_equal(mstimer_remaining(&timer), ULONG_MAX - 99UL, NULL);
    zassert_true(mstimer_expired(&timer), NULL);

    /* Expiry stays true across half the unsigned range, then wraps. */
    half = expired_half_range();
    Fake_Milliseconds = 6000UL + half - 1UL;
    zassert_true(mstimer_expired(&timer), NULL);
    Fake_Milliseconds = 6000UL + half;
    zassert_false(mstimer_expired(&timer), NULL);

    Fake_Milliseconds = ULONG_MAX - 10UL;
    mstimer_set(&timer, 5000);
    Fake_Milliseconds = 25;
    zassert_equal(mstimer_elapsed(&timer), 36, NULL);
}

/**
 * @brief Test mstimer_reset() periodic alignment
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(mstimer_tests, test_mstimer_reset)
#else
static void test_mstimer_reset(void)
#endif
{
    struct mstimer timer = { 0 };

    Now_Advance = 0;
    Fake_Milliseconds = 1000;
    mstimer_set(&timer, 5000);
    /* 100 ms after the deadline. Reset keeps the original period. */
    Fake_Milliseconds = 6100;
    mstimer_reset(&timer);
    zassert_equal(timer.start, 6000, NULL);
    zassert_equal(mstimer_interval(&timer), 5000, NULL);
    zassert_equal(mstimer_elapsed(&timer), 100, NULL);
    zassert_equal(mstimer_remaining(&timer), 4900, NULL);
    zassert_false(mstimer_expired(&timer), NULL);

    /* Reset does not sample the clock, so an early reset moves the
     * deadline forward by one interval. */
    Fake_Milliseconds = 1000;
    mstimer_set(&timer, 5000);
    Fake_Milliseconds = 2000;
    mstimer_reset(&timer);
    zassert_equal(timer.start, 6000, NULL);
    zassert_equal(mstimer_interval(&timer), 5000, NULL);
    zassert_false(mstimer_expired(&timer), NULL);
}

/**
 * @brief Test mstimer_restart() elapsed return and single-sample origin
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(mstimer_tests, test_mstimer_restart)
#else
static void test_mstimer_restart(void)
#endif
{
    struct mstimer timer = { 0 };
    unsigned long elapsed = 0;
    unsigned long interval = 5000;

    Now_Advance = 0;
    Fake_Milliseconds = 1000;
    mstimer_set(&timer, interval);
    zassert_equal(mstimer_interval(&timer), interval, NULL);
    zassert_equal(mstimer_elapsed(&timer), 0, NULL);

    Fake_Milliseconds = 2500;
    zassert_equal(mstimer_elapsed(&timer), 1500, NULL);
    elapsed = mstimer_restart(&timer);
    zassert_equal(elapsed, 1500, NULL);
    zassert_equal(timer.start, Fake_Milliseconds, NULL);
    zassert_equal(mstimer_interval(&timer), interval, NULL);
    zassert_equal(mstimer_elapsed(&timer), 0, NULL);
    zassert_equal(mstimer_remaining(&timer), interval, NULL);

    /* The same lateness that mstimer_reset() absorbs becomes drift
     * when the timer is restarted from now. */
    Fake_Milliseconds = 1000;
    mstimer_set(&timer, interval);
    Fake_Milliseconds = 6100;
    elapsed = mstimer_restart(&timer);
    zassert_equal(elapsed, 5100, NULL);
    zassert_equal(timer.start, 6100, NULL);
    zassert_equal(mstimer_remaining(&timer), interval, NULL);

    /* The clock moves inside this mstimer_now() sample. The returned
     * elapsed time includes that movement, and the new origin does too. */
    Now_Advance = 7;
    elapsed = mstimer_restart(&timer);
    zassert_equal(elapsed, 7, NULL);
    zassert_equal(timer.start, Fake_Milliseconds, NULL);
    Now_Advance = 0;
    zassert_equal(mstimer_elapsed(&timer), 0, NULL);

    /* Unsigned wrap: elapsed is the distance across the rollover. */
    Fake_Milliseconds = ULONG_MAX - 10UL;
    mstimer_set(&timer, interval);
    Fake_Milliseconds = 25;
    zassert_equal(mstimer_elapsed(&timer), 36, NULL);
    elapsed = mstimer_restart(&timer);
    zassert_equal(elapsed, 36, NULL);
    zassert_equal(timer.start, 25, NULL);
    zassert_equal(mstimer_elapsed(&timer), 0, NULL);
}

/**
 * @brief Test mstimer_expire()
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(mstimer_tests, test_mstimer_expire)
#else
static void test_mstimer_expire(void)
#endif
{
    struct mstimer timer = { 0 };

    Now_Advance = 0;
    Fake_Milliseconds = 1000;
    mstimer_set(&timer, 5000);
    zassert_false(mstimer_expired(&timer), NULL);
    mstimer_expire(&timer);
    zassert_equal(timer.start, 1000UL - 5000UL, NULL);
    zassert_equal(mstimer_interval(&timer), 5000, NULL);
    zassert_true(mstimer_expired(&timer), NULL);

    /* A zero interval is unchanged and still does not expire. */
    Fake_Milliseconds = 2000;
    mstimer_set(&timer, 0);
    mstimer_expire(&timer);
    zassert_equal(timer.start, 2000, NULL);
    zassert_equal(mstimer_interval(&timer), 0, NULL);
    zassert_false(mstimer_expired(&timer), NULL);
}

/**
 * @brief Test mstimer_callback() and mstimer_callback_handler()
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(mstimer_tests, test_mstimer_callback)
#else
static void test_mstimer_callback(void)
#endif
{
    Now_Advance = 0;
    Callback_Fast_Count = 0;
    Callback_Slow_Count = 0;
    Callback_Idle_Count = 0;
    Fake_Milliseconds = 10000;

    mstimer_callback(NULL, callback_fast, 1000);
    mstimer_callback(&Callback_Idle, callback_idle, 0);
    mstimer_callback(&Callback_Fast, callback_fast, 1000);
    mstimer_callback(&Callback_Slow, callback_slow, 2500);
    mstimer_callback(NULL, callback_fast, 1000);

    zassert_equal(mstimer_interval(&Callback_Idle.timer), 0, NULL);
    zassert_equal(mstimer_interval(&Callback_Fast.timer), 1000, NULL);
    zassert_equal(mstimer_interval(&Callback_Slow.timer), 2500, NULL);
    zassert_equal_ptr(Callback_Idle.next, &Callback_Fast, NULL);
    zassert_equal_ptr(Callback_Fast.next, &Callback_Slow, NULL);
    zassert_is_null(Callback_Slow.next, NULL);

    mstimer_callback_handler();
    zassert_equal(Callback_Fast_Count, 0, NULL);
    zassert_equal(Callback_Slow_Count, 0, NULL);
    zassert_equal(Callback_Idle_Count, 0, NULL);

    Fake_Milliseconds = 11000;
    mstimer_callback_handler();
    zassert_equal(Callback_Fast_Count, 1, NULL);
    zassert_equal(Callback_Slow_Count, 0, NULL);
    zassert_equal(Callback_Idle_Count, 0, NULL);
    zassert_equal(Callback_Fast.timer.start, 11000, NULL);
    zassert_false(mstimer_expired(&Callback_Fast.timer), NULL);

    mstimer_callback_handler();
    zassert_equal(Callback_Fast_Count, 1, NULL);

    Fake_Milliseconds = 12500;
    mstimer_callback_handler();
    zassert_equal(Callback_Fast_Count, 2, NULL);
    zassert_equal(Callback_Slow_Count, 1, NULL);
    zassert_equal(Callback_Idle_Count, 0, NULL);
    zassert_equal(Callback_Fast.timer.start, 12000, NULL);
    zassert_equal(Callback_Slow.timer.start, 12500, NULL);

    /* One handler call fires a repeating callback once. A timer that
     * is more than one interval late stays expired. */
    Fake_Milliseconds = 15000;
    mstimer_callback_handler();
    zassert_equal(Callback_Fast_Count, 3, NULL);
    zassert_equal(Callback_Fast.timer.start, 13000, NULL);
    zassert_true(mstimer_expired(&Callback_Fast.timer), NULL);
    zassert_equal(Callback_Slow_Count, 2, NULL);
    zassert_equal(Callback_Slow.timer.start, 15000, NULL);
    zassert_equal(Callback_Idle_Count, 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(mstimer_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        mstimer_tests, ztest_unit_test(test_mstimer_set),
        ztest_unit_test(test_mstimer_elapsed_remaining_expired),
        ztest_unit_test(test_mstimer_reset),
        ztest_unit_test(test_mstimer_restart),
        ztest_unit_test(test_mstimer_expire),
        ztest_unit_test(test_mstimer_callback));

    ztest_run_test_suite(mstimer_tests);
}
#endif
