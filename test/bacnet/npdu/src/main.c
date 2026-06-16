/**
 * @file
 * @brief test BACnet NPDU encoding and decoding API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#include <string.h>
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

/**
 * @brief Test npdu_i_am_router_to_network_process
 */
struct test_dnet_add_data {
    uint16_t snet;
    uint16_t net[16];
    unsigned count;
};

static void
test_dnet_add_callback(uint16_t snet, uint16_t net, const BACNET_ADDRESS *addr)
{
    struct test_dnet_add_data *data =
        (struct test_dnet_add_data *)(uintptr_t)(const void *)addr;
    data->snet = snet;
    if (data->count < 16) {
        data->net[data->count++] = net;
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_tests, test_NPDU_I_Am_Router_To_Network_Process)
#else
static void test_NPDU_I_Am_Router_To_Network_Process(void)
#endif
{
    struct test_dnet_add_data cb_data = { 0 };
    uint8_t npdu[16] = { 0 };
    uint16_t npdu_size = 0;
    uint16_t snet = 42;

    /* encode two network numbers: 100 and 200 */
    npdu[0] = 0x00;
    npdu[1] = 100;
    npdu[2] = 0x00;
    npdu[3] = 200;
    npdu_size = 4;

    npdu_i_am_router_to_network_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, npdu_size,
        test_dnet_add_callback);

    zassert_equal(cb_data.count, 2, NULL);
    zassert_equal(cb_data.snet, snet, NULL);
    zassert_equal(cb_data.net[0], 100, NULL);
    zassert_equal(cb_data.net[1], 200, NULL);

    /* empty buffer - nothing should be added */
    memset(&cb_data, 0, sizeof(cb_data));
    npdu_i_am_router_to_network_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, 1, test_dnet_add_callback);
    zassert_equal(cb_data.count, 0, NULL);

    /* NULL callback - no crash */
    npdu_i_am_router_to_network_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, npdu_size, NULL);

    /* single network number */
    memset(&cb_data, 0, sizeof(cb_data));
    npdu[0] = 0x00;
    npdu[1] = 0xFF;
    npdu_size = 2;
    npdu_i_am_router_to_network_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, npdu_size,
        test_dnet_add_callback);
    zassert_equal(cb_data.count, 1, NULL);
    zassert_equal(cb_data.net[0], 255, NULL);
}

/**
 * @brief Test npdu_init_routing_table_process
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(npdu_tests, test_NPDU_Init_Routing_Table_Process)
#else
static void test_NPDU_Init_Routing_Table_Process(void)
#endif
{
    struct test_dnet_add_data cb_data = { 0 };
    uint8_t npdu[64] = { 0 };
    uint16_t npdu_size = 0;
    uint16_t snet = 10;
    uint8_t *p = npdu;

    /* encode 2 entries:
     * net_count = 2
     * Entry 1: DNET=300, PortID=1, PortInfoLen=0
     * Entry 2: DNET=400, PortID=2, PortInfoLen=3, PortInfo={0xAA,0xBB,0xCC} */
    *p++ = 2; /* net_count */
    /* entry 1 */
    *p++ = 0x01; /* DNET high */
    *p++ = 0x2C; /* DNET low: 300 */
    *p++ = 1; /* PortID */
    *p++ = 0; /* PortInfoLen */
    /* entry 2 */
    *p++ = 0x01; /* DNET high */
    *p++ = 0x90; /* DNET low: 400 */
    *p++ = 2; /* PortID */
    *p++ = 3; /* PortInfoLen */
    *p++ = 0xAA;
    *p++ = 0xBB;
    *p++ = 0xCC;
    npdu_size = (uint16_t)(p - npdu);

    npdu_init_routing_table_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, npdu_size,
        test_dnet_add_callback);

    zassert_equal(cb_data.count, 2, NULL);
    zassert_equal(cb_data.snet, snet, NULL);
    zassert_equal(cb_data.net[0], 300, NULL);
    zassert_equal(cb_data.net[1], 400, NULL);

    /* malformed: too short (<=1 byte) */
    memset(&cb_data, 0, sizeof(cb_data));
    npdu_init_routing_table_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, 1, test_dnet_add_callback);
    zassert_equal(cb_data.count, 0, NULL);

    /* net_count == 0 */
    memset(&cb_data, 0, sizeof(cb_data));
    npdu[0] = 0;
    npdu_init_routing_table_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, npdu_size,
        test_dnet_add_callback);
    zassert_equal(cb_data.count, 0, NULL);

    /* NULL callback - no crash */
    npdu[0] = 2;
    npdu_init_routing_table_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, npdu_size, NULL);

    /* truncated buffer - only enough bytes for first entry's
     * DNET+PortID+PortInfoLen but not enough for second entry's 4-byte minimum;
     * first entry should be added */
    memset(&cb_data, 0, sizeof(cb_data));
    npdu[0] = 2;
    /* npdu_size=7: net_count(1)+DNET(2)+PortID(1)+PortInfoLen(1)+2bytes of
     * entry2 After processing entry 1: npdu_len=3, which is < 4 so loop exits
     */
    npdu_init_routing_table_process(
        snet, (BACNET_ADDRESS *)&cb_data, npdu, 7, test_dnet_add_callback);
    zassert_equal(cb_data.count, 1, NULL);
    zassert_equal(cb_data.net[0], 300, NULL);
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
        ztest_unit_test(test_NPDU_Data_Expecting_Reply),
        ztest_unit_test(test_NPDU_I_Am_Router_To_Network_Process),
        ztest_unit_test(test_NPDU_Init_Routing_Table_Process));

    ztest_run_test_suite(npdu_tests);
}
#endif
