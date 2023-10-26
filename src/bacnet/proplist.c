/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2012 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#include <stdint.h>
#include "bacnet/bacenum.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/rpm.h"
#include "bacnet/rp.h"
#include "bacnet/proplist.h"

/**
 * Function that returns the number of BACnet object properties in a list
 *
 * @param pList - array of type 'int' that is a list of BACnet object
 * properties, terminated by a '-1' value.
 */
unsigned property_list_count(const int *pList)
{
    unsigned property_count = 0;

    if (pList) {
        while (*pList != -1) {
            property_count++;
            pList++;
        }
    }

    return property_count;
}

/**
 * For a given object property, returns the true if in the property list
 *
 * @param pList - array of type 'int' that is a list of BACnet object
 * @param object_property - property enumeration or propritary value
 *
 * @return true if object_property is a member of the property list
 */
bool property_list_member(const int *pList, int object_property)
{
    bool status = false;

    if (pList) {
        while ((*pList) != -1) {
            if (object_property == (*pList)) {
                status = true;
                break;
            }
            pList++;
        }
    }

    return status;
}

/**
 * ReadProperty handler for this property.  For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  rpdata - ReadProperty data, including requested data and
 * data for the reply, or error response.
 *
 * @return number of APDU bytes in the response, or
 * BACNET_STATUS_ERROR on error.
 */
int property_list_encode(BACNET_READ_PROPERTY_DATA *rpdata,
    const int *pListRequired,
    const int *pListOptional,
    const int *pListProprietary)
{
    int apdu_len = 0; /* return value */
    uint8_t *apdu = NULL;
    int max_apdu_len = 0;
    uint32_t count = 0;
    unsigned required_count = 0;
    unsigned optional_count = 0;
    unsigned proprietary_count = 0;
    int len = 0;
    unsigned i = 0; /* loop index */

    required_count = property_list_count(pListRequired);
    optional_count = property_list_count(pListOptional);
    proprietary_count = property_list_count(pListProprietary);
    /* total of all counts */
    count = required_count + optional_count + proprietary_count;
    if (required_count >= 3) {
        /* less the 3 always required properties */
        count -= 3;
        if (property_list_member(pListRequired, PROP_PROPERTY_LIST)) {
            /* property-list should not be in the required list
               because this module handles it transparently.
               Handle the case where it might be in the required list. */
            count -= 1;
        }
    }
    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    max_apdu_len = rpdata->application_data_len;
    switch (rpdata->object_property) {
        case PROP_PROPERTY_LIST:
            if (rpdata->array_index == 0) {
                /* Array element zero is the number of elements in the array */
                apdu_len = encode_application_unsigned(&apdu[0], count);
            } else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                /* if no index was specified, then try to encode the entire list
                 */
                /* into one packet. */
                if (required_count > 3) {
                    for (i = 0; i < required_count; i++) {
                        if ((pListRequired[i] == PROP_OBJECT_TYPE) ||
                            (pListRequired[i] == PROP_OBJECT_IDENTIFIER) ||
                            (pListRequired[i] == PROP_OBJECT_NAME) ||
                            (pListRequired[i] == PROP_PROPERTY_LIST)) {
                            continue;
                        } else {
                            len = encode_application_enumerated(
                                &apdu[apdu_len], (uint32_t)pListRequired[i]);
                        }
                        /* add it if we have room */
                        if ((apdu_len + len) < max_apdu_len) {
                            apdu_len += len;
                        } else {
                            rpdata->error_code =
                                ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                            apdu_len = BACNET_STATUS_ABORT;
                            break;
                        }
                    }
                }
                if (optional_count) {
                    for (i = 0; i < optional_count; i++) {
                        len = encode_application_enumerated(
                            &apdu[apdu_len], (uint32_t)pListOptional[i]);
                        /* add it if we have room */
                        if ((apdu_len + len) < max_apdu_len) {
                            apdu_len += len;
                        } else {
                            rpdata->error_code =
                                ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                            apdu_len = BACNET_STATUS_ABORT;
                            break;
                        }
                    }
                }
                if (proprietary_count) {
                    for (i = 0; i < proprietary_count; i++) {
                        len = encode_application_enumerated(
                            &apdu[apdu_len], (uint32_t)pListProprietary[i]);
                        /* add it if we have room */
                        if ((apdu_len + len) < max_apdu_len) {
                            apdu_len += len;
                        } else {
                            rpdata->error_code =
                                ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                            apdu_len = BACNET_STATUS_ABORT;
                            break;
                        }
                    }
                }
            } else {
                if (rpdata->array_index <= count) {
                    count = 0;
                    if (required_count > 3) {
                        for (i = 0; i < required_count; i++) {
                            if ((pListRequired[i] == PROP_OBJECT_TYPE) ||
                                (pListRequired[i] == PROP_OBJECT_IDENTIFIER) ||
                                (pListRequired[i] == PROP_OBJECT_NAME) ||
                                (pListRequired[i] == PROP_PROPERTY_LIST)) {
                                continue;
                            } else {
                                count++;
                            }
                            if (count == rpdata->array_index) {
                                apdu_len = encode_application_enumerated(
                                    &apdu[apdu_len],
                                    (uint32_t)pListRequired[i]);
                                break;
                            }
                        }
                    }
                    if ((apdu_len == 0) && (optional_count > 0)) {
                        for (i = 0; i < optional_count; i++) {
                            count++;
                            if (count == rpdata->array_index) {
                                apdu_len = encode_application_enumerated(
                                    &apdu[apdu_len],
                                    (uint32_t)pListOptional[i]);
                                break;
                            }
                        }
                    }
                    if ((apdu_len == 0) && (proprietary_count > 0)) {
                        for (i = 0; i < proprietary_count; i++) {
                            count++;
                            if (count == rpdata->array_index) {
                                apdu_len = encode_application_enumerated(
                                    &apdu[apdu_len],
                                    (uint32_t)pListProprietary[i]);
                                break;
                            }
                        }
                    }
                } else {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    return apdu_len;
}

/**
 * ReadProperty handler for common properties.  For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  rpdata - ReadProperty data, including requested data and
 * data for the reply, or error response.
 * @param device_instance_number - device instance number
 *
 * @return number of APDU bytes in the response, or
 * BACNET_STATUS_ERROR on error.
 */
int property_list_common_encode(
    BACNET_READ_PROPERTY_DATA *rpdata, uint32_t device_instance_number)
{
    int apdu_len = BACNET_STATUS_ERROR;
    uint8_t *apdu = NULL;

    if (!rpdata) {
        return 0;
    }
    if ((rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            /*  only array properties can have array options */
            if (rpdata->array_index != BACNET_ARRAY_ALL) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
                apdu_len = BACNET_STATUS_ERROR;
            } else {
                /* Device Object exception: requested instance
                   may not match our instance if a wildcard */
                if (rpdata->object_type == OBJECT_DEVICE) {
                    rpdata->object_instance = device_instance_number;
                }
                apdu_len = encode_application_object_id(
                    &apdu[0], rpdata->object_type, rpdata->object_instance);
            }
            break;
        case PROP_OBJECT_TYPE:
            /*  only array properties can have array options */
            if (rpdata->array_index != BACNET_ARRAY_ALL) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
                apdu_len = BACNET_STATUS_ERROR;
            } else {
                apdu_len = encode_application_enumerated(
                    &apdu[0], rpdata->object_type);
            }
            break;
        default:
            break;
    }

    return apdu_len;
}

/**
 * @brief Determine if the property is a common property
 * @param property - property value for comparison
 * @return true if the property is a common object property
 */
bool property_list_common(BACNET_PROPERTY_ID property)
{
    bool status = false;

    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_TYPE:
            status = true;
            break;
        default:
            break;
    }

    return status;
}
