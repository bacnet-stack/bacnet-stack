#include "bacnet/datalink/bvlc-sc.h"

typedef enum BACNet_Option_Validation_Type
{
  BACNET_USER_OPTION_VALIDATION,
  BACNET_PDU_DEST_OPTION_VALIDATION,
  BACNET_PDU_DATA_OPTION_VALIDATION
} BACNET_OPTION_VALIDATION_TYPE;

static bool bvlc_sc_validate_options_headers(
                BACNET_OPTION_VALIDATION_TYPE  validation_type,
                uint8_t                       *option_headers,
                uint16_t                       option_headers_max_len,
                uint16_t                      *out_option_headers_real_length,
                uint8_t                      **out_last_option_marker_ptr,
                uint16_t                      *out_option_header_num,
                BACNET_ERROR_CODE             *error,
                BACNET_ERROR_CLASS            *class)
{
  int options_len = 0;
  uint8_t flags = 0;
  uint8_t option;
  uint16_t hdr_len;
  
  if(!option_headers_max_len || !out_option_headers_real_length ||
     !error || !class) {
    return false;
  }

  if(out_option_header_num) {
    *out_option_header_num = 0;
  }

  while(options_len < option_headers_max_len) {
    flags = option_headers[options_len];

    if(out_last_option_marker_ptr) {
      *out_last_option_marker_ptr = &option_headers[options_len];
    }

    option = flags & BVLC_SC_HEADER_OPTION_TYPE_MASK;

    if(option != BVLC_SC_OPTION_TYPE_SECURE_PATH && 
       option != BVLC_SC_OPTION_TYPE_PROPRIETARY ) {
      *error = ERROR_CODE_HEADER_ENCODING_ERROR;
      *class = ERROR_CLASS_COMMUNICATION;
      return false;
    }

    if(option == BVLC_SC_OPTION_TYPE_SECURE_PATH) {
      if(validation_type == BACNET_PDU_DEST_OPTION_VALIDATION) {
        /* According BACNet standard secure path header option can be added
          only to data options. (AB.2.3.1 Secure Path Header Option) */
        *error = ERROR_CODE_HEADER_ENCODING_ERROR;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }
      if(flags & BVLC_SC_HEADER_DATA) {
        /* securepath option header does not have data header
           according bacnet stadard */
        *error = ERROR_CODE_HEADER_ENCODING_ERROR;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }
      options_len++;
    }
    else if (option == BVLC_SC_OPTION_TYPE_PROPRIETARY) {
      if(!(flags & BVLC_SC_HEADER_DATA)) {
        /* proprietary option must have header data */
        *error = ERROR_CODE_HEADER_ENCODING_ERROR;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }
      options_len++;
      if(options_len + 2 > option_headers_max_len) {
        /* not enough data to process header that means
           that probably message is incomplete */
        *error = ERROR_CODE_MESSAGE_INCOMPLETE;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }
      memcpy(&hdr_len, &option_headers[options_len], 2);
      options_len += 2 /* header length */ + 
                     hdr_len /* length of data header */;
    }

    if(options_len > option_headers_max_len) {
      /* not enough data to process header that means
         that probably message is incomplete */
      *error = ERROR_CODE_MESSAGE_INCOMPLETE;
      *class = ERROR_CLASS_COMMUNICATION;
      return false;
    }

    if(out_option_header_num) {
      if(*out_option_header_num < USHRT_MAX) {
        *out_option_header_num += 1;
      }
      else {
        if(flags & BVLC_SC_HEADER_MORE) {
          return false;
        }
      }
    }

    if(!(flags & BVLC_SC_HEADER_MORE)) {
      break;
    }
  } 

  *out_option_headers_real_length = options_len;
  return true;
}

static uint16_t bvlc_sc_add_option(bool      to_data_option,
                                   uint8_t  *pdu,
                                   uint16_t  pdu_size,
                                   uint8_t  *in_pdu,
                                   uint16_t  in_pdu_len,
                                   uint8_t  *sc_option,
                                   uint16_t  sc_option_len)
{
  uint16_t offs = 4;
  uint8_t flags = 0;
  bool found_end = false;
  uint16_t options_len = 0;
  uint8_t* last_option_marker_ptr = NULL;
  uint8_t mask;
  BACNET_ERROR_CODE  error;
  BACNET_ERROR_CLASS class;
  BACNET_OPTION_VALIDATION_TYPE vt;

  if(!in_pdu_len || !in_pdu || !sc_option_len ||
     !pdu_size   || !pdu    || !sc_option ) {
    return 0;
  }

  if(pdu_size < 4 || in_pdu_len < 4) {
    return 0;
  }

  if(((int)(sc_option_len)) + ((int)(in_pdu_len)) > USHRT_MAX) {
    return 0;
  }

  if(pdu_size < in_pdu_len) {
    return 0;
  }

  if((((int)pdu_size)) < 
                 ((int)(sc_option_len)) + ((int)(in_pdu_len))) {
    return 0;
  }

  /* ensure that sc_option does not have flag more options */

  if(sc_option[0] & BVLC_SC_HEADER_MORE) {
    return 0;
  }

  if(!to_data_option &&
     (sc_option[0] & BVLC_SC_HEADER_OPTION_TYPE_MASK) ==
                                        BVLC_SC_OPTION_TYPE_SECURE_PATH) {
    /* According BACNet standard secure path header option can be added
       only to data options. (AB.2.3.1 Secure Path Header Option) */
    return 0;
  }

  /* ensure that user wants to add valid option */
  if(!bvlc_sc_validate_options_headers(BACNET_USER_OPTION_VALIDATION,
                                       sc_option, sc_option_len,
                                       &options_len, NULL, NULL,
                                       &error, &class)) {
    return 0;
  }

  flags = in_pdu[1];

  if(flags & BVLC_SC_CONTROL_DEST_VADDR) {
    offs += BVLC_SC_VMAC_SIZE;
  }

  if(flags & BVLC_SC_CONTROL_ORIG_VADDR) {
    offs += BVLC_SC_VMAC_SIZE;
  }

  if(offs > in_pdu_len) {
    return 0;
  }

  /* insert options */

  if(to_data_option) {
    mask = BVLC_SC_CONTROL_DATA_OPTIONS;
    vt = BACNET_PDU_DATA_OPTION_VALIDATION;

    if(flags & BVLC_SC_CONTROL_DEST_OPTIONS) {
      /* some options are already presented in message.
         Validate them at first. */
      if(!bvlc_sc_validate_options_headers(BACNET_PDU_DEST_OPTION_VALIDATION,
                                           &in_pdu[offs], 
                                           in_pdu_len - offs, 
                                           &options_len,
                                           &last_option_marker_ptr,
                                           NULL,
                                           &error,
                                           &class)) {
        return 0;
      }
      offs += options_len;
    }
  }
  else {
    mask = BVLC_SC_CONTROL_DEST_OPTIONS;
    vt = BACNET_PDU_DEST_OPTION_VALIDATION;
  }

  if((flags & mask)) {
    /* some options are already presented in message.
       Validate them at first. */

    if(!bvlc_sc_validate_options_headers(vt,
                                         &in_pdu[offs], 
                                          in_pdu_len - offs, 
                                         &options_len,
                                         &last_option_marker_ptr,
                                          NULL,
                                          &error,
                                          &class)) {
      return 0;
    }
  }

  if(pdu == in_pdu) {
    /* user would like to use the same buffer where in_pdu
       is already stored. So we need to expand it for sc_option_len */
    memmove(&pdu[offs + sc_option_len], 
            &pdu[offs], in_pdu_len -  offs);
    memcpy(&pdu[offs], sc_option, sc_option_len);
  }
  else {
     /* user would like to use a new buffer for in_pdu with option.*/
    memcpy(pdu, in_pdu, offs);
    memcpy(&pdu[offs], sc_option, sc_option_len);
    if((flags & mask)) {
      pdu[offs] |= BVLC_SC_HEADER_MORE;
    }
    memcpy(&pdu[offs+sc_option_len],
           &in_pdu[offs], in_pdu_len -  offs);
  }
  if((flags & mask)) {
    pdu[offs] |= BVLC_SC_HEADER_MORE;
  }
  else {
    pdu[1] |= mask;
  }
  return in_pdu_len + sc_option_len;
}


uint16_t bvlc_sc_add_option_to_destination_options(uint8_t  *out_pdu,
                                                   uint16_t  out_pdu_size,
                                                   uint8_t  *pdu,
                                                   uint16_t  pdu_size,
                                                   uint8_t  *sc_option,
                                                   uint16_t  sc_option_len)
{
  return bvlc_sc_add_option(false, out_pdu, out_pdu_size, pdu,
                            pdu_size, sc_option, sc_option_len);
}

uint16_t bvlc_sc_add_option_to_data_options(uint8_t  *out_pdu,
                                            uint16_t  out_pdu_size,
                                            uint8_t  *pdu,
                                            uint16_t  pdu_size,
                                            uint8_t  *sc_option,
                                            uint16_t  sc_option_len)
{
  return bvlc_sc_add_option(true, out_pdu, out_pdu_size, pdu,
                            pdu_size, sc_option, sc_option_len);
}

uint16_t bvlc_sc_encode_proprietary_option(uint8_t  *pdu,
                                           uint16_t  pdu_size,
                                           bool      must_understand,
                                           uint16_t  vendor_id,
                                           uint8_t   proprietary_option_type,
                                           uint8_t  *proprietary_data,
                                           uint16_t  proprietary_data_len)
{
  uint16_t total_len = sizeof(vendor_id) + 
                       proprietary_data_len + sizeof(uint8_t);

  if(proprietary_data_len > BVLC_SC_NPDU_MAX - 
                            3 /* header marker len + headerlength len) */ - 
                            sizeof(vendor_id)) {
    return 0;
  }

  if(pdu_size < total_len + 3 /* header marker len + header data len) */) {
    return 0;
  }

  /* reset More Options, Must Understand and Header Data flags
     thay will be updated as a result of call bvlc_sc_add_sc_option() */

  pdu[0] = BVLC_SC_OPTION_TYPE_PROPRIETARY;

  if(must_understand) {
    pdu[0] |= BVLC_SC_HEADER_MUST_UNDERSTAND;
  }

  pdu[0] |= BVLC_SC_HEADER_DATA;
  memcpy(&pdu[1], &total_len, sizeof(total_len));
  memcpy(&pdu[3], &vendor_id, sizeof(vendor_id));
  memcpy(&pdu[5], 
         &proprietary_option_type, sizeof(proprietary_option_type));
  memcpy(&pdu[6], proprietary_data, proprietary_data_len);
  return total_len + 3;
}

// returns length of created option
uint16_t bvlc_sc_encode_secure_path_option(uint8_t* pdu,
                                           uint16_t pdu_size,
                                           bool     must_understand)
{
  if(pdu_size < 1) {
    return 0;
  }

  pdu[0] = BVLC_SC_OPTION_TYPE_SECURE_PATH;

  if(must_understand) {
    pdu[0] |= BVLC_SC_HEADER_MUST_UNDERSTAND;
  }
  return 1;
}

/**
 * @brief Decodes BVLC header options marker. Function assumes to be called iteratively
 *        until end of option list will be reached. User must call this function on
 *        previously validated options list only because sanity checks are omitted.
 *        That valided option list can be get by some bvlc_sc_encode_ function calls.
 *
 * @param in_option_list - buffer contaning list of header options.It must point
 *                         to head list item to be decoded.
 * @param in_option_list_len - length in bytes of header options list
 * @param out_opt_type - pointer to store decoded option type, must not be NULL
 * @param out_must_understand - pointer to store decoded 'must understand'
 *                              flag from options marker, must not be NULL.
 * @param out_next_option - pointer to next option list item to be decoded, can be NULL
 *                          if no any option items left.
 *
 * @return 0 in a case if decoding failed, otherwise returns
 *         length value of decoded option in bytes.
 *
 * Typically code for parsing of option list looks like following:
 *
 *  uint8_t *current = in_options_list;
 *  int option_len = 0;
 *  while(current) {
 *    option_len = bvlc_sc_decode_option_hdr(current, &in_options_list_len, &type, ...., &current);
 *    if(option_len == 0) {
        // handle error
        break;
      }
 *    in_options_list_len -= option_len;
 *  }
 *
 */

static unsigned int bvlc_sc_decode_option_hdr(
                         uint8_t              *in_options_list,
                         uint16_t              in_option_list_len,
                         BVLC_SC_OPTION_TYPE  *out_opt_type,
                         bool                 *out_must_understand,
                         uint8_t             **out_next_option)
{
  uint16_t len;

  *out_next_option = NULL;
  *out_opt_type = (BVLC_SC_OPTION_TYPE)
                  (in_options_list[0] & BVLC_SC_HEADER_OPTION_TYPE_MASK);
  *out_must_understand = (in_options_list[0] & BVLC_SC_HEADER_MUST_UNDERSTAND)
                          ? true : false;

  if(*out_opt_type == BVLC_SC_OPTION_TYPE_SECURE_PATH) {
    if(in_options_list[0] & BVLC_SC_HEADER_MORE) {
      *out_next_option = in_options_list + 1;
    }
    return 1;
  }
  else if(*out_opt_type == BVLC_SC_OPTION_TYPE_PROPRIETARY) {
    uint16_t hdr_len;
    memcpy(&hdr_len, &in_options_list[1], sizeof(hdr_len));
    hdr_len += sizeof(hdr_len) + 1;
    if(in_options_list[0] & BVLC_SC_HEADER_MORE) {
      *out_next_option = in_options_list + hdr_len;
    }
    return hdr_len;
  }
  else {
    return (unsigned int) 0;
  }
}

/**
 * @brief Decodes BVLC header proprietary option
 *
 *
 * @param in_option - buffer contaning option data
 * @param in_option_len - length of option data buffer
 *
 * @return true if proprietary option was decoded succefully, otherwise returns false.
 *
 */

static bool bvlc_sc_decode_proprietary_option(
                 uint8_t   *in_options_list,
                 uint16_t   in_options_list_len,
                 uint16_t  *out_vendor_id,
                 uint8_t   *out_proprietary_option_type,
                 uint8_t  **out_proprietary_data,
                 uint16_t  *out_proprietary_data_len)
{
  uint16_t  len;
  uint16_t  hdr_len;

  if(!in_options_list ||
     !out_vendor_id ||
     !out_proprietary_option_type ||
     !out_proprietary_data ||
     !out_proprietary_data_len) {
    return false;
  }

  *out_proprietary_data = NULL;
  *out_proprietary_data_len = 0;
  memcpy(&hdr_len, &in_options_list[1], sizeof(hdr_len));
  memcpy(out_vendor_id, &in_options_list[3], sizeof(uint16_t));
  *out_proprietary_option_type =  in_options_list[5];

  if(hdr_len > 3) {
    *out_proprietary_data = &in_options_list[6];
    *out_proprietary_data_len = hdr_len - 3;
  }

  return true;
}

static unsigned int bvlc_sc_encode_common(
                         uint8_t                *pdu,
                         int                     pdu_len,
                         uint8_t                 bvlc_function,
                         uint16_t                message_id,
                         BACNET_SC_VMAC_ADDRESS *origin,
                         BACNET_SC_VMAC_ADDRESS *dest)
{
  unsigned int offs = 4;
  if(pdu_len < 4) {
    return 0;
  }
  pdu[0] = bvlc_function;
  pdu[1]= 0;
  memcpy(&pdu[2], &message_id, sizeof(message_id));

  if(origin) {
    if(pdu_len < offs + BVLC_SC_VMAC_SIZE) {
      return 0;
    }
    pdu[1] |= BVLC_SC_CONTROL_ORIG_VADDR;
    memcpy(&pdu[offs], &origin->address[0], BVLC_SC_VMAC_SIZE);
    offs += BVLC_SC_VMAC_SIZE;
  }

  if(dest) {
    if(pdu_len < offs + BVLC_SC_VMAC_SIZE) {
      return 0;
    }
    pdu[1] |= BVLC_SC_CONTROL_DEST_VADDR;
    memcpy(&pdu[offs], &dest->address[0], BVLC_SC_VMAC_SIZE);
    offs += BVLC_SC_VMAC_SIZE;
  }

  return offs;
}


/**
 * @brief bvlc_sc_encode_result
 *
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param result_code - BVLC result code
 *
 * @return number of bytes encoded, in a case of error returns 0.
 *
 * bvlc_function: 1-octet   check bvlc-sc.h for valid values
 * result_code:   1-octet   X'00' ACK: Successful completion.
                                  The 'Error Header Marker'
                                  and all subsequent parameters
                                  shall be absent.
 *                          X'01' NAK: The BVLC function failed.
                                  The 'Error Header Marker',
                                  the 'Error Class', the 'Error Code',
                                  and the 'Error Details' shall be present.
 */

unsigned int bvlc_sc_encode_result(
                  uint8_t                *pdu,
                  int                     pdu_len,
                  uint16_t                message_id,
                  BACNET_SC_VMAC_ADDRESS *origin,
                  BACNET_SC_VMAC_ADDRESS *dest,
                  uint8_t                 bvlc_function,
                  uint8_t                 result_code,
                  uint8_t                *error_header_marker,
                  uint16_t               *error_class,
                  uint16_t               *error_code,
                  uint8_t                *utf8_details_string )
{
  unsigned int offs;

  if(bvlc_function > (uint16_t) BVLC_SC_PROPRIETARY_MESSAGE) {
    /* unsupported bvlc function */
    return 0;
  }

  if(result_code != 0 && result_code != 1) {
    /* According AB.2.4.1 BVLC-Result Format result code
       must be 0x00 or 0x01 */
    return 0;
  }

  if(result_code) {
    if(!error_header_marker || !error_class || !error_code) {
       /* According AB.2.4.1 BVLC-Result Format error_class
          and error_code must be presented */
      return 0;
    }
  }

  offs = bvlc_sc_encode_common(pdu, pdu_len,
                               BVLC_SC_RESULT, message_id, origin, dest);

  if(!offs) {
    return 0;
  }

  if(pdu_len < offs + 2) {
    return 0;
  }

  pdu[offs++] = bvlc_function;
  pdu[offs++] = result_code;

  if(!result_code)
  {
    if(error_header_marker || error_class ||
       error_code || utf8_details_string) {
      return 0;
    }
    return offs;
  }

  if(pdu_len < offs + 4) {
    return 0;
  }

  pdu[offs++] = *error_header_marker;
  memcpy(&pdu[offs], error_class, sizeof(*error_class));
  offs += sizeof(*error_class);
  memcpy(&pdu[offs], error_code, sizeof(*error_code));
  offs += sizeof(*error_code);

  if(utf8_details_string) {
    if(pdu_len < offs + strlen((const char *)utf8_details_string)) {
      return 0;
    }
    memcpy(&pdu[offs],
            utf8_details_string,strlen((const char *)utf8_details_string));
    offs += strlen((const char *)utf8_details_string);
  }

  return offs;
}

static bool bvlc_sc_decode_result(BVLC_SC_DECODED_DATA *payload,
                                  uint8_t              *packed_payload,
                                  uint16_t              packed_payload_len,
                                  BACNET_ERROR_CODE    *error,
                                  BACNET_ERROR_CLASS   *class)
{
  int i;

  if(packed_payload_len < 2) {
    *error = ERROR_CODE_MESSAGE_INCOMPLETE;
    *class = ERROR_CLASS_COMMUNICATION;
    return false;
  }
  memset(&payload->result, 0, sizeof(payload->result));

  if(packed_payload[0] > (uint8_t) BVLC_SC_PROPRIETARY_MESSAGE) {
    /* unknown BVLC function */
    *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
    *class = ERROR_CLASS_COMMUNICATION;
    return false;
  }

  payload->result.bvlc_function = packed_payload[0];
  if(packed_payload[1] != 0 && packed_payload[1] != 1) {
    /* result code must be 1 or 0 */
    *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
    *class = ERROR_CLASS_COMMUNICATION;
    return false;
  }

  payload->result.result = packed_payload[1];

  if(packed_payload[1] == 1) {
    if(packed_payload_len < 7) {
      *error = ERROR_CODE_MESSAGE_INCOMPLETE;
      *class = ERROR_CLASS_COMMUNICATION;
      return false;
    }

    payload->result.error_header_marker = packed_payload[2];
    memcpy(&payload->result.error_class,
           &packed_payload[3], sizeof(payload->result.error_class));
    memcpy(&payload->result.error_code,
           &packed_payload[5], sizeof(payload->result.error_code));

    if(packed_payload_len > 7) {
      payload->result.utf8_details_string = &packed_payload[7];
      payload->result.utf8_details_string_len = packed_payload_len - 7;
      for( i = 0; i < packed_payload_len - 7; i++) {
        if(payload->result.utf8_details_string[i] == 0) {
           break;
        }
      }
      if(i != packed_payload_len - 7) {
        *error = ERROR_CODE_UNEXPECTED_DATA;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }
    }
    else if(packed_payload_len != 7) {
      *error = ERROR_CODE_UNEXPECTED_DATA;
      *class = ERROR_CLASS_COMMUNICATION;
      return false;
    }
  }
  else if(packed_payload_len != 2) {
    *error = ERROR_CODE_UNEXPECTED_DATA;
    *class = ERROR_CLASS_COMMUNICATION;
    return false;
  }
  return true;
}

unsigned int bvlc_sc_encode_encapsulated_npdu(
                  uint8_t                *pdu,
                  int                     pdu_len,
                  uint16_t                message_id,
                  BACNET_SC_VMAC_ADDRESS *origin,
                  BACNET_SC_VMAC_ADDRESS *dest,
                  uint8_t                *npdu,
                  uint16_t                npdu_size)
{
  uint16_t offs;

  offs = bvlc_sc_encode_common(pdu, pdu_len,
                               BVLC_SC_ENCAPSULATED_NPDU, message_id,
                               origin, dest);
  if(!offs) {
    return 0;
  }

  if(pdu_len < offs + npdu_size) {
    return 0;
  }

  memcpy(&pdu[offs], npdu, npdu_size);
  offs += npdu_size;
  return offs;
}

unsigned int bvlc_sc_encode_address_resolution(
                  uint8_t                *pdu,
                  int                     pdu_len,
                  uint16_t                message_id,
                  BACNET_SC_VMAC_ADDRESS *origin,
                  BACNET_SC_VMAC_ADDRESS *dest)
{
  uint16_t offs;

  offs = bvlc_sc_encode_common(pdu, pdu_len,
                               BVLC_SC_ADDRESS_RESOLUTION, message_id,
                               origin, dest);

  return offs;
}

unsigned int bvlc_sc_encode_address_resolution_ack(
                  uint8_t                *pdu,
                  int                     pdu_len,
                  uint16_t                message_id,
                  BACNET_SC_VMAC_ADDRESS *origin,
                  BACNET_SC_VMAC_ADDRESS *dest,
                  uint8_t                *web_socket_uris,
                  uint16_t                web_socket_uris_len)
{
  uint16_t offs;

  offs = bvlc_sc_encode_common(pdu, pdu_len,
                               BVLC_SC_ADDRESS_RESOLUTION_ACK, message_id,
                               origin, dest);
  if(!offs) {
     return 0;
  }

  if(web_socket_uris && web_socket_uris_len) {
    memcpy(&pdu[offs], web_socket_uris, web_socket_uris_len);
    offs += web_socket_uris_len;
  }

  return offs;
}

unsigned int bvlc_sc_encode_advertisiment(
                  uint8_t                           *pdu,
                  int                                pdu_len,
                  uint16_t                           message_id,
                  BACNET_SC_VMAC_ADDRESS            *origin,
                  BACNET_SC_VMAC_ADDRESS            *dest,
                  BVLC_SC_HUB_CONNECTION_STATUS      hub_status,
                  BVLC_SC_DIRECT_CONNECTION_SUPPORT  support,
                  uint16_t                           max_blvc_len,
                  uint16_t                           max_npdu_size) 
{
  uint16_t offs;

  offs = bvlc_sc_encode_common(pdu, pdu_len,
                               BVLC_SC_ADVERTISIMENT, message_id,
                               origin,dest);
  if(!offs) {
     return 0;
  }

  pdu[offs++] = (uint8_t) hub_status;
  pdu[offs++] = (uint8_t) support;
  memcpy(&pdu[offs], &max_blvc_len, sizeof(max_blvc_len));
  offs += sizeof(max_blvc_len);
  memcpy(&pdu[offs], &max_npdu_size, sizeof(max_npdu_size));
  offs += sizeof(max_npdu_size);
  return offs;
}

static bool bvlc_sc_decode_advertisiment(
                 BVLC_SC_DECODED_DATA *payload,
                 uint8_t              *packed_payload,
                 uint16_t              packed_payload_len,
                 BACNET_ERROR_CODE    *error,
                 BACNET_ERROR_CLASS   *class)
{
  if(packed_payload[0] > BVLC_SC_HUB_CONNECTION_FAILOVER_HUB_CONNECTED) {
    *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
    *class = ERROR_CLASS_COMMUNICATION;
    return false;
  }

  if(packed_payload[1] > BVLC_SC_DIRECT_CONNECTIONS_ACCEPT_SUPPORTED) {
    *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
    *class = ERROR_CLASS_COMMUNICATION;
    return false;
  }
  payload->advertisiment.hub_status = packed_payload[0];
  payload->advertisiment.support = packed_payload[1];
  memcpy(&payload->advertisiment.max_blvc_len,
         &packed_payload[2], sizeof(payload->advertisiment.max_blvc_len));
  memcpy(&payload->advertisiment.max_npdu_len,
         &packed_payload[4], sizeof(payload->advertisiment.max_npdu_len));
  return true;
}

unsigned int bvlc_sc_encode_advertisiment_solicitation(
                  uint8_t                *pdu,
                  int                     pdu_len,
                  uint16_t                message_id,
                  BACNET_SC_VMAC_ADDRESS *origin,
                  BACNET_SC_VMAC_ADDRESS *dest )
{
  uint16_t offs;

  offs = bvlc_sc_encode_common(pdu, pdu_len,
                               BVLC_SC_ADVERTISIMENT_SOLICITATION,
                               message_id, origin, dest);
  return offs;
}


unsigned int bvlc_sc_encode_connect_request(
                  uint8_t                *pdu,
                  int                     pdu_len,
                  uint16_t                message_id,
                  BACNET_SC_VMAC_ADDRESS *local_vmac,
                  BACNET_SC_UUID         *local_uuid,
                  uint16_t                max_blvc_len,
                  uint16_t                max_npdu_size) 
{
  uint16_t offs;

  if(!local_vmac || !local_uuid ) {
    return 0;
  }

  offs = bvlc_sc_encode_common(pdu, pdu_len,
                               BVLC_SC_CONNECT_REQUEST, message_id,
                               NULL, NULL);
  if(!offs) {
     return 0;
  }

  if(pdu_len < offs + BVLC_SC_VMAC_SIZE +
                   BVLC_SC_UUID_SIZE + 2 * sizeof(uint16_t) ) {
    return 0;
  }

  memcpy(&pdu[offs], local_vmac, BVLC_SC_VMAC_SIZE);
  offs += BVLC_SC_VMAC_SIZE;
  memcpy(&pdu[offs], local_uuid, BVLC_SC_UUID_SIZE);
  offs += BVLC_SC_UUID_SIZE;
  memcpy(&pdu[offs], &max_blvc_len, sizeof(max_blvc_len));
  offs += sizeof(max_blvc_len);
  memcpy(&pdu[offs], &max_npdu_size, sizeof(max_npdu_size));
  offs += sizeof(max_npdu_size);
  return offs;
}

static bool bvlc_sc_decode_connect_request(
                  BVLC_SC_DECODED_DATA *payload,
                  uint8_t              *packed_payload,
                  uint16_t              packed_payload_len,
                  BACNET_ERROR_CODE    *error,
                  BACNET_ERROR_CLASS   *class)
{
  if(packed_payload_len < 26) {
    *error = ERROR_CODE_MESSAGE_INCOMPLETE;
    *class = ERROR_CLASS_COMMUNICATION;
    return false;
  }
  else if(packed_payload_len > 26) {
    *error = ERROR_CODE_UNEXPECTED_DATA;
    *class = ERROR_CLASS_COMMUNICATION;
    return false;
  }
  payload->connect_request.local_vmac =
                   (BACNET_SC_VMAC_ADDRESS *) packed_payload;
  packed_payload += BVLC_SC_VMAC_SIZE;
  payload->connect_request.local_uuid = (BACNET_SC_UUID*) packed_payload;
  packed_payload += BVLC_SC_UUID_SIZE;
  memcpy(&payload->connect_request.max_blvc_len, packed_payload,
          sizeof(payload->connect_request.max_blvc_len));
  packed_payload += sizeof(payload->connect_request.max_blvc_len);
  memcpy(&payload->connect_request.max_npdu_len,
          packed_payload, sizeof(payload->connect_request.max_npdu_len));
  packed_payload += sizeof(payload->connect_request.max_npdu_len);
  return true;
}

unsigned int bvlc_sc_encode_connect_accept(
                  uint8_t                *pdu,
                  int                     pdu_len,
                  uint16_t                message_id,
                  BACNET_SC_VMAC_ADDRESS *local_vmac,
                  BACNET_SC_UUID         *local_uuid,
                  uint16_t                max_blvc_len,
                  uint16_t                max_npdu_len) 
{
  uint16_t offs;

  if(!local_vmac || !local_uuid ) {
    return 0;
  }

  offs = bvlc_sc_encode_common(pdu, pdu_len,
                               BVLC_SC_CONNECT_ACCEPT, message_id,
                               NULL, NULL);
  if(!offs) {
     return 0;
  }

  if(pdu_len < offs + BVLC_SC_VMAC_SIZE + 
                   BVLC_SC_UUID_SIZE + 2 * sizeof(uint16_t) ) {
    return 0;
  }

  memcpy(&pdu[offs], local_vmac, BVLC_SC_VMAC_SIZE);
  offs += BVLC_SC_VMAC_SIZE;
  memcpy(&pdu[offs], local_uuid, BVLC_SC_UUID_SIZE);
  offs += BVLC_SC_UUID_SIZE;
  memcpy(&pdu[offs], &max_blvc_len, sizeof(max_blvc_len));
  offs += sizeof(max_blvc_len);
  memcpy(&pdu[offs], &max_npdu_len, sizeof(max_npdu_len));
  offs += sizeof(max_npdu_len);
  return offs;
}

static bool bvlc_sc_decode_connect_accept(
                 BVLC_SC_DECODED_DATA *payload,
                 uint8_t              *packed_payload,
                 uint16_t              packed_payload_len,
                 BACNET_ERROR_CODE    *error,
                 BACNET_ERROR_CLASS   *class)
{
  if(packed_payload_len < 26) {
    *error = ERROR_CODE_MESSAGE_INCOMPLETE;
    *class = ERROR_CLASS_COMMUNICATION;
    return false;
  }
  else if(packed_payload_len > 26) {
    *error = ERROR_CODE_UNEXPECTED_DATA;
    *class = ERROR_CLASS_COMMUNICATION;
    return false;
  }

  payload->connect_accept.local_vmac =
                   (BACNET_SC_VMAC_ADDRESS *) packed_payload;
  packed_payload += BVLC_SC_VMAC_SIZE;
  payload->connect_accept.local_uuid = (BACNET_SC_UUID*) packed_payload;
  packed_payload += BVLC_SC_UUID_SIZE;
  memcpy(&payload->connect_accept.max_blvc_len,
          packed_payload, sizeof(payload->connect_accept.max_blvc_len));
  packed_payload += sizeof(payload->connect_accept.max_blvc_len);
  memcpy(&payload->connect_accept.max_npdu_len,
         packed_payload, sizeof(payload->connect_accept.max_npdu_len));
  packed_payload += sizeof(payload->connect_accept.max_npdu_len);
  return true;
}

unsigned int bvlc_sc_encode_disconnect_request(uint8_t *pdu,
                                               int      pdu_len,
                                               uint16_t message_id)
{
  uint16_t offs;

  offs = bvlc_sc_encode_common(pdu, pdu_len,
                               BVLC_SC_DISCONNECT_REQUEST, message_id,
                               NULL, NULL);
  return offs;
}

unsigned int bvlc_sc_encode_disconnect_ack(uint8_t *pdu,
                                           int      pdu_len,
                                           uint16_t message_id)
{
  uint16_t offs;

  offs = bvlc_sc_encode_common(pdu, pdu_len,
                               BVLC_SC_DISCONNECT_ACK, message_id,
                               NULL, NULL);
  return offs;
}

unsigned int bvlc_sc_encode_heartbeat_request(uint8_t *pdu,
                                              int      pdu_len,
                                              uint16_t message_id)
{
  uint16_t offs;

  offs = bvlc_sc_encode_common(pdu, pdu_len,
                               BVLC_SC_HEARTBEAT_REQUEST, message_id,
                               NULL, NULL);
  return offs;
}

unsigned int bvlc_sc_encode_heartbeat_ack(uint8_t *pdu,
                                          int      pdu_len,
                                          uint16_t message_id)
{
  uint16_t offs;

  offs = bvlc_sc_encode_common(pdu, pdu_len,
                               BVLC_SC_HEARTBEAT_ACK, message_id,
                               NULL, NULL);
  return offs;
}

unsigned int bvlc_sc_encode_proprietary_message(
                  uint8_t                 *pdu,
                  int                      pdu_len,
                  uint16_t                 message_id,
                  BACNET_SC_VMAC_ADDRESS  *origin,
                  BACNET_SC_VMAC_ADDRESS  *dest,
                  uint16_t                 vendor_id,
                  uint8_t                  proprietary_function,
                  uint8_t                 *proprietary_data,
                  uint16_t                 proprietary_data_len)
{
  uint16_t offs;

  offs = bvlc_sc_encode_common(pdu, pdu_len,
                               BVLC_SC_PROPRIETARY_MESSAGE, message_id,
                               origin, dest);
  if(!offs) {
     return 0;
  }

  if(pdu_len < offs + sizeof(vendor_id) + 
     sizeof(proprietary_function) + proprietary_data_len) {
    return 0;
  }

  memcpy(&pdu[offs], &vendor_id, sizeof(vendor_id));
  offs += sizeof(vendor_id);
  memcpy(&pdu[offs], &proprietary_function, sizeof(proprietary_function));
  offs += sizeof(proprietary_function);
  memcpy(&pdu[offs], proprietary_data, proprietary_data_len);
  offs += proprietary_data_len;
  return offs;
}

static bool bvlc_sc_decode_proprietary(
                 BVLC_SC_DECODED_DATA *payload,
                 uint8_t              *packed_payload,
                 uint16_t              packed_payload_len,
                 BACNET_ERROR_CODE    *error,
                 BACNET_ERROR_CLASS   *class)
{
  if(packed_payload_len < 3) {
    *error = ERROR_CODE_MESSAGE_INCOMPLETE;
    *class = ERROR_CLASS_COMMUNICATION;
    return false;
  }

  memcpy(&payload->proprietary.vendor_id,
          packed_payload, sizeof(payload->proprietary.vendor_id));
  packed_payload += sizeof(payload->proprietary.vendor_id);
  payload->proprietary.function = packed_payload[0];
  packed_payload++;
  if(packed_payload_len - 3 > 0) {
    payload->proprietary.data = packed_payload;
    payload->proprietary.data_len = packed_payload_len - 3;
  }
  else {
    payload->proprietary.data = NULL;
    payload->proprietary.data_len = 0;
  }
  return true;
}

static bool bvlc_sc_decode_hdr(uint8_t                 *message,
                               int                      message_len,
                               BVLC_SC_DECODED_HDR     *hdr,
                               BACNET_ERROR_CODE       *error,
                               BACNET_ERROR_CLASS      *class)
{
  int offs = 4;
  bool ret = false;
  uint16_t hdr_opt_len = 0;

  if(message_len < 4) {
    *error = ERROR_CODE_MESSAGE_INCOMPLETE;
    *class = ERROR_CLASS_COMMUNICATION;
    return ret;
  }

  if(message[0] > BVLC_SC_PROPRIETARY_MESSAGE) {
    *error = ERROR_CODE_BVLC_FUNCTION_UNKNOWN;
    *class = ERROR_CLASS_COMMUNICATION;
    return ret;
  }

  memset(hdr, 0, sizeof(*hdr));
  hdr->bvlc_function = message[0];
  memcpy(&hdr->message_id, &message[2], sizeof(hdr->message_id));

  if(message[1] & BVLC_SC_CONTROL_ORIG_VADDR) {
    hdr->origin = (BACNET_SC_VMAC_ADDRESS *) &message[offs];
    offs += BVLC_SC_VMAC_SIZE;
    if(offs > message_len) {
      *error = ERROR_CODE_MESSAGE_INCOMPLETE;
      *class = ERROR_CLASS_COMMUNICATION;
      return false;
    }
  }

  if(message[1] & BVLC_SC_CONTROL_DEST_VADDR) {
    hdr->dest = (BACNET_SC_VMAC_ADDRESS *) &message[offs];
    offs += BVLC_SC_VMAC_SIZE;
    if(offs > message_len) {
      *error = ERROR_CODE_MESSAGE_INCOMPLETE;
      *class = ERROR_CLASS_COMMUNICATION;
      return false;
    }
  }

  if(message[1] & BVLC_SC_CONTROL_DEST_OPTIONS) {
    ret = bvlc_sc_validate_options_headers(BACNET_PDU_DEST_OPTION_VALIDATION,
                                          &message[offs],
                                           message_len - offs,
                                          &hdr_opt_len,
                                          NULL,
                                          &hdr->dest_options_num,
                                          error,
                                          class);
    if(!ret) {
      return false;
    }
    hdr->dest_options = &message[offs];
    hdr->dest_options_len = hdr_opt_len;
    offs += hdr_opt_len;
    if(offs > message_len) {
      *error = ERROR_CODE_MESSAGE_INCOMPLETE;
      *class = ERROR_CLASS_COMMUNICATION;
      return false;
    }
  }

  if(message[1] & BVLC_SC_CONTROL_DATA_OPTIONS) {
    ret = bvlc_sc_validate_options_headers(BACNET_PDU_DATA_OPTION_VALIDATION,
                                          &message[offs],
                                           message_len - offs,
                                          &hdr_opt_len,
                                           NULL,
                                          &hdr->data_options_num,
                                          error,
                                          class);
    if(!ret) {
      return false;
    }
    hdr->data_options = &message[offs];
    hdr->data_options_len = hdr_opt_len;
    offs += hdr_opt_len;
    if(offs > message_len) {
      *error = ERROR_CODE_MESSAGE_INCOMPLETE;
      *class = ERROR_CLASS_COMMUNICATION;
      return false;
    }
  }

  if(message_len - offs <= 0) {
    // no payload
    return true;
  }

  hdr->payload = &message[offs];
  hdr->payload_len = message_len - offs;
  return true;
}

static bool bvlc_sc_decode_header_options(
                 BVLC_SC_DECODED_HDR_OPTION *option_array,
                 uint16_t                    option_array_length,
                 uint8_t                    *options_list,
                 uint16_t                    options_list_len)
{
  uint8_t* next_option = options_list;
  int option_len = 0;
  int ret;
  bool res;
  int i = 0;

  while(next_option) {

     ret = bvlc_sc_decode_option_hdr(options_list,
                                     options_list_len - (uint16_t)option_len,
                                     &option_array[i].type,
                                     &option_array[i].must_understand,
                                     &next_option);
     if(!ret) {
      return false;
     }

     option_array[i].packed_header_marker = options_list[0];

     if(option_array[i].type == BVLC_SC_OPTION_TYPE_PROPRIETARY) {
       res = bvlc_sc_decode_proprietary_option(
                  options_list,
                  options_list_len - (uint16_t)option_len,
                  &option_array[i].specific.proprietary.vendor_id,
                  &option_array[i].specific.proprietary.option_type,
                  &option_array[i].specific.proprietary.data,
                  &option_array[i].specific.proprietary.data_len );
       if(!res) {
        return false;
       }
     }

     option_len += ret;
     i++;
     options_list = next_option;
  }

  return true;
}

static bool bvlc_sc_decode_dest_options_if_exists(
                 BVLC_SC_DECODED_MESSAGE *message,
                 BACNET_ERROR_CODE       *error,
                 BACNET_ERROR_CLASS      *class)
{
  if(message->hdr.dest_options) {
    if(!bvlc_sc_decode_header_options(message->dest_options,
                                       BVLC_SC_HEADER_OPTION_MAX,
                                       message->hdr.dest_options,
                                       message->hdr.dest_options_len)) {
      *error = ERROR_CODE_HEADER_ENCODING_ERROR;
      *class = ERROR_CLASS_COMMUNICATION;
      return false;
    }
  }
  return true;
}

static bool bvlc_sc_decode_data_options_if_exists(
                 BVLC_SC_DECODED_MESSAGE *message,
                 BACNET_ERROR_CODE       *error,
                 BACNET_ERROR_CLASS      *class)
{
  if(message->hdr.data_options) {
    if(!bvlc_sc_decode_header_options(message->data_options,
                                       BVLC_SC_HEADER_OPTION_MAX,
                                       message->hdr.data_options,
                                       message->hdr.data_options_len)) {
      *error = ERROR_CODE_HEADER_ENCODING_ERROR;
      *class = ERROR_CLASS_COMMUNICATION;
      return false;
    }
  }
  return true;
}

bool bvlc_sc_decode_message(uint8_t                  *buf,
                             uint16_t                 buf_len,
                             BVLC_SC_DECODED_MESSAGE *message,
                             BACNET_ERROR_CODE       *error,
                             BACNET_ERROR_CLASS      *class)
{
  if(!message || !buf_len || !error || !class) {
    return false;
  }

  memset(message->data_options, 0, sizeof(message->data_options));
  memset(message->dest_options, 0, sizeof(message->dest_options));
  memset(&message->payload, 0, sizeof(message->payload));

  if(!bvlc_sc_decode_hdr(buf, buf_len, &message->hdr, error, class)) {
    return false;
  }

  if(message->hdr.dest_options) {
    if(message->hdr.dest_options_num > BVLC_SC_HEADER_OPTION_MAX) {
      /* dest header options list is too long */
      *error = ERROR_CODE_OUT_OF_MEMORY;
      *class = ERROR_CLASS_RESOURCES;
      return false;
    }
  }

  if(message->hdr.data_options) {
    if(message->hdr.data_options_num > BVLC_SC_HEADER_OPTION_MAX) {
      /* data header options list is too long */
      *error = ERROR_CODE_OUT_OF_MEMORY;
      *class = ERROR_CLASS_RESOURCES;
      return false;
    }
  }

  switch(message->hdr.bvlc_function) {
    case BVLC_SC_RESULT:
    {
        if(message->hdr.data_options) {
          /* The BVLC-Result message must not have data options */
          *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
          *class = ERROR_CLASS_COMMUNICATION;
          return false;
        }

        if(!message->hdr.payload || !message->hdr.payload_len) {
          *error = ERROR_CODE_PAYLOAD_EXPECTED;
          *class = ERROR_CLASS_COMMUNICATION;
          return false;
        }

        if(!bvlc_sc_decode_dest_options_if_exists(message, error, class)) {
          return false;
        }

        if(!bvlc_sc_decode_result(&message->payload,
                                   message->hdr.payload,
                                   message->hdr.payload_len,
                                   error,
                                   class )) {
          return false;
        }
        break;
    }
    case BVLC_SC_ENCAPSULATED_NPDU:
    {
      if(!message->hdr.payload || !message->hdr.payload_len) {
        *error = ERROR_CODE_MESSAGE_INCOMPLETE;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }

      if(!bvlc_sc_decode_dest_options_if_exists(message, error, class)) {
        return false;
      }

      if(!bvlc_sc_decode_data_options_if_exists(message, error, class)) {
        return false;
      }

      message->payload.encapsulated_npdu.npdu = message->hdr.payload;
      message->payload.encapsulated_npdu.npdu_len = message->hdr.payload_len;
      break;
    }
    case BVLC_SC_ADDRESS_RESOLUTION:
    {
      if(message->hdr.data_options) {
        /* The Address-Resolution message must not have data options */
        *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }

      if(message->hdr.payload || message->hdr.payload_len) {
        *error = ERROR_CODE_UNEXPECTED_DATA;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }

      if(!bvlc_sc_decode_dest_options_if_exists(message, error, class)) {
        return false;
      }
      break;
    }
    case BVLC_SC_ADDRESS_RESOLUTION_ACK:
    {
      if(message->hdr.data_options) {
        /* The Address-Resolution Ack message must not have data options */
        *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }
      if(!bvlc_sc_decode_dest_options_if_exists(message, error, class)) {
        return false;
      }
      if(message->hdr.payload_len) {
        message->payload.address_resolution_ack.utf8_websocket_uri_string =
                 message->hdr.payload;
      }
      else {
        message->payload.address_resolution_ack.utf8_websocket_uri_string =
                 NULL;
      }
      message->payload.address_resolution_ack.utf8_websocket_uri_string_len =
               message->hdr.payload_len;
      break;
    }
    case BVLC_SC_ADVERTISIMENT:
    {
      if(message->hdr.data_options) {
        /* The advertisiment message must not have data options */
        *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }

      if(!message->hdr.payload || !message->hdr.payload_len) {
        *error = ERROR_CODE_MESSAGE_INCOMPLETE;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }

      if(message->hdr.payload_len < 6) {
        *error = ERROR_CODE_PAYLOAD_EXPECTED;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }
      else if(message->hdr.payload_len > 6) {
        *error = ERROR_CODE_UNEXPECTED_DATA;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }

      if(!bvlc_sc_decode_dest_options_if_exists(message, error, class)) {
        return false;
      }

      if(!bvlc_sc_decode_advertisiment(&message->payload,
                                        message->hdr.payload,
                                        message->hdr.payload_len,
                                        error,
                                        class)) {
        return false;
      }
      break;
    }
    case BVLC_SC_ADVERTISIMENT_SOLICITATION:
    {
      if(message->hdr.data_options) {
        *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
        *class = ERROR_CLASS_COMMUNICATION;
        /* The advertisiment solicitation message must not have data options */
        return false;
      }

      if(message->hdr.payload || message->hdr.payload_len) {
        *error = ERROR_CODE_UNEXPECTED_DATA;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }

      if(!bvlc_sc_decode_dest_options_if_exists(message, error, class)) {
        return false;
      }
      break;
    }
    case BVLC_SC_CONNECT_REQUEST:
    case BVLC_SC_CONNECT_ACCEPT:
    case BVLC_SC_DISCONNECT_REQUEST:
    case BVLC_SC_DISCONNECT_ACK:
    case BVLC_SC_HEARTBEAT_REQUEST:
    case BVLC_SC_HEARTBEAT_ACK:
    {
      if(message->hdr.origin != 0) {
        *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }

      if(message->hdr.dest != 0) {
        *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }

      if(message->hdr.data_options) {
        *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }

      if(message->hdr.bvlc_function == BVLC_SC_CONNECT_REQUEST ||
         message->hdr.bvlc_function == BVLC_SC_CONNECT_ACCEPT) {
        if(!message->hdr.payload || !message->hdr.payload_len) {
          *error = ERROR_CODE_MESSAGE_INCOMPLETE;
          *class = ERROR_CLASS_COMMUNICATION;
          return false;
        }
      }
      else {
        if(message->hdr.payload || message->hdr.payload_len) {
          *error = ERROR_CODE_UNEXPECTED_DATA;
          *class = ERROR_CLASS_COMMUNICATION;
          return false;
        }
      }

      if(!bvlc_sc_decode_dest_options_if_exists(message, error, class)) {
        return false;
      }

      if(message->hdr.bvlc_function == BVLC_SC_CONNECT_REQUEST) {
        if(!bvlc_sc_decode_connect_request(&message->payload,
                                            message->hdr.payload,
                                            message->hdr.payload_len,
                                            error,
                                            class)) {
          return false;
        }
      }
      else if(message->hdr.bvlc_function == BVLC_SC_CONNECT_ACCEPT) {
        if(!bvlc_sc_decode_connect_accept(&message->payload,
                                           message->hdr.payload,
                                           message->hdr.payload_len,
                                           error,
                                           class)) {
          return false;
        }
      }
      break;
    }
    case BVLC_SC_PROPRIETARY_MESSAGE:
    {
      if(message->hdr.data_options) {
        /* The proprietary message must not have data options */
        *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }

      if(!message->hdr.payload || !message->hdr.payload_len) {
        *error = ERROR_CODE_MESSAGE_INCOMPLETE;
        *class = ERROR_CLASS_COMMUNICATION;
        return false;
      }

      if(!bvlc_sc_decode_dest_options_if_exists(message, error, class)) {
        return false;
      }

      if(!bvlc_sc_decode_proprietary(&message->payload,
                                      message->hdr.payload,
                                      message->hdr.payload_len,
                                      error,
                                      class)) {
        return false;
      }
      break;
    }
    default:
    {
      return false;
    }
  }
  return true;
}