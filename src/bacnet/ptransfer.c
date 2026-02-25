/**
 * @file
 * @brief BACnet PrivateTransfer encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2009
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/ptransfer.h"

/**
 * @brief Encode the service parameters for both confirmed and
 *  unconfirmed private transfer
 *
 * ConfirmedPrivateTransfer-Request ::= SEQUENCE {
 *   vendorID               [0] Unsigned,
 *   serviceNumber          [1] Unsigned,
 *   serviceParameters      [2] ABSTRACT-SYNTAX.&Type OPTIONAL
 * }
 * UnconfirmedPrivateTransfer-Request ::= SEQUENCE {
 *   vendorID               [0] Unsigned,
 *   serviceNumber          [1] Unsigned,
 *   serviceParameters      [2] ABSTRACT-SYNTAX.&Type OPTIONAL
 * }
 *
 * @param apdu [in] The APDU buffer for encoding, or NULL for length
 * @param apdu_size [in] The maximum APDU size for encoding
 * @param private_data [in] The BACNET_PRIVATE_TRANSFER_DATA structure to encode
 * @return number of bytes encoded, or negative on error
 */
int private_transfer_request_encode(
    uint8_t *apdu, const BACNET_PRIVATE_TRANSFER_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    len = encode_context_unsigned(apdu, 0, data->vendorID);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_unsigned(apdu, 1, data->serviceNumber);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_opening_tag(apdu, 2);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    for (len = 0; len < data->serviceParametersLen; len++) {
        if (apdu) {
            *apdu = data->serviceParameters[len];
            apdu++;
        }
        apdu_len++;
    }
    len = encode_closing_tag(apdu, 2);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode the service parameters for both confirmed and
 *  unconfirmed private transfer, with APDU length checking
 * @param apdu [in] The APDU buffer for encoding, or NULL for length
 * @param apdu_size [in] The maximum APDU size for encoding
 * @param private_data [in] The BACNET_PRIVATE_TRANSFER_DATA structure to encode
 * @return number of bytes encoded, or zero on error
 */
int private_transfer_request_service_encode(
    uint8_t *apdu, uint16_t apdu_size, const BACNET_PRIVATE_TRANSFER_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = private_transfer_request_encode(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = private_transfer_request_encode(apdu, data);
    }

    return apdu_len;
}

/**
 * @brief Encode a ConfirmedPrivateTransfer-Error APDU
 * ConfirmedPrivateTransfer-Error ::= SEQUENCE {
 *  errorType       [0] Error,
 *  vendorID        [1] Unsigned,
 *  serviceNumber [2] Unsigned,
 *  errorParameters [3] ABSTRACT-SYNTAX.&Type OPTIONAL
 * }
 * @param apdu [in] The APDU buffer for encoding, or NULL for length
 * @param apdu_size [in] The maximum APDU size for encoding
 * @param invoke_id [in] The invoke ID for the error response
 * @param data [in] The BACNET_PRIVATE_TRANSFER_DATA structure to encode
 * @return number of bytes encoded, or zero on error
 */
int ptransfer_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, const BACNET_PRIVATE_TRANSFER_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_PRIVATE_TRANSFER;
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = private_transfer_request_encode(apdu, data);
    apdu_len += len;

    return apdu_len;
}

int uptransfer_encode_apdu(
    uint8_t *apdu, const BACNET_PRIVATE_TRANSFER_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    if (apdu) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_PRIVATE_TRANSFER;
    }
    len = 2;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = private_transfer_request_encode(apdu, data);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode the service parameters for both confirmed and
 * unconfirmed private transfer
 * ConfirmedPrivateTransfer-Request ::= SEQUENCE {
 *   vendorID               [0] Unsigned,
 *   serviceNumber          [1] Unsigned,
 *   serviceParameters      [2] ABSTRACT-SYNTAX.&Type OPTIONAL
 * }
 * UnconfirmedPrivateTransfer-Request ::= SEQUENCE {
 *   vendorID               [0] Unsigned,
 *   serviceNumber          [1] Unsigned,
 *   serviceParameters      [2] ABSTRACT-SYNTAX.&Type OPTIONAL
 * }
 * @param apdu [in] The APDU buffer for decoding
 * @param apdu_size [in] The length of the APDU buffer for decoding
 * @param private_data [out] The BACNET_PRIVATE_TRANSFER_DATA structure to
 * decode into
 * @return number of bytes decoded, or negative on error
 */
int ptransfer_decode_service_request(
    uint8_t *apdu, unsigned apdu_size, BACNET_PRIVATE_TRANSFER_DATA *data)
{
    int apdu_len = 0, len = 0, application_len = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    /* check for value pointers */
    if (!(apdu && apdu_size)) {
        return BACNET_STATUS_ERROR;
    }
    /* Tag 0: vendorID */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &unsigned_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (data) {
        if (unsigned_value > UINT16_MAX) {
            data->vendorID = UINT16_MAX;
        } else {
            data->vendorID = (uint16_t)unsigned_value;
        }
    }
    /* Tag 1: serviceNumber */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &unsigned_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (data) {
        if (unsigned_value > UINT32_MAX) {
            data->serviceNumber = UINT32_MAX;
        } else {
            data->serviceNumber = (uint32_t)unsigned_value;
        }
    }
    /* Tag 2: serviceParameters */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
        application_len =
            bacnet_enclosed_data_length(&apdu[apdu_len], apdu_size - apdu_len);
        if (application_len < 0) {
            return BACNET_STATUS_ERROR;
        }
        /* add the tag length */
        apdu_len += len;
        /* postpone decoding the serviceParameters */
        if (data) {
            data->serviceParameters = &apdu[apdu_len];
            data->serviceParametersLen = application_len;
        }
        apdu_len += application_len;
        if (bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Encode an Error acknowledge for a Private Transfer service.
 * ConfirmedPrivateTransfer-Error ::= SEQUENCE {
 *  errorType       [0] Error,
 *  vendorID        [1] Unsigned,
 *  serviceNumber [2] Unsigned,
 *  errorParameters [3] ABSTRACT-SYNTAX.&Type OPTIONAL
 * }
 * @param apdu [in] The APDU buffer for encoding, or NULL for length
 * @param error_class [in] The BACNET_ERROR_CLASS to encode
 * @param error_code [in] The BACNET_ERROR_CODE to encode
 * @param private_data [in] The BACNET_PRIVATE_TRANSFER_DATA structure to encode
 * @return number of bytes encoded, or zero on error
 */
int ptransfer_error_encode_service(
    uint8_t *apdu,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code,
    const BACNET_PRIVATE_TRANSFER_DATA *private_data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0; /* length of the part of the encoding */

    len = encode_opening_tag(apdu, 0);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_enumerated(apdu, error_class);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_enumerated(apdu, error_code);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, 0);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_unsigned(apdu, 1, private_data->vendorID);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_unsigned(apdu, 2, private_data->serviceNumber);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_opening_tag(apdu, 3);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    for (len = 0; len < private_data->serviceParametersLen; len++) {
        if (apdu) {
            *apdu = private_data->serviceParameters[len];
            apdu++;
        }
        apdu_len++;
    }
    len = encode_closing_tag(apdu, 3);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode an Error acknowledge for a Private Transfer service.
 * ConfirmedPrivateTransfer-Error ::= SEQUENCE {
 *  errorType       [0] Error,
 *  vendorID        [1] Unsigned,
 *  serviceNumber [2] Unsigned,
 *  errorParameters [3] ABSTRACT-SYNTAX.&Type OPTIONAL
 * }
 * @param apdu [in] The APDU buffer for encoding, or NULL for length
 * @param apdu_size [in] The maximum APDU size for encoding
 * @param invoke_id [in] The invoke ID for the error response
 * @param error_class [in] The BACNET_ERROR_CLASS to encode
 * @param error_code [in] The BACNET_ERROR_CODE to encode
 * @param private_data [in] The BACNET_PRIVATE_TRANSFER_DATA structure to encode
 * @return number of bytes encoded, or zero on error
 */
int ptransfer_error_encode_apdu(
    uint8_t *apdu,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code,
    const BACNET_PRIVATE_TRANSFER_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0; /* length of the part of the encoding */

    if (apdu) {
        apdu[0] = PDU_TYPE_ERROR;
        apdu[1] = invoke_id;
        apdu[2] = SERVICE_CONFIRMED_PRIVATE_TRANSFER;
    }
    len = 3;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* service parameters */
    len = ptransfer_error_encode_service(apdu, error_class, error_code, data);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode an Error acknowledge for a Private Transfer service.
 * @param apdu [in] The APDU buffer for decoding.
 * @param apdu_size [in] The length of the APDU buffer.
 * @param error_class [out] The decoded error class.
 * @param error_code [out] The decoded error code.
 * @param private_data [out] The decoded private transfer data.
 * @return number of bytes decoded, or negative on error.
 */
int ptransfer_error_decode_service_request(
    uint8_t *apdu,
    unsigned apdu_size,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code,
    BACNET_PRIVATE_TRANSFER_DATA *private_data)
{
    int apdu_len = 0, len = 0;
    int application_len = 0;
    uint32_t enum_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    /* check for value pointer and minimum size */
    if (!(apdu && apdu_size)) {
        return BACNET_STATUS_ERROR;
    }
    /* Tag 0: Error */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* error class */
    len = bacnet_enumerated_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &enum_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (error_class) {
        *error_class = (BACNET_ERROR_CLASS)enum_value;
    }
    /* error code */
    len = bacnet_enumerated_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &enum_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (error_code) {
        *error_code = (BACNET_ERROR_CODE)enum_value;
    }
    if (bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* Tag 1: vendorID */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &unsigned_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (private_data) {
        private_data->vendorID = (uint16_t)unsigned_value;
    }
    /* Tag 2: serviceNumber */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &unsigned_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (private_data) {
        private_data->serviceNumber = (uint32_t)unsigned_value;
    }
    /* Tag 3: serviceParameters */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 3, &len)) {
        application_len =
            bacnet_enclosed_data_length(&apdu[apdu_len], apdu_size - apdu_len);
        if (application_len < 0) {
            return BACNET_STATUS_ERROR;
        }
        /* update the tag length */
        apdu_len += len;
        /* postpone serviceParameters decoding */
        if (private_data) {
            private_data->serviceParameters = &apdu[apdu_len];
            private_data->serviceParametersLen = application_len;
        }
        apdu_len += application_len;
        if (bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 3, &len)) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

int ptransfer_ack_encode_apdu(
    uint8_t *apdu,
    uint8_t invoke_id,
    const BACNET_PRIVATE_TRANSFER_DATA *private_data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK; /* complex ACK service */
        apdu[1] = invoke_id; /* original invoke id from request */
        apdu[2] = SERVICE_CONFIRMED_PRIVATE_TRANSFER; /* service choice */
        apdu_len = 3;
        /* service ack follows */
        /*
                ConfirmedPrivateTransfer-ACK ::= SEQUENCE {
                vendorID               [0] Unsigned,
                serviceNumber          [1] Unsigned,
                resultBlock            [2] ABSTRACT-SYNTAX.&Type OPTIONAL
            }
        */
        len =
            encode_context_unsigned(&apdu[apdu_len], 0, private_data->vendorID);
        apdu_len += len;
        len = encode_context_unsigned(
            &apdu[apdu_len], 1, private_data->serviceNumber);
        apdu_len += len;
        len = encode_opening_tag(&apdu[apdu_len], 2);
        apdu_len += len;
        for (len = 0; len < private_data->serviceParametersLen; len++) {
            apdu[apdu_len] = private_data->serviceParameters[len];
            apdu_len++;
        }
        len = encode_closing_tag(&apdu[apdu_len], 2);
        apdu_len += len;
    }

    return apdu_len;
}

/* ptransfer_ack_decode_service_request() is the same as
       ptransfer_decode_service_request */
