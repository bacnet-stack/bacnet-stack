/**
 * @file
 * @brief Encode/Decode You-Are-Request service
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
 * @brief Encode a You-Are-Request APDU
 *
 *  You-Are-Request ::= SEQUENCE {
 *      vendor-id          Unsigned16,
 *      model-name         CharacterString,
 *      serial-number      CharacterString,
 *      device-identifier  BACnetObjectIdentifier OPTIONAL,
 *      device-mac-address OctetString OPTIONAL
 *  }
 *
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL for
 *  length determination
 * @param device_id the Device Object_Identifier to be assigned
 *  in the qualified device.
 * @param vendor_id the identity of the vendor of the device that
 *  is qualified to receive this You-Are service request.
 * @param model_name the model name of the device qualified to receive
 *  the You-Are service request.
 * @param serial_number the serial identifier of the device qualified
 *  to receive the You-Are service request.
 * @param mac_address the device MAC address that is to be configured
 *  in the qualified device.
 * @return The length of the apdu encoded
 */
int you_are_request_encode(
    uint8_t *apdu,
    uint32_t device_id,
    uint16_t vendor_id,
    const BACNET_CHARACTER_STRING *model_name,
    const BACNET_CHARACTER_STRING *serial_number,
    const BACNET_OCTET_STRING *mac_address)
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
    if (apdu) {
        apdu += len;
    }
    if (device_id < BACNET_MAX_INSTANCE) {
        len = encode_application_unsigned(apdu, device_id);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    if (mac_address && mac_address->length) {
        len = encode_application_octet_string(apdu, mac_address);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode a You-Are-Request unconfirmed service APDU
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL for
 *  length determination
 * @param device_id the Device Object_Identifier to be assigned
 *  in the qualified device.
 * @param vendor_id the identity of the vendor of the device that
 *  is qualified to receive this You-Are service request.
 * @param model_name the model name of the device qualified to receive
 *  the You-Are service request.
 * @param serial_number the serial identifier of the device qualified
 *  to receive the You-Are service request.
 * @param mac_address the device MAC address that is to be configured
 *  in the qualified device.
 * @return The length of the apdu encoded
 */
int you_are_request_service_encode(
    uint8_t *apdu,
    uint32_t device_id,
    uint16_t vendor_id,
    const BACNET_CHARACTER_STRING *model_name,
    const BACNET_CHARACTER_STRING *serial_number,
    const BACNET_OCTET_STRING *mac_address)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_YOU_ARE;
    }
    len = 2;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = you_are_request_encode(
        apdu, device_id, vendor_id, model_name, serial_number, mac_address);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode a You-Are-Request APDU
 * @param apdu [in] Buffer containing the APDU
 * @param apdu_size [in] The length of the APDU
 * @param device_id the Device Object_Identifier to be assigned
 *  in the qualified device.
 * @param vendor_id the identity of the vendor of the device that
 *  is qualified to receive this You-Are service request.
 * @param model_name the model name of the device qualified to receive
 *  the You-Are service request.
 * @param serial_number the serial identifier of the device qualified
 *  to receive the You-Are service request.
 * @param mac_address the device MAC address that is to be configured
 *  in the qualified device.
 * @return The number of bytes decoded , or #BACNET_STATUS_ERROR on error
 */
int you_are_request_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    uint32_t *device_id,
    uint16_t *vendor_id,
    BACNET_CHARACTER_STRING *model_name,
    BACNET_CHARACTER_STRING *serial_number,
    BACNET_OCTET_STRING *mac_address)
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
    len = bacnet_unsigned_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (unsigned_value <= UINT32_MAX) {
            if (device_id) {
                *device_id = (uint32_t)unsigned_value;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else if (len == 0) {
        /* optional - skip apdu_len increment */
        if (device_id) {
            *device_id = UINT32_MAX;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_octet_string_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, mac_address);
    if (len > 0) {
        apdu_len += len;
    } else if (len == 0) {
        /* optional - skip apdu_len increment */
        if (mac_address) {
            mac_address->length = 0;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}
