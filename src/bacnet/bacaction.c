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
#include <string.h>

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
#if defined(BACACTION_OCTET_STRING)
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            apdu_len = encode_application_octet_string(
                apdu, &value->type.Octet_String);
            break;
#endif
#if defined(BACACTION_CHARACTER_STRING)
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            apdu_len = encode_application_character_string(
                apdu, &value->type.Character_String);
            break;
#endif
#if defined(BACACTION_BIT_STRING)
        case BACNET_APPLICATION_TAG_BIT_STRING:
            apdu_len =
                encode_application_bitstring(apdu, &value->type.Bit_String);
            break;
#endif
#if defined(BACACTION_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            apdu_len =
                encode_application_enumerated(apdu, value->type.Enumerated);
            break;
#endif
#if defined(BACACTION_DATE)
        case BACNET_APPLICATION_TAG_DATE:
            apdu_len = encode_application_date(apdu, &value->type.Date);
            break;
#endif
#if defined(BACACTION_TIME)
        case BACNET_APPLICATION_TAG_TIME:
            apdu_len = encode_application_time(apdu, &value->type.Time);
            break;
#endif
#if defined(BACACTION_OBJECT_ID)
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            apdu_len = encode_application_object_id(
                apdu, (int)value->type.Object_Id.type,
                value->type.Object_Id.instance);
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
    len = bacnet_tag_decode(&apdu[apdu_len], apdu_size - apdu_len, &tag);
    if (len > 0) {
        apdu_len += len;
        if (tag.application) {
            value->tag = tag.number;
            switch (tag.number) {
#if defined(BACACTION_NULL)
                case BACNET_APPLICATION_TAG_NULL:
                    len = 0;
                    break;
#endif
#if defined(BACACTION_BOOLEAN)
                case BACNET_APPLICATION_TAG_BOOLEAN:
                    value->type.Boolean = decode_boolean(tag.len_value_type);
                    len = 0;
                    break;
#endif
#if defined(BACACTION_UNSIGNED)
                case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                    len = bacnet_unsigned_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Unsigned_Int);
                    break;
#endif
#if defined(BACACTION_SIGNED)
                case BACNET_APPLICATION_TAG_SIGNED_INT:
                    len = bacnet_signed_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Signed_Int);
                    break;
#endif
#if defined(BACACTION_REAL)
                case BACNET_APPLICATION_TAG_REAL:
                    len = bacnet_real_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Real);
                    break;
#endif
#if defined(BACACTION_DOUBLE)
                case BACNET_APPLICATION_TAG_DOUBLE:
                    len = bacnet_double_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Double);
                    break;
#endif
#if defined(BACACTION_OCTET_STRING)
                case BACNET_APPLICATION_TAG_OCTET_STRING:
                    len = bacnet_octet_string_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Octet_String);
                    break;
#endif
#if defined(BACACTION_CHARACTER_STRING)
                case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                    len = bacnet_character_string_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Character_String);
                    break;
#endif
#if defined(BACACTION_BIT_STRING)
                case BACNET_APPLICATION_TAG_BIT_STRING:
                    len = bacnet_bitstring_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Bit_String);
                    break;
#endif
#if defined(BACACTION_ENUMERATED)
                case BACNET_APPLICATION_TAG_ENUMERATED:
                    len = bacnet_enumerated_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Enumerated);
                    break;
#endif
#if defined(BACACTION_DATE)
                case BACNET_APPLICATION_TAG_DATE:
                    len = bacnet_date_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Date);
                    break;
#endif
#if defined(BACACTION_TIME)
                case BACNET_APPLICATION_TAG_TIME:
                    len = bacnet_time_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Time);
                    break;
#endif
#if defined(BACACTION_OBJECT_ID)
                case BACNET_APPLICATION_TAG_OBJECT_ID:
                    len = bacnet_object_id_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Object_Id.type,
                        &value->type.Object_Id.instance);
                    break;
#endif
                default:
                    return BACNET_STATUS_ERROR;
            }
            if ((len == 0) && (tag.number != BACNET_APPLICATION_TAG_NULL) &&
                (tag.number != BACNET_APPLICATION_TAG_BOOLEAN) &&
                (tag.number != BACNET_APPLICATION_TAG_OCTET_STRING)) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
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
#if defined(BACACTION_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                status = octetstring_value_same(
                    &value1->type.Octet_String, &value2->type.Octet_String);
                break;
#endif
#if defined(BACACTION_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                status = characterstring_same(
                    &value1->type.Character_String,
                    &value2->type.Character_String);
                break;
#endif
#if defined(BACACTION_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                status = bitstring_same(
                    &value1->type.Bit_String, &value2->type.Bit_String);
                break;
#endif
#if defined(BACACTION_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                if (value1->type.Enumerated == value2->type.Enumerated) {
                    status = true;
                }
                break;
#endif
#if defined(BACACTION_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                status =
                    (datetime_compare_date(
                         &value1->type.Date, &value2->type.Date) == 0);
                break;
#endif
#if defined(BACACTION_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                status =
                    (datetime_compare_time(
                         &value1->type.Time, &value2->type.Time) == 0);
                break;
#endif
#if defined(BACACTION_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                status = bacnet_object_id_same(
                    value1->type.Object_Id.type,
                    value1->type.Object_Id.instance,
                    value2->type.Object_Id.type,
                    value2->type.Object_Id.instance);
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
    if (entry->Property_Value.data_len > BACNET_CONSTRUCTED_VALUE_SIZE) {
        return BACNET_STATUS_REJECT;
    }
    len = (int)entry->Property_Value.data_len;
    if (apdu && len > 0) {
        memcpy(
            apdu, &entry->Property_Value.data[0],
            entry->Property_Value.data_len);
    }
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
    uint32_t instance = 0;
    BACNET_OBJECT_TYPE type = OBJECT_NONE; /* for decoding */
    uint32_t property = 0; /* for decoding */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_CONSTRUCTED_VALUE_TYPE *property_value = NULL;
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
    if (entry) {
        property_value = &entry->Property_Value;
    }
    len = bacnet_constructed_value_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 4, property_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    /* priority [5] Unsigned (1..16) OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 5, &unsigned_value);
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
        &apdu[apdu_len], apdu_size - apdu_len, 6, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (entry) {
            entry->Post_Delay = (uint32_t)unsigned_value;
        }
    } else {
        /* wrong tag or malformed - optional - skip apdu_len increment */
        if (entry) {
            entry->Post_Delay = UINT32_MAX;
        }
    }
    /* quitOnFailure [7] BOOLEAN */
    len = bacnet_boolean_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 7, &boolean_value);
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
        &apdu[apdu_len], apdu_size - apdu_len, 8, &boolean_value);
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
    if (entry1->Property_Value.data_len != entry2->Property_Value.data_len) {
        return false;
    }
    if ((entry1->Property_Value.data_len > 0) &&
        (memcmp(
             &entry1->Property_Value.data[0], &entry2->Property_Value.data[0],
             entry1->Property_Value.data_len) != 0)) {
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
