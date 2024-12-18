/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <stdlib.h> /* For calloc() */
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/datalink/bvlc.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void test_BVLC_Address(
    const BACNET_IP_ADDRESS *bip_address_1,
    const BACNET_IP_ADDRESS *bip_address_2)
{
    zassert_false(bvlc_address_different(bip_address_1, bip_address_2), NULL);
}

static void test_BVLC_Broadcast_Distribution_Mask(
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *bd_mask_1,
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *bd_mask_2)
{
    zassert_false(
        bvlc_broadcast_distribution_mask_different(bd_mask_1, bd_mask_2), NULL);
}

static void test_BVLC_Broadcast_Distribution_Table_Entry(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry_1,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry_2)
{
    if (bdt_entry_1 && bdt_entry_2) {
        zassert_equal(bdt_entry_1->valid, bdt_entry_2->valid, NULL);
        test_BVLC_Address(
            &bdt_entry_1->dest_address, &bdt_entry_2->dest_address);
        test_BVLC_Broadcast_Distribution_Mask(
            &bdt_entry_1->broadcast_mask, &bdt_entry_2->broadcast_mask);
    }

    return;
}

static void test_BVLC_Foreign_Device_Table_Entry(
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry_1,
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry_2)
{
    if (fdt_entry_1 && fdt_entry_2) {
        zassert_equal(fdt_entry_1->valid, fdt_entry_2->valid, NULL);
        test_BVLC_Address(
            &fdt_entry_1->dest_address, &fdt_entry_2->dest_address);
        zassert_equal(fdt_entry_1->ttl_seconds, fdt_entry_2->ttl_seconds, NULL);
        zassert_equal(
            fdt_entry_1->ttl_seconds_remaining,
            fdt_entry_2->ttl_seconds_remaining, NULL);
    }

    return;
}

static int test_BVLC_Header(
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint8_t *message_type,
    uint16_t *message_length)

{
    int bytes_consumed = 0;
    int len = 0;

    if (pdu && message_type && message_length) {
        len = bvlc_decode_header(pdu, pdu_len, message_type, message_length);
        zassert_equal(len, 4, NULL);
        bytes_consumed = len;
    }

    return bytes_consumed;
}

static void test_BVLC_Result_Code(uint16_t result_code)
{
    uint8_t pdu[50] = { 0 };
    uint16_t test_result_code = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, test_len = 0;

    len = bvlc_encode_result(pdu, sizeof(pdu), result_code);
    zassert_equal(len, 6, NULL);
    test_len = test_BVLC_Header(pdu, len, &message_type, &length);
    zassert_equal(test_len, 4, NULL);
    zassert_equal(message_type, BVLC_RESULT, NULL);
    zassert_equal(length, 6, NULL);
    test_len += bvlc_decode_result(&pdu[4], length - 4, &test_result_code);
    zassert_equal(len, test_len, NULL);
    zassert_equal(result_code, test_result_code, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_Result)
#else
static void test_BVLC_Result(void)
#endif
{
    uint16_t result_code[] = {
        BVLC_RESULT_SUCCESSFUL_COMPLETION,
        BVLC_RESULT_WRITE_BROADCAST_DISTRIBUTION_TABLE_NAK,
        BVLC_RESULT_READ_BROADCAST_DISTRIBUTION_TABLE_NAK,
        BVLC_RESULT_REGISTER_FOREIGN_DEVICE_NAK,
        BVLC_RESULT_READ_FOREIGN_DEVICE_TABLE_NAK,
        BVLC_RESULT_DELETE_FOREIGN_DEVICE_TABLE_ENTRY_NAK,
        BVLC_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK
    };
    unsigned int i = 0;
    size_t result_code_max = sizeof(result_code) / sizeof(result_code[0]);

    for (i = 0; i < result_code_max; i++) {
        test_BVLC_Result_Code(result_code[i]);
    }
}

static void
test_BVLC_Original_Unicast_NPDU_Message(uint8_t *npdu, uint16_t npdu_len)
{
    uint8_t test_npdu[50] = { 0 };
    uint8_t pdu[60] = { 0 };
    uint16_t test_npdu_len = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, msg_len = 0, test_len = 0;
    uint16_t i = 0;

    len = bvlc_encode_original_unicast(pdu, sizeof(pdu), npdu, npdu_len);
    msg_len = 4 + npdu_len;
    zassert_equal(len, msg_len, NULL);
    test_len = test_BVLC_Header(pdu, len, &message_type, &length);
    zassert_equal(test_len, 4, NULL);
    zassert_equal(message_type, BVLC_ORIGINAL_UNICAST_NPDU, NULL);
    zassert_equal(length, msg_len, NULL);
    test_len += bvlc_decode_original_unicast(
        &pdu[4], length - 4, test_npdu, sizeof(test_npdu), &test_npdu_len);
    zassert_equal(len, test_len, NULL);
    zassert_equal(msg_len, test_len, NULL);
    zassert_equal(npdu_len, test_npdu_len, NULL);
    for (i = 0; i < npdu_len; i++) {
        zassert_equal(npdu[i], test_npdu[i], NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_Original_Unicast_NPDU)
#else
static void test_BVLC_Original_Unicast_NPDU(void)
#endif
{
    uint8_t npdu[50] = { 0 };
    uint16_t npdu_len = 0;
    uint16_t i = 0;

    test_BVLC_Original_Unicast_NPDU_Message(npdu, npdu_len);
    /* now with some NPDU data */
    for (i = 0; i < sizeof(npdu); i++) {
        npdu[i] = i;
    }
    npdu_len = sizeof(npdu);
    test_BVLC_Original_Unicast_NPDU_Message(npdu, npdu_len);
}

static void test_BVLC_Original_Broadcast_NPDU_Message(
    const uint8_t *npdu, uint16_t npdu_len)
{
    uint8_t test_npdu[50] = { 0 };
    uint8_t pdu[60] = { 0 };
    uint16_t test_npdu_len = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, msg_len = 0, test_len = 0;
    uint16_t i = 0;

    len = bvlc_encode_original_broadcast(pdu, sizeof(pdu), npdu, npdu_len);
    msg_len = 4 + npdu_len;
    zassert_equal(len, msg_len, NULL);
    test_len = test_BVLC_Header(pdu, len, &message_type, &length);
    zassert_equal(test_len, 4, NULL);
    zassert_equal(message_type, BVLC_ORIGINAL_BROADCAST_NPDU, NULL);
    zassert_equal(length, msg_len, NULL);
    test_len += bvlc_decode_original_broadcast(
        &pdu[4], length - 4, test_npdu, sizeof(test_npdu), &test_npdu_len);
    zassert_equal(len, test_len, NULL);
    zassert_equal(msg_len, test_len, NULL);
    zassert_equal(npdu_len, test_npdu_len, NULL);
    for (i = 0; i < npdu_len; i++) {
        zassert_equal(npdu[i], test_npdu[i], NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_Original_Broadcast_NPDU)
#else
static void test_BVLC_Original_Broadcast_NPDU(void)
#endif
{
    uint8_t npdu[50] = { 0 };
    uint16_t npdu_len = 0;
    uint16_t i = 0;

    test_BVLC_Original_Broadcast_NPDU_Message(npdu, npdu_len);
    /* now with some NPDU data */
    for (i = 0; i < sizeof(npdu); i++) {
        npdu[i] = i;
    }
    npdu_len = sizeof(npdu);
    test_BVLC_Original_Broadcast_NPDU_Message(npdu, npdu_len);
}

static void test_BVLC_Forwarded_NPDU_Message(
    const uint8_t *npdu,
    uint16_t npdu_len,
    const BACNET_IP_ADDRESS *bip_address)
{
    uint8_t test_npdu[50] = { 0 };
    uint8_t pdu[75] = { 0 };
    BACNET_IP_ADDRESS test_bip_address = { 0 };
    uint16_t test_npdu_len = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, msg_len = 0, test_len = 0;
    uint16_t i = 0;

    len = bvlc_encode_forwarded_npdu(
        pdu, sizeof(pdu), bip_address, npdu, npdu_len);
    msg_len = 1 + 1 + 2 + BIP_ADDRESS_MAX + npdu_len;
    zassert_equal(len, msg_len, NULL);
    test_len = test_BVLC_Header(pdu, len, &message_type, &length);
    zassert_equal(test_len, 4, NULL);
    zassert_equal(message_type, BVLC_FORWARDED_NPDU, NULL);
    zassert_equal(length, msg_len, NULL);
    test_len += bvlc_decode_forwarded_npdu(
        &pdu[4], length - 4, &test_bip_address, test_npdu, sizeof(test_npdu),
        &test_npdu_len);
    zassert_equal(len, test_len, NULL);
    zassert_equal(msg_len, test_len, NULL);
    test_BVLC_Address(bip_address, &test_bip_address);
    zassert_equal(npdu_len, test_npdu_len, NULL);
    for (i = 0; i < npdu_len; i++) {
        zassert_equal(npdu[i], test_npdu[i], NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_Forwarded_NPDU)
#else
static void test_BVLC_Forwarded_NPDU(void)
#endif
{
    uint8_t npdu[50] = { 0 };
    BACNET_IP_ADDRESS bip_address = { 0 };
    uint16_t npdu_len = 0;
    uint16_t i = 0;

    test_BVLC_Forwarded_NPDU_Message(npdu, npdu_len, &bip_address);
    for (i = 0; i < sizeof(bip_address.address); i++) {
        bip_address.address[i] = i;
    }
    bip_address.port = 47808;
    /* now with some NPDU data */
    for (i = 0; i < sizeof(npdu); i++) {
        npdu[i] = i;
    }
    npdu_len = sizeof(npdu);
    test_BVLC_Forwarded_NPDU_Message(npdu, npdu_len, &bip_address);
}

static void test_BVLC_Register_Foreign_Device_Message(uint16_t ttl_seconds)
{
    uint8_t pdu[60] = { 0 };
    uint16_t test_ttl_seconds = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, test_len = 0;
    const int msg_len = 6;

    len = bvlc_encode_register_foreign_device(pdu, sizeof(pdu), ttl_seconds);
    zassert_equal(len, msg_len, NULL);
    test_len = test_BVLC_Header(pdu, len, &message_type, &length);
    zassert_equal(test_len, 4, NULL);
    zassert_equal(message_type, BVLC_REGISTER_FOREIGN_DEVICE, NULL);
    zassert_equal(length, msg_len, NULL);
    test_len += bvlc_decode_register_foreign_device(
        &pdu[4], length - 4, &test_ttl_seconds);
    zassert_equal(len, test_len, NULL);
    zassert_equal(msg_len, test_len, NULL);
    zassert_equal(ttl_seconds, test_ttl_seconds, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_Register_Foreign_Device)
#else
static void test_BVLC_Register_Foreign_Device(void)
#endif
{
    uint16_t ttl_seconds = 0;

    test_BVLC_Register_Foreign_Device_Message(ttl_seconds);
    ttl_seconds = 600;
    test_BVLC_Register_Foreign_Device_Message(ttl_seconds);
}

static void test_BVLC_Delete_Foreign_Device_Message(
    const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry)
{
    uint8_t pdu[64] = { 0 };
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY test_fdt_entry = { 0 };
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, test_len = 0;
    const int msg_len = 0x000A;

    len = bvlc_encode_delete_foreign_device(
        pdu, sizeof(pdu), &fdt_entry->dest_address);
    zassert_equal(len, msg_len, NULL);
    test_len = test_BVLC_Header(pdu, len, &message_type, &length);
    zassert_equal(test_len, 4, NULL);
    zassert_equal(message_type, BVLC_DELETE_FOREIGN_DEVICE_TABLE_ENTRY, NULL);
    zassert_equal(length, msg_len, NULL);
    test_len += bvlc_decode_delete_foreign_device(
        &pdu[4], length - 4, &test_fdt_entry.dest_address);
    zassert_equal(len, test_len, NULL);
    zassert_equal(msg_len, test_len, NULL);
    if (msg_len != test_len) {
        printf("msg:%u test:%u\n", msg_len, test_len);
    }
    test_BVLC_Address(&fdt_entry->dest_address, &test_fdt_entry.dest_address);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_Delete_Foreign_Device)
#else
static void test_BVLC_Delete_Foreign_Device(void)
#endif
{
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY fdt_entry = { 0 };
    unsigned int i = 0;

    /* test with zeros */
    test_BVLC_Delete_Foreign_Device_Message(&fdt_entry);
    /* test with valid values */
    for (i = 0; i < sizeof(fdt_entry.dest_address.address); i++) {
        fdt_entry.dest_address.address[i] = i;
    }
    fdt_entry.dest_address.port = 47808;
    fdt_entry.ttl_seconds = 600;
    fdt_entry.ttl_seconds_remaining = 42;
    fdt_entry.next = NULL;
    test_BVLC_Delete_Foreign_Device_Message(&fdt_entry);
}

static void
test_BVLC_Secure_BVLL_Message(const uint8_t *sbuf, uint16_t sbuf_len)
{
    uint8_t test_sbuf[50] = { 0 };
    uint8_t pdu[60] = { 0 };
    uint16_t test_sbuf_len = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, msg_len = 0, test_len = 0;
    uint16_t i = 0;

    len = bvlc_encode_secure_bvll(pdu, sizeof(pdu), sbuf, sbuf_len);
    msg_len = 1 + 1 + 2 + sbuf_len;
    zassert_equal(len, msg_len, NULL);
    test_len = test_BVLC_Header(pdu, len, &message_type, &length);
    zassert_equal(test_len, 4, NULL);
    zassert_equal(message_type, BVLC_SECURE_BVLL, NULL);
    zassert_equal(length, msg_len, NULL);
    test_len += bvlc_decode_secure_bvll(
        &pdu[4], length - 4, test_sbuf, sizeof(test_sbuf), &test_sbuf_len);
    zassert_equal(len, test_len, NULL);
    zassert_equal(msg_len, test_len, NULL);
    zassert_equal(sbuf_len, test_sbuf_len, NULL);
    for (i = 0; i < sbuf_len; i++) {
        zassert_equal(sbuf[i], test_sbuf[i], NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_Secure_BVLL)
#else
static void test_BVLC_Secure_BVLL(void)
#endif
{
    uint8_t sbuf[50] = { 0 };
    uint16_t sbuf_len = 0;
    uint16_t i = 0;

    test_BVLC_Secure_BVLL_Message(sbuf, sbuf_len);
    /* now with some NPDU data */
    for (i = 0; i < sizeof(sbuf); i++) {
        sbuf[i] = i;
    }
    sbuf_len = sizeof(sbuf);
    test_BVLC_Secure_BVLL_Message(sbuf, sbuf_len);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_Read_Broadcast_Distribution_Table_Message)
#else
static void test_BVLC_Read_Broadcast_Distribution_Table_Message(void)
#endif
{
    uint8_t pdu[60] = { 0 };
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, msg_len = 0, test_len = 0;

    len = bvlc_encode_read_broadcast_distribution_table(pdu, sizeof(pdu));
    msg_len = 1 + 1 + 2;
    zassert_equal(len, msg_len, NULL);
    test_len = test_BVLC_Header(pdu, len, &message_type, &length);
    zassert_equal(test_len, 4, NULL);
    zassert_equal(message_type, BVLC_READ_BROADCAST_DIST_TABLE, NULL);
    zassert_equal(length, msg_len, NULL);
}

static void test_BVLC_Distribute_Broadcast_To_Network_Message(
    const uint8_t *npdu, uint16_t npdu_len)
{
    uint8_t test_npdu[50] = { 0 };
    uint8_t pdu[60] = { 0 };
    uint16_t test_npdu_len = 0;
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, msg_len = 0, test_len = 0;
    uint16_t i = 0;

    len = bvlc_encode_distribute_broadcast_to_network(
        pdu, sizeof(pdu), npdu, npdu_len);
    msg_len = 4 + npdu_len;
    zassert_equal(len, msg_len, NULL);
    test_len = test_BVLC_Header(pdu, len, &message_type, &length);
    zassert_equal(test_len, 4, NULL);
    zassert_equal(message_type, BVLC_DISTRIBUTE_BROADCAST_TO_NETWORK, NULL);
    zassert_equal(length, msg_len, NULL);
    test_len += bvlc_decode_distribute_broadcast_to_network(
        &pdu[4], length - 4, test_npdu, sizeof(test_npdu), &test_npdu_len);
    zassert_equal(len, test_len, NULL);
    zassert_equal(msg_len, test_len, NULL);
    zassert_equal(npdu_len, test_npdu_len, NULL);
    for (i = 0; i < npdu_len; i++) {
        zassert_equal(npdu[i], test_npdu[i], NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_Distribute_Broadcast_To_Network)
#else
static void test_BVLC_Distribute_Broadcast_To_Network(void)
#endif
{
    uint8_t npdu[50] = { 0 };
    uint16_t npdu_len = 0;
    uint16_t i = 0;

    test_BVLC_Distribute_Broadcast_To_Network_Message(npdu, npdu_len);
    /* now with some NPDU data */
    for (i = 0; i < sizeof(npdu); i++) {
        npdu[i] = i;
    }
    npdu_len = sizeof(npdu);
    test_BVLC_Distribute_Broadcast_To_Network_Message(npdu, npdu_len);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_Broadcast_Distribution_Table_Encode)
#else
static void test_BVLC_Broadcast_Distribution_Table_Encode(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    uint16_t apdu_len = 0;
    uint16_t test_apdu_len = 0;
    uint16_t i = 0;
    uint16_t count = 0;
    uint16_t test_count = 0;
    bool status = false;
    BACNET_ERROR_CODE error_code;
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY bdt_list[5] = { 0 };
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY bdt_entry = { 0 };
    BACNET_IP_ADDRESS dest_address = { 0 };
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK broadcast_mask = { 0 };
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY test_bdt_list[5] = { 0 };

    /* configure a BDT entry */
    count = sizeof(bdt_list) / sizeof(bdt_list[0]);
    bvlc_broadcast_distribution_table_link_array(&bdt_list[0], count);
    for (i = 0; i < count; i++) {
        status = bvlc_address_port_from_ascii(
            &dest_address, "192.168.0.255", "0xBAC0");
        zassert_true(status, NULL);
        dest_address.port += i;
        broadcast_mask.address[0] = 255;
        broadcast_mask.address[1] = 255;
        broadcast_mask.address[2] = 255;
        broadcast_mask.address[3] = 255;
        status = bvlc_broadcast_distribution_table_entry_set(
            &bdt_entry, &dest_address, &broadcast_mask);
        zassert_true(status, NULL);
        status = bvlc_broadcast_distribution_table_entry_append(
            &bdt_list[0], &bdt_entry);
        zassert_true(status, NULL);
    }
    test_count = bvlc_broadcast_distribution_table_count(&bdt_list[0]);
    if (test_count != count) {
        printf("size=%u count=%u\n", count, test_count);
    }
    zassert_equal(test_count, count, NULL);
    /* test the encode/decode pair */
    apdu_len = bvlc_broadcast_distribution_table_encode(
        &apdu[0], sizeof(apdu), &bdt_list[0]);
    test_count = sizeof(test_bdt_list) / sizeof(test_bdt_list[0]);
    bvlc_broadcast_distribution_table_link_array(&test_bdt_list[0], test_count);
    test_apdu_len = bvlc_broadcast_distribution_table_decode(
        &apdu[0], apdu_len, &error_code, &test_bdt_list[0]);
    zassert_equal(test_apdu_len, apdu_len, NULL);
    count = bvlc_broadcast_distribution_table_count(&test_bdt_list[0]);
    zassert_equal(test_count, count, NULL);
    for (i = 0; i < count; i++) {
        status = bvlc_broadcast_distribution_table_entry_different(
            &bdt_list[i], &test_bdt_list[i]);
        zassert_false(status, NULL);
    }
}

static void test_BVLC_Write_Broadcast_Distribution_Table_Message(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list)
{
    uint8_t pdu[480] = { 0 };
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, msg_len = 0, test_len = 0;
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *test_bdt_list = NULL;
    uint16_t i = 0;
    uint16_t count = 0;

    count = bvlc_broadcast_distribution_table_valid_count(bdt_list);
    test_bdt_list =
        calloc(count, sizeof(BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY));
    bvlc_broadcast_distribution_table_link_array(test_bdt_list, count);
    /* encode the message */
    len = bvlc_encode_write_broadcast_distribution_table(
        pdu, sizeof(pdu), bdt_list);
    msg_len = 4 + (count * BACNET_IP_BDT_ENTRY_SIZE);
    zassert_equal(len, msg_len, NULL);
    test_len = test_BVLC_Header(pdu, len, &message_type, &length);
    zassert_equal(test_len, 4, NULL);
    zassert_equal(message_type, BVLC_WRITE_BROADCAST_DISTRIBUTION_TABLE, NULL);
    zassert_equal(length, msg_len, NULL);
    test_len += bvlc_decode_write_broadcast_distribution_table(
        &pdu[4], length - 4, test_bdt_list);
    zassert_equal(msg_len, test_len, NULL);
    for (i = 0; i < count; i++) {
        test_BVLC_Broadcast_Distribution_Table_Entry(
            &bdt_list[i], &test_bdt_list[i]);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_Write_Broadcast_Distribution_Table)
#else
static void test_BVLC_Write_Broadcast_Distribution_Table(void)
#endif
{
    uint16_t i = 0;
    uint16_t count = 0;
    uint16_t test_count = 0;
    bool status = false;
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY bdt_list[5] = { 0 };
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY bdt_entry = { 0 };
    BACNET_IP_ADDRESS dest_address = { 0 };
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK broadcast_mask = { 0 };

    /* configure a BDT entry */
    count = sizeof(bdt_list) / sizeof(bdt_list[0]);
    bvlc_broadcast_distribution_table_link_array(&bdt_list[0], count);
    for (i = 0; i < count; i++) {
        status = bvlc_address_port_from_ascii(
            &dest_address, "192.168.0.255", "0xBAC0");
        zassert_true(status, NULL);
        dest_address.port += i;
        broadcast_mask.address[0] = 255;
        broadcast_mask.address[1] = 255;
        broadcast_mask.address[2] = 255;
        broadcast_mask.address[3] = 255;
        status = bvlc_broadcast_distribution_table_entry_set(
            &bdt_entry, &dest_address, &broadcast_mask);
        zassert_true(status, NULL);
        status = bvlc_broadcast_distribution_table_entry_append(
            &bdt_list[0], &bdt_entry);
        zassert_true(status, NULL);
    }
    test_count = bvlc_broadcast_distribution_table_count(&bdt_list[0]);
    if (test_count != count) {
        printf("size=%u count=%u\n", count, test_count);
    }
    zassert_equal(test_count, count, NULL);
    test_count = bvlc_broadcast_distribution_table_valid_count(&bdt_list[0]);
    zassert_equal(test_count, count, NULL);
    for (i = 1; i < 5; i++) {
        status = bvlc_broadcast_distribution_table_entry_different(
            &bdt_list[0], &bdt_list[i]);
        zassert_true(status, NULL);
    }
    test_BVLC_Write_Broadcast_Distribution_Table_Message(&bdt_list[0]);
}

static void test_BVLC_Read_Foreign_Device_Table_Ack_Message(
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list)
{
    uint8_t pdu[480] = { 0 };
    uint8_t message_type = 0;
    uint16_t length = 0;
    int len = 0, msg_len = 0, test_len = 0;
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *test_fdt_list = NULL;
    uint16_t i = 0;
    uint16_t count = 0;

    count = bvlc_foreign_device_table_valid_count(fdt_list);
    test_fdt_list = calloc(count, sizeof(BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY));
    bvlc_foreign_device_table_link_array(test_fdt_list, count);
    /* encode the message */
    len = bvlc_encode_read_foreign_device_table_ack(pdu, sizeof(pdu), fdt_list);
    msg_len = 4 + (count * BACNET_IP_FDT_ENTRY_SIZE);
    zassert_equal(len, msg_len, NULL);
    test_len = test_BVLC_Header(pdu, len, &message_type, &length);
    zassert_equal(test_len, 4, NULL);
    zassert_equal(message_type, BVLC_READ_FOREIGN_DEVICE_TABLE_ACK, NULL);
    zassert_equal(length, msg_len, NULL);
    test_len += bvlc_decode_read_foreign_device_table_ack(
        &pdu[4], length - 4, test_fdt_list);
    zassert_equal(msg_len, test_len, NULL);
    for (i = 0; i < count; i++) {
        test_BVLC_Foreign_Device_Table_Entry(&fdt_list[i], &test_fdt_list[i]);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_Read_Foreign_Device_Table_Ack)
#else
static void test_BVLC_Read_Foreign_Device_Table_Ack(void)
#endif
{
    uint16_t i = 0;
    uint16_t count = 0;
    uint16_t test_count = 0;
    uint16_t test_port_start = 0xBAC1;
    bool status = false;
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY fdt_list[5] = { 0 };
    BACNET_IP_ADDRESS dest_address = { 0 };

    status = bvlc_address_from_ascii(&dest_address, "192.168.0.1");
    zassert_true(status, NULL);
    /* configure a FDT entry */
    count = sizeof(fdt_list) / sizeof(fdt_list[0]);
    bvlc_foreign_device_table_link_array(fdt_list, count);
    for (i = 0; i < count; i++) {
        dest_address.port = test_port_start + i;
        status = bvlc_foreign_device_table_entry_add(
            &fdt_list[0], &dest_address, 12345);
        zassert_true(status, NULL);
        /* add again should only update TTL */
        status = bvlc_foreign_device_table_entry_add(
            &fdt_list[0], &dest_address, 12345);
        zassert_true(status, NULL);
    }
    test_count = bvlc_foreign_device_table_count(fdt_list);
    zassert_equal(test_count, count, NULL);
    if (test_count != count) {
        printf("size=%u count=%u\n", count, test_count);
    }
    test_count = bvlc_foreign_device_table_valid_count(fdt_list);
    zassert_equal(test_count, count, NULL);
    test_BVLC_Read_Foreign_Device_Table_Ack_Message(fdt_list);
    /* cleanup */
    for (i = 0; i < count; i++) {
        dest_address.port = test_port_start + i;
        status =
            bvlc_foreign_device_table_entry_delete(&fdt_list[0], &dest_address);
        zassert_true(status, NULL);
    }
    test_count = bvlc_foreign_device_table_valid_count(fdt_list);
    zassert_equal(test_count, 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_Address_Copy)
#else
static void test_BVLC_Address_Copy(void)
#endif
{
    unsigned int i = 0;
    BACNET_IP_ADDRESS src = { 0 };
    BACNET_IP_ADDRESS dst = { 0 };
    bool status = false;

    /* test with zeros */
    status = bvlc_address_copy(&dst, &src);
    zassert_true(status, NULL);
    status = bvlc_address_different(&dst, &src);
    zassert_false(status, NULL);
    /* test with valid values */
    for (i = 0; i < sizeof(src.address); i++) {
        src.address[i] = 1 + i;
    }
    src.port = 47808;
    status = bvlc_address_copy(&dst, &src);
    zassert_true(status, NULL);
    status = bvlc_address_different(&dst, &src);
    zassert_false(status, NULL);
    /* test for different port */
    dst.port = 47809;
    status = bvlc_address_different(&dst, &src);
    zassert_true(status, NULL);
    /* test for different address */
    dst.port = src.port;
    for (i = 0; i < sizeof(src.address); i++) {
        dst.address[i] = 0;
        status = bvlc_address_different(&dst, &src);
        zassert_true(status, NULL);
        dst.address[i] = 1 + i;
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_Address_Get_Set)
#else
static void test_BVLC_Address_Get_Set(void)
#endif
{
    uint16_t i = 0;
    BACNET_ADDRESS bsrc = { 0 };
    BACNET_IP_ADDRESS src = { 0 };
    BACNET_IP_ADDRESS dst = { 0 };
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK mask = { 0 };
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK test_mask = { 0 };
    const uint32_t broadcast_mask = 0x12345678;
    uint32_t test_broadcast_mask = 0;
    uint8_t octet0 = 192;
    uint8_t octet1 = 168;
    uint8_t octet2 = 1;
    uint8_t octet3 = 255;
    uint8_t test_octet0 = 0;
    uint8_t test_octet1 = 0;
    uint8_t test_octet2 = 0;
    uint8_t test_octet3 = 0;
    const uint16_t dnet = 12345;
    uint16_t snet = 0;
    bool status = false;

    for (i = 0; i < 255; i++) {
        octet0 = i;
        octet1 = i;
        octet2 = i;
        octet3 = i;
        status = bvlc_address_set(&src, octet0, octet1, octet2, octet3);
        zassert_true(status, NULL);
        status = bvlc_address_get(
            &src, &test_octet0, &test_octet1, &test_octet2, &test_octet3);
        zassert_true(status, NULL);
        zassert_equal(octet0, test_octet0, NULL);
        zassert_equal(octet1, test_octet1, NULL);
        zassert_equal(octet2, test_octet2, NULL);
        zassert_equal(octet3, test_octet3, NULL);
    }
    /* test the ASCII hex to address */
    /* test invalid */
    status = bvlc_address_from_ascii(&src, "256");
    zassert_false(status, NULL);
    status = bvlc_address_from_ascii(&src, "192.168.0.1");
    zassert_true(status, NULL);
    status = bvlc_address_set(&dst, 192, 168, 0, 1);
    zassert_true(status, NULL);
    status = bvlc_address_different(&dst, &src);
    zassert_false(status, NULL);
    /* test zero compression */
    status = bvlc_address_from_ascii(&src, "127...");
    zassert_true(status, NULL);
    status = bvlc_address_set(&dst, 127, 0, 0, 0);
    zassert_true(status, NULL);
    status = bvlc_address_different(&dst, &src);
    if (status) {
        status = bvlc_address_get(
            &src, &test_octet0, &test_octet1, &test_octet2, &test_octet3);
        printf(
            "src:%u.%u.%u.%u\n", (unsigned)test_octet0, (unsigned)test_octet1,
            (unsigned)test_octet2, (unsigned)test_octet3);
        status = bvlc_address_get(
            &dst, &test_octet0, &test_octet1, &test_octet2, &test_octet3);
        printf(
            "dst:%u.%u.%u.%u\n", (unsigned)test_octet0, (unsigned)test_octet1,
            (unsigned)test_octet2, (unsigned)test_octet3);
    }
    zassert_false(status, NULL);
    /* BACnet to IPv4 address conversions */
    status = bvlc_address_port_from_ascii(&src, "192.168.0.1", "0xBAC0");
    zassert_true(status, NULL);
    status = bvlc_ip_address_to_bacnet_local(&bsrc, &src);
    zassert_true(status, NULL);
    status = bvlc_ip_address_from_bacnet_local(&dst, &bsrc);
    zassert_true(status, NULL);
    status = bvlc_address_different(&dst, &src);
    zassert_false(status, NULL);
    status = bvlc_ip_address_to_bacnet_remote(&bsrc, dnet, &src);
    zassert_true(status, NULL);
    status = bvlc_ip_address_from_bacnet_remote(&dst, &snet, &bsrc);
    zassert_true(status, NULL);
    zassert_equal(snet, dnet, NULL);
    status = bvlc_ip_address_from_bacnet_remote(&dst, NULL, &bsrc);
    zassert_true(status, NULL);
    /* Broadcast Distribution Mask conversions */
    status = bvlc_broadcast_distribution_mask_from_host(&mask, broadcast_mask);
    zassert_true(status, NULL);
    status =
        bvlc_broadcast_distribution_mask_to_host(&test_broadcast_mask, &mask);
    zassert_true(status, NULL);
    zassert_equal(test_broadcast_mask, broadcast_mask, NULL);
    octet0 = 0x12;
    octet1 = 0x34;
    octet2 = 0x56;
    octet3 = 0x78;
    bvlc_broadcast_distribution_mask_set(
        &test_mask, octet0, octet1, octet2, octet3);
    status = bvlc_broadcast_distribution_mask_different(&mask, &test_mask);
    zassert_false(status, NULL);
    bvlc_broadcast_distribution_mask_get(
        &test_mask, &test_octet0, &test_octet1, &test_octet2, &test_octet3);
    zassert_equal(octet0, test_octet0, NULL);
    zassert_equal(octet1, test_octet1, NULL);
    zassert_equal(octet2, test_octet2, NULL);
    zassert_equal(octet3, test_octet3, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bvlc_tests, test_BVLC_BBMD_Address)
#else
static void test_BVLC_BBMD_Address(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int apdu_len = 0;
    int test_apdu_len = 0;
    BACNET_HOST_N_PORT bbmd_address;
    BACNET_IP_ADDRESS test_bbmd_address;
    BACNET_ERROR_CODE error_code = ERROR_CODE_SUCCESS;
#if 0
    bool status = false;

    status =
        bvlc_address_port_from_ascii(&bbmd_address, "192.168.0.255", "0xBAC0");
    zassert_true(status, NULL);
#endif
    bbmd_address.host_ip_address = true;
    bbmd_address.host_name = false;
    bbmd_address.host.ip_address.length = 4;
    bbmd_address.host.ip_address.value[0] = 192;
    bbmd_address.host.ip_address.value[1] = 168;
    bbmd_address.host.ip_address.value[2] = 0;
    bbmd_address.host.ip_address.value[3] = 255;
    bbmd_address.port = 0xBAC0;
    apdu_len = bvlc_foreign_device_bbmd_host_address_encode(
        apdu, sizeof(apdu), &bbmd_address);
    zassert_not_equal(apdu_len, 0, NULL);
    test_apdu_len = bvlc_foreign_device_bbmd_host_address_decode(
        apdu, apdu_len, &error_code, &test_bbmd_address);
    if (test_apdu_len < 0) {
        printf("BVLC: error-code=%s\n", bactext_error_code_name(error_code));
    }
    zassert_not_equal(test_apdu_len, 0, NULL);
    zassert_not_equal(test_apdu_len, BACNET_STATUS_ERROR, NULL);
    zassert_not_equal(test_apdu_len, BACNET_STATUS_ABORT, NULL);
    zassert_not_equal(test_apdu_len, BACNET_STATUS_REJECT, NULL);
#if 0
    status = bvlc_address_different(&bbmd_address, &test_bbmd_address);
    zassert_false(status, NULL);
#endif
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bvlc_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bvlc_tests, ztest_unit_test(test_BVLC_Result),
        ztest_unit_test(test_BVLC_Write_Broadcast_Distribution_Table),
        ztest_unit_test(test_BVLC_Read_Broadcast_Distribution_Table_Message),
        ztest_unit_test(test_BVLC_Forwarded_NPDU),
        ztest_unit_test(test_BVLC_Register_Foreign_Device),
        ztest_unit_test(test_BVLC_Read_Foreign_Device_Table_Ack),
        ztest_unit_test(test_BVLC_Delete_Foreign_Device),
        ztest_unit_test(test_BVLC_Distribute_Broadcast_To_Network),
        ztest_unit_test(test_BVLC_Broadcast_Distribution_Table_Encode),
        ztest_unit_test(test_BVLC_Original_Unicast_NPDU),
        ztest_unit_test(test_BVLC_Original_Broadcast_NPDU),
        ztest_unit_test(test_BVLC_Secure_BVLL),
        ztest_unit_test(test_BVLC_Address_Copy),
        ztest_unit_test(test_BVLC_Address_Get_Set),
        ztest_unit_test(test_BVLC_BBMD_Address));

    ztest_run_test_suite(bvlc_tests);
}
#endif
