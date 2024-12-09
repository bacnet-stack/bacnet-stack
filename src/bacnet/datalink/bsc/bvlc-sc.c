/**
 * @file
 * @brief API for encoding/decoding of BACnet/SC BVLC messages
 * @author Kirill Neznamov <kirill.neznamov@dsr-corporation.com>
 * @date May 2022
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include "bacnet/datalink/bsc/bvlc-sc.h"

typedef enum BACNet_Option_Validation_Type {
    BACNET_USER_OPTION_VALIDATION,
    BACNET_PDU_DEST_OPTION_VALIDATION,
    BACNET_PDU_DATA_OPTION_VALIDATION
} BACNET_OPTION_VALIDATION_TYPE;

static const char *s_message_is_incompleted = "header options is truncated";
static const char *s_invalid_header_option_type =
    "header option type must be 'Secure Path' or 'Proprietary Header'";
static const char *s_invalid_header_1 =
    "'Secure Path' header option can be added only to data options in bvlc "
    "message";
static const char *s_invalid_header_2 =
    "'Secure Path' header option must not have header data";
static const char *s_invalid_header_3 =
    "'Proprietary Header' option must have header data";
static const char *s_result_incomplete =
    "BVLC-Result message has incomplete payload";
static const char *s_result_incorrect_bvlc_function =
    "parameter 'Result For BVLC Function' is out of range";
static const char *s_result_incorrect_result_code =
    "parameter 'Result Code' must be 0x00 (ACK) or 0x01(NAK)";
static const char *s_result_inconsistent =
    "BVLC-Result message has data inconsistency in payload";
static const char *s_result_unexpected_data =
    "BVLC-Result message is longer than expected";
static const char *s_advertisiment_incomplete =
    "advertisiment message has incomplete payload";
static const char *s_advertisiment_unexpected =
    "advertisiment message is longer than expected";
static const char *s_advertisiment_param1_error =
    "parameter 'Hub Connection Status' in advertisiment message must be in "
    "range [0, 2]";
static const char *s_advertisiment_param2_error =
    "parameter 'Accept Direct Connections'in advertisiment message must be in "
    "range [0, 1]";
static const char *s_connect_request_incomplete =
    "connect-request message has incomplete payload";
static const char *s_connect_request_unexpected =
    "connect-request message is longer than expected";
static const char *s_connect_accept_incomplete =
    "connect-accept message has incomplete payload";
static const char *s_connect_accept_unexpected =
    "connect-accept is longer than expected";
static const char *s_proprietary_incomplete =
    "proprietary message has incomplete payload";
static const char *s_hdr_incomplete1 =
    "message is incomplete, 'Originating Virtual Address' field is truncated";
static const char *s_hdr_incomplete2 =
    "message is incomplete, 'Destination Virtual Address' field is truncated";
static const char *s_unknown_bvlc_function =
    "unknown value of 'BVLC Function' field in message";
static const char *s_dest_options_list_too_long =
    "message contains more than #BVLC_SC_HEADER_OPTION_MAX options in destion "
    "options list";
static const char *s_data_options_list_too_long =
    "message contains more than #BVLC_SC_HEADER_OPTION_MAX options in data "
    "options list";
static const char *s_result_unexpected_data_options =
    "BVLC-Result message must not have data options";
static const char *s_result_payload_expected =
    "BVLC-Result message must have payload";
static const char *s_encapsulated_npdu_payload_expected =
    "encapsulated-npdu message must have payload";
static const char *s_address_resolution_data_options =
    "address-resolution message must not have data options";
static const char *s_address_resolution_unexpected =
    "address-resolution message is longer than expected";
static const char *s_address_resolution_ack_data_options =
    "address-resolutio-ack message must not have data options";
static const char *s_advertisiment_data_options =
    "advertisiment message must not have data options";
static const char *s_advertisiment_payload_expected =
    "advertisiment message must have payload";
static const char *s_advertisiment_solicitation_data_options =
    "advertisiment solicitation message must not have data options";
static const char *s_advertisiment_solicitation_payload_expected =
    "advertisiment solicitation message must have payload";
static const char *s_origin_unexpected =
    "'Originating Virtual Address' field must be absent in message";
static const char *s_dest_unexpected =
    "'Destination Virtual Address' field must be absent in message";
static const char *s_data_option_unexpected =
    "message must not have data options";
static const char *s_message_too_long = "message is longer than expected";
static const char *s_absent_payload = "payload is absent in the message";
static const char *s_proprietary_data_options =
    "proprietary message must not have data options";
static const char *s_proprietary_payload =
    "proprietary message must have payload";

/**
 * @brief Validate BVLC-SC header options
 * @param validation_type - type of validation
 * @param option_headers - pointer to the option headers
 * @param option_headers_max_len - maximum length of the option headers
 * @param out_option_headers_real_length - pointer to the real length of the
 * option headers
 * @param out_option_header_num - pointer to the number of the option headers
 * @param error - pointer to the error code
 * @param class - pointer to the error class
 * @param error_desc_string - pointer to the error description string
 * @return true if the option headers are valid, false otherwise
 */
static bool bvlc_sc_validate_options_headers(
    BACNET_OPTION_VALIDATION_TYPE validation_type,
    uint8_t *option_headers,
    size_t option_headers_max_len,
    size_t *out_option_headers_real_length,
    size_t *out_option_header_num,
    BACNET_ERROR_CODE *error,
    BACNET_ERROR_CLASS *class,
    const char **error_desc_string)
{
    int options_len = 0;
    uint8_t flags = 0;
    uint8_t option;
    uint16_t hdr_len;

    if (!option_headers_max_len || !out_option_headers_real_length) {
        *error = ERROR_CODE_MESSAGE_INCOMPLETE;
        *class = ERROR_CLASS_COMMUNICATION;
        *error_desc_string = s_message_is_incompleted;
        return false;
    }

    if (out_option_header_num) {
        *out_option_header_num = 0;
    }

    while (options_len < option_headers_max_len) {
        flags = option_headers[options_len];

        option = flags & BVLC_SC_HEADER_OPTION_TYPE_MASK;

        if (option != BVLC_SC_OPTION_TYPE_SECURE_PATH &&
            option != BVLC_SC_OPTION_TYPE_PROPRIETARY) {
            *error = ERROR_CODE_HEADER_ENCODING_ERROR;
            *class = ERROR_CLASS_COMMUNICATION;
            *error_desc_string = s_invalid_header_option_type;
            return false;
        }

        if (option == BVLC_SC_OPTION_TYPE_SECURE_PATH) {
            if (validation_type == BACNET_PDU_DEST_OPTION_VALIDATION) {
                /* According BACNet standard secure path header option can be
                  added only to data options. (AB.2.3.1 Secure Path Header
                  Option) */
                *error = ERROR_CODE_HEADER_ENCODING_ERROR;
                *class = ERROR_CLASS_COMMUNICATION;
                *error_desc_string = s_invalid_header_1;
                return false;
            }
            if (flags & BVLC_SC_HEADER_DATA) {
                /* securepath option header does not have data header
                   according bacnet stadard */
                *error = ERROR_CODE_HEADER_ENCODING_ERROR;
                *class = ERROR_CLASS_COMMUNICATION;
                *error_desc_string = s_invalid_header_2;
                return false;
            }
            options_len++;
        } else if (option == BVLC_SC_OPTION_TYPE_PROPRIETARY) {
            if (!(flags & BVLC_SC_HEADER_DATA)) {
                /* proprietary option must have header data */
                *error = ERROR_CODE_HEADER_ENCODING_ERROR;
                *class = ERROR_CLASS_COMMUNICATION;
                *error_desc_string = s_invalid_header_3;
                return false;
            }
            options_len++;
            if (options_len + 2 > option_headers_max_len) {
                /* not enough data to process header that means
                   that probably message is incomplete */
                *error = ERROR_CODE_MESSAGE_INCOMPLETE;
                *class = ERROR_CLASS_COMMUNICATION;
                *error_desc_string = s_message_is_incompleted;
                return false;
            }
            memcpy(&hdr_len, &option_headers[options_len], 2);
            options_len +=
                2 /* header length */ + hdr_len /* length of data header */;
        }

        if (options_len > option_headers_max_len) {
            /* not enough data to process header that means
               that probably message is incomplete */
            *error = ERROR_CODE_MESSAGE_INCOMPLETE;
            *class = ERROR_CLASS_COMMUNICATION;
            *error_desc_string = s_message_is_incompleted;
            return false;
        }

        if (out_option_header_num && (*out_option_header_num < USHRT_MAX)) {
            *out_option_header_num += 1;
        }

        if (!(flags & BVLC_SC_HEADER_MORE)) {
            break;
        }
    }

    *out_option_headers_real_length = options_len;
    return true;
}

/**
 * @brief Function adds header option to data option list
 *  into provided pdu and stores the result in out_pdu.
 *  Note that last added option will be the first option in
 *  the list.
 * @param out_pdu - buffer where modified pdu will be stored in
 * @param out_pdu_size - size of the out_pdu
 * @param pdu - input pdu to which user wants to add option
 * @param pdu_size - size of input pdu
 * @param sc_option - pointer to encoded header option
 * @param sc_option_len - length of encoded header option
 * @return returns length (>0) in bytes of a pdu with added option
 */
static size_t bvlc_sc_add_option(
    bool to_data_option,
    uint8_t *pdu,
    size_t pdu_size,
    uint8_t *in_pdu,
    size_t in_pdu_len,
    uint8_t *sc_option,
    size_t sc_option_len)
{
    size_t offs = 4;
    uint8_t flags = 0;
    size_t options_len = 0;
    uint8_t mask;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    BACNET_OPTION_VALIDATION_TYPE vt;
    const char *err_desc;

    if (!in_pdu_len || !in_pdu || !sc_option_len || !pdu_size || !pdu ||
        !sc_option) {
        return 0;
    }

    if (pdu_size < 4 || in_pdu_len < 4) {
        return 0;
    }

    if (sc_option_len + in_pdu_len > USHRT_MAX) {
        return 0;
    }

    if (pdu_size < in_pdu_len) {
        return 0;
    }

    if (pdu_size < sc_option_len + in_pdu_len) {
        return 0;
    }

    /* ensure that sc_option does not have flag more options */

    if (sc_option[0] & BVLC_SC_HEADER_MORE) {
        return 0;
    }

    if (!to_data_option &&
        (sc_option[0] & BVLC_SC_HEADER_OPTION_TYPE_MASK) ==
            BVLC_SC_OPTION_TYPE_SECURE_PATH) {
        /* According BACNet standard secure path header option can be added
           only to data options. (AB.2.3.1 Secure Path Header Option) */
        return 0;
    }

    /* ensure that user wants to add valid option */
    if (!bvlc_sc_validate_options_headers(
            BACNET_USER_OPTION_VALIDATION, sc_option, sc_option_len,
            &options_len, NULL, &error, &class, &err_desc)) {
        return 0;
    }

    flags = in_pdu[1];

    if (flags & BVLC_SC_CONTROL_DEST_VADDR) {
        offs += BVLC_SC_VMAC_SIZE;
    }

    if (flags & BVLC_SC_CONTROL_ORIG_VADDR) {
        offs += BVLC_SC_VMAC_SIZE;
    }

    if (offs > in_pdu_len) {
        return 0;
    }

    /* insert options */

    if (to_data_option) {
        mask = BVLC_SC_CONTROL_DATA_OPTIONS;
        vt = BACNET_PDU_DATA_OPTION_VALIDATION;

        if (flags & BVLC_SC_CONTROL_DEST_OPTIONS) {
            /* some options are already presented in message.
               Validate them at first. */
            if (!bvlc_sc_validate_options_headers(
                    BACNET_PDU_DEST_OPTION_VALIDATION, &in_pdu[offs],
                    in_pdu_len - offs, &options_len, NULL, &error, &class,
                    &err_desc)) {
                return 0;
            }
            offs += options_len;
        }
    } else {
        mask = BVLC_SC_CONTROL_DEST_OPTIONS;
        vt = BACNET_PDU_DEST_OPTION_VALIDATION;
    }

    if ((flags & mask)) {
        /* some options are already presented in message.
           Validate them at first. */

        if (!bvlc_sc_validate_options_headers(
                vt, &in_pdu[offs], in_pdu_len - offs, &options_len, NULL,
                &error, &class, &err_desc)) {
            return 0;
        }
    }

    if (pdu == in_pdu) {
        /* user would like to use the same buffer where in_pdu
           is already stored. So we need to expand it for sc_option_len */
        memmove(&pdu[offs + sc_option_len], &pdu[offs], in_pdu_len - offs);
        memcpy(&pdu[offs], sc_option, sc_option_len);
    } else {
        /* user would like to use a new buffer for in_pdu with option.*/
        memcpy(pdu, in_pdu, offs);
        memcpy(&pdu[offs], sc_option, sc_option_len);
        if ((flags & mask)) {
            pdu[offs] |= BVLC_SC_HEADER_MORE;
        }
        memcpy(&pdu[offs + sc_option_len], &in_pdu[offs], in_pdu_len - offs);
    }
    if ((flags & mask)) {
        pdu[offs] |= BVLC_SC_HEADER_MORE;
    } else {
        pdu[1] |= mask;
    }
    return in_pdu_len + sc_option_len;
}

/**
 * @brief Function adds header option to destination option list
          into provided pdu and stores the result in out_pdu.
          Note that last added option will be the first option in
          the list.
 * @param out_pdu - buffer where modified pdu will be stored in
 * @param out_pdu_size - size of the out_pdu
 * @param pdu - input pdu to which user wants to add option
 * @param pdu_size - size of input pdu
 * @param sc_option - pointer to encoded header option
 * @param sc_option_len - length of encoded header option
 * @return returns length (>0) in bytes of a pdu with added option
           or 0 in a case of error (validation of pdu or sc_option fails)
 */
size_t bvlc_sc_add_option_to_destination_options(
    uint8_t *out_pdu,
    size_t out_pdu_size,
    uint8_t *pdu,
    size_t pdu_size,
    uint8_t *sc_option,
    size_t sc_option_len)
{
    return bvlc_sc_add_option(
        false, out_pdu, out_pdu_size, pdu, pdu_size, sc_option, sc_option_len);
}

/**
 * @brief Function adds header option to data option list
          into provided pdu and stores the result in out_pdu.
          Note that last added option will be the first option in
          the list.
 * @param out_pdu - buffer where modified pdu will be stored in
 * @param out_pdu_size - size of the out_pdu
 * @param pdu - input pdu to which user wants to add option
 * @param pdu_size - size of input pdu
 * @param sc_option - pointer to encoded header option
 * @param sc_option_len - length of encoded header option
 * @return returns length (>0) in bytes of a pdu with added option
           or 0 in a case of error (validation of pdu or sc_option fails)
 */
size_t bvlc_sc_add_option_to_data_options(
    uint8_t *out_pdu,
    size_t out_pdu_size,
    uint8_t *pdu,
    size_t pdu_size,
    uint8_t *sc_option,
    size_t sc_option_len)
{
    return bvlc_sc_add_option(
        true, out_pdu, out_pdu_size, pdu, pdu_size, sc_option, sc_option_len);
}

/**
 * @brief Function encodes proprietary header option in correspondence
 *        of BACNet standard AB.2.3.2 Proprietary Header Options.
 *        Any proprietary header option shall consist of the
 *        following fields:
 *
 * Header Marker                 1-octet   'More Options' = 0 or 1,
 *                                         'Must Understand' = 0 or 1,
 *                                         'Header Data Flag' = 1,
 *                                         'Header Option Type' = 31
 * Header Length                 2-octets   Length of 'Header Data' field,
 *                                          in octets.
 * Header Data                   3-N octets Required to include:
 *   Vendor Identifier           2-octets   Vendor Identifier, with most
 *                                          significant octet first, of the
 *                                          organization defining this option.
 *   Proprietary Option Type     1-octet    An indication of the proprietary
 *                                          header option type.
 *   Proprietary Header data     Variable   A proprietary string of octets.
 *                                          Can be zero length.
 *
 * @param pdu - buffer to store encoded option
 * @param pdu_size - size of pdu
 * @param must_understand - value for 'Must Understand' flag.
 * @param pdu_size - size of input pdu
 * @param vendor_id - vendor identifier
 * @param proprietary_option_type - proprietary header option type.
 * @param proprietary_data - buffer with proprietary data
 * @param proprietary_data_len - size of buffer with proprietary data
 * @return value (>0) in bytes of a encoded options or 0 in a case of
 *  an error
 */
size_t bvlc_sc_encode_proprietary_option(
    uint8_t *pdu,
    size_t pdu_size,
    bool must_understand,
    uint16_t vendor_id,
    uint8_t proprietary_option_type,
    uint8_t *proprietary_data,
    size_t proprietary_data_len)
{
    size_t total_len =
        sizeof(vendor_id) + proprietary_data_len + sizeof(uint8_t);

    if (proprietary_data_len > BVLC_SC_NPDU_SIZE -
            3 /* header marker len + header length len) */ -
            sizeof(vendor_id)) {
        return 0;
    }

    if (pdu_size < total_len + 3 /* header marker len + header data len) */) {
        return 0;
    }

    /* reset More Options, Must Understand and Header Data flags
       they will be updated as a result of call bvlc_sc_add_sc_option() */

    pdu[0] = BVLC_SC_OPTION_TYPE_PROPRIETARY;

    if (must_understand) {
        pdu[0] |= BVLC_SC_HEADER_MUST_UNDERSTAND;
    }

    pdu[0] |= BVLC_SC_HEADER_DATA;
    memcpy(&pdu[1], &total_len, sizeof(total_len));
    memcpy(&pdu[3], &vendor_id, sizeof(vendor_id));
    memcpy(&pdu[5], &proprietary_option_type, sizeof(proprietary_option_type));
    memcpy(&pdu[6], proprietary_data, proprietary_data_len);
    return total_len + 3;
}

/**
 * @brief Function encodes security path header option in correspondence
 *        of BACNet standard AB.2.3.1 Secure Path Header Option.
 *        Any proprietary header option shall consist of the
 *        following fields:
 *
 * Header Marker                 1-octet   'Last Option' = 0 or 1,
 *                                         'Must Understand' = 0 or 1,
 *                                         'Header Data Flag' = 1,
 *                                         'Header Option Type' = 31
 *
 * @param pdu - buffer to store encoded option
 * @param pdu_size - size of pdu
 * @param must_understand - value for 'Must Understand' flag.
 * @return value (>0) in bytes of a encoded options or 0 in a case of
 *  an error
 */
size_t bvlc_sc_encode_secure_path_option(
    uint8_t *pdu, size_t pdu_size, bool must_understand)
{
    if (pdu_size < 1) {
        return 0;
    }

    pdu[0] = BVLC_SC_OPTION_TYPE_SECURE_PATH;

    if (must_understand) {
        pdu[0] |= BVLC_SC_HEADER_MUST_UNDERSTAND;
    }
    return 1;
}

/**
 * @brief Decodes BVLC header options marker. Function assumes to be called
 *        iteratively until end of option list will be reached. User must
 *        call this function on previously validated options list only
 *        because sanity checks are omitted.That valided option list can
 *        be get by some bvlc_sc_encode_ function calls.
 *
 * @param in_option_list - buffer containing list of header options.It must
                           point  to head list item to be decoded.
 * @param out_opt_type - pointer to store decoded option type, must not
                         be NULL
 * @param out_must_understand - pointer to store decoded 'must understand'
 *                              flag from options marker, must not be NULL.
 * @param out_next_option - pointer to next option list item to be decoded,
 *                          can be NULL if no any option items left.
 *
 * @return 0 in a case if decoding failed, otherwise returns
 *         length value of decoded option in bytes.
 *
 * Typically code for parsing of option list looks like following:
 *
 *  uint8_t *current = in_options_list;
 *  int option_len = 0;
 *  while(current) {
 *    bvlc_sc_decode_option_hdr(current,
 *                              &type, ...., &current);
 *  }
 *
 */

static void bvlc_sc_decode_option_hdr(
    uint8_t *in_options_list,
    BVLC_SC_OPTION_TYPE *out_opt_type,
    bool *out_must_understand,
    uint8_t **out_next_option)
{
    *out_next_option = NULL;
    *out_opt_type = (BVLC_SC_OPTION_TYPE)(in_options_list[0] &
                                          BVLC_SC_HEADER_OPTION_TYPE_MASK);
    *out_must_understand =
        (in_options_list[0] & BVLC_SC_HEADER_MUST_UNDERSTAND) ? true : false;

    if (*out_opt_type == BVLC_SC_OPTION_TYPE_SECURE_PATH) {
        if (in_options_list[0] & BVLC_SC_HEADER_MORE) {
            *out_next_option = in_options_list + 1;
        }
    } else if (*out_opt_type == BVLC_SC_OPTION_TYPE_PROPRIETARY) {
        uint16_t hdr_len;
        memcpy(&hdr_len, &in_options_list[1], sizeof(hdr_len));
        hdr_len += sizeof(hdr_len) + 1;
        if (in_options_list[0] & BVLC_SC_HEADER_MORE) {
            *out_next_option = in_options_list + hdr_len;
        }
    }
}

/**
 * @brief Decodes BVLC header proprietary option
 *  Function assumes to be called iteratively until end of option list
 *  will be reached. User must call this function on previously validated
 *  options list only because sanity checks are omitted. That valided
 *  option list can be get by some bvlc_sc_encode_ function calls.
 * @param in_options_list - buffer containing list of header options.It must
 *  point  to head list item to be decoded.
 * @param out_vendor_id - pointer to store vendor id, must not be NULL
 * @param out_proprietary_option_type - pointer to store proprietary option
 *  type, must not be NULL
 * @param out_proprietary_data - pointer to store proprietary data, can be NULL
 * @param out_proprietary_data_len - pointer to store length of proprietary
 *  data, can be NULL
 * @return 0 in a case if decoding failed, otherwise return
 */
static void bvlc_sc_decode_proprietary_option(
    uint8_t *in_options_list,
    uint16_t *out_vendor_id,
    uint8_t *out_proprietary_option_type,
    uint8_t **out_proprietary_data,
    size_t *out_proprietary_data_len)
{
    uint16_t hdr_len;

    *out_proprietary_data = NULL;
    *out_proprietary_data_len = 0;
    memcpy(&hdr_len, &in_options_list[1], sizeof(hdr_len));
    memcpy(out_vendor_id, &in_options_list[3], sizeof(uint16_t));
    *out_proprietary_option_type = in_options_list[5];

    if (hdr_len > 3) {
        *out_proprietary_data = &in_options_list[6];
        *out_proprietary_data_len = hdr_len - 3;
    }
}

/**
 * @brief encode BVLC-SC header
 *
 * BACNet standard AB.2.2 BVLC-SC Header Format:
 * BVLC Function               1-octet        BVLC function code
 * Control Flags               1-octet        Control flags
 * Message ID                  2-octets       Message identifier
 * Originating Virtual Address 0 or 6-octets  If absent, message is from
 *                                           connection peer node
 * Destination Virtual Address 0 or 6-octets  If absent, message is for
 *
 * @param pdu - A buffer to store the encoded pdu.
 * @param pdu_len - Size of the buffer to store the encoded pdu.
 * @param bvlc_function - BVLC function code, check BVLC_SC_MESSAGE_TYPE enum.
 * @param message_id - The message identifier
 * @param origin - Originating virtual address, can be NULL
 * @param dest  - Destination virtual address, can be NULL
 * @return number of bytes encoded, in a case of error returns 0.
 */
static size_t bvlc_sc_encode_common(
    uint8_t *pdu,
    size_t pdu_len,
    uint8_t bvlc_function,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest)
{
    size_t offs = 4;
    if (pdu_len < 4) {
        return 0;
    }
    pdu[0] = bvlc_function;
    pdu[1] = 0;
    memcpy(&pdu[2], &message_id, sizeof(message_id));

    if (origin) {
        if (pdu_len < offs + BVLC_SC_VMAC_SIZE) {
            return 0;
        }
        pdu[1] |= BVLC_SC_CONTROL_ORIG_VADDR;
        memcpy(&pdu[offs], &origin->address[0], BVLC_SC_VMAC_SIZE);
        offs += BVLC_SC_VMAC_SIZE;
    }

    if (dest) {
        if (pdu_len < offs + BVLC_SC_VMAC_SIZE) {
            return 0;
        }
        pdu[1] |= BVLC_SC_CONTROL_DEST_VADDR;
        memcpy(&pdu[offs], &dest->address[0], BVLC_SC_VMAC_SIZE);
        offs += BVLC_SC_VMAC_SIZE;
    }

    return offs;
}

/**
 * @brief Function encodes the BVLC-Result message according BACNet standard
 *        AB.2.4.1 BVLC-Result Format.
 *
 * BVLC Function               1-octet        (X'00') BVLC-Result
 * Control Flags               1-octet        Control flags.
 * Message ID                  2-octets       The message identifier of the
 *                                            message for which this message
 *                                            is the result.
 * Originating Virtual Address 0 or 6-octets  If absent, message is from
 *                                            connection peer node
 * Destination Virtual Address 0 or 6-octets  If absent, message is for
 *                                            connection peer node
 * Destination Options         Variable       Optional, 0 to N header options
 * Data Options                0-octets       Shall be absent.
 * Payload
 *  Result For BVLC Function   1-octet        BVLC function for which this
 *                                            is a result
 *  Result Code                1-octet(X'00') ACK: Successful completion. The
 *                                           'Error Header Marker' and all
 *                                            subsequent parameters shall be
 *                                            absent.
 *                                    (X'01') NAK: The BVLC function failed.
 *                                            The 'Error Header Marker',
 *                                            the 'Error Class', the 'Error
 *                                            Code', and the 'Error Details'
 *                                            shall be present.
 *  Error Header Marker        1-octet        The header marker of the
 *  (Conditional)                             destination option that caused
 *                                            the BVLC function to fail. If
 *                                            the NAK is unrelated to a header
 *                                            option, this parameter shall be
 *                                            X'00'.
 *  Error Class                2-octets       The 'Error Class' field of the
 *  (Conditional)                            'Error' datatype defined in
 *                                            Clause 21.
 *  Error Code                 2-octets       The 'Error Code' field of the
 *  (Conditional)                            'Error' datatype defined in
 *                                            Clause 21.
 *  Error Details              Variable       UTF-8 reason text. Can be an
 *  (Conditional)                             empty string using no octets.
 *                                            Note that this string is not
 *                                            encoded as defined in Clause
 *                                            20.2.9, has no character set
 *                                            indication octet, and no trailing
 *                                            zero octets. See BVLC-Result
 *                                            examples in Clause AB.2.17.
 *
 *
 * @param pdu - A buffer to store the encoded pdu.
 * @param pdu_len - Size of the buffer to store the encoded pdu.
 * @param message_id- The message identifier
 * @param origin - Originating virtual address, can be NULL
 * @param dest  - Destination virtual address, can be NULL
 * @param bvlc_function - BVLC function for which this is a result.
 *                        check BVLC_SC_MESSAGE_TYPE enum.
 * @param result_code - 0 (ACK) or 1 (NAK)
 * @param error_header_marker -can be NULL depending on result code.
 *                             The header marker of the destination option
 *                             that caused the BVLC function to fail
 * @param error_class - can be NULL depending on result code.
 *                      Check BACNET_ERROR_CLASS enum.
 * @param error_code - can be NULL depending on result code.
 *                     check BACNET_ERROR_CODE enum.
 * @param utf8_details_string - can be NULL depending on result code.
 *                              UTF-8 reason text.
 * @return number of bytes encoded, in a case of error returns 0.
 */
size_t bvlc_sc_encode_result(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t bvlc_function,
    uint8_t result_code,
    uint8_t *error_header_marker,
    uint16_t *error_class,
    uint16_t *error_code,
    const uint8_t *utf8_details_string)
{
    size_t offs;

    if (bvlc_function > (uint16_t)BVLC_SC_PROPRIETARY_MESSAGE) {
        /* unsupported bvlc function */
        return 0;
    }

    if (result_code != 0 && result_code != 1) {
        /* According AB.2.4.1 BVLC-Result Format result code
           must be 0x00 or 0x01 */
        return 0;
    }

    if (result_code) {
        if (!error_class || !error_code) {
            /* According AB.2.4.1 BVLC-Result Format error_class
               and error_code must be presented */
            return 0;
        }
    }

    offs = bvlc_sc_encode_common(
        pdu, pdu_len, BVLC_SC_RESULT, message_id, origin, dest);

    if (!offs) {
        return 0;
    }

    if (pdu_len < offs + 2) {
        return 0;
    }

    pdu[offs++] = bvlc_function;
    pdu[offs++] = result_code;

    if (!result_code) {
        if (error_header_marker || error_class || error_code ||
            utf8_details_string) {
            return 0;
        }
        return offs;
    }

    if (pdu_len < offs + 4) {
        return 0;
    }

    if (error_header_marker) {
        pdu[offs++] = *error_header_marker;
    } else {
        pdu[offs++] = 0;
    }

    memcpy(&pdu[offs], error_class, sizeof(*error_class));
    offs += sizeof(*error_class);
    memcpy(&pdu[offs], error_code, sizeof(*error_code));
    offs += sizeof(*error_code);

    if (utf8_details_string) {
        if (pdu_len < offs + strlen((const char *)utf8_details_string)) {
            return 0;
        }
        memcpy(
            &pdu[offs], utf8_details_string,
            strlen((const char *)utf8_details_string));
        offs += strlen((const char *)utf8_details_string);
    }

    return offs;
}

/**
 * @brief Function decodes the BVLC-Result message according BACNet standard
 *       AB.2.4.1 BVLC-Result Format.
 * @param payload - pointer to the decoded data
 * @param packed_payload - pointer to the packed data
 * @param packed_payload_len - size of the packed data
 * @param error - pointer to the error code
 * @param class - pointer to the error class
 * @param err_desc - pointer to the error description string
 * @return true if the data is decoded successfully, false otherwise
 */
static bool bvlc_sc_decode_result(
    BVLC_SC_DECODED_DATA *payload,
    uint8_t *packed_payload,
    size_t packed_payload_len,
    BACNET_ERROR_CODE *error,
    BACNET_ERROR_CLASS *class,
    const char **err_desc)
{
    size_t i;

    if (packed_payload_len < 2) {
        *error = ERROR_CODE_MESSAGE_INCOMPLETE;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = s_result_incomplete;
        return false;
    }

    memset(&payload->result, 0, sizeof(payload->result));

    if (packed_payload[0] > (uint8_t)BVLC_SC_PROPRIETARY_MESSAGE) {
        /* unknown BVLC function */
        *error = ERROR_CODE_PARAMETER_OUT_OF_RANGE;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = s_result_incorrect_bvlc_function;
        return false;
    }

    payload->result.bvlc_function = packed_payload[0];
    if (packed_payload[1] != 0 && packed_payload[1] != 1) {
        /* result code must be 1 or 0 */
        *error = ERROR_CODE_PARAMETER_OUT_OF_RANGE;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = s_result_incorrect_result_code;
        return false;
    }

    payload->result.result = packed_payload[1];

    if (packed_payload[1] == 1) {
        if (packed_payload_len < 7) {
            *error = ERROR_CODE_MESSAGE_INCOMPLETE;
            *class = ERROR_CLASS_COMMUNICATION;
            *err_desc = s_result_incomplete;
            return false;
        }

        payload->result.error_header_marker = packed_payload[2];
        memcpy(
            &payload->result.error_class, &packed_payload[3],
            sizeof(payload->result.error_class));
        memcpy(
            &payload->result.error_code, &packed_payload[5],
            sizeof(payload->result.error_code));

        if (packed_payload_len > 7) {
            payload->result.utf8_details_string = &packed_payload[7];
            payload->result.utf8_details_string_len = packed_payload_len - 7;
            for (i = 0; i < packed_payload_len - 7; i++) {
                if (payload->result.utf8_details_string[i] == 0) {
                    break;
                }
            }
            if (i != packed_payload_len - 7) {
                *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_result_inconsistent;
                return false;
            }
        }
    } else if (packed_payload_len > 2) {
        /* According EA-001-4 'Clarifying BVLC-Result in BACnet/SC */
        /* If a BVLC message is received that is longer than expected, */
        /* a BVLC-Result NAK shall be returned if it was a unicast message, */
        /* indicating an 'Error Class' of COMMUNICATION and 'Error Code' of */
        /* UNEXPECTED_DATA. The message shall be discarded and not be processed.
         */
        *error = ERROR_CODE_UNEXPECTED_DATA;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = s_result_unexpected_data;
        return false;
    }
    return true;
}

/**
 * @brief Function encodes the Encapsulated-NPDU message according
 *        BACNet standard AB.2.5 Encapsulated-NPDU.
 *
 * BVLC Function               1-octet        (X'01') Encapsulated-NPDU
 * Control Flags               1-octet        Control flags.
 * Message ID                  2-octets       The message identifier of the
 *                                            message for which this message
 *                                            is the result.
 * Originating Virtual Address 0 or 6-octets  If absent, message is from
 *                                            connection peer node
 * Destination Virtual Address 0 or 6-octets  If absent, message is for
 *                                            connection peer node
 * Destination Options         Variable       Optional, 0 to N header options
 * Data Options                0-octets       Optional, 0 to N header options
 * Payload
 *  BACnet NPDU                Variable
 *
 * @param pdu - A buffer to store the encoded pdu.
 * @param pdu_len - Size of the buffer to store the encoded pdu.
 * @param message_id- The message identifier
 * @param origin - Originating virtual address, can be NULL
 * @param dest  - Destination virtual address, can be NULL
 * @param npdu - Buffer which contains BACnet NPDU
 * @param npdu_size - Size of buffer which contains BACnet NPDU
 * @return number of bytes encoded, in a case of error returns 0.
 */
size_t bvlc_sc_encode_encapsulated_npdu(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *npdu,
    size_t npdu_size)
{
    size_t offs;

    offs = bvlc_sc_encode_common(
        pdu, pdu_len, BVLC_SC_ENCAPSULATED_NPDU, message_id, origin, dest);
    if (!offs) {
        return 0;
    }

    if (pdu_len < offs + npdu_size) {
        return 0;
    }

    memcpy(&pdu[offs], npdu, npdu_size);
    offs += npdu_size;
    return offs;
}

/**
 * @brief Function encodes the Address-Resolution message according BACNet
 * standard AB.2.6 Address-Resolution
 *
 * BVLC Function               1-octet        (X'02') Address-Resolution
 * Control Flags               1-octet        Control flags.
 * Message ID                  2-octets       The message identifier of the
 *                                            message for which this message
 *                                            is the result.
 * Originating Virtual Address 0 or 6-octets  If absent, message is from
 *                                            connection peer node
 * Destination Virtual Address 0 or 6-octets  If absent, message is for
 *                                            connection peer node
 * Destination Options         Variable       Optional, 0 to N header options
 * Data Options                0-octets       Shall be absent
 *
 * @param pdu - A buffer to store the encoded pdu.
 * @param pdu_len - Size of the buffer to store the encoded pdu.
 * @param message_id- The message identifier
 * @param origin - Originating virtual address, can be NULL
 * @param dest  - Destination virtual address, can be NULL
 * @return number of bytes encoded, in a case of error returns 0.
 */
size_t bvlc_sc_encode_address_resolution(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest)
{
    size_t offs;

    offs = bvlc_sc_encode_common(
        pdu, pdu_len, BVLC_SC_ADDRESS_RESOLUTION, message_id, origin, dest);

    return offs;
}

/**
 * @brief Function encodes the Address-Resolution-ACK message according
 *        BACNet standard AB.2.7.1 Address-Resolution-ACK Format.
 *
 * BVLC Function               1-octet        (X'03') Address-Resolution-ACK
 * Control Flags               1-octet        Control flags.
 * Message ID                  2-octets       The message identifier of the
 *                                            message for which this message
 *                                            is the result.
 * Originating Virtual Address 0 or 6-octets  If absent, message is from
 *                                            connection peer node
 * Destination Virtual Address 0 or 6-octets  If absent, message is for
 *                                            connection peer node
 * Destination Options         Variable       Optional, 0 to N header options
 * Data Options                0-octets       Shall be absent.
 * Payload
 *  WebSocket-URIs             Variable       UTF-8 string containing a list
 *                                            of WebSocket URIs as of RFC3986,
 *                                            separated by a single space
 *                                            character (X'20'), where the
 *                                            source BACnet/SC node accepts
 *                                            direct connections. Can be an
 *                                            empty string using zero octets.
 *                                            See Clause AB.3.3.
 *
 * @param pdu - A buffer to store the encoded pdu.
 * @param pdu_len - Size of the buffer to store the encoded pdu.
 * @param message_id- The message identifier
 * @param origin - Originating virtual address, can be NULL
 * @param dest  - Destination virtual address, can be NULL
 * @param web_socket_uris - UTF-8 string containing a list of
 *                          WebSocket URIs as of RFC 3986, separated by a
 *                          single space character (X'20'), where the source
 *                          BACnet/SC node accepts direct connections.
 *                          Can be an empty string using zero octets.
 * @param web_socket_uris_len- length of web_socket_uris without trailing zero.
 * @return number of bytes encoded, in a case of error returns 0.
 */
size_t bvlc_sc_encode_address_resolution_ack(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *web_socket_uris,
    size_t web_socket_uris_len)
{
    size_t offs;

    offs = bvlc_sc_encode_common(
        pdu, pdu_len, BVLC_SC_ADDRESS_RESOLUTION_ACK, message_id, origin, dest);
    if (!offs) {
        return 0;
    }

    if (web_socket_uris && web_socket_uris_len) {
        memcpy(&pdu[offs], web_socket_uris, web_socket_uris_len);
        offs += web_socket_uris_len;
    }

    return offs;
}

/**
 * @brief Function encodes the Advertisement message according
 *        BACNet standard AB.2.8.1 Advertisement Format.
 *
 * BVLC Function               1-octet        (X'04') Advertisement
 * Control Flags               1-octet        Control flags.
 * Message ID                  2-octets       The message identifier of the
 *                                            message for which this message
 *                                            is the result.
 * Originating Virtual Address 0 or 6-octets  If absent, message is from
 *                                            connection peer node
 * Destination Virtual Address 0 or 6-octets  If absent, message is for
 *                                            connection peer node
 * Destination Options         Variable       Optional, 0 to N header options
 * Data Options                0-octets       Shall be absent.
 *
 * Payload
 *  Hub Connection Status      1-octet        X'00' No hub connection.
 *                                            X'01' Connected to primary hub.
 *                                            X'02' Connected to failover hub.
 *  Accept Direct Connections  1-octet        X'00' The node does not support
 *                                                  accepting direct
 *                                                  connections.
 *                                            X'01' The node supports accepting
 *                                                  direct connections.
 *  Maximum BVLC Length        2-octet        The maximum BVLC message size
 *                                            that can be received and
 *                                            processed by the node, in number
 *                                            of octets.
 *  Maximum NPDU Length        2-octet        The maximum NPDU message size
 *                                            that can be handled by the node's
 *                                            network entity, in number of
 *                                            octets.
 * @param pdu - A buffer to store the encoded pdu.
 * @param pdu_len - Size of the buffer to store the encoded pdu.
 * @param message_id- The message identifier
 * @param origin - Originating virtual address, can be NULL
 * @param dest  - Destination virtual address, can be NULL
 * @param hub_status - hub connection status
 * @param support- accept direct connections
 * @param max_bvlc_len - the maximum BVLC message size
 * @param max_npdu_size - the maximum NPDU message size
 * @return number of bytes encoded, in a case of error returns 0.
 */
size_t bvlc_sc_encode_advertisiment(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    BACNET_SC_HUB_CONNECTOR_STATE hub_status,
    BVLC_SC_DIRECT_CONNECTION_SUPPORT support,
    uint16_t max_bvlc_len,
    uint16_t max_npdu_size)
{
    size_t offs;

    offs = bvlc_sc_encode_common(
        pdu, pdu_len, BVLC_SC_ADVERTISIMENT, message_id, origin, dest);
    if (!offs) {
        return 0;
    }

    pdu[offs++] = (uint8_t)hub_status;
    pdu[offs++] = (uint8_t)support;
    memcpy(&pdu[offs], &max_bvlc_len, sizeof(max_bvlc_len));
    offs += sizeof(max_bvlc_len);
    memcpy(&pdu[offs], &max_npdu_size, sizeof(max_npdu_size));
    offs += sizeof(max_npdu_size);
    return offs;
}

/**
 * @brief Function encodes the Advertisement-Solicitation message according
 *       BACNet standard AB.2.9.1 Advertisement-Solicitation Format.
 * @param payload - pointer to the decoded data
 * @param packed_payload - pointer to packed data
 * @param packed_payload_len - size of packed data
 * @param error - pointer to the error code
 * @param class - pointer to the error class
 * @param err_desc - pointer to the error description string
 * @return true if the data is decoded successfully, false otherwise
 */
static bool bvlc_sc_decode_advertisiment(
    BVLC_SC_DECODED_DATA *payload,
    uint8_t *packed_payload,
    size_t packed_payload_len,
    BACNET_ERROR_CODE *error,
    BACNET_ERROR_CLASS *class,
    const char **err_desc)
{
    if (packed_payload_len < 6) {
        *error = ERROR_CODE_MESSAGE_INCOMPLETE;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = s_advertisiment_incomplete;
        return false;
    }
    if (packed_payload_len > 6) {
        *error = ERROR_CODE_UNEXPECTED_DATA;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = s_advertisiment_unexpected;
        return false;
    }
    if (packed_payload[0] > BACNET_SC_HUB_CONNECTOR_STATE_MAX) {
        *error = ERROR_CODE_PARAMETER_OUT_OF_RANGE;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = s_advertisiment_param1_error;
        return false;
    }

    if (packed_payload[1] > BVLC_SC_DIRECT_CONNECTION_SUPPORT_MAX) {
        *error = ERROR_CODE_PARAMETER_OUT_OF_RANGE;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = s_advertisiment_param2_error;
        return false;
    }

    payload->advertisiment.hub_status = packed_payload[0];
    payload->advertisiment.support = packed_payload[1];
    memcpy(
        &payload->advertisiment.max_bvlc_len, &packed_payload[2],
        sizeof(payload->advertisiment.max_bvlc_len));
    memcpy(
        &payload->advertisiment.max_npdu_len, &packed_payload[4],
        sizeof(payload->advertisiment.max_npdu_len));
    return true;
}

/**
 * @brief Function encodes the Advertisement-Solicitation message according
 *        BACNet standard AB.2.9.1 Advertisement-Solicitation Format.
 *
 * BVLC Function               1-octet        (X'05') Advertisement
 * Control Flags               1-octet        Control flags.
 * Message ID                  2-octets       The message identifier of the
 *                                            message for which this message
 *                                            is the result.
 * Originating Virtual Address 0 or 6-octets  If absent, message is from
 *                                            connection peer node
 * Destination Virtual Address 0 or 6-octets  If absent, message is for
 *                                            connection peer node
 * Destination Options         Variable       Optional, 0 to N header options
 * Data Options                0-octets       Shall be absent.
 *
 * @param pdu - A buffer to store the encoded pdu.
 * @param pdu_len - Size of the buffer to store the encoded pdu.
 * @param message_id- The message identifier
 * @param origin - Originating virtual address, can be NULL
 * @param dest  - Destination virtual address, can be NULL
 * @return number of bytes encoded, in a case of error returns 0.
 */
size_t bvlc_sc_encode_advertisiment_solicitation(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest)
{
    size_t offs;

    offs = bvlc_sc_encode_common(
        pdu, pdu_len, BVLC_SC_ADVERTISIMENT_SOLICITATION, message_id, origin,
        dest);
    return offs;
}

/**
 * @brief Function encodes the Connect-Request message according
 *        BACNet standard AB.2.10.1 Connect-Request Format.
 *
 * BVLC Function               1-octet        (X'06') Advertisement
 * Control Flags               1-octet        Control flags.
 * Message ID                  2-octets       The message identifier of the
 *                                            message for which this message
 *                                            is the result.
 * Originating Virtual Address 0              Absent, is always for
 *                                            connection peer node.
 * Destination Virtual Address 0              If absent, message is for
 *                                            connection peer node
 * Destination Options         Variable       Optional, 0 to N header options
 * Data Options                0-octets       Shall be absent.
 *
 * Payload
 *  VMAC Address               6-octets       The VMAC address of the
 *                                            requesting node.
 *  Device UUID                16-octet       The device UUID of the
 *                                            requesting node.
 *  Maximum BVLC Length        2-octet        The maximum BVLC message size
 *                                            that can be received and
 *                                            processed by the node, in number
 *                                            of octets.
 *  Maximum NPDU Length        2-octet        The maximum NPDU message size
 *                                            that can be handled by the node's
 *                                            network entity, in number of
 *                                            octets.
 * @param pdu - A buffer to store the encoded pdu.
 * @param pdu_len - Size of the buffer to store the encoded pdu.
 * @param message_id- The message identifier
 * @param local_vmac - VMAC address
 * @param local_uuid  - The device UUID
 * @param max_bvlc_len - the maximum BVLC message size
 * @param max_npdu_size - the maximum NPDU message size
 * @return number of bytes encoded, in a case of error returns 0.
 */
size_t bvlc_sc_encode_connect_request(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *local_vmac,
    BACNET_SC_UUID *local_uuid,
    uint16_t max_bvlc_len,
    uint16_t max_npdu_size)
{
    size_t offs;

    if (!local_vmac || !local_uuid) {
        return 0;
    }

    offs = bvlc_sc_encode_common(
        pdu, pdu_len, BVLC_SC_CONNECT_REQUEST, message_id, NULL, NULL);
    if (!offs) {
        return 0;
    }

    if (pdu_len <
        (offs + BVLC_SC_VMAC_SIZE + BVLC_SC_UUID_SIZE + 2 * sizeof(uint16_t))) {
        return 0;
    }

    memcpy(&pdu[offs], local_vmac, BVLC_SC_VMAC_SIZE);
    offs += BVLC_SC_VMAC_SIZE;
    memcpy(&pdu[offs], local_uuid, BVLC_SC_UUID_SIZE);
    offs += BVLC_SC_UUID_SIZE;
    memcpy(&pdu[offs], &max_bvlc_len, sizeof(max_bvlc_len));
    offs += sizeof(max_bvlc_len);
    memcpy(&pdu[offs], &max_npdu_size, sizeof(max_npdu_size));
    offs += sizeof(max_npdu_size);
    return (unsigned int)offs;
}

/**
 * @brief Function decodes the Connect-Request message according
 *       BACNet standard AB.2.10.1 Connect-Request Format.
 * @param payload - pointer to the decoded data
 * @param packed_payload - pointer to the packed data
 * @param packed_payload_len - size of the packed data
 * @param error - pointer to the error code
 * @param class - pointer to the error class
 * @param err_desc - pointer to the error description string
 * @return true if the data is decoded successfully, false otherwise
 */
static bool bvlc_sc_decode_connect_request(
    BVLC_SC_DECODED_DATA *payload,
    uint8_t *packed_payload,
    size_t packed_payload_len,
    BACNET_ERROR_CODE *error,
    BACNET_ERROR_CLASS *class,
    const char **err_desc)
{
    if (packed_payload_len < 26) {
        *error = ERROR_CODE_MESSAGE_INCOMPLETE;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = s_connect_request_incomplete;
        return false;
    } else if (packed_payload_len > 26) {
        *error = ERROR_CODE_UNEXPECTED_DATA;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = s_connect_request_unexpected;
        return false;
    }
    payload->connect_request.vmac = (BACNET_SC_VMAC_ADDRESS *)packed_payload;
    packed_payload += BVLC_SC_VMAC_SIZE;
    payload->connect_request.uuid = (BACNET_SC_UUID *)packed_payload;
    packed_payload += BVLC_SC_UUID_SIZE;
    memcpy(
        &payload->connect_request.max_bvlc_len, packed_payload,
        sizeof(payload->connect_request.max_bvlc_len));
    packed_payload += sizeof(payload->connect_request.max_bvlc_len);
    memcpy(
        &payload->connect_request.max_npdu_len, packed_payload,
        sizeof(payload->connect_request.max_npdu_len));
    return true;
}

/**
 * @brief Function encodes the Connect-Accept message according
 *        BACNet standard AB.2.11.1 Connect-Accept Format.
 *
 * BVLC Function               1-octet        (X'07') Advertisement
 * Control Flags               1-octet        Control flags.
 * Message ID                  2-octets       The message identifier of the
 *                                            message for which this message
 *                                            is the result.
 * Originating Virtual Address 0              Absent, is always for
 *                                            connection peer node.
 * Destination Virtual Address 0              If absent, message is for
 *                                            connection peer node
 * Destination Options         Variable       Optional, 0 to N header options
 * Data Options                0-octets       Shall be absent.
 *
 * Payload
 *  VMAC Address               6-octets       The VMAC address of the
 *                                            requesting node.
 *  Device UUID                16-octet       The device UUID of the
 *                                            requesting node.
 *  Maximum BVLC Length        2-octet        The maximum BVLC message size
 *                                            that can be received and
 *                                            processed by the node, in number
 *                                            of octets.
 *  Maximum NPDU Length        2-octet        The maximum NPDU message size
 *                                            that can be handled by the node's
 *                                            network entity, in number of
 *                                            octets.
 * @param pdu - A buffer to store the encoded pdu.
 * @param pdu_len - Size of the buffer to store the encoded pdu.
 * @param message_id- The message identifier
 * @param local_vmac - VMAC address
 * @param local_uuid  - The device UUID
 * @param max_bvlc_len - the maximum BVLC message size
 * @param max_npdu_size - the maximum NPDU message size
 * @return number of bytes encoded, in a case of error returns 0.
 */
size_t bvlc_sc_encode_connect_accept(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *local_vmac,
    BACNET_SC_UUID *local_uuid,
    uint16_t max_bvlc_len,
    uint16_t max_npdu_len)
{
    size_t offs;

    if (!local_vmac || !local_uuid) {
        return 0;
    }

    offs = bvlc_sc_encode_common(
        pdu, pdu_len, BVLC_SC_CONNECT_ACCEPT, message_id, NULL, NULL);
    if (!offs) {
        return 0;
    }

    if (pdu_len <
        (offs + BVLC_SC_VMAC_SIZE + BVLC_SC_UUID_SIZE + 2 * sizeof(uint16_t))) {
        return 0;
    }

    memcpy(&pdu[offs], local_vmac, BVLC_SC_VMAC_SIZE);
    offs += BVLC_SC_VMAC_SIZE;
    memcpy(&pdu[offs], local_uuid, BVLC_SC_UUID_SIZE);
    offs += BVLC_SC_UUID_SIZE;
    memcpy(&pdu[offs], &max_bvlc_len, sizeof(max_bvlc_len));
    offs += sizeof(max_bvlc_len);
    memcpy(&pdu[offs], &max_npdu_len, sizeof(max_npdu_len));
    offs += sizeof(max_npdu_len);
    return (unsigned int)offs;
}

/**
 * @brief Function decodes the Connect-Accept message according
 *      BACNet standard AB.2.11.1 Connect-Accept Format.
 * @param payload - pointer to the decoded data
 * @param packed_payload - pointer to the packed data
 * @param packed_payload_len - size of the packed data
 * @param error - pointer to the error code
 * @param class - pointer to the error class
 * @param err_desc - pointer to the error description string
 * @return true if the data is decoded successfully, false otherwise
 */
static bool bvlc_sc_decode_connect_accept(
    BVLC_SC_DECODED_DATA *payload,
    uint8_t *packed_payload,
    size_t packed_payload_len,
    BACNET_ERROR_CODE *error,
    BACNET_ERROR_CLASS *class,
    const char **err_desc)
{
    if (packed_payload_len < 26) {
        *error = ERROR_CODE_MESSAGE_INCOMPLETE;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = s_connect_accept_incomplete;
        return false;
    } else if (packed_payload_len > 26) {
        *error = ERROR_CODE_UNEXPECTED_DATA;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = s_connect_accept_unexpected;
        return false;
    }

    payload->connect_accept.vmac = (BACNET_SC_VMAC_ADDRESS *)packed_payload;
    packed_payload += BVLC_SC_VMAC_SIZE;
    payload->connect_accept.uuid = (BACNET_SC_UUID *)packed_payload;
    packed_payload += BVLC_SC_UUID_SIZE;
    memcpy(
        &payload->connect_accept.max_bvlc_len, packed_payload,
        sizeof(payload->connect_accept.max_bvlc_len));
    packed_payload += sizeof(payload->connect_accept.max_bvlc_len);
    memcpy(
        &payload->connect_accept.max_npdu_len, packed_payload,
        sizeof(payload->connect_accept.max_npdu_len));
    return true;
}

/**
 * @brief Function encodes the Disconnect-Request message according
 *        BACNet standard AB.2.12.1 Disconnect-Request Format.
 *
 * BVLC Function               1-octet        (X'08') Advertisement
 * Control Flags               1-octet        Control flags.
 * Message ID                  2-octets       The message identifier of the
 *                                            message for which this message
 *                                            is the result.
 * Originating Virtual Address 0              Absent, is always for
 *                                            connection peer node.
 * Destination Virtual Address 0              If absent, message is for
 *                                            connection peer node
 * Destination Options         Variable       Optional, 0 to N header options
 * Data Options                0-octets       Shall be absent.
 *
 * @param pdu - A buffer to store the encoded pdu.
 * @param pdu_len - Size of the buffer to store the encoded pdu.
 * @param message_id- The message identifier
 * @return number of bytes encoded, in a case of error returns 0.
 */
size_t bvlc_sc_encode_disconnect_request(
    uint8_t *pdu, size_t pdu_len, uint16_t message_id)
{
    size_t offs;

    offs = bvlc_sc_encode_common(
        pdu, pdu_len, BVLC_SC_DISCONNECT_REQUEST, message_id, NULL, NULL);
    return offs;
}

/**
 * @brief Function encodes the  Disconnect-ACK message according
 *        BACNet standard AB.2.13.1 Disconnect-ACK Format.
 *
 * BVLC Function               1-octet        (X'09') Advertisement
 * Control Flags               1-octet        Control flags.
 * Message ID                  2-octets       The message identifier of the
 *                                            message for which this message
 *                                            is the result.
 * Originating Virtual Address 0              Absent, is always for
 *                                            connection peer node.
 * Destination Virtual Address 0              If absent, message is for
 *                                            connection peer node
 * Destination Options         Variable       Optional, 0 to N header options
 * Data Options                0-octets       Shall be absent.
 *
 * @param pdu - A buffer to store the encoded pdu.
 * @param pdu_len - Size of the buffer to store the encoded pdu.
 * @param message_id- The message identifier
 * @return number of bytes encoded, in a case of error returns 0.
 */
size_t
bvlc_sc_encode_disconnect_ack(uint8_t *pdu, size_t pdu_len, uint16_t message_id)
{
    size_t offs;

    offs = bvlc_sc_encode_common(
        pdu, pdu_len, BVLC_SC_DISCONNECT_ACK, message_id, NULL, NULL);
    return offs;
}

/**
 * @brief Function encodes the  Heartbeat-Request message according
 *        BACNet standard AB.2.14.1 Heartbeat-Request Format.
 *
 * BVLC Function               1-octet        (X'0A') Advertisement
 * Control Flags               1-octet        Control flags.
 * Message ID                  2-octets       The message identifier of the
 *                                            message for which this message
 *                                            is the result.
 * Originating Virtual Address 0              Absent, is always for
 *                                            connection peer node.
 * Destination Virtual Address 0              If absent, message is for
 *                                            connection peer node
 * Destination Options         Variable       Optional, 0 to N header options
 * Data Options                0-octets       Shall be absent.
 *
 * @param pdu - A buffer to store the encoded pdu.
 * @param pdu_len - Size of the buffer to store the encoded pdu.
 * @param message_id- The message identifier
 * @return number of bytes encoded, in a case of error returns 0.
 */
size_t bvlc_sc_encode_heartbeat_request(
    uint8_t *pdu, size_t pdu_len, uint16_t message_id)
{
    size_t offs;

    offs = bvlc_sc_encode_common(
        pdu, pdu_len, BVLC_SC_HEARTBEAT_REQUEST, message_id, NULL, NULL);
    return offs;
}

/**
 * @brief Function encodes the Heartbeat-ACK message according
 *        BACNet standard AB.2.15.1 Heartbeat-ACK Format.
 *
 * BVLC Function               1-octet        (X'0B') Advertisement
 * Control Flags               1-octet        Control flags.
 * Message ID                  2-octets       The message identifier of the
 *                                            message for which this message
 *                                            is the result.
 * Originating Virtual Address 0              Absent, is always for
 *                                            connection peer node.
 * Destination Virtual Address 0              If absent, message is for
 *                                            connection peer node
 * Destination Options         Variable       Optional, 0 to N header options
 * Data Options                0-octets       Shall be absent.
 *
 * @param pdu - A buffer to store the encoded pdu.
 * @param pdu_len - Size of the buffer to store the encoded pdu.
 * @param message_id- The message identifier
 * @return number of bytes encoded, in a case of error returns 0.
 */
size_t
bvlc_sc_encode_heartbeat_ack(uint8_t *pdu, size_t pdu_len, uint16_t message_id)
{
    size_t offs;

    offs = bvlc_sc_encode_common(
        pdu, pdu_len, BVLC_SC_HEARTBEAT_ACK, message_id, NULL, NULL);
    return offs;
}

/**
 * @brief Function encodes the Proprietary Message according BACNet standard
 *        AB.2.16.1 Proprietary Message Format.
 *
 * BVLC Function               1-octet        (X'0C') Proprietary-Message.
 * Control Flags               1-octet        Control flags.
 * Message ID                  2-octets       The message identifier of the
 *                                            message for which this message
 *                                            is the result.
 * Originating Virtual Address 0 or 6-octets  If absent, message is from
 *                                            connection peer node
 * Destination Virtual Address 0 or 6-octets  If absent, message is for
 *                                            connection peer node
 * Destination Options         Variable       Optional, 0 to N header options
 * Data Options                0-octets       Shall be absent.
 * Payload                     3-N octets     The payload shall consist of at
 *                                            least the vendor identifier and
 *                                            the proprietary function octet.
 *  Vendor identifier          2-octets       Vendor Identifier, with most
 *                                            significant octet first, of the
 *                                            organization defining this
 *                                            message.
 *  Proprietary Function       1-octet        The vendor-defined function code.
 *
 *  Proprietary Data           Variable       Optional vendor-defined payload
 *                                            data.
 * @param pdu - A buffer to store the encoded pdu.
 * @param pdu_len - Size of the buffer to store the encoded pdu.
 * @param message_id- The message identifier
 * @param origin - Originating virtual address, can be NULL
 * @param dest  - Destination virtual address, can be NULL
 * @param vendor_id - vendor identifier.
 * @param proprietary_function - The vendor-defined function code
 * @param proprietary_data - buffer which holds optional vendor-defined
 *                           payload data.
 * @param proprietary_data_len - length of proprietary_data buffer.
 * @return number of bytes encoded, in a case of error returns 0.
 */
size_t bvlc_sc_encode_proprietary_message(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint16_t vendor_id,
    uint8_t proprietary_function,
    uint8_t *proprietary_data,
    size_t proprietary_data_len)
{
    size_t offs;

    offs = bvlc_sc_encode_common(
        pdu, pdu_len, BVLC_SC_PROPRIETARY_MESSAGE, message_id, origin, dest);
    if (!offs) {
        return 0;
    }

    if (pdu_len < (offs + sizeof(vendor_id) + sizeof(proprietary_function) +
                   proprietary_data_len)) {
        return 0;
    }

    memcpy(&pdu[offs], &vendor_id, sizeof(vendor_id));
    offs += sizeof(vendor_id);
    memcpy(&pdu[offs], &proprietary_function, sizeof(proprietary_function));
    offs += sizeof(proprietary_function);
    memcpy(&pdu[offs], proprietary_data, proprietary_data_len);
    offs += proprietary_data_len;
    return (unsigned int)offs;
}

/**
 * @brief Function decodes the Proprietary Message according BACNet standard
 *       AB.2.16.1 Proprietary Message Format.
 * @param payload - pointer to the decoded data
 * @param packed_payload - pointer to the packed data
 * @param packed_payload_len - size of the packed data
 * @param error - pointer to the error code
 * @param class - pointer to the error class
 * @param err_desc - pointer to the error description string
 * @return true if the data is decoded successfully, false otherwise
 */
static bool bvlc_sc_decode_proprietary(
    BVLC_SC_DECODED_DATA *payload,
    uint8_t *packed_payload,
    size_t packed_payload_len,
    BACNET_ERROR_CODE *error,
    BACNET_ERROR_CLASS *class,
    const char **err_desc)
{
    if (packed_payload_len < 3) {
        *error = ERROR_CODE_MESSAGE_INCOMPLETE;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = s_proprietary_incomplete;
        return false;
    }

    memcpy(
        &payload->proprietary.vendor_id, packed_payload,
        sizeof(payload->proprietary.vendor_id));
    packed_payload += sizeof(payload->proprietary.vendor_id);
    payload->proprietary.function = packed_payload[0];
    packed_payload++;
    if (packed_payload_len - 3 > 0) {
        payload->proprietary.data = packed_payload;
        payload->proprietary.data_len = packed_payload_len - 3;
    } else {
        payload->proprietary.data = NULL;
        payload->proprietary.data_len = 0;
    }
    return true;
}

/**
 * @brief Decode the BVLC-SC header
 * @param message - buffer with the message
 * @param message_len - length of the message
 * @param hdr - pointer to the decoded header
 * @param error - pointer to the error code
 * @param class - pointer to the error class
 * @param err_desc - pointer to the error description string
 * @return true if the data is decoded successfully, false otherwise
 */
static bool bvlc_sc_decode_hdr(
    uint8_t *message,
    size_t message_len,
    BVLC_SC_DECODED_HDR *hdr,
    BACNET_ERROR_CODE *error,
    BACNET_ERROR_CLASS *class,
    const char **err_desc)
{
    size_t offs = 4;
    bool ret = false;
    size_t hdr_opt_len = 0;

    memset(hdr, 0, sizeof(*hdr));

    if (message_len < 4) {
        /* According EA-001-4 'Clarifying BVLC-Result in BACnet/SC ' */
        /* If a BVLC message is received that has fewer than four octets, a */
        /* BVLC-Result NAK shall not be returned. */
        /* The message shall be discarded and not be processed. */
        *error = ERROR_CODE_DISCARD;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = NULL;
        return ret;
    }

    hdr->bvlc_function = message[0];
    memcpy(&hdr->message_id, &message[2], sizeof(hdr->message_id));

    if (message[1] & BVLC_SC_CONTROL_ORIG_VADDR) {
        hdr->origin = (BACNET_SC_VMAC_ADDRESS *)&message[offs];
        offs += BVLC_SC_VMAC_SIZE;
        if (offs > message_len) {
            hdr->origin = NULL;
            *error = ERROR_CODE_MESSAGE_INCOMPLETE;
            *class = ERROR_CLASS_COMMUNICATION;
            *err_desc = s_hdr_incomplete1;
            return false;
        }
    }

    if (message[1] & BVLC_SC_CONTROL_DEST_VADDR) {
        hdr->dest = (BACNET_SC_VMAC_ADDRESS *)&message[offs];
        offs += BVLC_SC_VMAC_SIZE;
        if (offs > message_len) {
            hdr->dest = NULL;
            *error = ERROR_CODE_MESSAGE_INCOMPLETE;
            *class = ERROR_CLASS_COMMUNICATION;
            *err_desc = s_hdr_incomplete2;
            return false;
        }
    }

    /* in order to handle correctly item AB.3.1.5 Common Error Situations, */
    /* in a case a BVLC message is received with unknown BVLC function, */
    /* upper layer logic must understand if the message is unicast or not. */
    /* That's why that error handling is put after filling of address fields. */

    if (message[0] > BVLC_SC_PROPRIETARY_MESSAGE) {
        *error = ERROR_CODE_BVLC_FUNCTION_UNKNOWN;
        *class = ERROR_CLASS_COMMUNICATION;
        *err_desc = s_unknown_bvlc_function;
        return ret;
    }

    if (message[1] & BVLC_SC_CONTROL_DEST_OPTIONS) {
        ret = bvlc_sc_validate_options_headers(
            BACNET_PDU_DEST_OPTION_VALIDATION, &message[offs],
            message_len - offs, &hdr_opt_len, &hdr->dest_options_num, error,
            class, err_desc);

        if (!ret) {
            return false;
        }
        hdr->dest_options = &message[offs];
        hdr->dest_options_len = hdr_opt_len;
        offs += hdr_opt_len;
    }

    if (message[1] & BVLC_SC_CONTROL_DATA_OPTIONS) {
        ret = bvlc_sc_validate_options_headers(
            BACNET_PDU_DATA_OPTION_VALIDATION, &message[offs],
            message_len - offs, &hdr_opt_len, &hdr->data_options_num, error,
            class, err_desc);
        if (!ret) {
            return false;
        }
        hdr->data_options = &message[offs];
        hdr->data_options_len = hdr_opt_len;
        offs += hdr_opt_len;
    }

    if (message_len >= offs) {
        /* no payload */
        return true;
    }

    hdr->payload = &message[offs];
    hdr->payload_len = message_len - offs;
    return true;
}

/**
 * @brief Function decodes the BACnet SC header options.
 * @param option_array - pointer to the array of decoded options
 * @param options_list - pointer to the packed options list
 * @return true if the data is decoded successfully, false otherwise
 */
static bool bvlc_sc_decode_header_options(
    BVLC_SC_DECODED_HDR_OPTION *option_array, uint8_t *options_list)
{
    uint8_t *next_option = options_list;
    size_t i = 0;

    while (next_option) {
        bvlc_sc_decode_option_hdr(
            options_list, &option_array[i].type,
            &option_array[i].must_understand, &next_option);

        option_array[i].packed_header_marker = options_list[0];

        if (option_array[i].type == BVLC_SC_OPTION_TYPE_PROPRIETARY) {
            bvlc_sc_decode_proprietary_option(
                options_list, &option_array[i].specific.proprietary.vendor_id,
                &option_array[i].specific.proprietary.option_type,
                &option_array[i].specific.proprietary.data,
                &option_array[i].specific.proprietary.data_len);
        }
        i++;
        options_list = next_option;
    }

    return true;
}

/**
 * @brief Function decodes the BACnet SC header options if they exist.
 * @param message - pointer to the decoded message
 */
static void
bvlc_sc_decode_dest_options_if_exists(BVLC_SC_DECODED_MESSAGE *message)
{
    if (message->hdr.dest_options) {
        bvlc_sc_decode_header_options(
            message->dest_options, message->hdr.dest_options);
    }
}

/**
 * @brief Function decodes the BACnet SC header options if they exist.
 * @param message - pointer to the decoded message
 */
static void
bvlc_sc_decode_data_options_if_exists(BVLC_SC_DECODED_MESSAGE *message)
{
    if (message->hdr.data_options) {
        bvlc_sc_decode_header_options(
            message->data_options, message->hdr.data_options);
    }
}

/**
 * @brief Function decodes BACNet/SC message.
 *
 * @param buf - A buffer which holds BACNet/SC PDU.
 * @param buf_len - length of a buffer which holds BACNet/SC PDU.
 * @param message- pointer to structure for decoded data.
 * @param error - the value of parameter is filled if function returns false.
 *                Check BACNET_ERROR_CLASS enum.
 * @param class - the value of parameter is filled if function returns false.
 *                heck BACNET_ERROR_CODE enum.
 * @return true if PDU was successfully decoded otherwise returns false,
 *         and error and class parameters are filled with corresponded codes.
 */
bool bvlc_sc_decode_message(
    uint8_t *buf,
    size_t buf_len,
    BVLC_SC_DECODED_MESSAGE *message,
    BACNET_ERROR_CODE *error,
    BACNET_ERROR_CLASS *class,
    const char **err_desc)
{
    if (!message || !buf_len || !error || !class || !buf || !err_desc) {
        return false;
    }

    memset(message->data_options, 0, sizeof(message->data_options));
    memset(message->dest_options, 0, sizeof(message->dest_options));
    memset(&message->payload, 0, sizeof(message->payload));

    if (!bvlc_sc_decode_hdr(
            buf, buf_len, &message->hdr, error, class, err_desc)) {
        return false;
    }

    if (message->hdr.dest_options) {
        if (message->hdr.dest_options_num > BVLC_SC_HEADER_OPTION_MAX) {
            /* dest header options list is too long */
            *error = ERROR_CODE_OUT_OF_MEMORY;
            *class = ERROR_CLASS_RESOURCES;
            *err_desc = s_dest_options_list_too_long;
            return false;
        }
    }

    if (message->hdr.data_options) {
        if (message->hdr.data_options_num > BVLC_SC_HEADER_OPTION_MAX) {
            /* data header options list is too long */
            *error = ERROR_CODE_OUT_OF_MEMORY;
            *class = ERROR_CLASS_RESOURCES;
            *err_desc = s_data_options_list_too_long;
            return false;
        }
    }

    switch (message->hdr.bvlc_function) {
        case BVLC_SC_RESULT: {
            if (message->hdr.data_options) {
                /* The BVLC-Result message must not have data options */
                *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_result_unexpected_data_options;
                return false;
            }

            if (!message->hdr.payload || !message->hdr.payload_len) {
                *error = ERROR_CODE_PAYLOAD_EXPECTED;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_result_payload_expected;
                return false;
            }

            bvlc_sc_decode_dest_options_if_exists(message);

            if (!bvlc_sc_decode_result(
                    &message->payload, message->hdr.payload,
                    message->hdr.payload_len, error, class, err_desc)) {
                return false;
            }
            break;
        }
        case BVLC_SC_ENCAPSULATED_NPDU: {
            if (!message->hdr.payload || !message->hdr.payload_len) {
                *error = ERROR_CODE_PAYLOAD_EXPECTED;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_encapsulated_npdu_payload_expected;
                return false;
            }

            bvlc_sc_decode_dest_options_if_exists(message);
            bvlc_sc_decode_data_options_if_exists(message);

            message->payload.encapsulated_npdu.npdu = message->hdr.payload;
            message->payload.encapsulated_npdu.npdu_len =
                message->hdr.payload_len;
            break;
        }
        case BVLC_SC_ADDRESS_RESOLUTION: {
            if (message->hdr.data_options) {
                /* The Address-Resolution message must not have data options */
                *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_address_resolution_data_options;
                return false;
            }

            if (message->hdr.payload || message->hdr.payload_len) {
                /* According EA-001-4 'Clarifying BVLC-Result in BACnet/SC */
                /* If a BVLC message is received that is longer than expected,
                 */
                /* a BVLC-Result NAK shall be returned if it was a unicast */
                /* message, indicating an 'Error Class' of COMMUNICATION and */
                /* 'Error Code' of UNEXPECTED_DATA. The message shall be */
                /* discarded and not be processed. */

                *error = ERROR_CODE_UNEXPECTED_DATA;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_address_resolution_unexpected;
                return false;
            }

            bvlc_sc_decode_dest_options_if_exists(message);
            break;
        }
        case BVLC_SC_ADDRESS_RESOLUTION_ACK: {
            if (message->hdr.data_options) {
                /* The Address-Resolution Ack message must not have data options
                 */
                *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_address_resolution_ack_data_options;
                return false;
            }

            bvlc_sc_decode_dest_options_if_exists(message);

            if (message->hdr.payload_len) {
                message->payload.address_resolution_ack
                    .utf8_websocket_uri_string = message->hdr.payload;
            } else {
                message->payload.address_resolution_ack
                    .utf8_websocket_uri_string = NULL;
            }
            message->payload.address_resolution_ack
                .utf8_websocket_uri_string_len = message->hdr.payload_len;
            break;
        }
        case BVLC_SC_ADVERTISIMENT: {
            if (message->hdr.data_options) {
                /* The advertisiment message must not have data options */
                *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_advertisiment_data_options;
                return false;
            }

            if (!message->hdr.payload || !message->hdr.payload_len) {
                *error = ERROR_CODE_PAYLOAD_EXPECTED;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_advertisiment_payload_expected;
                return false;
            }

            bvlc_sc_decode_dest_options_if_exists(message);

            if (!bvlc_sc_decode_advertisiment(
                    &message->payload, message->hdr.payload,
                    message->hdr.payload_len, error, class, err_desc)) {
                return false;
            }
            break;
        }
        case BVLC_SC_ADVERTISIMENT_SOLICITATION: {
            if (message->hdr.data_options) {
                /* The advertisiment solicitation message must not have data
                 * options */
                *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_advertisiment_solicitation_data_options;
                return false;
            }

            if (message->hdr.payload || message->hdr.payload_len) {
                *error = ERROR_CODE_UNEXPECTED_DATA;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_advertisiment_solicitation_payload_expected;
                return false;
            }

            bvlc_sc_decode_dest_options_if_exists(message);
            break;
        }
        case BVLC_SC_CONNECT_REQUEST:
        case BVLC_SC_CONNECT_ACCEPT:
        case BVLC_SC_DISCONNECT_REQUEST:
        case BVLC_SC_DISCONNECT_ACK:
        case BVLC_SC_HEARTBEAT_REQUEST:
        case BVLC_SC_HEARTBEAT_ACK: {
            if (message->hdr.origin != 0) {
                *error = ERROR_CODE_HEADER_ENCODING_ERROR;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_origin_unexpected;
                return false;
            }

            if (message->hdr.dest != 0) {
                *error = ERROR_CODE_HEADER_ENCODING_ERROR;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_dest_unexpected;
                return false;
            }

            if (message->hdr.data_options) {
                *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_data_option_unexpected;
                return false;
            }

            if (message->hdr.bvlc_function == BVLC_SC_CONNECT_REQUEST ||
                message->hdr.bvlc_function == BVLC_SC_CONNECT_ACCEPT) {
                if (!message->hdr.payload || !message->hdr.payload_len) {
                    *error = ERROR_CODE_PAYLOAD_EXPECTED;
                    *class = ERROR_CLASS_COMMUNICATION;
                    *err_desc = s_absent_payload;
                    return false;
                }
            } else {
                if (message->hdr.payload || message->hdr.payload_len) {
                    *error = ERROR_CODE_UNEXPECTED_DATA;
                    *class = ERROR_CLASS_COMMUNICATION;
                    *err_desc = s_message_too_long;
                    return false;
                }
            }

            bvlc_sc_decode_dest_options_if_exists(message);

            if (message->hdr.bvlc_function == BVLC_SC_CONNECT_REQUEST) {
                if (!bvlc_sc_decode_connect_request(
                        &message->payload, message->hdr.payload,
                        message->hdr.payload_len, error, class, err_desc)) {
                    return false;
                }
            } else if (message->hdr.bvlc_function == BVLC_SC_CONNECT_ACCEPT) {
                if (!bvlc_sc_decode_connect_accept(
                        &message->payload, message->hdr.payload,
                        message->hdr.payload_len, error, class, err_desc)) {
                    return false;
                }
            }
            break;
        }
        case BVLC_SC_PROPRIETARY_MESSAGE: {
            if (message->hdr.data_options) {
                /* The proprietary message must not have data options */
                *error = ERROR_CODE_INCONSISTENT_PARAMETERS;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_proprietary_data_options;
                return false;
            }

            if (!message->hdr.payload || !message->hdr.payload_len) {
                *error = ERROR_CODE_PAYLOAD_EXPECTED;
                *class = ERROR_CLASS_COMMUNICATION;
                *err_desc = s_proprietary_payload;
                return false;
            }

            bvlc_sc_decode_dest_options_if_exists(message);

            if (!bvlc_sc_decode_proprietary(
                    &message->payload, message->hdr.payload,
                    message->hdr.payload_len, error, class, err_desc)) {
                return false;
            }
            break;
        }
        default:
            *error = ERROR_CODE_BVLC_FUNCTION_UNKNOWN;
            *class = ERROR_CLASS_COMMUNICATION;
            *err_desc = s_unknown_bvlc_function;
            return false;
    }
    return true;
}

/**
 * @brief Function removes destination address of BACNet/SC message
 *                 and sets originating address instead of it.
 *                 It does it job only if message has destination address
 *                 and does not have origination address, otherwise pdu
 *                 stays unchanged.
 * @param pdu - BACNet/SC PDU.
 * @param pdu_len - length of a buffer which holds BACNet/SC PDU.
 * @param orig- origination vmac.
 */
void bvlc_sc_remove_dest_set_orig(
    uint8_t *pdu, size_t pdu_len, BACNET_SC_VMAC_ADDRESS *orig)
{
    size_t offs = 4;
    if (pdu && pdu_len > 4) {
        if (!(pdu[1] & BVLC_SC_CONTROL_ORIG_VADDR) &&
            (pdu[1] & BVLC_SC_CONTROL_DEST_VADDR)) {
            pdu[1] &= ~(BVLC_SC_CONTROL_DEST_VADDR);
            pdu[1] |= BVLC_SC_CONTROL_ORIG_VADDR;
            memcpy(&pdu[offs], orig, sizeof(*orig));
        }
    }
}

/**
 * @brief Function changes or adds originating address into BACNet/SC message.
 *                 It is assumed that ppdu points to buffer with pdu that has
 *                 BSC_PRE bytes behind.
 * @param ppdu - pointer to buffer which holds BACNet/SC PDU.
 * @param pdu_len - length of a buffer which holds BACNet/SC PDU.
 * @param orig- origination vmac.
 * @return new pdu length if function succeeded and ppdu points to beginning of
 *         changed pdu, otherwise returns old pdu_len and ppdu is not changed.
 */
size_t
bvlc_sc_set_orig(uint8_t **ppdu, size_t pdu_len, BACNET_SC_VMAC_ADDRESS *orig)
{
    uint8_t buf[BSC_PRE];
    uint8_t *pdu = *ppdu;
    size_t offs = 4;
    if (pdu && pdu_len > 4) {
        if (pdu[1] & BVLC_SC_CONTROL_ORIG_VADDR) {
            memcpy(&pdu[offs], orig, sizeof(orig->address));
            return pdu_len;
        } else {
            memcpy(buf, pdu, 4);
            buf[1] |= BVLC_SC_CONTROL_ORIG_VADDR;
            memcpy(&buf[4], orig, sizeof(orig->address));
            memcpy(pdu - BVLC_SC_VMAC_SIZE, buf, BVLC_SC_VMAC_SIZE + 4);
            *ppdu = pdu - BVLC_SC_VMAC_SIZE;
            return pdu_len + BVLC_SC_VMAC_SIZE;
        }
    } else {
        return pdu_len;
    }
}

/**
 * @brief Function checks if vmac address is broadcast.
 * @param vmac - pointer vmac address.
 * @return true if vmac is broadcast. otherwise returns false.
 */
bool bvlc_sc_is_vmac_broadcast(BACNET_SC_VMAC_ADDRESS *vmac)
{
    int i;
    for (i = 0; i < BVLC_SC_VMAC_SIZE; i++) {
        if (vmac->address[i] != 0xFF) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Function checks if it is needed to send BVLC result
 *        response message for given decoded BACNet/SC message.
 *        In a case of errors, standard requires to send such kind
 *        of responses for unicast messages of specific types.
 * @param dm - pointer to decoded BACNet/SC message.
 * @return true if vmac is broadcast, otherwise returns false.
 */
bool bvlc_sc_need_send_bvlc_result(BVLC_SC_DECODED_MESSAGE *dm)
{
    if (dm->hdr.dest == NULL || !bvlc_sc_is_vmac_broadcast(dm->hdr.dest)) {
        if (dm->hdr.bvlc_function == BVLC_SC_CONNECT_REQUEST ||
            dm->hdr.bvlc_function == BVLC_SC_DISCONNECT_REQUEST ||
            dm->hdr.bvlc_function == BVLC_SC_ENCAPSULATED_NPDU ||
            dm->hdr.bvlc_function == BVLC_SC_ADDRESS_RESOLUTION ||
            dm->hdr.bvlc_function == BVLC_SC_ADVERTISIMENT_SOLICITATION ||
            dm->hdr.bvlc_function == BVLC_SC_HEARTBEAT_REQUEST ||
            dm->hdr.bvlc_function > BVLC_SC_PROPRIETARY_MESSAGE) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Function checks if destination address of input BACNet/SC
 *        message is broadcast.
 * @param  pdu- buffer with BACNet/SC message.
 * @param  pdu_len- length of buffer of BACNet/SC message.
 * @return true if destination address of input BACNet/SC
 *         is broadcast, otherwise returns false.
 */
bool bvlc_sc_pdu_has_dest_broadcast(uint8_t *pdu, size_t pdu_len)
{
    size_t offs = 4;

    if (pdu_len >= 4) {
        if (pdu[1] & BVLC_SC_CONTROL_ORIG_VADDR) {
            offs += BVLC_SC_VMAC_SIZE;
        }
        if (pdu[1] & BVLC_SC_CONTROL_DEST_VADDR &&
            (offs + BVLC_SC_VMAC_SIZE) <= pdu_len) {
            return bvlc_sc_is_vmac_broadcast(
                (BACNET_SC_VMAC_ADDRESS *)&pdu[offs]);
        }
    }
    return false;
}

/**
 * @brief Function checks if input BACNet/SC message has
 *        destination address field.
 * @param  pdu- buffer with BACNet/SC message.
 * @param  pdu_len- length of buffer of BACNet/SC message.
 * @return true if destination address is presented in
 *         input BACNet/SC message, otherwise returns false.
 */
bool bvlc_sc_pdu_has_no_dest(uint8_t *pdu, size_t pdu_len)
{
    if (pdu_len >= 4) {
        if (pdu[1] & BVLC_SC_CONTROL_DEST_VADDR) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Function puts destination address of
 *        BACNet/SC message into vmac if message
 *        contains it.
 * @param  pdu- buffer with BACNet/SC message.
 * @param  pdu_len- length of buffer of BACNet/SC message.
 * @return true if destination address is presented and was
 *         placed into vmac, otherwise returns false.
 */
bool bvlc_sc_pdu_get_dest(
    uint8_t *pdu, size_t pdu_len, BACNET_SC_VMAC_ADDRESS *vmac)
{
    size_t offs = 4;

    if (pdu_len >= 4) {
        if (pdu[1] & BVLC_SC_CONTROL_ORIG_VADDR) {
            offs += BVLC_SC_VMAC_SIZE;
        }
        if (pdu[1] & BVLC_SC_CONTROL_DEST_VADDR &&
            (offs + BVLC_SC_VMAC_SIZE) <= pdu_len) {
            memcpy(&vmac->address[0], &pdu[offs], BVLC_SC_VMAC_SIZE);
            return true;
        }
    }
    return false;
}

/**
 * @brief Function removes originating and destination
 *        address fields from input BACNet/SC message.
 * @param  ppdu- pointer to buffer of  BACNet/SC message.
 * @param  pdu_len- length of buffer of BACNet/SC message.
 * @return new length of changed pdu if originating or destination
 *         addresses were removed or old pdu length if
 *         pdu was not changed. If pdu was changed, ppdu contains
 *         updated pointer to buffer to modified BACNet/SC message.
 */
size_t bvlc_sc_remove_orig_and_dest(uint8_t **ppdu, size_t pdu_len)
{
    uint8_t *pdu = *ppdu;
    size_t offs = 4;

    if (pdu_len > 4) {
        if (pdu[1] & BVLC_SC_CONTROL_ORIG_VADDR) {
            offs += BVLC_SC_VMAC_SIZE;
        }
        if (pdu[1] & BVLC_SC_CONTROL_DEST_VADDR) {
            offs += BVLC_SC_VMAC_SIZE;
        }
        pdu[1] &= ~(BVLC_SC_CONTROL_ORIG_VADDR);
        pdu[1] &= ~(BVLC_SC_CONTROL_DEST_VADDR);
        memmove(&pdu[offs - 4], pdu, 4);
        *ppdu = &pdu[offs - 4];
        return pdu_len - offs + 4;
    }
    return pdu_len;
}
