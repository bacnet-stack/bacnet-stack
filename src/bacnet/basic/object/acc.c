/**
 * @file
 * @brief BACnet accumulator Objects used to represent meter registers
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2017
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/proplist.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/basic/object/acc.h"

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List = NULL;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_ACCUMULATOR;

struct object_data {
    BACNET_UNSIGNED_INTEGER Present_Value;
    BACNET_UNSIGNED_INTEGER Max_Pres_Value;
    BACNET_CHARACTER_STRING Object_Name;
    const char *Description;
    BACNET_ENGINEERING_UNITS Units;
    int32_t Scale;
    bool Out_Of_Service : 1;
    void *Context;
};

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    PROP_SCALE,
    PROP_UNITS,
    PROP_MAX_PRES_VALUE,
    -1
};

static const int32_t Properties_Optional[] = { PROP_DESCRIPTION, -1 };

static const int32_t Properties_Proprietary[] = { -1 };

/* Every object shall have a Writable Property_List property
   which is a BACnetARRAY of property identifiers,
   one property identifier for each property within this object
   that is always writable.  */
static const int32_t Writable_Properties[] = {
    /* unordered list of writable properties */
    PROP_OBJECT_NAME,
    PROP_PRESENT_VALUE,
    PROP_OUT_OF_SERVICE,
    PROP_SCALE,
    PROP_UNITS,
    PROP_MAX_PRES_VALUE,
    -1
};

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
void Accumulator_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary)
{
    if (pRequired) {
        *pRequired = Properties_Required;
    }
    if (pOptional) {
        *pOptional = Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Properties_Proprietary;
    }

    return;
}

/**
 * @brief Get the list of writable properties for an Accumulator object
 * @param  object_instance - object-instance number of the object
 * @param  properties - Pointer to the pointer of writable properties.
 */
void Accumulator_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties)
{
    (void)object_instance;
    if (properties) {
        *properties = Writable_Properties;
    }
}

/**
 * @brief Gets an object from the list using an instance number as the key
 * @param  object_instance - object-instance number of the object
 * @return object found in the list, or NULL if not found
 */
static struct object_data *Object_Data(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * Determines if a given Accumulator instance is valid
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if the instance is valid, and false if not
 */
bool Accumulator_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject = Object_Data(object_instance);

    return (pObject != NULL);
}

/**
 * Determines the number of Accumulator objects
 *
 * @return  Number of Accumulator objects
 */
unsigned Accumulator_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Accumulator objects where N is Accumulator_Count().
 *
 * @param  index - 0..Accumulator_Count() value
 *
 * @return  object instance-number for the given index
 */
uint32_t Accumulator_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Accumulator objects where N is Accumulator_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or MAX_ACCUMULATORS
 * if not valid.
 */
unsigned Accumulator_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
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
bool Accumulator_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        status = characterstring_copy(object_name, &pObject->Object_Name);
    }

    return status;
}

/**
 * For a given object instance-number, sets the object-name from
 * a characterstring. Note that the object name must be unique
 * within this device.
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - holds the object-name to be set
 *
 * @return  true if object-name was set
 */
bool Accumulator_Object_Name_Set(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        status = characterstring_copy(&pObject->Object_Name, object_name);
    }

    return status;
}

/**
 * For a given object instance-number, determines the present-value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  present-value of the object
 */
BACNET_UNSIGNED_INTEGER Accumulator_Present_Value(uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER value = 0;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        value = pObject->Present_Value;
    }

    return value;
}

/**
 * For a given object instance-number, sets the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACNET_UNSIGNED_INTEGER value
 *
 * @return  true if values are within range and present-value is set.
 */
bool Accumulator_Present_Value_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    bool status = false;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        pObject->Present_Value = value;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, returns the units property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  units property value
 */
BACNET_ENGINEERING_UNITS Accumulator_Units(uint32_t object_instance)
{
    BACNET_ENGINEERING_UNITS units = UNITS_NO_UNITS;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        units = pObject->Units;
    }

    return units;
}

/**
 * For a given object instance-number, sets the units property value
 *
 * @param  object_instance - object-instance number of the object
 * @param  units - BACNET_ENGINEERING_UNITS value
 * @return true if the units is set successfully
 */
bool Accumulator_Units_Set(uint32_t instance, BACNET_ENGINEERING_UNITS units)
{
    bool status = false;
    struct object_data *pObject = Object_Data(instance);

    if (pObject) {
        pObject->Units = units;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, returns the scale property value
 *
 * Option         Datatype    Indicated Value in Units
 * float-scale    REAL        Present_Value x Scale
 * integer-scale  INTEGER     Present_Value x 10 Scale
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  scale property integer value
 */
int32_t Accumulator_Scale_Integer(uint32_t object_instance)
{
    int32_t scale = 0;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        scale = pObject->Scale;
    }

    return scale;
}

/**
 * For a given object instance-number, returns the scale property value
 *
 * Option         Datatype    Indicated Value in Units
 * float-scale    REAL        Present_Value x Scale
 * integer-scale  INTEGER     Present_Value x 10 Scale
 *
 * @param  object_instance - object-instance number of the object
 * @param  scale -  scale property integer value
 *
 * @return  true if valid object and value is within range
 */
bool Accumulator_Scale_Integer_Set(uint32_t object_instance, int32_t scale)
{
    bool status = false;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        pObject->Scale = scale;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, returns the max-present-value
 *
 * @param  object_instance - object-instance number of the object
 * @return  max-present-value of the object
 */
BACNET_UNSIGNED_INTEGER Accumulator_Max_Pres_Value(uint32_t object_instance)
{
    BACNET_UNSIGNED_INTEGER max_value = 0;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        max_value = pObject->Max_Pres_Value;
    }

    return max_value;
}

/**
 * For a given object instance-number, sets the max-present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACNET_UNSIGNED_INTEGER value
 *
 * @return  true if values are within range and max-present-value is set.
 */
bool Accumulator_Max_Pres_Value_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    bool status = false;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        pObject->Max_Pres_Value = value;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, returns the description
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  description text or NULL if not found
 */
const char *Accumulator_Description(uint32_t instance)
{
    const char *name = NULL;
    struct object_data *pObject = Object_Data(instance);

    if (pObject) {
        name = pObject->Description;
    }

    return name;
}

/**
 * For a given object instance-number, sets the description
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 */
bool Accumulator_Description_Set(uint32_t instance, const char *new_name)
{
    bool status = false;
    struct object_data *pObject = Object_Data(instance);

    if (pObject) {
        pObject->Description = new_name;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, returns the Out_Of_Service property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  Out_Of_Service property value
 */
bool Accumulator_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        value = pObject->Out_Of_Service;
    }

    return value;
}

/**
 * For a given object instance-number, sets the Out_Of_Service property value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - Out_Of_Service property value to be set
 * @return true if the value is set successfully
 */
bool Accumulator_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    bool status = false;
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        pObject->Out_Of_Service = value;
        status = true;
    }

    return status;
}

/**
 * ReadProperty handler for this object.  For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  rpdata - BACNET_READ_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 *
 * @return number of APDU bytes in the response, or
 * BACNET_STATUS_ERROR on error.
 */
int Accumulator_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch ((int)rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], Object_Type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Accumulator_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_unsigned(
                &apdu[0], Accumulator_Present_Value(rpdata->object_instance));
            break;
        case PROP_SCALE:
            /* context tagged choice: [0]=REAL, [1]=INTEGER */
            apdu_len = encode_context_signed(
                &apdu[apdu_len], 1,
                Accumulator_Scale_Integer(rpdata->object_instance));
            break;
        case PROP_MAX_PRES_VALUE:
            apdu_len = encode_application_unsigned(
                &apdu[0], Accumulator_Max_Pres_Value(rpdata->object_instance));
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(
                &bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                Accumulator_Out_Of_Service(rpdata->object_instance));
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(
                &apdu[0], Accumulator_Out_Of_Service(rpdata->object_instance));
            break;
        case PROP_UNITS:
            apdu_len = encode_application_enumerated(
                &apdu[0], Accumulator_Units(rpdata->object_instance));
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string, Accumulator_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
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
 * WriteProperty handler for this object.  For the given WriteProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  wp_data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 *
 * @return false if an error is loaded, true if no errors
 */
bool Accumulator_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    int len = 0;
    bool status = false;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    /* decode the some of the request */
    len = bacapp_decode_known_array_property(
        wp_data->application_data, wp_data->application_data_len, &value,
        wp_data->object_type, wp_data->object_property, wp_data->array_index);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    switch ((int)wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                if (value.type.Unsigned_Int <=
                    Accumulator_Max_Pres_Value(wp_data->object_instance)) {
                    Accumulator_Present_Value_Set(
                        wp_data->object_instance, value.type.Unsigned_Int);
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    status = false;
                }
            }
            break;
        case PROP_OBJECT_NAME:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_CHARACTER_STRING);
            if (status) {
                Accumulator_Object_Name_Set(
                    wp_data->object_instance, &value.type.Character_String);
            }
            break;
        case PROP_SCALE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_SCALE);
            if (status) {
                if (value.type.Scale.float_scale) {
                    /* we only support integer scale, so reject if float scale
                     * is used */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    status = false;
                } else {
                    Accumulator_Scale_Integer_Set(
                        wp_data->object_instance, value.type.Signed_Int);
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Accumulator_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_UNITS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated <= UINT16_MAX) {
                    Accumulator_Units_Set(
                        wp_data->object_instance, value.type.Enumerated);
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    status = false;
                }
            }
            break;
        case PROP_MAX_PRES_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                Accumulator_Max_Pres_Value_Set(
                    wp_data->object_instance, value.type.Unsigned_Int);
            }
            break;
        default:
            if (property_lists_member(
                    Properties_Required, Properties_Optional,
                    Properties_Proprietary, wp_data->object_property)) {
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
 * @brief Set the context used with a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void *Accumulator_Context_Get(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return pObject->Context;
    }

    return NULL;
}

/**
 * @brief Set the context used with a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void Accumulator_Context_Set(uint32_t object_instance, void *context)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Context = context;
    }
}

/**
 * @brief Creates a CharacterString Value object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Accumulator_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    char text[32] = "";
    int index;

    if (!Object_List) {
        Object_List = Keylist_Create();
    }
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
            /* add to list */
            index = Keylist_Data_Add(Object_List, object_instance, pObject);
            if (index < 0) {
                free(pObject);
                return BACNET_MAX_INSTANCE;
            }
            characterstring_init_ansi(
                &pObject->Object_Name,
                bacnet_snprintf_to_ascii(
                    text, sizeof(text), "ACCUMULATOR-%lu",
                    (unsigned long)object_instance));
            pObject->Description = "";
            pObject->Present_Value = 0;
            pObject->Units = UNITS_WATT_HOURS;
            pObject->Scale = 1;
            pObject->Max_Pres_Value = BACNET_UNSIGNED_INTEGER_MAX;
            /* Out_Of_Service is used to simulate a fault condition,
               so set to false by default */
            pObject->Out_Of_Service = false;
        } else {
            return BACNET_MAX_INSTANCE;
        }
    }

    return object_instance;
}

/**
 * @brief Delete an object and its data from the object list
 * @param  object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Accumulator_Delete(uint32_t object_instance)
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
 * Initialize the character string values.
 */
void Accumulator_Init(void)
{
    /* nothing to do */
}
