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
int iam_encode_apdu(uint8_t *apdu,
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
        apdu_len = 2;
        len = encode_application_object_id(
            &apdu[apdu_len], OBJECT_DEVICE, device_id);
        apdu_len += len;
        len = encode_application_unsigned(&apdu[apdu_len], max_apdu);
        apdu_len += len;
        len = encode_application_enumerated(
            &apdu[apdu_len], (uint32_t)segmentation);
        apdu_len += len;
        len = encode_application_unsigned(&apdu[apdu_len], vendor_id);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Decode the I-Am service.
 *
 * @param apdu  Receive buffer
 * @param pDevice_id  Pointer to the variable that shal ltake the device Id.
 * @param pMax_apdu  Pointer to a variable that shall take the decoded length.
 * @param pSegmentation  Pointer to a variable taking if segmentation is used.
 * @param pVendor_id  Pointer to a variable taking the vendor id.
 *
 * @return Total length of the apdu, zero otherwise.
 */
int iam_decode_service_request(uint8_t *apdu,
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
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    uint32_t enum_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    /* OBJECT ID - object id */
    len = decode_tag_number_and_value(&apdu[apdu_len], &tag_number, &len_value);
    apdu_len += len;
    if (tag_number != BACNET_APPLICATION_TAG_OBJECT_ID) {
        return -1;
    }
    len = decode_object_id(&apdu[apdu_len], &object_type, &object_instance);
    apdu_len += len;
    if (object_type != OBJECT_DEVICE) {
        return -1;
    }
    if (pDevice_id) {
        *pDevice_id = object_instance;
    }
    /* MAX APDU - unsigned */
    len = decode_tag_number_and_value(&apdu[apdu_len], &tag_number, &len_value);
    apdu_len += len;
    if (tag_number != BACNET_APPLICATION_TAG_UNSIGNED_INT) {
        return -1;
    }
    len = decode_unsigned(&apdu[apdu_len], len_value, &unsigned_value);
    apdu_len += len;
    if (pMax_apdu) {
        *pMax_apdu = (unsigned)unsigned_value;
    }
    /* Segmentation - enumerated */
    len = decode_tag_number_and_value(&apdu[apdu_len], &tag_number, &len_value);
    apdu_len += len;
    if (tag_number != BACNET_APPLICATION_TAG_ENUMERATED) {
        return -1;
    }
    len = decode_enumerated(&apdu[apdu_len], len_value, &enum_value);
    apdu_len += len;
    if (enum_value >= MAX_BACNET_SEGMENTATION) {
        return -1;
    }
    if (pSegmentation) {
        *pSegmentation = (int)enum_value;
    }
    /* Vendor ID - unsigned16 */
    len = decode_tag_number_and_value(&apdu[apdu_len], &tag_number, &len_value);
    apdu_len += len;
    if (tag_number != BACNET_APPLICATION_TAG_UNSIGNED_INT) {
        return -1;
    }
    len = decode_unsigned(&apdu[apdu_len], len_value, &unsigned_value);
    apdu_len += len;
    if (unsigned_value > 0xFFFF) {
        return -1;
    }
    if (pVendor_id) {
        *pVendor_id = (uint16_t)unsigned_value;
    }

    return apdu_len;
}
