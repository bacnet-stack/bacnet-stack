/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/datalink/crc.h>
#include <bacnet/basic/sys/bytes.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test CRC8 from Annex G 1.0 of BACnet Standard
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(crc_tests, testCRC8)
#else
static void testCRC8(void)
#endif
{
    uint8_t crc = 0xff; /* accumulates the crc value */
    uint8_t frame_crc; /* appended to the end of the frame */

    crc = CRC_Calc_Header(0x00, crc);
    zassert_equal(crc, 0x55, NULL);
    crc = CRC_Calc_Header(0x10, crc);
    zassert_equal(crc, 0xC2, NULL);
    crc = CRC_Calc_Header(0x05, crc);
    zassert_equal(crc, 0xBC, NULL);
    crc = CRC_Calc_Header(0x00, crc);
    zassert_equal(crc, 0x95, NULL);
    crc = CRC_Calc_Header(0x00, crc);
    zassert_equal(crc, 0x73, NULL);
    /* send the ones complement of the CRC in place of */
    /* the CRC, and the resulting CRC will always equal 0x55. */
    frame_crc = ~crc;
    zassert_equal(frame_crc, 0x8C, NULL);
    /* use the ones complement value and the next to last CRC value */
    crc = CRC_Calc_Header(frame_crc, crc);
    zassert_equal(crc, 0x55, NULL);
}

/**
 * @brief Test CRC8 from Annex G 2.0 of BACnet Standard
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(crc_tests, testCRC16)
#else
static void testCRC16(void)
#endif
{
    uint16_t crc = 0xffff;
    uint16_t data_crc;

    crc = CRC_Calc_Data(0x01, crc);
    zassert_equal(crc, 0x1E0E, NULL);
    crc = CRC_Calc_Data(0x22, crc);
    zassert_equal(crc, 0xEB70, NULL);
    crc = CRC_Calc_Data(0x30, crc);
    zassert_equal(crc, 0x42EF, NULL);
    /* send the ones complement of the CRC in place of */
    /* the CRC, and the resulting CRC will always equal 0xF0B8. */
    data_crc = ~crc;
    zassert_equal(data_crc, 0xBD10, NULL);
    crc = CRC_Calc_Data(LO_BYTE(data_crc), crc);
    zassert_equal(crc, 0x0F3A, NULL);
    crc = CRC_Calc_Data(HI_BYTE(data_crc), crc);
    zassert_equal(crc, 0xF0B8, NULL);
}

/**
 * @brief "Test" to create/log generated CRC8 table
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(crc_tests, testCRC8CreateTable)
#else
static void testCRC8CreateTable(void)
#endif
{
    uint8_t crc = 0xff; /* accumulates the crc value */
    int i;

    printf("static const uint8_t HeaderCRC[256] =\n");
    printf("{\n");
    printf("    ");
    for (i = 0; i < 256; i++) {
        crc = CRC_Calc_Header(i, 0);
        printf("0x%02x, ", crc);
        if (!((i + 1) % 8)) {
            printf("\n");
            if (i != 255) {
                printf("    ");
            }
        }
    }
    printf("};\n");
}

/**
 * @brief "Test" to create/log generated CRC16 table
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(crc_tests, testCRC16CreateTable)
#else
static void testCRC16CreateTable(void)
#endif
{
    uint16_t crc;
    int i;

    printf("static const uint16_t DataCRC[256] =\n");
    printf("{\n");
    printf("    ");
    for (i = 0; i < 256; i++) {
        crc = CRC_Calc_Data(i, 0);
        printf("0x%04x, ", crc);
        if (!((i + 1) % 8)) {
            printf("\n");
            if (i != 255) {
                printf("    ");
            }
        }
    }
    printf("};\n");
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(crc_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        crc_tests, ztest_unit_test(testCRC8), ztest_unit_test(testCRC16),
        ztest_unit_test(testCRC8CreateTable),
        ztest_unit_test(testCRC16CreateTable));

    ztest_run_test_suite(crc_tests);
}
#endif
