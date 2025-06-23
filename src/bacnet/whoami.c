/**
 * @file
 * @brief Encode/Decode Who-Am-I requests
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2025
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/whoami.h"

/**
 * @brief Encode a Who-Am-I-Request APDU
 *
 *  Who-Am-I-Request ::= SEQUENCE {
 *      vendor-id Unsigned16,
 *      model-name CharacterString,
 *      serial-number CharacterString
 *  }
 *
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL for
 *  length determination
 * @param vendor_id the identity of the vendor of the device initiating
 *  the Who-Am-I service request.
 * @param model_name the model name of the device initiating the Who-Am-I
 *  service request.
 * @param serial_number the serial identifier of the device initiating
 *  the Who-Am-I service request.
 * @return The length of the apdu encoded
 */
int who_am_i_request_encode(
    uint8_t *apdu,
    uint16_t vendor_id,
    const BACNET_CHARACTER_STRING *model_name,
    const BACNET_CHARACTER_STRING *serial_number)
{
    int len = 0;
    int apdu_len = 0;

    len = encode_application_unsigned(apdu, vendor_id);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_character_string(apdu, model_name);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_character_string(apdu, serial_number);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode a Who-Am-I-Request unconfirmed service APDU
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL for
 *  length determination
 * @param vendor_id the identity of the vendor of the device initiating
 *  the Who-Am-I service request.
 * @param model_name the model name of the device initiating the Who-Am-I
 *  service request.
 * @param serial_number the serial identifier of the device initiating
 *  the Who-Am-I service request.
 * @return The length of the apdu encoded
 */
int who_am_i_request_service_encode(
    uint8_t *apdu,
    uint16_t vendor_id,
    const BACNET_CHARACTER_STRING *model_name,
    const BACNET_CHARACTER_STRING *serial_number)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_WHO_AM_I; /* service choice */
    }
    len = 2;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = who_am_i_request_encode(apdu, vendor_id, model_name, serial_number);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode a Who-Is-Request APDU
 * @param apdu [in] Buffer containing the APDU
 * @param apdu_size [in] The length of the APDU
 * @param vendor_id the identity of the vendor of the device initiating
 *  the Who-Am-I service request.
 * @param model_name the model name of the device initiating the Who-Am-I
 *  service request.
 * @param serial_number the serial identifier of the device initiating
 *  the Who-Am-I service request.
 * @return The number of bytes decoded , or #BACNET_STATUS_ERROR on error
 */
int who_am_i_request_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    uint16_t *vendor_id,
    BACNET_CHARACTER_STRING *model_name,
    BACNET_CHARACTER_STRING *serial_number)
{
    int len = 0, apdu_len = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size == 0) {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_unsigned_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &unsigned_value);
    if (len > 0) {
        if (unsigned_value <= UINT16_MAX) {
            if (vendor_id) {
                *vendor_id = (uint16_t)unsigned_value;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    len = bacnet_character_string_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, model_name);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_character_string_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, serial_number);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}
