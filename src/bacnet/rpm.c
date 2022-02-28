/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

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
#include "bacnet/bacerror.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacapp.h"
#include "bacnet/memcopy.h"
#include "bacnet/rpm.h"

/** @file rpm.c  Encode/Decode Read Property Multiple and RPM ACKs  */

#if BACNET_SVC_RPM_A

/** Encode the initial portion of the service
 *
 * @param apdu  Pointer to the buffer for encoding.
 * @param invoke_id  Invoke ID
 *
 * @return Bytes encoded (usually 4) or zero on error.
 */
int rpm_encode_apdu_init(uint8_t *apdu, uint8_t invoke_id)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_READ_PROP_MULTIPLE; /* service choice */
        apdu_len = 4;
    }

    return apdu_len;
}

/** Encode the beginning, including
 *  Object-id and Read-Access of the service.
 *
 * @param apdu  Pointer to the buffer for encoding.
 * @param object_type  Object type to encode
 * @param object_instance  Object instance to encode
 *
 * @return Bytes encoded or zero on error.
 */
int rpm_encode_apdu_object_begin(
    uint8_t *apdu, BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu_len =
            encode_context_object_id(&apdu[0], 0, object_type, object_instance);
        /* Tag 1: sequence of ReadAccessSpecification */
        apdu_len += encode_opening_tag(&apdu[apdu_len], 1);
    }

    return apdu_len;
}

/** Encode the object properties of the service.
 *
 * @param apdu  Pointer to the buffer for encoding.
 * @param object_property  Object property.
 * @param array_index Optional array index.
 *
 * @return Bytes encoded or zero on error.
 */
int rpm_encode_apdu_object_property(uint8_t *apdu,
    BACNET_PROPERTY_ID object_property,
    BACNET_ARRAY_INDEX array_index)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu_len = encode_context_enumerated(&apdu[0], 0, object_property);
        /* optional array index */
        if (array_index != BACNET_ARRAY_ALL) {
            apdu_len +=
                encode_context_unsigned(&apdu[apdu_len], 1, array_index);
        }
    }

    return apdu_len;
}

/** Encode the end (closing tag) of the service
 *
 * @param apdu  Pointer to the buffer for encoding.
 *
 * @return Bytes encoded or zero on error.
 */
int rpm_encode_apdu_object_end(uint8_t *apdu)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu_len = encode_closing_tag(&apdu[0], 1);
    }

    return apdu_len;
}

/** Encode an RPM request, to be sent.
 *
 * @param apdu [in,out] Buffer to hold encoded bytes.
 * @param max_apdu [in] Length of apdu buffer.
 * @param invoke_id [in] The Invoke ID to use for this message.
 * @param read_access_data [in] The RPM data to be requested.
 * @return Length of encoded bytes, or 0 on failure.
 */
int rpm_encode_apdu(uint8_t *apdu,
    size_t max_apdu,
    uint8_t invoke_id,
    BACNET_READ_ACCESS_DATA *read_access_data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0; /* length of the data */
    BACNET_READ_ACCESS_DATA *rpm_object; /* current object */
    uint8_t apdu_temp[16]; /* temp for data before copy */
    BACNET_PROPERTY_REFERENCE *rpm_property; /* current property */

    len = rpm_encode_apdu_init(&apdu_temp[0], invoke_id);
    len = (int)memcopy(&apdu[0], &apdu_temp[0], (size_t)apdu_len, (size_t)len,
        (size_t)max_apdu);
    if (len == 0) {
        return 0;
    }
    apdu_len += len;
    rpm_object = read_access_data;
    while (rpm_object) {
        /* The encode function will return a length not more than 12. So the
         * temp buffer being 16 bytes is fine enough. */
        len = encode_context_object_id(&apdu_temp[0], 0,
            rpm_object->object_type, rpm_object->object_instance);
        len = (int)memcopy(&apdu[0], &apdu_temp[0], (size_t)apdu_len,
            (size_t)len, (size_t)max_apdu);
        if (len == 0) {
            return 0;
        }
        apdu_len += len;
        /* Tag 1: sequence of ReadAccessSpecification */
        len = encode_opening_tag(&apdu_temp[0], 1);
        len = (int)memcopy(&apdu[0], &apdu_temp[0], (size_t)apdu_len,
            (size_t)len, (size_t)max_apdu);
        if (len == 0) {
            return 0;
        }
        apdu_len += len;
        rpm_property = rpm_object->listOfProperties;
        while (rpm_property) {
            /* The encode function will return a length not more than 12. So the
             * temp buffer being 16 bytes is fine enough. Stuff as many
             * properties into it as APDU length will allow. */
            len = encode_context_enumerated(
                &apdu_temp[0], 0, rpm_property->propertyIdentifier);
            len = (int)memcopy(&apdu[0], &apdu_temp[0], (size_t)apdu_len,
                (size_t)len, (size_t)max_apdu);
            if (len == 0) {
                return 0;
            }
            apdu_len += len;
            /* optional array index */
            if (rpm_property->propertyArrayIndex != BACNET_ARRAY_ALL) {
                len = encode_context_unsigned(
                    &apdu_temp[0], 1, rpm_property->propertyArrayIndex);
                len = (int)memcopy(&apdu[0], &apdu_temp[0], (size_t)apdu_len,
                    (size_t)len, (size_t)max_apdu);
                if (len == 0) {
                    return 0;
                }
                apdu_len += len;
            }
            rpm_property = rpm_property->next;
            /* Full? */
            if ((unsigned)apdu_len >= max_apdu) {
                return 0;
            }
        }
        len = encode_closing_tag(&apdu_temp[0], 1);
        len = (int)memcopy(&apdu[0], &apdu_temp[0], (size_t)apdu_len,
            (size_t)len, (size_t)max_apdu);
        if (len == 0) {
            return 0;
        }
        apdu_len += len;
        rpm_object = rpm_object->next;
    }

    return apdu_len;
}

#endif

/** Decode the object portion of the service request only. Bails out if
 *  tags are wrong or missing/incomplete.
 *
 * @param apdu [in] Buffer of bytes received.
 * @param apdu_len [in] Count of valid bytes in the buffer.
 * @param rpmdata [in] The data structure to be filled.
 *
 * @return Length of decoded bytes, or 0 on failure.
 */
int rpm_decode_object_id(
    uint8_t *apdu, unsigned apdu_len, BACNET_RPM_DATA *rpmdata)
{
    int len = 0;
    BACNET_OBJECT_TYPE type = OBJECT_NONE; /* for decoding */

    /* check for value pointers */
    if (apdu && apdu_len && rpmdata) {
        if (apdu_len < 5) { /* Must be at least 2 tags and an object id */
            rpmdata->error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
            return BACNET_STATUS_REJECT;
        }
        /* Tag 0: Object ID */
        if (!decode_is_context_tag(&apdu[len++], 0)) {
            rpmdata->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        len += decode_object_id(&apdu[len], &type, &rpmdata->object_instance);
        rpmdata->object_type = type;
        /* Tag 1: sequence of ReadAccessSpecification */
        if (!decode_is_opening_tag_number(&apdu[len], 1)) {
            rpmdata->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        len++; /* opening tag is only one octet */
    }

    return len;
}

/** Decode the end portion of the service request only.
 *
 * @param apdu [in] Buffer of bytes received.
 * @param apdu_len [in] Count of valid bytes in the buffer.
 *
 * @return Length of decoded bytes (usually 1), or 0 on failure.
 */
int rpm_decode_object_end(uint8_t *apdu, unsigned apdu_len)
{
    int len = 0; /* total length of the apdu, return value */

    if (apdu && apdu_len) {
        if (decode_is_closing_tag_number(apdu, 1) == true) {
            len = 1;
        }
    }

    return len;
}

/** Decode the object property portion of the service request only
 *  BACnetPropertyReference ::= SEQUENCE {
 *      propertyIdentifier [0] BACnetPropertyIdentifier,
 *      propertyArrayIndex [1] Unsigned OPTIONAL
 *      --used only with array datatype
 *      -- if omitted with an array the entire array is referenced
 *  }
 *
 *  @param apdu  Pointer to received bytes.
 *  @param apdu_len  Count of received bytes.
 *  @param rpmdata  Pointer to the data structure to be filled.
 *
 *  @return Bytes decoded or zero on failure.
 */
int rpm_decode_object_property(
    uint8_t *apdu, unsigned apdu_len, BACNET_RPM_DATA *rpmdata)
{
    int len = 0;
    int option_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    uint32_t property = 0; /* for decoding */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0; /* for decoding */

    /* check for valid pointers */
    if (apdu && apdu_len && rpmdata) {
        /* Tag 0: propertyIdentifier */
        if (!IS_CONTEXT_SPECIFIC(apdu[len])) {
            rpmdata->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }

        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != 0) {
            rpmdata->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        /* Should be at least the unsigned value + 1 tag left */
        if ((len + len_value_type) >= apdu_len) {
            rpmdata->error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
            return BACNET_STATUS_REJECT;
        }
        len += decode_enumerated(&apdu[len], len_value_type, &property);
        rpmdata->object_property = (BACNET_PROPERTY_ID)property;
        /* Assume most probable outcome */
        rpmdata->array_index = BACNET_ARRAY_ALL;
        /* Tag 1: Optional propertyArrayIndex */
        if (IS_CONTEXT_SPECIFIC(apdu[len]) && !IS_CLOSING_TAG(apdu[len])) {
            option_len = decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value_type);
            if (tag_number == 1) {
                len += option_len;
                /* Should be at least the unsigned array index + 1 tag left */
                if ((len + len_value_type) >= apdu_len) {
                    rpmdata->error_code =
                        ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                    return BACNET_STATUS_REJECT;
                }
                len += decode_unsigned(
                    &apdu[len], len_value_type, &unsigned_value);
                rpmdata->array_index = unsigned_value;
            }
        }
    }

    return len;
}

/** Encode the acknowledge for a RPM.
 *
 * @param apdu [in] Buffer of bytes to transmit.
 * @param invoke_id [in] Invoke Id.
 *
 * @return Length of encoded bytes (usually 3) or 0 on failure.
 */
int rpm_ack_encode_apdu_init(uint8_t *apdu, uint8_t invoke_id)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK; /* complex ACK service */
        apdu[1] = invoke_id; /* original invoke id from request */
        apdu[2] = SERVICE_CONFIRMED_READ_PROP_MULTIPLE; /* service choice */
        apdu_len = 3;
    }

    return apdu_len;
}

/** Encode the object type for an acknowledge of a RPM.
 *
 * @param apdu [in] Buffer of bytes to transmit.
 * @param rpmdata [in] Pointer to the data used to fill in the APDU.
 *
 * @return Length of encoded bytes or 0 on failure.
 */
int rpm_ack_encode_apdu_object_begin(uint8_t *apdu, BACNET_RPM_DATA *rpmdata)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        /* Tag 0: objectIdentifier */
        apdu_len = encode_context_object_id(
            &apdu[0], 0, rpmdata->object_type, rpmdata->object_instance);
        /* Tag 1: listOfResults */
        apdu_len += encode_opening_tag(&apdu[apdu_len], 1);
    }

    return apdu_len;
}

/** Encode the object property for an acknowledge of a RPM.
 *
 * @param apdu [in] Buffer of bytes to transmit.
 * @param object_property [in] Object property ID.
 * @param array_index  Optional array index
 *
 * @return Length of encoded bytes or 0 on failure.
 */
int rpm_ack_encode_apdu_object_property(uint8_t *apdu,
    BACNET_PROPERTY_ID object_property,
    BACNET_ARRAY_INDEX array_index)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        /* Tag 2: propertyIdentifier */
        apdu_len = encode_context_enumerated(&apdu[0], 2, object_property);
        /* Tag 3: optional propertyArrayIndex */
        if (array_index != BACNET_ARRAY_ALL) {
            apdu_len +=
                encode_context_unsigned(&apdu[apdu_len], 3, array_index);
        }
    }

    return apdu_len;
}

/** Encode the object property value for an acknowledge of a RPM.
 *
 * @param apdu [in] Buffer of bytes to transmit.
 * @param application_data [in] Pointer to the application data used to fill in
 * the APDU.
 * @param application_data_len [in] Length of the application data.
 *
 * @return Length of encoded bytes or 0 on failure.
 */
int rpm_ack_encode_apdu_object_property_value(
    uint8_t *apdu, uint8_t *application_data, unsigned application_data_len)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    unsigned len = 0;

    if (apdu) {
        /* Tag 4: propertyValue */
        apdu_len += encode_opening_tag(&apdu[apdu_len], 4);
        if (application_data ==
            &apdu[apdu_len]) { /* Is Data already in place? */
            apdu_len += application_data_len; /* Yes, step over data */
        } else { /* No, copy data in */
            for (len = 0; len < application_data_len; len++) {
                apdu[apdu_len++] = application_data[len];
            }
        }
        apdu_len += encode_closing_tag(&apdu[apdu_len], 4);
    }

    return apdu_len;
}

/** Encode the object property error for an acknowledge of a RPM.
 *
 * @param apdu [in] Buffer of bytes to transmit.
 * @param error_class [in] Error Class
 * @param error_code [in] Error Code
 *
 * @return Length of encoded bytes or 0 on failure.
 */
int rpm_ack_encode_apdu_object_property_error(
    uint8_t *apdu, BACNET_ERROR_CLASS error_class, BACNET_ERROR_CODE error_code)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        /* Tag 5: propertyAccessError */
        apdu_len += encode_opening_tag(&apdu[apdu_len], 5);
        apdu_len += encode_application_enumerated(&apdu[apdu_len], error_class);
        apdu_len += encode_application_enumerated(&apdu[apdu_len], error_code);
        apdu_len += encode_closing_tag(&apdu[apdu_len], 5);
    }

    return apdu_len;
}

/** Encode the end tag for an acknowledge of a RPM.
 *
 * @param apdu [in] Buffer of bytes to transmit.
 *
 * @return Length of encoded bytes or 0 on failure.
 */
int rpm_ack_encode_apdu_object_end(uint8_t *apdu)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu_len = encode_closing_tag(&apdu[0], 1);
    }

    return apdu_len;
}

#if BACNET_SVC_RPM_A

/* decode the object portion of the service request only */
int rpm_ack_decode_object_id(uint8_t *apdu,
    unsigned apdu_len,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *object_instance)
{
    unsigned len = 0;
    BACNET_OBJECT_TYPE type = OBJECT_NONE; /* for decoding */

    /* check for value pointers */
    if (apdu && apdu_len && object_type && object_instance) {
        /* Tag 0: objectIdentifier */
        if (!decode_is_context_tag(&apdu[len++], 0)) {
            return -1;
        }
        len += decode_object_id(&apdu[len], &type, object_instance);
        if (object_type) {
            *object_type = type;
        }
        /* Tag 1: listOfResults */
        if (!decode_is_opening_tag_number(&apdu[len], 1)) {
            return -1;
        }
        len++; /* opening tag is only one octet */
    }

    return (int)len;
}

/* is this the end of the list of this objects properties values? */
int rpm_ack_decode_object_end(uint8_t *apdu, unsigned apdu_len)
{
    int len = 0; /* total length of the apdu, return value */

    if (apdu && apdu_len) {
        if (decode_is_closing_tag_number(apdu, 1)) {
            len = 1;
        }
    }

    return len;
}

int rpm_ack_decode_object_property(uint8_t *apdu,
    unsigned apdu_len,
    BACNET_PROPERTY_ID *object_property,
    BACNET_ARRAY_INDEX *array_index)
{
    unsigned len = 0;
    unsigned tag_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    uint32_t property = 0; /* for decoding */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0; /* for decoding */

    /* check for valid pointers */
    if (apdu && apdu_len && object_property && array_index) {
        /* Tag 2: propertyIdentifier */
        if (!IS_CONTEXT_SPECIFIC(apdu[len])) {
            return -1;
        }
        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != 2) {
            return -1;
        }
        len += decode_enumerated(&apdu[len], len_value_type, &property);
        if (object_property) {
            *object_property = (BACNET_PROPERTY_ID)property;
        }
        /* Tag 3: Optional propertyArrayIndex */
        if ((len < apdu_len) && IS_CONTEXT_SPECIFIC(apdu[len]) &&
            (!IS_CLOSING_TAG(apdu[len]))) {
            tag_len = (unsigned)decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value_type);
            if (tag_number == 3) {
                len += tag_len;
                len += decode_unsigned(
                    &apdu[len], len_value_type, &unsigned_value);
                *array_index = unsigned_value;
            } else {
                *array_index = BACNET_ARRAY_ALL;
            }
        } else {
            *array_index = BACNET_ARRAY_ALL;
        }
    }

    return (int)len;
}
#endif
