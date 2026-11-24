/**
 * @file
 * @brief Unit tests for the DeviceCommunicationControl service handler
 * @author Ryan Mulder <ryan.mulder@antrum.com>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 *
 * Tests cover the order of the checks of the request: the password is
 * checked before the deprecated DISABLE is denied.
 */
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/ztest.h>
/* BACnet Stack defines - first */
#include <bacnet/bacdef.h>
/* BACnet Stack API */
#include <bacnet/bacenum.h>
#include <bacnet/bacerror.h>
#include <bacnet/bacstr.h>
#include <bacnet/apdu.h>
#include <bacnet/npdu.h>
#include <bacnet/dcc.h>
#include <bacnet/basic/services.h>
#include <bacnet/basic/tsm/tsm.h>

static const char Device_Password[] = "filister";
static const char Other_Password[] = "filiste";
static const char No_Password_Required[] = "";

/** The reply of the handler to a request */
struct dcc_reply {
    uint8_t pdu_type;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
};

/**
 * @brief Send a DeviceCommunicationControl request to the handler
 *  and decode its reply
 * @param state the 'Enable/Disable' parameter of the request
 * @param password the 'Password' parameter of the request,
 *  or NULL for a request without one
 * @return the PDU type of the reply, with the error class and code
 *  if the reply is an Error-PDU
 */
static struct dcc_reply
dcc_request(BACNET_COMMUNICATION_ENABLE_DISABLE state, const char *password)
{
    struct dcc_reply reply = { 0xFF, ERROR_CLASS_PROPRIETARY_LAST,
                               ERROR_CODE_PROPRIETARY_LAST };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    BACNET_CHARACTER_STRING password_string = { 0 };
    const BACNET_CHARACTER_STRING *request_password = NULL;
    uint8_t request[64] = { 0 };
    const uint8_t *apdu = NULL;
    int request_len = 0;
    int offset = 0;

    if (password) {
        characterstring_init_ansi(&password_string, password);
        request_password = &password_string;
    }
    request_len = dcc_apdu_encode(request, 1, state, request_password);

    service_data.invoke_id = 1;
    service_data.priority = MESSAGE_PRIORITY_NORMAL;
    service_data.max_resp = MAX_APDU;
    memset(Handler_Transmit_Buffer, 0, sizeof(Handler_Transmit_Buffer));
    handler_device_communication_control(
        request, (uint16_t)request_len, &src, &service_data);

    offset = bacnet_npdu_decode(
        Handler_Transmit_Buffer, MAX_PDU, NULL, NULL, &npdu_data);
    if (offset <= 0) {
        return reply;
    }
    apdu = &Handler_Transmit_Buffer[offset];
    reply.pdu_type = apdu[0] & 0xF0;
    if (reply.pdu_type == PDU_TYPE_ERROR) {
        /* error header: PDU type, invoke ID, service choice */
        bacerror_decode_error_class_and_code(
            &apdu[3], MAX_APDU - 3, &reply.error_class, &reply.error_code);
    }

    return reply;
}

/**
 * @brief Set the password the device requires and enable communication
 * @param password the password, or an empty string for a device
 *  that requires no password
 */
static void dcc_device_setup(const char *password)
{
    handler_dcc_password_set(password);
    dcc_set_status_duration(COMMUNICATION_ENABLE, 0);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_dcc_tests, testHandlerDCC_DisableWithoutValidPassword)
#else
static void testHandlerDCC_DisableWithoutValidPassword(void)
#endif
{
    struct dcc_reply reply = { 0 };

    /* a request without the required password is not a valid request,
       so it is denied for its password and not for the deprecated DISABLE */
    dcc_device_setup(Device_Password);
    reply = dcc_request(COMMUNICATION_DISABLE, Other_Password);
    zassert_equal(reply.pdu_type, PDU_TYPE_ERROR, NULL);
    zassert_equal(reply.error_class, ERROR_CLASS_SECURITY, NULL);
    zassert_equal(reply.error_code, ERROR_CODE_PASSWORD_FAILURE, NULL);
    zassert_true(dcc_communication_enabled(), NULL);

    reply = dcc_request(COMMUNICATION_DISABLE, NULL);
    zassert_equal(reply.pdu_type, PDU_TYPE_ERROR, NULL);
    zassert_equal(reply.error_class, ERROR_CLASS_SECURITY, NULL);
    zassert_equal(reply.error_code, ERROR_CODE_PASSWORD_FAILURE, NULL);
    zassert_true(dcc_communication_enabled(), NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_dcc_tests, testHandlerDCC_DisableWithValidPassword)
#else
static void testHandlerDCC_DisableWithValidPassword(void)
#endif
{
    struct dcc_reply reply = { 0 };

    dcc_device_setup(Device_Password);
    reply = dcc_request(COMMUNICATION_DISABLE, Device_Password);
    zassert_equal(reply.pdu_type, PDU_TYPE_ERROR, NULL);
    zassert_equal(reply.error_class, ERROR_CLASS_SERVICES, NULL);
    zassert_equal(reply.error_code, ERROR_CODE_SERVICE_REQUEST_DENIED, NULL);
    zassert_true(dcc_communication_enabled(), NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_dcc_tests, testHandlerDCC_DisableNoPasswordRequired)
#else
static void testHandlerDCC_DisableNoPasswordRequired(void)
#endif
{
    struct dcc_reply reply = { 0 };

    /* a device that requires no password ignores the parameter */
    dcc_device_setup(No_Password_Required);
    reply = dcc_request(COMMUNICATION_DISABLE, NULL);
    zassert_equal(reply.pdu_type, PDU_TYPE_ERROR, NULL);
    zassert_equal(reply.error_class, ERROR_CLASS_SERVICES, NULL);
    zassert_equal(reply.error_code, ERROR_CODE_SERVICE_REQUEST_DENIED, NULL);

    reply = dcc_request(COMMUNICATION_DISABLE, Other_Password);
    zassert_equal(reply.pdu_type, PDU_TYPE_ERROR, NULL);
    zassert_equal(reply.error_class, ERROR_CLASS_SERVICES, NULL);
    zassert_equal(reply.error_code, ERROR_CODE_SERVICE_REQUEST_DENIED, NULL);
    zassert_true(dcc_communication_enabled(), NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_dcc_tests, testHandlerDCC_DisableInitiation)
#else
static void testHandlerDCC_DisableInitiation(void)
#endif
{
    struct dcc_reply reply = { 0 };

    dcc_device_setup(Device_Password);
    reply = dcc_request(COMMUNICATION_DISABLE_INITIATION, Other_Password);
    zassert_equal(reply.pdu_type, PDU_TYPE_ERROR, NULL);
    zassert_equal(reply.error_class, ERROR_CLASS_SECURITY, NULL);
    zassert_equal(reply.error_code, ERROR_CODE_PASSWORD_FAILURE, NULL);
    zassert_true(dcc_communication_enabled(), NULL);

    reply = dcc_request(COMMUNICATION_DISABLE_INITIATION, Device_Password);
    zassert_equal(reply.pdu_type, PDU_TYPE_SIMPLE_ACK, NULL);
    zassert_true(dcc_communication_initiation_disabled(), NULL);

    reply = dcc_request(COMMUNICATION_ENABLE, Device_Password);
    zassert_equal(reply.pdu_type, PDU_TYPE_SIMPLE_ACK, NULL);
    zassert_true(dcc_communication_enabled(), NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(h_dcc_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        h_dcc_tests,
        ztest_unit_test(testHandlerDCC_DisableWithoutValidPassword),
        ztest_unit_test(testHandlerDCC_DisableWithValidPassword),
        ztest_unit_test(testHandlerDCC_DisableNoPasswordRequired),
        ztest_unit_test(testHandlerDCC_DisableInitiation));

    ztest_run_test_suite(h_dcc_tests);
}
#endif
