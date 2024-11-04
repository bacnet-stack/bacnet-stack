/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/bacaddr.h>
#include <bacnet/basic/binding/address.h>

/* we are likely compiling the demo command line tools if print enabled */
#if !defined(BACNET_ADDRESS_CACHE_FILE)
#if PRINT_ENABLED
#define BACNET_ADDRESS_CACHE_FILE
#endif
#endif

/**
 * @addtogroup bacnet_tests
 * @{
 */
#ifdef BACNET_ADDRESS_CACHE_FILE
static const char *Address_Cache_Filename = "address_cache";
#endif

/**
 * @brief Test
 */
static void set_address(unsigned index, BACNET_ADDRESS *dest)
{
    unsigned i;

    for (i = 0; i < MAX_MAC_LEN; i++) {
        dest->mac[i] = index;
    }
    dest->mac_len = MAX_MAC_LEN;
    dest->net = 7;
    dest->len = MAX_MAC_LEN;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        dest->adr[i] = index;
    }
}

#ifdef BACNET_ADDRESS_CACHE_FILE
static void set_file_address(
    const char *pFilename,
    uint32_t device_id,
    BACNET_ADDRESS *dest,
    uint16_t max_apdu)
{
    unsigned i;
    FILE *pFile = NULL;

    pFile = fopen(pFilename, "w");

    if (pFile) {
        fprintf(pFile, "%lu ", (long unsigned int)device_id);
        for (i = 0; i < dest->mac_len; i++) {
            fprintf(pFile, "%02x", dest->mac[i]);
            if ((i + 1) < dest->mac_len) {
                fprintf(pFile, ":");
            }
        }
        fprintf(pFile, " %hu ", dest->net);
        if (dest->net) {
            for (i = 0; i < dest->len; i++) {
                fprintf(pFile, "%02x", dest->adr[i]);
                if ((i + 1) < dest->len) {
                    fprintf(pFile, ":");
                }
            }
        } else {
            fprintf(pFile, "0");
        }
        fprintf(pFile, " %hu\n", max_apdu);
        fclose(pFile);
    }
}
#endif

#ifdef BACNET_ADDRESS_CACHE_FILE
/* Validate that the address data in the file */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(address_tests, testAddressFile)
#else
static void testAddressFile(void)
#endif
{
    BACNET_ADDRESS src = { 0 };
    uint32_t device_id = 0;
    unsigned max_apdu = 480;
    BACNET_ADDRESS test_address = { 0 };
    unsigned test_max_apdu = 0;

    /* Create known data */
    /* create a fake address */
    device_id = 55555;
    src.mac_len = 1;
    src.mac[0] = 25;
    src.net = 0;
    src.adr[0] = 0;
    max_apdu = 50;
    set_file_address(Address_Cache_Filename, device_id, &src, max_apdu);
    /* retrieve it from the file, and see if we can find it */
    address_init();

    /* Verify */
    zassert_true(
        address_get_by_device(device_id, &test_max_apdu, &test_address), NULL);
    zassert_equal(test_max_apdu, max_apdu, NULL);
    zassert_true(bacnet_address_same(&test_address, &src), NULL);

    zassert_equal(address_count(), 1, NULL);
    address_remove_device(device_id);
    zassert_equal(address_count(), 0, NULL);

    /* create a fake address */
    device_id = 55555;
    src.mac_len = 6;
    src.mac[0] = 0xC0;
    src.mac[1] = 0xA8;
    src.mac[2] = 0x00;
    src.mac[3] = 0x18;
    src.mac[4] = 0xBA;
    src.mac[5] = 0xC0;
    src.net = 26001;
    src.len = 1;
    src.adr[0] = 25;
    max_apdu = 50;
    set_file_address(Address_Cache_Filename, device_id, &src, max_apdu);
    /* retrieve it from the file, and see if we can find it */
    address_init();
    zassert_true(
        address_get_by_device(device_id, &test_max_apdu, &test_address), NULL);
    zassert_equal(test_max_apdu, max_apdu, NULL);
    zassert_true(bacnet_address_same(&test_address, &src), NULL);

    zassert_equal(address_count(), 1, NULL);
    address_remove_device(device_id);
    zassert_equal(address_count(), 0, NULL);
}
#endif

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(address_tests, testAddress)
#else
static void testAddress(void)
#endif
{
    unsigned i, count;
    BACNET_ADDRESS src;
    uint32_t device_id = 0;
    unsigned max_apdu = 480;
    BACNET_ADDRESS test_address;
    uint32_t test_device_id = 0;
    unsigned test_max_apdu = 0;

    /* create a fake address database */
    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        set_address(i, &src);
        device_id = i * 255;
        address_add(device_id, max_apdu, &src);
        count = address_count();
        zassert_equal(count, (i + 1), NULL);
    }

    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        device_id = i * 255;
        set_address(i, &src);
        /* test the lookup by device id */
        zassert_true(
            address_get_by_device(device_id, &test_max_apdu, &test_address),
            NULL);
        zassert_equal(test_max_apdu, max_apdu, NULL);
        zassert_true(bacnet_address_same(&test_address, &src), NULL);
        zassert_true(
            address_get_by_index(
                i, &test_device_id, &test_max_apdu, &test_address),
            NULL);
        zassert_equal(test_device_id, device_id, NULL);
        zassert_equal(test_max_apdu, max_apdu, NULL);
        zassert_true(bacnet_address_same(&test_address, &src), NULL);
        zassert_equal(address_count(), MAX_ADDRESS_CACHE, NULL);
        /* test the lookup by MAC */
        zassert_true(address_get_device_id(&src, &test_device_id), NULL);
        zassert_equal(test_device_id, device_id, NULL);
    }

    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        device_id = i * 255;
        address_remove_device(device_id);
        zassert_false(
            address_get_by_device(device_id, &test_max_apdu, &test_address),
            NULL);
        count = address_count();
        zassert_equal(count, (MAX_ADDRESS_CACHE - i - 1), NULL);
    }
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(address_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
#ifdef BACNET_ADDRESS_CACHE_FILE
    ztest_test_suite(
        address_tests, ztest_unit_test(testAddressFile),
        ztest_unit_test(testAddress));

    ztest_run_test_suite(address_tests);
#else
    ztest_test_suite(address_tests, ztest_unit_test(testAddress));

    ztest_run_test_suite(address_tests);
#endif
}
#endif
