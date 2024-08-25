/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @brief A basic BACnet Binary Value object implementation.
 * Binary Value objects are I/O objects with a present-value that
 * uses an enumerated two state active/inactive data type.
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
#include "bacnet/bacenum.h"
#include "bacnet/bactext.h"
#include "bacnet/bacapp.h"
#include "bacnet/wp.h"
#include "bacnet/rp.h"
#include "bacnet/cov.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "bacnet/basic/object/bv.h"

#include "bacnet/basic/sys/debug.h"
#if !defined(PRINT)
#define PRINT debug_perror
#endif

static const char *Default_Active_Text = "Active";
static const char *Default_Inactive_Text = "Inactive";
struct object_data {
    bool Out_Of_Service : 1;
    bool Change_Of_Value : 1;
    bool Present_Value : 1;
    bool Write_Enabled : 1;
    bool Polarity : 1;
    unsigned Event_State : 3;
    uint8_t Reliability;
    const char *Object_Name;
    const char *Active_Text;
    const char *Inactive_Text;
    const char *Description;
#if defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING)
    uint32_t Time_Delay;
    uint32_t Notification_Class;
    unsigned Event_Enable : 3;
    unsigned Event_Detection_Enable : 1;
    unsigned Notify_Type : 1;
    ACKED_INFO Acked_Transitions[MAX_BACNET_EVENT_TRANSITION];
    BACNET_DATE_TIME Event_Time_Stamps[MAX_BACNET_EVENT_TRANSITION];
    /* time to generate event notification */
    uint32_t Remaining_Time_Delay;
    /* AckNotification information */
    ACK_NOTIFICATION Ack_notify_data;
    BACNET_BINARY_PV Alarm_Value;
#endif
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_BINARY_VALUE;
/* callback for present value writes */
static binary_value_write_present_value_callback
    Binary_Value_Write_Present_Value_Callback;

/* clang-format off */
/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Binary_Value_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_PRESENT_VALUE,
    PROP_STATUS_FLAGS,
    PROP_EVENT_STATE,
    PROP_OUT_OF_SERVICE,
    -1
};

static const int Binary_Value_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_RELIABILITY,
    PROP_ACTIVE_TEXT,
    PROP_INACTIVE_TEXT,
#if defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING)
    PROP_TIME_DELAY,
    PROP_NOTIFICATION_CLASS,
    PROP_ALARM_VALUE,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_DETECTION_ENABLE,
#endif
    -1
};

static const int Binary_Value_Properties_Proprietary[] = {
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
void Binary_Value_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Binary_Value_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Binary_Value_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Binary_Value_Properties_Proprietary;
    }

    return;
}

/**
 * @brief Gets an object from the list using an instance number as the key
 * @param  object_instance - object-instance number of the object
 * @return object found in the list, or NULL if not found
 */
static struct object_data *Binary_Value_Object(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * @brief Determines if a given object instance is valid
 * @param  object_instance - object-instance number of the object
 * @return  true if the instance is valid, and false if not
 */
bool Binary_Value_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * @brief Determines the number of objects
 * @return  Number of objects
 */
unsigned Binary_Value_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Determines the object instance-number for a given 0..N index
 * of objects where N is the count.
 * @param  index - 0..N value
 * @return  object instance-number for a valid given index, or UINT32_MAX
 */
uint32_t Binary_Value_Index_To_Instance(unsigned index)
{
    uint32_t instance = UINT32_MAX;

    (void)Keylist_Index_Key(Object_List, index, &instance);

    return instance;
}

/**
 * @brief For a given object instance-number, determines a 0..N index
 * of objects where N is the count.
 * @param  object_instance - object-instance number of the object
 * @return  index for the given instance-number, or count if not valid.
 */
unsigned Binary_Value_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * @brief Convert from boolean to BACNET_BINARY_PV enumeration
 * @param  value - boolean value
 * @return  BACNET_BINARY_PV enumeration
 */
static BACNET_BINARY_PV Binary_Present_Value(bool value)
{
    BACNET_BINARY_PV binary_value = BINARY_INACTIVE;

    if (value) {
        binary_value = BINARY_ACTIVE;
    }

    return binary_value;
}

/**
 * @brief Convert from BACNET_BINARY_PV enumeration to boolean
 * @param binary_value BACNET_BINARY_PV enumeration
 * @return boolean value
 */
static bool Binary_Present_Value_Boolean(BACNET_BINARY_PV binary_value)
{
    bool boolean_value = false;

    if (binary_value == BINARY_ACTIVE) {
        boolean_value = true;
    }

    return boolean_value;
}

/**
 * @brief Convert from boolean to BACNET_POLARITY enumeration
 * @param  value - boolean value
 * @return  BACNET_POLARITY enumeration
 */
static BACNET_POLARITY Binary_Polarity(bool value)
{
    BACNET_POLARITY polarity = POLARITY_NORMAL;

    if (value) {
        polarity = POLARITY_REVERSE;
    }

    return polarity;
}

/**
 * @brief Convert from BACNET_POLARITY enumeration to boolean
 * @param binary_value BACNET_POLARITY enumeration
 * @return boolean value
 */
static bool Binary_Polarity_Boolean(BACNET_POLARITY polarity)
{
    bool boolean_value = false;

    if (polarity == POLARITY_REVERSE) {
        boolean_value = true;
    }

    return boolean_value;
}

/**
 * For a given object instance-number, return the present value.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  Present value
 */
BACNET_BINARY_PV Binary_Value_Present_Value(uint32_t object_instance)
{
    BACNET_BINARY_PV value = BINARY_INACTIVE;
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        value = Binary_Present_Value(pObject->Present_Value);
        if (Binary_Polarity(pObject->Polarity) != POLARITY_NORMAL) {
            if (value == BINARY_INACTIVE) {
                value = BINARY_ACTIVE;
            } else {
                value = BINARY_INACTIVE;
            }
        }
    }

    return value;
}

/**
 * @brief For a given object instance-number, checks the present-value for COV
 * @param  pObject - specific object with valid data
 * @param  value - floating point analog value
 */
static void Binary_Value_Present_Value_COV_Detect(
    struct object_data *pObject, BACNET_BINARY_PV value)
{
    if (pObject) {
        if (Binary_Present_Value(pObject->Present_Value) != value) {
            pObject->Change_Of_Value = true;
        }
    }
}

/**
 * @brief For a given object instance-number, returns the out-of-service
 * property value
 * @param object_instance - object-instance number of the object
 * @return out-of-service property value
 */
bool Binary_Value_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        value = pObject->Out_Of_Service;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the out-of-service
 *  property value
 * @param object_instance - object-instance number of the object
 * @param value - boolean out-of-service value
 * @return true if the out-of-service property value was set
 */
void Binary_Value_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        if (pObject->Out_Of_Service != value) {
            pObject->Out_Of_Service = value;
            pObject->Change_Of_Value = true;
        }
    }

    return;
}

/**
 * @brief For a given object instance-number, returns the reliability property
 * value
 * @param object_instance - object-instance number of the object
 * @return reliability property value
 */
BACNET_RELIABILITY Binary_Value_Reliability(uint32_t object_instance)
{
    BACNET_RELIABILITY value = RELIABILITY_NO_FAULT_DETECTED;
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
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
static bool Binary_Value_Object_Fault(struct object_data *pObject)
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
 * For a given object instance-number, sets the reliability
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - reliability enumerated value
 *
 * @return  true if values are within range and property is set.
 */
bool Binary_Value_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value)
{
    struct object_data *pObject;
    bool status = false;
    bool fault = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value <= 255) {
            fault = Binary_Value_Object_Fault(pObject);
            pObject->Reliability = value;
            if (fault != Binary_Value_Object_Fault(pObject)) {
                pObject->Change_Of_Value = true;
            }
            status = true;
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, gets the Fault status flag
 * @param  object_instance - object-instance number of the object
 * @return  true the status flag is in Fault
 */
static bool Binary_Value_Fault(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);

    return Binary_Value_Object_Fault(pObject);
}

/**
 * @brief For a given object instance-number, determines if the COV flag
 *  has been triggered.
 * @param  object_instance - object-instance number of the object
 * @return  true if the COV flag is set
 */
bool Binary_Value_Change_Of_Value(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        status = pObject->Change_Of_Value;
    }

    return status;
}

/**
 * @brief For a given object instance-number, clears the COV flag
 * @param  object_instance - object-instance number of the object
 */
void Binary_Value_Change_Of_Value_Clear(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        pObject->Change_Of_Value = false;
    }

    return;
}

/**
 * @brief For a given object instance-number, loads the value_list with the COV
 * data.
 * @param  object_instance - object-instance number of the object
 * @param  value_list - list of COV data
 * @return  true if the value list is encoded
 */
bool Binary_Value_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list)
{
    bool status = false;
    const bool in_alarm = false;
    bool out_of_service = false;
    bool fault = false;
    const bool overridden = false;
    BACNET_BINARY_PV present_value = BINARY_INACTIVE;
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        if (pObject->Reliability != RELIABILITY_NO_FAULT_DETECTED) {
            fault = true;
        }
        out_of_service = pObject->Out_Of_Service;
        if (pObject->Present_Value) {
            present_value = BINARY_ACTIVE;
        }
        status = cov_value_list_encode_enumerated(
            value_list, present_value, in_alarm, fault, overridden,
            out_of_service);
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the present-value
 * @param  object_instance - object-instance number of the object
 * @param  value - enumerated binary present-value
 * @return  true if values are within range and present-value is set.
 */
bool Binary_Value_Present_Value_Set(
    uint32_t object_instance, BACNET_BINARY_PV value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        if (value <= MAX_BINARY_PV) {
            /* de-polarize */
            if (Binary_Polarity(pObject->Polarity) != POLARITY_NORMAL) {
                if (value == BINARY_INACTIVE) {
                    value = BINARY_ACTIVE;
                } else {
                    value = BINARY_INACTIVE;
                }
            }
            Binary_Value_Present_Value_COV_Detect(pObject, value);
            pObject->Present_Value = Binary_Present_Value_Boolean(value);
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - floating point analog value
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Binary_Value_Present_Value_Write(
    uint32_t object_instance,
    BACNET_BINARY_PV value,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;
    BACNET_BINARY_PV old_value = BINARY_INACTIVE;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        if (value <= MAX_BINARY_PV) {
            if (pObject->Write_Enabled) {
                old_value = Binary_Present_Value(pObject->Present_Value);
                Binary_Value_Present_Value_COV_Detect(pObject, value);
                pObject->Present_Value = Binary_Present_Value_Boolean(value);
                if (pObject->Out_Of_Service) {
                    /* The physical point that the object represents
                        is not in service. This means that changes to the
                        Present_Value property are decoupled from the
                        physical point when the value of Out_Of_Service
                        is true. */
                } else if (Binary_Value_Write_Present_Value_Callback) {
                    Binary_Value_Write_Present_Value_Callback(
                        object_instance, old_value, value);
                }
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        }
    } else {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/**
 * @brief Get the object name
 * @param  object_instance - object-instance number of the object
 * @param  object_name - holds the object-name to be retrieved
 * @return  true if object-name was retrieved
 */
bool Binary_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    static char text[32] = ""; /* okay for single thread */
    bool status = false;
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        if (pObject->Object_Name == NULL) {
            snprintf(
                text, sizeof(text), "BINARY INPUT %lu",
                (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, text);
        } else {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the object-name
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be set
 * @return  true if object-name was set
 */
bool Binary_Value_Name_Set(uint32_t object_instance, char *new_name)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        if (new_name) {
            status = true;
            pObject->Object_Name = new_name;
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the polarity property.
 * @param  object_instance - object-instance number of the object
 * @return  the polarity property of the object.
 */
BACNET_POLARITY Binary_Value_Polarity(uint32_t object_instance)
{
    BACNET_POLARITY polarity = POLARITY_NORMAL;
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        polarity = Binary_Polarity(pObject->Polarity);
    }

    return polarity;
}

/**
 * @brief For a given object instance-number, sets the polarity property
 * @param  object_instance - object-instance number of the object
 * @param  polarity - polarity property value
 * @return  true if polarity was set
 */
bool Binary_Value_Polarity_Set(
    uint32_t object_instance, BACNET_POLARITY polarity)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        pObject->Polarity = Binary_Polarity_Boolean(polarity);
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the description
 * @param  object_instance - object-instance number of the object
 * @return description text or NULL if not found
 */
char *Binary_Value_Description(uint32_t object_instance)
{
    char *name = NULL;
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        if (pObject->Description == NULL) {
            name = "";
        } else {
            name = (char *)pObject->Description;
        }
    }

    return name;
}

/**
 * @brief For a given object instance-number, sets the description
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 * @return  true if object-name was set
 */
bool Binary_Value_Description_Set(uint32_t object_instance, char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        status = true;
        pObject->Description = new_name;
    }

    return status;
}

/**
 * For a given object instance-number, returns the active text value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return active text or NULL if not found
 */
char *Binary_Value_Active_Text(uint32_t object_instance)
{
    char *name = NULL;
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        name = (char *)pObject->Active_Text;
    }

    return name;
}

/**
 * For a given object instance-number, sets the description
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 *
 * @return  true if object-name was set
 */
bool Binary_Value_Active_Text_Set(uint32_t object_instance, char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        status = true;
        pObject->Active_Text = new_name;
    }

    return status;
}

/**
 * For a given object instance-number, returns the active text value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return active text or NULL if not found
 */
char *Binary_Value_Inactive_Text(uint32_t object_instance)
{
    char *name = NULL;
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        name = (char *)pObject->Inactive_Text;
    }

    return name;
}

/**
 * For a given object instance-number, sets the description
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 *
 * @return  true if object-name was set
 */
bool Binary_Value_Inactive_Text_Set(uint32_t object_instance, char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        status = true;
        pObject->Inactive_Text = new_name;
    }

    return status;
}

#if defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING)
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
static int Binary_Value_Event_Time_Stamps_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = 0, len = 0;
    struct object_data *pObject = Binary_Value_Object(object_instance);

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
 * ReadProperty handler for this object.  For the given ReadProperty
 * data, the application_data is loaded or the error flags are set.
 *
 * @param  rpdata - BACNET_READ_PROPERTY_DATA data, including
 * requested data and space for the reply, or error response.
 *
 * @return number of APDU bytes in the response, or
 * BACNET_STATUS_ERROR on error.
 */
int Binary_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;
    bool state = false;
    struct object_data *pObject;
#if defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING)
    int apdu_size = 0;
#endif

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    if (!(pObject = Binary_Value_Object(rpdata->object_instance))) {
#if !(defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING))
        (void)pObject;
#endif
        return BACNET_STATUS_ERROR;
    }
    apdu = rpdata->application_data;
#if defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING)
    apdu_size = rpdata->application_data_len;
#endif
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], Object_Type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            /* note: object name must be unique in our device */
            Binary_Value_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_enumerated(
                &apdu[0], Binary_Value_Present_Value(rpdata->object_instance));
            break;
        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            state = Binary_Value_Fault(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, state);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Binary_Value_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Binary_Value_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_POLARITY:
            apdu_len = encode_application_enumerated(
                &apdu[0], Binary_Value_Polarity(rpdata->object_instance));
            break;
        case PROP_RELIABILITY:
            apdu_len = encode_application_enumerated(
                &apdu[0], Binary_Value_Reliability(rpdata->object_instance));
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string,
                Binary_Value_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_ACTIVE_TEXT:
            characterstring_init_ansi(
                &char_string,
                Binary_Value_Active_Text(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_INACTIVE_TEXT:
            characterstring_init_ansi(
                &char_string,
                Binary_Value_Inactive_Text(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
#if defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING)
        case PROP_ALARM_VALUE:
            /* note: you need to look up the actual value */
            apdu_len =
                encode_application_enumerated(&apdu[0], pObject->Alarm_Value);
            break;
        case PROP_TIME_DELAY:
            apdu_len =
                encode_application_unsigned(&apdu[0], pObject->Time_Delay);
            break;

        case PROP_NOTIFICATION_CLASS:
            apdu_len = encode_application_unsigned(
                &apdu[0], pObject->Notification_Class);
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

        case PROP_EVENT_DETECTION_ENABLE:
            apdu_len = encode_application_boolean(
                &apdu[0], pObject->Event_Detection_Enable);
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
                Binary_Value_Event_Time_Stamps_Encode,
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
    /* Only array properties can have array options. */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_PRIORITY_ARRAY) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * Set the requested property of the binary value.
 *
 * @param wp_data  Property requested, see for BACNET_WRITE_PROPERTY_DATA
 * details.
 *
 * @return true if successful
 */
bool Binary_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    struct object_data *pObject;

    /* Valid data? */
    if (wp_data == NULL) {
        return false;
    }
    if (wp_data->application_data_len == 0) {
        return false;
    }

    /* Decode the some of the request. */
    len = bacapp_decode_application_data(
        wp_data->application_data, wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    if (!(pObject = Binary_Value_Object(wp_data->object_instance))) {
#if (!BINARY_VALUE_INTRINSIC_REPORTING)
        (void)pObject;
#endif
        return BACNET_STATUS_ERROR;
    }
    /* Only array properties can have array options. */
    if ((wp_data->object_property != PROP_PRIORITY_ARRAY) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                status = Binary_Value_Present_Value_Write(
                    wp_data->object_instance, value.type.Enumerated,
                    &wp_data->error_class, &wp_data->error_code);
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Binary_Value_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_POLARITY:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated < MAX_POLARITY) {
                    Binary_Value_Polarity_Set(
                        wp_data->object_instance,
                        (BACNET_POLARITY)value.type.Enumerated);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
#if defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING)
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

        case PROP_ALARM_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated <= MAX_BINARY_PV) {
                    Binary_Value_Alarm_Value_Set(
                        wp_data->object_instance,
                        (BACNET_BINARY_PV)value.type.Enumerated);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
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
                    Binary_Value_Properties_Required,
                    Binary_Value_Properties_Optional,
                    Binary_Value_Properties_Proprietary,
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
void Binary_Value_Write_Present_Value_Callback_Set(
    binary_value_write_present_value_callback cb)
{
    Binary_Value_Write_Present_Value_Callback = cb;
}

/**
 * @brief Determines a object write-enabled flag state
 * @param object_instance - object-instance number of the object
 * @return  write-enabled status flag
 */
bool Binary_Value_Write_Enabled(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        value = pObject->Write_Enabled;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the write-enabled flag
 * @param object_instance - object-instance number of the object
 */
void Binary_Value_Write_Enable(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        pObject->Write_Enabled = true;
    }
}

/**
 * @brief For a given object instance-number, clears the write-enabled flag
 * @param object_instance - object-instance number of the object
 */
void Binary_Value_Write_Disable(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Binary_Value_Object(object_instance);
    if (pObject) {
        pObject->Write_Enabled = false;
    }
}

/**
 * @brief Creates a Binary Output object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Binary_Value_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    int index = 0;

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
#if defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING)
            unsigned j;
#endif
            pObject->Object_Name = NULL;
            pObject->Description = NULL;
            pObject->Reliability = RELIABILITY_NO_FAULT_DETECTED;
            pObject->Present_Value = false;
            pObject->Out_Of_Service = false;
            pObject->Active_Text = Default_Active_Text;
            pObject->Inactive_Text = Default_Inactive_Text;
            pObject->Change_Of_Value = false;
            pObject->Write_Enabled = false;
            pObject->Polarity = false;
#if defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING)
            pObject->Event_State = EVENT_STATE_NORMAL;
            pObject->Event_Detection_Enable = true;
            /* notification class not connected */
            pObject->Notification_Class = BACNET_MAX_INSTANCE;
            /* initialize Event time stamps using wildcards and set
             * Acked_transitions */
            for (j = 0; j < MAX_BACNET_EVENT_TRANSITION; j++) {
                datetime_wildcard_set(&pObject->Event_Time_Stamps[j]);
                pObject->Acked_Transitions[j].bIsAcked = true;
            }

            /* Set handler for GetEventInformation function */
            handler_get_event_information_set(
                Object_Type, Binary_Value_Event_Information);
            /* Set handler for AcknowledgeAlarm function */
            handler_alarm_ack_set(Object_Type, Binary_Value_Alarm_Ack);
            /* Set handler for GetAlarmSummary Service */
            handler_get_alarm_summary_set(
                Object_Type, Binary_Value_Alarm_Summary);
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
 * Initializes the Binary Input object data
 */
void Binary_Value_Cleanup(void)
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
 * Creates a Binary Input object
 */
bool Binary_Value_Delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data_Delete(Object_List, object_instance);
    if (pObject) {
        free(pObject);
        status = true;
    }

    return status;
}

/**
 * Initializes the Binary Input object data
 */
void Binary_Value_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}

/**
 * For a given object instance-number, gets the event-state property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  event-state property value
 */
unsigned Binary_Value_Event_State(uint32_t object_instance)
{
    unsigned state = EVENT_STATE_NORMAL;
#if !defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING)
    (void)object_instance;
#else
    struct object_data *pObject = Binary_Value_Object(object_instance);

    if (pObject) {
        state = pObject->Event_State;
    }
#endif

    return state;
}

#if defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING)
/**
 * For a given object instance-number, gets the event-detection-enable property
 * value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  event-detection-enable property value
 */
bool Binary_Value_Event_Detection_Enable(uint32_t object_instance)
{
    bool retval = false;
#if !(defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING))
    (void)object_instance;
#else
    struct object_data *pObject = Binary_Value_Object(object_instance);

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
bool Binary_Value_Event_Detection_Enable_Set(
    uint32_t object_instance, bool value)
{
    bool retval = false;
#if !(defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING))
    (void)object_instance;
    (void)value;
#else
    struct object_data *pObject = Binary_Value_Object(object_instance);

    if (pObject) {
        pObject->Event_Detection_Enable = value;
        retval = true;
    }
#endif

    return retval;
}

#if defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING)
/**
 * @brief Gets an object from the list using its index in the list
 * @param index - index of the object in the list
 * @return object found in the list, or NULL if not found
 */
static struct object_data *Binary_Value_Object_Index(int index)
{
    return Keylist_Data_Index(Object_List, index);
}

/**
 * For a given object instance-number, returns the event_enable property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  event_enable property value
 */
uint32_t Binary_Value_Event_Enable(uint32_t object_instance)
{
    uint32_t event_enable = 0;
    struct object_data *pObject = Binary_Value_Object(object_instance);

    if (pObject) {
        event_enable = pObject->Event_Enable;
    }

    return event_enable;
}

/**
 * For a given object instance-number, sets the event_enable property value
 *
 * @param object_instance - object-instance number of the object
 * @param event_enable - event_enable property value - the combination of bits:
 *                       EVENT_ENABLE_TO_OFFNORMAL, EVENT_ENABLE_TO_FAULT,
 * EVENT_ENABLE_TO_NORMAL
 * @return true if the event_enable property value was set
 */
bool Binary_Value_Event_Enable_Set(
    uint32_t object_instance, uint32_t event_enable)
{
    bool status = false;
    struct object_data *pObject = Binary_Value_Object(object_instance);

    if (pObject) {
        if (!(event_enable &
              ~(EVENT_ENABLE_TO_OFFNORMAL | EVENT_ENABLE_TO_FAULT |
                EVENT_ENABLE_TO_NORMAL))) {
            pObject->Event_Enable = event_enable;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, returns the notify_type property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  notify_type property value
 */
BACNET_NOTIFY_TYPE Binary_Value_Notify_Type(uint32_t object_instance)
{
    BACNET_NOTIFY_TYPE notify_type = NOTIFY_EVENT;
    struct object_data *pObject = Binary_Value_Object(object_instance);

    if (pObject) {
        notify_type = pObject->Notify_Type;
    }

    return notify_type;
}

/**
 * For a given object instance-number, sets the notify_type property value
 *
 * @param object_instance - object-instance number of the object
 * @param notify_type - notify_type property value from the set <NOTIFY_EVENT,
 * NOTIFY_ALARM>
 *
 * @return true if the notify_type property value was set
 */
bool Binary_Value_Notify_Type_Set(
    uint32_t object_instance, BACNET_NOTIFY_TYPE notify_type)
{
    bool status = false;
    struct object_data *pObject = Binary_Value_Object(object_instance);

    if (pObject) {
        if ((notify_type == NOTIFY_EVENT) || (notify_type == NOTIFY_ALARM)) {
            pObject->Notify_Type = notify_type;
            status = true;
        }
    }

    return status;
}
#endif

int Binary_Value_Event_Information(
    unsigned index, BACNET_GET_EVENT_INFORMATION_DATA *getevent_data)
{
    struct object_data *pObject = Binary_Value_Object_Index(index);

    bool IsNotAckedTransitions;
    bool IsActiveEvent;
    int i;

    /* check index */
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
    } else
        return -1; /* end of list  */

    if ((IsActiveEvent) || (IsNotAckedTransitions)) {
        /* Object Identifier */
        getevent_data->objectIdentifier.type = Object_Type;
        getevent_data->objectIdentifier.instance =
            Binary_Value_Index_To_Instance(index);
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

int Binary_Value_Alarm_Ack(
    BACNET_ALARM_ACK_DATA *alarmack_data, BACNET_ERROR_CODE *error_code)
{
    struct object_data *pObject = NULL;

    if (!alarmack_data) {
        return -1;
    }
    pObject =
        Binary_Value_Object(alarmack_data->eventObjectIdentifier.instance);

    if (!pObject) {
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return -1;
    }

    switch (alarmack_data->eventStateAcked) {
        case EVENT_STATE_OFFNORMAL:
            if (pObject->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked ==
                false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &pObject->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                /* Send ack notification */
                pObject->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked =
                    true;
            } else if (alarmack_data->eventStateAcked == pObject->Event_State) {
                /* Send ack notification */
            } else {
                *error_code = ERROR_CODE_INVALID_EVENT_STATE;
                return -1;
            }
            break;

        case EVENT_STATE_FAULT:
            if (pObject->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked ==
                false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &pObject->Acked_Transitions[TRANSITION_TO_FAULT]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                /* Send ack notification */
                pObject->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked = true;
            } else if (alarmack_data->eventStateAcked == pObject->Event_State) {
                /* Send ack notification */
            } else {
                *error_code = ERROR_CODE_INVALID_EVENT_STATE;
                return -1;
            }
            break;

        case EVENT_STATE_NORMAL:
            if (pObject->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked ==
                false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &pObject->Acked_Transitions[TRANSITION_TO_NORMAL]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                /* Send ack notification */
                pObject->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked =
                    true;
            } else if (alarmack_data->eventStateAcked == pObject->Event_State) {
                /* Send ack notification */
            } else {
                *error_code = ERROR_CODE_INVALID_EVENT_STATE;
                return -1;
            }
            break;

        default:
            return -2;
    }
    pObject->Ack_notify_data.bSendAckNotify = true;
    pObject->Ack_notify_data.EventState = alarmack_data->eventStateAcked;

    return 1;
}

int Binary_Value_Alarm_Summary(
    unsigned index, BACNET_GET_ALARM_SUMMARY_DATA *getalarm_data)
{
    struct object_data *pObject = Binary_Value_Object_Index(index);

    if (getalarm_data == NULL) {
        PRINT(
            "[%s %d]: NULL pointer parameter! getalarm_data = %p\r\n", __FILE__,
            __LINE__, (void *)getalarm_data);
        return -2;
    }

    /* check index */
    if (pObject) {
        /* Event_State is not equal to NORMAL  and
           Notify_Type property value is ALARM */
        if ((pObject->Event_State != EVENT_STATE_NORMAL) &&
            (pObject->Notify_Type == NOTIFY_ALARM)) {
            /* Object Identifier */
            getalarm_data->objectIdentifier.type = Object_Type;
            getalarm_data->objectIdentifier.instance =
                Binary_Value_Index_To_Instance(index);
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
        } else
            return 0; /* no active alarm at this index */
    } else
        return -1; /* end of list  */
}

/**
 * For a given object instance-number, returns the time_delay property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  time_delay property value
 */
uint32_t Binary_Value_Time_Delay(uint32_t object_instance)
{
    uint32_t time_delay = 0;
    struct object_data *pObject = Binary_Value_Object(object_instance);

    if (pObject) {
        time_delay = pObject->Time_Delay;
    }

    return time_delay;
}

/**
 * For a given object instance-number, sets the time_delay property value
 *
 * @param object_instance - object-instance number of the object
 * @param time_delay - time_delay property value
 *
 * @return true if the time_delay property value was set
 */
bool Binary_Value_Time_Delay_Set(uint32_t object_instance, uint32_t time_delay)
{
    bool status = false;
    struct object_data *pObject = Binary_Value_Object(object_instance);

    if (pObject) {
        pObject->Time_Delay = time_delay;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, returns the notification_class property
 * value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  notification_class property value
 */
uint32_t Binary_Value_Notification_Class(uint32_t object_instance)
{
    uint32_t notification_class = BACNET_MAX_INSTANCE;
    struct object_data *pObject = Binary_Value_Object(object_instance);

    if (pObject) {
        notification_class = pObject->Notification_Class;
    }

    return notification_class;
}

/**
 * For a given object instance-number, sets the notification_class property
 * value
 *
 * @param object_instance - object-instance number of the object
 * @param notification_class - notification_class property value
 *
 * @return true if the notification_class property value was set
 */
bool Binary_Value_Notification_Class_Set(
    uint32_t object_instance, uint32_t notification_class)
{
    bool status = false;
    struct object_data *pObject = Binary_Value_Object(object_instance);

    if (pObject) {
        pObject->Notification_Class = notification_class;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, returns the alarm_value property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  alarm_value property value
 */
BACNET_BINARY_PV Binary_Value_Alarm_Value(uint32_t object_instance)
{
    BACNET_BINARY_PV alarm_value = BINARY_NULL;
    struct object_data *pObject = Binary_Value_Object(object_instance);

    if (pObject) {
        alarm_value = pObject->Alarm_Value;
    }

    return alarm_value;
}

/**
 * For a given object instance-number, sets the alarm_value property value
 *
 * @param object_instance - object-instance number of the object
 * @param Alarm_Value - alarm_value property value
 *
 * @return true if the alarm_value property value was set
 */
bool Binary_Value_Alarm_Value_Set(
    uint32_t object_instance, BACNET_BINARY_PV value)
{
    bool status = false;
    struct object_data *pObject = Binary_Value_Object(object_instance);

    if (pObject) {
        if (pObject->Polarity != POLARITY_NORMAL) {
            value =
                (value == BINARY_INACTIVE) ? BINARY_ACTIVE : BINARY_INACTIVE;
        }
        pObject->Alarm_Value = value;
        status = true;
    }

    return status;
}
#endif /* (INTRINSIC_REPORTING) */

void Binary_Value_Intrinsic_Reporting(uint32_t object_instance)
{
#if !(defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING))
    (void)object_instance;
#else
    BACNET_EVENT_NOTIFICATION_DATA event_data = { 0 };
    BACNET_CHARACTER_STRING msgText = { 0 };
    uint8_t FromState = 0;
    uint8_t ToState = 0;
    BACNET_BINARY_PV PresentVal = BINARY_INACTIVE;
    bool SendNotify = false;
    struct object_data *pObject = Binary_Value_Object(object_instance);

    if (!pObject) {
        return;
    }

    /* check whether Intrinsic reporting is enabled */
    if (!pObject->Event_Detection_Enable) {
        return; /* limits are not configured */
    }

    if (pObject->Ack_notify_data.bSendAckNotify) {
        /* clean bSendAckNotify flag */
        pObject->Ack_notify_data.bSendAckNotify = false;
        /* copy toState */
        ToState = pObject->Ack_notify_data.EventState;
        PRINT("Binary-Input[%d]: Send AckNotification.\n", object_instance);
        characterstring_init_ansi(&msgText, "AckNotification");

        /* Notify Type */
        event_data.notifyType = NOTIFY_ACK_NOTIFICATION;

        /* Send EventNotification. */
        SendNotify = true;
    } else {
        /* actual Present_Value */
        PresentVal = Binary_Value_Present_Value(object_instance);
        FromState = pObject->Event_State;
        switch (pObject->Event_State) {
            case EVENT_STATE_NORMAL:
                /* (a) If pCurrentState is NORMAL, and pMonitoredValue is equal
                   to any of the values contained in pAlarmValues for
                       pTimeDelay, then indicate a transition to the OFFNORMAL
                   event state.
                */
                if ((PresentVal == pObject->Alarm_Value) &&
                    ((pObject->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ==
                     EVENT_ENABLE_TO_OFFNORMAL)) {
                    if (!pObject->Remaining_Time_Delay)
                        pObject->Event_State = EVENT_STATE_OFFNORMAL;
                    else
                        pObject->Remaining_Time_Delay--;
                    break;
                }

                /* value of the object is still in the same event state */
                pObject->Remaining_Time_Delay = pObject->Time_Delay;
                break;

            case EVENT_STATE_OFFNORMAL:
                /* (b) If pCurrentState is OFFNORMAL, and pMonitoredValue is not
                   equal to any of the values contained in pAlarmValues for
                   pTimeDelayNormal, then indicate a transition to the NORMAL
                   event state.
                */
                if ((PresentVal != pObject->Alarm_Value) &&
                    ((pObject->Event_Enable & EVENT_ENABLE_TO_NORMAL) ==
                     EVENT_ENABLE_TO_NORMAL)) {
                    if (!pObject->Remaining_Time_Delay)
                        pObject->Event_State = EVENT_STATE_NORMAL;
                    else
                        pObject->Remaining_Time_Delay--;
                    break;
                }

                /* value of the object is still in the same event state */
                pObject->Remaining_Time_Delay = pObject->Time_Delay;
                break;

            default:
                return; /* shouldn't happen */
        } /* switch (FromState) */

        ToState = pObject->Event_State;

        if (FromState != ToState) {
            /* Event_State has changed.
               Need to fill only the basic parameters of this type of event.
               Other parameters will be filled in common function. */

            switch (ToState) {
                case EVENT_STATE_NORMAL:
                    characterstring_init_ansi(
                        &msgText, "Back to normal state from off-normal");
                    break;

                case EVENT_STATE_OFFNORMAL:
                    characterstring_init_ansi(
                        &msgText, "Back to off-normal state from normal");
                    break;

                default:
                    break;
            } /* switch (ToState) */
            PRINT(
                "Binary-Input[%d]: Event_State goes from %.128s to %.128s.\n",
                object_instance, bactext_event_state_name(FromState),
                bactext_event_state_name(ToState));
            /* Notify Type */
            event_data.notifyType = pObject->Notify_Type;

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
                case EVENT_STATE_OFFNORMAL:
                    datetime_copy(
                        &pObject->Event_Time_Stamps[TRANSITION_TO_OFFNORMAL],
                        &event_data.timeStamp.value.dateTime);
                    break;
                case EVENT_STATE_FAULT:
                    datetime_copy(
                        &pObject->Event_Time_Stamps[TRANSITION_TO_FAULT],
                        &event_data.timeStamp.value.dateTime);
                    break;
                case EVENT_STATE_NORMAL:
                    datetime_copy(
                        &pObject->Event_Time_Stamps[TRANSITION_TO_NORMAL],
                        &event_data.timeStamp.value.dateTime);
                    break;
                default:
                    break;
            }
        } else {
            /* fill event_data timeStamp */
            switch (ToState) {
                case EVENT_STATE_FAULT:
                    datetime_copy(
                        &event_data.timeStamp.value.dateTime,
                        &pObject->Event_Time_Stamps[TRANSITION_TO_FAULT]);
                    break;
                case EVENT_STATE_NORMAL:
                    datetime_copy(
                        &event_data.timeStamp.value.dateTime,
                        &pObject->Event_Time_Stamps[TRANSITION_TO_NORMAL]);
                    break;
                case EVENT_STATE_OFFNORMAL:
                    datetime_copy(
                        &event_data.timeStamp.value.dateTime,
                        &pObject->Event_Time_Stamps[TRANSITION_TO_OFFNORMAL]);
                    break;
                default:
                    break;
            }
        }

        /* Notification Class */
        event_data.notificationClass = pObject->Notification_Class;

        /* Event Type */
        event_data.eventType = EVENT_CHANGE_OF_STATE;

        /* Message Text */
        event_data.messageText = &msgText;

        /* Notify Type */
        /* filled before */

        /* From State */
        if (event_data.notifyType != NOTIFY_ACK_NOTIFICATION)
            event_data.fromState = FromState;

        /* To State */
        event_data.toState = pObject->Event_State;

        /* Event Values */
        if (event_data.notifyType != NOTIFY_ACK_NOTIFICATION) {
            /* Value that exceeded a limit. */
            event_data.notificationParams.changeOfState.newState =
                (BACNET_PROPERTY_STATE) {
                    .tag = PROP_STATE_BINARY_VALUE,
                    .state = { .binaryValue = pObject->Present_Value }
                };
            /* Status_Flags of the referenced object. */
            bitstring_init(
                &event_data.notificationParams.changeOfState.statusFlags);
            bitstring_set_bit(
                &event_data.notificationParams.changeOfState.statusFlags,
                STATUS_FLAG_IN_ALARM,
                pObject->Event_State != EVENT_STATE_NORMAL);
            bitstring_set_bit(
                &event_data.notificationParams.changeOfState.statusFlags,
                STATUS_FLAG_FAULT, false);
            bitstring_set_bit(
                &event_data.notificationParams.changeOfState.statusFlags,
                STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(
                &event_data.notificationParams.changeOfState.statusFlags,
                STATUS_FLAG_OUT_OF_SERVICE, pObject->Out_Of_Service);
        }

        /* add data from notification class */
        PRINT(
            "Binary-Value[%d]: Notification Class[%d]-%s "
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
            PRINT("Binary-Value[%d]: Ack Required!\n", object_instance);
            switch (event_data.toState) {
                case EVENT_STATE_OFFNORMAL:
                    pObject->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                        .bIsAcked = false;
                    pObject->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;

                case EVENT_STATE_FAULT:
                    pObject->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked =
                        false;
                    pObject->Acked_Transitions[TRANSITION_TO_FAULT].Time_Stamp =
                        event_data.timeStamp.value.dateTime;
                    break;

                case EVENT_STATE_NORMAL:
                    pObject->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked =
                        false;
                    pObject->Acked_Transitions[TRANSITION_TO_NORMAL]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;

                default: /* shouldn't happen */
                    break;
            }
        }
    }
#endif /* defined(INTRINSIC_REPORTING) && (BINARY_VALUE_INTRINSIC_REPORTING) \
        */
}
