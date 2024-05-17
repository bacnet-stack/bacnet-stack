/**
 * @file
 * @brief Unit test for BACnetHostNPort encode and decode API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/bacdest.h>
#include <bacnet/hostnport.h>

static void test_HostNPortCodec(BACNET_HOST_N_PORT *data)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_HOST_N_PORT test_data = { 0 };
    BACNET_ERROR_CODE error_code = ERROR_CODE_SUCCESS;
    int len = 0, apdu_len = 0, null_len = 0, test_len = 0;
    uint8_t tag_number = 0;
    bool status = false;

    null_len = host_n_port_encode(NULL, data);
    apdu_len = host_n_port_encode(apdu, data);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_true(apdu_len != BACNET_STATUS_ERROR, NULL);
    null_len = host_n_port_decode(apdu, apdu_len, NULL, NULL);
    test_len = host_n_port_decode(apdu, apdu_len, &error_code, &test_data);
    zassert_equal(test_len, null_len, NULL);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    while (test_len) {
        test_len--;
        len = host_n_port_decode(apdu, test_len, NULL, NULL);
        zassert_true(len < 0, "len=%d test_len=%d", len, test_len);
    }

    null_len = host_n_port_context_encode(NULL, tag_number, data);
    apdu_len = host_n_port_context_encode(apdu, tag_number, data);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_true(apdu_len != BACNET_STATUS_ERROR, NULL);

    null_len = host_n_port_context_decode(
        apdu, apdu_len, tag_number, &error_code, NULL);
    test_len = host_n_port_context_decode(
        apdu, apdu_len, tag_number, &error_code, &test_data);
    zassert_equal(test_len, null_len, NULL);
    zassert_true(test_len > 0, "test_len=%d", len);
    status = host_n_port_same(&test_data, data);
    zassert_true(status, NULL);

    status = host_n_port_copy(&test_data, data);
    zassert_true(status, NULL);
    status = host_n_port_same(&test_data, data);
    zassert_true(status, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(create_object_tests, test_HostNPort)
#else
static void test_HostNPort(void)
#endif
{
    BACNET_HOST_N_PORT data = { 0 };
    int len = 0, apdu_len = 0, null_len = 0, test_len = 0;

    /* none */
    test_HostNPortCodec(&data);
    /* IP Address */
    octetstring_init_ascii_hex(&data.host.ip_address, "c0a80101");
    data.host_ip_address = true;
    data.host_name = false;
    data.port = 0xBAC0;
    test_HostNPortCodec(&data);
    /* Host Name */
    characterstring_init_ansi(&data.host.name, "bacnet.org");
    data.host_ip_address = false;
    data.host_name = true;
    data.port = 0xBAC0;
    test_HostNPortCodec(&data);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(host_n_port_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(host_n_port_tests, ztest_unit_test(test_HostNPort));

    ztest_run_test_suite(host_n_port_tests);
}
#endif
