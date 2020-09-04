/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <limits.h>
#include <ztest.h>
#include <bacnet/basic/sys/fifo.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Unit Test for the FIFO buffer
 */
static void testFIFOBuffer(void)
{
    /* FIFO data structure */
    FIFO_BUFFER test_buffer = { 0 };
    /* FIFO data store. Note:  size must be a power of two! */
    volatile uint8_t data_store[64] = { 0 };
    uint8_t add_data[40] = { "RoseSteveLouPatRachelJessicaDaniAmyHerb" };
    uint8_t test_add_data[40] = { 0 };
    uint8_t test_data = 0;
    unsigned index = 0;
    unsigned count = 0;
    bool status = 0;

    FIFO_Init(&test_buffer, data_store, sizeof(data_store));
    zassert_true(FIFO_Empty(&test_buffer), NULL);

    /* load the buffer */
    for (test_data = 0; test_data < sizeof(data_store); test_data++) {
        zassert_false(FIFO_Full(&test_buffer), NULL);
        zassert_true(FIFO_Available(&test_buffer, 1), NULL);
        status = FIFO_Put(&test_buffer, test_data);
        zassert_true(status, NULL);
        zassert_false(FIFO_Empty(&test_buffer), NULL);
    }
    /* not able to put any more */
    zassert_true(FIFO_Full(&test_buffer), NULL);
    zassert_false(FIFO_Available(&test_buffer, 1), NULL);
    status = FIFO_Put(&test_buffer, 42);
    zassert_false(status, NULL);
    /* unload the buffer */
    for (index = 0; index < sizeof(data_store); index++) {
        zassert_false(FIFO_Empty(&test_buffer), NULL);
        test_data = FIFO_Peek(&test_buffer);
        zassert_equal(test_data, index, NULL);
        test_data = FIFO_Get(&test_buffer);
        zassert_equal(test_data, index, NULL);
        zassert_true(FIFO_Available(&test_buffer, 1), NULL);
        zassert_false(FIFO_Full(&test_buffer), NULL);
    }
    zassert_true(FIFO_Empty(&test_buffer), NULL);
    test_data = FIFO_Get(&test_buffer);
    zassert_equal(test_data, 0, NULL);
    test_data = FIFO_Peek(&test_buffer);
    zassert_equal(test_data, 0, NULL);
    zassert_true(FIFO_Empty(&test_buffer), NULL);
    /* test the ring around the buffer */
    for (index = 0; index < sizeof(data_store); index++) {
        zassert_true(FIFO_Empty(&test_buffer), NULL);
        zassert_true(FIFO_Available(&test_buffer, 4), NULL);
        for (count = 1; count < 4; count++) {
            test_data = count;
            status = FIFO_Put(&test_buffer, test_data);
            zassert_true(status, NULL);
            zassert_false(FIFO_Empty(&test_buffer), NULL);
        }
        for (count = 1; count < 4; count++) {
            zassert_false(FIFO_Empty(&test_buffer), NULL);
            test_data = FIFO_Peek(&test_buffer);
            zassert_equal(test_data, count, NULL);
            test_data = FIFO_Get(&test_buffer);
            zassert_equal(test_data, count, NULL);
        }
    }
    zassert_true(FIFO_Empty(&test_buffer), NULL);
    /* test Add */
    zassert_true(FIFO_Available(&test_buffer, sizeof(add_data)), NULL);
    status = FIFO_Add(&test_buffer, add_data, sizeof(add_data));
    zassert_true(status, NULL);
    count = FIFO_Count(&test_buffer);
    zassert_equal(count, sizeof(add_data), NULL);
    zassert_false(FIFO_Empty(&test_buffer), NULL);
    for (index = 0; index < sizeof(add_data); index++) {
        /* unload the buffer */
        zassert_false(FIFO_Empty(&test_buffer), NULL);
        test_data = FIFO_Peek(&test_buffer);
        zassert_equal(test_data, add_data[index], NULL);
        test_data = FIFO_Get(&test_buffer);
        zassert_equal(test_data, add_data[index], NULL);
    }
    zassert_true(FIFO_Empty(&test_buffer), NULL);
    /* test Pull */
    zassert_true(FIFO_Available(&test_buffer, sizeof(add_data)), NULL);
    status = FIFO_Add(&test_buffer, add_data, sizeof(add_data));
    zassert_true(status, NULL);
    count = FIFO_Count(&test_buffer);
    zassert_equal(count, sizeof(add_data), NULL);
    zassert_false(FIFO_Empty(&test_buffer), NULL);
    count = FIFO_Pull(&test_buffer, &test_add_data[0], sizeof(test_add_data));
    zassert_true(FIFO_Empty(&test_buffer), NULL);
    zassert_equal(count, sizeof(test_add_data), NULL);
    for (index = 0; index < sizeof(add_data); index++) {
        zassert_equal(test_add_data[index], add_data[index], NULL);
    }
    zassert_true(FIFO_Available(&test_buffer, sizeof(add_data)), NULL);
    status = FIFO_Add(&test_buffer, test_add_data, sizeof(add_data));
    zassert_true(status, NULL);
    zassert_false(FIFO_Empty(&test_buffer), NULL);
    for (index = 0; index < sizeof(add_data); index++) {
        count = FIFO_Pull(&test_buffer, &test_add_data[0], 1);
        zassert_equal(count, 1, NULL);
        zassert_equal(test_add_data[0], add_data[index], NULL);
    }
    zassert_true(FIFO_Empty(&test_buffer), NULL);
    /* test flush */
    status = FIFO_Add(&test_buffer, test_add_data, sizeof(test_add_data));
    zassert_true(status, NULL);
    zassert_false(FIFO_Empty(&test_buffer), NULL);
    FIFO_Flush(&test_buffer);
    zassert_true(FIFO_Empty(&test_buffer), NULL);

    return;
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(fifo_tests,
     ztest_unit_test(testFIFOBuffer)
     );

    ztest_run_test_suite(fifo_tests);
}
