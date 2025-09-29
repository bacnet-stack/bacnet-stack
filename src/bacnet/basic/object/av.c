/**
 * @file
 * @brief A basic BACnet Analog Input Object implementation.
 * An analog value object is an I/O object with a present-value that
 * uses an single precision floating point data type.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Krzysztof Malorny <malornykrzysztof@gmail.com>
 * @date 2006, 2011
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "bacnet/basic/object/av.h"

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_ANALOG_VALUE;
/* callback for present value writes */
static analog_value_write_present_value_callback
    Analog_Value_Write_Present_Value_Callback;

/* clang-format off */
/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Analog_Value_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME, PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE, PROP_STATUS_FLAGS, PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE, PROP_UNITS, -1
};

static const int Analog_Value_Properties_Optional[] = {
    PROP_DESCRIPTION, PROP_RELIABILITY, PROP_COV_INCREMENT,
#if defined(INTRINSIC_REPORTING)
    PROP_TIME_DELAY, PROP_NOTIFICATION_CLASS, PROP_HIGH_LIMIT,
    PROP_LOW_LIMIT, PROP_DEADBAND, PROP_LIMIT_ENABLE, PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS, PROP_NOTIFY_TYPE, PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_DETECTION_ENABLE,
#endif
    -1
};

static const int Analog_Value_Properties_Proprietary[] = {
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
void Analog_Value_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Analog_Value_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Analog_Value_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Analog_Value_Properties_Proprietary;
    }

    return;
}

/**
 * @brief Gets an object from the list using an instance number as the key
 * @param  object_instance - object-instance number of the object
 * @return object found in the list, or NULL if not found
 */
static struct analog_value_descr *Analog_Value_Object(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

#if defined(INTRINSIC_REPORTING)
/**
 * @brief Gets an object from the list using its index in the list
 * @param index - index of the object in the list
 * @return object found in the list, or NULL if not found
 */
static struct analog_value_descr *Analog_Value_Object_Index(int index)
{
    return Keylist_Data_Index(Object_List, index);
}
#endif

/**
 * @brief Determines if a given object instance is valid
 * @param  object_instance - object-instance number of the object
 * @return  true if the instance is valid, and false if not
 */
bool Analog_Value_Valid_Instance(uint32_t object_instance)
{
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * @brief Determines the number of objects
 * @return  Number of objects
 */
unsigned Analog_Value_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Determines the object instance-number for a given 0..(N-1) index
 * of objects where N is Analog_Value_Count().
 * @param  index - 0..(N-1) where N is Analog_Value_Count().
 * @return  object instance-number for the given index
 */
uint32_t Analog_Value_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * @brief For a given object instance-number, determines a 0..(N-1) index
 * of objects where N is Analog_Value_Count().
 * @param  object_instance - object-instance number of the object
 * @return  index for the given instance-number, or >= Analog_Value_Count()
 * if not valid.
 */
unsigned Analog_Value_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * @brief For a given object instance-number, determines the present value.
 * @param  object_instance - object-instance number of the object
 * @return  present-value of the object
 */
float Analog_Value_Present_Value(uint32_t object_instance)
{
    float value = 0.0f;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
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
Analog_Value_COV_Detect(struct analog_value_descr *pObject, float value)
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
 * For a given object instance-number, sets the present-value at a given
 * priority 1..16.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - floating point analog value
 * @param  priority - priority 1..16
 *
 * @return  true if values are within range and present-value is set.
 */
bool Analog_Value_Present_Value_Set(
    uint32_t object_instance, float value, uint8_t priority)
{
    bool status = false;
    struct analog_value_descr *pObject;

    (void)priority;
    pObject = Analog_Value_Object(object_instance);
    if (pObject) {
        Analog_Value_COV_Detect(pObject, value);
        pObject->Present_Value = value;
        status = true;
    }

    return status;
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
bool Analog_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text_string[32] = "";
    bool status = false;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                text_string, sizeof(text_string), "ANALOG VALUE %lu",
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
bool Analog_Value_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
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
const char *Analog_Value_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
    if (pObject) {
        name = pObject->Object_Name;
    }

    return name;
}

/**
 * For a given object instance-number, gets the event-state property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  event-state property value
 */
unsigned Analog_Value_Event_State(uint32_t object_instance)
{
    unsigned state = EVENT_STATE_NORMAL;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
    if (pObject) {
        state = pObject->Event_State;
    }

    return state;
}

/**
 * For a given object instance-number, gets the event-detection-enable property
 * value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  event-detection-enable property value
 */
bool Analog_Value_Event_Detection_Enable(uint32_t object_instance)
{
    bool retval = false;
#if !defined(INTRINSIC_REPORTING)
    (void)object_instance;
#else
    struct analog_value_descr *pObject = Analog_Value_Object(object_instance);

    if (pObject) {
        retval = pObject->Event_Detection_Enable;
    }
#endif

    return retval;
}

/**
 * For a given object instance-number, sets the event-detection-enable property
 * value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  event-detection-enable property value
 */
bool Analog_Value_Event_Detection_Enable_Set(
    uint32_t object_instance, bool value)
{
    bool retval = false;
#if !defined(INTRINSIC_REPORTING)
    (void)object_instance;
    (void)value;
#else
    struct analog_value_descr *pObject = Analog_Value_Object(object_instance);

    if (pObject) {
        pObject->Event_Detection_Enable = value;
        retval = true;
    }
#endif

    return retval;
}

/**
 * @brief For a given object instance-number, returns the description
 * @param  object_instance - object-instance number of the object
 * @return description text or NULL if not found
 */
const char *Analog_Value_Description(uint32_t object_instance)
{
    const char *name = NULL;
    const struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
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
bool Analog_Value_Description_Set(
    uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
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
BACNET_RELIABILITY Analog_Value_Reliability(uint32_t object_instance)
{
    BACNET_RELIABILITY value = RELIABILITY_NO_FAULT_DETECTED;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
    if (pObject) {
        value = pObject->Reliability;
    }

    return value;
}

/**
 * @brief For a given object, gets the Fault status flag
 * @param  object_instance - object-instance number of the object
 * @return  true the status flag is in Fault
 */
static bool Analog_Value_Object_Fault(const struct analog_value_descr *pObject)
{
    bool fault = false;

    if (pObject) {
        if (pObject->Reliability != RELIABILITY_NO_FAULT_DETECTED) {
            fault = true;
        }
    }

    return fault;
}

/**
 * @brief For a given object instance-number, sets the reliability
 * @param  object_instance - object-instance number of the object
 * @param  value - reliability property value
 * @return  true if the reliability property value was set
 */
bool Analog_Value_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value)
{
    bool status = false;
    bool fault = false;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
    if (pObject) {
        fault = Analog_Value_Object_Fault(pObject);
        pObject->Reliability = value;
        if (fault != Analog_Value_Object_Fault(pObject)) {
            pObject->Changed = true;
        }
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the Fault status flag
 * @param  object_instance - object-instance number of the object
 * @return  true the status flag is in Fault
 */
static bool Analog_Value_Fault(uint32_t object_instance)
{
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);

    return Analog_Value_Object_Fault(pObject);
}

/**
 * @brief For a given object instance-number, determines the COV status
 * @param  object_instance - object-instance number of the object
 * @return  true if the COV flag is set
 */
bool Analog_Value_Change_Of_Value(uint32_t object_instance)
{
    bool changed = false;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
    if (pObject) {
        changed = pObject->Changed;
    }

    return changed;
}

/**
 * @brief For a given object instance-number, clears the COV flag
 * @param  object_instance - object-instance number of the object
 */
void Analog_Value_Change_Of_Value_Clear(uint32_t object_instance)
{
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
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
bool Analog_Value_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list)
{
    bool status = false;
    bool in_alarm = false;
    bool out_of_service = false;
    bool fault = false;
    const bool overridden = false;
    float present_value = 0.0f;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
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
float Analog_Value_COV_Increment(uint32_t object_instance)
{
    float value = 0.0f;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
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
void Analog_Value_COV_Increment_Set(uint32_t object_instance, float value)
{
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
    if (pObject) {
        pObject->COV_Increment = value;
        Analog_Value_COV_Detect(pObject, pObject->Present_Value);
    }
}

/**
 * For a given object instance-number, returns the units property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  units property value
 */
uint16_t Analog_Value_Units(uint32_t object_instance)
{
    uint16_t units = UNITS_NO_UNITS;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
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
bool Analog_Value_Units_Set(uint32_t object_instance, uint16_t units)
{
    bool status = false;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
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
bool Analog_Value_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
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
void Analog_Value_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
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
static int Analog_Value_Event_Time_Stamps_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = 0, len = 0;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object(object_instance);
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
 * @param rpdata  Property requested, see for BACNET_READ_PROPERTY_DATA details.
 * @return apdu len, or BACNET_STATUS_ERROR on error
 */
int Analog_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    float real_value = (float)1.414;
    uint8_t *apdu = NULL;
    ANALOG_VALUE_DESCR *CurrentAV;
    bool state = false;
#if defined(INTRINSIC_REPORTING)
    int apdu_size = 0;
#endif

    /* Valid data? */
    if (rpdata == NULL) {
        return 0;
    }
    if ((rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    CurrentAV = Analog_Value_Object(rpdata->object_instance);
    if (!CurrentAV) {
        rpdata->error_class = ERROR_CLASS_OBJECT;
        rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return BACNET_STATUS_ERROR;
    }
    apdu = rpdata->application_data;
#if defined(INTRINSIC_REPORTING)
    apdu_size = rpdata->application_data_len;
#endif
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], Object_Type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            if (Analog_Value_Object_Name(
                    rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_PRESENT_VALUE:
            real_value = Analog_Value_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(
                &bit_string, STATUS_FLAG_IN_ALARM,
                (CurrentAV->Event_State != EVENT_STATE_NORMAL));
            state = Analog_Value_Fault(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, state);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Analog_Value_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], CurrentAV->Event_State);
            break;
        case PROP_RELIABILITY:
            apdu_len =
                encode_application_enumerated(&apdu[0], CurrentAV->Reliability);
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len =
                encode_application_boolean(&apdu[0], CurrentAV->Out_Of_Service);
            break;
        case PROP_UNITS:
            apdu_len =
                encode_application_enumerated(&apdu[0], CurrentAV->Units);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string,
                Analog_Value_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_COV_INCREMENT:
            apdu_len =
                encode_application_real(&apdu[0], CurrentAV->COV_Increment);
            break;
#if defined(INTRINSIC_REPORTING)
        case PROP_TIME_DELAY:
            apdu_len =
                encode_application_unsigned(&apdu[0], CurrentAV->Time_Delay);
            break;
        case PROP_NOTIFICATION_CLASS:
            apdu_len = encode_application_unsigned(
                &apdu[0], CurrentAV->Notification_Class);
            break;
        case PROP_HIGH_LIMIT:
            apdu_len = encode_application_real(&apdu[0], CurrentAV->High_Limit);
            break;
        case PROP_LOW_LIMIT:
            apdu_len = encode_application_real(&apdu[0], CurrentAV->Low_Limit);
            break;
        case PROP_DEADBAND:
            apdu_len = encode_application_real(&apdu[0], CurrentAV->Deadband);
            break;
        case PROP_LIMIT_ENABLE:
            bitstring_init(&bit_string);
            bitstring_set_bit(
                &bit_string, 0,
                (CurrentAV->Limit_Enable & EVENT_LOW_LIMIT_ENABLE) ? true
                                                                   : false);
            bitstring_set_bit(
                &bit_string, 1,
                (CurrentAV->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) ? true
                                                                    : false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_ENABLE:
            bitstring_init(&bit_string);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_OFFNORMAL,
                (CurrentAV->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ? true
                                                                      : false);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_FAULT,
                (CurrentAV->Event_Enable & EVENT_ENABLE_TO_FAULT) ? true
                                                                  : false);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_NORMAL,
                (CurrentAV->Event_Enable & EVENT_ENABLE_TO_NORMAL) ? true
                                                                   : false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_DETECTION_ENABLE:
            apdu_len = encode_application_boolean(
                &apdu[0], CurrentAV->Event_Detection_Enable);
            break;
        case PROP_ACKED_TRANSITIONS:
            bitstring_init(&bit_string);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_OFFNORMAL,
                CurrentAV->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_FAULT,
                CurrentAV->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_NORMAL,
                CurrentAV->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_NOTIFY_TYPE:
            apdu_len = encode_application_enumerated(
                &apdu[0], CurrentAV->Notify_Type ? NOTIFY_EVENT : NOTIFY_ALARM);
            break;
        case PROP_EVENT_TIME_STAMPS:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Analog_Value_Event_Time_Stamps_Encode,
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

    return apdu_len;
}

/**
 * @brief WriteProperty handler for this object.  For the given WriteProperty
 * data, the application_data is loaded or the error flags are set.
 * @param  wp_data - BACNET_WRITE_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 * @return false if an error is loaded, true if no errors
 */
bool Analog_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    float old_value = 0.0f;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    ANALOG_VALUE_DESCR *CurrentAV;

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
    CurrentAV = Analog_Value_Object(wp_data->object_instance);
    if (!CurrentAV) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */
                if (wp_data->priority == 6) {
                    /* Command priority 6 is reserved for use by Minimum On/Off
                       algorithm and may not be used for other purposes in any
                       object. */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                } else {
                    old_value =
                        Analog_Value_Present_Value(wp_data->object_instance);
                    if (Analog_Value_Present_Value_Set(
                            wp_data->object_instance, value.type.Real,
                            wp_data->priority)) {
                        status = true;
                        if (Analog_Value_Write_Present_Value_Callback) {
                            Analog_Value_Write_Present_Value_Callback(
                                wp_data->object_instance, old_value,
                                Analog_Value_Present_Value(
                                    wp_data->object_instance));
                        }
                    } else {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                CurrentAV->Out_Of_Service = value.type.Boolean;
            }
            break;
        case PROP_UNITS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated <= UINT16_MAX) {
                    CurrentAV->Units = value.type.Enumerated;
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_COV_INCREMENT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                if (value.type.Real >= 0.0f) {
                    Analog_Value_COV_Increment_Set(
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
                CurrentAV->Time_Delay = value.type.Unsigned_Int;
                CurrentAV->Remaining_Time_Delay = CurrentAV->Time_Delay;
            }
            break;
        case PROP_NOTIFICATION_CLASS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                CurrentAV->Notification_Class = value.type.Unsigned_Int;
            }
            break;
        case PROP_HIGH_LIMIT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                CurrentAV->High_Limit = value.type.Real;
            }
            break;
        case PROP_LOW_LIMIT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                CurrentAV->Low_Limit = value.type.Real;
            }
            break;
        case PROP_DEADBAND:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                CurrentAV->Deadband = value.type.Real;
            }
            break;
        case PROP_LIMIT_ENABLE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BIT_STRING);
            if (status) {
                if (value.type.Bit_String.bits_used == 2) {
                    CurrentAV->Limit_Enable = value.type.Bit_String.value[0];
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
                    CurrentAV->Event_Enable = value.type.Bit_String.value[0];
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
                        CurrentAV->Notify_Type = 1;
                        break;
                    case NOTIFY_ALARM:
                        CurrentAV->Notify_Type = 0;
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
                    Analog_Value_Properties_Required,
                    Analog_Value_Properties_Optional,
                    Analog_Value_Properties_Proprietary,
                    wp_data->object_property)) {
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
void Analog_Value_Write_Present_Value_Callback_Set(
    analog_value_write_present_value_callback cb)
{
    Analog_Value_Write_Present_Value_Callback = cb;
}

/**
 * @brief Analog Value intrinsic reporting function.
 * @param object_instance [in] BACnet object-instance number of the object
 */
void Analog_Value_Intrinsic_Reporting(uint32_t object_instance)
{
#if defined(INTRINSIC_REPORTING)
    BACNET_EVENT_NOTIFICATION_DATA event_data = { 0 };
    BACNET_CHARACTER_STRING msgText = { 0 };
    ANALOG_VALUE_DESCR *CurrentAV = NULL;
    uint8_t FromState = 0;
    uint8_t ToState = 0;
    float ExceededLimit = 0.0f;
    float PresentVal = 0.0f;
    bool SendNotify = false;

    CurrentAV = Analog_Value_Object(object_instance);
    if (!CurrentAV) {
        return;
    }

    /* check whether Intrinsic reporting is enabled */
    if (!CurrentAV->Event_Detection_Enable) {
        return; /* limits are not configured */
    }

    if (CurrentAV->Ack_notify_data.bSendAckNotify) {
        /* clean bSendAckNotify flag */
        CurrentAV->Ack_notify_data.bSendAckNotify = false;
        /* copy toState */
        ToState = CurrentAV->Ack_notify_data.EventState;
        debug_printf(
            "Send Acknotification for (%s,%u).\n",
            bactext_object_type_name(Object_Type), (unsigned)object_instance);
        characterstring_init_ansi(&msgText, "AckNotification");

        /* Notify Type */
        event_data.notifyType = NOTIFY_ACK_NOTIFICATION;

        /* Send EventNotification. */
        SendNotify = true;
    } else {
        /* actual Present_Value */
        PresentVal = Analog_Value_Present_Value(object_instance);
        FromState = CurrentAV->Event_State;
        switch (CurrentAV->Event_State) {
            case EVENT_STATE_NORMAL:
                /* A TO-OFFNORMAL event is generated under these conditions:
                   (a) the Present_Value must exceed the High_Limit for a
                   minimum period of time, specified in the Time_Delay property,
                   and (b) the HighLimitEnable flag must be set in the
                   Limit_Enable property, and
                   (c) the TO-OFFNORMAL flag must be set in the Event_Enable
                   property. */
                if ((PresentVal > CurrentAV->High_Limit) &&
                    ((CurrentAV->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) ==
                     EVENT_HIGH_LIMIT_ENABLE) &&
                    ((CurrentAV->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ==
                     EVENT_ENABLE_TO_OFFNORMAL)) {
                    if (!CurrentAV->Remaining_Time_Delay) {
                        CurrentAV->Event_State = EVENT_STATE_HIGH_LIMIT;
                    } else {
                        CurrentAV->Remaining_Time_Delay--;
                    }
                    break;
                }

                /* A TO-OFFNORMAL event is generated under these conditions:
                   (a) the Present_Value must exceed the Low_Limit plus the
                   Deadband for a minimum period of time, specified in the
                   Time_Delay property, and (b) the LowLimitEnable flag must be
                   set in the Limit_Enable property, and
                   (c) the TO-NORMAL flag must be set in the Event_Enable
                   property. */
                if ((PresentVal < CurrentAV->Low_Limit) &&
                    ((CurrentAV->Limit_Enable & EVENT_LOW_LIMIT_ENABLE) ==
                     EVENT_LOW_LIMIT_ENABLE) &&
                    ((CurrentAV->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ==
                     EVENT_ENABLE_TO_OFFNORMAL)) {
                    if (!CurrentAV->Remaining_Time_Delay) {
                        CurrentAV->Event_State = EVENT_STATE_LOW_LIMIT;
                    } else {
                        CurrentAV->Remaining_Time_Delay--;
                    }
                    break;
                }
                /* value of the object is still in the same event state */
                CurrentAV->Remaining_Time_Delay = CurrentAV->Time_Delay;
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
                if (((PresentVal <
                      CurrentAV->High_Limit - CurrentAV->Deadband) &&
                     ((CurrentAV->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) ==
                      EVENT_HIGH_LIMIT_ENABLE) &&
                     ((CurrentAV->Event_Enable & EVENT_ENABLE_TO_NORMAL) ==
                      EVENT_ENABLE_TO_NORMAL)) ||
                    /* 13.3.6 (c) If pCurrentState is HIGH_LIMIT, and the
                     * HighLimitEnable flag of pLimitEnable is FALSE, then
                     * indicate a transition to the NORMAL event state. */
                    (!(CurrentAV->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE))) {
                    if ((!CurrentAV->Remaining_Time_Delay) ||
                        (!(CurrentAV->Limit_Enable &
                           EVENT_HIGH_LIMIT_ENABLE))) {
                        CurrentAV->Event_State = EVENT_STATE_NORMAL;
                    } else {
                        CurrentAV->Remaining_Time_Delay--;
                    }
                    break;
                }
                /* value of the object is still in the same event state */
                CurrentAV->Remaining_Time_Delay = CurrentAV->Time_Delay;
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
                if (((PresentVal >
                      CurrentAV->Low_Limit + CurrentAV->Deadband) &&
                     ((CurrentAV->Limit_Enable & EVENT_LOW_LIMIT_ENABLE) ==
                      EVENT_LOW_LIMIT_ENABLE) &&
                     ((CurrentAV->Event_Enable & EVENT_ENABLE_TO_NORMAL) ==
                      EVENT_ENABLE_TO_NORMAL)) ||
                    /* 13.3.6 (f) If pCurrentState is LOW_LIMIT, and the
                     * LowLimitEnable flag of pLimitEnable is FALSE, then
                     * indicate a transition to the NORMAL event state. */
                    (!(CurrentAV->Limit_Enable & EVENT_LOW_LIMIT_ENABLE))) {
                    if ((!CurrentAV->Remaining_Time_Delay) ||
                        (!(CurrentAV->Limit_Enable & EVENT_LOW_LIMIT_ENABLE))) {
                        CurrentAV->Event_State = EVENT_STATE_NORMAL;
                    } else {
                        CurrentAV->Remaining_Time_Delay--;
                    }
                    break;
                }
                /* value of the object is still in the same event state */
                CurrentAV->Remaining_Time_Delay = CurrentAV->Time_Delay;
                break;

            default:
                return; /* shouldn't happen */
        } /* switch (FromState) */

        ToState = CurrentAV->Event_State;

        if (FromState != ToState) {
            /* Event_State has changed.
               Need to fill only the basic parameters of this type of event.
               Other parameters will be filled in common function. */

            switch (ToState) {
                case EVENT_STATE_HIGH_LIMIT:
                    ExceededLimit = CurrentAV->High_Limit;
                    characterstring_init_ansi(&msgText, "Goes to high limit");
                    break;

                case EVENT_STATE_LOW_LIMIT:
                    ExceededLimit = CurrentAV->Low_Limit;
                    characterstring_init_ansi(&msgText, "Goes to low limit");
                    break;

                case EVENT_STATE_NORMAL:
                    if (FromState == EVENT_STATE_HIGH_LIMIT) {
                        ExceededLimit = CurrentAV->High_Limit;
                        characterstring_init_ansi(
                            &msgText, "Back to normal state from high limit");
                    } else {
                        ExceededLimit = CurrentAV->Low_Limit;
                        characterstring_init_ansi(
                            &msgText, "Back to normal state from low limit");
                    }
                    break;

                default:
                    ExceededLimit = 0;
                    break;
            } /* switch (ToState) */
            debug_printf(
                "Event_State for (%s,%u) goes from %s to %s.\n",
                bactext_object_type_name(Object_Type),
                (unsigned)object_instance, bactext_event_state_name(FromState),
                bactext_event_state_name(ToState));
            /* Notify Type */
            event_data.notifyType = CurrentAV->Notify_Type;

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
                    CurrentAV->Event_Time_Stamps[TRANSITION_TO_OFFNORMAL] =
                        event_data.timeStamp.value.dateTime;
                    break;

                case EVENT_STATE_FAULT:
                    CurrentAV->Event_Time_Stamps[TRANSITION_TO_FAULT] =
                        event_data.timeStamp.value.dateTime;
                    break;

                case EVENT_STATE_NORMAL:
                    CurrentAV->Event_Time_Stamps[TRANSITION_TO_NORMAL] =
                        event_data.timeStamp.value.dateTime;
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
                        &CurrentAV->Event_Time_Stamps[TRANSITION_TO_OFFNORMAL]);
                    break;
                case EVENT_STATE_FAULT:
                    datetime_copy(
                        &event_data.timeStamp.value.dateTime,
                        &CurrentAV->Event_Time_Stamps[TRANSITION_TO_FAULT]);
                    break;
                case EVENT_STATE_NORMAL:
                    datetime_copy(
                        &event_data.timeStamp.value.dateTime,
                        &CurrentAV->Event_Time_Stamps[TRANSITION_TO_NORMAL]);
                    break;
                default:
                    break;
            }
        }
        /* Notification Class */
        event_data.notificationClass = CurrentAV->Notification_Class;

        /* Event Type */
        event_data.eventType = EVENT_OUT_OF_RANGE;

        /* Message Text */
        event_data.messageText = &msgText;

        /* Notify Type */
        /* filled before */

        /* From State */
        if (event_data.notifyType != NOTIFY_ACK_NOTIFICATION) {
            event_data.fromState = FromState;
        }

        /* To State */
        event_data.toState = CurrentAV->Event_State;

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
                CurrentAV->Event_State != EVENT_STATE_NORMAL);
            bitstring_set_bit(
                &event_data.notificationParams.outOfRange.statusFlags,
                STATUS_FLAG_FAULT, false);
            bitstring_set_bit(
                &event_data.notificationParams.outOfRange.statusFlags,
                STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(
                &event_data.notificationParams.outOfRange.statusFlags,
                STATUS_FLAG_OUT_OF_SERVICE, CurrentAV->Out_Of_Service);
            /* Deadband used for limit checking. */
            event_data.notificationParams.outOfRange.deadband =
                CurrentAV->Deadband;
            /* Limit that was exceeded. */
            event_data.notificationParams.outOfRange.exceededLimit =
                ExceededLimit;
        }

        /* add data from notification class */
        debug_printf(
            "Analog-Value[%d]: Notification Class[%d]-%s "
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
            debug_printf("Analog-Value[%d]: Ack Required!\n", object_instance);
            switch (event_data.toState) {
                case EVENT_STATE_OFFNORMAL:
                case EVENT_STATE_HIGH_LIMIT:
                case EVENT_STATE_LOW_LIMIT:
                    CurrentAV->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                        .bIsAcked = false;
                    CurrentAV->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;
                case EVENT_STATE_FAULT:
                    CurrentAV->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked =
                        false;
                    CurrentAV->Acked_Transitions[TRANSITION_TO_FAULT]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;
                case EVENT_STATE_NORMAL:
                    CurrentAV->Acked_Transitions[TRANSITION_TO_NORMAL]
                        .bIsAcked = false;
                    CurrentAV->Acked_Transitions[TRANSITION_TO_NORMAL]
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
 * @brief Handles getting the Event Information for this object.
 * @param  index - index number of the object 0..count
 * @param  getevent_data - data for the Event Information
 * @return 1 if an active event is found, 0 if no active event, -1 if
 * end of list
 */
int Analog_Value_Event_Information(
    unsigned index, BACNET_GET_EVENT_INFORMATION_DATA *getevent_data)
{
    bool IsNotAckedTransitions;
    bool IsActiveEvent;
    int i;
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object_Index(index);
    if (pObject) {
        /* Event_State not equal to NORMAL */
        IsActiveEvent = (pObject->Event_State != EVENT_STATE_NORMAL);
        /* Acked_Transitions property, which has at least one of the bits
           (TO-OFFNORMAL, TO-FAULT, TONORMAL) set to FALSE. */
        IsNotAckedTransitions =
            (pObject->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked ==
             false) ||
            (pObject->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked ==
             false) ||
            (pObject->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked ==
             false);
    } else {
        return -1; /* end of list  */
    }
    if ((IsActiveEvent) || (IsNotAckedTransitions)) {
        /* Object Identifier */
        getevent_data->objectIdentifier.type = Object_Type;
        getevent_data->objectIdentifier.instance =
            Analog_Value_Index_To_Instance(index);
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
    } else {
        return 0; /* no active event at this index */
    }
}

/**
 * @brief Acknowledges the Event Information for this object.
 * @param alarmack_data - data for the Event Acknowledgement
 * @param error_code - error code for the Event Acknowledgement
 * @return 1 if successful, -1 if error, -2 if request is out-of-range
 */
int Analog_Value_Alarm_Ack(
    BACNET_ALARM_ACK_DATA *alarmack_data, BACNET_ERROR_CODE *error_code)
{
    ANALOG_VALUE_DESCR *CurrentAV;

    if (!alarmack_data) {
        return -1;
    }
    CurrentAV =
        Analog_Value_Object(alarmack_data->eventObjectIdentifier.instance);
    if (!CurrentAV) {
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return -1;
    }
    switch (alarmack_data->eventStateAcked) {
        case EVENT_STATE_OFFNORMAL:
        case EVENT_STATE_HIGH_LIMIT:
        case EVENT_STATE_LOW_LIMIT:
            if (CurrentAV->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                    .bIsAcked == false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &CurrentAV->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                /* Clean transitions flag. */
                CurrentAV->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked =
                    true;
            } else if (
                alarmack_data->eventStateAcked == CurrentAV->Event_State) {
                /* Send ack notification */
            } else {
                *error_code = ERROR_CODE_INVALID_EVENT_STATE;
                return -1;
            }
            break;

        case EVENT_STATE_FAULT:
            if (CurrentAV->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked ==
                false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &CurrentAV->Acked_Transitions[TRANSITION_TO_FAULT]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                /* Send ack notification */
                CurrentAV->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked =
                    true;
            } else if (
                alarmack_data->eventStateAcked == CurrentAV->Event_State) {
                /* Send ack notification */
            } else {
                *error_code = ERROR_CODE_INVALID_EVENT_STATE;
                return -1;
            }
            break;

        case EVENT_STATE_NORMAL:
            if (CurrentAV->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked ==
                false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &CurrentAV->Acked_Transitions[TRANSITION_TO_NORMAL]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                /* Send ack notification */
                CurrentAV->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked =
                    true;
            } else if (
                alarmack_data->eventStateAcked == CurrentAV->Event_State) {
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
    CurrentAV->Ack_notify_data.bSendAckNotify = true;
    CurrentAV->Ack_notify_data.EventState = alarmack_data->eventStateAcked;

    /* Return OK */
    return 1;
}

/**
 * @brief Handles getting the Alarm Summary for this object.
 * @param  index - index number of the object 0..count
 * @param  getalarm_data - data for the Alarm Summary
 * @return 1 if an active alarm is found, 0 if no active alarm, -1 if
 * end of list
 */
int Analog_Value_Alarm_Summary(
    unsigned index, BACNET_GET_ALARM_SUMMARY_DATA *getalarm_data)
{
    struct analog_value_descr *pObject;

    pObject = Analog_Value_Object_Index(index);
    if (pObject) {
        /* Event_State is not equal to NORMAL  and
           Notify_Type property value is ALARM */
        if ((pObject->Event_State != EVENT_STATE_NORMAL) &&
            (pObject->Notify_Type == NOTIFY_ALARM)) {
            /* Object Identifier */
            getalarm_data->objectIdentifier.type = Object_Type;
            getalarm_data->objectIdentifier.instance =
                Analog_Value_Index_To_Instance(index);
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
 * @brief Set the context used with a specific object instance
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void *Analog_Value_Context_Get(uint32_t object_instance)
{
    struct analog_value_descr *pObject;

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
void Analog_Value_Context_Set(uint32_t object_instance, void *context)
{
    struct analog_value_descr *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Context = context;
    }
}

/**
 * @brief Creates a Analog Value object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Analog_Value_Create(uint32_t object_instance)
{
    struct analog_value_descr *pObject = NULL;
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
        pObject = calloc(1, sizeof(struct analog_value_descr));
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
            pObject->Event_Detection_Enable = true;
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
 * @brief Deletes an Analog Value object
 * @param object_instance - object-instance number of the object
 * @return true if the object-instance was deleted
 */
bool Analog_Value_Delete(uint32_t object_instance)
{
    bool status = false;
    struct analog_value_descr *pObject = NULL;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        free(pObject);
        status = true;
    }

    return status;
}

/**
 * @brief Deletes all the Analog Values and their data
 */
void Analog_Value_Cleanup(void)
{
    struct analog_value_descr *pObject;

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
 * @brief Initializes the Analog Value object data
 */
void Analog_Value_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
#if defined(INTRINSIC_REPORTING)
    /* Set handler for GetEventInformation function */
    handler_get_event_information_set(
        Object_Type, Analog_Value_Event_Information);
    /* Set handler for AcknowledgeAlarm function */
    handler_alarm_ack_set(Object_Type, Analog_Value_Alarm_Ack);
    /* Set handler for GetAlarmSummary Service */
    handler_get_alarm_summary_set(Object_Type, Analog_Value_Alarm_Summary);
#endif
}
