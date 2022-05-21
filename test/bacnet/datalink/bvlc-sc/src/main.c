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

#if 0
typedef struct
{
  uint8_t         bvlc_function;
  uint16_t        message_id;
  BACNET_ADDRESS  origin;
  BACNET_ADDRESS  dest;
  uint8_t        *dest_options;     /* pointer to packed dest options list in message */
  uint16_t        dest_options_len;
  uint16_t        dest_options_num; /* number of filled items in dest_options */
  uint8_t        *data_options;     /* pointer to packed data options list in message */
  uint16_t        data_options_len;
  uint16_t        data_options_num; /* number of filled items in data_options */
  uint8_t        *payload;          /* packed payload, points to data in message */
  uint16_t        payload_len;
} bvlc_sc_unpacked_hdr_t;
#endif

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

static void test_1_option(uint8_t                 *pdu, 
                          uint16_t                 pdu_size,
                          uint8_t                  bvlc_function,
                          uint16_t                 message_id,
                          BACNET_SC_VMAC_ADDRESS  *origin,
                          BACNET_SC_VMAC_ADDRESS  *dest,
                          bool                     dest_options_absent,
                          bool                     data_options_absent,
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
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, bvlc_function, message_id,
                               origin, dest, dest_options_absent,
                               data_options_absent, payload_len);
  zassert_equal(ret, true, NULL);
  zassert_equal(message.hdr.dest_options_num, 1, NULL);
  zassert_equal(message.dest_options[0].type,
                BVLC_SC_OPTION_TYPE_SECURE_PATH, NULL);
  zassert_equal(message.dest_options[0].must_understand, true, NULL);
  zassert_equal(message.hdr.payload_len, payload_len, NULL);
  res = memcmp(message.hdr.payload, payload, payload_len);
  zassert_equal(res, 0, NULL);
}

/* 3 options are added to pdu in total: 1 sc, 2 proprietary */

static void test_3_options(uint8_t                *pdu, 
                          uint16_t                 pdu_size,
                          uint8_t                  bvlc_function,
                          uint16_t                 message_id,
                          BACNET_SC_VMAC_ADDRESS  *origin,
                          BACNET_SC_VMAC_ADDRESS  *dest,
                          bool                     dest_options_absent,
                          bool                     data_options_absent,
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
  len = bvlc_sc_add_option_to_destination_options(buf,
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
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id,
                               NULL, NULL, false, true, 2);
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
                vendor_id2, NULL);
  zassert_equal(message.dest_options[0].specific.proprietary.option_type,
                proprietary_option_type2, NULL);
  zassert_equal(message.dest_options[0].specific.proprietary.data_len,
                sizeof(proprietary_data2), NULL);
  res = memcmp(message.dest_options[0].specific.proprietary.data,
               proprietary_data2, sizeof(proprietary_data2));
  zassert_equal(res, 0, NULL);
  zassert_equal(message.dest_options[1].type,
                BVLC_SC_OPTION_TYPE_PROPRIETARY, NULL);
  zassert_equal(message.dest_options[1].must_understand, true, NULL);
  zassert_equal(message.dest_options[1].packed_header_marker & 
                BVLC_SC_HEADER_MORE, BVLC_SC_HEADER_MORE, NULL);
  zassert_equal(message.dest_options[1].packed_header_marker & 
                BVLC_SC_HEADER_DATA, BVLC_SC_HEADER_DATA, NULL);
  zassert_equal(message.dest_options[2].type,
                BVLC_SC_OPTION_TYPE_SECURE_PATH, NULL);
  zassert_equal(message.dest_options[2].must_understand, true, NULL);
  zassert_equal(message.dest_options[2].packed_header_marker & 
                BVLC_SC_HEADER_MORE, 0, NULL);
  zassert_equal(message.dest_options[2].packed_header_marker & 
                BVLC_SC_HEADER_DATA, 0, NULL);
  zassert_equal(message.hdr.payload_len, payload_len, NULL);
  res = memcmp(message.hdr.payload, payload, payload_len);
  zassert_equal(res, 0, NULL);
}

static void test_options(uint8_t                 *pdu, 
                         uint16_t                 pdu_size,
                         uint8_t                  bvlc_function,
                         uint16_t                 message_id,
                         BACNET_SC_VMAC_ADDRESS  *origin,
                         BACNET_SC_VMAC_ADDRESS  *dest,
                         bool                     dest_options_absent,
                         bool                     data_options_absent,
                         uint8_t                 *payload,
                         uint16_t                 payload_len)
{
  test_1_option(pdu, pdu_size, bvlc_function, message_id, origin,
                dest, dest_options_absent, data_options_absent,
                payload,payload_len);
  test_3_options(pdu, pdu_size, bvlc_function, message_id, origin,
                 dest, dest_options_absent, data_options_absent,
                 payload, payload_len);
}

static void test_BVLC_RESULT(void)
{
  uint8_t buf[256];
  uint8_t optbuf[256];
  int len;
  BACNET_SC_VMAC_ADDRESS origin;
  BACNET_SC_VMAC_ADDRESS dest;
  uint16_t message_id = 0x7777;
  uint8_t bvlc_function = 1;
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

  /* simple test, no options, origin and dest presented */
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

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* no options, origin presented */
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

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* no options, dest presented */
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

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* no options, dest and origin absent */
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

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* no options, dest and origin absent */
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

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* no options, nak,no details string */
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

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* no options, nak , details string */
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

  memset(buf, 0, sizeof(buf));
  memset(&message, 0, sizeof(message));

  /* no options, dest and origin, nak , details string */
  len = bvlc_sc_encode_result(buf, sizeof(buf), message_id,
                               &dest, &origin, result_bvlc_function, 1,
                               &error_header_marker, &error_class,
                               &error_code, (uint8_t*) error_details_string );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  ret = verify_bsc_bvll_header(&message.hdr, BVLC_SC_RESULT, message_id,
                               &dest, &origin, true, true,
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

  /* origin and dest absent, option test*/
  len = bvlc_sc_encode_result(buf, sizeof(buf), message_id,
                              NULL, NULL, result_bvlc_function,
                              0, NULL, NULL, NULL, NULL );
  zassert_not_equal(len, 0, NULL);
  ret = bvlc_sc_decode_message(buf, len, &message, &error, &class);
  zassert_equal(ret, true, NULL);
  test_options(buf, len, BVLC_SC_RESULT, message_id,
               NULL, NULL, false, true, message.hdr.payload, 2);

}

void test_main(void)
{
    ztest_test_suite(bvlc_sc_tests,
      ztest_unit_test(test_BVLC_RESULT)
     );

    ztest_run_test_suite(bvlc_sc_tests);
}
