/**
 * @file
 * @brief BACnetActionCommand codec used by Command objects
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacaction.h"
#include "bacnet/bacdcode.h"

/**
 * @brief Encode property value according to the application tag
 * @param apdu - Pointer to the buffer to encode to, or NULL for length
 * @param value - Pointer to the property value to encode from
 * @return number of bytes encoded
 */
int bacnet_action_property_value_encode(
    uint8_t *apdu, const BACNET_ACTION_PROPERTY_VALUE *value)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (!value) {
        return 0;
    }
    switch (value->tag) {
#if defined(BACACTION_NULL)
        case BACNET_APPLICATION_TAG_NULL:
            if (apdu) {
                apdu[0] = value->tag;
            }
            apdu_len++;
            break;
#endif
#if defined(BACACTION_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            apdu_len = encode_application_boolean(apdu, value->type.Boolean);
            break;
#endif
#if defined(BACACTION_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            apdu_len =
                encode_application_unsigned(apdu, value->type.Unsigned_Int);
            break;
#endif
#if defined(BACACTION_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            apdu_len = encode_application_signed(apdu, value->type.Signed_Int);
            break;
#endif
#if defined(BACACTION_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            apdu_len = encode_application_real(apdu, value->type.Real);
            break;
#endif
#if defined(BACACTION_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            apdu_len = encode_application_double(apdu, value->type.Double);
            break;
#endif
#if defined(BACACTION_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            apdu_len =
                encode_application_enumerated(apdu, value->type.Enumerated);
            break;
#endif
        default:
            break;
    }

    return apdu_len;
}

/**
 * @brief Decode property value from the application buffer
 * @param apdu - Pointer to the buffer to decode from
 * @param apdu_size Size of the buffer to decode from
 * @param value - Pointer to the property value to decode to
 * @return number of bytes encoded
 */
int bacnet_action_property_value_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_ACTION_PROPERTY_VALUE *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_TAG tag = { 0 };

    if (!value) {
        return 0;
    }
    if (!apdu) {
        return 0;
    }
    len = bacnet_tag_decode(apdu, apdu_size, &tag);
    if ((len > 0) && tag.application) {
        if (value) {
            value->tag = tag.number;
        }
        switch (tag.number) {
#if defined(BACACTION_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                apdu_len = len;
                break;
#endif
#if defined(BACACTION_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                apdu_len = bacnet_boolean_application_decode(
                    apdu, apdu_size, &value->type.Boolean);
                break;
#endif
#if defined(BACACTION_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                apdu_len = bacnet_unsigned_application_decode(
                    apdu, apdu_size, &value->type.Unsigned_Int);
                break;
#endif
#if defined(BACACTION_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                apdu_len = bacnet_signed_application_decode(
                    apdu, apdu_size, &value->type.Signed_Int);
                break;
#endif
#if defined(BACACTION_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                apdu_len = bacnet_real_application_decode(
                    apdu, apdu_size, &value->type.Real);
                break;
#endif
#if defined(BACACTION_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                apdu_len = bacnet_double_application_decode(
                    apdu, apdu_size, &value->type.Double);
                break;
#endif
#if defined(BACACTION_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                apdu_len = bacnet_enumerated_application_decode(
                    apdu, apdu_size, &value->type.Enumerated);
                break;
#endif
            default:
                break;
        }
    }

    return apdu_len;
}

/**
 * @brief Compare two BACnetActionPropertyValue complex datatypes
 * @param value1 [in] The first structure to compare
 * @param value2 [in] The second structure to compare
 * @return true if the two structures are the same
 */
bool bacnet_action_property_value_same(
    const BACNET_ACTION_PROPERTY_VALUE *value1,
    const BACNET_ACTION_PROPERTY_VALUE *value2)
{
    bool status = false; /*return value */

    if ((value1 == NULL) || (value2 == NULL)) {
        return false;
    }
    /* does the tag match? */
    if (value1->tag == value2->tag) {
        status = true;
    }
    if (status) {
        /* second test for same-ness */
        status = false;
        /* does the value match? */
        switch (value1->tag) {
#if defined(BACACTION_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                status = true;
                break;
#endif
#if defined(BACACTION_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                if (value1->type.Boolean == value2->type.Boolean) {
                    status = true;
                }
                break;
#endif
#if defined(BACACTION_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                if (value1->type.Unsigned_Int == value2->type.Unsigned_Int) {
                    status = true;
                }
                break;
#endif
#if defined(BACACTION_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                if (value1->type.Signed_Int == value2->type.Signed_Int) {
                    status = true;
                }
                break;
#endif
#if defined(BACACTION_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                if (!islessgreater(value1->type.Real, value2->type.Real)) {
                    status = true;
                }
                break;
#endif
#if defined(BACACTION_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                if (!islessgreater(value1->type.Double, value2->type.Double)) {
                    status = true;
                }
                break;
#endif
#if defined(BACACTION_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                if (value1->type.Enumerated == value2->type.Enumerated) {
                    status = true;
                }
                break;
#endif
            case BACNET_APPLICATION_TAG_EMPTYLIST:
                status = true;
                break;
            default:
                status = false;
                break;
        }
    }

    return status;
}

/**
 * @brief Encode the BACnetActionCommand complex datatype
 *
 *    BACnetActionCommand ::= SEQUENCE {
 *       deviceIdentifier [0] BACnetObjectIdentifier OPTIONAL,
 *       objectIdentifier [1] BACnetObjectIdentifier,
 *       propertyIdentifier [2] BACnetPropertyIdentifier,
 *       propertyArrayIndex [3] Unsigned OPTIONAL,
 *       --used only with array datatype
 *       propertyValue [4] ABSTRACT-SYNTAX.&Type,
 *       priority [5] Unsigned (1..16) OPTIONAL,
 *       --used only when property is commandable
 *       postDelay [6] Unsigned OPTIONAL,
 *       quitOnFailure [7] BOOLEAN,
 *       writeSuccessful [8] BOOLEAN
 *   }
 *
 * @param apdu [out] The APDU buffer to encode into, or NULL for length
 * @param entry [in] The BACNET_ACTION_LIST structure to encode
 * @return The length of the encoded data, or BACNET_STATUS_REJECT on error
 */
int bacnet_action_command_encode(uint8_t *apdu, const BACNET_ACTION_LIST *entry)
{
    int len = 0;
    int apdu_len = 0;

    if (!entry) {
        return BACNET_STATUS_REJECT;
    }
    /* deviceIdentifier [0] BACnetObjectIdentifier OPTIONAL */
    if (entry->Device_Id.instance <= BACNET_MAX_INSTANCE) {
        len = encode_context_object_id(
            apdu, 0, entry->Device_Id.type, entry->Device_Id.instance);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    } else {
        return BACNET_STATUS_REJECT;
    }
    /* objectIdentifier [1] BACnetObjectIdentifier */
    len = encode_context_object_id(
        apdu, 1, entry->Object_Id.type, entry->Object_Id.instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* propertyIdentifier [2] BACnetPropertyIdentifier */
    len = encode_context_enumerated(apdu, 2, entry->Property_Identifier);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* propertyArrayIndex [3] Unsigned OPTIONAL */
    if (entry->Property_Array_Index != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(apdu, 3, entry->Property_Array_Index);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    /* propertyValue [4] ABSTRACT-SYNTAX.&Type */
    len = encode_opening_tag(apdu, 4);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacnet_action_property_value_encode(apdu, &entry->Value);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, 4);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* priority [5] Unsigned (1..16) OPTIONAL */
    if (entry->Priority != BACNET_NO_PRIORITY) {
        len = encode_context_unsigned(apdu, 5, entry->Priority);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    /* postDelay [6] Unsigned OPTIONAL */
    if (entry->Post_Delay != UINT32_MAX) {
        len = encode_context_unsigned(apdu, 6, entry->Post_Delay);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    /* quitOnFailure [7] BOOLEAN */
    len = encode_context_boolean(apdu, 7, entry->Quit_On_Failure);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* writeSuccessful [8] BOOLEAN */
    len = encode_context_boolean(apdu, 8, entry->Write_Successful);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode the BACnetActionCommand complex datatype
 * @param apdu [in] The APDU buffer to decode
 * @param apdu_size [in] The size of the APDU buffer
 * @param tag [in] The BACNET_APPLICATION_TAG of the value
 * @param entry [out] The BACNET_ACTION_LIST structure to fill
 * @return The length of the decoded data, or BACNET_STATUS_ERROR on error
 */
int bacnet_action_command_decode(
    const uint8_t *apdu, size_t apdu_size, BACNET_ACTION_LIST *entry)
{
    int len = 0;
    int apdu_len = 0;
    int data_len = 0;
    uint32_t instance = 0;
    BACNET_OBJECT_TYPE type = OBJECT_NONE; /* for decoding */
    uint32_t property = 0; /* for decoding */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    bool boolean_value = false;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* deviceIdentifier [0] BACnetObjectIdentifier OPTIONAL */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &type, &instance);
    if (len > 0) {
        if (instance > BACNET_MAX_INSTANCE) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        if (entry) {
            entry->Device_Id.type = type;
            entry->Device_Id.instance = instance;
        }
    } else if (len == 0) {
        /* wrong tag - optional - skip apdu_len increment */
        if (entry) {
            entry->Device_Id.type = OBJECT_DEVICE;
            entry->Device_Id.instance = BACNET_MAX_INSTANCE;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* objectIdentifier [1] BACnetObjectIdentifier */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &type, &instance);
    if (len > 0) {
        if (instance > BACNET_MAX_INSTANCE) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        if (entry) {
            entry->Object_Id.type = type;
            entry->Object_Id.instance = instance;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* propertyIdentifier [2] BACnetPropertyIdentifier */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &property);
    if (len > 0) {
        apdu_len += len;
        if (entry) {
            entry->Property_Identifier = (BACNET_PROPERTY_ID)property;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* propertyArrayIndex [3] Unsigned OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 3, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (entry) {
            entry->Property_Array_Index = unsigned_value;
        }
    } else {
        /* wrong tag or malformed - optional - skip apdu_len increment */
        if (entry) {
            entry->Property_Array_Index = BACNET_ARRAY_ALL;
        }
    }
    /* propertyValue [4] ABSTRACT-SYNTAX.&Type */
    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 4, &len)) {
        return BACNET_STATUS_ERROR;
    }
    /* determine the length of the data blob */
    data_len =
        bacnet_enclosed_data_length(&apdu[apdu_len], apdu_size - apdu_len);
    if (data_len == BACNET_STATUS_ERROR) {
        return BACNET_STATUS_ERROR;
    }
    /* count the opening tag number length */
    apdu_len += len;
    /* decode data from the APDU */
    if (data_len > MAX_APDU) {
        /* not enough size in application_data to store the data chunk */
        return BACNET_STATUS_ERROR;
    }
    if (entry) {
        len = bacnet_action_property_value_decode(
            &apdu[apdu_len], data_len, &entry->Value);
        if (len < 0) {
            /* signal internal error */
            entry->Value.tag = BACNET_APPLICATION_TAG_ERROR;
        }
    }
    /* add on the data length */
    apdu_len += data_len;
    /* closing */
    if (!bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 4, &len)) {
        return BACNET_STATUS_ERROR;
    }
    /* count the closing tag number length */
    apdu_len += len;
    /* priority [5] Unsigned (1..16) OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_len - apdu_size, 5, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if ((unsigned_value >= BACNET_MIN_PRIORITY) &&
            (unsigned_value <= BACNET_MAX_PRIORITY)) {
            if (entry) {
                entry->Priority = (uint8_t)unsigned_value;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        /* wrong tag or malformed - optional - skip apdu_len increment */
        if (entry) {
            entry->Priority = BACNET_NO_PRIORITY;
        }
    }
    /* postDelay [6] Unsigned OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_len - apdu_size, 6, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (entry) {
            entry->Post_Delay = (uint8_t)unsigned_value;
        }
    } else {
        /* wrong tag or malformed - optional - skip apdu_len increment */
        if (entry) {
            entry->Post_Delay = UINT32_MAX;
        }
    }
    /* quitOnFailure [7] BOOLEAN */
    len = bacnet_boolean_context_decode(
        &apdu[apdu_len], apdu_len - apdu_size, 7, &boolean_value);
    if (len > 0) {
        apdu_len += len;
        if (entry) {
            entry->Quit_On_Failure = boolean_value;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* writeSuccessful [8] BOOLEAN */
    len = bacnet_boolean_context_decode(
        &apdu[apdu_len], apdu_len - apdu_size, 8, &boolean_value);
    if (len > 0) {
        apdu_len += len;
        if (entry) {
            entry->Write_Successful = boolean_value;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare two BACnetActionCommand complex datatypes
 * @param entry1 [in] The first BACNET_ACTION_LIST structure to compare
 * @param entry2 [in] The second BACNET_ACTION_LIST structure to compare
 * @return True if the two structures are the same, else False
 */
bool bacnet_action_command_same(
    const BACNET_ACTION_LIST *entry1, const BACNET_ACTION_LIST *entry2)
{
    if (!entry1 || !entry2) {
        return false;
    }
    if (entry1->Device_Id.type != entry2->Device_Id.type) {
        return false;
    }
    if (entry1->Device_Id.instance != entry2->Device_Id.instance) {
        return false;
    }
    if (entry1->Object_Id.type != entry2->Object_Id.type) {
        return false;
    }
    if (entry1->Object_Id.instance != entry2->Object_Id.instance) {
        return false;
    }
    if (entry1->Property_Identifier != entry2->Property_Identifier) {
        return false;
    }
    if (entry1->Property_Array_Index != entry2->Property_Array_Index) {
        return false;
    }
    if (!bacnet_action_property_value_same(&entry1->Value, &entry2->Value)) {
        return false;
    }
    if (entry1->Priority != entry2->Priority) {
        return false;
    }
    if (entry1->Post_Delay != entry2->Post_Delay) {
        return false;
    }
    if (entry1->Quit_On_Failure != entry2->Quit_On_Failure) {
        return false;
    }
    if (entry1->Write_Successful != entry2->Write_Successful) {
        return false;
    }

    return true;
}
