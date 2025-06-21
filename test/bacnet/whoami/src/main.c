/**
 * @file
 * @brief Unit test for Who-Am-I-Request
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdef.h>
#include <bacnet/bacstr.h>
#include <bacnet/whoami.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Who-Am-I-Request service decoder
 * @param apdu [in] Buffer containing the APDU
 * @param apdu_size [in] The length of the APDU
 * @param vendor_id the identity of the vendor of the device initiating
 *  the Who-Am-I service request.
 * @param model_name the model name of the device initiating the Who-Am-I
 *  service request.
 * @param serial_number the serial identifier of the device initiating
 *  the Who-Am-I service request.
 * @return The number of bytes decoded, or #BACNET_STATUS_ERROR on error
 */
static int who_am_i_request_service_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    uint16_t *vendor_id,
    BACNET_CHARACTER_STRING *model_name,
    BACNET_CHARACTER_STRING *serial_number)
{
    int len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size < 2) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu[0] != PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu[1] != SERVICE_UNCONFIRMED_WHO_AM_I) {
        return BACNET_STATUS_ERROR;
    }
    apdu += 2;
    apdu_size -= 2;
    len = who_am_i_request_decode(
        apdu, apdu_size, vendor_id, model_name, serial_number);

    return len;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(who_am_i_tests, test_who_am_i)
#else
static void test_who_am_i(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int len = 0, apdu_len = 0, null_len = 0;
    bool status;
    uint16_t vendor_id = 0, test_vendor_id = 0;
    BACNET_CHARACTER_STRING model_name = { 0 };
    BACNET_CHARACTER_STRING test_model_name = { 0 };
    BACNET_CHARACTER_STRING serial_number = { 0 };
    BACNET_CHARACTER_STRING test_serial_number = { 0 };

    vendor_id = 260;
    characterstring_init_ansi(&model_name, "BDK ATXX4 MSTP");
    characterstring_init_ansi(&serial_number, "1234567890");

    null_len = who_am_i_request_service_encode(
        NULL, vendor_id, &model_name, &serial_number);
    len = who_am_i_request_service_encode(
        apdu, vendor_id, &model_name, &serial_number);
    zassert_equal(null_len, len, NULL);
    apdu_len = len;
    len = who_am_i_request_service_decode(
        apdu, apdu_len, &test_vendor_id, &test_model_name, &test_serial_number);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(test_vendor_id, vendor_id, NULL);
    status = characterstring_same(&test_model_name, &model_name);
    zassert_true(status, NULL);
    status = characterstring_same(&test_serial_number, &serial_number);
    zassert_true(status, NULL);
    len = who_am_i_request_service_decode(
        NULL, apdu_len, &test_vendor_id, &test_model_name, &test_serial_number);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);

    null_len =
        who_am_i_request_encode(NULL, vendor_id, &model_name, &serial_number);
    len = who_am_i_request_encode(apdu, vendor_id, &model_name, &serial_number);
    zassert_equal(null_len, len, NULL);
    apdu_len = len;
    len = who_am_i_request_decode(
        apdu, apdu_len, &test_vendor_id, &test_model_name, &test_serial_number);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(test_vendor_id, vendor_id, NULL);
    status = characterstring_same(&test_model_name, &model_name);
    zassert_true(status, NULL);
    status = characterstring_same(&test_serial_number, &serial_number);
    zassert_true(status, NULL);
    len = who_am_i_request_decode(
        NULL, apdu_len, &test_vendor_id, &test_model_name, &test_serial_number);
    while (apdu_len) {
        apdu_len--;
        len = who_am_i_request_decode(
            apdu, apdu_len, &test_vendor_id, &test_model_name,
            &test_serial_number);
        zassert_equal(
            len, BACNET_STATUS_ERROR, "apdu_len=%d len=%d", apdu_len, len);
    }
}
/**
 * @}
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(who_am_i_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(who_am_i_tests, ztest_unit_test(test_who_am_i));

    ztest_run_test_suite(who_am_i_tests);
}
#endif
