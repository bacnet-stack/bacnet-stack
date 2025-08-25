/**
 * @file
 * @brief I-Have service encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"
#include "bacnet/dcc.h"
#include "bacnet/bacdcode.h"
#include "bacnet/iam.h"

/**
 * @brief Encode the I-Am Request.
 * @param apdu  Transmit buffer
 * @param device_id  Device Id
 * @param max_apdu  Transmit buffer size.
 * @param segmentation  True, if segmentation shall be featured.
 * @param vendor_id  Vendor Id
 * @return Total length of the apdu, zero otherwise.
 */
int bacnet_iam_request_encode(
    uint8_t *apdu,
    uint32_t device_id,
    unsigned max_apdu,
    int segmentation,
    uint16_t vendor_id)
{
    int len; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    len = encode_application_object_id(apdu, OBJECT_DEVICE, device_id);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_unsigned(apdu, max_apdu);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_enumerated(apdu, (uint32_t)segmentation);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_unsigned(apdu, vendor_id);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode the WriteGroup service request
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t bacnet_iam_service_request_encode(
    uint8_t *apdu,
    size_t apdu_size,
    uint32_t device_id,
    unsigned max_apdu,
    int segmentation,
    uint16_t vendor_id)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = bacnet_iam_request_encode(
        NULL, device_id, max_apdu, segmentation, vendor_id);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = bacnet_iam_request_encode(
            apdu, device_id, max_apdu, segmentation, vendor_id);
    }

    return apdu_len;
}

/**
 * @brief Encode the I-Am service.
 *
 * @param apdu  Transmit buffer
 * @param device_id  Device Id
 * @param max_apdu  Transmit buffer size.
 * @param segmentation  True, if segmentation shall be featured.
 * @param vendor_id  Vendor Id
 *
 * @return Total length of the apdu, zero otherwise.
 */
int iam_encode_apdu(
    uint8_t *apdu,
    uint32_t device_id,
    unsigned max_apdu,
    int segmentation,
    uint16_t vendor_id)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_I_AM; /* service choice */
    }
    len = 2;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacnet_iam_request_encode(
        apdu, device_id, max_apdu, segmentation, vendor_id);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode the I-Am-Request.
 * @ingroup BIBB-DM-DOB
 * @param apdu [in] Buffer containing the APDU
 * @param apdu_size [in] The length of the APDU
 * @param pDevice_id  Pointer to the variable that shall take the device Id.
 * @param pMax_apdu  Pointer to a variable that shall take the decoded length.
 * @param pSegmentation  Pointer to a variable taking if segmentation is used.
 * @param pVendor_id  Pointer to a variable taking the vendor id.
 * @return The number of bytes decoded , or #BACNET_STATUS_ERROR on error
 */
int bacnet_iam_request_decode(
    const uint8_t *apdu,
    unsigned apdu_size,
    uint32_t *pDevice_id,
    unsigned *pMax_apdu,
    int *pSegmentation,
    uint16_t *pVendor_id)
{
    int len = 0;
    int apdu_len = 0; /* total length of the apdu, return value */
    /* should be a Device Object */
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;
    uint32_t enum_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    /* OBJECT ID - object id */
    len = bacnet_object_id_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &object_type, &object_instance);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (object_type != OBJECT_DEVICE) {
        return BACNET_STATUS_ERROR;
    }
    if (pDevice_id) {
        *pDevice_id = object_instance;
    }
    /* MAX APDU - unsigned */
    len = bacnet_unsigned_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &unsigned_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (pMax_apdu) {
        *pMax_apdu = (unsigned)unsigned_value;
    }
    /* Segmentation - enumerated */
    len = bacnet_enumerated_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &enum_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (enum_value >= MAX_BACNET_SEGMENTATION) {
        return BACNET_STATUS_ERROR;
    }
    if (pSegmentation) {
        *pSegmentation = (int)enum_value;
    }
    /* Vendor ID - unsigned16 */
    len = bacnet_unsigned_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &unsigned_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (unsigned_value > 0xFFFF) {
        return BACNET_STATUS_ERROR;
    }
    if (pVendor_id) {
        *pVendor_id = (uint16_t)unsigned_value;
    }

    return apdu_len;
}

/**
 * @brief Decode the I-Am service.
 *
 * @param apdu  Receive buffer
 * @param pDevice_id  Pointer to the variable that shall take the device Id.
 * @param pMax_apdu  Pointer to a variable that shall take the decoded length.
 * @param pSegmentation  Pointer to a variable taking if segmentation is used.
 * @param pVendor_id  Pointer to a variable taking the vendor id.
 *
 * @return Total length of the apdu, zero otherwise.
 */
int iam_decode_service_request(
    const uint8_t *apdu,
    uint32_t *pDevice_id,
    unsigned *pMax_apdu,
    int *pSegmentation,
    uint16_t *pVendor_id)
{
    return bacnet_iam_request_decode(
        apdu, MAX_APDU, pDevice_id, pMax_apdu, pSegmentation, pVendor_id);
}
