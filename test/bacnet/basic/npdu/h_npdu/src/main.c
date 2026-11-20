/**
 * @file
 * @brief test the basic NPDU handler API
 * @copyright SPDX-License-Identifier: MIT
 *
 * Exercises the Network-Number-Is handler registered via
 * npdu_set_network_number_is_handler(), by feeding npdu_handler() a
 * Network-Number-Is network layer message and checking that the callback
 * is invoked (or not) as required by the BACnet standard.
 *
 * Exercises npdu_send_what_is_network_number() with a datalink that
 * captures the message, and checks that the message is only sent to the
 * local network as required by the BACnet standard.
 */
#include <string.h>
#include <zephyr/ztest.h>
#include <bacnet/bacaddr.h>
#include <bacnet/bacdcode.h>
#include <bacnet/bacint.h>
#include <bacnet/npdu.h>
#include <bacnet/basic/npdu/h_npdu.h>
#include <bacnet/datalink/datalink.h>

/* the MAC address of a BACnet/IP broadcast, and of a BACnet/IP device */
static const uint8_t Broadcast_MAC[6] = { 192, 168, 1, 255, 0xBA, 0xC0 };
static const uint8_t Device_MAC[6] = { 192, 168, 1, 10, 0xBA, 0xC0 };

/* the address that the datalink reports as its own */
static BACNET_ADDRESS Datalink_My_Address;
/* captured state from the datalink */
static unsigned Sent_Count;
static BACNET_ADDRESS Sent_Dest;
static uint8_t Sent_PDU[MAX_NPDU + 3];
static unsigned Sent_PDU_Len;

/**
 * @brief Datalink that captures the PDU instead of sending it
 */
int datalink_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    (void)npdu_data;
    Sent_Count++;
    bacnet_address_copy(&Sent_Dest, dest);
    Sent_PDU_Len = pdu_len;
    if (pdu_len <= sizeof(Sent_PDU)) {
        memcpy(Sent_PDU, pdu, pdu_len);
    }

    return (int)pdu_len;
}

/**
 * @brief Datalink broadcast address, which is a global broadcast
 */
void datalink_get_broadcast_address(BACNET_ADDRESS *dest)
{
    memset(dest, 0, sizeof(BACNET_ADDRESS));
    dest->mac_len = sizeof(Broadcast_MAC);
    memcpy(dest->mac, Broadcast_MAC, sizeof(Broadcast_MAC));
    dest->net = BACNET_BROADCAST_NETWORK;
}

void datalink_get_my_address(BACNET_ADDRESS *my_address)
{
    bacnet_address_copy(my_address, &Datalink_My_Address);
}

/**
 * @brief Reset the datalink capture
 */
static void datalink_capture_init(void)
{
    memset(&Datalink_My_Address, 0, sizeof(Datalink_My_Address));
    Datalink_My_Address.mac_len = sizeof(Device_MAC);
    memcpy(Datalink_My_Address.mac, Device_MAC, sizeof(Device_MAC));
    Sent_Count = 0;
    memset(&Sent_Dest, 0xFF, sizeof(Sent_Dest));
    memset(Sent_PDU, 0, sizeof(Sent_PDU));
    Sent_PDU_Len = 0;
}

/**
 * @brief Check that the captured PDU is a What-Is-Network-Number message
 *  that devices shall not ignore: no DNET/DADR and no SNET/SADR.
 */
static void verify_sent_what_is_network_number_is_local(void)
{
    BACNET_ADDRESS dest = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    int len = 0;

    zassert_equal(Sent_Count, 1, NULL);
    /* version, control, and message type - nothing else */
    zassert_equal(Sent_PDU_Len, 3, NULL);
    len = bacnet_npdu_decode(
        Sent_PDU, (uint16_t)Sent_PDU_Len, &dest, &src, &npdu_data);
    zassert_equal(len, 3, NULL);
    zassert_true(npdu_data.network_layer_message, NULL);
    zassert_equal(
        npdu_data.network_message_type, NETWORK_MESSAGE_WHAT_IS_NETWORK_NUMBER,
        NULL);
    zassert_false(npdu_data.data_expecting_reply, NULL);
    zassert_equal(dest.net, 0, NULL);
    zassert_equal(dest.len, 0, NULL);
    zassert_equal(src.net, 0, NULL);
    zassert_equal(src.len, 0, NULL);
}

/**
 * @brief Check that the datalink was given its own broadcast address, so
 *  that a datalink that needs a broadcast MAC address can send the message.
 */
static void verify_sent_to_datalink_broadcast_address(void)
{
    BACNET_ADDRESS broadcast = { 0 };

    datalink_get_broadcast_address(&broadcast);
    zassert_true(bacnet_address_same(&Sent_Dest, &broadcast), NULL);
}

/**
 * @brief Check that the datalink was given the MAC address of the device,
 *  and was not asked to reach another network.
 */
static void verify_sent_to_device_mac_address(void)
{
    zassert_equal(Sent_Dest.mac_len, sizeof(Device_MAC), NULL);
    zassert_mem_equal(Sent_Dest.mac, Device_MAC, sizeof(Device_MAC), NULL);
    zassert_equal(Sent_Dest.net, 0, NULL);
    zassert_equal(Sent_Dest.len, 0, NULL);
}

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

/**
 * @brief Test that What-Is-Network-Number without a destination is a
 *  local broadcast, not the global broadcast of the datalink.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_handler_tests, test_What_Is_Network_Number_Local_Broadcast)
#else
static void test_What_Is_Network_Number_Local_Broadcast(void)
#endif
{
    int bytes_sent = 0;

    datalink_capture_init();
    bytes_sent = npdu_send_what_is_network_number(NULL);

    zassert_equal(bytes_sent, 3, NULL);
    verify_sent_what_is_network_number_is_local();
    verify_sent_to_datalink_broadcast_address();
}

/**
 * @brief Test that What-Is-Network-Number to a global broadcast address,
 *  such as the datalink broadcast address, is a local broadcast.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_handler_tests, test_What_Is_Network_Number_Global_Broadcast)
#else
static void test_What_Is_Network_Number_Global_Broadcast(void)
#endif
{
    BACNET_ADDRESS dst = { 0 };

    datalink_capture_init();
    datalink_get_broadcast_address(&dst);
    npdu_send_what_is_network_number(&dst);

    verify_sent_what_is_network_number_is_local();
    verify_sent_to_datalink_broadcast_address();
}

/**
 * @brief Test that What-Is-Network-Number to the MAC address of a device
 *  on the local network is a local unicast.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_handler_tests, test_What_Is_Network_Number_Local_Unicast)
#else
static void test_What_Is_Network_Number_Local_Unicast(void)
#endif
{
    BACNET_ADDRESS dst = { 0 };

    datalink_capture_init();
    dst.mac_len = sizeof(Device_MAC);
    memcpy(dst.mac, Device_MAC, sizeof(Device_MAC));
    npdu_send_what_is_network_number(&dst);

    verify_sent_what_is_network_number_is_local();
    verify_sent_to_device_mac_address();
}

/**
 * @brief Test that What-Is-Network-Number is never routed: a destination
 *  on a remote network is sent to the router MAC address without
 *  DNET/DADR, or as a local broadcast when there is no router MAC address.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_handler_tests, test_What_Is_Network_Number_Never_Routed)
#else
static void test_What_Is_Network_Number_Never_Routed(void)
#endif
{
    BACNET_ADDRESS dst = { 0 };

    datalink_capture_init();
    dst.mac_len = sizeof(Device_MAC);
    memcpy(dst.mac, Device_MAC, sizeof(Device_MAC));
    dst.net = 86;
    dst.len = 1;
    dst.adr[0] = 0x12;
    npdu_send_what_is_network_number(&dst);

    verify_sent_what_is_network_number_is_local();
    verify_sent_to_device_mac_address();

    datalink_capture_init();
    memset(&dst, 0, sizeof(dst));
    dst.net = 86;
    npdu_send_what_is_network_number(&dst);

    verify_sent_what_is_network_number_is_local();
    verify_sent_to_datalink_broadcast_address();
}

/**
 * @brief Test that What-Is-Network-Number has no SNET/SADR, even when
 *  the address of the sender has a network number.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_handler_tests, test_What_Is_Network_Number_No_Source_Network)
#else
static void test_What_Is_Network_Number_No_Source_Network(void)
#endif
{
    datalink_capture_init();
    Datalink_My_Address.net = 7;
    Datalink_My_Address.len = 1;
    Datalink_My_Address.adr[0] = 0x34;
    npdu_send_what_is_network_number(NULL);

    verify_sent_what_is_network_number_is_local();
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
        ztest_unit_test(test_Network_Number_Is_Handler_Replaced),
        ztest_unit_test(test_What_Is_Network_Number_Local_Broadcast),
        ztest_unit_test(test_What_Is_Network_Number_Global_Broadcast),
        ztest_unit_test(test_What_Is_Network_Number_Local_Unicast),
        ztest_unit_test(test_What_Is_Network_Number_Never_Routed),
        ztest_unit_test(test_What_Is_Network_Number_No_Source_Network));

    ztest_run_test_suite(npdu_handler_tests);
}
#endif
