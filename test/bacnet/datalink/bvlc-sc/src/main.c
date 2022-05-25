/*
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <stdlib.h>  /* For calloc() */
#include <ztest.h>
#include <bacnet/datalink/bvlc-sc.h>


static bool verify_bsc_bvll_header(
                   BVLC_SC_DECODED_HDR     *hdr,
                   uint8_t                  bvlc_function,
                   uint16_t                 message_id,
                   BACNET_SC_VMAC_ADDRESS  *origin,
                   BACNET_SC_VMAC_ADDRESS  *dest,
                   bool                     dest_options_absent,
                   bool                     data_options_absent,
                   uint16_t                 payload_len)
{
  if(hdr->bvlc_function != bvlc_function) {
    return false;
  }

  if(hdr->message_id != message_id) {
    return false;
  }

  if(origin) {
    if(memcmp(hdr->origin->address, origin->address, BVLC_SC_VMAC_SIZE) != 0) {
      return false;
    }
  }
  else {
    if(hdr->origin) {
      return false;
    }
  }

  if(dest) {
    if(memcmp(hdr->dest->address, dest->address, BVLC_SC_VMAC_SIZE) != 0) {
      return false;
    }
  }
  else {
    if(hdr->dest) {
      return false;
    }
  }

  if(dest_options_absent) {
    if(hdr->dest_options != NULL) {
      return false;
    }
    if(hdr->dest_options_len != 0) {
      return false;
    }
    if(hdr->dest_options_num != 0) {
      return false;
    }
  }
  else {
    if(!hdr->dest_options){
      return false;
    }
    if(!hdr->dest_options_len){
      return false;
    }
    if(!hdr->dest_options_num){
      return false;
    }
  }

  if(data_options_absent) {
    if(hdr->data_options != NULL) {
      return false;
    }
    if(hdr->data_options_len != 0) {
      return false;
    }
    if(hdr->data_options_num != 0) {
      return false;
    }
  }
  else {
    if(!hdr->data_options){
      return false;
    }
    if(!hdr->data_options_len){
      return false;
    }
    if(!hdr->data_options_num){
      return false;
    }
  }

  if(hdr->payload_len != payload_len) {
    return false;
  }

  return true;
}

static void test_1_option_data(uint8_t                 *pdu, 
                               uint16_t                 pdu_size,
                               uint8_t                  bvlc_function,
                               uint16_t                 message_id,
                               BACNET_SC_VMAC_ADDRESS  *origin,
                               BACNET_SC_VMAC_ADDRESS  *dest,
                               uint8_t                 *payload,
                               uint16_t                 payload_len)
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

  zassert_equal(true, sizeof(buf)>= pdu_size ? true : false, NULL);
  memcpy(buf, pdu, pdu_size);

  optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true );
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_data_options(buf,
                                           sizeof(buf),
                                           buf,
                                           pdu_size,
                                           optbuf,
                                           optlen);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
                               origin, dest, true,
                               false, payload_len);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.data_options_num, 1, NULL);
  zassert_equal(message.data_options[0].type,
                BVLC_SC_OPTION_TYPE_SECURE_PATH, NULL);
  zassert_equal(message.data_options[0].must_understand, true, NULL);
  zassert_equal(message.hdr.payload_len, payload_len, NULL);
  res = memcmp(message.hdr.payload, payload, payload_len);
  zassert_equal(res, 0, NULL);
}

/* 3 options are added to pdu in total: 1 sc, 2 proprietary */

static void test_3_options_different_buffer_data(
                          uint8_t                *pdu,
                          uint16_t                 pdu_size,
                          uint8_t                  bvlc_function,
                          uint16_t                 message_id,
                          BACNET_SC_VMAC_ADDRESS  *origin,
                          BACNET_SC_VMAC_ADDRESS  *dest,
                          uint8_t                 *payload,
                          uint16_t                 payload_len)
{
  uint8_t                 buf[256];
  uint8_t                 buf1[256];
  uint8_t                 optbuf[256];
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE       error;
  BACNET_ERROR_CLASS      class;
  int                     optlen;
  int                     len;
  bool                    ret;
  uint16_t                vendor_id1;
  uint16_t                vendor_id2;
  uint8_t                 proprietary_option_type1;
  uint8_t                 proprietary_option_type2;
  uint8_t                 proprietary_data1[17];
  uint8_t                 proprietary_data2[1];
  int                     res;

  zassert_equal(true, sizeof(buf)>= pdu_size ? true : false, NULL);
  memcpy(buf, pdu, pdu_size);

  optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true );
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_data_options(buf1,
                                           sizeof(buf1),
                                           buf,
                                           pdu_size,
                                           optbuf,
                                           optlen);
  zassert_not_equal(len, 0, NULL);
  vendor_id1 = 0xdead;
  proprietary_option_type1 = 0x77;
  memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id1,
                                             proprietary_option_type1,
                                             proprietary_data1,
                                             sizeof(proprietary_data1));
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_data_options(buf,
                                           sizeof(buf),
                                           buf1,
                                           len,
                                           optbuf,
                                           optlen);
  zassert_not_equal(len, 0, NULL);
  vendor_id2 = 0xbeaf;
  proprietary_option_type2 = 0x33;
  memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id2,
                                             proprietary_option_type2,
                                             proprietary_data2,
                                             sizeof(proprietary_data2));
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_data_options(buf1,
                                           sizeof(buf),
                                           buf,
                                           len,
                                           optbuf,
                                           optlen);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf1, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
                               origin, dest, true, false, payload_len);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.data_options_num, 3, NULL);

  zassert_equal(message.data_options[0].type,
                BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
  zassert_equal(message.data_options[0].must_understand, true, NULL);
  zassert_equal(message.data_options[0].packed_header_marker & 
                BVLC_SC_HEADER_MORE, BVLC_SC_HEADER_MORE, NULL);
  zassert_equal(message.data_options[0].packed_header_marker & 
                BVLC_SC_HEADER_DATA, BVLC_SC_HEADER_DATA, NULL);
  zassert_equal(message.data_options[0].specific.proprietary.vendor_id,
                vendor_id2, NULL);
  zassert_equal(message.data_options[0].specific.proprietary.option_type,
                proprietary_option_type2, NULL);
  zassert_equal(message.data_options[0].specific.proprietary.data_len,
                sizeof(proprietary_data2), NULL);
  res = memcmp(message.data_options[0].specific.proprietary.data,
               proprietary_data2, sizeof(proprietary_data2));
  zassert_equal(res, 0, NULL);
  zassert_equal(message.data_options[1].type,
                BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
  zassert_equal(message.data_options[1].must_understand, true, NULL);
  zassert_equal(message.data_options[1].packed_header_marker & 
                BVLC_SC_HEADER_MORE, BVLC_SC_HEADER_MORE, NULL);
  zassert_equal(message.data_options[1].packed_header_marker & 
                BVLC_SC_HEADER_DATA, BVLC_SC_HEADER_DATA, NULL);
  zassert_equal(message.data_options[2].type,
                BVLC_SC_OPTION_TYPE_SECURE_PATH, NULL);
  zassert_equal(message.data_options[2].must_understand, true, NULL);
  zassert_equal(message.data_options[2].packed_header_marker & 
                BVLC_SC_HEADER_MORE, 0, NULL);
  zassert_equal(message.data_options[2].packed_header_marker & 
                BVLC_SC_HEADER_DATA, 0, NULL);
  zassert_equal(message.hdr.payload_len, payload_len, NULL);
  res = memcmp(message.hdr.payload, payload, payload_len);
  zassert_equal(res, 0, NULL);
}

/* 3 options are added to pdu in total: 1 sc, 2 proprietary */

static void test_3_options_data(uint8_t                  *pdu, 
                                uint16_t                 pdu_size,
                                uint8_t                  bvlc_function,
                                uint16_t                 message_id,
                                BACNET_SC_VMAC_ADDRESS  *origin,
                                BACNET_SC_VMAC_ADDRESS  *dest,
                                uint8_t                 *payload,
                                uint16_t                 payload_len)
{
  uint8_t                 buf[256];
  uint8_t                 optbuf[256];
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE       error;
  BACNET_ERROR_CLASS      class;
  int                     optlen;
  int                     len;
  bool                    ret;
  uint16_t                vendor_id1;
  uint16_t                vendor_id2;
  uint8_t                 proprietary_option_type1;
  uint8_t                 proprietary_option_type2;
  uint8_t                 proprietary_data1[17];
  uint8_t                 proprietary_data2[1];
  int                     res;

  zassert_equal(true, sizeof(buf)>= pdu_size ? true : false, NULL);
  memcpy(buf, pdu, pdu_size);

  optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true );
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_data_options(buf,
                                           sizeof(buf),
                                           buf,
                                           pdu_size,
                                           optbuf,
                                           optlen);
  zassert_not_equal(len, 0, NULL);
  vendor_id1 = 0xdead;
  proprietary_option_type1 = 0x77;
  memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id1,
                                             proprietary_option_type1,
                                             proprietary_data1,
                                             sizeof(proprietary_data1));
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_data_options(buf,
                                           sizeof(buf),
                                           buf,
                                           len,
                                           optbuf,
                                           optlen);
  zassert_not_equal(len, 0, NULL);
  vendor_id2 = 0xbeaf;
  proprietary_option_type2 = 0x33;
  memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id2,
                                             proprietary_option_type2,
                                             proprietary_data2,
                                             sizeof(proprietary_data2));
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_data_options(buf,
                                           sizeof(buf),
                                           buf,
                                           len,
                                           optbuf,
                                           optlen);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
                               origin, dest, true, false, payload_len);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.data_options_num, 3, NULL);
  zassert_equal(message.data_options[0].type,
                BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
  zassert_equal(message.data_options[0].must_understand, true, NULL);
  zassert_equal(message.data_options[0].packed_header_marker & 
                BVLC_SC_HEADER_MORE, BVLC_SC_HEADER_MORE, NULL);
  zassert_equal(message.data_options[0].packed_header_marker & 
                BVLC_SC_HEADER_DATA, BVLC_SC_HEADER_DATA, NULL);
  zassert_equal(message.data_options[0].specific.proprietary.vendor_id,
                vendor_id2, NULL);
  zassert_equal(message.data_options[0].specific.proprietary.option_type,
                proprietary_option_type2, NULL);
  zassert_equal(message.data_options[0].specific.proprietary.data_len,
                sizeof(proprietary_data2), NULL);
  res = memcmp(message.data_options[0].specific.proprietary.data,
               proprietary_data2, sizeof(proprietary_data2));
  zassert_equal(res, 0, NULL);
  zassert_equal(message.data_options[1].type,
                BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
  zassert_equal(message.data_options[1].must_understand, true, NULL);
  zassert_equal(message.data_options[1].packed_header_marker & 
                BVLC_SC_HEADER_MORE, BVLC_SC_HEADER_MORE, NULL);
  zassert_equal(message.data_options[1].packed_header_marker & 
                BVLC_SC_HEADER_DATA, BVLC_SC_HEADER_DATA, NULL);
  zassert_equal(message.data_options[2].type,
                BVLC_SC_OPTION_TYPE_SECURE_PATH, NULL);
  zassert_equal(message.data_options[2].must_understand, true, NULL);
  zassert_equal(message.data_options[2].packed_header_marker & 
                BVLC_SC_HEADER_MORE, 0, NULL);
  zassert_equal(message.data_options[2].packed_header_marker & 
                BVLC_SC_HEADER_DATA, 0, NULL);
  zassert_equal(message.hdr.payload_len, payload_len, NULL);
  res = memcmp(message.hdr.payload, payload, payload_len);
  zassert_equal(res, 0, NULL);
}

static void test_5_options_data(uint8_t                 *pdu, 
                                uint16_t                 pdu_size,
                                uint8_t                  bvlc_function,
                                uint16_t                 message_id,
                                BACNET_SC_VMAC_ADDRESS  *origin,
                                BACNET_SC_VMAC_ADDRESS  *dest,
                                uint8_t                 *payload,
                                uint16_t                 payload_len)
{
  uint8_t                 buf[256];
  uint8_t                 optbuf[256];
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE       error;
  BACNET_ERROR_CLASS      class;
  int                     optlen;
  int                     len;
  bool                    ret;
  int                     res;
  uint16_t                vendor_id1;
  uint16_t                vendor_id2;
  uint8_t                 proprietary_option_type1;
  uint8_t                 proprietary_option_type2;
  uint8_t                 proprietary_data1[17];
  uint8_t                 proprietary_data2[1];
  int                     i;

  zassert_equal(true, sizeof(buf)>= pdu_size ? true : false, NULL);
  memcpy(buf, pdu, pdu_size);

  vendor_id1 = 0xdead;
  proprietary_option_type1 = 0x77;
  memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id1,
                                             proprietary_option_type1,
                                             proprietary_data1,
                                             sizeof(proprietary_data1));
  zassert_not_equal(optlen, 0, NULL);
  len = pdu_size;
  for(i = 0; i < 5; i ++) {
    len = bvlc_sc_add_option_to_data_options(buf,
                                             sizeof(buf),
                                             buf,
                                             len,
                                             optbuf,
                                             optlen);
    zassert_not_equal(len, 0, NULL);
  }
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_OUT_OF_MEMORY, NULL);
  zassert_equal(class, ERROR_CLASS_RESOURCES, NULL);
}

/* check message decoding when header option has incorrect more bit flag */

static void test_options_incorrect_more_bit_data(
                           uint8_t                 *pdu, 
                           uint16_t                 pdu_size,
                           uint8_t                  bvlc_function,
                           uint16_t                 message_id,
                           BACNET_SC_VMAC_ADDRESS  *origin,
                           BACNET_SC_VMAC_ADDRESS  *dest,
                           uint8_t                 *payload,
                           uint16_t                 payload_len)
{
  uint8_t                 buf[256];
  uint8_t                 optbuf[256];
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE       error;
  BACNET_ERROR_CLASS      class;
  int                     optlen;
  int                     len;
  bool                    ret;
  int                     res;
  uint16_t                vendor_id1;
  uint16_t                vendor_id2;
  uint8_t                 proprietary_option_type1;
  uint8_t                 proprietary_option_type2;
  uint8_t                 proprietary_data1[17];
  uint8_t                 proprietary_data2[5];
  int                     offs = 4;

  zassert_equal(true, sizeof(buf)>= pdu_size ? true : false, NULL);
  memcpy(buf, pdu, pdu_size);

  vendor_id1 = 0xdead;
  proprietary_option_type1 = 0x77;
  memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id1,
                                             proprietary_option_type1,
                                             proprietary_data1,
                                             sizeof(proprietary_data1));
  zassert_not_equal(optlen, 0, NULL);
  vendor_id2 = 0xbeaf;
  proprietary_option_type2 = 0x33;
  memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id2,
                                             proprietary_option_type2,
                                             proprietary_data2,
                                             sizeof(proprietary_data2));
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_data_options(buf,
                                           sizeof(buf),
                                           buf,
                                           len,
                                           optbuf,
                                           optlen);
  zassert_not_equal(len, 0, NULL);
  if(buf[1] & BVLC_SC_CONTROL_ORIG_VADDR) {
    offs += BVLC_SC_VMAC_SIZE;
  }

  if(buf[1] & BVLC_SC_CONTROL_DEST_VADDR) {
    offs += BVLC_SC_VMAC_SIZE;
  }

  buf[offs] &= ~(BVLC_SC_HEADER_MORE);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_UNEXPECTED_DATA, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}


/* check message decoding when header option has incorrect more bit flag */

static void test_options_incorrect_data_bit_data(
                           uint8_t                 *pdu, 
                           uint16_t                 pdu_size,
                           uint8_t                  bvlc_function,
                           uint16_t                 message_id,
                           BACNET_SC_VMAC_ADDRESS  *origin,
                           BACNET_SC_VMAC_ADDRESS  *dest,
                           uint8_t                 *payload,
                           uint16_t                 payload_len)
{
  uint8_t                 buf[256];
  uint8_t                 optbuf[256];
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE       error;
  BACNET_ERROR_CLASS      class;
  int                     optlen;
  int                     len = pdu_size;
  bool                    ret;
  int                     res;
  uint16_t                vendor_id1;
  uint8_t                 proprietary_option_type1;
  uint8_t                 proprietary_data1[17];
  int                     offs = 4;

  zassert_equal(true, sizeof(buf)>= pdu_size ? true : false, NULL);
  memcpy(buf, pdu, pdu_size);

  vendor_id1 = 0xdead;
  proprietary_option_type1 = 0x77;
  memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id1,
                                             proprietary_option_type1,
                                             proprietary_data1,
                                             sizeof(proprietary_data1));
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_data_options(buf,
                                           sizeof(buf),
                                           buf,
                                           len,
                                           optbuf,
                                           optlen);
  zassert_not_equal(len, 0, NULL);
  if(buf[1] & BVLC_SC_CONTROL_ORIG_VADDR) {
    offs += BVLC_SC_VMAC_SIZE;
  }

  if(buf[1] & BVLC_SC_CONTROL_DEST_VADDR) {
    offs += BVLC_SC_VMAC_SIZE;
  }

  buf[offs] &= ~(BVLC_SC_HEADER_DATA);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

/* secure path option must not be in dest option headers. this test
   checks this case */

static void test_1_option_dest_incorrect(uint8_t                 *pdu, 
                                         uint16_t                 pdu_size,
                                         uint8_t                  bvlc_function,
                                         uint16_t                 message_id,
                                         BACNET_SC_VMAC_ADDRESS  *origin,
                                         BACNET_SC_VMAC_ADDRESS  *dest,
                                         uint8_t                 *payload,
                                         uint16_t                 payload_len)
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

  zassert_equal(true, sizeof(buf)>= pdu_size ? true : false, NULL);
  memcpy(buf, pdu, pdu_size);

  optlen = bvlc_sc_encode_secure_path_option(optbuf, sizeof(optbuf), true );
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_destination_options(buf,
                                                  sizeof(buf),
                                                  buf,
                                                  pdu_size,
                                                  optbuf,
                                                  optlen);
  zassert_equal(len, 0, NULL);
  len = bvlc_sc_add_option_to_data_options(buf,
                                           sizeof(buf),
                                           buf,
                                           pdu_size,
                                           optbuf,
                                           optlen);
  zassert_not_equal(len, 0, NULL);
  buf[1] &= ~(BVLC_SC_CONTROL_DATA_OPTIONS);
  buf[1] |= BVLC_SC_CONTROL_DEST_OPTIONS;
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_1_option_dest(uint8_t                 *pdu, 
                               uint16_t                 pdu_size,
                               uint8_t                  bvlc_function,
                               uint16_t                 message_id,
                               BACNET_SC_VMAC_ADDRESS  *origin,
                               BACNET_SC_VMAC_ADDRESS  *dest,
                               uint8_t                 *payload,
                               uint16_t                 payload_len)
{
  uint8_t                 buf[256];
  uint8_t                 optbuf[256];
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE       error;
  BACNET_ERROR_CLASS      class;
  int                     optlen;
  int                     len;
  bool                    ret;
  int                     res;
  uint16_t                vendor_id1;
  uint8_t                 proprietary_option_type1;
  uint8_t                 proprietary_data1[17];

  zassert_equal(true, sizeof(buf)>= pdu_size ? true : false, NULL);
  memcpy(buf, pdu, pdu_size);
  vendor_id1 = 0xdead;
  proprietary_option_type1 = 0x77;
  memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id1,
                                             proprietary_option_type1,
                                             proprietary_data1,
                                             sizeof(proprietary_data1));


  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_destination_options(buf,
                                                  sizeof(buf),
                                                  buf,
                                                  pdu_size,
                                                  optbuf,
                                                  optlen);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
                               origin, dest, false,
                               true, payload_len);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.dest_options_num, 1, NULL);
  zassert_equal(message.dest_options[0].type,
                BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
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
}

/* 3 proprietary options are added to pdu */

static void test_3_options_dest(uint8_t                  *pdu, 
                                uint16_t                 pdu_size,
                                uint8_t                  bvlc_function,
                                uint16_t                 message_id,
                                BACNET_SC_VMAC_ADDRESS  *origin,
                                BACNET_SC_VMAC_ADDRESS  *dest,
                                uint8_t                 *payload,
                                uint16_t                 payload_len)
{
  uint8_t                 buf[256];
  uint8_t                 optbuf[256];
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE       error;
  BACNET_ERROR_CLASS      class;
  int                     optlen;
  int                     len = pdu_size;
  bool                    ret;
  uint16_t                vendor_id1;
  uint16_t                vendor_id2;
  uint16_t                vendor_id3;
  uint8_t                 proprietary_option_type1;
  uint8_t                 proprietary_option_type2;
  uint8_t                 proprietary_option_type3;
  uint8_t                 proprietary_data1[17];
  uint8_t                 proprietary_data2[1];
  uint8_t                 proprietary_data3[43];
  int                     res;

  zassert_equal(true, sizeof(buf)>= pdu_size ? true : false, NULL);
  memcpy(buf, pdu, pdu_size);

  vendor_id1 = 0xdead;
  proprietary_option_type1 = 0x77;
  memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id1,
                                             proprietary_option_type1,
                                             proprietary_data1,
                                             sizeof(proprietary_data1));
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_destination_options(buf,
                                                  sizeof(buf),
                                                  buf,
                                                  len,
                                                  optbuf,
                                                  optlen);
  zassert_not_equal(len, 0, NULL);
  vendor_id2 = 0xbeaf;
  proprietary_option_type2 = 0x33;
  memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id2,
                                             proprietary_option_type2,
                                             proprietary_data2,
                                             sizeof(proprietary_data2));
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_destination_options(buf,
                                                  sizeof(buf),
                                                  buf,
                                                  len,
                                                  optbuf,
                                                  optlen);
  zassert_not_equal(len, 0, NULL);

  vendor_id3 = 0xF00D;
  proprietary_option_type3 = 0x08;
  memset(proprietary_data3, 0x55, sizeof(proprietary_data3));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id3,
                                             proprietary_option_type3,
                                             proprietary_data3,
                                             sizeof(proprietary_data3));
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_destination_options(buf,
                                                  sizeof(buf),
                                                  buf,
                                                  len,
                                                  optbuf,
                                                  optlen);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
                               origin, dest, false, true, payload_len);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.dest_options_num, 3, NULL);
  zassert_equal(message.dest_options[0].type,
                BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
  zassert_equal(message.dest_options[0].must_understand, true, NULL);
  zassert_equal(message.dest_options[0].packed_header_marker & 
                BVLC_SC_HEADER_MORE, BVLC_SC_HEADER_MORE, NULL);
  zassert_equal(message.dest_options[0].packed_header_marker & 
                BVLC_SC_HEADER_DATA, BVLC_SC_HEADER_DATA, NULL);
  zassert_equal(message.dest_options[0].specific.proprietary.vendor_id,
                vendor_id3, NULL);
  zassert_equal(message.dest_options[0].specific.proprietary.option_type,
                proprietary_option_type3, NULL);
  zassert_equal(message.dest_options[0].specific.proprietary.data_len,
                sizeof(proprietary_data3), NULL);
  res = memcmp(message.dest_options[0].specific.proprietary.data,
               proprietary_data3, sizeof(proprietary_data3));
  zassert_equal(res, 0, NULL);
  zassert_equal(message.dest_options[1].type,
                BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
  zassert_equal(message.dest_options[1].must_understand, true, NULL);
  zassert_equal(message.dest_options[1].packed_header_marker & 
                BVLC_SC_HEADER_MORE, BVLC_SC_HEADER_MORE, NULL);
  zassert_equal(message.dest_options[1].packed_header_marker & 
                BVLC_SC_HEADER_DATA, BVLC_SC_HEADER_DATA, NULL);
  zassert_equal(message.dest_options[1].specific.proprietary.vendor_id,
                vendor_id2, NULL);
  zassert_equal(message.dest_options[1].specific.proprietary.option_type,
                proprietary_option_type2, NULL);
  zassert_equal(message.dest_options[1].specific.proprietary.data_len,
                sizeof(proprietary_data2), NULL);
  res = memcmp(message.dest_options[1].specific.proprietary.data,
               proprietary_data2, sizeof(proprietary_data2));
  zassert_equal(res, 0, NULL);
  zassert_equal(message.dest_options[2].type,
                BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
  zassert_equal(message.dest_options[2].must_understand, true, NULL);
  zassert_equal(message.dest_options[2].packed_header_marker & 
                BVLC_SC_HEADER_MORE, 0, NULL);
  zassert_equal(message.dest_options[2].packed_header_marker & 
                BVLC_SC_HEADER_DATA, BVLC_SC_HEADER_DATA, NULL);
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
}

static void test_3_options_dest_different_buffer(
               uint8_t                  *pdu, 
               uint16_t                 pdu_size,
               uint8_t                  bvlc_function,
               uint16_t                 message_id,
               BACNET_SC_VMAC_ADDRESS  *origin,
               BACNET_SC_VMAC_ADDRESS  *dest,
               uint8_t                 *payload,
               uint16_t                 payload_len)
{
  uint8_t                 buf1[256];
  uint8_t                 buf[256];
  uint8_t                 optbuf[256];
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE       error;
  BACNET_ERROR_CLASS      class;
  int                     optlen;
  int                     len = pdu_size;
  bool                    ret;
  uint16_t                vendor_id1;
  uint16_t                vendor_id2;
  uint16_t                vendor_id3;
  uint8_t                 proprietary_option_type1;
  uint8_t                 proprietary_option_type2;
  uint8_t                 proprietary_option_type3;
  uint8_t                 proprietary_data1[17];
  uint8_t                 proprietary_data2[1];
  uint8_t                 proprietary_data3[43];
  int                     res;

  zassert_equal(true, sizeof(buf)>= pdu_size ? true : false, NULL);
  memcpy(buf, pdu, pdu_size);

  vendor_id1 = 0xdead;
  proprietary_option_type1 = 0x77;
  memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id1,
                                             proprietary_option_type1,
                                             proprietary_data1,
                                             sizeof(proprietary_data1));
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_destination_options(buf1,
                                                  sizeof(buf1),
                                                  buf,
                                                  len,
                                                  optbuf,
                                                  optlen);
  zassert_not_equal(len, 0, NULL);
  vendor_id2 = 0xbeaf;
  proprietary_option_type2 = 0x33;
  memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id2,
                                             proprietary_option_type2,
                                             proprietary_data2,
                                             sizeof(proprietary_data2));
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_destination_options(buf,
                                                  sizeof(buf),
                                                  buf1,
                                                  len,
                                                  optbuf,
                                                  optlen);
  zassert_not_equal(len, 0, NULL);

  vendor_id3 = 0xF00D;
  proprietary_option_type3 = 0x08;
  memset(proprietary_data3, 0x55, sizeof(proprietary_data3));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id3,
                                             proprietary_option_type3,
                                             proprietary_data3,
                                             sizeof(proprietary_data3));
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_destination_options(buf1,
                                                  sizeof(buf1),
                                                  buf,
                                                  len,
                                                  optbuf,
                                                  optlen);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf1, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
                               origin, dest, false, true, payload_len);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.dest_options_num, 3, NULL);
  zassert_equal(message.dest_options[0].type,
                BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
  zassert_equal(message.dest_options[0].must_understand, true, NULL);
  zassert_equal(message.dest_options[0].packed_header_marker & 
                BVLC_SC_HEADER_MORE, BVLC_SC_HEADER_MORE, NULL);
  zassert_equal(message.dest_options[0].packed_header_marker & 
                BVLC_SC_HEADER_DATA, BVLC_SC_HEADER_DATA, NULL);
  zassert_equal(message.dest_options[0].specific.proprietary.vendor_id,
                vendor_id3, NULL);
  zassert_equal(message.dest_options[0].specific.proprietary.option_type,
                proprietary_option_type3, NULL);
  zassert_equal(message.dest_options[0].specific.proprietary.data_len,
                sizeof(proprietary_data3), NULL);
  res = memcmp(message.dest_options[0].specific.proprietary.data,
               proprietary_data3, sizeof(proprietary_data3));
  zassert_equal(res, 0, NULL);
  zassert_equal(message.dest_options[1].type,
                BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
  zassert_equal(message.dest_options[1].must_understand, true, NULL);
  zassert_equal(message.dest_options[1].packed_header_marker & 
                BVLC_SC_HEADER_MORE, BVLC_SC_HEADER_MORE, NULL);
  zassert_equal(message.dest_options[1].packed_header_marker & 
                BVLC_SC_HEADER_DATA, BVLC_SC_HEADER_DATA, NULL);
  zassert_equal(message.dest_options[1].specific.proprietary.vendor_id,
                vendor_id2, NULL);
  zassert_equal(message.dest_options[1].specific.proprietary.option_type,
                proprietary_option_type2, NULL);
  zassert_equal(message.dest_options[1].specific.proprietary.data_len,
                sizeof(proprietary_data2), NULL);
  res = memcmp(message.dest_options[1].specific.proprietary.data,
               proprietary_data2, sizeof(proprietary_data2));
  zassert_equal(res, 0, NULL);
  zassert_equal(message.dest_options[2].type,
                BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
  zassert_equal(message.dest_options[2].must_understand, true, NULL);
  zassert_equal(message.dest_options[2].packed_header_marker & 
                BVLC_SC_HEADER_MORE, 0, NULL);
  zassert_equal(message.dest_options[2].packed_header_marker & 
                BVLC_SC_HEADER_DATA, BVLC_SC_HEADER_DATA, NULL);
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
}

static void test_5_options_dest(uint8_t                 *pdu, 
                                uint16_t                 pdu_size,
                                uint8_t                  bvlc_function,
                                uint16_t                 message_id,
                                BACNET_SC_VMAC_ADDRESS  *origin,
                                BACNET_SC_VMAC_ADDRESS  *dest,
                                uint8_t                 *payload,
                                uint16_t                 payload_len)
{
  uint8_t                 buf[256];
  uint8_t                 optbuf[256];
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE       error;
  BACNET_ERROR_CLASS      class;
  int                     optlen;
  int                     len;
  bool                    ret;
  int                     res;
  uint16_t                vendor_id1;
  uint16_t                vendor_id2;
  uint8_t                 proprietary_option_type1;
  uint8_t                 proprietary_option_type2;
  uint8_t                 proprietary_data1[17];
  uint8_t                 proprietary_data2[1];
  int                     i;

  zassert_equal(true, sizeof(buf)>= pdu_size ? true : false, NULL);
  memcpy(buf, pdu, pdu_size);

  vendor_id1 = 0xdead;
  proprietary_option_type1 = 0x77;
  memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id1,
                                             proprietary_option_type1,
                                             proprietary_data1,
                                             sizeof(proprietary_data1));
  zassert_not_equal(optlen, 0, NULL);
  len = pdu_size;
  for(i = 0; i < 5; i ++) {
    len = bvlc_sc_add_option_to_destination_options(buf,
                                                    sizeof(buf),
                                                    buf,
                                                    len,
                                                    optbuf,
                                                    optlen);
    zassert_not_equal(len, 0, NULL);
  }
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_OUT_OF_MEMORY, NULL);
  zassert_equal(class, ERROR_CLASS_RESOURCES, NULL);
}

static void test_options_incorrect_data_bit_dest(
                           uint8_t                 *pdu, 
                           uint16_t                 pdu_size,
                           uint8_t                  bvlc_function,
                           uint16_t                 message_id,
                           BACNET_SC_VMAC_ADDRESS  *origin,
                           BACNET_SC_VMAC_ADDRESS  *dest,
                           uint8_t                 *payload,
                           uint16_t                 payload_len)
{
  uint8_t                 buf[256];
  uint8_t                 optbuf[256];
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE       error;
  BACNET_ERROR_CLASS      class;
  int                     optlen;
  int                     len = pdu_size;
  bool                    ret;
  int                     res;
  uint16_t                vendor_id1;
  uint8_t                 proprietary_option_type1;
  uint8_t                 proprietary_data1[17];
  int                     offs = 4;

  zassert_equal(true, sizeof(buf)>= pdu_size ? true : false, NULL);
  memcpy(buf, pdu, pdu_size);

  vendor_id1 = 0xdead;
  proprietary_option_type1 = 0x77;
  memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id1,
                                             proprietary_option_type1,
                                             proprietary_data1,
                                             sizeof(proprietary_data1));
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_destination_options(buf,
                                                  sizeof(buf),
                                                  buf,
                                                  len,
                                                  optbuf,
                                                  optlen);
  zassert_not_equal(len, 0, NULL);
  if(buf[1] & BVLC_SC_CONTROL_ORIG_VADDR) {
    offs += BVLC_SC_VMAC_SIZE;
  }

  if(buf[1] & BVLC_SC_CONTROL_DEST_VADDR) {
    offs += BVLC_SC_VMAC_SIZE;
  }

  buf[offs] &= ~(BVLC_SC_HEADER_DATA);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_HEADER_ENCODING_ERROR, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_options_incorrect_more_bit_dest(
                           uint8_t                 *pdu, 
                           uint16_t                 pdu_size,
                           uint8_t                  bvlc_function,
                           uint16_t                 message_id,
                           BACNET_SC_VMAC_ADDRESS  *origin,
                           BACNET_SC_VMAC_ADDRESS  *dest,
                           uint8_t                 *payload,
                           uint16_t                 payload_len)
{
  uint8_t                 buf[256];
  uint8_t                 optbuf[256];
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE       error;
  BACNET_ERROR_CLASS      class;
  int                     optlen;
  int                     len;
  bool                    ret;
  int                     res;
  uint16_t                vendor_id1;
  uint16_t                vendor_id2;
  uint8_t                 proprietary_option_type1;
  uint8_t                 proprietary_option_type2;
  uint8_t                 proprietary_data1[17];
  uint8_t                 proprietary_data2[5];
  int                     offs = 4;

  zassert_equal(true, sizeof(buf)>= pdu_size ? true : false, NULL);
  memcpy(buf, pdu, pdu_size);

  vendor_id1 = 0xdead;
  proprietary_option_type1 = 0x77;
  memset(proprietary_data1, 0x99, sizeof(proprietary_data1));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id1,
                                             proprietary_option_type1,
                                             proprietary_data1,
                                             sizeof(proprietary_data1));
  zassert_not_equal(optlen, 0, NULL);
  vendor_id2 = 0xbeaf;
  proprietary_option_type2 = 0x33;
  memset(proprietary_data2, 0x11, sizeof(proprietary_data2));

  optlen = bvlc_sc_encode_proprietary_option(optbuf,
                                             sizeof(optbuf),
                                             true,
                                             vendor_id2,
                                             proprietary_option_type2,
                                             proprietary_data2,
                                             sizeof(proprietary_data2));
  zassert_not_equal(optlen, 0, NULL);
  len = bvlc_sc_add_option_to_destination_options(buf,
                                                  sizeof(buf),
                                                  buf,
                                                  len,
                                                  optbuf,
                                                  optlen);
  zassert_not_equal(len, 0, NULL);
  if(buf[1] & BVLC_SC_CONTROL_ORIG_VADDR) {
    offs += BVLC_SC_VMAC_SIZE;
  }

  if(buf[1] & BVLC_SC_CONTROL_DEST_VADDR) {
    offs += BVLC_SC_VMAC_SIZE;
  }

  buf[offs] &= ~(BVLC_SC_HEADER_MORE);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_UNEXPECTED_DATA, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_options(uint8_t                 *pdu, 
                         uint16_t                 pdu_size,
                         uint8_t                  bvlc_function,
                         uint16_t                 message_id,
                         BACNET_SC_VMAC_ADDRESS  *origin,
                         BACNET_SC_VMAC_ADDRESS  *dest,
                         bool                     test_dest_option,
                         bool                     test_data_option,
                         uint8_t                 *payload,
                         uint16_t                 payload_len,
                         bool                     ignore_more_bit_test)
{
  if(!test_dest_option && test_data_option) {
    test_1_option_data(pdu, pdu_size, bvlc_function, message_id, origin,
                   dest, payload, payload_len);
    test_3_options_data(pdu, pdu_size, bvlc_function, message_id, origin,
                   dest, payload, payload_len);
    test_3_options_different_buffer_data(pdu, pdu_size, bvlc_function,
                   message_id, origin, dest, payload, payload_len);
    test_5_options_data(pdu, pdu_size, bvlc_function, message_id, origin,
                   dest, payload, payload_len);
    if(!ignore_more_bit_test) {
      test_options_incorrect_more_bit_data(pdu, pdu_size, bvlc_function,
                     message_id, origin, dest, payload, payload_len);
    }
    test_options_incorrect_data_bit_data(pdu, pdu_size, bvlc_function,
                   message_id, origin, dest, payload, payload_len);
  }
  else if (test_dest_option && !test_data_option) {
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
    if(!ignore_more_bit_test) {
      test_options_incorrect_more_bit_dest(pdu, pdu_size, bvlc_function,
                     message_id, origin, dest, payload, payload_len);
     }
    test_options_incorrect_data_bit_dest(pdu, pdu_size, bvlc_function,
                   message_id, origin, dest, payload, payload_len);
  }
}

static void test_BVLC_RESULT(void)
{
  uint8_t buf[256];
  int len;
  BACNET_SC_VMAC_ADDRESS origin;
  BACNET_SC_VMAC_ADDRESS dest;
  uint16_t message_id = 0x7777;
  uint8_t result_bvlc_function = 3;
  bool ret;
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE error;
  BACNET_ERROR_CLASS class;
  uint8_t error_header_marker = 0xcc;
  uint16_t error_class = 0xaa;
  uint16_t error_code = 0xdd;
  char* error_details_string = "something bad has happend";

  memset(origin.address, 0x23, BVLC_SC_VMAC_SIZE);
  memset(dest.address, 0x44, BVLC_SC_VMAC_SIZE);

  /* origin and dest presented */
  len = bvlc_sc_encode_result(buf, sizeof(buf), message_id,
                              &origin, &dest, result_bvlc_function,
                              0, NULL, NULL, NULL, NULL );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id,
                               &origin, &dest, true, true, 2);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.payload.result.bvlc_function,
                result_bvlc_function,NULL);
  zassert_equal(message.payload.result.result, 0, NULL);
  test_options(buf, len, BVLC_SC_RESULT, message_id,
               &origin, &dest, true, false,
               message.hdr.payload, message.hdr.payload_len, false);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* origin presented */
  len = bvlc_sc_encode_result(buf, sizeof(buf), message_id,
                              &origin, NULL, result_bvlc_function,
                              0,NULL, NULL, NULL, NULL );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id,
                               &origin, NULL, true, true, 2);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.payload.result.bvlc_function,
               result_bvlc_function, NULL);
  zassert_equal(message.payload.result.result, 0, NULL);
  test_options(buf, len, BVLC_SC_RESULT, message_id,
               &origin, NULL, true, false,
               message.hdr.payload, message.hdr.payload_len, false);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* dest presented */
  len = bvlc_sc_encode_result(buf, sizeof(buf), message_id,
                              NULL, &dest, result_bvlc_function,
                              0, NULL, NULL, NULL, NULL );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id,
                               NULL, &dest, true, true, 2);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.payload.result.bvlc_function,
                result_bvlc_function, NULL);
  zassert_equal(message.payload.result.result, 0, NULL);
  test_options(buf, len, BVLC_SC_RESULT, message_id,
               NULL, &dest, true, false,
               message.hdr.payload, message.hdr.payload_len, false);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* dest and origin absent */
  len = bvlc_sc_encode_result(buf, sizeof(buf), message_id,
                              NULL, NULL, result_bvlc_function,
                              0, NULL, NULL, NULL, NULL );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id,
                               NULL, NULL, true, true, 2);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.payload.result.bvlc_function,
                result_bvlc_function, NULL);
  zassert_equal(message.payload.result.result, 0, NULL);
  test_options(buf, len, BVLC_SC_RESULT, message_id,
               NULL, NULL, true, false,
               message.hdr.payload, message.hdr.payload_len, false);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* nak, no details string */
  len = bvlc_sc_encode_result(buf, sizeof(buf), message_id,
                               NULL, NULL, result_bvlc_function, 1,
                               &error_header_marker, &error_class,
                               &error_code, NULL );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id,
                               NULL, NULL, true, true, 7);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.payload.result.bvlc_function,
                result_bvlc_function, NULL);
  zassert_equal(message.payload.result.result, 1, NULL);
  zassert_equal(message.payload.result.error_header_marker,
                error_header_marker, NULL);
  zassert_equal(message.payload.result.error_class, error_class, NULL);
  zassert_equal(message.payload.result.error_code, error_code, NULL);
  zassert_equal(message.payload.result.utf8_details_string, NULL, NULL);
  zassert_equal(message.payload.result.utf8_details_string_len, 0, NULL);
  test_options(buf, len, BVLC_SC_RESULT, message_id,
               NULL, NULL, true, false,
               message.hdr.payload, message.hdr.payload_len, false);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* nak , details string */
  len = bvlc_sc_encode_result(buf, sizeof(buf), message_id,
                               NULL, NULL, result_bvlc_function, 1,
                               &error_header_marker, &error_class,
                               &error_code, (uint8_t*) error_details_string );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id,
                               NULL, NULL, true, true,
                               7 + strlen(error_details_string));
  zassert_equal(ret, true, NULL);
  zassert_equal(message.payload.result.bvlc_function,
                result_bvlc_function, NULL);
  zassert_equal(message.payload.result.result, 1, NULL);
  zassert_equal(message.payload.result.error_header_marker,
                error_header_marker, NULL);
  zassert_equal(message.payload.result.error_class, error_class, NULL);
  zassert_equal(message.payload.result.error_code, error_code, NULL);
  zassert_equal(message.payload.result.utf8_details_string_len,
                strlen(error_details_string), NULL);
  ret = memcmp(message.payload.result.utf8_details_string,
               error_details_string, strlen(error_details_string)) == 0 ?
               true : false;
  zassert_equal(ret, true, NULL);
  test_options(buf, len, BVLC_SC_RESULT, message_id,
               NULL, NULL, true, false,
               message.hdr.payload, message.hdr.payload_len, false);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* dest and origin, nak , details string */
  len = bvlc_sc_encode_result(buf, sizeof(buf), message_id,
                               &origin, &dest, result_bvlc_function, 1,
                               &error_header_marker, &error_class,
                               &error_code, (uint8_t*) error_details_string );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id,
                               &origin, &dest, true, true,
                               7 + strlen(error_details_string));
  zassert_equal(ret, true, NULL);
  zassert_equal(message.payload.result.bvlc_function,
                result_bvlc_function, NULL);
  zassert_equal(message.payload.result.result, 1, NULL);
  zassert_equal(message.payload.result.error_header_marker,
                error_header_marker, NULL);
  zassert_equal(message.payload.result.error_class, error_class, NULL);
  zassert_equal(message.payload.result.error_code, error_code, NULL);
  zassert_equal(message.payload.result.utf8_details_string_len,
                strlen(error_details_string), NULL);
  ret = memcmp(message.payload.result.utf8_details_string,
               error_details_string, strlen(error_details_string)) == 0 ?
               true : false;
  zassert_equal(ret, true, NULL);
  test_options(buf, len, BVLC_SC_RESULT, message_id,
               &origin, &dest, true, false,
               message.hdr.payload, message.hdr.payload_len, false);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* truncated message, case 1 */
  len = bvlc_sc_encode_result(buf, sizeof(buf), message_id,
                              NULL, NULL, result_bvlc_function, 1,
                              &error_header_marker, &error_class,
                              &error_code, (uint8_t*) error_details_string);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 2 */

  ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* origin and dest absent, result ok*/
  len = bvlc_sc_encode_result(buf, sizeof(buf), message_id,
                              NULL, NULL, result_bvlc_function,
                              0, NULL, NULL, NULL, NULL );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  test_options(buf, len, BVLC_SC_RESULT, message_id,
               NULL, NULL, true, false, message.hdr.payload, 2, false);

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

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  memset(&origin.address, 0x63, BVLC_SC_VMAC_SIZE);
  memset(&dest.address, 0x24, BVLC_SC_VMAC_SIZE);
  memset(npdu, 0x99, sizeof(npdu));
  npdulen = 50;

  /* dest and origin absent */
  len = bvlc_sc_encode_encapsulated_npdu(buf, sizeof(buf), message_id,
                                         NULL, NULL, npdu, npdulen );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ENCAPSULATED_NPDU,
                               message_id, NULL, NULL, true, true, npdulen);
  zassert_equal(ret, true, NULL);
  zassert_not_equal(message.hdr.payload, 0, NULL);
  zassert_equal(message.hdr.payload_len, npdulen, NULL);
  res = memcmp(message.hdr.payload, npdu, npdulen);
  zassert_equal(res, 0, NULL);
  test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id,
               NULL, NULL, true, false,
               npdu, npdulen, true);
  test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id,
               NULL, NULL, false, true,
               npdu, npdulen, true);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));
  /* origin is presented, dest is absent */
  len = bvlc_sc_encode_encapsulated_npdu(buf, sizeof(buf), message_id,
                                         &origin, NULL, npdu, npdulen );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ENCAPSULATED_NPDU,
                               message_id, &origin, NULL, true, true, npdulen);
  zassert_equal(ret, true, NULL);
  zassert_not_equal(message.hdr.payload, 0, NULL);
  zassert_equal(message.hdr.payload_len, npdulen, NULL);
  res = memcmp(message.hdr.payload, npdu, npdulen);
  zassert_equal(res, 0, NULL);
  test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id,
               &origin, NULL, true, false,
               npdu, npdulen, true);
  test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id,
               &origin, NULL, false, true,
               npdu, npdulen, true);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));
  /* origin is absent, dest is presented */
  len = bvlc_sc_encode_encapsulated_npdu(buf, sizeof(buf), message_id,
                                         NULL, &dest, npdu, npdulen );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ENCAPSULATED_NPDU,
                               message_id, NULL, &dest, true, true, npdulen);
  zassert_equal(ret, true, NULL);
  zassert_not_equal(message.hdr.payload, 0, NULL);
  zassert_equal(message.hdr.payload_len, npdulen, NULL);
  res = memcmp(message.hdr.payload, npdu, npdulen);
  zassert_equal(res, 0, NULL);
  test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id,
               NULL, &dest, true, false,
               npdu, npdulen, true);
  test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id,
               NULL, &dest, false, true,
               npdu, npdulen, true);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));
  /* both dest and origin are presented */
  len = bvlc_sc_encode_encapsulated_npdu(buf, sizeof(buf), message_id,
                                         &origin, &dest, npdu, npdulen );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ENCAPSULATED_NPDU,
                               message_id, &origin, &dest, true,
                               true, npdulen);
  zassert_equal(ret, true, NULL);
  zassert_not_equal(message.hdr.payload, 0, NULL);
  zassert_equal(message.hdr.payload_len, npdulen, NULL);
  res = memcmp(message.hdr.payload, npdu, npdulen);
  zassert_equal(res, 0, NULL);
  test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id,
               &origin, &dest, true, false,
               npdu, npdulen, true);
  test_options(buf, len, BVLC_SC_ENCAPSULATED_NPDU, message_id,
               &origin, &dest, false, true,
               npdu, npdulen, true);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));
  /* truncated message, case 1 */
  len = bvlc_sc_encode_encapsulated_npdu(buf, sizeof(buf), message_id,
                                         &origin, &dest, npdu, npdulen );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 2 */

  ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

  /* truncated message, case 3 */

  ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 4 */

  ret = bvlc_sc_decode_message(buf, 16, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 5 */

  ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

  /* zero payload test */
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));
  len = bvlc_sc_encode_encapsulated_npdu(buf, sizeof(buf), message_id,
                                         &origin, &dest, npdu, 0 );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* 1 byte payload test */
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));
  len = bvlc_sc_encode_encapsulated_npdu(buf, sizeof(buf), message_id,
                                         &origin, &dest, npdu, 1 );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ENCAPSULATED_NPDU,
                               message_id, &origin, &dest, true,
                               true, 1);
  zassert_equal(ret, true, NULL);
}

static void test_ADDRESS_RESOLUTION(void)
{
  uint8_t buf[256];
  int len;
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE error;
  BACNET_ERROR_CLASS class;
  uint16_t message_id = 0x514a;
  int res;
  BACNET_SC_VMAC_ADDRESS origin;
  BACNET_SC_VMAC_ADDRESS dest;
  bool ret;

  memset(&origin.address, 0x27, BVLC_SC_VMAC_SIZE);
  memset(&dest.address, 0xaa, BVLC_SC_VMAC_SIZE);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* dest and origin absent */
  len = bvlc_sc_encode_address_resolution(buf, sizeof(buf), message_id,
                                          NULL, NULL);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION,
                               message_id, NULL, NULL, true, true, 0);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.payload, NULL, NULL);
  zassert_equal(message.hdr.payload_len, 0, NULL);
  test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION, message_id,
               NULL, NULL, true, false,
               NULL, 0, false);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* origin is presented, dest is absent */
  len = bvlc_sc_encode_address_resolution(buf, sizeof(buf), message_id,
                                          &origin, NULL);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION,
                               message_id, &origin, NULL, true, true, 0);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.payload, NULL, NULL);
  zassert_equal(message.hdr.payload_len, 0, NULL);
  test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION, message_id,
               &origin, NULL, true, false,
               NULL, 0, false);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));
  /* origin is absent, dest is presented */
  len = bvlc_sc_encode_address_resolution(buf, sizeof(buf), message_id,
                                          NULL, &dest);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION,
                               message_id, NULL, &dest, true, true, 0);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.payload, NULL, NULL);
  zassert_equal(message.hdr.payload_len, 0, NULL);
  test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION, message_id,
               NULL, &dest, true, false,
               NULL, 0, false);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));
  /* origin and dest are presented */
  len = bvlc_sc_encode_address_resolution(buf, sizeof(buf), message_id,
                                          &origin, &dest);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION,
                               message_id, &origin, &dest, true, true, 0);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.payload, NULL, NULL);
  zassert_equal(message.hdr.payload_len, 0, NULL);
  test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION, message_id,
               &origin, &dest, true, false,
               NULL, 0, false);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));
  /* truncated message, case 1 */
  len = bvlc_sc_encode_address_resolution(buf, sizeof(buf), message_id,
                                          &origin, &dest);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 2 */

  ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

  /* truncated message, case 3 */

  ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 5 */

  ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_ADDRESS_RESOLUTION_ACK(void)
{
  uint8_t buf[256];
  int len;
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE error;
  BACNET_ERROR_CLASS class;
  uint16_t message_id = 0xf1d3;
  int res;
  BACNET_SC_VMAC_ADDRESS origin;
  BACNET_SC_VMAC_ADDRESS dest;
  bool ret;
  char web_socket_uris[256];

  web_socket_uris[0] = 0;
  sprintf(web_socket_uris, "%s %s", "web_socket_uri1", "web_socket_uri2");
  memset(&origin.address, 0x91, BVLC_SC_VMAC_SIZE);
  memset(&dest.address, 0xef, BVLC_SC_VMAC_SIZE);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* dest and origin absent */
  len = bvlc_sc_encode_address_resolution_ack(
             buf, sizeof(buf), message_id,
             NULL, NULL, (uint8_t *)web_socket_uris, strlen(web_socket_uris));
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION_ACK,
                               message_id, NULL, NULL, true,
                               true, strlen(web_socket_uris));
  zassert_equal(ret, true, NULL);
  res = memcmp(message.hdr.payload,
               web_socket_uris, strlen(web_socket_uris));
  zassert_equal(res, 0, NULL);
  zassert_not_equal(message.payload.address_resolution_ack.
                    utf8_websocket_uri_string, NULL, NULL);
  zassert_equal(message.payload.address_resolution_ack.
                utf8_websocket_uri_string_len, strlen(web_socket_uris), NULL);
  zassert_equal(message.hdr.payload_len, strlen(web_socket_uris), NULL);
  test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION_ACK, message_id,
               NULL, NULL, true, false,
               (uint8_t *)web_socket_uris, strlen(web_socket_uris), true);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* origin is presented, dest is absent */
  len = bvlc_sc_encode_address_resolution_ack(
             buf, sizeof(buf), message_id,
             &origin, NULL, (uint8_t *)web_socket_uris,
             strlen(web_socket_uris));
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION_ACK,
                               message_id, &origin, NULL, true,
                               true, strlen(web_socket_uris));
  zassert_equal(ret, true, NULL);
  res = memcmp(message.hdr.payload,
               web_socket_uris, strlen(web_socket_uris));
  zassert_equal(res, 0, NULL);
  zassert_not_equal(message.payload.address_resolution_ack.
                    utf8_websocket_uri_string, NULL, NULL);
  zassert_equal(message.payload.address_resolution_ack.
                utf8_websocket_uri_string_len, strlen(web_socket_uris), NULL);
  zassert_equal(message.hdr.payload_len, strlen(web_socket_uris), NULL);
  test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION_ACK, message_id,
               &origin, NULL, true, false,
               (uint8_t *)web_socket_uris, strlen(web_socket_uris), true);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* origin is absent, dest is presented */
  len = bvlc_sc_encode_address_resolution_ack(
             buf, sizeof(buf), message_id,
             NULL, &dest, (uint8_t *)web_socket_uris,
             strlen(web_socket_uris));
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION_ACK,
                               message_id, NULL, &dest, true,
                               true, strlen(web_socket_uris));
  zassert_equal(ret, true, NULL);
  res = memcmp(message.hdr.payload,
               web_socket_uris, strlen(web_socket_uris));
  zassert_equal(res, 0, NULL);
  zassert_not_equal(message.payload.address_resolution_ack.
                    utf8_websocket_uri_string, NULL, NULL);
  zassert_equal(message.payload.address_resolution_ack.
                utf8_websocket_uri_string_len, strlen(web_socket_uris), NULL);
  zassert_equal(message.hdr.payload_len, strlen(web_socket_uris), NULL);
  test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION_ACK, message_id,
               NULL, &dest, true, false,
               (uint8_t *)web_socket_uris, strlen(web_socket_uris), true);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* origin and dest are presented */
  len = bvlc_sc_encode_address_resolution_ack(
             buf, sizeof(buf), message_id,
             &origin, &dest, (uint8_t *)web_socket_uris,
             strlen(web_socket_uris));
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION_ACK,
                               message_id, &origin, &dest, true,
                               true, strlen(web_socket_uris));
  zassert_equal(ret, true, NULL);
  res = memcmp(message.hdr.payload,
               web_socket_uris, strlen(web_socket_uris));
  zassert_equal(res, 0, NULL);
  zassert_not_equal(message.payload.address_resolution_ack.
                    utf8_websocket_uri_string, NULL, NULL);
  zassert_equal(message.payload.address_resolution_ack.
                utf8_websocket_uri_string_len, strlen(web_socket_uris), NULL);
  zassert_equal(message.hdr.payload_len, strlen(web_socket_uris), NULL);
  test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION_ACK, message_id,
               &origin, &dest, true, false,
               (uint8_t *)web_socket_uris, strlen(web_socket_uris), true);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* zero payload test */
  len = bvlc_sc_encode_address_resolution_ack(
             buf, sizeof(buf), message_id,
             &origin, &dest, NULL, 0);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADDRESS_RESOLUTION_ACK,
                               message_id, &origin, &dest, true,
                               true, 0);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.payload, NULL, NULL);
  zassert_equal(message.payload.address_resolution_ack.
                utf8_websocket_uri_string, NULL, NULL);
  zassert_equal(message.payload.address_resolution_ack.
                utf8_websocket_uri_string_len, 0, NULL);
  zassert_equal(message.hdr.payload_len, 0, NULL);
  test_options(buf, len, BVLC_SC_ADDRESS_RESOLUTION_ACK, message_id,
               &origin, &dest, true, false,
               NULL, 0, true);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* truncated message, case 1 */

  len = bvlc_sc_encode_address_resolution_ack(
             buf, sizeof(buf), message_id,
             &origin, &dest, (uint8_t *)web_socket_uris,
             strlen(web_socket_uris));
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 2 */

  ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

  /* truncated message, case 3 */

  ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 4 */

  ret = bvlc_sc_decode_message(buf, 15, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 5 */

  ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_ADVERTISIMENT(void)
{
  uint8_t buf[256];
  int len;
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE error;
  BACNET_ERROR_CLASS class;
  uint16_t message_id = 0xe2ad;
  int res;
  BACNET_SC_VMAC_ADDRESS origin;
  BACNET_SC_VMAC_ADDRESS dest;
  bool ret;
  BVLC_SC_HUB_CONNECTION_STATUS hub_status =
          BVLC_SC_HUB_CONNECTION_PRIMARY_HUB_CONNECTED;
  BVLC_SC_DIRECT_CONNECTION_SUPPORT support =
          BVLC_SC_DIRECT_CONNECTIONS_ACCEPT_SUPPORTED;
  uint16_t max_blvc_len = 567;
  uint16_t max_npdu_len = 1323;

  memset(&origin.address, 0xe1, BVLC_SC_VMAC_SIZE);
  memset(&dest.address, 0x4f, BVLC_SC_VMAC_SIZE);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* dest and origin absent */
  len = bvlc_sc_encode_advertisiment(
             buf, sizeof(buf), message_id,NULL, NULL,
             hub_status, support, max_blvc_len, max_npdu_len);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADVERTISIMENT,
                               message_id, NULL, NULL, true,
                               true, 6);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.payload.advertisiment.hub_status,
                hub_status, NULL);
  zassert_equal(message.payload.advertisiment.support,
                support, NULL);
  zassert_equal(message.payload.advertisiment.max_blvc_len,
                max_blvc_len, NULL);
  zassert_equal(message.payload.advertisiment.max_npdu_len,
                max_npdu_len, NULL);
  test_options(buf, len, BVLC_SC_ADVERTISIMENT, message_id,
               NULL, NULL, true, false,
               message.hdr.payload, message.hdr.payload_len, false);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* origin is presented, dest is absent */
  len = bvlc_sc_encode_advertisiment(
             buf, sizeof(buf), message_id, &origin, NULL,
             hub_status, support, max_blvc_len, max_npdu_len);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADVERTISIMENT,
                               message_id, &origin, NULL, true,
                               true, 6);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.payload.advertisiment.hub_status,
                hub_status, NULL);
  zassert_equal(message.payload.advertisiment.support,
                support, NULL);
  zassert_equal(message.payload.advertisiment.max_blvc_len,
                max_blvc_len, NULL);
  zassert_equal(message.payload.advertisiment.max_npdu_len,
                max_npdu_len, NULL);
  test_options(buf, len, BVLC_SC_ADVERTISIMENT, message_id,
               &origin, NULL, true, false,
               message.hdr.payload, message.hdr.payload_len, false);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* origin is absent, dest is presented */
  len = bvlc_sc_encode_advertisiment(
             buf, sizeof(buf), message_id, NULL, &dest,
             hub_status, support, max_blvc_len, max_npdu_len);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADVERTISIMENT,
                               message_id, NULL, &dest, true,
                               true, 6);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.payload.advertisiment.hub_status,
                hub_status, NULL);
  zassert_equal(message.payload.advertisiment.support,
                support, NULL);
  zassert_equal(message.payload.advertisiment.max_blvc_len,
                max_blvc_len, NULL);
  zassert_equal(message.payload.advertisiment.max_npdu_len,
                max_npdu_len, NULL);
  test_options(buf, len, BVLC_SC_ADVERTISIMENT, message_id,
                NULL, &dest, true, false,
               message.hdr.payload, message.hdr.payload_len, false);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* origin and dest are presented */

  len = bvlc_sc_encode_advertisiment(
             buf, sizeof(buf), message_id, &origin, &dest,
             hub_status, support, max_blvc_len, max_npdu_len);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_ADVERTISIMENT,
                               message_id, &origin, &dest, true,
                               true, 6);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.payload.advertisiment.hub_status,
                hub_status, NULL);
  zassert_equal(message.payload.advertisiment.support,
                support, NULL);
  zassert_equal(message.payload.advertisiment.max_blvc_len,
                max_blvc_len, NULL);
  zassert_equal(message.payload.advertisiment.max_npdu_len,
                max_npdu_len, NULL);
  test_options(buf, len, BVLC_SC_ADVERTISIMENT, message_id,
               &origin, &dest, true, false,
               message.hdr.payload, message.hdr.payload_len, false);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* truncated message, case 1 */

  len = bvlc_sc_encode_advertisiment(
             buf, sizeof(buf), message_id, &origin, &dest,
             hub_status, support, max_blvc_len, max_npdu_len);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 2 */

  ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

  /* truncated message, case 3 */

  ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 4 */

  ret = bvlc_sc_decode_message(buf, 15, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 5 */

  ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

static void test_ADVERTISIMENT_SOLICITATION(void)
{
  uint8_t buf[256];
  int len;
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE error;
  BACNET_ERROR_CLASS class;
  uint16_t message_id = 0xaf4a;
  int res;
  BACNET_SC_VMAC_ADDRESS origin;
  BACNET_SC_VMAC_ADDRESS dest;
  bool ret;

  memset(&origin.address, 0x17, BVLC_SC_VMAC_SIZE);
  memset(&dest.address, 0x1a, BVLC_SC_VMAC_SIZE);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* dest and origin absent */
  len = bvlc_sc_encode_advertisiment_solicitation(
               buf, sizeof(buf), message_id,
               NULL, NULL);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr,
                               BVLC_SC_ADVERTISIMENT_SOLICITATION,
                               message_id, NULL, NULL, true, true, 0);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.payload, NULL, NULL);
  zassert_equal(message.hdr.payload_len, 0, NULL);
  test_options(buf, len, BVLC_SC_ADVERTISIMENT_SOLICITATION, message_id,
               NULL, NULL, true, false,
               NULL, 0, false);

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* origin is presented, dest is absent */
  len = bvlc_sc_encode_advertisiment_solicitation(
               buf, sizeof(buf), message_id,
               &origin, NULL);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr,
                               BVLC_SC_ADVERTISIMENT_SOLICITATION,
                               message_id, &origin, NULL, true, true, 0);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.payload, NULL, NULL);
  zassert_equal(message.hdr.payload_len, 0, NULL);
  test_options(buf, len, BVLC_SC_ADVERTISIMENT_SOLICITATION, message_id,
               &origin, NULL, true, false,
               NULL, 0, false);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));
  /* origin is absent, dest is presented */
  len = bvlc_sc_encode_advertisiment_solicitation(
               buf, sizeof(buf), message_id,
               NULL, &dest);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr,
                               BVLC_SC_ADVERTISIMENT_SOLICITATION,
                               message_id, NULL, &dest, true, true, 0);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.payload, NULL, NULL);
  zassert_equal(message.hdr.payload_len, 0, NULL);
  test_options(buf, len, BVLC_SC_ADVERTISIMENT_SOLICITATION, message_id,
               NULL, &dest, true, false,
               NULL, 0, false);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));
  /* origin and dest are presented */
  len = bvlc_sc_encode_advertisiment_solicitation(
               buf, sizeof(buf), message_id,
               &origin, &dest);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr,
                               BVLC_SC_ADVERTISIMENT_SOLICITATION,
                               message_id, &origin, &dest, true, true, 0);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.payload, NULL, NULL);
  zassert_equal(message.hdr.payload_len, 0, NULL);
  test_options(buf, len, BVLC_SC_ADVERTISIMENT_SOLICITATION, message_id,
               &origin, &dest, true, false,
               NULL, 0, false);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));
  /* truncated message, case 1 */
  len = bvlc_sc_encode_advertisiment_solicitation(
               buf, sizeof(buf), message_id,
               &origin, &dest);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 2 */

  ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

  /* truncated message, case 3 */

  ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 5 */

  ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
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
  BACNET_SC_VMAC_ADDRESS origin;
  BACNET_SC_VMAC_ADDRESS dest;
  bool ret;
  uint16_t max_blvc_len = 9997;
  uint16_t max_npdu_len = 3329;
  BACNET_SC_VMAC_ADDRESS local_vmac;
  BACNET_SC_UUID         local_uuid;

  memset(&local_vmac, 0x88, sizeof(local_vmac));
  memset(&local_uuid, 0x22, sizeof(local_uuid));

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  len = bvlc_sc_encode_connect_request(
               buf, sizeof(buf), message_id,
               &local_vmac, &local_uuid, max_blvc_len, max_npdu_len);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr,
                               BVLC_SC_CONNECT_REQUEST,
                               message_id, NULL, NULL, true, true, 26);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.payload_len, 26, NULL);
  res = memcmp(message.payload.connect_request.local_vmac,
               &local_vmac, BVLC_SC_VMAC_SIZE);
  zassert_equal(res, 0, NULL);
  res = memcmp(message.payload.connect_request.local_uuid,
               &local_uuid, BVLC_SC_VMAC_SIZE);
  zassert_equal(res, 0, NULL);
  zassert_equal(message.payload.connect_request.max_blvc_len,
                max_blvc_len, NULL);
  zassert_equal(message.payload.connect_request.max_npdu_len,
                max_npdu_len, NULL);
  test_options(buf, len, BVLC_SC_CONNECT_REQUEST, message_id,
               NULL, NULL, true, false,
               message.hdr.payload, message.hdr.payload_len, false);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));
  /* truncated message, case 1 */
  len = bvlc_sc_encode_connect_request(
               buf, sizeof(buf), message_id,
               &local_vmac, &local_uuid, max_blvc_len, max_npdu_len);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 2 */

  ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

  /* truncated message, case 3 */

  ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 5 */

  ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* message has dest */
  buf[1] |= BVLC_SC_CONTROL_DEST_VADDR;
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_INCONSISTENT_PARAMETERS, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* message has origin */
  buf[1] |= BVLC_SC_CONTROL_ORIG_VADDR;
  buf[1] &= ~BVLC_SC_CONTROL_DEST_VADDR;
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_INCONSISTENT_PARAMETERS, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* message has both dest and orign */
  buf[1] |= BVLC_SC_CONTROL_ORIG_VADDR;
  buf[1] |= BVLC_SC_CONTROL_DEST_VADDR;
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_INCONSISTENT_PARAMETERS, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
}

#if 0
  /* dest and origin absent */
  /* origin is presented, dest is absent */
  /* origin is absent, dest is presented */
  /* origin and dest are presented */
#endif

static void test_CONNECT_ACCEPT(void)
{
  uint8_t buf[256];
  int len;
  BVLC_SC_DECODED_MESSAGE message;
  BACNET_ERROR_CODE error;
  BACNET_ERROR_CLASS class;
  uint16_t message_id = 0x0203;
  int res;
  BACNET_SC_VMAC_ADDRESS origin;
  BACNET_SC_VMAC_ADDRESS dest;
  bool ret;
  uint16_t max_blvc_len = 1027;
  uint16_t max_npdu_len = 22;
  BACNET_SC_VMAC_ADDRESS local_vmac;
  BACNET_SC_UUID         local_uuid;

  memset(&local_vmac, 0x33, sizeof(local_vmac));
  memset(&local_uuid, 0x11, sizeof(local_uuid));

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  len = bvlc_sc_encode_connect_accept(
               buf, sizeof(buf), message_id,
               &local_vmac, &local_uuid, max_blvc_len, max_npdu_len);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr,
                               BVLC_SC_CONNECT_ACCEPT,
                               message_id, NULL, NULL, true, true, 26);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.payload_len, 26, NULL);
  res = memcmp(message.payload.connect_accept.local_vmac,
               &local_vmac, BVLC_SC_VMAC_SIZE);
  zassert_equal(res, 0, NULL);
  res = memcmp(message.payload.connect_accept.local_uuid,
               &local_uuid, BVLC_SC_VMAC_SIZE);
  zassert_equal(res, 0, NULL);
  zassert_equal(message.payload.connect_accept.max_blvc_len,
                max_blvc_len, NULL);
  zassert_equal(message.payload.connect_accept.max_npdu_len,
                max_npdu_len, NULL);
  test_options(buf, len, BVLC_SC_CONNECT_ACCEPT, message_id,
               NULL, NULL, true, false,
               message.hdr.payload, message.hdr.payload_len, false);
  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));
  /* truncated message, case 1 */
  len = bvlc_sc_encode_connect_accept(
               buf, sizeof(buf), message_id,
               &local_vmac, &local_uuid, max_blvc_len, max_npdu_len);
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, 5, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 2 */

  ret = bvlc_sc_decode_message(buf, 6, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);

  /* truncated message, case 3 */

  ret = bvlc_sc_decode_message(buf, 13, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* truncated message, case 5 */

  ret = bvlc_sc_decode_message(buf, 4, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_MESSAGE_INCOMPLETE, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* message has dest */
  buf[1] |= BVLC_SC_CONTROL_DEST_VADDR;
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_INCONSISTENT_PARAMETERS, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* message has origin */
  buf[1] |= BVLC_SC_CONTROL_ORIG_VADDR;
  buf[1] &= ~BVLC_SC_CONTROL_DEST_VADDR;
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_INCONSISTENT_PARAMETERS, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);
  /* message has both dest and orign */
  buf[1] |= BVLC_SC_CONTROL_ORIG_VADDR;
  buf[1] |= BVLC_SC_CONTROL_DEST_VADDR;
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, false, NULL);
  zassert_equal(error, ERROR_CODE_INCONSISTENT_PARAMETERS, NULL);
  zassert_equal(class, ERROR_CLASS_COMMUNICATION, NULL);


}
void test_main(void)
{
    ztest_test_suite(bvlc_sc_tests,
        ztest_unit_test(test_BVLC_RESULT),
        ztest_unit_test(test_ENCAPSULATED_NPDU),
        ztest_unit_test(test_ADDRESS_RESOLUTION),
        ztest_unit_test(test_ADDRESS_RESOLUTION_ACK),
        ztest_unit_test(test_ADVERTISIMENT),
        ztest_unit_test(test_ADVERTISIMENT_SOLICITATION),
        ztest_unit_test(test_CONNECT_REQUEST),
        ztest_unit_test(test_CONNECT_ACCEPT)
     );

    ztest_run_test_suite(bvlc_sc_tests);
}
