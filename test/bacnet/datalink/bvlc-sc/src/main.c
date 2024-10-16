/**
 * @file
 * @brief tests for BACnet/SC encode/decode APIs
 * @author Kirill Neznamov
 * @date May 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h> /* For calloc() */
#include <zephyr/ztest.h>
#include <bacnet/datalink/bsc/bvlc-sc.h>

static bool verify_bsc_bvll_header(BVLC_SC_DECODED_HDR *hdr,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    bool dest_options_absent,
    bool data_options_absent,
    uint16_t payload_len)
{
    if (hdr->bvlc_function != bvlc_function) {
        return false;
    }

    if (hdr->message_id != message_id) {
        return false;
    }

    if (origin) {
        if (memcmp(hdr->origin->address, origin->address, BVLC_SC_VMAC_SIZE) !=
            0) {
            return false;
        }
    } else {
        if (hdr->origin) {
            return false;
        }
    }

    if (dest) {
        if (memcmp(hdr->dest->address, dest->address, BVLC_SC_VMAC_SIZE) != 0) {
            return false;
        }
    } else {
        if (hdr->dest) {
            return false;
        }
    }

    if (dest_options_absent) {
        if (hdr->dest_options != NULL) {
            return false;
        }
        if (hdr->dest_options_len != 0) {
            return false;
        }
        if (hdr->dest_options_num != 0) {
            return false;
        }
    } else {
        if (!hdr->dest_options) {
            return false;
        }
        if (!hdr->dest_options_len) {
            return false;
        }
        if (!hdr->dest_options_num) {
            return false;
        }
    }

    if (data_options_absent) {
        if (hdr->data_options != NULL) {
            return false;
        }
        if (hdr->data_options_len != 0) {
            return false;
        }
        if (hdr->data_options_num != 0) {
            return false;
        }
    } else {
        if (!hdr->data_options) {
            return false;
        }
        if (!hdr->data_options_len) {
            return false;
        }
        if (!hdr->data_options_num) {
            return false;
        }
    }

    if (hdr->payload_len != payload_len) {
        return false;
    }

    return true;
}

static void test_header_modifications(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len,
    bool dest_options_absent,
    bool data_options_absent)
{
    uint8_t sbuf[BSC_PRE + 256];
    uint8_t *buf = &sbuf[BSC_PRE];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    const char *err_desc = NULL;
    int len;
    bool ret;
    BACNET_SC_VMAC_ADDRESS test;
    BACNET_SC_VMAC_ADDRESS test_dest;
    uint8_t *ppdu;
    int res;

    memset(&test.address[0], 0x99, sizeof(test.address));
    memcpy(buf, pdu, pdu_size);

    if (dest && !origin) {
        bvlc_sc_remove_dest_set_orig(buf, pdu_size, &test);
        ret = bvlc_sc_decode_message(
            buf, pdu_size, &message, &error, &class, &err_desc);
        zassert_equal(ret, true, NULL);
        ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
            &test, NULL, dest_options_absent, data_options_absent, payload_len);
        zassert_equal(ret, true, NULL);
        zassert_equal(message.hdr.payload_len, payload_len, NULL);
        res = memcmp(message.hdr.payload, payload, payload_len);
        zassert_equal(res, 0, NULL);
        zassert_equal(
            memcmp(&test.address, message.hdr.origin, sizeof(test.address)), 0,
            NULL);
        zassert_equal(message.hdr.dest, NULL, NULL);
    } else {
        bvlc_sc_remove_dest_set_orig(buf, pdu_size, &test);
        ret = bvlc_sc_decode_message(
            buf, pdu_size, &message, &error, &class, &err_desc);
        zassert_equal(ret, true, NULL);
        ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
            origin, dest, dest_options_absent, data_options_absent,
            payload_len);
        zassert_equal(ret, true, NULL);
        zassert_equal(message.hdr.payload_len, payload_len, NULL);
        res = memcmp(message.hdr.payload, payload, payload_len);
        zassert_equal(res, 0, NULL);

        if (origin) {
            zassert_equal(memcmp(&origin->address, message.hdr.origin,
                              sizeof(origin->address)),
                0, NULL);
        } else {
            zassert_equal(message.hdr.origin, NULL, NULL);
        }
        if (dest) {
            zassert_equal(
                memcmp(&dest->address, message.hdr.dest, sizeof(dest->address)),
                0, NULL);
            zassert_equal(bvlc_sc_pdu_has_no_dest(buf, pdu_size), false, NULL);
            ret = bvlc_sc_pdu_get_dest(buf, pdu_size, &test_dest);
            zassert_equal(ret, true, NULL);
            zassert_equal(memcmp(&test_dest.address, message.hdr.dest,
                              sizeof(dest->address)),
                0, NULL);
        } else {
            zassert_equal(message.hdr.dest, NULL, NULL);
            zassert_equal(bvlc_sc_pdu_has_no_dest(buf, pdu_size), true, NULL);
            ret = bvlc_sc_pdu_get_dest(buf, pdu_size, &test_dest);
            zassert_equal(ret, false, NULL);
        }
    }

    memcpy(buf, pdu, pdu_size);
    ppdu = buf;

    /* origin address can presented only in specific message types */

    if (bvlc_function != BVLC_SC_CONNECT_REQUEST &&
        bvlc_function != BVLC_SC_CONNECT_ACCEPT &&
        bvlc_function != BVLC_SC_DISCONNECT_REQUEST &&
        bvlc_function != BVLC_SC_DISCONNECT_ACK &&
        bvlc_function != BVLC_SC_HEARTBEAT_REQUEST &&
        bvlc_function != BVLC_SC_HEARTBEAT_ACK) {
        len = bvlc_sc_set_orig(&ppdu, pdu_size, &test);
        ret = bvlc_sc_decode_message(
            ppdu, len, &message, &error, &class, &err_desc);
        zassert_equal(ret, true, NULL);
        ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
            &test, dest, dest_options_absent, data_options_absent, payload_len);
        zassert_equal(ret, true, NULL);
        zassert_equal(message.hdr.payload_len, payload_len, NULL);
        res = memcmp(message.hdr.payload, payload, payload_len);
        zassert_equal(res, 0, NULL);
        zassert_equal(
            memcmp(&test.address, message.hdr.origin, sizeof(test.address)), 0,
            NULL);
    } else {
        len = pdu_size;
    }

    memcpy(buf, pdu, pdu_size);
    ppdu = buf;
    len = bvlc_sc_remove_orig_and_dest(&ppdu, pdu_size);
    ret =
        bvlc_sc_decode_message(ppdu, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.origin, NULL, NULL);
    zassert_equal(message.hdr.dest, NULL, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id, NULL,
        NULL, dest_options_absent, data_options_absent, payload_len);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload_len, payload_len, NULL);
    res = memcmp(message.hdr.payload, payload, payload_len);
    zassert_equal(res, 0, NULL);
}

static void test_1_option_data(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    const char *err_desc = NULL;
    int optlen;
    int len;
    bool ret;
    int res;

    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);

    optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true);
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, pdu_size, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
        origin, dest, true, false, payload_len);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.data_options_num, 1, NULL);
    zassert_equal(
        message.data_options[0].type, BVLC_SC_OPTION_TYPE_SECURE_PATH, NULL);
    zassert_equal(message.data_options[0].must_understand, true, NULL);
    zassert_equal(message.hdr.payload_len, payload_len, NULL);
    res = memcmp(message.hdr.payload, payload, payload_len);
    zassert_equal(res, 0, NULL);
    test_header_modifications(buf, len, bvlc_function, message_id, origin, dest,
        payload, payload_len, true, false);
}

/* 3 options are added to pdu in total: 1 sc, 2 proprietary */

static void test_3_options_different_buffer_data(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t buf1[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len;
    bool ret;
    uint16_t vendor_id1;
    uint16_t vendor_id2;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_option_type2;
    uint8_t proprietary_data1[17];
    uint8_t proprietary_data2[1];
    int res;
    const char *err_desc = NULL;

    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);

    optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true);
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf1, sizeof(buf1), buf, pdu_size, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf1, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    vendor_id2 = 0xbeaf;
    proprietary_option_type2 = 0x33;
    memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id2, proprietary_option_type2, proprietary_data2,
        sizeof(proprietary_data2));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf1, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret =
        bvlc_sc_decode_message(buf1, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
        origin, dest, true, false, payload_len);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.data_options_num, 3, NULL);

    zassert_equal(
        message.data_options[0].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.data_options[0].must_understand, true, NULL);
    zassert_equal(
        message.data_options[0].packed_header_marker & BVLC_SC_HEADER_MORE,
        BVLC_SC_HEADER_MORE, NULL);
    zassert_equal(
        message.data_options[0].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.data_options[0].specific.proprietary.vendor_id,
        vendor_id2, NULL);
    zassert_equal(message.data_options[0].specific.proprietary.option_type,
        proprietary_option_type2, NULL);
    zassert_equal(message.data_options[0].specific.proprietary.data_len,
        sizeof(proprietary_data2), NULL);
    res = memcmp(message.data_options[0].specific.proprietary.data,
        proprietary_data2, sizeof(proprietary_data2));
    zassert_equal(res, 0, NULL);
    zassert_equal(
        message.data_options[1].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.data_options[1].must_understand, true, NULL);
    zassert_equal(
        message.data_options[1].packed_header_marker & BVLC_SC_HEADER_MORE,
        BVLC_SC_HEADER_MORE, NULL);
    zassert_equal(
        message.data_options[1].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(
        message.data_options[2].type, BVLC_SC_OPTION_TYPE_SECURE_PATH, NULL);
    zassert_equal(message.data_options[2].must_understand, true, NULL);
    zassert_equal(
        message.data_options[2].packed_header_marker & BVLC_SC_HEADER_MORE, 0,
        NULL);
    zassert_equal(
        message.data_options[2].packed_header_marker & BVLC_SC_HEADER_DATA, 0,
        NULL);
    zassert_equal(message.hdr.payload_len, payload_len, NULL);
    res = memcmp(message.hdr.payload, payload, payload_len);
    zassert_equal(res, 0, NULL);
}

/* 3 options are added to pdu in total: 1 sc, 2 proprietary */

static void test_3_options_data(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len = pdu_size;
    bool ret;
    uint16_t vendor_id1;
    uint16_t vendor_id2;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_option_type2;
    uint8_t proprietary_data1[17];
    uint8_t proprietary_data2[1];
    int res;
    const char *err_desc = NULL;

    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);
    vendor_id2 = 0xbeaf;
    proprietary_option_type2 = 0x33;
    memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id2, proprietary_option_type2, proprietary_data2,
        sizeof(proprietary_data2));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);

    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true);
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);

    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
        origin, dest, true, false, payload_len);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.data_options_num, 3, NULL);
    zassert_equal(
        message.data_options[2].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.data_options[2].must_understand, true, NULL);
    zassert_equal(
        message.data_options[2].packed_header_marker & BVLC_SC_HEADER_MORE, 0,
        NULL);
    zassert_equal(
        message.data_options[2].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.data_options[2].specific.proprietary.vendor_id,
        vendor_id2, NULL);
    zassert_equal(message.data_options[2].specific.proprietary.option_type,
        proprietary_option_type2, NULL);
    zassert_equal(message.data_options[2].specific.proprietary.data_len,
        sizeof(proprietary_data2), NULL);
    res = memcmp(message.data_options[2].specific.proprietary.data,
        proprietary_data2, sizeof(proprietary_data2));
    zassert_equal(res, 0, NULL);
    zassert_equal(
        message.data_options[1].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.data_options[1].must_understand, true, NULL);
    zassert_equal(
        message.data_options[1].packed_header_marker & BVLC_SC_HEADER_MORE,
        BVLC_SC_HEADER_MORE, NULL);
    zassert_equal(
        message.data_options[1].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(
        message.data_options[0].type, BVLC_SC_OPTION_TYPE_SECURE_PATH, NULL);
    zassert_equal(message.data_options[0].must_understand, true, NULL);
    zassert_equal(
        message.data_options[0].packed_header_marker & BVLC_SC_HEADER_MORE,
        BVLC_SC_HEADER_MORE, NULL);
    zassert_equal(
        message.data_options[0].packed_header_marker & BVLC_SC_HEADER_DATA, 0,
        NULL);
    zassert_equal(message.hdr.payload_len, payload_len, NULL);
    res = memcmp(message.hdr.payload, payload, payload_len);
    zassert_equal(res, 0, NULL);
    test_header_modifications(buf, len, bvlc_function, message_id, origin, dest,
        payload, payload_len, true, false);
}

static void test_5_options_data(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len;
    bool ret;
    uint16_t vendor_id1;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_data1[17];
    int i;
    const char *err_desc = NULL;
    (void) bvlc_function;
    (void) message_id;
    (void) origin;
    (void) dest;
    (void) payload;
    (void) payload_len;

    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);

    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    len = pdu_size;
    for (i = 0; i < 5; i++) {
        len = bvlc_sc_add_option_to_data_options(
            buf, sizeof(buf), buf, len, optbuf, optlen);
        zassert_not_equal(len, 0, NULL);
    }
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_OUT_OF_MEMORY, NULL);
    zassert_equal(class, ERROR_CLASS_RESOURCES, NULL);
}

/* check message decoding when header option has incorrect more bit flag */

static void test_options_incorrect_more_bit_data(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len = pdu_size;
    bool ret;
    uint16_t vendor_id1;
    uint16_t vendor_id2;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_option_type2;
    uint8_t proprietary_data1[17];
    uint8_t proprietary_data2[5];
    int offs = 4;
    const char *err_desc = NULL;
    (void) bvlc_function;
    (void) message_id;
    (void) origin;
    (void) dest;
    (void) payload;
    (void) payload_len;

    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);

    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    vendor_id2 = 0xbeaf;
    proprietary_option_type2 = 0x33;
    memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id2, proprietary_option_type2, proprietary_data2,
        sizeof(proprietary_data2));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    if (buf[1] & BVLC_SC_CONTROL_ORIG_VADDR) {
        offs += BVLC_SC_VMAC_SIZE;
    }

    if (buf[1] & BVLC_SC_CONTROL_DEST_VADDR) {
        offs += BVLC_SC_VMAC_SIZE;
    }

    buf[offs] &= ~(BVLC_SC_HEADER_MORE);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_UNEXPECTED_DATA, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

/* check message decoding when header option has incorrect more bit flag */

static void test_options_incorrect_data_bit_data(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len = pdu_size;
    bool ret;
    uint16_t vendor_id1;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_data1[17];
    int offs = 4;
    const char *err_desc = NULL;
    (void) bvlc_function;
    (void) message_id;
    (void) origin;
    (void) payload;
    (void) payload_len;
    (void) dest;
    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);

    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    if (buf[1] & BVLC_SC_CONTROL_ORIG_VADDR) {
        offs += BVLC_SC_VMAC_SIZE;
    }

    if (buf[1] & BVLC_SC_CONTROL_DEST_VADDR) {
        offs += BVLC_SC_VMAC_SIZE;
    }

    buf[offs] &= ~(BVLC_SC_HEADER_DATA);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

/* secure path option must not be in dest option headers. this test
   checks this case */

static void test_1_option_dest_incorrect(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len;
    bool ret;
    const char *err_desc = NULL;
    (void) bvlc_function;
    (void) message_id;
    (void) origin;
    (void) payload;
    (void) payload_len;
    (void) dest;
    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);

    optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true);
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, pdu_size, optbuf, optlen);
    zassert_equal(len, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, pdu_size, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    buf[1] &= ~(BVLC_SC_CONTROL_DATA_OPTIONS);
    buf[1] |= BVLC_SC_CONTROL_DEST_OPTIONS;
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_1_option_dest(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len;
    bool ret;
    int res;
    uint16_t vendor_id1;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_data1[17];
    const char *err_desc = NULL;

    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);
    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));

    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, pdu_size, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
        origin, dest, false, true, payload_len);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.dest_options_num, 1, NULL);
    zassert_equal(
        message.dest_options[0].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.dest_options[0].must_understand, true, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.vendor_id,
        vendor_id1, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.option_type,
        proprietary_option_type1, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.data_len,
        sizeof(proprietary_data1), NULL);
    res = memcmp(message.dest_options[0].specific.proprietary.data,
        proprietary_data1, sizeof(proprietary_data1));
    zassert_equal(res, 0, NULL);
    zassert_equal(message.hdr.payload_len, payload_len, NULL);
    res = memcmp(message.hdr.payload, payload, payload_len);
    zassert_equal(res, 0, NULL);
    test_header_modifications(buf, len, bvlc_function, message_id, origin, dest,
        payload, payload_len, false, true);
}

/* 3 proprietary options are added to pdu */

static void test_3_options_dest(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len = pdu_size;
    bool ret;
    uint16_t vendor_id1;
    uint16_t vendor_id2;
    uint16_t vendor_id3;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_option_type2;
    uint8_t proprietary_option_type3;
    uint8_t proprietary_data1[17];
    uint8_t proprietary_data2[1];
    uint8_t proprietary_data3[43];
    int res;
    const char *err_desc = NULL;

    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);

    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    vendor_id2 = 0xbeaf;
    proprietary_option_type2 = 0x33;
    memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id2, proprietary_option_type2, proprietary_data2,
        sizeof(proprietary_data2));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);

    vendor_id3 = 0xF00D;
    proprietary_option_type3 = 0x08;
    memset(proprietary_data3, 0x55, sizeof(proprietary_data3));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id3, proprietary_option_type3, proprietary_data3,
        sizeof(proprietary_data3));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
        origin, dest, false, true, payload_len);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.dest_options_num, 3, NULL);
    zassert_equal(
        message.dest_options[0].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.dest_options[0].must_understand, true, NULL);
    zassert_equal(
        message.dest_options[0].packed_header_marker & BVLC_SC_HEADER_MORE,
        BVLC_SC_HEADER_MORE, NULL);
    zassert_equal(
        message.dest_options[0].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.vendor_id,
        vendor_id3, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.option_type,
        proprietary_option_type3, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.data_len,
        sizeof(proprietary_data3), NULL);
    res = memcmp(message.dest_options[0].specific.proprietary.data,
        proprietary_data3, sizeof(proprietary_data3));
    zassert_equal(res, 0, NULL);
    zassert_equal(
        message.dest_options[1].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.dest_options[1].must_understand, true, NULL);
    zassert_equal(
        message.dest_options[1].packed_header_marker & BVLC_SC_HEADER_MORE,
        BVLC_SC_HEADER_MORE, NULL);
    zassert_equal(
        message.dest_options[1].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.vendor_id,
        vendor_id2, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.option_type,
        proprietary_option_type2, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.data_len,
        sizeof(proprietary_data2), NULL);
    res = memcmp(message.dest_options[1].specific.proprietary.data,
        proprietary_data2, sizeof(proprietary_data2));
    zassert_equal(res, 0, NULL);
    zassert_equal(
        message.dest_options[2].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.dest_options[2].must_understand, true, NULL);
    zassert_equal(
        message.dest_options[2].packed_header_marker & BVLC_SC_HEADER_MORE, 0,
        NULL);
    zassert_equal(
        message.dest_options[2].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.dest_options[2].specific.proprietary.vendor_id,
        vendor_id1, NULL);
    zassert_equal(message.dest_options[2].specific.proprietary.option_type,
        proprietary_option_type1, NULL);
    zassert_equal(message.dest_options[2].specific.proprietary.data_len,
        sizeof(proprietary_data1), NULL);
    res = memcmp(message.dest_options[2].specific.proprietary.data,
        proprietary_data1, sizeof(proprietary_data1));
    zassert_equal(res, 0, NULL);
    zassert_equal(message.hdr.payload_len, payload_len, NULL);
    res = memcmp(message.hdr.payload, payload, payload_len);
    zassert_equal(res, 0, NULL);
    test_header_modifications(buf, len, bvlc_function, message_id, origin, dest,
        payload, payload_len, false, true);
}

static void test_3_options_dest_different_buffer(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf1[256];
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len = pdu_size;
    bool ret;
    uint16_t vendor_id1;
    uint16_t vendor_id2;
    uint16_t vendor_id3;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_option_type2;
    uint8_t proprietary_option_type3;
    uint8_t proprietary_data1[17];
    uint8_t proprietary_data2[1];
    uint8_t proprietary_data3[43];
    int res;
    const char *err_desc = NULL;

    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);

    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf1, sizeof(buf1), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    vendor_id2 = 0xbeaf;
    proprietary_option_type2 = 0x33;
    memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id2, proprietary_option_type2, proprietary_data2,
        sizeof(proprietary_data2));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf1, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);

    vendor_id3 = 0xF00D;
    proprietary_option_type3 = 0x08;
    memset(proprietary_data3, 0x55, sizeof(proprietary_data3));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id3, proprietary_option_type3, proprietary_data3,
        sizeof(proprietary_data3));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf1, sizeof(buf1), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret =
        bvlc_sc_decode_message(buf1, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
        origin, dest, false, true, payload_len);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.dest_options_num, 3, NULL);
    zassert_equal(
        message.dest_options[0].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.dest_options[0].must_understand, true, NULL);
    zassert_equal(
        message.dest_options[0].packed_header_marker & BVLC_SC_HEADER_MORE,
        BVLC_SC_HEADER_MORE, NULL);
    zassert_equal(
        message.dest_options[0].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.vendor_id,
        vendor_id3, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.option_type,
        proprietary_option_type3, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.data_len,
        sizeof(proprietary_data3), NULL);
    res = memcmp(message.dest_options[0].specific.proprietary.data,
        proprietary_data3, sizeof(proprietary_data3));
    zassert_equal(res, 0, NULL);
    zassert_equal(
        message.dest_options[1].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.dest_options[1].must_understand, true, NULL);
    zassert_equal(
        message.dest_options[1].packed_header_marker & BVLC_SC_HEADER_MORE,
        BVLC_SC_HEADER_MORE, NULL);
    zassert_equal(
        message.dest_options[1].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.vendor_id,
        vendor_id2, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.option_type,
        proprietary_option_type2, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.data_len,
        sizeof(proprietary_data2), NULL);
    res = memcmp(message.dest_options[1].specific.proprietary.data,
        proprietary_data2, sizeof(proprietary_data2));
    zassert_equal(res, 0, NULL);
    zassert_equal(
        message.dest_options[2].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.dest_options[2].must_understand, true, NULL);
    zassert_equal(
        message.dest_options[2].packed_header_marker & BVLC_SC_HEADER_MORE, 0,
        NULL);
    zassert_equal(
        message.dest_options[2].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.dest_options[2].specific.proprietary.vendor_id,
        vendor_id1, NULL);
    zassert_equal(message.dest_options[2].specific.proprietary.option_type,
        proprietary_option_type1, NULL);
    zassert_equal(message.dest_options[2].specific.proprietary.data_len,
        sizeof(proprietary_data1), NULL);
    res = memcmp(message.dest_options[2].specific.proprietary.data,
        proprietary_data1, sizeof(proprietary_data1));
    zassert_equal(res, 0, NULL);
    zassert_equal(message.hdr.payload_len, payload_len, NULL);
    res = memcmp(message.hdr.payload, payload, payload_len);
    zassert_equal(res, 0, NULL);
    test_header_modifications(buf1, len, bvlc_function, message_id, origin,
        dest, payload, payload_len, false, true);
}

static void test_5_options_dest(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len = pdu_size;
    bool ret;
    uint16_t vendor_id1;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_data1[17];
    int i;
    const char *err_desc = NULL;
    (void) bvlc_function;
    (void) message_id;
    (void) origin;
    (void) dest;
    (void) payload;
    (void) payload_len;

    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);

    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    len = pdu_size;
    for (i = 0; i < 5; i++) {
        len = bvlc_sc_add_option_to_destination_options(
            buf, sizeof(buf), buf, len, optbuf, optlen);
        zassert_not_equal(len, 0, NULL);
    }
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_OUT_OF_MEMORY, NULL);
    zassert_equal(class, ERROR_CLASS_RESOURCES, NULL);
}

static void test_options_incorrect_data_bit_dest(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len = pdu_size;
    bool ret;
    uint16_t vendor_id1;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_data1[17];
    int offs = 4;
    const char *err_desc = NULL;
    (void) bvlc_function;
    (void) message_id;
    (void) origin;
    (void) dest;
    (void) payload;
    (void) payload_len;

    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);

    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    if (buf[1] & BVLC_SC_CONTROL_ORIG_VADDR) {
        offs += BVLC_SC_VMAC_SIZE;
    }

    if (buf[1] & BVLC_SC_CONTROL_DEST_VADDR) {
        offs += BVLC_SC_VMAC_SIZE;
    }

    buf[offs] &= ~(BVLC_SC_HEADER_DATA);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_options_incorrect_more_bit_dest(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len = pdu_size;
    bool ret;
    uint16_t vendor_id1;
    uint16_t vendor_id2;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_option_type2;
    uint8_t proprietary_data1[17];
    uint8_t proprietary_data2[5];
    int offs = 4;
    const char *err_desc = NULL;
    (void) bvlc_function;
    (void) message_id;
    (void) origin;
    (void) dest;
    (void) payload;
    (void) payload_len;
    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);

    vendor_id1 = 0x0;
    proprietary_option_type1 = 0x0;
    memset(proprietary_data1, 0x0, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    vendor_id2 = 0xbeaf;
    proprietary_option_type2 = 0x33;
    memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id2, proprietary_option_type2, proprietary_data2,
        sizeof(proprietary_data2));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    if (buf[1] & BVLC_SC_CONTROL_ORIG_VADDR) {
        offs += BVLC_SC_VMAC_SIZE;
    }

    if (buf[1] & BVLC_SC_CONTROL_DEST_VADDR) {
        offs += BVLC_SC_VMAC_SIZE;
    }

    buf[offs] &= ~(BVLC_SC_HEADER_MORE);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    if (error != ERROR_CODE_UNEXPECTED_DATA &&
        error != ERROR_CODE_PARAMETER_OUT_OF_RANGE) {
        zassert_equal(1, 2, NULL);
    }
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_options_mixed_case1(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len = pdu_size;
    bool ret;
    uint16_t vendor_id1;
    uint16_t vendor_id2;
    uint16_t vendor_id3;
    uint16_t vendor_id4;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_option_type2;
    uint8_t proprietary_option_type3;
    uint8_t proprietary_option_type4;
    uint8_t proprietary_data1[17];
    uint8_t proprietary_data2[1];
    uint8_t proprietary_data3[43];
    uint8_t proprietary_data4[23];
    int res;
    const char *err_desc = NULL;

    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);

    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    vendor_id2 = 0xbeaf;
    proprietary_option_type2 = 0x33;
    memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id2, proprietary_option_type2, proprietary_data2,
        sizeof(proprietary_data2));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);

    vendor_id3 = 0xF00D;
    proprietary_option_type3 = 0x08;
    memset(proprietary_data3, 0x55, sizeof(proprietary_data3));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id3, proprietary_option_type3, proprietary_data3,
        sizeof(proprietary_data3));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    vendor_id4 = 0xabcd;
    proprietary_option_type4 = 0x64;
    memset(proprietary_data4, 0x97, sizeof(proprietary_data4));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id4, proprietary_option_type4, proprietary_data4,
        sizeof(proprietary_data4));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
        origin, dest, false, false, payload_len);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.dest_options_num, 2, NULL);
    zassert_equal(message.hdr.data_options_num, 2, NULL);
    zassert_equal(
        message.dest_options[0].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.dest_options[0].must_understand, true, NULL);
    zassert_equal(
        message.dest_options[0].packed_header_marker & BVLC_SC_HEADER_MORE,
        BVLC_SC_HEADER_MORE, NULL);
    zassert_equal(
        message.dest_options[0].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.vendor_id,
        vendor_id3, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.option_type,
        proprietary_option_type3, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.data_len,
        sizeof(proprietary_data3), NULL);
    res = memcmp(message.dest_options[0].specific.proprietary.data,
        proprietary_data3, sizeof(proprietary_data3));
    zassert_equal(res, 0, NULL);
    zassert_equal(
        message.dest_options[1].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.dest_options[1].must_understand, true, NULL);
    zassert_equal(
        message.dest_options[1].packed_header_marker & BVLC_SC_HEADER_MORE, 0,
        NULL);
    zassert_equal(
        message.dest_options[1].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.vendor_id,
        vendor_id1, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.option_type,
        proprietary_option_type1, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.data_len,
        sizeof(proprietary_data1), NULL);
    res = memcmp(message.dest_options[1].specific.proprietary.data,
        proprietary_data1, sizeof(proprietary_data1));
    zassert_equal(res, 0, NULL);
    /* data options */

    zassert_equal(
        message.data_options[0].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.data_options[0].must_understand, true, NULL);
    zassert_equal(
        message.data_options[0].packed_header_marker & BVLC_SC_HEADER_MORE,
        BVLC_SC_HEADER_MORE, NULL);
    zassert_equal(
        message.data_options[0].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.data_options[0].specific.proprietary.vendor_id,
        vendor_id4, NULL);
    zassert_equal(message.data_options[0].specific.proprietary.option_type,
        proprietary_option_type4, NULL);
    zassert_equal(message.data_options[0].specific.proprietary.data_len,
        sizeof(proprietary_data4), NULL);
    res = memcmp(message.data_options[0].specific.proprietary.data,
        proprietary_data4, sizeof(proprietary_data4));
    zassert_equal(res, 0, NULL);
    zassert_equal(
        message.data_options[1].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.data_options[1].must_understand, true, NULL);
    zassert_equal(
        message.data_options[1].packed_header_marker & BVLC_SC_HEADER_MORE, 0,
        NULL);
    zassert_equal(
        message.data_options[1].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.data_options[1].specific.proprietary.vendor_id,
        vendor_id2, NULL);
    zassert_equal(message.data_options[1].specific.proprietary.option_type,
        proprietary_option_type2, NULL);
    zassert_equal(message.data_options[1].specific.proprietary.data_len,
        sizeof(proprietary_data2), NULL);
    res = memcmp(message.data_options[1].specific.proprietary.data,
        proprietary_data2, sizeof(proprietary_data2));
    zassert_equal(res, 0, NULL);
    zassert_equal(message.hdr.payload_len, payload_len, NULL);
    res = memcmp(message.hdr.payload, payload, payload_len);
    zassert_equal(res, 0, NULL);
    test_header_modifications(buf, len, bvlc_function, message_id, origin, dest,
        payload, payload_len, false, false);
}

static void test_options_mixed_case2(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len = pdu_size;
    bool ret;
    uint16_t vendor_id1;
    uint16_t vendor_id2;
    uint16_t vendor_id3;
    uint16_t vendor_id4;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_option_type2;
    uint8_t proprietary_option_type3;
    uint8_t proprietary_option_type4;
    uint8_t proprietary_data1[17];
    uint8_t proprietary_data2[1];
    uint8_t proprietary_data3[43];
    uint8_t proprietary_data4[23];
    int res;
    const char *err_desc = NULL;

    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);

    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    vendor_id2 = 0xbeaf;
    proprietary_option_type2 = 0x33;
    memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id2, proprietary_option_type2, proprietary_data2,
        sizeof(proprietary_data2));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);

    vendor_id3 = 0xF00D;
    proprietary_option_type3 = 0x08;
    memset(proprietary_data3, 0x55, sizeof(proprietary_data3));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id3, proprietary_option_type3, proprietary_data3,
        sizeof(proprietary_data3));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    vendor_id4 = 0xabcd;
    proprietary_option_type4 = 0x64;
    memset(proprietary_data4, 0x97, sizeof(proprietary_data4));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id4, proprietary_option_type4, proprietary_data4,
        sizeof(proprietary_data4));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
        origin, dest, false, false, payload_len);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.dest_options_num, 2, NULL);
    zassert_equal(message.hdr.data_options_num, 2, NULL);
    zassert_equal(
        message.dest_options[0].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.dest_options[0].must_understand, true, NULL);
    zassert_equal(
        message.dest_options[0].packed_header_marker & BVLC_SC_HEADER_MORE,
        BVLC_SC_HEADER_MORE, NULL);
    zassert_equal(
        message.dest_options[0].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.vendor_id,
        vendor_id4, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.option_type,
        proprietary_option_type4, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.data_len,
        sizeof(proprietary_data4), NULL);
    res = memcmp(message.dest_options[0].specific.proprietary.data,
        proprietary_data4, sizeof(proprietary_data4));
    zassert_equal(res, 0, NULL);
    zassert_equal(
        message.dest_options[1].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.dest_options[1].must_understand, true, NULL);
    zassert_equal(
        message.dest_options[1].packed_header_marker & BVLC_SC_HEADER_MORE, 0,
        NULL);
    zassert_equal(
        message.dest_options[1].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.vendor_id,
        vendor_id3, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.option_type,
        proprietary_option_type3, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.data_len,
        sizeof(proprietary_data3), NULL);
    res = memcmp(message.dest_options[1].specific.proprietary.data,
        proprietary_data3, sizeof(proprietary_data3));
    zassert_equal(res, 0, NULL);
    /* data options */

    zassert_equal(
        message.data_options[0].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.data_options[0].must_understand, true, NULL);
    zassert_equal(
        message.data_options[0].packed_header_marker & BVLC_SC_HEADER_MORE,
        BVLC_SC_HEADER_MORE, NULL);
    zassert_equal(
        message.data_options[0].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.data_options[0].specific.proprietary.vendor_id,
        vendor_id2, NULL);
    zassert_equal(message.data_options[0].specific.proprietary.option_type,
        proprietary_option_type2, NULL);
    zassert_equal(message.data_options[0].specific.proprietary.data_len,
        sizeof(proprietary_data2), NULL);
    res = memcmp(message.data_options[0].specific.proprietary.data,
        proprietary_data2, sizeof(proprietary_data2));
    zassert_equal(res, 0, NULL);
    zassert_equal(
        message.data_options[1].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.data_options[1].must_understand, true, NULL);
    zassert_equal(
        message.data_options[1].packed_header_marker & BVLC_SC_HEADER_MORE, 0,
        NULL);
    zassert_equal(
        message.data_options[1].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.data_options[1].specific.proprietary.vendor_id,
        vendor_id1, NULL);
    zassert_equal(message.data_options[1].specific.proprietary.option_type,
        proprietary_option_type1, NULL);
    zassert_equal(message.data_options[1].specific.proprietary.data_len,
        sizeof(proprietary_data1), NULL);
    res = memcmp(message.data_options[1].specific.proprietary.data,
        proprietary_data1, sizeof(proprietary_data1));
    zassert_equal(res, 0, NULL);
    zassert_equal(message.hdr.payload_len, payload_len, NULL);
    res = memcmp(message.hdr.payload, payload, payload_len);
    zassert_equal(res, 0, NULL);
    test_header_modifications(buf, len, bvlc_function, message_id, origin, dest,
        payload, payload_len, false, false);
}

static void test_options_mixed_case3(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    int optlen;
    int len = pdu_size;
    bool ret;
    uint16_t vendor_id1;
    uint16_t vendor_id2;
    uint16_t vendor_id3;
    uint16_t vendor_id4;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_option_type2;
    uint8_t proprietary_option_type3;
    uint8_t proprietary_option_type4;
    uint8_t proprietary_data1[17];
    uint8_t proprietary_data2[1];
    uint8_t proprietary_data3[43];
    uint8_t proprietary_data4[23];
    int res;
    const char *err_desc = NULL;

    zassert_equal(true, sizeof(buf) >= pdu_size ? true : false, NULL);
    memcpy(buf, pdu, pdu_size);

    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    vendor_id2 = 0xbeaf;
    proprietary_option_type2 = 0x33;
    memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id2, proprietary_option_type2, proprietary_data2,
        sizeof(proprietary_data2));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);

    vendor_id3 = 0xF00D;
    proprietary_option_type3 = 0x08;
    memset(proprietary_data3, 0x55, sizeof(proprietary_data3));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id3, proprietary_option_type3, proprietary_data3,
        sizeof(proprietary_data3));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    vendor_id4 = 0xabcd;
    proprietary_option_type4 = 0x64;
    memset(proprietary_data4, 0x97, sizeof(proprietary_data4));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id4, proprietary_option_type4, proprietary_data4,
        sizeof(proprietary_data4));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
        origin, dest, false, false, payload_len);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.dest_options_num, 2, NULL);
    zassert_equal(message.hdr.data_options_num, 2, NULL);
    zassert_equal(
        message.dest_options[0].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.dest_options[0].must_understand, true, NULL);
    zassert_equal(
        message.dest_options[0].packed_header_marker & BVLC_SC_HEADER_MORE,
        BVLC_SC_HEADER_MORE, NULL);
    zassert_equal(
        message.dest_options[0].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.vendor_id,
        vendor_id2, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.option_type,
        proprietary_option_type2, NULL);
    zassert_equal(message.dest_options[0].specific.proprietary.data_len,
        sizeof(proprietary_data2), NULL);
    res = memcmp(message.dest_options[0].specific.proprietary.data,
        proprietary_data2, sizeof(proprietary_data2));
    zassert_equal(res, 0, NULL);
    zassert_equal(
        message.dest_options[1].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.dest_options[1].must_understand, true, NULL);
    zassert_equal(
        message.dest_options[1].packed_header_marker & BVLC_SC_HEADER_MORE, 0,
        NULL);
    zassert_equal(
        message.dest_options[1].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.vendor_id,
        vendor_id1, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.option_type,
        proprietary_option_type1, NULL);
    zassert_equal(message.dest_options[1].specific.proprietary.data_len,
        sizeof(proprietary_data1), NULL);
    res = memcmp(message.dest_options[1].specific.proprietary.data,
        proprietary_data1, sizeof(proprietary_data1));
    zassert_equal(res, 0, NULL);
    /* data options */

    zassert_equal(
        message.data_options[0].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.data_options[0].must_understand, true, NULL);
    zassert_equal(
        message.data_options[0].packed_header_marker & BVLC_SC_HEADER_MORE,
        BVLC_SC_HEADER_MORE, NULL);
    zassert_equal(
        message.data_options[0].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.data_options[0].specific.proprietary.vendor_id,
        vendor_id4, NULL);
    zassert_equal(message.data_options[0].specific.proprietary.option_type,
        proprietary_option_type4, NULL);
    zassert_equal(message.data_options[0].specific.proprietary.data_len,
        sizeof(proprietary_data4), NULL);
    res = memcmp(message.data_options[0].specific.proprietary.data,
        proprietary_data4, sizeof(proprietary_data4));
    zassert_equal(res, 0, NULL);
    zassert_equal(
        message.data_options[1].type, BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
    zassert_equal(message.data_options[1].must_understand, true, NULL);
    zassert_equal(
        message.data_options[1].packed_header_marker & BVLC_SC_HEADER_MORE, 0,
        NULL);
    zassert_equal(
        message.data_options[1].packed_header_marker & BVLC_SC_HEADER_DATA,
        BVLC_SC_HEADER_DATA, NULL);
    zassert_equal(message.data_options[1].specific.proprietary.vendor_id,
        vendor_id3, NULL);
    zassert_equal(message.data_options[1].specific.proprietary.option_type,
        proprietary_option_type3, NULL);
    zassert_equal(message.data_options[1].specific.proprietary.data_len,
        sizeof(proprietary_data3), NULL);
    res = memcmp(message.data_options[1].specific.proprietary.data,
        proprietary_data3, sizeof(proprietary_data3));
    zassert_equal(res, 0, NULL);
    zassert_equal(message.hdr.payload_len, payload_len, NULL);
    res = memcmp(message.hdr.payload, payload, payload_len);
    zassert_equal(res, 0, NULL);
    test_header_modifications(buf, len, bvlc_function, message_id, origin, dest,
        payload, payload_len, false, false);
}

static void test_options(uint8_t *pdu,
    uint16_t pdu_size,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    bool test_dest_option,
    bool test_data_option,
    uint8_t *payload,
    uint16_t payload_len,
    bool ignore_more_bit_test)
{
    if (!test_dest_option && test_data_option) {
        test_1_option_data(pdu, pdu_size, bvlc_function, message_id, origin,
            dest, payload, payload_len);
        test_3_options_data(pdu, pdu_size, bvlc_function, message_id, origin,
            dest, payload, payload_len);
        test_3_options_different_buffer_data(pdu, pdu_size, bvlc_function,
            message_id, origin, dest, payload, payload_len);
        test_5_options_data(pdu, pdu_size, bvlc_function, message_id, origin,
            dest, payload, payload_len);
        if (!ignore_more_bit_test) {
            test_options_incorrect_more_bit_data(pdu, pdu_size, bvlc_function,
                message_id, origin, dest, payload, payload_len);
        }
        test_options_incorrect_data_bit_data(pdu, pdu_size, bvlc_function,
            message_id, origin, dest, payload, payload_len);
    } else if (test_dest_option && !test_data_option) {
        test_1_option_dest_incorrect(pdu, pdu_size, bvlc_function, message_id,
            origin, dest, payload, payload_len);
        test_1_option_dest(pdu, pdu_size, bvlc_function, message_id, origin,
            dest, payload, payload_len);
        test_3_options_dest(pdu, pdu_size, bvlc_function, message_id, origin,
            dest, payload, payload_len);
        test_3_options_dest_different_buffer(pdu, pdu_size, bvlc_function,
            message_id, origin, dest, payload, payload_len);
        test_5_options_dest(pdu, pdu_size, bvlc_function, message_id, origin,
            dest, payload, payload_len);
        if (!ignore_more_bit_test) {
            test_options_incorrect_more_bit_dest(pdu, pdu_size, bvlc_function,
                message_id, origin, dest, payload, payload_len);
        }
        test_options_incorrect_data_bit_dest(pdu, pdu_size, bvlc_function,
            message_id, origin, dest, payload, payload_len);
    } else if (test_dest_option && test_data_option) {
        test_options_mixed_case1(pdu, pdu_size, bvlc_function, message_id,
            origin, dest, payload, payload_len);
        test_options_mixed_case2(pdu, pdu_size, bvlc_function, message_id,
            origin, dest, payload, payload_len);
        test_options_mixed_case3(pdu, pdu_size, bvlc_function, message_id,
            origin, dest, payload, payload_len);
    }
}

static void test_BVLC_RESULT(void)
{
    uint8_t sbuf[256 + BSC_PRE];
    uint8_t *buf = &sbuf[BSC_PRE];
    int buf_len = 256;
    int len;
    uint8_t optbuf[256];
    int optlen;
    BACNET_SC_VMAC_ADDRESS origin;
    BACNET_SC_VMAC_ADDRESS new_origin;
    BACNET_SC_VMAC_ADDRESS dest;
    BACNET_SC_VMAC_ADDRESS test_dest;
    uint16_t message_id = 0x7777;
    uint8_t result_bvlc_function = 3;
    bool ret;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint8_t error_header_marker = 0xcc;
    uint16_t error_class = 0xaa;
    uint16_t error_code = 0xdd;
    char error_details_string[100];
    const char *err_desc = NULL;
    uint8_t *pdu;

    sprintf(error_details_string, "%s", "something bad has happend");

    memset(origin.address, 0x23, BVLC_SC_VMAC_SIZE);
    memset(dest.address, 0x44, BVLC_SC_VMAC_SIZE);
    memset(new_origin.address, 0x93, BVLC_SC_VMAC_SIZE);

    /* origin and dest presented */
    len = bvlc_sc_encode_result(buf, buf_len, message_id, &origin, &dest,
        result_bvlc_function, 0, NULL, NULL, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id,
        &origin, &dest, true, true, 2);
    zassert_equal(ret, true, NULL);
    zassert_equal(
        message.payload.result.bvlc_function, result_bvlc_function, NULL);
    zassert_equal(message.payload.result.result, 0, NULL);
    test_options(buf, len, BVLC_SC_RESULT, message_id, &origin, &dest, true,
        false, message.hdr.payload, message.hdr.payload_len, false);
    ret = bvlc_sc_pdu_has_no_dest(buf, len);
    zassert_equal(ret, false, NULL);
    memset(&test_dest, 0, sizeof(test_dest));
    ret = bvlc_sc_pdu_get_dest(buf, len, &test_dest);
    zassert_equal(ret, true, NULL);
    zassert_equal(0,
        memcmp(&dest.address, &test_dest.address, sizeof(test_dest.address)),
        NULL);

    pdu = buf;
    len = bvlc_sc_set_orig(&pdu, len, &new_origin);
    ret = bvlc_sc_decode_message(pdu, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id,
        &new_origin, &dest, true, true, 2);
    zassert_equal(ret, true, NULL);
    zassert_equal(
        message.payload.result.bvlc_function, result_bvlc_function, NULL);
    zassert_equal(message.payload.result.result, 0, NULL);
    test_options(pdu, len, BVLC_SC_RESULT, message_id, &new_origin, &dest, true,
        false, message.hdr.payload, message.hdr.payload_len, false);
    pdu = buf;
    len = bvlc_sc_remove_orig_and_dest(&pdu, len);
    ret = bvlc_sc_decode_message(pdu, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(
        &message.hdr, BVLC_SC_RESULT, message_id, NULL, NULL, true, true, 2);
    zassert_equal(ret, true, NULL);
    zassert_equal(
        message.payload.result.bvlc_function, result_bvlc_function, NULL);
    zassert_equal(message.payload.result.result, 0, NULL);
    test_options(pdu, len, BVLC_SC_RESULT, message_id, NULL, NULL, true, false,
        message.hdr.payload, message.hdr.payload_len, false);

    memset(buf, 0, buf_len);
    memset(&message, 0, sizeof(message));

    /* origin presented */
    len = bvlc_sc_encode_result(buf, buf_len, message_id, &origin, NULL,
        result_bvlc_function, 0, NULL, NULL, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_pdu_has_no_dest(buf, len);
    zassert_equal(ret, true, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(
        &message.hdr, BVLC_SC_RESULT, message_id, &origin, NULL, true, true, 2);
    zassert_equal(ret, true, NULL);
    zassert_equal(
        message.payload.result.bvlc_function, result_bvlc_function, NULL);
    zassert_equal(message.payload.result.result, 0, NULL);
    test_options(buf, len, BVLC_SC_RESULT, message_id, &origin, NULL, true,
        false, message.hdr.payload, message.hdr.payload_len, false);
    pdu = buf;
    len = bvlc_sc_remove_orig_and_dest(&pdu, len);
    ret = bvlc_sc_decode_message(pdu, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(
        &message.hdr, BVLC_SC_RESULT, message_id, NULL, NULL, true, true, 2);
    zassert_equal(ret, true, NULL);
    zassert_equal(
        message.payload.result.bvlc_function, result_bvlc_function, NULL);
    zassert_equal(message.payload.result.result, 0, NULL);
    test_options(pdu, len, BVLC_SC_RESULT, message_id, NULL, NULL, true, false,
        message.hdr.payload, message.hdr.payload_len, false);

    memset(buf, 0, buf_len);
    memset(&message, 0, sizeof(message));

    /* dest presented */
    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, &dest,
        result_bvlc_function, 0, NULL, NULL, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(
        &message.hdr, BVLC_SC_RESULT, message_id, NULL, &dest, true, true, 2);
    zassert_equal(ret, true, NULL);
    zassert_equal(
        message.payload.result.bvlc_function, result_bvlc_function, NULL);
    zassert_equal(message.payload.result.result, 0, NULL);
    test_options(buf, len, BVLC_SC_RESULT, message_id, NULL, &dest, true, false,
        message.hdr.payload, message.hdr.payload_len, false);
    ret = bvlc_sc_pdu_has_no_dest(buf, len);
    zassert_equal(ret, false, NULL);
    memset(&test_dest, 0, sizeof(test_dest));
    ret = bvlc_sc_pdu_get_dest(buf, len, &test_dest);
    zassert_equal(ret, true, NULL);
    zassert_equal(0,
        memcmp(&dest.address, &test_dest.address, sizeof(test_dest.address)),
        NULL);
    pdu = buf;
    len = bvlc_sc_set_orig(&pdu, len, &new_origin);
    ret = bvlc_sc_decode_message(pdu, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id,
        &new_origin, &dest, true, true, 2);
    zassert_equal(ret, true, NULL);
    zassert_equal(
        message.payload.result.bvlc_function, result_bvlc_function, NULL);
    zassert_equal(message.payload.result.result, 0, NULL);
    test_options(pdu, len, BVLC_SC_RESULT, message_id, &new_origin, &dest, true,
        false, message.hdr.payload, message.hdr.payload_len, false);

    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, &dest,
        result_bvlc_function, 0, NULL, NULL, NULL, NULL);
    pdu = buf;
    len = bvlc_sc_remove_orig_and_dest(&pdu, len);
    ret = bvlc_sc_decode_message(pdu, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(
        &message.hdr, BVLC_SC_RESULT, message_id, NULL, NULL, true, true, 2);
    zassert_equal(ret, true, NULL);
    zassert_equal(
        message.payload.result.bvlc_function, result_bvlc_function, NULL);
    zassert_equal(message.payload.result.result, 0, NULL);
    test_options(pdu, len, BVLC_SC_RESULT, message_id, NULL, NULL, true, false,
        message.hdr.payload, message.hdr.payload_len, false);

    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, &dest,
        result_bvlc_function, 0, NULL, NULL, NULL, NULL);

    bvlc_sc_remove_dest_set_orig(buf, len, &new_origin);

    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id,
        &new_origin, NULL, true, true, 2);
    zassert_equal(ret, true, NULL);
    zassert_equal(
        message.payload.result.bvlc_function, result_bvlc_function, NULL);
    zassert_equal(message.payload.result.result, 0, NULL);
    test_options(buf, len, BVLC_SC_RESULT, message_id, &new_origin, NULL, true,
        false, message.hdr.payload, message.hdr.payload_len, false);

    memset(buf, 0, buf_len);
    memset(&message, 0, sizeof(message));

    /* dest and origin absent */
    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, NULL,
        result_bvlc_function, 0, NULL, NULL, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_pdu_has_no_dest(buf, len);
    zassert_equal(ret, true, NULL);
    memset(&test_dest, 0, sizeof(test_dest));
    ret = bvlc_sc_pdu_get_dest(buf, len, &test_dest);
    zassert_equal(ret, false, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(
        &message.hdr, BVLC_SC_RESULT, message_id, NULL, NULL, true, true, 2);
    zassert_equal(ret, true, NULL);
    zassert_equal(
        message.payload.result.bvlc_function, result_bvlc_function, NULL);
    zassert_equal(message.payload.result.result, 0, NULL);
    test_options(buf, len, BVLC_SC_RESULT, message_id, NULL, NULL, true, false,
        message.hdr.payload, message.hdr.payload_len, false);

    memset(buf, 0, buf_len);
    memset(&message, 0, sizeof(message));

    /* nak, no details string */
    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, NULL,
        result_bvlc_function, 1, &error_header_marker, &error_class,
        &error_code, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(
        &message.hdr, BVLC_SC_RESULT, message_id, NULL, NULL, true, true, 7);
    zassert_equal(ret, true, NULL);
    zassert_equal(
        message.payload.result.bvlc_function, result_bvlc_function, NULL);
    zassert_equal(message.payload.result.result, 1, NULL);
    zassert_equal(
        message.payload.result.error_header_marker, error_header_marker, NULL);
    zassert_equal(message.payload.result.error_class, error_class, NULL);
    zassert_equal(message.payload.result.error_code, error_code, NULL);
    zassert_equal(message.payload.result.utf8_details_string, NULL, NULL);
    zassert_equal(message.payload.result.utf8_details_string_len, 0, NULL);
    test_options(buf, len, BVLC_SC_RESULT, message_id, NULL, NULL, true, false,
        message.hdr.payload, message.hdr.payload_len, false);

    memset(buf, 0, buf_len);
    memset(&message, 0, sizeof(message));

    /* nak , details string */
    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, NULL,
        result_bvlc_function, 1, &error_header_marker, &error_class,
        &error_code, (uint8_t *)error_details_string);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id, NULL,
        NULL, true, true, 7 + strlen(error_details_string));
    zassert_equal(ret, true, NULL);
    zassert_equal(
        message.payload.result.bvlc_function, result_bvlc_function, NULL);
    zassert_equal(message.payload.result.result, 1, NULL);
    zassert_equal(
        message.payload.result.error_header_marker, error_header_marker, NULL);
    zassert_equal(message.payload.result.error_class, error_class, NULL);
    zassert_equal(message.payload.result.error_code, error_code, NULL);
    zassert_equal(message.payload.result.utf8_details_string_len,
        strlen(error_details_string), NULL);
    ret = memcmp(message.payload.result.utf8_details_string,
              error_details_string, strlen(error_details_string)) == 0
        ? true
        : false;
    zassert_equal(ret, true, NULL);
    test_options(buf, len, BVLC_SC_RESULT, message_id, NULL, NULL, true, false,
        message.hdr.payload, message.hdr.payload_len, false);

    memset(buf, 0, buf_len);
    memset(&message, 0, sizeof(message));

    /* dest and origin, nak , details string */
    len = bvlc_sc_encode_result(buf, buf_len, message_id, &origin, &dest,
        result_bvlc_function, 1, &error_header_marker, &error_class,
        &error_code, (uint8_t *)error_details_string);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id,
        &origin, &dest, true, true, 7 + strlen(error_details_string));
    zassert_equal(ret, true, NULL);
    zassert_equal(
        message.payload.result.bvlc_function, result_bvlc_function, NULL);
    zassert_equal(message.payload.result.result, 1, NULL);
    zassert_equal(
        message.payload.result.error_header_marker, error_header_marker, NULL);
    zassert_equal(message.payload.result.error_class, error_class, NULL);
    zassert_equal(message.payload.result.error_code, error_code, NULL);
    zassert_equal(message.payload.result.utf8_details_string_len,
        strlen(error_details_string), NULL);
    ret = memcmp(message.payload.result.utf8_details_string,
              error_details_string, strlen(error_details_string)) == 0
        ? true
        : false;
    zassert_equal(ret, true, NULL);
    test_options(buf, len, BVLC_SC_RESULT, message_id, &origin, &dest, true,
        false, message.hdr.payload, message.hdr.payload_len, false);

    memset(buf, 0, buf_len);
    memset(&message, 0, sizeof(message));

    /* truncated message, case 1 */
    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, NULL,
        result_bvlc_function, 1, &error_header_marker, &error_class,
        &error_code, (uint8_t *)error_details_string);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 2 */

    ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    memset(buf, 0, buf_len);
    memset(&message, 0, sizeof(message));

    /* origin and dest absent, result ok*/
    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, NULL,
        result_bvlc_function, 0, NULL, NULL, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    test_options(buf, len, BVLC_SC_RESULT, message_id, NULL, NULL, true, false,
        message.hdr.payload, 2, false);

    /* data option test */
    memset(buf, 0, buf_len);
    memset(&message, 0, sizeof(message));
    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, NULL,
        result_bvlc_function, 0, NULL, NULL, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true);
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_INCONSISTENT_PARAMETERS, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* zero payload test */
    memset(buf, 0, buf_len);
    memset(&message, 0, sizeof(message));
    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, NULL,
        result_bvlc_function, 0, NULL, NULL, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_PAYLOAD_EXPECTED, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* bad result code */
    memset(buf, 0, buf_len);
    memset(&message, 0, sizeof(message));
    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, NULL,
        result_bvlc_function, 0, NULL, NULL, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    buf[5] = 4;
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_PARAMETER_OUT_OF_RANGE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* utf-8 string with zero symbol inside */
    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, NULL,
        result_bvlc_function, 1, &error_header_marker, &error_class,
        &error_code, (uint8_t *)error_details_string);
    zassert_not_equal(len, 0, NULL);
    buf[13] = 0;
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_INCONSISTENT_PARAMETERS, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* bad payload case 1 */
    memset(buf, 0, buf_len);
    memset(&message, 0, sizeof(message));
    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, NULL,
        result_bvlc_function, 1, &error_header_marker, &error_class,
        &error_code, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 10, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* bad payload case 2 */
    memset(buf, 0, buf_len);
    memset(&message, 0, sizeof(message));
    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, NULL,
        result_bvlc_function, 0, NULL, NULL, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* bad payload case 3 */
    memset(buf, 0, buf_len);
    memset(&message, 0, sizeof(message));
    len = bvlc_sc_encode_result(buf, buf_len, message_id, NULL, NULL,
        result_bvlc_function, 0, NULL, NULL, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 7, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_UNEXPECTED_DATA, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_ENCAPSULATED_NPDU(void)
{
    uint8_t buf[256];
    int len;
    uint8_t npdu[256];
    int npdulen;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint16_t message_id = 0x1789;
    int res;
    BACNET_SC_VMAC_ADDRESS origin;
    BACNET_SC_VMAC_ADDRESS dest;
    bool ret;
    const char *err_desc = NULL;

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    memset(&origin.address, 0x63, BVLC_SC_VMAC_SIZE);
    memset(&dest.address, 0x24, BVLC_SC_VMAC_SIZE);
    memset(npdu, 0x99, sizeof(npdu));
    npdulen = 50;

    /* dest and origin absent */
    len = bvlc_sc_encode_encapsulated_npdu(
        buf, sizeof(buf), message_id, NULL, NULL, npdu, npdulen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ENCAPSULATED_NPDU,
        message_id, NULL, NULL, true, true, npdulen);
    zassert_equal(ret, true, NULL);
    zassert_not_equal(message.hdr.payload, 0, NULL);
    zassert_equal(message.hdr.payload_len, npdulen, NULL);
    res = memcmp(message.hdr.payload, npdu, npdulen);
    zassert_equal(res, 0, NULL);
    test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id, NULL, NULL,
        true, false, npdu, npdulen, true);
    test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id, NULL, NULL,
        false, true, npdu, npdulen, true);
    test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id, NULL, NULL,
        true, true, npdu, npdulen, true);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* origin is presented, dest is absent */
    len = bvlc_sc_encode_encapsulated_npdu(
        buf, sizeof(buf), message_id, &origin, NULL, npdu, npdulen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ENCAPSULATED_NPDU,
        message_id, &origin, NULL, true, true, npdulen);
    zassert_equal(ret, true, NULL);
    zassert_not_equal(message.hdr.payload, 0, NULL);
    zassert_equal(message.hdr.payload_len, npdulen, NULL);
    res = memcmp(message.hdr.payload, npdu, npdulen);
    zassert_equal(res, 0, NULL);
    test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id, &origin, NULL,
        true, false, npdu, npdulen, true);
    test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id, &origin, NULL,
        false, true, npdu, npdulen, true);
    test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id, &origin, NULL,
        true, true, npdu, npdulen, true);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* origin is absent, dest is presented */
    len = bvlc_sc_encode_encapsulated_npdu(
        buf, sizeof(buf), message_id, NULL, &dest, npdu, npdulen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ENCAPSULATED_NPDU,
        message_id, NULL, &dest, true, true, npdulen);
    zassert_equal(ret, true, NULL);
    zassert_not_equal(message.hdr.payload, 0, NULL);
    zassert_equal(message.hdr.payload_len, npdulen, NULL);
    res = memcmp(message.hdr.payload, npdu, npdulen);
    zassert_equal(res, 0, NULL);
    test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id, NULL, &dest,
        true, false, npdu, npdulen, true);
    test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id, NULL, &dest,
        false, true, npdu, npdulen, true);
    test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id, NULL, &dest,
        true, true, npdu, npdulen, true);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* both dest and origin are presented */
    len = bvlc_sc_encode_encapsulated_npdu(
        buf, sizeof(buf), message_id, &origin, &dest, npdu, npdulen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ENCAPSULATED_NPDU,
        message_id, &origin, &dest, true, true, npdulen);
    zassert_equal(ret, true, NULL);
    zassert_not_equal(message.hdr.payload, 0, NULL);
    zassert_equal(message.hdr.payload_len, npdulen, NULL);
    res = memcmp(message.hdr.payload, npdu, npdulen);
    zassert_equal(res, 0, NULL);
    test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id, &origin,
        &dest, true, false, npdu, npdulen, true);
    test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id, &origin,
        &dest, false, true, npdu, npdulen, true);
    test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id, &origin,
        &dest, true, true, npdu, npdulen, true);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* truncated message, case 1 */
    len = bvlc_sc_encode_encapsulated_npdu(
        buf, sizeof(buf), message_id, &origin, &dest, npdu, npdulen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 2 */

    ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* truncated message, case 3 */

    ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 4 */

    ret = bvlc_sc_decode_message(buf, 16, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_PAYLOAD_EXPECTED, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 5 */

    ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* zero payload test */
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    len = bvlc_sc_encode_encapsulated_npdu(
        buf, sizeof(buf), message_id, &origin, &dest, npdu, 0);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_PAYLOAD_EXPECTED, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* 1 byte payload test */
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    len = bvlc_sc_encode_encapsulated_npdu(
        buf, sizeof(buf), message_id, &origin, &dest, npdu, 1);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ENCAPSULATED_NPDU,
        message_id, &origin, &dest, true, true, 1);
    zassert_equal(ret, true, NULL);
}

static void test_ADDRESS_RESOLUTION(void)
{
    uint8_t buf[256];
    int len;
    uint8_t optbuf[256];
    int optlen;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint16_t message_id = 0x514a;
    BACNET_SC_VMAC_ADDRESS origin;
    BACNET_SC_VMAC_ADDRESS dest;
    bool ret;
    const char *err_desc = NULL;

    memset(&origin.address, 0x27, BVLC_SC_VMAC_SIZE);
    memset(&dest.address, 0xaa, BVLC_SC_VMAC_SIZE);

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* dest and origin absent */
    len = bvlc_sc_encode_address_resolution(
        buf, sizeof(buf), message_id, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION,
        message_id, NULL, NULL, true, true, 0);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 0, NULL);
    test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION, message_id, NULL, NULL,
        true, false, NULL, 0, false);

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* origin is presented, dest is absent */
    len = bvlc_sc_encode_address_resolution(
        buf, sizeof(buf), message_id, &origin, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION,
        message_id, &origin, NULL, true, true, 0);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 0, NULL);
    test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION, message_id, &origin,
        NULL, true, false, NULL, 0, false);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* origin is absent, dest is presented */
    len = bvlc_sc_encode_address_resolution(
        buf, sizeof(buf), message_id, NULL, &dest);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION,
        message_id, NULL, &dest, true, true, 0);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 0, NULL);
    test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION, message_id, NULL, &dest,
        true, false, NULL, 0, false);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* origin and dest are presented */
    len = bvlc_sc_encode_address_resolution(
        buf, sizeof(buf), message_id, &origin, &dest);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION,
        message_id, &origin, &dest, true, true, 0);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 0, NULL);
    test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION, message_id, &origin,
        &dest, true, false, NULL, 0, false);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* truncated message, case 1 */
    len = bvlc_sc_encode_address_resolution(
        buf, sizeof(buf), message_id, &origin, &dest);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 2 */

    ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* truncated message, case 3 */

    ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 5 */

    ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* data options test */
    optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true);
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_INCONSISTENT_PARAMETERS, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_ADDRESS_RESOLUTION_ACK(void)
{
    uint8_t buf[256];
    int len;
    uint8_t optbuf[256];
    int optlen;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint16_t message_id = 0xf1d3;
    int res;
    BACNET_SC_VMAC_ADDRESS origin;
    BACNET_SC_VMAC_ADDRESS dest;
    bool ret;
    char web_socket_uris[256];
    const char *err_desc = NULL;

    web_socket_uris[0] = 0;
    sprintf(web_socket_uris, "%s %s", "web_socket_uri1", "web_socket_uri2");
    memset(&origin.address, 0x91, BVLC_SC_VMAC_SIZE);
    memset(&dest.address, 0xef, BVLC_SC_VMAC_SIZE);

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* dest and origin absent */
    len = bvlc_sc_encode_address_resolution_ack(buf, sizeof(buf), message_id,
        NULL, NULL, (uint8_t *)web_socket_uris, strlen(web_socket_uris));
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION_ACK,
        message_id, NULL, NULL, true, true, strlen(web_socket_uris));
    zassert_equal(ret, true, NULL);
    res = memcmp(message.hdr.payload, web_socket_uris, strlen(web_socket_uris));
    zassert_equal(res, 0, NULL);
    zassert_not_equal(
        message.payload.address_resolution_ack.utf8_websocket_uri_string, NULL,
        NULL);
    zassert_equal(
        message.payload.address_resolution_ack.utf8_websocket_uri_string_len,
        strlen(web_socket_uris), NULL);
    zassert_equal(message.hdr.payload_len, strlen(web_socket_uris), NULL);
    test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION_ACK, message_id, NULL,
        NULL, true, false, (uint8_t *)web_socket_uris, strlen(web_socket_uris),
        true);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* origin is presented, dest is absent */
    len = bvlc_sc_encode_address_resolution_ack(buf, sizeof(buf), message_id,
        &origin, NULL, (uint8_t *)web_socket_uris, strlen(web_socket_uris));
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION_ACK,
        message_id, &origin, NULL, true, true, strlen(web_socket_uris));
    zassert_equal(ret, true, NULL);
    res = memcmp(message.hdr.payload, web_socket_uris, strlen(web_socket_uris));
    zassert_equal(res, 0, NULL);
    zassert_not_equal(
        message.payload.address_resolution_ack.utf8_websocket_uri_string, NULL,
        NULL);
    zassert_equal(
        message.payload.address_resolution_ack.utf8_websocket_uri_string_len,
        strlen(web_socket_uris), NULL);
    zassert_equal(message.hdr.payload_len, strlen(web_socket_uris), NULL);
    test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION_ACK, message_id, &origin,
        NULL, true, false, (uint8_t *)web_socket_uris, strlen(web_socket_uris),
        true);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* origin is absent, dest is presented */
    len = bvlc_sc_encode_address_resolution_ack(buf, sizeof(buf), message_id,
        NULL, &dest, (uint8_t *)web_socket_uris, strlen(web_socket_uris));
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION_ACK,
        message_id, NULL, &dest, true, true, strlen(web_socket_uris));
    zassert_equal(ret, true, NULL);
    res = memcmp(message.hdr.payload, web_socket_uris, strlen(web_socket_uris));
    zassert_equal(res, 0, NULL);
    zassert_not_equal(
        message.payload.address_resolution_ack.utf8_websocket_uri_string, NULL,
        NULL);
    zassert_equal(
        message.payload.address_resolution_ack.utf8_websocket_uri_string_len,
        strlen(web_socket_uris), NULL);
    zassert_equal(message.hdr.payload_len, strlen(web_socket_uris), NULL);
    test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION_ACK, message_id, NULL,
        &dest, true, false, (uint8_t *)web_socket_uris, strlen(web_socket_uris),
        true);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* origin and dest are presented */
    len = bvlc_sc_encode_address_resolution_ack(buf, sizeof(buf), message_id,
        &origin, &dest, (uint8_t *)web_socket_uris, strlen(web_socket_uris));
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION_ACK,
        message_id, &origin, &dest, true, true, strlen(web_socket_uris));
    zassert_equal(ret, true, NULL);
    res = memcmp(message.hdr.payload, web_socket_uris, strlen(web_socket_uris));
    zassert_equal(res, 0, NULL);
    zassert_not_equal(
        message.payload.address_resolution_ack.utf8_websocket_uri_string, NULL,
        NULL);
    zassert_equal(
        message.payload.address_resolution_ack.utf8_websocket_uri_string_len,
        strlen(web_socket_uris), NULL);
    zassert_equal(message.hdr.payload_len, strlen(web_socket_uris), NULL);
    test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION_ACK, message_id, &origin,
        &dest, true, false, (uint8_t *)web_socket_uris, strlen(web_socket_uris),
        true);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* zero payload test */
    len = bvlc_sc_encode_address_resolution_ack(
        buf, sizeof(buf), message_id, &origin, &dest, NULL, 0);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION_ACK,
        message_id, &origin, &dest, true, true, 0);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(
        message.payload.address_resolution_ack.utf8_websocket_uri_string, NULL,
        NULL);
    zassert_equal(
        message.payload.address_resolution_ack.utf8_websocket_uri_string_len, 0,
        NULL);
    zassert_equal(message.hdr.payload_len, 0, NULL);
    test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION_ACK, message_id, &origin,
        &dest, true, false, NULL, 0, true);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* truncated message, case 1 */

    len = bvlc_sc_encode_address_resolution_ack(buf, sizeof(buf), message_id,
        &origin, &dest, (uint8_t *)web_socket_uris, strlen(web_socket_uris));
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 2 */

    ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* truncated message, case 3 */

    ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 4 */

    ret = bvlc_sc_decode_message(buf, 15, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 5 */

    ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* data options test */
    optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true);
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_INCONSISTENT_PARAMETERS, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_ADVERTISIMENT(void)
{
    uint8_t buf[256];
    int len;
    uint8_t optbuf[256];
    int optlen;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint16_t message_id = 0xe2ad;
    BACNET_SC_VMAC_ADDRESS origin;
    BACNET_SC_VMAC_ADDRESS dest;
    bool ret;
    BACNET_SC_HUB_CONNECTOR_STATE hub_status =
        BACNET_CONNECTED_TO_PRIMARY;
    BVLC_SC_DIRECT_CONNECTION_SUPPORT support =
        BVLC_SC_DIRECT_CONNECTIONS_ACCEPT_SUPPORTED;
    uint16_t max_bvlc_len = 567;
    uint16_t max_npdu_len = 1323;
    const char *err_desc = NULL;

    memset(&origin.address, 0xe1, BVLC_SC_VMAC_SIZE);
    memset(&dest.address, 0x4f, BVLC_SC_VMAC_SIZE);

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* dest and origin absent */
    len = bvlc_sc_encode_advertisiment(buf, sizeof(buf), message_id, NULL, NULL,
        hub_status, support, max_bvlc_len, max_npdu_len);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADVERTISIMENT,
        message_id, NULL, NULL, true, true, 6);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.payload.advertisiment.hub_status, hub_status, NULL);
    zassert_equal(message.payload.advertisiment.support, support, NULL);
    zassert_equal(
        message.payload.advertisiment.max_bvlc_len, max_bvlc_len, NULL);
    zassert_equal(
        message.payload.advertisiment.max_npdu_len, max_npdu_len, NULL);
    test_options(buf, len, BVLC_SC_ADVERTISIMENT, message_id, NULL, NULL, true,
        false, message.hdr.payload, message.hdr.payload_len, false);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* origin is presented, dest is absent */
    len = bvlc_sc_encode_advertisiment(buf, sizeof(buf), message_id, &origin,
        NULL, hub_status, support, max_bvlc_len, max_npdu_len);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADVERTISIMENT,
        message_id, &origin, NULL, true, true, 6);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.payload.advertisiment.hub_status, hub_status, NULL);
    zassert_equal(message.payload.advertisiment.support, support, NULL);
    zassert_equal(
        message.payload.advertisiment.max_bvlc_len, max_bvlc_len, NULL);
    zassert_equal(
        message.payload.advertisiment.max_npdu_len, max_npdu_len, NULL);
    test_options(buf, len, BVLC_SC_ADVERTISIMENT, message_id, &origin, NULL,
        true, false, message.hdr.payload, message.hdr.payload_len, false);

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* origin is absent, dest is presented */
    len = bvlc_sc_encode_advertisiment(buf, sizeof(buf), message_id, NULL,
        &dest, hub_status, support, max_bvlc_len, max_npdu_len);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADVERTISIMENT,
        message_id, NULL, &dest, true, true, 6);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.payload.advertisiment.hub_status, hub_status, NULL);
    zassert_equal(message.payload.advertisiment.support, support, NULL);
    zassert_equal(
        message.payload.advertisiment.max_bvlc_len, max_bvlc_len, NULL);
    zassert_equal(
        message.payload.advertisiment.max_npdu_len, max_npdu_len, NULL);
    test_options(buf, len, BVLC_SC_ADVERTISIMENT, message_id, NULL, &dest, true,
        false, message.hdr.payload, message.hdr.payload_len, false);

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* origin and dest are presented */

    len = bvlc_sc_encode_advertisiment(buf, sizeof(buf), message_id, &origin,
        &dest, hub_status, support, max_bvlc_len, max_npdu_len);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADVERTISIMENT,
        message_id, &origin, &dest, true, true, 6);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.payload.advertisiment.hub_status, hub_status, NULL);
    zassert_equal(message.payload.advertisiment.support, support, NULL);
    zassert_equal(
        message.payload.advertisiment.max_bvlc_len, max_bvlc_len, NULL);
    zassert_equal(
        message.payload.advertisiment.max_npdu_len, max_npdu_len, NULL);
    test_options(buf, len, BVLC_SC_ADVERTISIMENT, message_id, &origin, &dest,
        true, false, message.hdr.payload, message.hdr.payload_len, false);

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* truncated message, case 1 */

    len = bvlc_sc_encode_advertisiment(buf, sizeof(buf), message_id, &origin,
        &dest, hub_status, support, max_bvlc_len, max_npdu_len);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 2 */

    ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* truncated message, case 3 */

    ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 4 */

    ret = bvlc_sc_decode_message(buf, 15, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 5 */

    ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* bad hub connection param */
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    len = bvlc_sc_encode_advertisiment(buf, sizeof(buf), message_id, NULL, NULL,
        5, support, max_bvlc_len, max_npdu_len);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_PARAMETER_OUT_OF_RANGE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* bad support param */
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    len = bvlc_sc_encode_advertisiment(buf, sizeof(buf), message_id, NULL, NULL,
        hub_status, 4, max_bvlc_len, max_npdu_len);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_PARAMETER_OUT_OF_RANGE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* payload len < 6 */
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    len = bvlc_sc_encode_advertisiment(buf, sizeof(buf), message_id, NULL, NULL,
        hub_status, support, max_bvlc_len, max_npdu_len);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* zero payload test  */
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    len = bvlc_sc_encode_advertisiment(buf, sizeof(buf), message_id, NULL, NULL,
        hub_status, support, max_bvlc_len, max_npdu_len);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_PAYLOAD_EXPECTED, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* data options presented */
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    len = bvlc_sc_encode_advertisiment(buf, sizeof(buf), message_id, NULL, NULL,
        hub_status, support, max_bvlc_len, max_npdu_len);
    zassert_not_equal(len, 0, NULL);
    optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true);
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_INCONSISTENT_PARAMETERS, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_ADVERTISIMENT_SOLICITATION(void)
{
    uint8_t buf[256];
    int len;
    uint8_t optbuf[256];
    int optlen;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint16_t message_id = 0xaf4a;
    BACNET_SC_VMAC_ADDRESS origin;
    BACNET_SC_VMAC_ADDRESS dest;
    bool ret;
    const char *err_desc = NULL;

    memset(&origin.address, 0x17, BVLC_SC_VMAC_SIZE);
    memset(&dest.address, 0x1a, BVLC_SC_VMAC_SIZE);

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* dest and origin absent */
    len = bvlc_sc_encode_advertisiment_solicitation(
        buf, sizeof(buf), message_id, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret =
        verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADVERTISIMENT_SOLICITATION,
            message_id, NULL, NULL, true, true, 0);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 0, NULL);
    test_options(buf, len, BVLC_SC_ADVERTISIMENT_SOLICITATION, message_id, NULL,
        NULL, true, false, NULL, 0, false);

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* origin is presented, dest is absent */
    len = bvlc_sc_encode_advertisiment_solicitation(
        buf, sizeof(buf), message_id, &origin, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret =
        verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADVERTISIMENT_SOLICITATION,
            message_id, &origin, NULL, true, true, 0);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 0, NULL);
    test_options(buf, len, BVLC_SC_ADVERTISIMENT_SOLICITATION, message_id,
        &origin, NULL, true, false, NULL, 0, false);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* origin is absent, dest is presented */
    len = bvlc_sc_encode_advertisiment_solicitation(
        buf, sizeof(buf), message_id, NULL, &dest);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret =
        verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADVERTISIMENT_SOLICITATION,
            message_id, NULL, &dest, true, true, 0);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 0, NULL);
    test_options(buf, len, BVLC_SC_ADVERTISIMENT_SOLICITATION, message_id, NULL,
        &dest, true, false, NULL, 0, false);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* origin and dest are presented */
    len = bvlc_sc_encode_advertisiment_solicitation(
        buf, sizeof(buf), message_id, &origin, &dest);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret =
        verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADVERTISIMENT_SOLICITATION,
            message_id, &origin, &dest, true, true, 0);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 0, NULL);
    test_options(buf, len, BVLC_SC_ADVERTISIMENT_SOLICITATION, message_id,
        &origin, &dest, true, false, NULL, 0, false);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* truncated message, case 1 */
    len = bvlc_sc_encode_advertisiment_solicitation(
        buf, sizeof(buf), message_id, &origin, &dest);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 2 */

    ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* truncated message, case 3 */

    ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 5 */

    ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* data options test */
    optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true);
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_INCONSISTENT_PARAMETERS, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_CONNECT_REQUEST(void)
{
    uint8_t buf[256];
    int len;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint16_t message_id = 0x41af;
    int res;
    bool ret;
    uint16_t max_bvlc_len = 9997;
    uint16_t max_npdu_len = 3329;
    BACNET_SC_VMAC_ADDRESS local_vmac;
    BACNET_SC_UUID local_uuid;
    const char *err_desc = NULL;

    memset(&local_vmac, 0x88, sizeof(local_vmac));
    memset(&local_uuid, 0x22, sizeof(local_uuid));

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    len = bvlc_sc_encode_connect_request(buf, sizeof(buf), message_id,
        &local_vmac, &local_uuid, max_bvlc_len, max_npdu_len);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_CONNECT_REQUEST,
        message_id, NULL, NULL, true, true, 26);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload_len, 26, NULL);
    res = memcmp(
        message.payload.connect_request.vmac, &local_vmac, BVLC_SC_VMAC_SIZE);
    zassert_equal(res, 0, NULL);
    res = memcmp(
        message.payload.connect_request.uuid, &local_uuid, BVLC_SC_VMAC_SIZE);
    zassert_equal(res, 0, NULL);
    zassert_equal(
        message.payload.connect_request.max_bvlc_len, max_bvlc_len, NULL);
    zassert_equal(
        message.payload.connect_request.max_npdu_len, max_npdu_len, NULL);
    test_options(buf, len, BVLC_SC_CONNECT_REQUEST, message_id, NULL, NULL,
        true, false, message.hdr.payload, message.hdr.payload_len, false);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* truncated message, case 1 */
    len = bvlc_sc_encode_connect_request(buf, sizeof(buf), message_id,
        &local_vmac, &local_uuid, max_bvlc_len, max_npdu_len);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 2 */

    ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* truncated message, case 3 */

    ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 5 */

    ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_PAYLOAD_EXPECTED, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* message has dest */
    buf[1] |= BVLC_SC_CONTROL_DEST_VADDR;
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* message has origin */
    buf[1] |= BVLC_SC_CONTROL_ORIG_VADDR;
    buf[1] &= ~BVLC_SC_CONTROL_DEST_VADDR;
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* message has both dest and orign */
    buf[1] |= BVLC_SC_CONTROL_ORIG_VADDR;
    buf[1] |= BVLC_SC_CONTROL_DEST_VADDR;
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_CONNECT_ACCEPT(void)
{
    uint8_t buf[256];
    int len;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint16_t message_id = 0x0203;
    int res;
    bool ret;
    uint16_t max_bvlc_len = 1027;
    uint16_t max_npdu_len = 22;
    BACNET_SC_VMAC_ADDRESS local_vmac;
    BACNET_SC_UUID local_uuid;
    const char *err_desc = NULL;

    memset(&local_vmac, 0x33, sizeof(local_vmac));
    memset(&local_uuid, 0x11, sizeof(local_uuid));

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    len = bvlc_sc_encode_connect_accept(buf, sizeof(buf), message_id,
        &local_vmac, &local_uuid, max_bvlc_len, max_npdu_len);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_CONNECT_ACCEPT,
        message_id, NULL, NULL, true, true, 26);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload_len, 26, NULL);
    res = memcmp(
        message.payload.connect_accept.vmac, &local_vmac, BVLC_SC_VMAC_SIZE);
    zassert_equal(res, 0, NULL);
    res = memcmp(
        message.payload.connect_accept.uuid, &local_uuid, BVLC_SC_VMAC_SIZE);
    zassert_equal(res, 0, NULL);
    zassert_equal(
        message.payload.connect_accept.max_bvlc_len, max_bvlc_len, NULL);
    zassert_equal(
        message.payload.connect_accept.max_npdu_len, max_npdu_len, NULL);
    test_options(buf, len, BVLC_SC_CONNECT_ACCEPT, message_id, NULL, NULL, true,
        false, message.hdr.payload, message.hdr.payload_len, false);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* truncated message, case 1 */
    len = bvlc_sc_encode_connect_accept(buf, sizeof(buf), message_id,
        &local_vmac, &local_uuid, max_bvlc_len, max_npdu_len);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 2 */

    ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* truncated message, case 3 */

    ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 5 */

    ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_PAYLOAD_EXPECTED, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* message has dest */
    buf[1] |= BVLC_SC_CONTROL_DEST_VADDR;
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* message has origin */
    buf[1] |= BVLC_SC_CONTROL_ORIG_VADDR;
    buf[1] &= ~BVLC_SC_CONTROL_DEST_VADDR;
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* message has both dest and orign */
    buf[1] |= BVLC_SC_CONTROL_ORIG_VADDR;
    buf[1] |= BVLC_SC_CONTROL_DEST_VADDR;
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_DISCONNECT_REQUEST(void)
{
    uint8_t buf[256];
    int len;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint16_t message_id = 0x0203;
    bool ret;
    const char *err_desc = NULL;

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    len = bvlc_sc_encode_disconnect_request(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_DISCONNECT_REQUEST,
        message_id, NULL, NULL, true, true, 0);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 0, NULL);
    test_options(buf, len, BVLC_SC_DISCONNECT_REQUEST, message_id, NULL, NULL,
        true, false, NULL, 0, false);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* truncated message, case 1 */
    len = bvlc_sc_encode_disconnect_request(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_UNEXPECTED_DATA, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    ret = bvlc_sc_decode_message(buf, 3, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_OTHER, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_DISCONNECT_ACK(void)
{
    uint8_t buf[256];
    int len;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint16_t message_id = 0x0203;
    bool ret;
    const char *err_desc = NULL;

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    len = bvlc_sc_encode_disconnect_ack(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_DISCONNECT_ACK,
        message_id, NULL, NULL, true, true, 0);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 0, NULL);
    test_options(buf, len, BVLC_SC_DISCONNECT_ACK, message_id, NULL, NULL, true,
        false, NULL, 0, false);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* truncated message, case 1 */
    len = bvlc_sc_encode_disconnect_request(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_UNEXPECTED_DATA, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    ret = bvlc_sc_decode_message(buf, 3, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_OTHER, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_HEARTBEAT_REQUEST(void)
{
    uint8_t buf[256];
    int len;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint16_t message_id = 0x0203;
    bool ret;
    const char *err_desc = NULL;

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    len = bvlc_sc_encode_heartbeat_request(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_HEARTBEAT_REQUEST,
        message_id, NULL, NULL, true, true, 0);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 0, NULL);
    test_options(buf, len, BVLC_SC_HEARTBEAT_REQUEST, message_id, NULL, NULL,
        true, false, NULL, 0, false);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* truncated message, case 1 */
    len = bvlc_sc_encode_disconnect_request(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_UNEXPECTED_DATA, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    ret = bvlc_sc_decode_message(buf, 3, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_OTHER, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_HEARTBEAT_ACK(void)
{
    uint8_t buf[256];
    int len;
    uint8_t optbuf[256];
    int optlen;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint16_t message_id = 0x0203;
    bool ret;
    const char *err_desc = NULL;

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    len = bvlc_sc_encode_heartbeat_ack(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_HEARTBEAT_ACK,
        message_id, NULL, NULL, true, true, 0);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 0, NULL);
    test_options(buf, len, BVLC_SC_HEARTBEAT_ACK, message_id, NULL, NULL, true,
        false, NULL, 0, false);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* truncated message, case 1 */
    len = bvlc_sc_encode_heartbeat_ack(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_UNEXPECTED_DATA, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    ret = bvlc_sc_decode_message(buf, 3, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_OTHER, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* data options test */
    optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true);
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_INCONSISTENT_PARAMETERS, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_PROPRIETARY_MESSAGE(void)
{
    uint8_t buf[256];
    int len;
    uint8_t optbuf[256];
    int optlen;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint16_t message_id = 0x4455;
    int res;
    bool ret;
    uint8_t proprietary_function = 0xea;
    BACNET_SC_VMAC_ADDRESS origin;
    BACNET_SC_VMAC_ADDRESS dest;
    uint16_t vendor_id = 0xaabb;
    uint8_t data[34];
    const char *err_desc = NULL;

    memset(data, 0x66, sizeof(data));
    memset(&origin.address, 0x2f, BVLC_SC_VMAC_SIZE);
    memset(&dest.address, 0xf3, BVLC_SC_VMAC_SIZE);

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* dest and origin absent */
    len = bvlc_sc_encode_proprietary_message(buf, sizeof(buf), message_id, NULL,
        NULL, vendor_id, proprietary_function, data, sizeof(data));
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_PROPRIETARY_MESSAGE,
        message_id, NULL, NULL, true, true, 3 + sizeof(data));
    zassert_equal(ret, true, NULL);
    zassert_not_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 3 + sizeof(data), NULL);
    zassert_equal(message.payload.proprietary.vendor_id, vendor_id, NULL);
    zassert_equal(
        message.payload.proprietary.function, proprietary_function, NULL);
    zassert_equal(message.payload.proprietary.data_len, sizeof(data), NULL);

    res = memcmp(message.payload.proprietary.data, data, sizeof(data));
    zassert_equal(res, 0, NULL);
    test_options(buf, len, BVLC_SC_PROPRIETARY_MESSAGE, message_id, NULL, NULL,
        true, false, message.hdr.payload, message.hdr.payload_len, true);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* origin is presented, dest is absent */
    len = bvlc_sc_encode_proprietary_message(buf, sizeof(buf), message_id,
        &origin, NULL, vendor_id, proprietary_function, data, sizeof(data));
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_PROPRIETARY_MESSAGE,
        message_id, &origin, NULL, true, true, 3 + sizeof(data));
    zassert_equal(ret, true, NULL);
    zassert_not_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 3 + sizeof(data), NULL);
    zassert_equal(message.payload.proprietary.vendor_id, vendor_id, NULL);
    zassert_equal(
        message.payload.proprietary.function, proprietary_function, NULL);
    zassert_equal(message.payload.proprietary.data_len, sizeof(data), NULL);

    res = memcmp(message.payload.proprietary.data, data, sizeof(data));
    zassert_equal(res, 0, NULL);
    test_options(buf, len, BVLC_SC_PROPRIETARY_MESSAGE, message_id, &origin,
        NULL, true, false, message.hdr.payload, message.hdr.payload_len, true);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* origin is absent, dest is presented */
    len = bvlc_sc_encode_proprietary_message(buf, sizeof(buf), message_id, NULL,
        &dest, vendor_id, proprietary_function, data, sizeof(data));
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_PROPRIETARY_MESSAGE,
        message_id, NULL, &dest, true, true, 3 + sizeof(data));
    zassert_equal(ret, true, NULL);
    zassert_not_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 3 + sizeof(data), NULL);
    zassert_equal(message.payload.proprietary.vendor_id, vendor_id, NULL);
    zassert_equal(
        message.payload.proprietary.function, proprietary_function, NULL);
    zassert_equal(message.payload.proprietary.data_len, sizeof(data), NULL);

    res = memcmp(message.payload.proprietary.data, data, sizeof(data));
    zassert_equal(res, 0, NULL);
    test_options(buf, len, BVLC_SC_PROPRIETARY_MESSAGE, message_id, NULL, &dest,
        true, false, message.hdr.payload, message.hdr.payload_len, true);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* origin and dest are presented */
    len = bvlc_sc_encode_proprietary_message(buf, sizeof(buf), message_id,
        &origin, &dest, vendor_id, proprietary_function, data, sizeof(data));
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_PROPRIETARY_MESSAGE,
        message_id, &origin, &dest, true, true, 3 + sizeof(data));
    zassert_equal(ret, true, NULL);
    zassert_not_equal(message.hdr.payload, NULL, NULL);
    zassert_equal(message.hdr.payload_len, 3 + sizeof(data), NULL);
    zassert_equal(message.payload.proprietary.vendor_id, vendor_id, NULL);
    zassert_equal(
        message.payload.proprietary.function, proprietary_function, NULL);
    zassert_equal(message.payload.proprietary.data_len, sizeof(data), NULL);

    res = memcmp(message.payload.proprietary.data, data, sizeof(data));
    zassert_equal(res, 0, NULL);
    test_options(buf, len, BVLC_SC_PROPRIETARY_MESSAGE, message_id, &origin,
        &dest, true, false, message.hdr.payload, message.hdr.payload_len, true);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* truncated message, case 1 */
    len = bvlc_sc_encode_proprietary_message(buf, sizeof(buf), message_id,
        &origin, &dest, vendor_id, proprietary_function, data, sizeof(data));
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 2 */

    ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* truncated message, case 3 */

    ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    /* truncated message, case 5 */

    ret = bvlc_sc_decode_message(buf, 18, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* data options test */
    optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true);
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_INCONSISTENT_PARAMETERS, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* zero payload test */
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* truncated message, case 1 */
    len = bvlc_sc_encode_proprietary_message(buf, sizeof(buf), message_id, NULL,
        NULL, vendor_id, proprietary_function, data, sizeof(data));
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_PAYLOAD_EXPECTED, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* zero payload test */
    ret = bvlc_sc_decode_message(buf, 7, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    zassert_equal(message.payload.proprietary.vendor_id, vendor_id, NULL);
    zassert_equal(
        message.payload.proprietary.function, proprietary_function, NULL);
    zassert_equal(message.payload.proprietary.data_len, 0, NULL);
    zassert_equal(message.payload.proprietary.data, NULL, NULL);
}

static void test_BAD_HEADER_OPTIONS(void)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    uint8_t npdu[256];
    uint16_t npdulen;
    int len;
    int optlen;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint16_t message_id = 0x0203;
    bool ret;
    uint16_t vendor_id1;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_data1[17];
    BACNET_SC_VMAC_ADDRESS origin;
    BACNET_SC_VMAC_ADDRESS dest;
    const char *err_desc = NULL;

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* form secure path option with data flag enabled (which is incorrect ) */

    len = bvlc_sc_encode_heartbeat_request(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);

    optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true);
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    buf[4] |= BVLC_SC_HEADER_DATA;
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    /* form unknown header option */

    len = bvlc_sc_encode_heartbeat_request(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);

    optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true);
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    buf[4] |= 2;
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* form unknown header option */

    len = bvlc_sc_encode_heartbeat_request(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);

    optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true);
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    buf[4] |= 2;
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* truncated proprietary option, case 1 */
    len = bvlc_sc_encode_heartbeat_request(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);
    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* truncated proprietary option, case 2 */

    len = bvlc_sc_encode_heartbeat_request(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);
    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);

    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    ret = bvlc_sc_decode_message(buf, 9, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* truncated message with destination options */
    len = bvlc_sc_encode_heartbeat_request(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);
    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* call add option func with bad parameters */
    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    zassert_not_equal(optlen, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), NULL, len, optbuf, optlen);
    zassert_equal(len, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, 3, optbuf, optlen);
    zassert_equal(len, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, 64000, optbuf, 64000);
    zassert_equal(len, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, 100, buf, 120, optbuf, optlen);
    zassert_equal(len, 0, NULL);
    len = bvlc_sc_add_option_to_destination_options(
        buf, 101, buf, 100, optbuf, optlen);
    zassert_equal(len, 0, NULL);
    optbuf[0] |= BVLC_SC_HEADER_MORE;
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, 20, optbuf, optlen);
    zassert_equal(len, 0, NULL);
    optbuf[0] = 23;
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, 20, optbuf, optlen);
    zassert_equal(len, 0, NULL);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    /* truncated message with destination options */
    len = bvlc_sc_encode_heartbeat_request(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);
    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_not_equal(len, 0, NULL);
    buf[4] = 23;
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_equal(len, 0, NULL);
    /* message with incorrect dest options */
    len = bvlc_sc_encode_heartbeat_request(buf, sizeof(buf), message_id);
    zassert_not_equal(len, 0, NULL);
    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));
    len = bvlc_sc_add_option_to_destination_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    buf[4] = 23;
    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, len, optbuf, optlen);
    zassert_equal(len, 0, NULL);

    /* one more incorrect call to add options */
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
    memset(&origin.address, 0x63, BVLC_SC_VMAC_SIZE);
    memset(&dest.address, 0x24, BVLC_SC_VMAC_SIZE);
    memset(npdu, 0x99, sizeof(npdu));
    npdulen = 50;

    len = bvlc_sc_encode_encapsulated_npdu(
        buf, sizeof(buf), message_id, &origin, &dest, npdu, npdulen);

    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        sizeof(proprietary_data1));

    len = bvlc_sc_add_option_to_data_options(
        buf, sizeof(buf), buf, 4, optbuf, optlen);
    zassert_equal(len, 0, NULL);
}

static void test_BAD_ENCODE_PARAMS(void)
{
    uint8_t buf[256];
    uint8_t optbuf[256];
    uint8_t npdu[256];
    uint16_t npdulen;
    int len;
    int optlen;
    BVLC_SC_DECODED_MESSAGE message;
    uint16_t message_id = 0x0203;
    uint16_t vendor_id1;
    uint8_t proprietary_option_type1;
    uint8_t proprietary_data1[17];
    BACNET_SC_VMAC_ADDRESS origin;
    BACNET_SC_VMAC_ADDRESS dest;
    uint8_t error_header_marker = 0xcc;
    uint16_t error_class = 0xaa;
    uint16_t error_code = 0xdd;
    char *error_details_string = "something bad has happend";
    BACNET_SC_VMAC_ADDRESS local_vmac;
    BACNET_SC_UUID local_uuid;
    uint8_t data[34];
    uint8_t proprietary_function = 0xea;

    memset(data, 0x66, sizeof(data));
    memset(&local_uuid, 0x22, sizeof(local_uuid));
    memset(&local_vmac, 0x42, sizeof(local_vmac));
    memset(&origin.address, 0x63, BVLC_SC_VMAC_SIZE);
    memset(&dest.address, 0x24, BVLC_SC_VMAC_SIZE);
    memset(npdu, 0x99, sizeof(npdu));
    npdulen = 50;

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    vendor_id1 = 0xdead;
    proprietary_option_type1 = 0x77;
    memset(proprietary_data1, 0x99, sizeof(proprietary_data1));
    /* case 1 */
    optlen = bvlc_sc_encode_proprietary_option(optbuf, sizeof(optbuf), true,
        vendor_id1, proprietary_option_type1, proprietary_data1,
        BVLC_SC_NPDU_SIZE - 3);
    zassert_equal(optlen, 0, NULL);
    /* case 2 */
    optlen = bvlc_sc_encode_proprietary_option(optbuf, 3, true, vendor_id1,
        proprietary_option_type1, proprietary_data1, sizeof(proprietary_data1));
    zassert_equal(optlen, 0, NULL);
    /* case 3 */
    optlen = bvlc_sc_encode_secure_path_option(optbuf, 0, true);
    zassert_equal(optlen, 0, NULL);
    /* case 4 */
    len = bvlc_sc_encode_heartbeat_request(buf, 3, message_id);
    zassert_equal(len, 0, NULL);
    /* case 5 */
    len = bvlc_sc_encode_heartbeat_request(buf, 3, message_id);
    zassert_equal(len, 0, NULL);
    /* case 6 */
    len = bvlc_sc_encode_encapsulated_npdu(
        buf, 5, message_id, &origin, NULL, npdu, npdulen);
    zassert_equal(len, 0, NULL);
    /* case 7 */
    len = bvlc_sc_encode_encapsulated_npdu(
        buf, 6 + BVLC_SC_VMAC_SIZE, message_id, &origin, &dest, npdu, npdulen);
    zassert_equal(len, 0, NULL);
    /* case 8 */
    len = bvlc_sc_encode_result(buf, sizeof(buf), message_id, &origin, &dest,
        99, 0, NULL, NULL, NULL, NULL);
    zassert_equal(len, 0, NULL);
    /* case 9 */
    len = bvlc_sc_encode_result(buf, sizeof(buf), message_id, &origin, &dest, 1,
        4, NULL, NULL, NULL, NULL);
    zassert_equal(len, 0, NULL);
    /* case 9 */
    len = bvlc_sc_encode_result(buf, sizeof(buf), message_id, &origin, &dest, 1,
        1, NULL, NULL, NULL, NULL);
    zassert_equal(len, 0, NULL);
    /* case 10  */
    len = bvlc_sc_encode_result(
        buf, 3, message_id, &origin, &dest, 1, 0, NULL, NULL, NULL, NULL);
    zassert_equal(len, 0, NULL);
    /* case 11  */
    len = bvlc_sc_encode_result(
        buf, 5, message_id, NULL, NULL, 1, 0, NULL, NULL, NULL, NULL);
    zassert_equal(len, 0, NULL);
    /* case 12  */
    len = bvlc_sc_encode_result(
        buf, 7, message_id, NULL, NULL, 1, 0, (uint8_t *)1, NULL, NULL, NULL);
    zassert_equal(len, 0, NULL);
    /* case 13  */
    len = bvlc_sc_encode_result(buf, 7, message_id, NULL, NULL, 1, 1,
        &error_header_marker, &error_code, &error_class,
        (uint8_t *)error_details_string);
    zassert_equal(len, 0, NULL);
    /* case 13  */
    len = bvlc_sc_encode_result(buf, 12, message_id, NULL, NULL, 1, 1,
        &error_header_marker, &error_code, &error_class,
        (uint8_t *)error_details_string);
    zassert_equal(len, 0, NULL);
    /* case 14  */
    len = bvlc_sc_encode_encapsulated_npdu(
        buf, 3, message_id, NULL, NULL, npdu, npdulen);
    zassert_equal(len, 0, NULL);
    /* case 15  */
    len = bvlc_sc_encode_encapsulated_npdu(
        buf, 6, message_id, NULL, NULL, npdu, npdulen);
    zassert_equal(len, 0, NULL);
    /* case 16  */
    len = bvlc_sc_encode_address_resolution_ack(
        buf, 3, message_id, NULL, NULL, NULL, 0);
    zassert_equal(len, 0, NULL);
    /* case 17 */
    len = bvlc_sc_encode_advertisiment(
        buf, 3, message_id, NULL, NULL, 1, 1, 1, 1);
    zassert_equal(len, 0, NULL);
    /* case 18 */
    len = bvlc_sc_encode_connect_request(
        buf, sizeof(buf), message_id, NULL, NULL, 1, 1);
    zassert_equal(len, 0, NULL);
    /* case 19 */
    len = bvlc_sc_encode_connect_request(
        buf, 3, message_id, &local_vmac, &local_uuid, 1, 1);
    zassert_equal(len, 0, NULL);
    /* case 20 */
    len = bvlc_sc_encode_connect_request(
        buf, 5, message_id, &local_vmac, &local_uuid, 1, 1);
    zassert_equal(len, 0, NULL);
    /* case 21 */
    len = bvlc_sc_encode_connect_accept(
        buf, sizeof(buf), message_id, NULL, NULL, 1, 1);
    zassert_equal(len, 0, NULL);
    /* case 22 */
    len = bvlc_sc_encode_connect_accept(
        buf, 3, message_id, &local_vmac, &local_uuid, 1, 1);
    zassert_equal(len, 0, NULL);
    /* case 23 */
    len = bvlc_sc_encode_connect_accept(
        buf, 5, message_id, &local_vmac, &local_uuid, 1, 1);
    zassert_equal(len, 0, NULL);
    /* case 24 */
    len = bvlc_sc_encode_proprietary_message(buf, 3, message_id, NULL, NULL,
        vendor_id1, proprietary_function, data, sizeof(data));
    zassert_equal(len, 0, NULL);
    /* case 25 */
    len = bvlc_sc_encode_proprietary_message(buf, 5, message_id, NULL, NULL,
        vendor_id1, proprietary_function, data, sizeof(data));
    zassert_equal(len, 0, NULL);
}

static void test_BAD_DECODE_PARAMS(void)
{
    uint8_t buf[256];
    uint8_t npdu[256];
    uint16_t npdulen = 50;
    int len;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    uint16_t message_id = 0x0203;
    bool ret;
    const char *err_desc = NULL;

    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));

    /* case 1 */

    len = bvlc_sc_encode_encapsulated_npdu(
        buf, sizeof(buf), message_id, NULL, NULL, npdu, npdulen);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, NULL, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);

    /* case 2 */
    buf[0] = 99;
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, false, NULL);
    zassert_equal(error, ERROR_CODE_BVLC_FUNCTION_UNKNOWN, NULL);
    zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
    memset(buf, 0, sizeof(buf));
    memset(&message, 0, sizeof(message));
}

static void test_BROADCAST(void)
{
    uint8_t buf[256];
    int len;
    BACNET_SC_VMAC_ADDRESS dest;
    BACNET_SC_VMAC_ADDRESS orig;
    bool ret;
    uint8_t *pdu;
    BVLC_SC_DECODED_MESSAGE message;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    const char *err_desc = NULL;
    BACNET_SC_UUID uuid = {{ 0x34 }};

    memset(&dest.address, 0xFF, sizeof(dest.address));
    memset(&orig.address, 0x12, sizeof(orig.address));
    ret = bvlc_sc_is_vmac_broadcast(&dest);
    zassert_equal(ret, true, NULL);
    ret = bvlc_sc_is_vmac_broadcast(&orig);
    zassert_equal(ret, false, NULL);
    len = bvlc_sc_encode_advertisiment_solicitation(
        buf, sizeof(buf), 0xF00D, &orig, &orig);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_pdu_has_dest_broadcast(buf, len);
    zassert_equal(ret, false, NULL);
    len = bvlc_sc_encode_advertisiment_solicitation(
        buf, sizeof(buf), 0xF00D, &orig, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_pdu_has_dest_broadcast(buf, len);
    zassert_equal(ret, false, NULL);
    len = bvlc_sc_encode_disconnect_ack(buf, sizeof(buf), 0xF00D);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_pdu_has_dest_broadcast(buf, len);
    zassert_equal(ret, false, NULL);
    len = bvlc_sc_encode_advertisiment_solicitation(
        buf, sizeof(buf), 0xF00D, NULL, &orig);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_pdu_has_dest_broadcast(buf, len);
    zassert_equal(ret, false, NULL);
    len = bvlc_sc_encode_advertisiment_solicitation(
        buf, sizeof(buf), 0xF00D, NULL, &dest);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_pdu_has_dest_broadcast(buf, len);
    zassert_equal(ret, true, NULL);
    len = bvlc_sc_encode_advertisiment_solicitation(
        buf, sizeof(buf), 0xF00D, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_pdu_has_dest_broadcast(buf, len);
    zassert_equal(ret, false, NULL);
    pdu = buf;
    len = bvlc_sc_set_orig(&pdu, len, &orig);
    zassert_equal(len, 4, NULL);
    len = bvlc_sc_remove_orig_and_dest(&pdu, len);
    zassert_equal(len, 4, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = bvlc_sc_need_send_bvlc_result(&message);
    zassert_equal(ret, true, NULL);
    len = bvlc_sc_encode_advertisiment_solicitation(
        buf, sizeof(buf), 0xF00D, NULL, &dest);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = bvlc_sc_need_send_bvlc_result(&message);
    zassert_equal(ret, false, NULL);
    len = bvlc_sc_encode_connect_request(
        buf, sizeof(buf), 0xF00D, &orig, &uuid, 1000, 1000);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = bvlc_sc_need_send_bvlc_result(&message);
    zassert_equal(ret, true, NULL);
    len = bvlc_sc_encode_connect_accept(
        buf, sizeof(buf), 0xF00D, &orig, &uuid, 1000, 1000);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = bvlc_sc_need_send_bvlc_result(&message);
    zassert_equal(ret, false, NULL);
    len = bvlc_sc_encode_disconnect_request(buf, sizeof(buf), 0xF00D);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = bvlc_sc_need_send_bvlc_result(&message);
    zassert_equal(ret, true, NULL);
    len = bvlc_sc_encode_encapsulated_npdu(
        buf, sizeof(buf), 0xF00D, NULL, NULL, (uint8_t *)&orig, sizeof(orig));
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = bvlc_sc_need_send_bvlc_result(&message);
    zassert_equal(ret, true, NULL);
    len = bvlc_sc_encode_proprietary_message(
        buf, sizeof(buf), 0xF00D, NULL, NULL, 123, 123, NULL, 0);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = bvlc_sc_need_send_bvlc_result(&message);
    zassert_equal(ret, false, NULL);
    len = bvlc_sc_encode_heartbeat_request(buf, sizeof(buf), 0xF00D);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = bvlc_sc_need_send_bvlc_result(&message);
    zassert_equal(ret, true, NULL);
    len =
        bvlc_sc_encode_address_resolution(buf, sizeof(buf), 0xF00D, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = bvlc_sc_need_send_bvlc_result(&message);
    zassert_equal(ret, true, NULL);
    len = bvlc_sc_encode_advertisiment_solicitation(
        buf, sizeof(buf), 0xF00D, NULL, NULL);
    zassert_not_equal(len, 0, NULL);
    ret = bvlc_sc_decode_message(buf, len, &message, &error, &class, &err_desc);
    zassert_equal(ret, true, NULL);
    ret = bvlc_sc_need_send_bvlc_result(&message);
    zassert_equal(ret, true, NULL);
}

void test_main(void)
{
    ztest_test_suite(bvlc_sc_tests, ztest_unit_test(test_BVLC_RESULT),
        ztest_unit_test(test_ENCAPSULATED_NPDU),
        ztest_unit_test(test_ADDRESS_RESOLUTION),
        ztest_unit_test(test_ADDRESS_RESOLUTION_ACK),
        ztest_unit_test(test_ADVERTISIMENT),
        ztest_unit_test(test_ADVERTISIMENT_SOLICITATION),
        ztest_unit_test(test_CONNECT_REQUEST),
        ztest_unit_test(test_CONNECT_ACCEPT),
        ztest_unit_test(test_DISCONNECT_REQUEST),
        ztest_unit_test(test_DISCONNECT_ACK),
        ztest_unit_test(test_HEARTBEAT_REQUEST),
        ztest_unit_test(test_HEARTBEAT_ACK),
        ztest_unit_test(test_PROPRIETARY_MESSAGE),
        ztest_unit_test(test_BAD_HEADER_OPTIONS),
        ztest_unit_test(test_BAD_ENCODE_PARAMS),
        ztest_unit_test(test_BAD_DECODE_PARAMS),
        ztest_unit_test(test_BROADCAST));

    ztest_run_test_suite(bvlc_sc_tests);
}
