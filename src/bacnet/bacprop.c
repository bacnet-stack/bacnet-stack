/**
 * @file
 * @brief BACnet property tag lookup functions
 * @author John Goulah <bigjohngoulah@users.sourceforge.net>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdbool.h>
#include <string.h>
#if PRINT_ENABLED
#include <stdio.h>
#endif
#include "bacnet/bacprop.h"

PROP_TAG_DATA bacnet_object_device_property_tag_map[] = {
    { PROP_OBJECT_IDENTIFIER, BACNET_APPLICATION_TAG_OBJECT_ID },
    { PROP_OBJECT_NAME, BACNET_APPLICATION_TAG_CHARACTER_STRING },
    { PROP_OBJECT_TYPE, BACNET_APPLICATION_TAG_ENUMERATED },
    { PROP_SYSTEM_STATUS, BACNET_APPLICATION_TAG_ENUMERATED },
    { PROP_VENDOR_NAME, BACNET_APPLICATION_TAG_CHARACTER_STRING },
    { PROP_VENDOR_IDENTIFIER, BACNET_APPLICATION_TAG_UNSIGNED_INT },
    { PROP_MODEL_NAME, BACNET_APPLICATION_TAG_CHARACTER_STRING },
    { PROP_FIRMWARE_REVISION, BACNET_APPLICATION_TAG_CHARACTER_STRING },
    { PROP_APPLICATION_SOFTWARE_VERSION,
        BACNET_APPLICATION_TAG_CHARACTER_STRING },
    { PROP_PROTOCOL_VERSION, BACNET_APPLICATION_TAG_UNSIGNED_INT },
    { PROP_PROTOCOL_CONFORMANCE_CLASS, BACNET_APPLICATION_TAG_UNSIGNED_INT },
    { PROP_PROTOCOL_SERVICES_SUPPORTED, BACNET_APPLICATION_TAG_BIT_STRING },
    { PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED, BACNET_APPLICATION_TAG_BIT_STRING },
    { PROP_MAX_APDU_LENGTH_ACCEPTED, BACNET_APPLICATION_TAG_UNSIGNED_INT },
    { PROP_SEGMENTATION_SUPPORTED, BACNET_APPLICATION_TAG_ENUMERATED },
    { PROP_APDU_TIMEOUT, BACNET_APPLICATION_TAG_UNSIGNED_INT },
    { PROP_NUMBER_OF_APDU_RETRIES, BACNET_APPLICATION_TAG_UNSIGNED_INT },
    { -1, -1 }
};

/**
 * Search for the given index in the data list.
 *
 * @param data_list  Pointer to the list.
 * @param index  Index to search for.
 * @param default_ret  Default return value.
 *
 * @return Value found or the default value.
 */
signed bacprop_tag_by_index_default(
    const PROP_TAG_DATA *data_list, signed index, signed default_ret)
{
    signed pUnsigned = 0;

    if (data_list) {
        while (data_list->prop_id != -1) {
            if (data_list->prop_id == index) {
                pUnsigned = data_list->tag_id;
                break;
            }
            data_list++;
        }
    }

    return pUnsigned ? pUnsigned : default_ret;
}

/**
 * Return the value of the given property, if available.
 *
 * @param type Object type
 * @param prop Property
 *
 * @return Value found or -1
 */
signed bacprop_property_tag(BACNET_OBJECT_TYPE type, signed prop)
{
    switch (type) {
        case OBJECT_DEVICE:
            return bacprop_tag_by_index_default(
                bacnet_object_device_property_tag_map, prop, -1);
        default:
#if PRINT_ENABLED
            fprintf(stderr, "Unsupported object type");
#endif
            break;
    }

    return -1;
}
