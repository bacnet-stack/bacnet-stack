/**
 * @file
 * @brief Unit test for You-Are-Request
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdef.h>
#include <bacnet/bacstr.h>
#include <bacnet/youare.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Who-Am-I-Request service decoder
 * @param apdu [in] Buffer containing the APDU
 * @param apdu_size [in] The length of the APDU
 * @param device_id the Device Object_Identifier to be assigned
 *  in the qualified device.
 * @param vendor_id the identity of the vendor of the device that
 *  is qualified to receive this You-Are service request.
 * @param model_name the model name of the device qualified to receive
 *  the You-Are service request.
 * @param serial_number the serial identifier of the device qualified
 *  to receive the You-Are service request.
 * @param mac_address the device MAC address that is to be configured
 *  in the qualified device.
 * @return The number of bytes decoded, or #BACNET_STATUS_ERROR on error
 */
static int you_are_request_service_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    uint32_t *device_id,
    uint16_t *vendor_id,
    BACNET_CHARACTER_STRING *model_name,
    BACNET_CHARACTER_STRING *serial_number,
    BACNET_OCTET_STRING *mac_address)
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
    if (apdu[1] != SERVICE_UNCONFIRMED_YOU_ARE) {
        return BACNET_STATUS_ERROR;
    }
    apdu += 2;
    apdu_size -= 2;
    len = you_are_request_decode(
        apdu, apdu_size, device_id, vendor_id, model_name, serial_number,
        mac_address);

    return len;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(you_are_tests, test_you_are)
#else
static void test_you_are(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int len = 0, apdu_len = 0, null_len = 0;
    bool status;
    uint16_t vendor_id = 0, test_vendor_id = 0;
    uint32_t device_id = 0, test_device_id = 0;
    BACNET_CHARACTER_STRING model_name = { 0 };
    BACNET_CHARACTER_STRING test_model_name = { 0 };
    BACNET_CHARACTER_STRING serial_number = { 0 };
    BACNET_CHARACTER_STRING test_serial_number = { 0 };
    BACNET_OCTET_STRING mac_address = { 0 };
    BACNET_OCTET_STRING test_mac_address = { 0 };

    device_id = 42;
    vendor_id = 260;
    characterstring_init_ansi(&model_name, "BDK ATXX4 MSTP");
    characterstring_init_ansi(&serial_number, "1234567890");
    octetstring_init_ascii_hex(&mac_address, "0123456789ABCDEF");
    null_len = you_are_request_service_encode(
        NULL, device_id, vendor_id, &model_name, &serial_number, &mac_address);
    len = you_are_request_service_encode(
        apdu, device_id, vendor_id, &model_name, &serial_number, &mac_address);
    zassert_equal(null_len, len, NULL);
    apdu_len = len;
    len = you_are_request_service_decode(
        apdu, apdu_len, &test_device_id, &test_vendor_id, &test_model_name,
        &test_serial_number, &test_mac_address);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(test_device_id, device_id, NULL);
    zassert_equal(test_vendor_id, vendor_id, NULL);
    status = characterstring_same(&test_model_name, &model_name);
    zassert_true(status, NULL);
    status = characterstring_same(&test_serial_number, &serial_number);
    zassert_true(status, NULL);
    status = octetstring_value_same(&test_mac_address, &mac_address);
    zassert_true(status, NULL);
    len = you_are_request_service_decode(
        NULL, apdu_len, &test_device_id, &test_vendor_id, &test_model_name,
        &test_serial_number, &test_mac_address);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);

    null_len = you_are_request_encode(
        NULL, device_id, vendor_id, &model_name, &serial_number, &mac_address);
    len = you_are_request_encode(
        apdu, device_id, vendor_id, &model_name, &serial_number, &mac_address);
    zassert_equal(null_len, len, NULL);
    apdu_len = len;
    len = you_are_request_decode(
        apdu, apdu_len, &test_device_id, &test_vendor_id, &test_model_name,
        &test_serial_number, &test_mac_address);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(test_device_id, device_id, NULL);
    zassert_equal(test_vendor_id, vendor_id, NULL);
    status = characterstring_same(&test_model_name, &model_name);
    zassert_true(status, NULL);
    status = characterstring_same(&test_serial_number, &serial_number);
    zassert_true(status, NULL);
    status = octetstring_value_same(&test_mac_address, &mac_address);
    zassert_true(status, NULL);
    /* check for NULL apdu to decode */
    len = you_are_request_decode(
        NULL, apdu_len, &test_device_id, &test_vendor_id, &test_model_name,
        &test_serial_number, &test_mac_address);
    /* check for APDU too short - use no optional parameters */
    apdu_len = you_are_request_encode(
        apdu, UINT32_MAX, vendor_id, &model_name, &serial_number, NULL);
    while (apdu_len) {
        apdu_len--;
        len = you_are_request_decode(
            apdu, apdu_len, &test_device_id, &test_vendor_id, &test_model_name,
            &test_serial_number, &test_mac_address);
        zassert_equal(
            len, BACNET_STATUS_ERROR, "apdu_len=%d len=%d", apdu_len, len);
    }
    /* check for optional device-id parameter */
    apdu_len = you_are_request_encode(
        apdu, UINT32_MAX, vendor_id, &model_name, &serial_number, &mac_address);
    len = you_are_request_decode(
        apdu, apdu_len, &test_device_id, &test_vendor_id, &test_model_name,
        &test_serial_number, &test_mac_address);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(test_device_id, UINT32_MAX, NULL);
    zassert_equal(test_vendor_id, vendor_id, NULL);
    status = characterstring_same(&test_model_name, &model_name);
    zassert_true(status, NULL);
    status = characterstring_same(&test_serial_number, &serial_number);
    zassert_true(status, NULL);
    status = octetstring_value_same(&test_mac_address, &mac_address);
    zassert_true(status, NULL);
}
/**
 * @}
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(you_are_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(you_are_tests, ztest_unit_test(test_you_are));

    ztest_run_test_suite(you_are_tests);
}
#endif
