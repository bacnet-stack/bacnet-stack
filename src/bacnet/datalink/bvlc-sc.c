#include "bacnet/datalink/bvlc-sc.h"

static bool bsc_bvlc_validate_options_headers(uint8_t   *option_headers,
                                              uint16_t   option_headers_max_len,
                                              uint16_t  *out_option_headers_real_length,
                                              uint8_t  **out_last_option_marker_ptr,
                                              uint16_t   *out_option_header_num )
{
  int options_len = 0;
  uint8_t flags = 0;
  uint8_t option;
  uint16_t hdr_len;
  
  if(!option_headers_max_len || !out_option_headers_real_length) {
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

    option = flags & BSL_BVLC_HEADER_OPTION_TYPE_MASK;

    if(option != BSC_BVLC_OPTION_TYPE_SECURE_PATH && 
       option != BSC_BVLC_OPTION_TYPE_PROPRIETARY ) {
      return false;
    }

    if(option == BSC_BVLC_OPTION_TYPE_SECURE_PATH) {
      if(flags & BSC_BVLC_HEADER_DATA) {
        /* securepath option header does not have data header according bacnet stadard */
        return false;
      }
      options_len++;
    }
    else if (option == BSC_BVLC_OPTION_TYPE_PROPRIETARY) {
      if(!(flags & BSC_BVLC_HEADER_DATA)) {
        /* proprietary option must have header data */
        return false;
      }
      options_len++;
      if(options_len + 2 > option_headers_max_len) {
        return false;
      }
      memcpy(&hdr_len, &option_headers[options_len], 2);
      options_len += 1 + /*header marker */ + 
                     2 /* header length */ + hdr_len /* length of data header */;
    }

    if(options_len > option_headers_max_len) {
      return false;
    }

    if(out_option_header_num) {
      if(*out_option_header_num < USHRT_MAX) {
        *out_option_header_num += 1;
      }
      else {
        if(flags & BSC_BVLC_HEADER_MORE) {
          return false;
        }
      }
    }

    if(!(flags & BSC_BVLC_HEADER_MORE)) {
      break;
    }
  } 

  *out_option_headers_real_length = options_len;
  return true;
}

static uint16_t bsc_bvlc_add_sc_option(bool to_data_option,
                                       uint8_t* outbuf,
                                       uint16_t outbuf_len,
                                       uint8_t* blvc_message,
                                       uint16_t blvc_message_len,
                                       uint8_t* sc_option,
                                       uint16_t sc_option_len)
{
  uint16_t offs = 4;
  uint8_t flags = 0;
  bool found_end = false;
  uint16_t options_len = 0;
  uint8_t* last_option_marker_ptr = NULL;
  uint8_t mask;

  if(!blvc_message_len || !sc_option_len || !outbuf_len ||
     !outbuf || !blvc_message || !sc_option ) {
    return 0;
  }

  if(outbuf_len < 4 || blvc_message_len < 4) {
    return 0;
  }

  if(((int)(sc_option_len)) + ((int)(blvc_message_len)) > USHRT_MAX) {
    return 0;
  }

  if(outbuf_len < blvc_message_len) {
    return 0;
  }

  if((((int)outbuf_len)) < ((int)(sc_option_len)) + ((int)(blvc_message_len))) {
    return 0;
  }

  /* ensure that sc_option does not have flag more options */

  if(sc_option[0] & BSC_BVLC_HEADER_MORE) {
    return 0;
  }

  /* ensure that user wants to add valid option */

  if(!bsc_bvlc_validate_options_headers(sc_option, sc_option_len, &options_len, NULL, NULL)) {
    return 0;
  }

  flags = blvc_message[1];

  if(flags & BSC_BVLC_CONTROL_DEST_VADDR) {
    offs += BSC_VMAC_SIZE;
  }

  if(flags & BSC_BVLC_CONTROL_ORIG_VADDR) {
    offs += BSC_VMAC_SIZE;
  }

  if(offs >= blvc_message_len) {
    return 0;
  }

  /* insert options */

  if(to_data_option) {
    mask = BSC_BVLC_CONTROL_DATA_OPTIONS;
    if(flags & BSC_BVLC_CONTROL_DEST_OPTIONS) {
      /* some options are already presented in message. Validate them at first. */
      if(!bsc_bvlc_validate_options_headers(&blvc_message[offs], 
                                             blvc_message_len - offs, 
                                            &options_len,
                                            &last_option_marker_ptr,
                                            NULL)) {
        return 0;
      }
      offs += options_len;
    }
  }
  else {
    mask = BSC_BVLC_CONTROL_DEST_OPTIONS;
  }

  if(!(flags & mask)) {
    /* first option addded to message */
    memmove(&outbuf[0], &blvc_message[0], offs);
    outbuf[1] |= mask;
    memmove(&outbuf[offs], sc_option, sc_option_len);
    memmove(&outbuf[offs + sc_option_len], &blvc_message[offs], blvc_message_len - offs);
  }
  else {
    /* some options are already presented in message. Validate them at first. */
    if(!bsc_bvlc_validate_options_headers(&blvc_message[offs], 
                                           blvc_message_len - offs, 
                                          &options_len,
                                          &last_option_marker_ptr,
                                           NULL)) {
      return 0;
    }
    memmove(&outbuf[0], &blvc_message[0], offs);
    *last_option_marker_ptr |= BSC_BVLC_HEADER_MORE;
    memmove(&outbuf[offs], &blvc_message[offs], options_len);
    offs += options_len;
    memmove(&outbuf[offs], sc_option, sc_option_len);
    memmove(&outbuf[offs + sc_option_len], &blvc_message[offs], blvc_message_len - offs);
  }
  return blvc_message_len + sc_option_len;
}


uint16_t bsc_bvlc_add_sc_option_to_destination_options(uint8_t* outbuf,
                                                       uint16_t outbuf_len,
                                                       uint8_t* blvc_message,
                                                       uint16_t blvc_message_len,
                                                       uint8_t* sc_option,
                                                       uint16_t sc_option_len)
{
  return bsc_bvlc_add_sc_option(false, outbuf, outbuf_len, blvc_message,
                                blvc_message_len, sc_option, sc_option_len);
}

uint16_t bsc_bvlc_add_sc_option_to_data_options(uint8_t* outbuf,
                                                uint16_t outbuf_len,
                                                uint8_t* blvc_message,
                                                uint16_t blvc_message_len,
                                                uint8_t* sc_option,
                                                uint16_t sc_option_len)
{
  return bsc_bvlc_add_sc_option(true, outbuf, outbuf_len, blvc_message,
                                blvc_message_len, sc_option, sc_option_len);
}
// returns length of created option

uint16_t bsc_bvlc_encode_proprietary_option(uint8_t* outbuf,
                                            uint16_t outbuf_len,
                                            bool     must_understand,
                                            uint16_t vendor_id,
                                            uint8_t  proprietary_option_type,
                                            uint8_t* proprietary_data,
                                            uint16_t proprietary_data_len)
{
  uint16_t total_len = sizeof(vendor_id) + proprietary_data_len + sizeof(uint8_t);

  if(proprietary_data_len > BSC_MPDU_MAX - 3/* header marker len + header length len) */ - sizeof(vendor_id)) {
    return 0;
  }

  if(outbuf_len < total_len + 3 /* header marker len + header data len) */) {
    return 0;
  }

  /* reset More Options, Must Understand and Header Data flags
     thay will be updated as a result of call bsc_bvlc_add_sc_option() */

  outbuf[0] = BSC_BVLC_OPTION_TYPE_PROPRIETARY;

  if(must_understand) {
    outbuf[0] |= BSC_BVLC_HEADER_MUST_UNDERSTAND;
  }

  outbuf[0] |= BSC_BVLC_HEADER_DATA;
  memcpy(&outbuf[1], &total_len, sizeof(total_len));
  memcpy(&outbuf[3], &vendor_id, sizeof(vendor_id));
  memcpy(&outbuf[5], &proprietary_option_type, sizeof(proprietary_option_type));
  memcpy(&outbuf[6], proprietary_data, proprietary_data_len);
  return total_len + 3;
}

// returns length of created option
uint16_t bsc_bvlc_encode_secure_path_option(uint8_t* outbuf,
                                            uint16_t outbuf_len,
                                            bool     must_understand)
{
  if(outbuf_len < 1) {
    return 0;
  }

  outbuf[0] = BSC_BVLC_OPTION_TYPE_SECURE_PATH;

  if(must_understand) {
    outbuf[0] |= BSC_BVLC_HEADER_MUST_UNDERSTAND;
  }
  return 1;
}

/**
 * @brief Decodes BVLC header options marker. Function assumes to be called iteratively
 *        until end of option list will be reached. User must call this function on
 *        previously validated options list only because sanity checks are omitted.
 *        That valided option list can be get by some bsc_bvlc_encode_ function calls.
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
 * @return negative value in a case if decoding failed, otherwise returns
 *         length value of decoded option > 0 in bytes.
 *
 * Typically code for parsing of option list looks like following:
 *
 *  uint8_t *current = in_options_list;
 *  int option_len = 0;
 *  while(current) {
 *    option_len = bsc_bvlc_decode_option_hdr(current, &in_options_list_len, &type, ...., &current);
 *    if(option_len < 0) {
        // handle error
        break;
      }
 *    in_options_list_len -= option_len;
 *  }
 *
 */

static int bsc_bvlc_decode_option_hdr(uint8_t                     *in_options_list,
                                      uint16_t                     in_option_list_len,
                                      bsc_bvlc_hdr_option_type_t  *out_opt_type,
                                      bool                        *out_must_understand,
                                      uint8_t                    **out_next_option)
{
  uint16_t len;

  *out_next_option = NULL;
  *out_opt_type = (bsc_bvlc_hdr_option_type_t) (in_options_list[0] & BSL_BVLC_HEADER_OPTION_TYPE_MASK);
  *out_must_understand = (in_options_list[0] & BSC_BVLC_HEADER_MUST_UNDERSTAND) 
                          ? true : false;

  if(*out_opt_type == BSC_BVLC_OPTION_TYPE_SECURE_PATH) {
    if(in_options_list[0] & BSC_BVLC_HEADER_MORE) {
      *out_next_option = in_options_list + 1;
    }
    return 1;
  }
  else if(*out_opt_type == BSC_BVLC_OPTION_TYPE_PROPRIETARY) {
    uint16_t hdr_len;
    memcpy(&hdr_len, &in_options_list[1], sizeof(hdr_len));
    hdr_len += sizeof(hdr_len) + 1;
    if(in_options_list[0] & BSC_BVLC_HEADER_MORE) {
      *out_next_option = in_options_list + hdr_len;
    }
    return hdr_len;
  }
  else {
    return -1;
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

static bool bsc_bvlc_decode_proprietary_option(uint8_t   *in_options_list,
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

  if(!bsc_bvlc_validate_options_headers(in_options_list, 
                                        in_options_list_len, &len, NULL, NULL)) {
    return false;
  }

  *out_proprietary_data = NULL;
  *out_proprietary_data_len = 0;
  memcpy(&hdr_len, &in_options_list[1], sizeof(hdr_len));
  memcpy(&out_vendor_id, &in_options_list[3], sizeof(uint16_t));
  *out_proprietary_option_type =  in_options_list[5];

  if(hdr_len > 3) {
    *out_proprietary_data = &in_options_list[6];
    *out_proprietary_data_len = hdr_len - 3;
  }

  return true;
}

static unsigned int bsc_bvlc_encode_common(uint8_t        *out_buf,
                                           int             out_buf_len,
                                           uint8_t         bvlc_function,
                                           uint16_t        message_id,
                                           BACNET_ADDRESS *origin,
                                           BACNET_ADDRESS *dest)
{
  unsigned int offs = 4;
  if(out_buf_len < 4) {
    return 0;
  }
  out_buf[0] = bvlc_function;
  out_buf[1]= 0;
  memcpy(&out_buf[2], &message_id, sizeof(message_id));

  if(origin) {
    if(out_buf_len < offs + BSC_VMAC_SIZE) {
      return 0;
    }
    out_buf[1] |= BSC_BVLC_CONTROL_ORIG_VADDR;
    memcpy(&out_buf[offs], &origin->mac[0], BSC_VMAC_SIZE);
    offs += BSC_VMAC_SIZE;
  }

  if(dest) {
    if(out_buf_len < offs + BSC_VMAC_SIZE) {
      return 0;
    }
    out_buf[1] |= BSC_BVLC_CONTROL_DEST_VADDR;
    memcpy(&out_buf[offs], &dest->mac[0], BSC_VMAC_SIZE);
    offs += BSC_VMAC_SIZE;
  }

  return offs;
}


/**
 * @brief bsc_bvlc_encode_result
 *
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param result_code - BVLC result code
 *
 * @return number of bytes encoded, in a case of error returns 0.
 *
 * bvlc_function: 1-octet   check bvlc-sc.h for valid values
 * result_code:   1-octet   X'00' ACK: Successful completion. The 'Error Header Marker' 
                                  and all subsequent parameters shall be absent.
 *                          X'01' NAK: The BVLC function failed. The 'Error Header Marker',
                                  the 'Error Class', the 'Error Code', and the 'Error Details' shall be present.
 */

unsigned int bsc_bvlc_encode_result(uint8_t        *out_buf,
                                    int             out_buf_len,
                                    uint16_t        message_id,
                                    BACNET_ADDRESS *origin,
                                    BACNET_ADDRESS *dest,
                                    uint8_t         bvlc_function,
                                    uint8_t         result_code,
                                    uint8_t        *error_header_marker,
                                    uint16_t       *error_class,
                                    uint16_t       *error_code,
                                    uint8_t        *utf8_details_string )
{
  unsigned int offs;

  if(bvlc_function > (uint16_t) BSC_BVLC_PROPRIETARY_MESSAGE) {
    /* unsupported bvlc function */
    return 0;
  }

  if(result_code != 0 && result_code != 1) {
    /* According AB.2.4.1 BVLC-Result Format result code must be 0x00 or 0x01 */
    return 0;
  }

  if(result_code) {
    if(!error_header_marker || *error_header_marker > (uint8_t) BSC_BVLC_PROPRIETARY_MESSAGE) {
      /* unsupported bvlc function */
      return 0;
    }
    if(!error_class || !error_code) {
       /* According AB.2.4.1 BVLC-Result Format error_class and error_code must be presented */
      return 0;
    }
  }

  offs = bsc_bvlc_encode_common(out_buf, out_buf_len, BSC_BVLC_RESULT, message_id, origin, dest);

  if(!offs) {
    return 0;
  }

  if(out_buf_len < offs + 2) {
    return 0;
  }

  out_buf[offs++] = bvlc_function;
  out_buf[offs++] = result_code;

  if(!result_code)
    return offs;

  if(out_buf_len < offs + 4) {
    return 0;
  }

  out_buf[offs++] = *error_header_marker;
  memcpy(&out_buf[offs], error_class, sizeof(*error_class));
  offs += sizeof(*error_class);
  memcpy(&out_buf[offs], error_class, sizeof(*error_class));
  offs += sizeof(*error_class);

  if(utf8_details_string) {
    if(out_buf_len < offs + strlen((const char *)utf8_details_string)) {
      return 0;
    }
    memcpy(&out_buf[offs], utf8_details_string,strlen((const char *)utf8_details_string));
    offs += strlen((const char *)utf8_details_string);
  }

  return offs;
}

static bool bsc_bvlc_decode_result(bsc_bvlc_unpacked_data_t *payload,
                                   uint8_t                  *packed_payload,
                                   uint16_t                  packed_payload_len)
{
  if(packed_payload_len < 2) {
    return false;
  }
  memset(&payload->result, 0, sizeof(payload->result));

  if(packed_payload[0] > (uint8_t) BSC_BVLC_PROPRIETARY_MESSAGE) {
    /* unknown BVLC function */
    return false;
  }

  payload->result.bvlc_function = packed_payload[0];
  payload->result.result = packed_payload[1];
  if(!packed_payload[1]) {
    return false;
  }

  if(packed_payload[1] != 1) {
    /* result code must be 1 or 0 */
    return false;
  }

  payload->result.error_header_marker = packed_payload[2];
  memcpy(&payload->result.error_class, &packed_payload[3], sizeof(payload->result.error_class));
  memcpy(&payload->result.error_code, &packed_payload[5], sizeof(payload->result.error_code));

  if(packed_payload_len > 7) {
    payload->result.utf8_details_string = &packed_payload[7];
    payload->result.utf8_details_string_len = packed_payload_len - 7;
  }

  return true;
}

unsigned int bsc_bvlc_encode_encapsulated_npdu(uint8_t        *out_buf,
                                               int             out_buf_len,
                                               uint16_t        message_id,
                                               BACNET_ADDRESS *origin,
                                               BACNET_ADDRESS *dest,
                                               uint8_t        *npdu,
                                               uint16_t        npdu_len)
{
  uint16_t offs;

  offs = bsc_bvlc_encode_common(out_buf, out_buf_len,
                                BSC_BVLC_ENCAPSULATED_NPDU, message_id, origin, dest);
  if(!offs) {
    return 0;
  }

  if(out_buf_len < offs + npdu_len) {
    return 0;
  }

  memcpy(&out_buf[offs], npdu, npdu_len);
  offs += npdu_len;
  return offs;
}

unsigned int bsc_bvlc_encode_address_resolution(uint8_t        *out_buf,
                                               int             out_buf_len,
                                               uint16_t        message_id,
                                               BACNET_ADDRESS *origin,
                                               BACNET_ADDRESS *dest)
{
  uint16_t offs;

  offs = bsc_bvlc_encode_common(out_buf, out_buf_len,
                                BSC_BVLC_ADDRESS_RESOLUTION, message_id, origin, dest);

  return offs;
}

unsigned int bsc_bvlc_encode_address_resolution_ack(uint8_t        *out_buf,
                                                    int             out_buf_len,
                                                    uint16_t        message_id,
                                                    BACNET_ADDRESS *origin,
                                                    BACNET_ADDRESS *dest,
                                                    uint8_t        *web_socket_uris,
                                                    uint16_t        web_socket_uris_len)
{
  uint16_t offs;

  offs = bsc_bvlc_encode_common(out_buf, out_buf_len,
                                BSC_BVLC_ADDRESS_RESOLUTION_ACK, message_id, origin, dest);
  if(!offs) {
     return 0;
  }

  if(web_socket_uris && web_socket_uris_len) {
    memcpy(&out_buf[offs], web_socket_uris, web_socket_uris_len);
    offs += web_socket_uris_len;
  }

  return offs;
}

unsigned int bsc_bvlc_encode_advertisiment(uint8_t                              *out_buf,
                                           int                                   out_buf_len,
                                           uint16_t                              message_id,
                                           BACNET_ADDRESS                       *origin,
                                           BACNET_ADDRESS                       *dest,
                                           bsc_bvlc_hub_connection_status_t      hub_status,
                                           bsc_bvlc_direct_connection_support_t  support,
                                           uint16_t                              max_blvc_len,
                                           uint16_t                              max_npdu_len) 
{
  uint16_t offs;

  offs = bsc_bvlc_encode_common(out_buf, out_buf_len,
                                BSC_BVLC_ADVERTISIMENT, message_id, origin, dest);
  if(!offs) {
     return 0;
  }

  out_buf[offs++] = (uint8_t) hub_status;
  out_buf[offs++] = (uint8_t) support;
  memcpy(&out_buf[offs], &max_blvc_len, sizeof(max_blvc_len));
  offs += sizeof(max_blvc_len);
  memcpy(&out_buf[offs], &max_npdu_len, sizeof(max_npdu_len));
  offs += sizeof(max_npdu_len);
  return offs;
}

static bool bsc_bvlc_decode_advertisiment(bsc_bvlc_unpacked_data_t *payload,
                                          uint8_t                  *packed_payload,
                                          uint16_t                  packed_payload_len)
{
  if(packed_payload[0] > BSC_BVLC_HUB_CONNECTION_FAILOVER_HUB_CONNECTED) {
    return false;
  }

  if(packed_payload[1] > BSC_BVLC_DIRECT_CONNECTIONS_ACCEPT_SUPPORTED) {
    return false;
  }
  payload->advertisiment.hub_status = packed_payload[0];
  payload->advertisiment.support = packed_payload[1];
  memcpy(&payload->advertisiment.max_blvc_len, &packed_payload[2], sizeof(payload->advertisiment.max_blvc_len));
  memcpy(&payload->advertisiment.max_npdu_len, &packed_payload[4], sizeof(payload->advertisiment.max_npdu_len));
  return true;
}

unsigned int bsc_bvlc_encode_advertisiment_solicitation(uint8_t        *out_buf,
                                                        int             out_buf_len,
                                                        uint16_t        message_id,
                                                        BACNET_ADDRESS *origin,
                                                        BACNET_ADDRESS *dest )
{
  uint16_t offs;

  offs = bsc_bvlc_encode_common(out_buf, out_buf_len,
                                BSC_BVLC_ADVERTISIMENT_SOLICITATION, message_id, origin, dest);
  return offs;
}


unsigned int bsc_bvlc_encode_connect_request(uint8_t        *out_buf,
                                             int             out_buf_len,
                                             uint16_t        message_id,
                                             uint8_t         (*local_vmac)[BSC_VMAC_SIZE],
                                             uint8_t         (*local_uuid)[BSC_DEVICE_UUID_SIZE],
                                             uint16_t        max_blvc_len,
                                             uint16_t        max_npdu_len) 
{
  uint16_t offs;

  if(!local_vmac || !local_uuid ) {
    return 0;
  }

  offs = bsc_bvlc_encode_common(out_buf, out_buf_len,
                                BSC_BVLC_CONNECT_REQUEST, message_id, NULL, NULL);
  if(!offs) {
     return 0;
  }

  if(out_buf_len < offs + BSC_VMAC_SIZE + BSC_DEVICE_UUID_SIZE + 2 * sizeof(uint16_t) ) {
    return 0;
  }

  memcpy(&out_buf[offs], local_vmac, BSC_VMAC_SIZE);
  offs += BSC_VMAC_SIZE;
  memcpy(&out_buf[offs], local_uuid, BSC_DEVICE_UUID_SIZE);
  offs += BSC_DEVICE_UUID_SIZE;
  memcpy(&out_buf[offs], &max_blvc_len, sizeof(max_blvc_len));
  offs += sizeof(max_blvc_len);
  memcpy(&out_buf[offs], &max_npdu_len, sizeof(max_npdu_len));
  offs += sizeof(max_npdu_len);
  return offs;
}

static bool bsc_bvlc_decode_connect_request(bsc_bvlc_unpacked_data_t *payload,
                                            uint8_t                  *packed_payload,
                                            uint16_t                  packed_payload_len)
{
  if(packed_payload_len != 26) {
    return false;
  }

  memcpy(&payload->connect_request.local_vmac[0], packed_payload, sizeof(payload->connect_request.local_vmac));
  packed_payload += sizeof(payload->connect_request.local_vmac);
  memcpy(&payload->connect_request.local_uuid[0], packed_payload, sizeof(payload->connect_request.local_uuid));
  packed_payload += sizeof(payload->connect_request.local_uuid);
  memcpy(&payload->connect_request.max_blvc_len, packed_payload, sizeof(payload->connect_request.max_blvc_len));
  packed_payload += sizeof(payload->connect_request.max_blvc_len);
  memcpy(&payload->connect_request.max_npdu_len, packed_payload, sizeof(payload->connect_request.max_npdu_len));
  packed_payload += sizeof(payload->connect_request.max_npdu_len);
  return true;
}

unsigned int bsc_bvlc_encode_connect_accept(uint8_t        *out_buf,
                                            int             out_buf_len,
                                            uint16_t        message_id,
                                            uint8_t         (*local_vmac)[BSC_VMAC_SIZE],
                                            uint8_t         (*local_uuid)[BSC_DEVICE_UUID_SIZE],
                                            uint16_t        max_blvc_len,
                                            uint16_t        max_npdu_len) 
{
  uint16_t offs;

  if(!local_vmac || !local_uuid ) {
    return 0;
  }

  offs = bsc_bvlc_encode_common(out_buf, out_buf_len,
                                BSC_BVLC_CONNECT_ACCEPT, message_id, NULL, NULL);
  if(!offs) {
     return 0;
  }

  if(out_buf_len < offs + BSC_VMAC_SIZE + BSC_DEVICE_UUID_SIZE + 2 * sizeof(uint16_t) ) {
    return 0;
  }

  memcpy(&out_buf[offs], local_vmac, BSC_VMAC_SIZE);
  offs += BSC_VMAC_SIZE;
  memcpy(&out_buf[offs], local_uuid, BSC_DEVICE_UUID_SIZE);
  offs += BSC_DEVICE_UUID_SIZE;
  memcpy(&out_buf[offs], &max_blvc_len, sizeof(max_blvc_len));
  offs += sizeof(max_blvc_len);
  memcpy(&out_buf[offs], &max_npdu_len, sizeof(max_npdu_len));
  offs += sizeof(max_npdu_len);
  return offs;
}

static bool bsc_bvlc_decode_connect_accept(bsc_bvlc_unpacked_data_t *payload,
                                          uint8_t                  *packed_payload,
                                          uint16_t                  packed_payload_len)
{
  if(packed_payload_len != 26) {
    return false;
  }

  memcpy(&payload->connect_accept.local_vmac[0], packed_payload, sizeof(payload->connect_accept.local_vmac));
  packed_payload += sizeof(payload->connect_accept.local_vmac);
  memcpy(&payload->connect_accept.local_uuid[0], packed_payload, sizeof(payload->connect_accept.local_uuid));
  packed_payload += sizeof(payload->connect_accept.local_uuid);
  memcpy(&payload->connect_accept.max_blvc_len, packed_payload, sizeof(payload->connect_accept.max_blvc_len));
  packed_payload += sizeof(payload->connect_accept.max_blvc_len);
  memcpy(&payload->connect_accept.max_npdu_len, packed_payload, sizeof(payload->connect_accept.max_npdu_len));
  packed_payload += sizeof(payload->connect_accept.max_npdu_len);
  return true;
}

unsigned int bsc_bvlc_encode_disconnect_request(uint8_t *out_buf,
                                                int      out_buf_len,
                                                uint16_t message_id)
{
  uint16_t offs;

  offs = bsc_bvlc_encode_common(out_buf, out_buf_len,
                                BSC_BVLC_DISCONNECT_REQUEST, message_id, NULL, NULL);
  return offs;
}

unsigned int bsc_bvlc_encode_disconnect_ack(uint8_t *out_buf,
                                            int      out_buf_len,
                                            uint16_t message_id)
{
  uint16_t offs;

  offs = bsc_bvlc_encode_common(out_buf, out_buf_len,
                                BSC_BVLC_DISCONNECT_ACK, message_id, NULL, NULL);
  return offs;
}

unsigned int bsc_bvlc_encode_heartbeat_request(uint8_t *out_buf,
                                               int      out_buf_len,
                                               uint16_t message_id)
{
  uint16_t offs;

  offs = bsc_bvlc_encode_common(out_buf, out_buf_len,
                                BSC_BVLC_HEARTBEAT_REQUEST, message_id, NULL, NULL);
  return offs;
}

unsigned int bsc_bvlc_encode_heartbeat_ack(uint8_t *out_buf,
                                           int      out_buf_len,
                                           uint16_t message_id)
{
  uint16_t offs;

  offs = bsc_bvlc_encode_common(out_buf, out_buf_len,
                                BSC_BVLC_HEARTBEAT_ACK, message_id, NULL, NULL);
  return offs;
}

unsigned int bsc_bvlc_encode_proprietary_message(uint8_t         *out_buf,
                                                 int              out_buf_len,
                                                 uint16_t         message_id,
                                                 BACNET_ADDRESS  *origin,
                                                 BACNET_ADDRESS  *dest,
                                                 uint16_t         vendor_id,
                                                 uint8_t          proprietary_function,
                                                 uint8_t         *proprietary_data,
                                                 uint16_t         proprietary_data_len)
{
  uint16_t offs;

  offs = bsc_bvlc_encode_common(out_buf, out_buf_len,
                                BSC_BVLC_PROPRIETARY_MESSAGE, message_id, origin, dest);
  if(!offs) {
     return 0;
  }

  if(out_buf_len < offs + sizeof(vendor_id) + sizeof(proprietary_function) + proprietary_data_len) {
    return 0;
  }

  memcpy(&out_buf[offs], &vendor_id, sizeof(vendor_id));
  offs += sizeof(vendor_id);
  memcpy(&out_buf[offs], &proprietary_function, sizeof(proprietary_function));
  offs += sizeof(proprietary_function);
  memcpy(&out_buf[offs], proprietary_data, proprietary_data_len);
  offs += proprietary_data_len;
  return offs;
}

static bool bsc_bvlc_decode_proprietary(bsc_bvlc_unpacked_data_t *payload,
                                        uint8_t                  *packed_payload,
                                        uint16_t                  packed_payload_len)
{
  if(packed_payload_len < 3) {
    return false;
  }
  memcpy(&payload->proprietary.vendor_id, packed_payload, sizeof(payload->proprietary.vendor_id));
  packed_payload += sizeof(payload->proprietary.vendor_id);
  payload->proprietary.proprietary_function = packed_payload[0];
  packed_payload++;
  if(packed_payload_len - 3 > 0) {
    payload->proprietary.proprietary_data = packed_payload;
    payload->proprietary.proprietary_data_len = packed_payload_len - 3;
  }
  else {
    payload->proprietary.proprietary_data = NULL;
    payload->proprietary.proprietary_data_len = 0;
  }
  return true;
}

static bool bsc_bvlc_decode_hdr(uint8_t                 *message,
                                int                      message_len,
                                bsc_bvlc_unpacked_hdr_t *hdr)
{
  int offs = 4;
  bool ret = false;
  uint16_t hdr_opt_len = 0;

  if(!hdr) {
    return ret;
  }

  if(message_len < 4) {
    return ret;
  }

  if(message[0] > BSC_BVLC_PROPRIETARY_MESSAGE) {
    return ret;
  }

  memset(hdr, 0, sizeof(*hdr));
  hdr->bvlc_function = message[0];
  memcpy(&hdr->message_id, &message[2], sizeof(hdr->message_id));

  if(message[1] & BSC_BVLC_CONTROL_ORIG_VADDR) {
    if(offs >= message_len) {
      return false;
    }
    hdr->origin.mac_len = BSC_VMAC_SIZE;
    memcpy(hdr->origin.mac, &message[offs], BSC_VMAC_SIZE);
    offs += BSC_VMAC_SIZE;
  }

  if(message[1] & BSC_BVLC_CONTROL_DEST_VADDR) {
    if(offs >= message_len) {
      return false;
    }
    hdr->dest.mac_len = BSC_VMAC_SIZE;
    memcpy(hdr->dest.mac, &message[offs], BSC_VMAC_SIZE);
    offs += BSC_VMAC_SIZE;
  }

  if(message[1] & BSC_BVLC_CONTROL_DATA_OPTIONS) {
    if(offs >= message_len) {
      return false;
    }
    ret = bsc_bvlc_validate_options_headers(&message[offs],
                                             message_len - offs,
                                            &hdr_opt_len,
                                             NULL,
                                            &hdr->data_options_num);
    if(!ret) {
      return false;
    }
    hdr->data_options = &message[offs];
    hdr->data_options_len = hdr_opt_len;
    offs += hdr_opt_len;
  }

  if(message[1] & BSC_BVLC_CONTROL_DEST_OPTIONS) {
    if(offs >= message_len) {
      return false;
    }
    ret = bsc_bvlc_validate_options_headers(&message[offs],
                                             message_len - offs,
                                            &hdr_opt_len,
                                            NULL,
                                            &hdr->dest_options_num);
    if(!ret) {
      return false;
    }
    hdr->dest_options = &message[offs];
    hdr->dest_options_len = hdr_opt_len;
    offs += hdr_opt_len;
  }

  if(message_len - offs <= 0) {
    // no payload
    return true;
  }

  hdr->payload = &message[offs];
  hdr->payload_len = message_len - offs;
  return true;
}

static bool bsc_bvlc_decode_header_options(bsc_blvc_unpacked_hdr_option_t *option_array,
                                           uint16_t                        option_array_length,
                                           uint8_t                        *options_list,
                                           uint16_t                        options_list_len)
{
  uint8_t* next_option = options_list;
  int option_len = 0;
  int ret;
  bool res;
  int i = 0;

  while(next_option) {

     ret = bsc_bvlc_decode_option_hdr(options_list,
                                      options_list_len - (uint16_t)option_len,
                                      &option_array[i].type,
                                      &option_array[i].must_understand,
                                      &next_option);
     if(!ret) {
      return false;
     }

     if(option_array[i].type == BSC_BVLC_OPTION_TYPE_PROPRIETARY) {

     }

     res = bsc_bvlc_decode_proprietary_option(options_list,
                                              options_list_len - (uint16_t)option_len,
                                              &option_array[i].specific.proprietary.vendor_id,
                                              &option_array[i].specific.proprietary.option_type,
                                              &option_array[i].specific.proprietary.data,
                                              &option_array[i].specific.proprietary.data_len );
     if(!res) {
      return false;
     }

     option_len += ret;
     i++;
     options_list = next_option;
  }

  return true;
}

static bool bsc_bvlc_decode_dest_options_if_exists(bsc_bvlc_unpacked_message_t *message)
{
  if(message->hdr.dest_options) {
    if(!bsc_bvlc_decode_header_options(message->dest_options,
                                       BSC_HEADER_OPTION_MAX,
                                       message->hdr.dest_options,
                                       message->hdr.dest_options_len)) {
      return false;
    }
  }
  return true;
}

static bool bsc_bvlc_decode_data_options_if_exists(bsc_bvlc_unpacked_message_t *message)
{
  if(message->hdr.data_options) {
    if(!bsc_bvlc_decode_header_options(message->data_options,
                                       BSC_HEADER_OPTION_MAX,
                                       message->hdr.data_options,
                                       message->hdr.data_options_len)) {
      return false;
    }
  }
  return true;
}

bool bsc_bvlc_decode_message(uint8_t                     *buf,
                             int                          buf_len,
                             bsc_bvlc_unpacked_message_t *message )
{
  if(!message) {
    return NULL;
  }

  memset(message->data_options, 0, sizeof(message->data_options));
  memset(message->dest_options, 0, sizeof(message->dest_options));
  memset(&message->payload, 0, sizeof(message->payload));

  if(!bsc_bvlc_decode_hdr(buf, buf_len, &message->hdr)) {
    return false;
  }

  if(message->hdr.dest_options) {
    if(message->hdr.dest_options_num > BSC_HEADER_OPTION_MAX) {
      /* dest header options list is too long */
      return false;
    }
  }

  if(message->hdr.data_options) {
    if(message->hdr.data_options_num > BSC_HEADER_OPTION_MAX) {
      /* data header options list is too long */
      return false;
    }
  }

  switch(message->hdr.bvlc_function) {
    case BSC_BVLC_RESULT:
    {
        if(message->hdr.data_options) {
          /* The BVLC-Result message must not have data options */
          return false;
        }

        if(!message->hdr.payload || !message->hdr.payload_len) {
          return false;
        }

        if(!bsc_bvlc_decode_dest_options_if_exists(message)) {
          return false;
        }

        if(!bsc_bvlc_decode_result(&message->payload,
                                    message->hdr.payload,
                                    message->hdr.payload_len)) {
          return false;
        }
        break;
    }
    case BSC_BVLC_ENCAPSULATED_NPDU:
    {
      if(!message->hdr.payload || !message->hdr.payload_len) {
        return false;
      }

      if(!bsc_bvlc_decode_dest_options_if_exists(message)) {
        return false;
      }

      if(!bsc_bvlc_decode_data_options_if_exists(message)) {
        return false;
      }

      message->payload.encapsulated_npdu.npdu = message->hdr.payload;
      message->payload.encapsulated_npdu.npdu_len = message->hdr.payload_len;
      break;
    }
    case BSC_BVLC_ADDRESS_RESOLUTION:
    {
      if(message->hdr.data_options) {
        /* The Address-Resolution message must not have data options */
        return false;
      }

      if(message->hdr.payload || message->hdr.payload_len) {
        return false;
      }

      if(!bsc_bvlc_decode_dest_options_if_exists(message)) {
        return false;
      }

      break;
    }
    case BSC_BVLC_ADDRESS_RESOLUTION_ACK:
    {
      if(message->hdr.data_options) {
        /* The Address-Resolution Ack message must not have data options */
        return false;
      }

      if(!message->hdr.payload || !message->hdr.payload_len) {
        return false;
      }

      if(!bsc_bvlc_decode_dest_options_if_exists(message)) {
        return false;
      }
      message->payload.address_resolution_ack.utf8_websocket_uri_string = message->hdr.payload;
      message->payload.address_resolution_ack.utf8_websocket_uri_string_len = message->hdr.payload_len;
      break;
    }
    case BSC_BVLC_ADVERTISIMENT:
    {
      if(message->hdr.data_options) {
        /* The advertisiment message must not have data options */
        return false;
      }

      if(!message->hdr.payload || !message->hdr.payload_len) {
        return false;
      }

      if(message->hdr.payload_len != 6) {
        return false;
      }

      if(!bsc_bvlc_decode_dest_options_if_exists(message)) {
        return false;
      }

      if(!bsc_bvlc_decode_advertisiment(&message->payload,
                                         message->hdr.payload,
                                         message->hdr.payload_len)) {
        return false;
      }
      break;
    }
    case BSC_BVLC_ADVERTISIMENT_SOLICITATION:
    {
      if(message->hdr.data_options) {
        /* The advertisiment solicitationn message must not have data options */
        return false;
      }

      if(message->hdr.payload || message->hdr.payload_len) {
        return false;
      }

      if(!bsc_bvlc_decode_dest_options_if_exists(message)) {
        return false;
      }
      break;
    }
    case BSC_BVLC_CONNECT_REQUEST:
    case BSC_BVLC_CONNECT_ACCEPT:
    case BSC_BVLC_DISCONNECT_REQUEST:
    case BSC_BVLC_DISCONNECT_ACK:
    case BSC_BVLC_HEARTBEAT_REQUEST:
    case BSC_BVLC_HEARTBEAT_ACK:
    {
      if(message->hdr.origin.mac_len != 0) {
        return false;
      }

      if(message->hdr.dest.mac_len != 0) {
        return false;
      }

      if(message->hdr.data_options) {
        /* The connect request message must not have data options */
        return false;
      }

      if(!message->hdr.payload || !message->hdr.payload_len) {
        return false;
      }

      if(!bsc_bvlc_decode_dest_options_if_exists(message)) {
        return false;
      }

      if(message->hdr.bvlc_function == BSC_BVLC_CONNECT_REQUEST) {
        if(!bsc_bvlc_decode_connect_request(&message->payload,
                                            message->hdr.payload,
                                            message->hdr.payload_len)) {
          return false;
        }
      }
      else if(message->hdr.bvlc_function == BSC_BVLC_CONNECT_ACCEPT) {
        if(!bsc_bvlc_decode_connect_accept(&message->payload,
                                            message->hdr.payload,
                                            message->hdr.payload_len)) {
          return false;
        }
      }
      break;
    }
    case BSC_BVLC_PROPRIETARY_MESSAGE:
    {
      if(message->hdr.data_options) {
        /* The connect request message must not have data options */
        return false;
      }

      if(!message->hdr.payload || !message->hdr.payload_len) {
        return false;
      }

      if(!bsc_bvlc_decode_dest_options_if_exists(message)) {
        return false;
      }

      if(!bsc_bvlc_decode_proprietary(&message->payload,
                                       message->hdr.payload,
                                       message->hdr.payload_len)) {
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

