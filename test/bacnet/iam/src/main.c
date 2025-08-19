/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/iam.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static int iam_decode_apdu(
    const uint8_t *apdu,
    uint32_t *pDevice_id,
    unsigned *pMax_apdu,
    int *pSegmentation,
    uint16_t *pVendor_id)
{
    int apdu_len;

    /* valid data? */
    if (!apdu) {
        return -1;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST) {
        return -1;
    }
    if (apdu[1] != SERVICE_UNCONFIRMED_I_AM) {
        return -1;
    }
    apdu_len = 2;
    apdu_len += iam_decode_service_request(
        &apdu[2], pDevice_id, pMax_apdu, pSegmentation, pVendor_id);

    return apdu_len;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(iam_tests, testIAm)
#else
static void testIAm(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int test_len = 0, null_len = 0, apdu_len;
    uint32_t device_id = 42;
    unsigned max_apdu = 480;
    int segmentation = SEGMENTATION_NONE;
    uint16_t vendor_id = 42;
    uint32_t test_device_id = 0;
    unsigned test_max_apdu = 0;
    int test_segmentation = 0;
    uint16_t test_vendor_id = 0;

    null_len =
        iam_encode_apdu(NULL, device_id, max_apdu, segmentation, vendor_id);
    apdu_len =
        iam_encode_apdu(&apdu[0], device_id, max_apdu, segmentation, vendor_id);
    zassert_not_equal(apdu_len, 0, NULL);
    zassert_equal(apdu_len, null_len, NULL);

    test_len = iam_decode_apdu(
        &apdu[0], &test_device_id, &test_max_apdu, &test_segmentation,
        &test_vendor_id);
    zassert_equal(
        test_len, apdu_len, "test_len=%d apdu_len=%d", test_len, apdu_len);
    zassert_equal(test_device_id, device_id, NULL);
    zassert_equal(test_vendor_id, vendor_id, NULL);
    zassert_equal(test_max_apdu, max_apdu, NULL);
    zassert_equal(test_segmentation, segmentation, NULL);

    null_len = bacnet_iam_request_encode(
        NULL, device_id, max_apdu, segmentation, vendor_id);
    apdu_len = bacnet_iam_request_encode(
        &apdu[0], device_id, max_apdu, segmentation, vendor_id);
    zassert_not_equal(apdu_len, 0, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_iam_request_decode(
        &apdu[0], sizeof(apdu), &test_device_id, &test_max_apdu,
        &test_segmentation, &test_vendor_id);
    zassert_equal(test_len, apdu_len, NULL);
    zassert_equal(test_device_id, device_id, NULL);
    zassert_equal(test_vendor_id, vendor_id, NULL);
    zassert_equal(test_max_apdu, max_apdu, NULL);
    zassert_equal(test_segmentation, segmentation, NULL);
    test_len = bacnet_iam_request_decode(
        NULL, apdu_len, &test_device_id, &test_max_apdu, &test_segmentation,
        &test_vendor_id);
    zassert_equal(test_len, BACNET_STATUS_ERROR, NULL);
    test_len = bacnet_iam_request_decode(
        &apdu[0], apdu_len, NULL, &test_max_apdu, &test_segmentation,
        &test_vendor_id);
    zassert_equal(test_len, apdu_len, NULL);
    test_len = bacnet_iam_request_decode(
        &apdu[0], apdu_len, &test_device_id, NULL, &test_segmentation,
        &test_vendor_id);
    zassert_equal(test_len, apdu_len, NULL);
    test_len = bacnet_iam_request_decode(
        &apdu[0], apdu_len, &test_device_id, &test_max_apdu, NULL,
        &test_vendor_id);
    zassert_equal(test_len, apdu_len, NULL);
    test_len = bacnet_iam_request_decode(
        &apdu[0], apdu_len, &test_device_id, &test_max_apdu, &test_segmentation,
        NULL);
    zassert_equal(test_len, apdu_len, NULL);
    while (apdu_len) {
        apdu_len--;
        test_len = bacnet_iam_request_decode(
            &apdu[0], apdu_len, &test_device_id, &test_max_apdu,
            &test_segmentation, &test_vendor_id);
        zassert_equal(
            test_len, BACNET_STATUS_ERROR, "apdu_len=%d test_len=%d", apdu_len,
            test_len);
    }
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(iam_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(iam_tests, ztest_unit_test(testIAm));

    ztest_run_test_suite(iam_tests);
}
#endif
