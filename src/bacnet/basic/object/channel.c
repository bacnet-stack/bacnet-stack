/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2013
 * @brief The Channel object is a command object without a priority array,
 * and the present-value property proxies an ANY data type (sort of)
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/services.h"
#include "bacnet/proplist.h"
#include "bacnet/property.h"
#include "bacnet/basic/sys/keylist.h"
#if defined(CHANNEL_LIGHTING_COMMAND) || defined(CHANNEL_COLOR_COMMAND)
#include "bacnet/lighting.h"
#endif
/* me! */
#include "bacnet/basic/object/channel.h"

#ifndef CONTROL_GROUPS_MAX
#define CONTROL_GROUPS_MAX 8
#endif

#ifndef CHANNEL_MEMBERS_MAX
#define CHANNEL_MEMBERS_MAX 8
#endif

struct object_data {
    bool Out_Of_Service : 1;
    BACNET_CHANNEL_VALUE Present_Value;
    unsigned Last_Priority;
    BACNET_WRITE_STATUS Write_Status;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE Members[CHANNEL_MEMBERS_MAX];
    uint16_t Number;
    uint32_t Control_Groups[CONTROL_GROUPS_MAX];
    const char *Object_Name;
    const char *Description;
};

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;

static write_property_function Write_Property_Internal_Callback;

/* These arrays are used by the ReadPropertyMultiple handler
   property-list property (as of protocol-revision 14) */
static const int Channel_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_LAST_PRIORITY,
    PROP_WRITE_STATUS,
    PROP_STATUS_FLAGS,
    PROP_OUT_OF_SERVICE,
    PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES,
    PROP_CHANNEL_NUMBER,
    PROP_CONTROL_GROUPS,
    -1
};

static const int Channel_Properties_Optional[] = { -1 };

static const int Channel_Properties_Proprietary[] = { -1 };

/**
 * Returns the list of required, optional, and proprietary properties.
 * Used by ReadPropertyMultiple service.
 *
 * @param pRequired - pointer to list of int terminated by -1, of
 * BACnet required properties for this object.
 * @param pOptional - pointer to list of int terminated by -1, of
 * BACnet optkional properties for this object.
 * @param pProprietary - pointer to list of int terminated by -1, of
 * BACnet proprietary properties for this object.
 */
void Channel_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Channel_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Channel_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Channel_Properties_Proprietary;
    }

    return;
}

/**
 * Determines if a given Channel instance is valid
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if the instance is valid, and false if not
 */
bool Channel_Valid_Instance(uint32_t object_instance)
{
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * Determines the number of Channel objects
 *
 * @return  Number of Channel objects
 */
unsigned Channel_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Channel objects where N is Channel_Count().
 *
 * @param  index - 0..BACNET_CHANNELS_MAX value
 *
 * @return  object instance-number for the given index
 */
uint32_t Channel_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Channel objects where N is Channel_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or BACNET_CHANNELS_MAX
 * if not valid.
 */
unsigned Channel_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * For a given object instance-number, determines the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @return pointer to the BACNET_CHANNEL_VALUE present-value
 */
BACNET_CHANNEL_VALUE *Channel_Present_Value(uint32_t object_instance)
{
    BACNET_CHANNEL_VALUE *cvalue = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        cvalue = &pObject->Present_Value;
    }

    return cvalue;
}

/**
 * For a given object instance-number, determines the last priority.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return priority - priority 1..16
 */
unsigned Channel_Last_Priority(uint32_t object_instance)
{
    unsigned priority = 0;
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        priority = pObject->Last_Priority;
    }

    return priority;
}

/**
 * For a given object instance-number, determines the write status.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return BACNET_WRITE_STATUS value
 */
BACNET_WRITE_STATUS Channel_Write_Status(uint32_t object_instance)
{
    BACNET_WRITE_STATUS write_status = BACNET_WRITE_STATUS_IDLE;
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        write_status = pObject->Write_Status;
    }

    return write_status;
}

/**
 * For a given object instance-number, determines the Number
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return Channel Number value
 */
uint16_t Channel_Number(uint32_t object_instance)
{
    uint16_t value = 0;
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Number;
    }

    return value;
}

/**
 * For a given object instance-number, sets the channel-number
 * property value
 *
 * @param object_instance - object-instance number of the object
 * @param value - channel-number value to set
 *
 * @return true if set
 */
bool Channel_Number_Set(uint32_t object_instance, uint16_t value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Number = value;
        status = true;
    }

    return status;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet object instance number
 * @param array_index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
static int Channel_Reference_List_Member_Element_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value;
    unsigned count = 0;

    count = Channel_Reference_List_Member_Count(object_instance);
    if (array_index < count) {
        value = Channel_Reference_List_Member_Element(
            object_instance, array_index + 1);
        apdu_len = bacapp_encode_device_obj_property_ref(apdu, value);
    }

    return apdu_len;
}

/**
 * For a given object instance-number, determines the member count
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return member count
 */
static bool Channel_Reference_List_Member_Valid(
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember)
{
    bool status = false;

    if ((pMember) &&
        (pMember->objectIdentifier.instance != BACNET_MAX_INSTANCE) &&
        (pMember->deviceIdentifier.instance != BACNET_MAX_INSTANCE)) {
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, determines the member count
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return member count
 */
unsigned Channel_Reference_List_Member_Count(uint32_t object_instance)
{
    (void)object_instance;
    return CHANNEL_MEMBERS_MAX;
}

/**
 * For a given object instance-number, returns the member element
 *
 * @param object_instance - object-instance number of the object
 * @param  array_index - 1-based array index
 *
 * @return pointer to member element or NULL if not found
 */
BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *Channel_Reference_List_Member_Element(
    uint32_t object_instance, unsigned array_index)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject && (array_index > 0)) {
        array_index--;
        if (array_index < CHANNEL_MEMBERS_MAX) {
            pMember = &pObject->Members[array_index];
        }
    }

    return pMember;
}

/**
 * For a given object instance-number, returns the member element
 *
 * @param object_instance - object-instance number of the object
 * @param  array_index - 1-based array index
 *
 * @return pointer to member element or NULL if not found
 */
bool Channel_Reference_List_Member_Element_Set(
    uint32_t object_instance,
    unsigned array_index,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMemberSrc)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember = NULL;
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject && (array_index > 0)) {
        array_index--;
        if (array_index < CHANNEL_MEMBERS_MAX) {
            pMember = &pObject->Members[array_index];
            memcpy(
                pMember, pMemberSrc,
                sizeof(BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE));
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, adds a member element
 *
 * @param object_instance - object-instance number of the object
 * @param pMemberSrc - pointer to a object property reference element
 *
 * @return array_index - 1-based array index value for added element, or
 * zero if not added
 */
unsigned Channel_Reference_List_Member_Element_Add(
    uint32_t object_instance,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMemberSrc)
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember = NULL;
    unsigned array_index = 0;
    unsigned m = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        for (m = 0; m < CHANNEL_MEMBERS_MAX; m++) {
            pMember = &pObject->Members[m];
            if (!Channel_Reference_List_Member_Valid(pMember)) {
                /* first empty slot */
                array_index = 1 + m;
                memcpy(
                    pMember, pMemberSrc,
                    sizeof(BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE));
                break;
            }
        }
    }

    return array_index;
}

/**
 * For a given object instance-number, determines the Number
 *
 * @param  object_instance - object-instance number of the object
 * @param  array_index - 1-based array index
 *
 * @return group number in the array, or 0 if invalid
 */
uint16_t
Channel_Control_Groups_Element(uint32_t object_instance, int32_t array_index)
{
    uint16_t value = 0;
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if ((array_index > 0) && (array_index <= CONTROL_GROUPS_MAX)) {
            array_index--;
            value = pObject->Control_Groups[array_index];
        }
    }

    return value;
}

/**
 * For a given object instance-number, determines the Number
 *
 * @param object_instance - object-instance number of the object
 * @param array_index - 1-based array index
 * @param value - control group value 0..65535
 *
 * @return true if parameters are value and control group is set
 */
bool Channel_Control_Groups_Element_Set(
    uint32_t object_instance, int32_t array_index, uint16_t value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if ((array_index > 0) && (array_index <= CONTROL_GROUPS_MAX)) {
            array_index--;
            pObject->Control_Groups[array_index] = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
static int Channel_Control_Groups_Element_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    uint16_t value = 1;
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject && (array_index < CONTROL_GROUPS_MAX)) {
        value =
            Channel_Control_Groups_Element(object_instance, array_index + 1);
        apdu_len = encode_application_unsigned(apdu, value);
    }

    return apdu_len;
}

/**
 * For a given application value, copy to the channel value
 *
 * @param  cvalue - BACNET_CHANNEL_VALUE value
 * @param  value - BACNET_APPLICATION_DATA_VALUE value
 *
 * @return  true if values are able to be copied
 */
bool Channel_Value_Copy(
    BACNET_CHANNEL_VALUE *cvalue, const BACNET_APPLICATION_DATA_VALUE *value)
{
    bool status = false;

    if (!value || !cvalue) {
        return false;
    }
    switch (value->tag) {
#if defined(BACAPP_NULL)
        case BACNET_APPLICATION_TAG_NULL:
            cvalue->tag = value->tag;
            status = true;
            break;
#endif
#if defined(BACAPP_BOOLEAN) && defined(CHANNEL_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            cvalue->tag = value->tag;
            cvalue->type.Boolean = value->type.Boolean;
            status = true;
            break;
#endif
#if defined(BACAPP_UNSIGNED) && defined(CHANNEL_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            cvalue->tag = value->tag;
            cvalue->type.Unsigned_Int = value->type.Unsigned_Int;
            status = true;
            break;
#endif
#if defined(BACAPP_SIGNED) && defined(CHANNEL_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            cvalue->tag = value->tag;
            cvalue->type.Signed_Int = value->type.Signed_Int;
            status = true;
            break;
#endif
#if defined(BACAPP_REAL) && defined(CHANNEL_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            cvalue->tag = value->tag;
            cvalue->type.Real = value->type.Real;
            status = true;
            break;
#endif
#if defined(BACAPP_DOUBLE) && defined(CHANNEL_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            cvalue->tag = value->tag;
            cvalue->type.Double = value->type.Double;
            status = true;
            break;
#endif
#if defined(BACAPP_OCTET_STRING) && defined(CHANNEL_OCTET_STRING)
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            cvalue->tag = value->tag;
            octetstring_copy(
                &cvalue->type.Octet_String, &value->type.Octet_String);
            status = true;
            break;
#endif
#if defined(BACAPP_CHARACTER_STRING) && defined(CHANNEL_CHARACTER_STRING)
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            cvalue->tag = value->tag;
            characterstring_copy(
                &cvalue->type.Character_String, &value->type.Character_String);
            status = true;
            break;
#endif
#if defined(BACAPP_BIT_STRING) && defined(CHANNEL_BIT_STRING)
        case BACNET_APPLICATION_TAG_BIT_STRING:
            cvalue->tag = value->tag;
            bitstring_copy(&cvalue->type.Bit_String, &value->type.Bit_String);
            status = true;
            break;
#endif
#if defined(BACAPP_ENUMERATED) && defined(CHANNEL_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            cvalue->tag = value->tag;
            cvalue->type.Enumerated = value->type.Enumerated;
            status = true;
            break;
#endif
#if defined(BACAPP_DATE) && defined(CHANNEL_DATE)
        case BACNET_APPLICATION_TAG_DATE:
            cvalue->tag = value->tag;
            datetime_date_copy(&cvalue->type.Date, &value->type.Date);
            apdu_len = encode_application_date(apdu, &value->type.Date);
            status = true;
            break;
#endif
#if defined(BACAPP_TIME) && defined(CHANNEL_TIME)
        case BACNET_APPLICATION_TAG_TIME:
            cvalue->tag = value->tag;
            datetime_time_copy(&cvalue->type.Time, &value->type.Time);
            break;
#endif
#if defined(BACAPP_OBJECT_ID) && defined(CHANNEL_OBJECT_ID)
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            cvalue->tag = value->tag;
            cvalue->type.Object_Id.type = value->type.Object_Id.type;
            cvalue->type.Object_Id.instance = value->type.Object_Id.instance;
            status = true;
            break;
#endif
#if defined(BACAPP_TYPES_EXTRA) && defined(CHANNEL_LIGHTING_COMMAND)
        case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
            cvalue->tag = value->tag;
            lighting_command_copy(
                &cvalue->type.Lighting_Command, &value->type.Lighting_Command);
            status = true;
            break;
#endif
#if defined(BACAPP_TYPES_EXTRA) && defined(CHANNEL_COLOR_COMMAND)
        case BACNET_APPLICATION_TAG_COLOR_COMMAND:
            cvalue->tag = value->tag;
            color_command_copy(
                &cvalue->type.Color_Command, &value->type.Color_Command);
            status = true;
            break;
#endif
#if defined(BACAPP_TYPES_EXTRA) && defined(CHANNEL_XY_COLOR)
        case BACNET_APPLICATION_TAG_XY_COLOR:
            cvalue->tag = value->tag;
            xy_color_copy(&cvalue->type.XY_Color, &value->type.XY_Color);
            status = true;
            break;
#endif
        default:
            break;
    }

    return status;
}

/**
 * For a given application value, copy to the channel value
 *
 * @param  apdu - APDU buffer for storing the encoded data, or NULL for length
 * @param  apdu_max - size of APDU buffer available for storing data
 * @param  value - BACNET_CHANNEL_VALUE value
 *
 * @return  number of bytes in the APDU, or BACNET_STATUS_ERROR
 */
int Channel_Value_Encode(
    uint8_t *apdu, int apdu_max, const BACNET_CHANNEL_VALUE *value)
{
    return bacnet_channel_value_encode(apdu, apdu_max, value);
}

/**
 * For a given application value, coerce the encoding, if necessary
 *
 * @param  apdu - buffer to hold the encoding, or NULL for length
 * @param  value - BACNET_APPLICATION_DATA_VALUE value
 * @param  tag - application tag to be coerced, if possible
 *
 * @return  number of bytes in the APDU, or BACNET_STATUS_ERROR if error.
 */
static int Coerce_Data_Encode(
    uint8_t *apdu,
    const BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_APPLICATION_TAG tag)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    float float_value = 0.0;
    double double_value = 0.0;
    uint32_t unsigned_value = 0;
    int32_t signed_value = 0;
    bool boolean_value = false;

    if (value) {
        switch (value->tag) {
#if defined(BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                if ((tag == BACNET_APPLICATION_TAG_LIGHTING_COMMAND) ||
                    (tag == BACNET_APPLICATION_TAG_COLOR_COMMAND)) {
                    apdu_len = BACNET_STATUS_ERROR;
                } else {
                    /* no coercion */
                    if (apdu) {
                        *apdu = value->tag;
                    }
                    apdu_len++;
                }
                break;
#endif
#if defined(BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                if (tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                    apdu_len =
                        encode_application_boolean(apdu, value->type.Boolean);
                } else if (tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                    if (value->type.Boolean) {
                        unsigned_value = 1;
                    }
                    apdu_len =
                        encode_application_unsigned(apdu, unsigned_value);
                } else if (tag == BACNET_APPLICATION_TAG_SIGNED_INT) {
                    if (value->type.Boolean) {
                        signed_value = 1;
                    }
                    apdu_len = encode_application_signed(apdu, signed_value);
                } else if (tag == BACNET_APPLICATION_TAG_REAL) {
                    if (value->type.Boolean) {
                        float_value = 1;
                    }
                    apdu_len = encode_application_real(apdu, float_value);
                } else if (tag == BACNET_APPLICATION_TAG_DOUBLE) {
                    if (value->type.Boolean) {
                        double_value = 1;
                    }
                    apdu_len = encode_application_double(apdu, double_value);
                } else if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                    if (value->type.Boolean) {
                        unsigned_value = 1;
                    }
                    apdu_len =
                        encode_application_enumerated(apdu, unsigned_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
                break;
#endif
#if defined(BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                if (tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                    if (value->type.Unsigned_Int) {
                        boolean_value = true;
                    }
                    apdu_len = encode_application_boolean(apdu, boolean_value);
                } else if (tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                    unsigned_value = value->type.Unsigned_Int;
                    apdu_len =
                        encode_application_unsigned(apdu, unsigned_value);
                } else if (tag == BACNET_APPLICATION_TAG_SIGNED_INT) {
                    if (value->type.Unsigned_Int <= 2147483647) {
                        signed_value = value->type.Unsigned_Int;
                        apdu_len =
                            encode_application_signed(apdu, signed_value);
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                } else if (tag == BACNET_APPLICATION_TAG_REAL) {
                    if (value->type.Unsigned_Int <= 9999999) {
                        float_value = (float)value->type.Unsigned_Int;
                        apdu_len = encode_application_real(apdu, float_value);
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                } else if (tag == BACNET_APPLICATION_TAG_DOUBLE) {
                    double_value = (double)value->type.Unsigned_Int;
                    apdu_len = encode_application_double(apdu, double_value);
                } else if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                    unsigned_value = value->type.Unsigned_Int;
                    apdu_len =
                        encode_application_enumerated(apdu, unsigned_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
                break;
#endif
#if defined(BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                if (tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                    if (value->type.Signed_Int) {
                        boolean_value = true;
                    }
                    apdu_len = encode_application_boolean(apdu, boolean_value);
                } else if (tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                    if ((value->type.Signed_Int >= 0) &&
                        (value->type.Signed_Int <= 2147483647)) {
                        unsigned_value = value->type.Signed_Int;
                        apdu_len =
                            encode_application_unsigned(apdu, unsigned_value);
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                } else if (tag == BACNET_APPLICATION_TAG_SIGNED_INT) {
                    signed_value = value->type.Signed_Int;
                    apdu_len = encode_application_signed(apdu, signed_value);
                } else if (tag == BACNET_APPLICATION_TAG_REAL) {
                    if (value->type.Signed_Int <= 9999999) {
                        float_value = (float)value->type.Signed_Int;
                        apdu_len = encode_application_real(apdu, float_value);
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                } else if (tag == BACNET_APPLICATION_TAG_DOUBLE) {
                    double_value = (double)value->type.Signed_Int;
                    apdu_len = encode_application_double(apdu, double_value);
                } else if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                    unsigned_value = value->type.Signed_Int;
                    apdu_len =
                        encode_application_enumerated(apdu, unsigned_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
                break;
#endif
#if defined(BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                if (tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                    if (islessgreater(value->type.Real, 0.0F)) {
                        boolean_value = true;
                    }
                    apdu_len = encode_application_boolean(apdu, boolean_value);
                } else if (tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                    if ((value->type.Real >= 0.0F) &&
                        (value->type.Real <= 2147483000.0F)) {
                        unsigned_value = (uint32_t)value->type.Real;
                        apdu_len =
                            encode_application_unsigned(apdu, unsigned_value);
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                } else if (tag == BACNET_APPLICATION_TAG_SIGNED_INT) {
                    if ((value->type.Real >= -2147483000.0F) &&
                        (value->type.Real <= 214783000.0F)) {
                        signed_value = (int32_t)value->type.Real;
                        apdu_len =
                            encode_application_signed(apdu, signed_value);
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                } else if (tag == BACNET_APPLICATION_TAG_REAL) {
                    float_value = value->type.Real;
                    apdu_len = encode_application_real(apdu, float_value);
                } else if (tag == BACNET_APPLICATION_TAG_DOUBLE) {
                    double_value = value->type.Real;
                    apdu_len = encode_application_double(apdu, double_value);
                } else if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                    if ((value->type.Real >= 0.0F) &&
                        (value->type.Real <= 2147483000.0F)) {
                        unsigned_value = (uint32_t)value->type.Real;
                        apdu_len =
                            encode_application_enumerated(apdu, unsigned_value);
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                if (tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                    if (islessgreater(value->type.Double, 0.0)) {
                        boolean_value = true;
                    }
                    apdu_len = encode_application_boolean(apdu, boolean_value);
                } else if (tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                    if ((value->type.Double >= 0.0) &&
                        (value->type.Double <= 2147483000.0)) {
                        unsigned_value = (uint32_t)value->type.Double;
                        apdu_len =
                            encode_application_unsigned(apdu, unsigned_value);
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                } else if (tag == BACNET_APPLICATION_TAG_SIGNED_INT) {
                    if ((value->type.Double >= -2147483000.0) &&
                        (value->type.Double <= 214783000.0)) {
                        signed_value = (int32_t)value->type.Double;
                        apdu_len =
                            encode_application_signed(apdu, signed_value);
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                } else if (tag == BACNET_APPLICATION_TAG_REAL) {
                    if ((value->type.Double >= 3.4E-38) &&
                        (value->type.Double <= 3.4E+38)) {
                        float_value = (float)value->type.Double;
                        apdu_len = encode_application_real(apdu, float_value);
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                } else if (tag == BACNET_APPLICATION_TAG_DOUBLE) {
                    double_value = value->type.Double;
                    apdu_len = encode_application_double(apdu, double_value);
                } else if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                    if ((value->type.Double >= 0.0) &&
                        (value->type.Double <= 2147483000.0)) {
                        unsigned_value = (uint32_t)value->type.Double;
                        apdu_len =
                            encode_application_enumerated(apdu, unsigned_value);
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
                break;
#endif
#if defined(BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                if (tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                    if (value->type.Enumerated) {
                        boolean_value = true;
                    }
                    apdu_len = encode_application_boolean(apdu, boolean_value);
                } else if (tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                    unsigned_value = value->type.Enumerated;
                    apdu_len =
                        encode_application_unsigned(apdu, unsigned_value);
                } else if (tag == BACNET_APPLICATION_TAG_SIGNED_INT) {
                    if (value->type.Enumerated <= 2147483647) {
                        signed_value = value->type.Enumerated;
                        apdu_len =
                            encode_application_signed(apdu, signed_value);
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                } else if (tag == BACNET_APPLICATION_TAG_REAL) {
                    if (value->type.Enumerated <= 9999999) {
                        float_value = (float)value->type.Enumerated;
                        apdu_len = encode_application_real(apdu, float_value);
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                } else if (tag == BACNET_APPLICATION_TAG_DOUBLE) {
                    double_value = (double)value->type.Enumerated;
                    apdu_len = encode_application_double(apdu, double_value);
                } else if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                    unsigned_value = value->type.Enumerated;
                    apdu_len =
                        encode_application_enumerated(apdu, unsigned_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
                break;
#endif
#if defined(BACAPP_TYPES_EXTRA)
            case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
                if (tag == BACNET_APPLICATION_TAG_LIGHTING_COMMAND) {
                    apdu_len = lighting_command_encode(
                        apdu, &value->type.Lighting_Command);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
                break;
            case BACNET_APPLICATION_TAG_COLOR_COMMAND:
                if (tag == BACNET_APPLICATION_TAG_COLOR_COMMAND) {
                    apdu_len =
                        color_command_encode(apdu, &value->type.Color_Command);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
                break;
            case BACNET_APPLICATION_TAG_XY_COLOR:
                if (tag == BACNET_APPLICATION_TAG_XY_COLOR) {
                    apdu_len = xy_color_encode(apdu, &value->type.XY_Color);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
                break;
#endif
            default:
                apdu_len = BACNET_STATUS_ERROR;
                break;
        }
    }

    return apdu_len;
}

/**
 * For a given application value, coerce the encoding, if necessary
 *
 * @param  apdu - buffer to hold the encoding, or null for length
 * @param  value - BACNET_APPLICATION_DATA_VALUE value
 * @param  tag - application tag to be coerced, if possible
 *
 * @return  number of bytes in the APDU, or BACNET_STATUS_ERROR if error.
 */
int Channel_Coerce_Data_Encode(
    uint8_t *apdu,
    size_t apdu_size,
    const BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_APPLICATION_TAG tag)
{
    int len;

    len = Coerce_Data_Encode(NULL, value, tag);
    if ((len > 0) && (len <= apdu_size)) {
        len = Coerce_Data_Encode(apdu, value, tag);
    } else {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}

/**
 * For a given object instance-number, sets the present-value at a given
 * priority 1..16.
 *
 * @param  wp_data - all of the WriteProperty data structure
 *
 * @return  true if values are within range and present-value is sent.
 */
bool Channel_Write_Member_Value(
    BACNET_WRITE_PROPERTY_DATA *wp_data,
    const BACNET_APPLICATION_DATA_VALUE *value)
{
    bool status = false;
    int apdu_len = 0;

    if (wp_data && value) {
        if (((wp_data->object_type == OBJECT_ANALOG_INPUT) ||
             (wp_data->object_type == OBJECT_ANALOG_OUTPUT) ||
             (wp_data->object_type == OBJECT_ANALOG_VALUE)) &&
            (wp_data->object_property == PROP_PRESENT_VALUE) &&
            (wp_data->array_index == BACNET_ARRAY_ALL)) {
            apdu_len = Channel_Coerce_Data_Encode(
                wp_data->application_data, wp_data->application_data_len, value,
                BACNET_APPLICATION_TAG_REAL);
            if (apdu_len != BACNET_STATUS_ERROR) {
                wp_data->application_data_len = apdu_len;
                status = true;
            }
        } else if (
            ((wp_data->object_type == OBJECT_BINARY_INPUT) ||
             (wp_data->object_type == OBJECT_BINARY_OUTPUT) ||
             (wp_data->object_type == OBJECT_BINARY_VALUE)) &&
            (wp_data->object_property == PROP_PRESENT_VALUE) &&
            (wp_data->array_index == BACNET_ARRAY_ALL)) {
            apdu_len = Channel_Coerce_Data_Encode(
                wp_data->application_data, wp_data->application_data_len, value,
                BACNET_APPLICATION_TAG_ENUMERATED);
            if (apdu_len != BACNET_STATUS_ERROR) {
                wp_data->application_data_len = apdu_len;
                status = true;
            }
        } else if (
            ((wp_data->object_type == OBJECT_MULTI_STATE_INPUT) ||
             (wp_data->object_type == OBJECT_MULTI_STATE_OUTPUT) ||
             (wp_data->object_type == OBJECT_MULTI_STATE_VALUE)) &&
            (wp_data->object_property == PROP_PRESENT_VALUE) &&
            (wp_data->array_index == BACNET_ARRAY_ALL)) {
            apdu_len = Channel_Coerce_Data_Encode(
                wp_data->application_data, wp_data->application_data_len, value,
                BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (apdu_len != BACNET_STATUS_ERROR) {
                wp_data->application_data_len = apdu_len;
                status = true;
            }
        } else if (wp_data->object_type == OBJECT_LIGHTING_OUTPUT) {
            if ((wp_data->object_property == PROP_PRESENT_VALUE) &&
                (wp_data->array_index == BACNET_ARRAY_ALL)) {
                apdu_len = Channel_Coerce_Data_Encode(
                    wp_data->application_data, wp_data->application_data_len,
                    value, BACNET_APPLICATION_TAG_REAL);
                if (apdu_len != BACNET_STATUS_ERROR) {
                    wp_data->application_data_len = apdu_len;
                    status = true;
                }
            } else if (
                (wp_data->object_property == PROP_LIGHTING_COMMAND) &&
                (wp_data->array_index == BACNET_ARRAY_ALL)) {
                apdu_len = Channel_Coerce_Data_Encode(
                    wp_data->application_data, wp_data->application_data_len,
                    value, BACNET_APPLICATION_TAG_LIGHTING_COMMAND);
                if (apdu_len != BACNET_STATUS_ERROR) {
                    wp_data->application_data_len = apdu_len;
                    status = true;
                }
            }
        } else if (wp_data->object_type == OBJECT_COLOR) {
            if ((wp_data->object_property == PROP_PRESENT_VALUE) &&
                (wp_data->array_index == BACNET_ARRAY_ALL)) {
                apdu_len = Channel_Coerce_Data_Encode(
                    wp_data->application_data, wp_data->application_data_len,
                    value, BACNET_APPLICATION_TAG_XY_COLOR);
                if (apdu_len != BACNET_STATUS_ERROR) {
                    wp_data->application_data_len = apdu_len;
                    status = true;
                }
            } else if (
                (wp_data->object_property == PROP_COLOR_COMMAND) &&
                (wp_data->array_index == BACNET_ARRAY_ALL)) {
                apdu_len = Channel_Coerce_Data_Encode(
                    wp_data->application_data, wp_data->application_data_len,
                    value, BACNET_APPLICATION_TAG_COLOR_COMMAND);
                if (apdu_len != BACNET_STATUS_ERROR) {
                    wp_data->application_data_len = apdu_len;
                    status = true;
                }
            }
        } else if (wp_data->object_type == OBJECT_COLOR_TEMPERATURE) {
            apdu_len = Channel_Coerce_Data_Encode(
                wp_data->application_data, wp_data->application_data_len, value,
                BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (apdu_len != BACNET_STATUS_ERROR) {
                wp_data->application_data_len = apdu_len;
                status = true;
            }
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the present-value at a given
 * priority 1..16.
 *
 * @param pObject - object instance data
 * @param value - application value
 * @param priority - BACnet priority 0=none,1..16
 *
 * @return  true if values are within range and present-value is sent.
 */
static bool Channel_Write_Members(
    struct object_data *pObject,
    const BACNET_APPLICATION_DATA_VALUE *value,
    uint8_t priority)
{
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    bool status = false;
    unsigned m = 0;
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember = NULL;

    if (pObject && value) {
        pObject->Write_Status = BACNET_WRITE_STATUS_IN_PROGRESS;
        for (m = 0; m < CHANNEL_MEMBERS_MAX; m++) {
            pMember = &pObject->Members[m];
            /* NOTE: our implementation is for internal objects only */
            /* NOTE: we could check to match our Device ID, but then
               we would need to update all channels when our device ID
               changed.  Instead, we'll just screen when members are
               set. */
            if ((pMember->deviceIdentifier.type == OBJECT_DEVICE) &&
                (pMember->deviceIdentifier.instance != BACNET_MAX_INSTANCE) &&
                (pMember->objectIdentifier.instance != BACNET_MAX_INSTANCE)) {
                wp_data.object_type = pMember->objectIdentifier.type;
                wp_data.object_instance = pMember->objectIdentifier.instance;
                wp_data.object_property = pMember->propertyIdentifier;
                wp_data.array_index = pMember->arrayIndex;
                wp_data.priority = priority;
                wp_data.application_data_len = sizeof(wp_data.application_data);
                status = Channel_Write_Member_Value(&wp_data, value);
                if (status) {
                    if (Write_Property_Internal_Callback) {
                        status = Write_Property_Internal_Callback(&wp_data);
                    }
                } else {
                    pObject->Write_Status = BACNET_WRITE_STATUS_FAILED;
                }
            }
        }
        if (pObject->Write_Status == BACNET_WRITE_STATUS_IN_PROGRESS) {
            pObject->Write_Status = BACNET_WRITE_STATUS_SUCCESSFUL;
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the present-value at a given
 * priority 1..16.
 *
 * @param wp_data - all of the WriteProperty data structure
 * @param value - application value
 * @return true if values are within range and present-value is sent.
 */
bool Channel_Present_Value_Set(
    BACNET_WRITE_PROPERTY_DATA *wp_data,
    const BACNET_APPLICATION_DATA_VALUE *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, wp_data->object_instance);
    if (pObject) {
        if ((wp_data->priority > 0) &&
            (wp_data->priority <= BACNET_MAX_PRIORITY)) {
            if (wp_data->priority != 6 /* reserved */) {
                bacnet_channel_value_copy(
                    &pObject->Present_Value, &value->type.Channel_Value);
                (void)status;
                status =
                    Channel_Write_Members(pObject, value, wp_data->priority);
                (void)status;
                status = true;
            } else {
                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
        } else {
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        }
    }

    return status;
}

/**
 * For a given object instance-number, loads the object-name into
 * a characterstring. Note that the object name must be unique
 * within this device.
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - holds the object-name retrieved
 *
 * @return  true if object-name was retrieved
 */
bool Channel_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    char name_text[24] = "CHANNEL-4194303";
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                name_text, sizeof(name_text), "CHANNEL-%lu",
                (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, name_text);
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the object-name
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be set
 *
 * @return  true if object-name was set
 */
bool Channel_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = true;
        pObject->Object_Name = new_name;
    }

    return status;
}

/**
 * @brief Return the object name C string
 * @param object_instance [in] BACnet object instance number
 * @return object name or NULL if not found
 */
const char *Channel_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        name = pObject->Object_Name;
    }

    return name;
}

/**
 * For a given object instance-number, returns the out-of-service
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  out-of-service property value
 */
bool Channel_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    const struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Out_Of_Service;
    }

    return value;
}

/**
 * For a given object instance-number, sets the out-of-service property value
 *
 * @param object_instance - object-instance number of the object
 * @param value - boolean out-of-service value
 *
 * @return true if the out-of-service property value was set
 */
void Channel_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Out_Of_Service = value;
    }
}

/**
 * ReadProperty handler for this object.  For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  rpdata - ReadProperty data, including requested data and
 * data for the reply, or error response.
 *
 * @return number of APDU bytes in the response, or
 * BACNET_STATUS_ERROR on error.
 */
int Channel_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    const BACNET_CHANNEL_VALUE *cvalue = NULL;
    uint32_t unsigned_value = 0;
    unsigned count = 0;
    bool state = false;
    int apdu_size = 0;
    uint8_t *apdu = NULL;
    bool is_array;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    apdu_size = rpdata->application_data_len;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                apdu, OBJECT_CHANNEL, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Channel_Object_Name(rpdata->object_instance, &char_string);
            apdu_len = encode_application_character_string(apdu, &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(apdu, OBJECT_CHANNEL);
            break;
        case PROP_PRESENT_VALUE:
            cvalue = Channel_Present_Value(rpdata->object_instance);
            apdu_len = Channel_Value_Encode(apdu, apdu_size, cvalue);
            if (apdu_len == BACNET_STATUS_ERROR) {
                apdu_len = encode_application_null(apdu);
            }
            break;
        case PROP_LAST_PRIORITY:
            unsigned_value = Channel_Last_Priority(rpdata->object_instance);
            apdu_len = encode_application_unsigned(apdu, unsigned_value);
            break;
        case PROP_WRITE_STATUS:
            unsigned_value = (BACNET_WRITE_STATUS)Channel_Write_Status(
                rpdata->object_instance);
            apdu_len = encode_application_enumerated(apdu, unsigned_value);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Channel_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(apdu, &bit_string);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Channel_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(apdu, state);
            break;
        case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
            count =
                Channel_Reference_List_Member_Count(rpdata->object_instance);
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Channel_Reference_List_Member_Element_Encode, count, apdu,
                apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_CHANNEL_NUMBER:
            unsigned_value = Channel_Number(rpdata->object_instance);
            apdu_len = encode_application_unsigned(apdu, unsigned_value);
            break;
        case PROP_CONTROL_GROUPS:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Channel_Control_Groups_Element_Encode, CONTROL_GROUPS_MAX, apdu,
                apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    is_array = property_list_bacnet_array_member(
        rpdata->object_type, rpdata->object_property);
    if ((apdu_len >= 0) && (!is_array) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * WriteProperty handler for this object.  For the given WriteProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  wp_data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 *
 * @return false if an error is loaded, true if no errors
 */
bool Channel_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    int element_len = 0;
    uint32_t count = 0;
    bool is_array;

    /*  only array properties can have array options */
    is_array = property_list_bacnet_array_member(
        wp_data->object_type, wp_data->object_property);
    if (!is_array && (wp_data->array_index != BACNET_ARRAY_ALL)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    /* decode the first value of the request */
    len = bacapp_decode_known_property(
        wp_data->application_data, wp_data->application_data_len, &value,
        wp_data->object_type, wp_data->object_property);
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_CHANNEL_VALUE);
            if (status) {
                status = Channel_Present_Value_Set(wp_data, &value);
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Channel_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
            /* FIXME: add property handling */
            /*            status = */
            /*            Channel_List_Of_Object_Property_References_Set( */
            /*                wp_data, */
            /*                &value); */
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code =
                ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
            break;
        case PROP_CHANNEL_NUMBER:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                Channel_Number_Set(
                    wp_data->object_instance, value.type.Unsigned_Int);
            }
            break;
        case PROP_CONTROL_GROUPS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                if (wp_data->array_index == 0) {
                    /* Array element zero is the number of elements in the array
                     */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                } else if (wp_data->array_index == BACNET_ARRAY_ALL) {
                    count = CONTROL_GROUPS_MAX;
                    /* extra elements still encoded in application data */
                    element_len = len;
                    do {
                        if ((element_len > 0) &&
                            (value.tag ==
                             BACNET_APPLICATION_TAG_UNSIGNED_INT)) {
                            if ((wp_data->array_index <= CONTROL_GROUPS_MAX) &&
                                (value.type.Unsigned_Int <= 65535)) {
                                status = Channel_Control_Groups_Element_Set(
                                    wp_data->object_instance,
                                    wp_data->array_index,
                                    value.type.Unsigned_Int);
                            }
                            if (!status) {
                                wp_data->error_class = ERROR_CLASS_PROPERTY;
                                wp_data->error_code =
                                    ERROR_CODE_VALUE_OUT_OF_RANGE;
                                break;
                            }
                        }
                        count--;
                        if (count) {
                            element_len = bacapp_decode_application_data(
                                &wp_data->application_data[len],
                                wp_data->application_data_len - len, &value);
                            if (element_len < 0) {
                                wp_data->error_class = ERROR_CLASS_PROPERTY;
                                wp_data->error_code =
                                    ERROR_CODE_VALUE_OUT_OF_RANGE;
                                break;
                            }
                            len += element_len;
                        }
                    } while (count);
                } else {
                    if ((wp_data->array_index <= CONTROL_GROUPS_MAX) &&
                        (value.type.Unsigned_Int <= 65535)) {
                        status = Channel_Control_Groups_Element_Set(
                            wp_data->object_instance, wp_data->array_index,
                            value.type.Unsigned_Int);
                    }
                    if (!status) {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                }
            }
            break;
        default:
            if (property_lists_member(
                    Channel_Properties_Required, Channel_Properties_Optional,
                    Channel_Properties_Proprietary, wp_data->object_property)) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            }
            break;
    }

    return status;
}

/**
 * @brief Sets a callback used when present-value is written from BACnet
 * @param cb - callback used to provide indications
 */
void Channel_Write_Property_Internal_Callback_Set(write_property_function cb)
{
    Write_Property_Internal_Callback = cb;
}

/**
 * @brief Creates a new object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Channel_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    int index = 0;
    unsigned m, g;

    if (object_instance > BACNET_MAX_INSTANCE) {
        return BACNET_MAX_INSTANCE;
    } else if (object_instance == BACNET_MAX_INSTANCE) {
        /* wildcard instance */
        /* the Object_Identifier property of the newly created object
            shall be initialized to a value that is unique within the
            responding BACnet-user device. The method used to generate
            the object identifier is a local matter.*/
        object_instance = Keylist_Next_Empty_Key(Object_List, 1);
    }
    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        pObject = calloc(1, sizeof(struct object_data));
        if (pObject) {
            /* channel defaults */
            pObject->Object_Name = NULL;
            pObject->Present_Value.tag = BACNET_APPLICATION_TAG_EMPTYLIST;
            pObject->Out_Of_Service = false;
            pObject->Last_Priority = BACNET_NO_PRIORITY;
            pObject->Write_Status = BACNET_WRITE_STATUS_IDLE;
            for (m = 0; m < CHANNEL_MEMBERS_MAX; m++) {
                pObject->Members[m].objectIdentifier.type =
                    OBJECT_LIGHTING_OUTPUT;
                pObject->Members[m].objectIdentifier.instance =
                    BACNET_MAX_INSTANCE;
                pObject->Members[m].propertyIdentifier = PROP_PRESENT_VALUE;
                pObject->Members[m].arrayIndex = BACNET_ARRAY_ALL;
                pObject->Members[m].deviceIdentifier.type = OBJECT_DEVICE;
                pObject->Members[m].deviceIdentifier.instance =
                    BACNET_MAX_INSTANCE;
            }
            pObject->Number = 0;
            for (g = 0; g < CONTROL_GROUPS_MAX; g++) {
                pObject->Control_Groups[g] = 0;
            }
            /* add to list */
            index = Keylist_Data_Add(Object_List, object_instance, pObject);
            if (index < 0) {
                free(pObject);
                return BACNET_MAX_INSTANCE;
            }
        } else {
            return BACNET_MAX_INSTANCE;
        }
    }

    return object_instance;
}

/**
 * Deletes a dynamically created object
 * @param object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Channel_Delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        free(pObject);
        status = true;
    }

    return status;
}

/**
 * Deletes all the dynamic objects and their data
 */
void Channel_Cleanup(void)
{
    struct object_data *pObject;

    if (Object_List) {
        do {
            pObject = Keylist_Data_Pop(Object_List);
            if (pObject) {
                free(pObject);
            }
        } while (pObject);
        Keylist_Delete(Object_List);
        Object_List = NULL;
    }
}

/**
 * Initializes the object data
 */
void Channel_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
