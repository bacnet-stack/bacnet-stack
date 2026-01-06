/**
 * @file
 * @brief test BACnet NPDU encoding and decoding API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/abort.h>
#include <bacnet/bacerror.h>
#include <bacnet/bacdcode.h>
#include <bacnet/npdu.h>
#include <bacnet/reject.h>
#include <bacnet/rp.h>
#include <bacnet/whois.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_tests, test_NPDU_Network)
#else
static void test_NPDU_Network(void)
#endif
{
    uint8_t pdu[MAX_NPDU] = { 0 };
    BACNET_ADDRESS dest = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_ADDRESS npdu_dest = { 0 };
    BACNET_ADDRESS npdu_src = { 0 };
    int len = 0, null_len = 0, test_len = 0, legacy_len;
    bool data_expecting_reply = true;
    BACNET_NETWORK_MESSAGE_TYPE network_message_type =
        NETWORK_MESSAGE_NETWORK_NUMBER_IS;
    BACNET_MESSAGE_PRIORITY priority = MESSAGE_PRIORITY_NORMAL;
    BACNET_NPDU_DATA npdu_data = { 0 };
    bool network_layer_message = true;
    uint16_t vendor_id = 0; /* optional, if net message type is > 0x80 */

    npdu_encode_npdu_network(
        &npdu_data, network_message_type, data_expecting_reply, priority);
    null_len = bacnet_npdu_encode_pdu(NULL, 0, &dest, &src, &npdu_data);
    len = bacnet_npdu_encode_pdu(&pdu[0], sizeof(pdu), &dest, &src, &npdu_data);
    zassert_equal(len, null_len, NULL);
    zassert_not_equal(len, 0, NULL);
    /* can we get the info back? */
    legacy_len = npdu_decode(pdu, &npdu_dest, &npdu_src, &npdu_data);
    test_len =
        bacnet_npdu_decode(pdu, sizeof(pdu), &npdu_dest, &npdu_src, &npdu_data);
    zassert_equal(test_len, legacy_len, NULL);
    zassert_not_equal(test_len, 0, NULL);
    zassert_equal(len, test_len, NULL);
    zassert_equal(npdu_data.data_expecting_reply, data_expecting_reply, NULL);
    zassert_equal(npdu_data.network_layer_message, network_layer_message, NULL);
    zassert_equal(npdu_data.network_message_type, network_message_type, NULL);
    zassert_equal(npdu_data.vendor_id, vendor_id, NULL);
    zassert_equal(npdu_data.priority, priority, NULL);
    /* test for invalid short PDU */
    while (len) {
        len--;
        test_len =
            bacnet_npdu_decode(pdu, len, &npdu_dest, &npdu_src, &npdu_data);
        if (len == 2) {
            /* special case with no NPDU options */
            zassert_equal(len, test_len, NULL);
        } else {
            zassert_true(test_len <= 0, "len=%d test_len=%d\n", len, test_len);
        }
    }
}

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_tests, testNPDU2)
#else
static void testNPDU2(void)
#endif
{
    uint8_t pdu[MAX_NPDU] = { 0 };
    BACNET_ADDRESS dest = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_ADDRESS npdu_dest = { 0 };
    BACNET_ADDRESS npdu_src = { 0 };
    int len = 0;
    bool data_expecting_reply = true;
    BACNET_MESSAGE_PRIORITY priority = MESSAGE_PRIORITY_NORMAL;
    BACNET_NPDU_DATA npdu_data = { 0 };
    int i = 0; /* counter */
    int npdu_len = 0;
    bool network_layer_message = false; /* false if APDU */
    BACNET_NETWORK_MESSAGE_TYPE network_message_type = 0; /* optional */
    uint16_t vendor_id = 0; /* optional, if net message type is > 0x80 */

    dest.mac_len = 6;
    for (i = 0; i < dest.mac_len; i++) {
        dest.mac[i] = i;
    }
    /* DNET,DLEN,DADR */
    dest.net = 1;
    dest.len = 6;
    for (i = 0; i < dest.len; i++) {
        dest.adr[i] = i * 10;
    }
    src.mac_len = 1;
    for (i = 0; i < src.mac_len; i++) {
        src.mac[i] = 0x80;
    }
    /* SNET,SLEN,SADR */
    src.net = 2;
    src.len = 1;
    for (i = 0; i < src.len; i++) {
        src.adr[i] = 0x40;
    }
    npdu_encode_npdu_data(&npdu_data, true, priority);
    len = bacnet_npdu_encode_pdu(&pdu[0], sizeof(pdu), &dest, &src, &npdu_data);
    zassert_not_equal(len, 0, NULL);
    /* can we get the info back? */
    npdu_len =
        bacnet_npdu_decode(pdu, sizeof(pdu), &npdu_dest, &npdu_src, &npdu_data);
    zassert_not_equal(npdu_len, 0, NULL);
    zassert_equal(npdu_data.data_expecting_reply, data_expecting_reply, NULL);
    zassert_equal(npdu_data.network_layer_message, network_layer_message, NULL);
    if (npdu_data.network_layer_message) {
        zassert_equal(
            npdu_data.network_message_type, network_message_type, NULL);
    }
    zassert_equal(npdu_data.vendor_id, vendor_id, NULL);
    zassert_equal(npdu_data.priority, priority, NULL);
    /* DNET,DLEN,DADR */
    zassert_equal(npdu_dest.net, dest.net, NULL);
    zassert_equal(npdu_dest.len, dest.len, NULL);
    for (i = 0; i < dest.len; i++) {
        zassert_equal(npdu_dest.adr[i], dest.adr[i], NULL);
    }
    /* SNET,SLEN,SADR */
    zassert_equal(npdu_src.net, src.net, NULL);
    zassert_equal(npdu_src.len, src.len, NULL);
    for (i = 0; i < src.len; i++) {
        zassert_equal(npdu_src.adr[i], src.adr[i], NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_tests, testNPDU1)
#else
static void testNPDU1(void)
#endif
{
    uint8_t pdu[MAX_NPDU] = { 0 };
    BACNET_ADDRESS dest = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_ADDRESS npdu_dest = { 0 };
    BACNET_ADDRESS npdu_src = { 0 };
    int len = 0;
    bool data_expecting_reply = false;
    BACNET_MESSAGE_PRIORITY priority = MESSAGE_PRIORITY_NORMAL;
    BACNET_NPDU_DATA npdu_data = { 0 };
    int i = 0; /* counter */
    int npdu_len = 0;
    bool network_layer_message = false; /* false if APDU */
    BACNET_NETWORK_MESSAGE_TYPE network_message_type = 0; /* optional */
    uint16_t vendor_id = 0; /* optional, if net message type is > 0x80 */

    /* mac_len = 0 if global address */
    dest.mac_len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        dest.mac[i] = 0;
    }
    /* DNET,DLEN,DADR */
    dest.net = 0;
    dest.len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        dest.adr[i] = 0;
    }
    src.mac_len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        src.mac[i] = 0;
    }
    /* SNET,SLEN,SADR */
    src.net = 0;
    src.len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        src.adr[i] = 0;
    }
    npdu_encode_npdu_data(&npdu_data, false, priority);
    len = bacnet_npdu_encode_pdu(&pdu[0], sizeof(pdu), &dest, &src, &npdu_data);
    zassert_not_equal(len, 0, NULL);
    /* can we get the info back? */
    npdu_len =
        bacnet_npdu_decode(pdu, sizeof(pdu), &npdu_dest, &npdu_src, &npdu_data);
    zassert_not_equal(npdu_len, 0, NULL);
    zassert_equal(npdu_data.data_expecting_reply, data_expecting_reply, NULL);
    zassert_equal(npdu_data.network_layer_message, network_layer_message, NULL);
    if (npdu_data.network_layer_message) {
        zassert_equal(
            npdu_data.network_message_type, network_message_type, NULL);
    }
    zassert_equal(npdu_data.vendor_id, vendor_id, NULL);
    zassert_equal(npdu_data.priority, priority, NULL);
    zassert_equal(npdu_dest.mac_len, src.mac_len, NULL);
    zassert_equal(npdu_src.mac_len, dest.mac_len, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_tests, test_NPDU_Copy)
#else
static void test_NPDU_Copy(void)
#endif
{
    BACNET_NPDU_DATA npdu_data = { 0 };
    BACNET_NPDU_DATA npdu_data_copy = { 0 };
    bool data_expecting_reply = true;
    BACNET_MESSAGE_PRIORITY priority = MESSAGE_PRIORITY_LIFE_SAFETY;

    npdu_encode_npdu_data(&npdu_data, data_expecting_reply, priority);
    npdu_copy_data(&npdu_data_copy, &npdu_data);
    zassert_equal(
        npdu_data_copy.data_expecting_reply, npdu_data.data_expecting_reply,
        NULL);
    zassert_equal(
        npdu_data_copy.network_layer_message, npdu_data.network_layer_message,
        NULL);
    if (npdu_data_copy.network_layer_message) {
        zassert_equal(
            npdu_data_copy.network_message_type, npdu_data.network_message_type,
            NULL);
    }
    zassert_equal(npdu_data_copy.vendor_id, npdu_data.vendor_id, NULL);
    zassert_equal(npdu_data_copy.priority, npdu_data.priority, NULL);
}

/**
 * @brief Initialize the a data link broadcast address
 * @param dest - address to be filled with broadcast designator
 */
static void mstp_address_init(BACNET_ADDRESS *dest, uint8_t mac)
{
    int i = 0; /* counter */

    if (dest) {
        dest->mac_len = 1;
        dest->mac[0] = mac;
        dest->net = 0; /* local only, no routing */
        dest->len = 0; /* not routed */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = 0;
        }
    }

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_tests, test_NPDU_Confirmed_Service)
#else
static void test_NPDU_Confirmed_Service(void)
#endif
{
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    BACNET_ADDRESS test_address = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    uint8_t pdu[MAX_NPDU + MAX_APDU] = { 0 };
    int pdu_len = 0, npdu_len = 0, apdu_len = 0;
    uint8_t invoke_id = 1;
    bool status;

    mstp_address_init(&test_address, 1);
    rpdata.object_type = OBJECT_DEVICE;
    rpdata.object_instance = 12345;
    rpdata.object_property = PROP_OBJECT_NAME;
    rpdata.array_index = BACNET_ARRAY_ALL;
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.error_class = ERROR_CLASS_SERVICES;
    rpdata.error_code = ERROR_CODE_OTHER;
    npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
    npdu_len =
        npdu_encode_pdu(&pdu[0], &test_address, &test_address, &npdu_data);
    zassert_not_equal(npdu_len, 0, NULL);
    /* confirmed service */
    apdu_len = rp_encode_apdu(&pdu[npdu_len], invoke_id, &rpdata);
    zassert_true(apdu_len > 0, NULL);
    pdu_len = npdu_len + apdu_len;
    status = npdu_confirmed_service(pdu, pdu_len);
    zassert_true(status, NULL);
    /* unconfirmed service */
    apdu_len = whois_encode_apdu(&pdu[npdu_len], -1, -1);
    zassert_true(apdu_len > 0, NULL);
    pdu_len = npdu_len + apdu_len;
    status = npdu_confirmed_service(pdu, pdu_len);
    zassert_false(status, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_tests, test_NPDU_Segmented_Complex_Ack_Reply)
#else
static void test_NPDU_Segmented_Complex_Ack_Reply(void)
#endif
{
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    BACNET_ADDRESS test_address = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    uint8_t pdu[MAX_NPDU + MAX_APDU] = { 0 };
    int pdu_len = 0, npdu_len = 0, apdu_len = 0;
    uint8_t invoke_id = 1;
    bool status;

    mstp_address_init(&test_address, 1);
    rpdata.object_type = OBJECT_DEVICE;
    rpdata.object_instance = 12345;
    rpdata.object_property = PROP_OBJECT_NAME;
    rpdata.array_index = BACNET_ARRAY_ALL;
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.error_class = ERROR_CLASS_SERVICES;
    rpdata.error_code = ERROR_CODE_OTHER;
    npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
    npdu_len =
        npdu_encode_pdu(&pdu[0], &test_address, &test_address, &npdu_data);
    zassert_not_equal(npdu_len, 0, NULL);
    /* confirmed service */
    pdu_len = npdu_len;
    apdu_len = rp_ack_encode_apdu_init(&pdu[pdu_len], invoke_id, &rpdata);
    zassert_true(apdu_len > 0, NULL);
    pdu_len += apdu_len;
    apdu_len = rp_ack_encode_apdu_object_property_end(&pdu[pdu_len]);
    zassert_true(apdu_len > 0, NULL);
    pdu_len += apdu_len;
    status = npdu_is_segmented_complex_ack_reply(pdu, pdu_len);
    zassert_false(status, NULL);
    /* make it look segmented */
    pdu[npdu_len] |= BIT(3);
    status = npdu_is_segmented_complex_ack_reply(pdu, pdu_len);
    zassert_true(status, NULL);
}

static void test_npdu_is_expected_reply_too_short(
    const uint8_t *request_pdu,
    uint16_t request_pdu_len,
    BACNET_ADDRESS *request_address,
    uint16_t request_minimum_len,
    const uint8_t *reply_pdu,
    uint16_t reply_pdu_len,
    BACNET_ADDRESS *reply_address,
    uint16_t reply_minimum_len)
{
    int test_len;
    bool status;

    /* shrink the buffers to test for buffer out-of-bounds read */
    /* smallest valid request */
    test_len = request_minimum_len;
    while (test_len) {
        test_len--;
        status = npdu_is_expected_reply(
            request_pdu, test_len, request_address, reply_pdu, reply_pdu_len,
            reply_address);
        zassert_false(status, "test_len=%d\n", test_len);
    }
    /* smallest valid reply */
    test_len = reply_minimum_len;
    while (test_len) {
        test_len--;
        status = npdu_is_expected_reply(
            request_pdu, request_pdu_len, request_address, reply_pdu, test_len,
            reply_address);
        zassert_false(status, "test_len=%d\n", test_len);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_tests, test_NPDU_Data_Expecting_Reply)
#else
static void test_NPDU_Data_Expecting_Reply(void)
#endif
{
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    BACNET_ADDRESS test_address = { 0 }, reply_address = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    uint8_t request_pdu[MAX_NPDU + MAX_APDU] = { 0 };
    uint8_t reply_pdu[MAX_NPDU + MAX_APDU] = { 0 };
    int request_pdu_len = 0, reply_pdu_len = 0, npdu_len = 0, apdu_len = 0,
        request_npdu_len = 0;
    uint8_t invoke_id = 1;
    bool status;

    /* request */
    mstp_address_init(&test_address, 1);
    npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
    npdu_len = npdu_encode_pdu(
        &request_pdu[0], &test_address, &test_address, &npdu_data);
    zassert_not_equal(npdu_len, 0, NULL);
    rpdata.object_type = OBJECT_DEVICE;
    rpdata.object_instance = 12345;
    rpdata.object_property = PROP_OBJECT_NAME;
    rpdata.array_index = BACNET_ARRAY_ALL;
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.error_class = ERROR_CLASS_SERVICES;
    rpdata.error_code = ERROR_CODE_OTHER;
    apdu_len = rp_encode_apdu(&request_pdu[npdu_len], invoke_id, &rpdata);
    zassert_true(apdu_len > 0, NULL);
    request_pdu_len = npdu_len + apdu_len;
    request_npdu_len = npdu_len;
    /* reply */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    npdu_len = npdu_encode_pdu(
        &reply_pdu[0], &test_address, &test_address, &npdu_data);
    zassert_not_equal(npdu_len, 0, NULL);
    reply_pdu_len = npdu_len;
    apdu_len =
        rp_ack_encode_apdu_init(&reply_pdu[reply_pdu_len], invoke_id, &rpdata);
    zassert_true(apdu_len > 0, NULL);
    reply_pdu_len += apdu_len;
    apdu_len =
        rp_ack_encode_apdu_object_property_end(&reply_pdu[reply_pdu_len]);
    zassert_true(apdu_len > 0, NULL);
    reply_pdu_len += apdu_len;
    /* is this the reply? */
    status = npdu_is_expected_reply(
        request_pdu, request_pdu_len, &test_address, reply_pdu, reply_pdu_len,
        &test_address);
    zassert_true(status, NULL);
    test_npdu_is_expected_reply_too_short(
        request_pdu, request_pdu_len, &test_address, request_npdu_len + 4,
        reply_pdu, reply_pdu_len, &test_address, npdu_len + 3);
    /* using the MAC version of the function */
    status = npdu_is_data_expecting_reply(
        request_pdu, request_pdu_len, test_address.mac[0], reply_pdu,
        reply_pdu_len, test_address.mac[0]);
    zassert_true(status, NULL);
    /* different address */
    mstp_address_init(&reply_address, 4);
    npdu_len = npdu_encode_pdu(
        &reply_pdu[0], &test_address, &test_address, &npdu_data);
    status = npdu_is_expected_reply(
        request_pdu, request_pdu_len, &test_address, reply_pdu, reply_pdu_len,
        &reply_address);
    zassert_false(status, NULL);
    /* different protocol version*/
    request_pdu[0] = BACNET_PROTOCOL_VERSION + 1;
    status = npdu_is_expected_reply(
        request_pdu, request_pdu_len, &test_address, reply_pdu, reply_pdu_len,
        &test_address);
    zassert_false(status, NULL);
    request_pdu[0] = BACNET_PROTOCOL_VERSION;
    reply_pdu[0] = BACNET_PROTOCOL_VERSION + 1;
    status = npdu_is_expected_reply(
        request_pdu, request_pdu_len, &test_address, reply_pdu, reply_pdu_len,
        &test_address);
    zassert_false(status, NULL);
    reply_pdu[0] = BACNET_PROTOCOL_VERSION;
    /* different network priority */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_LIFE_SAFETY);
    npdu_len = npdu_encode_pdu(
        &reply_pdu[0], &test_address, &test_address, &npdu_data);
    zassert_not_equal(npdu_len, 0, NULL);
    status = npdu_is_expected_reply(
        request_pdu, request_pdu_len, &test_address, reply_pdu, reply_pdu_len,
        &test_address);
    zassert_false(status, NULL);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    npdu_len = npdu_encode_pdu(
        &reply_pdu[0], &test_address, &test_address, &npdu_data);
    zassert_not_equal(npdu_len, 0, NULL);
    /* different reply PDU type */
    reply_pdu[npdu_len + 2] = SERVICE_CONFIRMED_WRITE_PROPERTY;
    status = npdu_is_expected_reply(
        request_pdu, request_pdu_len, &test_address, reply_pdu, reply_pdu_len,
        &test_address);
    zassert_false(status, NULL);
    reply_pdu[npdu_len + 2] = SERVICE_CONFIRMED_READ_PROPERTY;
    /* change the invoke ID in the reply */
    reply_pdu[npdu_len + 1] = 2;
    status = npdu_is_expected_reply(
        request_pdu, request_pdu_len, &test_address, reply_pdu, reply_pdu_len,
        &test_address);
    zassert_false(status, NULL);
    reply_pdu[npdu_len + 1] = invoke_id;
    /* reply with ERROR PDU */
    apdu_len = bacerror_encode_apdu(
        &reply_pdu[npdu_len], invoke_id, SERVICE_CONFIRMED_READ_PROPERTY,
        ERROR_CLASS_OBJECT, ERROR_CODE_UNKNOWN_OBJECT);
    zassert_true(apdu_len > 0, NULL);
    reply_pdu_len = npdu_len + apdu_len;
    status = npdu_is_expected_reply(
        request_pdu, request_pdu_len, &test_address, reply_pdu, reply_pdu_len,
        &test_address);
    zassert_true(status, NULL);
    test_npdu_is_expected_reply_too_short(
        request_pdu, request_pdu_len, &test_address, request_npdu_len + 4,
        reply_pdu, reply_pdu_len, &test_address, npdu_len + 3);
    /* reply with REJECT PDU */
    apdu_len = reject_encode_apdu(
        &reply_pdu[npdu_len], invoke_id, REJECT_REASON_OTHER);
    zassert_true(apdu_len > 0, NULL);
    reply_pdu_len = npdu_len + apdu_len;
    status = npdu_is_expected_reply(
        request_pdu, request_pdu_len, &test_address, reply_pdu, reply_pdu_len,
        &test_address);
    zassert_true(status, NULL);
    test_npdu_is_expected_reply_too_short(
        request_pdu, request_pdu_len, &test_address, request_npdu_len + 4,
        reply_pdu, reply_pdu_len, &test_address, npdu_len + 2);
    /* reply with ABORT PDU */
    apdu_len = abort_encode_apdu(
        &reply_pdu[npdu_len], invoke_id, ABORT_REASON_OTHER, true);
    zassert_true(apdu_len > 0, NULL);
    reply_pdu_len = npdu_len + apdu_len;
    status = npdu_is_expected_reply(
        request_pdu, request_pdu_len, &test_address, reply_pdu, reply_pdu_len,
        &test_address);
    zassert_true(status, NULL);
    test_npdu_is_expected_reply_too_short(
        request_pdu, request_pdu_len, &test_address, request_npdu_len + 4,
        reply_pdu, reply_pdu_len, &test_address, npdu_len + 2);
    /* reply with simple ack - note this is totally fake! */
    apdu_len = encode_simple_ack(
        &reply_pdu[npdu_len], invoke_id, SERVICE_CONFIRMED_READ_PROPERTY);
    zassert_true(apdu_len > 0, NULL);
    reply_pdu_len = npdu_len + apdu_len;
    status = npdu_is_expected_reply(
        request_pdu, request_pdu_len, &test_address, reply_pdu, reply_pdu_len,
        &test_address);
    zassert_true(status, NULL);
    test_npdu_is_expected_reply_too_short(
        request_pdu, request_pdu_len, &test_address, request_npdu_len + 4,
        reply_pdu, reply_pdu_len, &test_address, npdu_len + 3);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(npdu_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        npdu_tests, ztest_unit_test(testNPDU1), ztest_unit_test(testNPDU2),
        ztest_unit_test(test_NPDU_Network), ztest_unit_test(test_NPDU_Copy),
        ztest_unit_test(test_NPDU_Confirmed_Service),
        ztest_unit_test(test_NPDU_Segmented_Complex_Ack_Reply),
        ztest_unit_test(test_NPDU_Data_Expecting_Reply));

    ztest_run_test_suite(npdu_tests);
}
#endif
