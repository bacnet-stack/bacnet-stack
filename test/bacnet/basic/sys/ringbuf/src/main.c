/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <limits.h>
#include <zephyr/ztest.h>
#include <bacnet/basic/sys/ringbuf.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
/**
 * Unit Test for the ring buffer
 *
 * @param  test_buffer - pointer to RING_BUFFER structure
 * @param  data_element - one data element
 * @param  element_size - size of one data element
 * @param  element_count - number of data elements in the store
 */
static void testRingAroundBuffer(
    RING_BUFFER *test_buffer,
    uint8_t *data_element,
    unsigned element_size,
    unsigned element_count)
{
    volatile uint8_t *test_data;
    unsigned index;
    unsigned data_index;
    unsigned count;
    uint8_t value;
    bool status;

    zassert_true(Ringbuf_Empty(test_buffer), NULL);
    /* test the ring around the buffer */
    for (index = 0; index < element_count; index++) {
        for (count = 1; count < 4; count++) {
            value = (index * count) % 255;
            for (data_index = 0; data_index < element_size; data_index++) {
                data_element[data_index] = value;
            }
            status = Ringbuf_Put(test_buffer, data_element);
            zassert_true(status, NULL);
            zassert_equal(Ringbuf_Count(test_buffer), count, NULL);
        }
        for (count = 1; count < 4; count++) {
            value = (index * count) % 255;
            test_data = Ringbuf_Peek(test_buffer);
            zassert_not_null(test_data, NULL);
            if (test_data) {
                for (data_index = 0; data_index < element_size; data_index++) {
                    zassert_equal(test_data[data_index], value, NULL);
                }
            }
            status = Ringbuf_Pop(test_buffer, NULL);
            zassert_true(status, NULL);
        }
    }
    zassert_true(Ringbuf_Empty(test_buffer), NULL);
}

/**
 * Unit Test for the ring buffer
 *
 * @param  data_store - buffer to store elements
 * @param  data_element - one data element
 * @param  element_size - size of one data element
 * @param  element_count - number of data elements in the store
 */
static void testRingBuf(
    uint8_t *data_store,
    unsigned data_store_size,
    uint8_t *data_element,
    unsigned element_size,
    unsigned element_count)
{
    RING_BUFFER test_buffer;
    volatile uint8_t *test_data;
    unsigned index;
    unsigned data_index;
    bool status;

    status = Ringbuf_Initialize(
        &test_buffer, data_store, data_store_size, element_size, element_count);
    if (!status) {
        return;
    }
    zassert_true(Ringbuf_Empty(&test_buffer), NULL);
    zassert_equal(Ringbuf_Depth(&test_buffer), 0, NULL);

    for (data_index = 0; data_index < element_size; data_index++) {
        data_element[data_index] = data_index;
    }
    status = Ringbuf_Put(&test_buffer, data_element);
    zassert_true(status, NULL);
    zassert_false(Ringbuf_Empty(&test_buffer), NULL);
    zassert_equal(Ringbuf_Depth(&test_buffer), 1, NULL);

    test_data = Ringbuf_Peek(&test_buffer);
    for (data_index = 0; data_index < element_size; data_index++) {
        zassert_equal(test_data[data_index], data_element[data_index], NULL);
    }
    zassert_false(Ringbuf_Empty(&test_buffer), NULL);
    (void)Ringbuf_Pop(&test_buffer, NULL);
    zassert_true(Ringbuf_Empty(&test_buffer), NULL);
    zassert_equal(Ringbuf_Depth(&test_buffer), 1, NULL);

    /* fill to max */
    for (index = 0; index < element_count; index++) {
        for (data_index = 0; data_index < element_size; data_index++) {
            data_element[data_index] = index;
        }
        status = Ringbuf_Put(&test_buffer, data_element);
        zassert_true(status, NULL);
        zassert_false(Ringbuf_Empty(&test_buffer), NULL);
        zassert_equal(Ringbuf_Depth(&test_buffer), (index + 1), NULL);
    }
    zassert_equal(Ringbuf_Depth(&test_buffer), element_count, NULL);
    /* verify actions on full buffer */
    for (index = 0; index < element_count; index++) {
        for (data_index = 0; data_index < element_size; data_index++) {
            data_element[data_index] = index;
        }
        status = Ringbuf_Put(&test_buffer, data_element);
        zassert_false(status, NULL);
        zassert_false(Ringbuf_Empty(&test_buffer), NULL);
        zassert_equal(Ringbuf_Depth(&test_buffer), element_count, NULL);
    }

    /* check buffer full */
    for (index = 0; index < element_count; index++) {
        test_data = Ringbuf_Peek(&test_buffer);
        zassert_not_null(test_data, NULL);
        if (test_data) {
            for (data_index = 0; data_index < element_size; data_index++) {
                zassert_equal(test_data[data_index], index, NULL);
            }
        }
        (void)Ringbuf_Pop(&test_buffer, NULL);
    }
    zassert_true(Ringbuf_Empty(&test_buffer), NULL);
    zassert_equal(Ringbuf_Depth(&test_buffer), element_count, NULL);
    Ringbuf_Depth_Reset(&test_buffer);
    zassert_equal(Ringbuf_Depth(&test_buffer), 0, NULL);

    testRingAroundBuffer(
        &test_buffer, data_element, element_size, element_count);

    /* adjust the internal index of Ringbuf to test unsigned wrapping */
    test_buffer.head = UINT_MAX - 1;
    test_buffer.tail = UINT_MAX - 1;

    testRingAroundBuffer(
        &test_buffer, data_element, element_size, element_count);
}

/**
 * Unit Test for the ring buffer with 16 data elements
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ringbuf_tests, testRingBufSizeSmall)
#else
static void testRingBufSizeSmall(void)
#endif
{
    uint8_t data_element[5];
    uint8_t data_store[sizeof(data_element) * NEXT_POWER_OF_2(16)];

    testRingBuf(
        data_store, sizeof(data_store), data_element, sizeof(data_element),
        sizeof(data_store) / sizeof(data_element));
}

/**
 * Unit Test for the ring buffer with 32 data elements
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ringbuf_tests, testRingBufSizeLarge)
#else
static void testRingBufSizeLarge(void)
#endif
{
    uint8_t data_element[16];
    uint8_t data_store[sizeof(data_element) * NEXT_POWER_OF_2(99)];

    testRingBuf(
        data_store, sizeof(data_store), data_element, sizeof(data_element),
        sizeof(data_store) / sizeof(data_element));
}

/**
 * Unit Test for the ring buffer with 32 data elements
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ringbuf_tests, testRingBufSizeInvalid)
#else
static void testRingBufSizeInvalid(void)
#endif
{
    RING_BUFFER test_buffer;
    uint8_t data_element[16];
    uint8_t data_store[sizeof(data_element) * 99];

    zassert_false(
        Ringbuf_Initialize(
            &test_buffer, data_store, sizeof(data_store), sizeof(data_element),
            sizeof(data_store) / sizeof(data_element)),
        NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ringbuf_tests, testRingBufPowerOfTwo)
#else
static void testRingBufPowerOfTwo(void)
#endif
{
    zassert_equal(NEXT_POWER_OF_2(3), 4, NULL);
    zassert_equal(NEXT_POWER_OF_2(100), 128, NULL);
    zassert_equal(NEXT_POWER_OF_2(127), 128, NULL);
    zassert_equal(NEXT_POWER_OF_2(128), 128, NULL);
    zassert_equal(NEXT_POWER_OF_2(129), 256, NULL);
    zassert_equal(NEXT_POWER_OF_2(300), 512, NULL);
    zassert_equal(NEXT_POWER_OF_2(500), 512, NULL);
}

/**
 * Unit Test for the ring buffer peek/pop next element
 *
 * @param  data_store - buffer to store elements
 * @param  data_element - one data element
 * @param  element_size - size of one data element
 * @param  element_count - number of data elements in the store
 */
static bool testRingBufNextElement(
    uint8_t *data_store,
    unsigned data_store_size,
    uint8_t *data_element,
    unsigned element_size,
    unsigned element_count)
{
    RING_BUFFER test_buffer;
    volatile uint8_t *test_data;
    unsigned index;
    unsigned data_index;
    bool status;
    status = Ringbuf_Initialize(
        &test_buffer, data_store, data_store_size, element_size, element_count);
    if (!status) {
        return false;
    }
    zassert_true(Ringbuf_Empty(&test_buffer), NULL);
    zassert_equal(Ringbuf_Depth(&test_buffer), 0, NULL);

    for (data_index = 0; data_index < element_size; data_index++) {
        data_element[data_index] = data_index;
    }
    status = Ringbuf_Put(&test_buffer, data_element);
    zassert_true(status, NULL);
    zassert_false(Ringbuf_Empty(&test_buffer), NULL);
    zassert_equal(Ringbuf_Depth(&test_buffer), 1, NULL);

    test_data = Ringbuf_Peek(&test_buffer);
    for (data_index = 0; data_index < element_size; data_index++) {
        zassert_equal(test_data[data_index], data_element[data_index], NULL);
    }
    zassert_false(Ringbuf_Empty(&test_buffer), NULL);
    (void)Ringbuf_Pop(&test_buffer, NULL);
    zassert_true(Ringbuf_Empty(&test_buffer), NULL);
    zassert_equal(Ringbuf_Depth(&test_buffer), 1, NULL);

    /* fill to max */
    for (index = 0; index < element_count; index++) {
        for (data_index = 0; data_index < element_size; data_index++) {
            data_element[data_index] = index;
        }
        status = Ringbuf_Put(&test_buffer, data_element);
        zassert_true(status, NULL);
        zassert_false(Ringbuf_Empty(&test_buffer), NULL);
        zassert_equal(Ringbuf_Depth(&test_buffer), (index + 1), NULL);
    }
    zassert_equal(Ringbuf_Depth(&test_buffer), element_count, NULL);
    zassert_equal(Ringbuf_Count(&test_buffer), element_count, NULL);

    /* Walk through ring buffer */
    test_data = Ringbuf_Peek(&test_buffer);
    zassert_not_null(test_data, NULL);
    for (index = 1; index < element_count; index++) {
        test_data = Ringbuf_Peek_Next(&test_buffer, (uint8_t *)test_data);
        zassert_not_null(test_data, NULL);
        if (test_data) {
            for (data_index = 0; data_index < element_size; data_index++) {
                zassert_equal(test_data[data_index], index, NULL);
            }
        }
    }
    zassert_equal(Ringbuf_Count(&test_buffer), element_count, NULL);
    /* Try to walk off end of buffer - should return NULL */
    test_data = Ringbuf_Peek_Next(&test_buffer, (uint8_t *)test_data);
    zassert_is_null(test_data, NULL);

    /* Walk through ring buffer and pop alternate elements */
    test_data = Ringbuf_Peek(&test_buffer);
    zassert_not_null(test_data, NULL);
    for (index = 1; index < element_count / 2; index++) {
        test_data = Ringbuf_Peek_Next(&test_buffer, (uint8_t *)test_data);
        zassert_not_null(test_data, NULL);
        (void)Ringbuf_Pop_Element(&test_buffer, (uint8_t *)test_data, NULL);
        test_data = Ringbuf_Peek_Next(&test_buffer, (uint8_t *)test_data);
    }
    zassert_equal(Ringbuf_Count(&test_buffer), element_count / 2 + 1, NULL);

    /* Walk through ring buffer and check data */
    test_data = Ringbuf_Peek(&test_buffer);
    zassert_not_null(test_data, NULL);
    for (index = 0; index < element_count / 2; index++) {
        if (test_data) {
            for (data_index = 0; data_index < element_size; data_index++) {
                zassert_equal(test_data[data_index], index * 2, NULL);
            }
        }
        test_data = Ringbuf_Peek_Next(&test_buffer, (uint8_t *)test_data);
        zassert_not_null(test_data, NULL);
    }
    zassert_equal(Ringbuf_Count(&test_buffer), element_count / 2 + 1, NULL);

    return true;
}

/**
 * Unit Test for the ring buffer with 16 data elements
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ringbuf_tests, testRingBufNextElementSizeSmall)
#else
static void testRingBufNextElementSizeSmall(void)
#endif
{
    bool status;
    uint8_t data_element[5];
    uint8_t data_store[sizeof(data_element) * NEXT_POWER_OF_2(16)];

    status = testRingBufNextElement(
        data_store, sizeof(data_store), data_element, sizeof(data_element),
        sizeof(data_store) / sizeof(data_element));
    zassert_true(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(ringbuf_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        ringbuf_tests, ztest_unit_test(testRingBufPowerOfTwo),
        ztest_unit_test(testRingBufSizeSmall),
        ztest_unit_test(testRingBufSizeLarge),
        ztest_unit_test(testRingBufSizeInvalid),
        ztest_unit_test(testRingBufNextElementSizeSmall));

    ztest_run_test_suite(ringbuf_tests);
}
#endif
