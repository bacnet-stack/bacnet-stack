/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet COBS encode/decode APIs
 */

#include <zephyr/ztest.h>
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
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(cobs_tests, test_COBS_Encode_Decode)
#else
static void test_COBS_Encode_Decode(void)
#endif
{
    uint8_t buffer[MAX_APDU] = { 0x55, 0xff, 0 };
    uint8_t encoded_buffer[COBS_ENCODED_SIZE(MAX_APDU)+
        COBS_ENCODED_CRC_SIZE] = { 0 };
    //uint8_t encoded_buffer[MAX_APDU*2] = { 0 };
    uint8_t test_buffer[MAX_APDU] = { 0 };
    unsigned i;
    size_t encoded_buffer_length, test_buffer_length;

    for (i = 2; i < sizeof(buffer); i++) {
        buffer[i] = i%0xff;
    }
    encoded_buffer_length = cobs_frame_encode(encoded_buffer,
        sizeof(encoded_buffer), buffer, sizeof(buffer));
    zassert_true(encoded_buffer_length > 0,
        "COBS encoded buffer empty!");
    test_buffer_length = cobs_frame_decode(test_buffer, sizeof(test_buffer),
        encoded_buffer, encoded_buffer_length);
    for (i = 0; i < sizeof(buffer); i++) {
        zassert_true(buffer[i] == test_buffer[i],
            "COBS encode/decode fail");
    }
    zassert_true(test_buffer_length == sizeof(buffer),
        "COBS encode/decode length fail");
}
/**
 * @}
 */


#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(cobs_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(cobs_tests,
     ztest_unit_test(test_COBS_Encode_Decode)
     );

    ztest_run_test_suite(cobs_tests);
}
#endif
