/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet COBS encode/decode APIs
 */

#include <ztest.h>
#include <stdlib.h>
#include <bacnet/datalink/cobs.h>
#include <bacnet/bytes.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test CRC8 from Annex G 1.0 of BACnet Standard
 */
static void test_COBS_Encode_Decode(void)
{
    uint8_t buffer[MAX_APDU] = { 0x55, 0xff, 0 };
    uint8_t encoded_buffer[MAX_APDU] = { 0 };
    uint8_t test_buffer[MAX_APDU] = { 0 };
    unsigned i;
    size_t encoded_buffer_length, test_buffer_length;

    srand(0);
    for (i = 2; i < sizeof(buffer); i++) {
        buffer[i] = rand()%255;
    }
    encoded_buffer_length = cobs_frame_encode(encoded_buffer, buffer, sizeof(buffer));
    test_buffer_length = cobs_frame_decode(test_buffer, encoded_buffer, encoded_buffer_length);
    zassert_true(test_buffer_length == sizeof(buffer), NULL);
    for (i = 0; i < sizeof(buffer); i++) {
        zassert_true(buffer[i] == test_buffer[i], NULL);
    }
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(cobs_tests,
     ztest_unit_test(test_COBS_Encode_Decode)
     );

    ztest_run_test_suite(cobs_tests);
}
