/**
 * @file
 * @brief A basic BACnet Analog Input Object implementation.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Krzysztof Malorny <malornykrzysztof@gmail.com>
 * @date 2005, 2011
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bactext.h"
#include "bacnet/datetime.h"
#include "bacnet/proplist.h"
#include "bacnet/timestamp.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/basic/sys/debug.h"
/* me! */
#include "bacnet/basic/object/ai.h"

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_ANALOG_INPUT;

/* clang-format off */
/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME, PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE, PROP_STATUS_FLAGS, PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE, PROP_UNITS, -1
};

static const int Properties_Optional[] = { 
    PROP_DESCRIPTION, PROP_RELIABILITY,  PROP_COV_INCREMENT,
#if defined(INTRINSIC_REPORTING)
    PROP_TIME_DELAY, PROP_NOTIFICATION_CLASS, PROP_HIGH_LIMIT,
    PROP_LOW_LIMIT, PROP_DEADBAND, PROP_LIMIT_ENABLE, PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS, PROP_NOTIFY_TYPE, PROP_EVENT_TIME_STAMPS,
#endif
    -1 
};

static const int Properties_Proprietary[] = { 
    -1 
};
/* clang-format on */

/**
 * Initialize the pointers for the required, the optional and the properitary
 * value properties.
 *
 * @param pRequired - Pointer to the pointer of required values.
 * @param pOptional - Pointer to the pointer of optional values.
 * @param pProprietary - Pointer to the pointer of properitary values.
 */
void Analog_Input_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
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
 * @brief Gets an object from the list using an instance number as the key
 * @param  object_instance - object-instance number of the object
 * @return object found in the list, or NULL if not found
 */
static struct analog_input_descr *Analog_Input_Object(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

#if defined(INTRINSIC_REPORTING)
/**
 * @brief Gets an object from the list using its index in the list
 * @param index - index of the object in the list
 * @return object found in the list, or NULL if not found
 */
static struct analog_input_descr *Analog_Input_Object_Index(int index)
{
    return Keylist_Data_Index(Object_List, index);
}
#endif

/**
 * @brief Determines if a given object instance is valid
 * @param  object_instance - object-instance number of the object
 * @return  true if the instance is valid, and false if not
 */
bool Analog_Input_Valid_Instance(uint32_t object_instance)
{
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * @brief Determines the number of objects
 * @return  Number of objects
 */
unsigned Analog_Input_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Determines the object instance-number for a given 0..(N-1) index
 * of objects where N is Analog_Input_Count().
 * @param  index - 0..(N-1) where N is Analog_Input_Count().
 * @return  object instance-number for the given index
 */
uint32_t Analog_Input_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * @brief For a given object instance-number, determines a 0..(N-1) index
 * of objects where N is Analog_Input_Count().
 * @param  object_instance - object-instance number of the object
 * @return  index for the given instance-number, or >= Analog_Input_Count()
 * if not valid.
 */
unsigned Analog_Input_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * @brief For a given object instance-number, determines the present-value
 * @param  object_instance - object-instance number of the object
 * @return  present-value of the object
 */
float Analog_Input_Present_Value(uint32_t object_instance)
{
    float value = 0.0f;
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        value = pObject->Present_Value;
    }

    return value;
}

/**
 * This function is used to detect a value change,
 * using the new value compared against the prior
 * value, using a delta as threshold.
 *
 * This method will update the COV-changed attribute.
 *
 * @param index  Object index
 * @param value  Given present value.
 */
static void
Analog_Input_COV_Detect(struct analog_input_descr *pObject, float value)
{
    float prior_value = 0.0f;
    float cov_increment = 0.0f;
    float cov_delta = 0.0f;

    if (pObject) {
        prior_value = pObject->Prior_Value;
        cov_increment = pObject->COV_Increment;
        if (prior_value > value) {
            cov_delta = prior_value - value;
        } else {
            cov_delta = value - prior_value;
        }
        if (cov_delta >= cov_increment) {
            pObject->Changed = true;
            pObject->Prior_Value = value;
        }
    }
}

/**
 * For a given object instance-number, sets the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - floating point analog value
 * @return  true if values are within range and present-value is set.
 */
void Analog_Input_Present_Value_Set(uint32_t object_instance, float value)
{
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        Analog_Input_COV_Detect(pObject, value);
        pObject->Present_Value = value;
    }
}

/**
 * For a given object instance-number, return the name.
 *
 * Note: the object name must be unique within this device
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - object name/string pointer
 *
 * @return  true/false
 */
bool Analog_Input_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32] = "";
    bool status = false;
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                text_string, sizeof(text_string), "ANALOG INPUT %lu",
                (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, text_string);
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
bool Analog_Input_Name_Set(uint32_t object_instance, char *new_name)
{
    bool status = false;
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        status = true;
        pObject->Object_Name = new_name;
    }

    return status;
}

/**
 * For a given object instance-number, gets the event-state property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  event-state property value
 */
unsigned Analog_Input_Event_State(uint32_t object_instance)
{
    unsigned state = EVENT_STATE_NORMAL;
#if defined(INTRINSIC_REPORTING)
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        state = pObject->Event_State;
    }
#else
    (void)object_instance;
#endif

    return state;
}

/**
 * @brief For a given object instance-number, returns the description
 * @param  object_instance - object-instance number of the object
 * @return description text or NULL if not found
 */
char *Analog_Input_Description(uint32_t object_instance)
{
    char *name = NULL;
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        name = pObject->Description;
    }

    return name;
}

/**
 * @brief For a given object instance-number, sets the description
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 * @return  true if object-name was set
 */
bool Analog_Input_Description_Set(uint32_t object_instance, char *new_name)
{
    bool status = false; /* return value */
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        pObject->Description = new_name;
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the reliability
 * @param  object_instance - object-instance number of the object
 * @return reliability property value
 */
BACNET_RELIABILITY Analog_Input_Reliability(uint32_t object_instance)
{
    BACNET_RELIABILITY value = RELIABILITY_NO_FAULT_DETECTED;
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        value = pObject->Reliability;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the reliability
 * @param  object_instance - object-instance number of the object
 * @param  value - reliability property value
 * @return  true if the reliability property value was set
 */
bool Analog_Input_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value)
{
    bool status = false;
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        pObject->Reliability = value;
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, determines the COV status
 * @param  object_instance - object-instance number of the object
 * @return  true if the COV flag is set
 */
bool Analog_Input_Change_Of_Value(uint32_t object_instance)
{
    bool changed = false;
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        changed = pObject->Changed;
    }

    return changed;
}

/**
 * @brief For a given object instance-number, clears the COV flag
 * @param  object_instance - object-instance number of the object
 */
void Analog_Input_Change_Of_Value_Clear(uint32_t object_instance)
{
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        pObject->Changed = false;
    }
}

/**
 * For a given object instance-number, loads the value_list with the COV data.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value_list - list of COV data
 *
 * @return  true if the value list is encoded
 */
bool Analog_Input_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list)
{
    bool status = false;
    bool in_alarm = false;
    bool out_of_service = false;
    bool fault = false;
    const bool overridden = false;
    float present_value = 0.0f;
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        if (pObject->Event_State != EVENT_STATE_NORMAL) {
            in_alarm = true;
        }
        if (pObject->Reliability != RELIABILITY_NO_FAULT_DETECTED) {
            fault = true;
        }
        out_of_service = pObject->Out_Of_Service;
        present_value = pObject->Present_Value;
        status = cov_value_list_encode_real(
            value_list, present_value, in_alarm, fault, overridden,
            out_of_service);
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the COV-Increment value
 * @param  object_instance - object-instance number of the object
 * @return  COV-Increment value
 */
float Analog_Input_COV_Increment(uint32_t object_instance)
{
    float value = 0.0f;
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        value = pObject->COV_Increment;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the COV-Increment value
 * @param  object_instance - object-instance number of the object
 * @param  value - COV-Increment value
 */
void Analog_Input_COV_Increment_Set(uint32_t object_instance, float value)
{
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        pObject->COV_Increment = value;
        Analog_Input_COV_Detect(pObject, pObject->Present_Value);
    }
}

/**
 * For a given object instance-number, returns the units property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  units property value
 */
uint16_t Analog_Input_Units(uint32_t object_instance)
{
    uint16_t units = UNITS_NO_UNITS;
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        units = pObject->Units;
    }

    return units;
}

/**
 * For a given object instance-number, sets the units property value
 *
 * @param object_instance - object-instance number of the object
 * @param units - units property value
 *
 * @return true if the units property value was set
 */
bool Analog_Input_Units_Set(uint32_t object_instance, uint16_t units)
{
    bool status = false;
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        pObject->Units = units;
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the out-of-service
 * property value
 * @param object_instance - object-instance number of the object
 * @return out-of-service property value
 */
bool Analog_Input_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        value = pObject->Out_Of_Service;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the out-of-service property
 * value
 * @param object_instance - object-instance number of the object
 * @param value - boolean out-of-service value
 * @return true if the out-of-service property value was set
 */
void Analog_Input_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        if (pObject->Out_Of_Service != value) {
            pObject->Changed = true;
        }
        pObject->Out_Of_Service = value;
    }
}

#if defined(INTRINSIC_REPORTING)
/**
 * @brief Encode a EventTimeStamps property element
 * @param object_instance [in] BACnet network port object instance number
 * @param index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
static int Analog_Input_Event_Time_Stamps_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = 0, len = 0;
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object(object_instance);
    if (pObject) {
        if (index < MAX_BACNET_EVENT_TRANSITION) {
            len = encode_opening_tag(apdu, TIME_STAMP_DATETIME);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_date(
                apdu, &pObject->Event_Time_Stamps[index].date);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_time(
                apdu, &pObject->Event_Time_Stamps[index].time);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_closing_tag(apdu, TIME_STAMP_DATETIME);
            apdu_len += len;
        } else {
            apdu_len = BACNET_STATUS_ERROR;
        }
    } else {
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}
#endif

/**
 * @brief For a given object instance-number, handles the ReadProperty service
 * @param  rpdata Property requested, see for BACNET_READ_PROPERTY_DATA details.
 * @return apdu len, or BACNET_STATUS_ERROR on error
 */
int Analog_Input_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    uint8_t *apdu = NULL;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    float real_value = (float)1.414;
#if defined(INTRINSIC_REPORTING)
    int apdu_size = 0;
#endif
    struct analog_input_descr *pObject;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    pObject = Analog_Input_Object(rpdata->object_instance);
    if (!pObject) {
        return BACNET_STATUS_ERROR;
    }
    apdu = rpdata->application_data;
#if defined(INTRINSIC_REPORTING)
    apdu_size = rpdata->application_data_len;
#endif
    switch ((int)rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], Object_Type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Analog_Input_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_PRESENT_VALUE:
            real_value = Analog_Input_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(
                &bit_string, STATUS_FLAG_IN_ALARM,
                pObject->Event_State != EVENT_STATE_NORMAL);
            bitstring_set_bit(
                &bit_string, STATUS_FLAG_FAULT,
                pObject->Reliability != RELIABILITY_NO_FAULT_DETECTED);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(
                &bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                pObject->Out_Of_Service);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len = encode_application_enumerated(
                &apdu[0], Analog_Input_Event_State(rpdata->object_instance));
            break;
        case PROP_RELIABILITY:
            apdu_len =
                encode_application_enumerated(&apdu[0], pObject->Reliability);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len =
                encode_application_boolean(&apdu[0], pObject->Out_Of_Service);
            break;
        case PROP_UNITS:
            apdu_len = encode_application_enumerated(&apdu[0], pObject->Units);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string,
                Analog_Input_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_COV_INCREMENT:
            apdu_len =
                encode_application_real(&apdu[0], pObject->COV_Increment);
            break;
#if defined(INTRINSIC_REPORTING)
        case PROP_TIME_DELAY:
            apdu_len =
                encode_application_unsigned(&apdu[0], pObject->Time_Delay);
            break;
        case PROP_NOTIFICATION_CLASS:
            apdu_len = encode_application_unsigned(
                &apdu[0], pObject->Notification_Class);
            break;
        case PROP_HIGH_LIMIT:
            apdu_len = encode_application_real(&apdu[0], pObject->High_Limit);
            break;
        case PROP_LOW_LIMIT:
            apdu_len = encode_application_real(&apdu[0], pObject->Low_Limit);
            break;
        case PROP_DEADBAND:
            apdu_len = encode_application_real(&apdu[0], pObject->Deadband);
            break;
        case PROP_LIMIT_ENABLE:
            bitstring_init(&bit_string);
            bitstring_set_bit(
                &bit_string, 0,
                (pObject->Limit_Enable & EVENT_LOW_LIMIT_ENABLE) ? true
                                                                 : false);
            bitstring_set_bit(
                &bit_string, 1,
                (pObject->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) ? true
                                                                  : false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_ENABLE:
            bitstring_init(&bit_string);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_OFFNORMAL,
                (pObject->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ? true
                                                                    : false);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_FAULT,
                (pObject->Event_Enable & EVENT_ENABLE_TO_FAULT) ? true : false);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_NORMAL,
                (pObject->Event_Enable & EVENT_ENABLE_TO_NORMAL) ? true
                                                                 : false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_ACKED_TRANSITIONS:
            bitstring_init(&bit_string);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_OFFNORMAL,
                pObject->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_FAULT,
                pObject->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_NORMAL,
                pObject->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_NOTIFY_TYPE:
            apdu_len = encode_application_enumerated(
                &apdu[0], pObject->Notify_Type ? NOTIFY_EVENT : NOTIFY_ALARM);
            break;
        case PROP_EVENT_TIME_STAMPS:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Analog_Input_Event_Time_Stamps_Encode,
                MAX_BACNET_EVENT_TRANSITION, apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
#endif
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) &&
        (rpdata->object_property != PROP_EVENT_TIME_STAMPS) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief WriteProperty handler for this object.  For the given WriteProperty
 * data, the application_data is loaded or the error flags are set.
 * @param  wp_data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return false if an error is loaded, true if no errors
 */
bool Analog_Input_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    struct analog_input_descr *pObject;

    /* Valid data? */
    if (wp_data == NULL) {
        return false;
    }
    if (wp_data->application_data_len == 0) {
        return false;
    }
    /* decode the some of the request */
    len = bacapp_decode_application_data(
        wp_data->application_data, wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    /*  only array properties can have array options */
    if ((wp_data->object_property != PROP_EVENT_TIME_STAMPS) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    pObject = Analog_Input_Object(wp_data->object_instance);
    if (!pObject) {
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                if (pObject->Out_Of_Service == true) {
                    Analog_Input_Present_Value_Set(
                        wp_data->object_instance, value.type.Real);
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                    status = false;
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Analog_Input_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_UNITS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                pObject->Units = value.type.Enumerated;
            }
            break;
        case PROP_COV_INCREMENT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                if (value.type.Real >= 0.0f) {
                    Analog_Input_COV_Increment_Set(
                        wp_data->object_instance, value.type.Real);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
#if defined(INTRINSIC_REPORTING)
        case PROP_TIME_DELAY:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                pObject->Time_Delay = value.type.Unsigned_Int;
                pObject->Remaining_Time_Delay = pObject->Time_Delay;
            }
            break;
        case PROP_NOTIFICATION_CLASS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                pObject->Notification_Class = value.type.Unsigned_Int;
            }
            break;
        case PROP_HIGH_LIMIT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                pObject->High_Limit = value.type.Real;
            }
            break;
        case PROP_LOW_LIMIT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                pObject->Low_Limit = value.type.Real;
            }
            break;
        case PROP_DEADBAND:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                pObject->Deadband = value.type.Real;
            }
            break;
        case PROP_LIMIT_ENABLE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BIT_STRING);
            if (status) {
                if (value.type.Bit_String.bits_used == 2) {
                    pObject->Limit_Enable = value.type.Bit_String.value[0];
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    status = false;
                }
            }
            break;
        case PROP_EVENT_ENABLE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BIT_STRING);
            if (status) {
                if (value.type.Bit_String.bits_used == 3) {
                    pObject->Event_Enable = value.type.Bit_String.value[0];
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    status = false;
                }
            }
            break;
        case PROP_NOTIFY_TYPE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                switch ((BACNET_NOTIFY_TYPE)value.type.Enumerated) {
                    case NOTIFY_EVENT:
                        pObject->Notify_Type = 1;
                        break;
                    case NOTIFY_ALARM:
                        pObject->Notify_Type = 0;
                        break;
                    default:
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                        status = false;
                        break;
                }
            }
            break;
#endif
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
 * @brief Handles the Intrinsic Reporting Service for the Analog Input Object
 * @param  object_instance - object-instance number of the object
 */
void Analog_Input_Intrinsic_Reporting(uint32_t object_instance)
{
#if defined(INTRINSIC_REPORTING)
    BACNET_EVENT_NOTIFICATION_DATA event_data = { 0 };
    BACNET_CHARACTER_STRING msgText = { 0 };
    struct analog_input_descr *CurrentAI = NULL;
    uint8_t FromState = 0;
    uint8_t ToState = 0;
    float ExceededLimit = 0.0f;
    float PresentVal = 0.0f;
    bool SendNotify = false;

    CurrentAI = Analog_Input_Object(object_instance);
    if (!CurrentAI) {
        return;
    }
    /* check limits */
    if (!CurrentAI->Limit_Enable) {
        return; /* limits are not configured */
    }
    if (CurrentAI->Ack_notify_data.bSendAckNotify) {
        /* clean bSendAckNotify flag */
        CurrentAI->Ack_notify_data.bSendAckNotify = false;
        /* copy toState */
        ToState = CurrentAI->Ack_notify_data.EventState;
        debug_printf(
            "Analog-Input[%d]: Send AckNotification.\n", object_instance);
        characterstring_init_ansi(&msgText, "AckNotification");
        /* Notify Type */
        event_data.notifyType = NOTIFY_ACK_NOTIFICATION;
        /* Send EventNotification. */
        SendNotify = true;
    } else {
        /* actual Present_Value */
        PresentVal = Analog_Input_Present_Value(object_instance);
        FromState = CurrentAI->Event_State;
        switch (CurrentAI->Event_State) {
            case EVENT_STATE_NORMAL:
                /* A TO-OFFNORMAL event is generated under these conditions:
                   (a) the Present_Value must exceed the High_Limit for a
                   minimum period of time, specified in the Time_Delay property,
                   and (b) the HighLimitEnable flag must be set in the
                   Limit_Enable property, and
                   (c) the TO-OFFNORMAL flag must be set in the Event_Enable
                   property. */
                if ((PresentVal > CurrentAI->High_Limit) &&
                    ((CurrentAI->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) ==
                     EVENT_HIGH_LIMIT_ENABLE) &&
                    ((CurrentAI->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ==
                     EVENT_ENABLE_TO_OFFNORMAL)) {
                    if (!CurrentAI->Remaining_Time_Delay)
                        CurrentAI->Event_State = EVENT_STATE_HIGH_LIMIT;
                    else
                        CurrentAI->Remaining_Time_Delay--;
                    break;
                }
                /* A TO-OFFNORMAL event is generated under these conditions:
                   (a) the Present_Value must exceed the Low_Limit plus the
                   Deadband for a minimum period of time, specified in the
                   Time_Delay property, and (b) the LowLimitEnable flag must be
                   set in the Limit_Enable property, and
                   (c) the TO-NORMAL flag must be set in the Event_Enable
                   property. */
                if ((PresentVal < CurrentAI->Low_Limit) &&
                    ((CurrentAI->Limit_Enable & EVENT_LOW_LIMIT_ENABLE) ==
                     EVENT_LOW_LIMIT_ENABLE) &&
                    ((CurrentAI->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ==
                     EVENT_ENABLE_TO_OFFNORMAL)) {
                    if (!CurrentAI->Remaining_Time_Delay)
                        CurrentAI->Event_State = EVENT_STATE_LOW_LIMIT;
                    else
                        CurrentAI->Remaining_Time_Delay--;
                    break;
                }
                /* value of the object is still in the same event state */
                CurrentAI->Remaining_Time_Delay = CurrentAI->Time_Delay;
                break;
            case EVENT_STATE_HIGH_LIMIT:
                /* Once exceeded, the Present_Value must fall below the
                   High_Limit minus the Deadband before a TO-NORMAL event is
                   generated under these conditions: (a) the Present_Value must
                   fall below the High_Limit minus the Deadband for a minimum
                   period of time, specified in the Time_Delay property, and (b)
                   the HighLimitEnable flag must be set in the Limit_Enable
                   property, and (c) the TO-NORMAL flag must be set in the
                   Event_Enable property. */
                if (
                     ((PresentVal <
                         CurrentAI->High_Limit - CurrentAI->Deadband) &&
                      ((CurrentAI->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) ==
                          EVENT_HIGH_LIMIT_ENABLE) &&
                      ((CurrentAI->Event_Enable & EVENT_ENABLE_TO_NORMAL) ==
                          EVENT_ENABLE_TO_NORMAL)) ||
                      /* 13.3.6 (c) If pCurrentState is HIGH_LIMIT, and the HighLimitEnable flag of pLimitEnable is FALSE,
                       * then indicate a transition to the NORMAL event state. */
                      (!(CurrentAI->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE))
                   ) {
                    if ((!CurrentAI->Remaining_Time_Delay) || (!(CurrentAI->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE)))
                        CurrentAI->Event_State = EVENT_STATE_NORMAL;
                    else
                        CurrentAI->Remaining_Time_Delay--;
                    break;
                }
                /* value of the object is still in the same event state */
                CurrentAI->Remaining_Time_Delay = CurrentAI->Time_Delay;
                break;
            case EVENT_STATE_LOW_LIMIT:
                /* Once the Present_Value has fallen below the Low_Limit,
                   the Present_Value must exceed the Low_Limit plus the Deadband
                   before a TO-NORMAL event is generated under these conditions:
                   (a) the Present_Value must exceed the Low_Limit plus the
                   Deadband for a minimum period of time, specified in the
                   Time_Delay property, and (b) the LowLimitEnable flag must be
                   set in the Limit_Enable property, and
                   (c) the TO-NORMAL flag must be set in the Event_Enable
                   property. */
                if (
                    ((PresentVal > CurrentAI->Low_Limit + CurrentAI->Deadband) &&
                    ((CurrentAI->Limit_Enable & EVENT_LOW_LIMIT_ENABLE) ==
                     EVENT_LOW_LIMIT_ENABLE) &&
                    ((CurrentAI->Event_Enable & EVENT_ENABLE_TO_NORMAL) ==
                        EVENT_ENABLE_TO_NORMAL)) ||
                    /* 13.3.6 (f) If pCurrentState is LOW_LIMIT, and the LowLimitEnable flag of pLimitEnable is FALSE,
                     * then indicate a transition to the NORMAL event state. */
                    (!(CurrentAI->Limit_Enable & EVENT_LOW_LIMIT_ENABLE))
                   ) {
                    if ( (!CurrentAI->Remaining_Time_Delay) || (!(CurrentAI->Limit_Enable & EVENT_LOW_LIMIT_ENABLE)) )
                        CurrentAI->Event_State = EVENT_STATE_NORMAL;
                    else
                        CurrentAI->Remaining_Time_Delay--;
                    break;
                }
                /* value of the object is still in the same event state */
                CurrentAI->Remaining_Time_Delay = CurrentAI->Time_Delay;
                break;
            default:
                return; /* shouldn't happen */
        } /* switch (FromState) */
        ToState = CurrentAI->Event_State;
        if (FromState != ToState) {
            /* Event_State has changed.
               Need to fill only the basic parameters of this type of event.
               Other parameters will be filled in common function. */
            switch (ToState) {
                case EVENT_STATE_HIGH_LIMIT:
                    ExceededLimit = CurrentAI->High_Limit;
                    characterstring_init_ansi(&msgText, "Goes to high limit");
                    break;

                case EVENT_STATE_LOW_LIMIT:
                    ExceededLimit = CurrentAI->Low_Limit;
                    characterstring_init_ansi(&msgText, "Goes to low limit");
                    break;

                case EVENT_STATE_NORMAL:
                    if (FromState == EVENT_STATE_HIGH_LIMIT) {
                        ExceededLimit = CurrentAI->High_Limit;
                        characterstring_init_ansi(
                            &msgText, "Back to normal state from high limit");
                    } else {
                        ExceededLimit = CurrentAI->Low_Limit;
                        characterstring_init_ansi(
                            &msgText, "Back to normal state from low limit");
                    }
                    break;

                default:
                    ExceededLimit = 0;
                    break;
            } /* switch (ToState) */
            debug_printf(
                "Analog-Input[%d]: Event_State goes from %s to %s.\n",
                object_instance, bactext_event_state_name(FromState),
                bactext_event_state_name(ToState));
            /* Notify Type */
            event_data.notifyType = CurrentAI->Notify_Type;

            /* Send EventNotification. */
            SendNotify = true;
        }
    }
    if (SendNotify) {
        /* Event Object Identifier */
        event_data.eventObjectIdentifier.type = Object_Type;
        event_data.eventObjectIdentifier.instance = object_instance;
        /* Time Stamp */
        event_data.timeStamp.tag = TIME_STAMP_DATETIME;
        if (event_data.notifyType != NOTIFY_ACK_NOTIFICATION) {
            datetime_local(
                &event_data.timeStamp.value.dateTime.date,
                &event_data.timeStamp.value.dateTime.time, NULL, NULL);
            /* fill Event_Time_Stamps */
            switch (ToState) {
                case EVENT_STATE_HIGH_LIMIT:
                case EVENT_STATE_LOW_LIMIT:
                    datetime_copy(
                        &CurrentAI->Event_Time_Stamps[TRANSITION_TO_OFFNORMAL],
                        &event_data.timeStamp.value.dateTime);
                    break;
                case EVENT_STATE_FAULT:
                    datetime_copy(
                        &CurrentAI->Event_Time_Stamps[TRANSITION_TO_FAULT],
                        &event_data.timeStamp.value.dateTime);
                    break;
                case EVENT_STATE_NORMAL:
                    datetime_copy(
                        &CurrentAI->Event_Time_Stamps[TRANSITION_TO_NORMAL],
                        &event_data.timeStamp.value.dateTime);
                    break;
                default:
                    break;
            }
        } else {
            /* fill event_data timeStamp */
            switch (ToState) {
                case EVENT_STATE_HIGH_LIMIT:
                case EVENT_STATE_LOW_LIMIT:
                    datetime_copy(
                        &event_data.timeStamp.value.dateTime,
                        &CurrentAI->Event_Time_Stamps[TRANSITION_TO_OFFNORMAL]);
                    break;
                case EVENT_STATE_FAULT:
                    datetime_copy(
                        &event_data.timeStamp.value.dateTime,
                        &CurrentAI->Event_Time_Stamps[TRANSITION_TO_FAULT]);
                    break;
                case EVENT_STATE_NORMAL:
                    datetime_copy(
                        &event_data.timeStamp.value.dateTime,
                        &CurrentAI->Event_Time_Stamps[TRANSITION_TO_NORMAL]);
                    break;
                default:
                    break;
            }
        }
        /* Notification Class */
        event_data.notificationClass = CurrentAI->Notification_Class;
        /* Event Type */
        event_data.eventType = EVENT_OUT_OF_RANGE;
        /* Message Text */
        event_data.messageText = &msgText;
        /* Notify Type */
        /* filled before */
        /* From State */
        if (event_data.notifyType != NOTIFY_ACK_NOTIFICATION)
            event_data.fromState = FromState;
        /* To State */
        event_data.toState = CurrentAI->Event_State;
        /* Event Values */
        if (event_data.notifyType != NOTIFY_ACK_NOTIFICATION) {
            /* Value that exceeded a limit. */
            event_data.notificationParams.outOfRange.exceedingValue =
                PresentVal;
            /* Status_Flags of the referenced object. */
            bitstring_init(
                &event_data.notificationParams.outOfRange.statusFlags);
            bitstring_set_bit(
                &event_data.notificationParams.outOfRange.statusFlags,
                STATUS_FLAG_IN_ALARM,
                CurrentAI->Event_State != EVENT_STATE_NORMAL);
            bitstring_set_bit(
                &event_data.notificationParams.outOfRange.statusFlags,
                STATUS_FLAG_FAULT, false);
            bitstring_set_bit(
                &event_data.notificationParams.outOfRange.statusFlags,
                STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(
                &event_data.notificationParams.outOfRange.statusFlags,
                STATUS_FLAG_OUT_OF_SERVICE, CurrentAI->Out_Of_Service);
            /* Deadband used for limit checking. */
            event_data.notificationParams.outOfRange.deadband =
                CurrentAI->Deadband;
            /* Limit that was exceeded. */
            event_data.notificationParams.outOfRange.exceededLimit =
                ExceededLimit;
        }
        /* add data from notification class */
        debug_printf(
            "Analog-Input[%d]: Notification Class[%d]-%s "
            "%u/%u/%u-%u:%u:%u.%u!\n",
            object_instance, event_data.notificationClass,
            bactext_event_type_name(event_data.eventType),
            (unsigned)event_data.timeStamp.value.dateTime.date.year,
            (unsigned)event_data.timeStamp.value.dateTime.date.month,
            (unsigned)event_data.timeStamp.value.dateTime.date.day,
            (unsigned)event_data.timeStamp.value.dateTime.time.hour,
            (unsigned)event_data.timeStamp.value.dateTime.time.min,
            (unsigned)event_data.timeStamp.value.dateTime.time.sec,
            (unsigned)event_data.timeStamp.value.dateTime.time.hundredths);
        Notification_Class_common_reporting_function(&event_data);
        /* Ack required */
        if ((event_data.notifyType != NOTIFY_ACK_NOTIFICATION) &&
            (event_data.ackRequired == true)) {
            debug_printf("Analog-Input[%d]: Ack Required!\n", object_instance);
            switch (event_data.toState) {
                case EVENT_STATE_OFFNORMAL:
                case EVENT_STATE_HIGH_LIMIT:
                case EVENT_STATE_LOW_LIMIT:
                    CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                        .bIsAcked = false;
                    CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;
                case EVENT_STATE_FAULT:
                    CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked =
                        false;
                    CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;
                case EVENT_STATE_NORMAL:
                    CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL]
                        .bIsAcked = false;
                    CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;
                default: /* shouldn't happen */
                    break;
            }
        }
    }
#else
    (void)object_instance;
#endif /* defined(INTRINSIC_REPORTING) */
}

#if defined(INTRINSIC_REPORTING)
/**
 * @brief Handles getting the Event Information for the Analog Input Object
 * @param  index - index number of the object 0..count
 * @param  getevent_data - data for the Event Information
 * @return 1 if an active event is found, 0 if no active event, -1 if
 * end of list
 */
int Analog_Input_Event_Information(
    unsigned index, BACNET_GET_EVENT_INFORMATION_DATA *getevent_data)
{
    bool IsNotAckedTransitions;
    bool IsActiveEvent;
    int i;
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object_Index(index);
    if (pObject) {
        /* Event_State not equal to NORMAL */
        IsActiveEvent = (pObject->Event_State != EVENT_STATE_NORMAL);

        /* Acked_Transitions property, which has at least one of the bits
           (TO-OFFNORMAL, TO-FAULT, TONORMAL) set to FALSE. */
        IsNotAckedTransitions =
            (pObject->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked ==
             false) |
            (pObject->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked ==
             false) |
            (pObject->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked ==
             false);
    } else {
        return -1; /* end of list  */
    }
    if ((IsActiveEvent) || (IsNotAckedTransitions)) {
        /* Object Identifier */
        getevent_data->objectIdentifier.type = Object_Type;
        getevent_data->objectIdentifier.instance =
            Analog_Input_Index_To_Instance(index);
        /* Event State */
        getevent_data->eventState = pObject->Event_State;
        /* Acknowledged Transitions */
        bitstring_init(&getevent_data->acknowledgedTransitions);
        bitstring_set_bit(
            &getevent_data->acknowledgedTransitions, TRANSITION_TO_OFFNORMAL,
            pObject->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked);
        bitstring_set_bit(
            &getevent_data->acknowledgedTransitions, TRANSITION_TO_FAULT,
            pObject->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked);
        bitstring_set_bit(
            &getevent_data->acknowledgedTransitions, TRANSITION_TO_NORMAL,
            pObject->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked);
        /* Event Time Stamps */
        for (i = 0; i < 3; i++) {
            getevent_data->eventTimeStamps[i].tag = TIME_STAMP_DATETIME;
            getevent_data->eventTimeStamps[i].value.dateTime =
                pObject->Event_Time_Stamps[i];
        }
        /* Notify Type */
        getevent_data->notifyType = pObject->Notify_Type;
        /* Event Enable */
        bitstring_init(&getevent_data->eventEnable);
        bitstring_set_bit(
            &getevent_data->eventEnable, TRANSITION_TO_OFFNORMAL,
            (pObject->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ? true : false);
        bitstring_set_bit(
            &getevent_data->eventEnable, TRANSITION_TO_FAULT,
            (pObject->Event_Enable & EVENT_ENABLE_TO_FAULT) ? true : false);
        bitstring_set_bit(
            &getevent_data->eventEnable, TRANSITION_TO_NORMAL,
            (pObject->Event_Enable & EVENT_ENABLE_TO_NORMAL) ? true : false);
        /* Event Priorities */
        Notification_Class_Get_Priorities(
            pObject->Notification_Class, getevent_data->eventPriorities);

        return 1; /* active event */
    } else
        return 0; /* no active event at this index */
}

/**
 * @brief Acknowledges the Event Information for the Analog Input Object
 * @param alarmack_data - data for the Event Acknowledgement
 * @param error_code - error code for the Event Acknowledgement
 * @return 1 if successful, -1 if error, -2 if request is out-of-range
 */
int Analog_Input_Alarm_Ack(
    BACNET_ALARM_ACK_DATA *alarmack_data, BACNET_ERROR_CODE *error_code)
{
    struct analog_input_descr *CurrentAI;

    if (!alarmack_data) {
        return -1;
    }
    CurrentAI =
        Analog_Input_Object(alarmack_data->eventObjectIdentifier.instance);
    if (!CurrentAI) {
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return -1;
    }
    switch (alarmack_data->eventStateAcked) {
        case EVENT_STATE_OFFNORMAL:
        case EVENT_STATE_HIGH_LIMIT:
        case EVENT_STATE_LOW_LIMIT:
            if (CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                    .bIsAcked == false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                /* Send ack notification */
                CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked =
                    true;
            } else if (
                alarmack_data->eventStateAcked == CurrentAI->Event_State) {
                /* Send ack notification */
            } else {
                *error_code = ERROR_CODE_INVALID_EVENT_STATE;
                return -1;
            }
            break;

        case EVENT_STATE_FAULT:
            if (CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked ==
                false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                /* Send ack notification */
                CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked =
                    true;
            } else if (
                alarmack_data->eventStateAcked == CurrentAI->Event_State) {
                /* Send ack notification */
            } else {
                *error_code = ERROR_CODE_INVALID_EVENT_STATE;
                return -1;
            }
            break;

        case EVENT_STATE_NORMAL:
            if (CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked ==
                false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                /* Send ack notification */
                CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked =
                    true;
            } else if (
                alarmack_data->eventStateAcked == CurrentAI->Event_State) {
                /* Send ack notification */
            } else {
                *error_code = ERROR_CODE_INVALID_EVENT_STATE;
                return -1;
            }
            break;

        default:
            return -2;
    }
    /* Need to send AckNotification. */
    CurrentAI->Ack_notify_data.bSendAckNotify = true;
    CurrentAI->Ack_notify_data.EventState = alarmack_data->eventStateAcked;

    return 1;
}

/**
 * @brief Handles getting the Alarm Summary for the Analog Input Object
 * @param  index - index number of the object 0..count
 * @param  getalarm_data - data for the Alarm Summary
 * @return 1 if an active alarm is found, 0 if no active alarm, -1 if
 * end of list
 */
int Analog_Input_Alarm_Summary(
    unsigned index, BACNET_GET_ALARM_SUMMARY_DATA *getalarm_data)
{
    struct analog_input_descr *pObject;

    pObject = Analog_Input_Object_Index(index);
    if (pObject) {
        /* Event_State is not equal to NORMAL  and
           Notify_Type property value is ALARM */
        if ((pObject->Event_State != EVENT_STATE_NORMAL) &&
            (pObject->Notify_Type == NOTIFY_ALARM)) {
            /* Object Identifier */
            getalarm_data->objectIdentifier.type = Object_Type;
            getalarm_data->objectIdentifier.instance =
                Analog_Input_Index_To_Instance(index);
            /* Alarm State */
            getalarm_data->alarmState = pObject->Event_State;
            /* Acknowledged Transitions */
            bitstring_init(&getalarm_data->acknowledgedTransitions);
            bitstring_set_bit(
                &getalarm_data->acknowledgedTransitions,
                TRANSITION_TO_OFFNORMAL,
                pObject->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked);
            bitstring_set_bit(
                &getalarm_data->acknowledgedTransitions, TRANSITION_TO_FAULT,
                pObject->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked);
            bitstring_set_bit(
                &getalarm_data->acknowledgedTransitions, TRANSITION_TO_NORMAL,
                pObject->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked);
            return 1; /* active alarm */
        } else {
            return 0; /* no active alarm at this index */
        }
    } else {
        return -1; /* end of list  */
    }
}
#endif /* defined(INTRINSIC_REPORTING) */

/**
 * @brief Creates a Analog Input object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Analog_Input_Create(uint32_t object_instance)
{
    struct analog_input_descr *pObject = NULL;
    int index = 0;
#if defined(INTRINSIC_REPORTING)
    unsigned j;
#endif

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
        pObject = calloc(1, sizeof(struct analog_input_descr));
        if (pObject) {
            pObject->Object_Name = NULL;
            pObject->Description = NULL;
            pObject->Reliability = RELIABILITY_NO_FAULT_DETECTED;
            pObject->COV_Increment = 1.0;
            pObject->Present_Value = 0.0f;
            pObject->Prior_Value = 0.0;
            pObject->Units = UNITS_PERCENT;
            pObject->Out_Of_Service = false;
            pObject->Changed = false;
            pObject->Event_State = EVENT_STATE_NORMAL;
#if defined(INTRINSIC_REPORTING)
            /* notification class not connected */
            pObject->Notification_Class = BACNET_MAX_INSTANCE;
            /* initialize Event time stamps using wildcards
            and set Acked_transitions */
            for (j = 0; j < MAX_BACNET_EVENT_TRANSITION; j++) {
                datetime_wildcard_set(&pObject->Event_Time_Stamps[j]);
                pObject->Acked_Transitions[j].bIsAcked = true;
            }
#endif
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
 * @brief Deletes an Analog Input object
 * @param object_instance - object-instance number of the object
 * @return true if the object-instance was deleted
 */
bool Analog_Input_Delete(uint32_t object_instance)
{
    bool status = false;
    struct analog_input_descr *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        free(pObject);
        status = true;
    }

    return status;
}

/**
 * @brief Deletes all the Analog Inputs and their data
 */
void Analog_Input_Cleanup(void)
{
    struct analog_input_descr *pObject;

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
 * @brief Initializes the Analog Input object data
 */
void Analog_Input_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
#if defined(INTRINSIC_REPORTING)
    /* Set handler for GetEventInformation function */
    handler_get_event_information_set(
        Object_Type, Analog_Input_Event_Information);
    /* Set handler for AcknowledgeAlarm function */
    handler_alarm_ack_set(Object_Type, Analog_Input_Alarm_Ack);
    /* Set handler for GetAlarmSummary Service */
    handler_get_alarm_summary_set(Object_Type, Analog_Input_Alarm_Summary);
#endif
}
