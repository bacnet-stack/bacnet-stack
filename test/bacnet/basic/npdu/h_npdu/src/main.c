/**
 * @file
 * @brief test the npdu_set_network_number_is_handler API
 * @copyright SPDX-License-Identifier: MIT
 *
 * Exercises the Network-Number-Is handler registered via
 * npdu_set_network_number_is_handler(), by feeding npdu_handler() a
 * Network-Number-Is network layer message and checking that the callback
 * is invoked (or not) as required by the BACnet standard.
 */
#include <string.h>
#include <zephyr/ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/bacint.h>
#include <bacnet/npdu.h>
#include <bacnet/basic/npdu/h_npdu.h>

/* captured state from the test callback */
static bool Callback_Called;
static uint16_t Callback_Network_Number;

static void test_network_number_is_callback(uint16_t network)
{
    Callback_Called = true;
    Callback_Network_Number = network;
}

/**
 * @brief Build a Network-Number-Is NPDU for a given source network.
 *
 * @param pdu [out] buffer to hold the encoded message
 * @param src_net [in] SNET to encode, or 0 for no SNET/SADR present
 * @param network_number [in] network number carried by the message
 * @param status [in] NETWORK_NUMBER_LEARNED or NETWORK_NUMBER_CONFIGURED
 * @return number of bytes encoded
 */
static int encode_network_number_is(
    uint8_t *pdu, uint16_t src_net, uint16_t network_number, uint8_t status)
{
    BACNET_NPDU_DATA npdu_data = { 0 };
    BACNET_ADDRESS dest = { 0 };
    BACNET_ADDRESS src = { 0 };
    int len;

    if (src_net) {
        src.net = src_net;
        src.len = 1;
        src.adr[0] = 0xaa;
    }
    npdu_encode_npdu_network(
        &npdu_data, NETWORK_MESSAGE_NETWORK_NUMBER_IS, false,
        MESSAGE_PRIORITY_NORMAL);
    len = npdu_encode_pdu(pdu, &dest, &src, &npdu_data);
    len += encode_unsigned16(&pdu[len], network_number);
    pdu[len] = status;
    len++;

    return len;
}

/**
 * @brief Test that the registered callback fires with the network number
 *  carried by a locally broadcast Network-Number-Is message.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_handler_tests, test_Network_Number_Is_Handler_Called)
#else
static void test_Network_Number_Is_Handler_Called(void)
#endif
{
    uint8_t pdu[MAX_NPDU + 3] = { 0 };
    BACNET_ADDRESS src = { 0 };
    int len;

    Callback_Called = false;
    Callback_Network_Number = 0;
    npdu_network_number_set(0);
    npdu_set_network_number_is_handler(test_network_number_is_callback);

    len = encode_network_number_is(pdu, 0, 1234, NETWORK_NUMBER_LEARNED);
    npdu_handler(&src, pdu, (uint16_t)len);

    zassert_true(Callback_Called, NULL);
    zassert_equal(Callback_Network_Number, 1234, NULL);
    zassert_equal(npdu_network_number(), 1234, NULL);
}

/**
 * @brief Test that clearing the handler (NULL) stops the callback from
 *  being invoked, while the local network number is still learned.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_handler_tests, test_Network_Number_Is_Handler_Cleared)
#else
static void test_Network_Number_Is_Handler_Cleared(void)
#endif
{
    uint8_t pdu[MAX_NPDU + 3] = { 0 };
    BACNET_ADDRESS src = { 0 };
    int len;

    Callback_Called = false;
    Callback_Network_Number = 0;
    npdu_network_number_set(0);
    npdu_set_network_number_is_handler(NULL);

    len = encode_network_number_is(pdu, 0, 5678, NETWORK_NUMBER_LEARNED);
    npdu_handler(&src, pdu, (uint16_t)len);

    zassert_false(Callback_Called, NULL);
    zassert_equal(npdu_network_number(), 5678, NULL);
}

/**
 * @brief Test that a Network-Number-Is message carrying SNET/SADR
 *  information (i.e. routed, not locally broadcast) is ignored, per
 *  the BACnet standard, and does not invoke the handler.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_handler_tests, test_Network_Number_Is_Handler_Ignores_Routed)
#else
static void test_Network_Number_Is_Handler_Ignores_Routed(void)
#endif
{
    uint8_t pdu[MAX_NPDU + 3] = { 0 };
    BACNET_ADDRESS src = { 0 };
    int len;

    Callback_Called = false;
    Callback_Network_Number = 0;
    npdu_network_number_set(0);
    npdu_set_network_number_is_handler(test_network_number_is_callback);

    len = encode_network_number_is(pdu, 5, 4321, NETWORK_NUMBER_LEARNED);
    npdu_handler(&src, pdu, (uint16_t)len);

    zassert_false(Callback_Called, NULL);
    zassert_equal(npdu_network_number(), 0, NULL);
}

/**
 * @brief Test that re-registering the handler replaces the previous one.
 */
static bool Second_Callback_Called;

static void test_network_number_is_second_callback(uint16_t network)
{
    Second_Callback_Called = true;
    Callback_Network_Number = network;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_handler_tests, test_Network_Number_Is_Handler_Replaced)
#else
static void test_Network_Number_Is_Handler_Replaced(void)
#endif
{
    uint8_t pdu[MAX_NPDU + 3] = { 0 };
    BACNET_ADDRESS src = { 0 };
    int len;

    Callback_Called = false;
    Second_Callback_Called = false;
    Callback_Network_Number = 0;
    npdu_network_number_set(0);
    npdu_set_network_number_is_handler(test_network_number_is_callback);
    npdu_set_network_number_is_handler(test_network_number_is_second_callback);

    len = encode_network_number_is(pdu, 0, 42, NETWORK_NUMBER_LEARNED);
    npdu_handler(&src, pdu, (uint16_t)len);

    zassert_false(Callback_Called, NULL);
    zassert_true(Second_Callback_Called, NULL);
    zassert_equal(Callback_Network_Number, 42, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(npdu_handler_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        npdu_handler_tests,
        ztest_unit_test(test_Network_Number_Is_Handler_Called),
        ztest_unit_test(test_Network_Number_Is_Handler_Cleared),
        ztest_unit_test(test_Network_Number_Is_Handler_Ignores_Routed),
        ztest_unit_test(test_Network_Number_Is_Handler_Replaced));

    ztest_run_test_suite(npdu_handler_tests);
}
#endif
