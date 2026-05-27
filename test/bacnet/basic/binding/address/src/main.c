/**
 * @file
 * @brief test BACnet address cache APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#include <string.h>
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
ZTEST(address_tests, test_rr_address)
#else
static void test_rr_address(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    BACNET_READ_RANGE_DATA pRequest = { 0 };
    BACNET_ADDRESS src = { 0 };
    unsigned i;
    int len;

    address_init();
    /* Fill the cache to its limit */
    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        src.mac_len = 1;
        src.mac[0] = (uint8_t)i;
        address_add(i, 480, &src);
    }

    /* Test ReadRange ALL */
    pRequest.object_type = OBJECT_DEVICE;
    pRequest.object_instance = 123;
    pRequest.object_property = PROP_DEVICE_ADDRESS_BINDING;
    pRequest.array_index = BACNET_ARRAY_ALL;
    pRequest.RequestType = RR_READ_ALL;
    pRequest.Overhead = 0;

    len = rr_address_list_encode(apdu, &pRequest);
    zassert_not_equal(len, 0, "ReadRange ALL failed - returned 0");
    zassert_true(
        bitstring_bit(&pRequest.ResultFlags, RESULT_FLAG_MORE_ITEMS),
        "Expected MORE_ITEMS flag");
    zassert_true(
        bitstring_bit(&pRequest.ResultFlags, RESULT_FLAG_FIRST_ITEM),
        "Expected FIRST_ITEM flag");

    /* Test ReadRange from the end to verify the fix */
    pRequest.RequestType = RR_BY_POSITION;
    pRequest.Range.RefIndex = MAX_ADDRESS_CACHE - 5 + 1;
    pRequest.Count = 5;
    bitstring_init(&pRequest.ResultFlags);

    len = rr_address_list_encode(apdu, &pRequest);
    zassert_not_equal(len, 0, "ReadRange end failed - returned 0");
    zassert_equal(pRequest.ItemCount, 5, "Expected 5 items");
    zassert_true(
        bitstring_bit(&pRequest.ResultFlags, RESULT_FLAG_LAST_ITEM),
        "Expected LAST_ITEM flag");

    /* Specifically test hitting the very last physical entry in Address_Cache
     */
    pRequest.RequestType = RR_BY_POSITION;
    pRequest.Range.RefIndex = MAX_ADDRESS_CACHE;
    pRequest.Count = 1;
    bitstring_init(&pRequest.ResultFlags);

    len = rr_address_list_encode(apdu, &pRequest);
    zassert_not_equal(len, 0, "ReadRange last item failed - returned 0");
    zassert_equal(pRequest.ItemCount, 1, "Expected 1 item");
    zassert_true(
        bitstring_bit(&pRequest.ResultFlags, RESULT_FLAG_LAST_ITEM),
        "Expected LAST_ITEM flag for the last item");
}

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
 * @brief Test address_list_encode for correct return value and no buffer
 * overrun.
 *
 * The function has two phases:
 *   1. A length-calculation pass (apdu == NULL) to find the total bytes needed.
 *   2. An encoding pass that must write exactly those bytes into the caller's
 *      buffer without exceeding it.
 *
 * Bugs guarded against:
 *   - Writing past the end of a buffer whose size was obtained from the
 *     NULL-apdu length-calculation call (original overrun).
 *   - Double-counting encoded bytes in the return value when apdu != NULL
 *     (second-loop len accumulation bug).
 *   - Advancing the apdu pointer by the accumulated total rather than by the
 *     per-entry size, which shifts subsequent entries to wrong offsets and
 *     can push the write pointer past the buffer end.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(address_tests, test_address_list_encode)
#else
static void test_address_list_encode(void)
#endif
{
    /* Large backing array; guard region sits immediately after apdu_len bytes
     */
    uint8_t apdu[1024];
    BACNET_ADDRESS src = { 0 };
    BACNET_ADDRESS addr = { 0 };
    int len_null = 0;
    int len_encoded = 0;
    unsigned apdu_len = 0;
    unsigned i = 0;
    uint32_t device_id = 0;
    unsigned max_apdu_size = 0;

    /* Reset the cache.  In BACNET_ADDRESS_CACHE_FILE builds address_init()
     * re-reads a file left by an earlier test, so drain any loaded entries
     * explicitly so the "empty cache" assertions below are reliable. */
    address_init();
    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        if (address_get_by_index(i, &device_id, &max_apdu_size, &addr)) {
            address_remove_device(device_id);
        }
    }
    zassert_equal(address_count(), 0, "Cache must be empty at test start");

    /* ------------------------------------------------------------------ */
    /* empty cache                                                          */
    /* ------------------------------------------------------------------ */
    len_null = address_list_encode(NULL, 0);
    zassert_equal(len_null, 0, "Empty cache: length-only call must return 0");

    len_encoded = address_list_encode(apdu, sizeof(apdu));
    zassert_equal(len_encoded, 0, "Empty cache: encode call must return 0");

    /* ------------------------------------------------------------------ */
    /* single MAC-layer entry (no routing, net == 0)                       */
    /* ------------------------------------------------------------------ */
    src.mac_len = 1;
    src.mac[0] = 0x05;
    src.net = 0;
    src.len = 0;
    address_add(1001, 480, &src);
    zassert_equal(address_count(), 1, NULL);

    /* length-only call must be positive */
    len_null = address_list_encode(NULL, 0);
    zassert_true(len_null > 0, "Single entry: length-only call must be > 0");

    /* encode into a large buffer: return value must equal length-only result */
    memset(apdu, 0, sizeof(apdu));
    len_encoded = address_list_encode(apdu, sizeof(apdu));
    zassert_equal(
        len_encoded, len_null,
        "Single entry: encoded length must match length-only call");

    /* too-small buffer: must fail without writing */
    apdu_len = (unsigned)len_null - 1;
    memset(apdu, 0xAA, sizeof(apdu));
    len_encoded = address_list_encode(apdu, apdu_len);
    zassert_equal(
        len_encoded, BACNET_STATUS_ABORT,
        "Single entry: too-small buffer must return BACNET_STATUS_ABORT");
    zassert_equal(
        apdu[0], 0xAA, "Single entry: too-small buffer must not be written");

    /* exactly-right-sized buffer: guard bytes after it must survive */
    apdu_len = (unsigned)len_null;
    memset(apdu, 0, sizeof(apdu));
    memset(apdu + apdu_len, 0xBB, 8);
    len_encoded = address_list_encode(apdu, apdu_len);
    zassert_equal(
        len_encoded, len_null,
        "Single entry: exact-sized buffer must return correct length");
    for (i = 0; i < 8; i++) {
        zassert_equal(
            apdu[apdu_len + i], 0xBB,
            "Single entry: guard bytes must not be overwritten");
    }

    /* ------------------------------------------------------------------ */
    /* two entries: this is the primary regression case for the overrun.   */
    /* With the second-loop bug, `apdu += len` advances the write pointer  */
    /* by the ever-growing accumulated total instead of by the per-entry   */
    /* size.  When the buffer is exactly the right size the second entry   */
    /* is written past the end of the buffer.                              */
    /* ------------------------------------------------------------------ */
    src.mac_len = 6;
    src.mac[0] = 0xC0;
    src.mac[1] = 0xA8;
    src.mac[2] = 0x00;
    src.mac[3] = 0x18;
    src.mac[4] = 0xBA;
    src.mac[5] = 0xC0;
    src.net = 26001;
    src.len = 1;
    src.adr[0] = 0x19;
    address_add(2002, 480, &src);
    zassert_equal(address_count(), 2, NULL);

    /* length-only call with 2 entries */
    len_null = address_list_encode(NULL, 0);
    zassert_true(len_null > 0, "Two entries: length-only call must be > 0");

    /* encode into a large buffer: return value must equal length-only result */
    memset(apdu, 0, sizeof(apdu));
    len_encoded = address_list_encode(apdu, sizeof(apdu));
    zassert_equal(
        len_encoded, len_null,
        "Two entries: encoded length must match length-only call");

    /* exactly-right-sized buffer: guard bytes must survive */
    apdu_len = (unsigned)len_null;
    memset(apdu, 0, sizeof(apdu));
    memset(apdu + apdu_len, 0xBB, 8);
    len_encoded = address_list_encode(apdu, apdu_len);
    zassert_equal(
        len_encoded, len_null,
        "Two entries: exact-sized buffer must return correct length");
    for (i = 0; i < 8; i++) {
        zassert_equal(
            apdu[apdu_len + i], 0xBB,
            "Two entries: guard bytes must not be overwritten");
    }

    /* too-small buffer with 2 entries: length returned, no bytes written */
    apdu_len = (unsigned)len_null - 1;
    memset(apdu, 0xAA, sizeof(apdu));
    len_encoded = address_list_encode(apdu, apdu_len);
    zassert_equal(
        len_encoded, len_null,
        "Two entries: too-small buffer must still return required length");
    zassert_equal(
        apdu[0], 0xAA, "Two entries: too-small buffer must not be written");
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
        ztest_unit_test(testAddress), ztest_unit_test(test_rr_address),
        ztest_unit_test(test_address_list_encode));

    ztest_run_test_suite(address_tests);
#else
    ztest_test_suite(
        address_tests, ztest_unit_test(testAddress),
        ztest_unit_test(test_rr_address),
        ztest_unit_test(test_address_list_encode));

    ztest_run_test_suite(address_tests);
#endif
}
#endif
