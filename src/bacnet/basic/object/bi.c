/**
 * @file
 * @author Steve Karg
 * @date 2006
 * @brief Binary Input object is an input object with a present-value that
 * uses an enumerated two state active/inactive data type.
 * @section LICENSE
 * Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
 * SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/cov.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "bacnet/basic/object/bi.h"

static const char *Default_Active_Text = "Active";
static const char *Default_Inactive_Text = "Inactive";
struct object_data {
    bool Out_Of_Service : 1;
    bool Change_Of_Value : 1;
    bool Present_Value : 1;
    bool Polarity : 1;
    bool Write_Enabled : 1;
    uint8_t Reliability;
    const char *Object_Name;
    const char *Active_Text;
    const char *Inactive_Text;
    const char *Description;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_BINARY_INPUT;
/* callback for present value writes */
static binary_input_write_present_value_callback
    Binary_Input_Write_Present_Value_Callback;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Binary_Input_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_PRESENT_VALUE, PROP_STATUS_FLAGS,
    PROP_EVENT_STATE, PROP_OUT_OF_SERVICE, PROP_POLARITY, -1 };

static const int Binary_Input_Properties_Optional[] = { PROP_DESCRIPTION, -1 };

static const int Binary_Input_Properties_Proprietary[] = { -1 };

/**
 * Initialize the pointers for the required, the optional and the properitary
 * value properties.
 *
 * @param pRequired - Pointer to the pointer of required values.
 * @param pOptional - Pointer to the pointer of optional values.
 * @param pProprietary - Pointer to the pointer of properitary values.
 */
void Binary_Input_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Binary_Input_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Binary_Input_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Binary_Input_Properties_Proprietary;
    }

    return;
}

/**
 * @brief Gets an object from the list using an instance number as the key
 * @param  object_instance - object-instance number of the object
 * @return object found in the list, or NULL if not found
 */
static struct object_data *Binary_Input_Object(uint32_t object_instance)
{
    return Keylist_Data(Object_List, object_instance);
}

/**
 * @brief Determines if a given Binary Input instance is valid
 * @param  object_instance - object-instance number of the object
 * @return  true if the instance is valid, and false if not
 */
bool Binary_Input_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * @brief Determines the number of Analog Value objects
 * @return  Number of Analog Value objects
 */
unsigned Binary_Input_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * @brief Determines the object instance-number for a given 0..N index
 * of Binary Input objects where N is Binary_Input_Count().
 * @param  index - 0..MAX_BINARY_INPUTS value
 * @return  object instance-number for the given index
 */
uint32_t Binary_Input_Index_To_Instance(unsigned index)
{
    return Keylist_Key(Object_List, index);
}

/**
 * @brief For a given object instance-number, determines a 0..N index
 * of Analog Value objects where N is Analog_Output_Count().
 * @param  object_instance - object-instance number of the object
 * @return  index for the given instance-number, or MAX_ANALOG_OUTPUTS
 * if not valid.
 */
unsigned Binary_Input_Instance_To_Index(uint32_t object_instance)
{
    return Keylist_Index(Object_List, object_instance);
}

/**
 * For a given object instance-number, determines the present-value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  present-value of the object
 */
BACNET_BINARY_PV Binary_Input_Present_Value(uint32_t object_instance)
{
    BACNET_BINARY_PV value = BINARY_INACTIVE;
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        value = pObject->Present_Value;
        if (pObject->Polarity != POLARITY_NORMAL) {
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
static void Binary_Input_Present_Value_COV_Detect(
    struct object_data *pObject, BACNET_BINARY_PV value)
{
    if (pObject) {
        if (pObject->Present_Value != value) {
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
bool Binary_Input_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        value = pObject->Out_Of_Service;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the out-of-service property value
 * @param object_instance - object-instance number of the object
 * @param value - boolean out-of-service value
 * @return true if the out-of-service property value was set
 */
void Binary_Input_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        if (pObject->Out_Of_Service != value) {
            pObject->Out_Of_Service = value;
            pObject->Change_Of_Value = true;
        }
    }

    return;
}

/**
 * @brief For a given object, gets the Fault status flag
 * @param  object_instance - object-instance number of the object
 * @return  true the status flag is in Fault
 */
static bool Binary_Input_Object_Fault(struct object_data *pObject)
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
 * @brief For a given object instance-number, gets the Fault status flag
 * @param  object_instance - object-instance number of the object
 * @return  true the status flag is in Fault
 */
static bool Binary_Input_Fault(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);

    return Binary_Input_Object_Fault(pObject);
}

/**
 * @brief For a given object instance-number, determines if the COV flag
 *  has been triggered.
 * @param  object_instance - object-instance number of the object
 * @return  true if the COV flag is set
 */
bool Binary_Input_Change_Of_Value(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        status = pObject->Change_Of_Value;
    }

    return status;
}

/**
 * @brief For a given object instance-number, clears the COV flag
 * @param  object_instance - object-instance number of the object
 */
void Binary_Input_Change_Of_Value_Clear(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        pObject->Change_Of_Value = false;
    }

    return;
}

/**
 * @brief For a given object instance-number, loads the value_list with the COV data.
 * @param  object_instance - object-instance number of the object
 * @param  value_list - list of COV data
 * @return  true if the value list is encoded
 */
bool Binary_Input_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list)
{
    bool status = false;
    const bool in_alarm = false;
    bool out_of_service = false;
    bool fault = false;
    const bool overridden = false;
    BACNET_BINARY_PV present_value = BINARY_INACTIVE;
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        if (pObject->Reliability != RELIABILITY_NO_FAULT_DETECTED) {
            fault = true;
        }
        out_of_service = pObject->Out_Of_Service;
        if (pObject->Present_Value) {
            present_value = BINARY_ACTIVE;
        }
        status = cov_value_list_encode_enumerated(value_list, present_value, 
            in_alarm, fault, overridden, out_of_service);
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the present-value
 * @param  object_instance - object-instance number of the object
 * @param  value - enumerated binary present-value
 * @return  true if values are within range and present-value is set.
 */
bool Binary_Input_Present_Value_Set(
    uint32_t object_instance, BACNET_BINARY_PV value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        if (value <= MAX_BINARY_PV) {
            if (pObject->Polarity != POLARITY_NORMAL) {
                if (value == BINARY_INACTIVE) {
                    value = BINARY_ACTIVE;
                } else {
                    value = BINARY_INACTIVE;
                }
            }
            Binary_Input_Present_Value_COV_Detect(pObject, value);
            pObject->Present_Value = true;
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
static bool Binary_Input_Present_Value_Write(
    uint32_t object_instance, BACNET_BINARY_PV value,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;
    BACNET_BINARY_PV old_value = BINARY_INACTIVE;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        if (value <= MAX_BINARY_PV) {
            if (pObject->Write_Enabled) {
                old_value = pObject->Present_Value;
                Binary_Input_Present_Value_COV_Detect(pObject, value);
                pObject->Present_Value = value;
                if (pObject->Out_Of_Service) {
                    /* The physical point that the object represents
                        is not in service. This means that changes to the
                        Present_Value property are decoupled from the
                        physical point when the value of Out_Of_Service
                        is true. */
                } else if (Binary_Input_Write_Present_Value_Callback) {
                    Binary_Input_Write_Present_Value_Callback(
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

bool Binary_Input_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32] = ""; /* okay for single thread */
    bool status = false;
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        if (pObject->Object_Name == NULL) {
            sprintf(
                text_string, "BINARY INPUT %lu", (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, text_string);
        } else {
            status = characterstring_init_ansi(object_name, pObject->Object_Name);
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
bool Binary_Input_Name_Set(uint32_t object_instance, char *new_name)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
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
BACNET_POLARITY Binary_Input_Polarity(uint32_t object_instance)
{
    BACNET_POLARITY polarity = POLARITY_NORMAL;
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        polarity = pObject->Polarity;
    }

    return polarity;
}

/**
 * @brief For a given object instance-number, sets the polarity property
 * @param  object_instance - object-instance number of the object
 * @param  polarity - polarity property value
 * @return  true if polarity was set
 */
bool Binary_Input_Polarity_Set(
    uint32_t object_instance, BACNET_POLARITY polarity)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        pObject->Polarity = polarity;
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the description
 * @param  object_instance - object-instance number of the object
 * @return description text or NULL if not found
 */
char *Binary_Input_Description(uint32_t object_instance)
{
    char *name = NULL;
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
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
bool Binary_Input_Description_Set(uint32_t object_instance, char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        status = true;
        pObject->Description = new_name;
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
int Binary_Input_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;
    bool state = false;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], Object_Type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            /* note: object name must be unique in our device */
            Binary_Input_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_enumerated(
                &apdu[0], Binary_Input_Present_Value(rpdata->object_instance));
            break;
        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            state = Binary_Input_Fault(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, state);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Binary_Input_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Binary_Input_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_POLARITY:
            apdu_len = encode_application_enumerated(
                &apdu[0], Binary_Input_Polarity(rpdata->object_instance));
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(&char_string,
                Binary_Input_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->array_index != BACNET_ARRAY_ALL)) {
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
bool Binary_Input_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

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
    if (wp_data->array_index != BACNET_ARRAY_ALL) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                status =
                    Binary_Input_Present_Value_Write(wp_data->object_instance,
                        value.type.Enumerated,
                        &wp_data->error_class, &wp_data->error_code);
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Binary_Input_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_POLARITY:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated < MAX_POLARITY) {
                    Binary_Input_Polarity_Set(wp_data->object_instance,
                        (BACNET_POLARITY)value.type.Enumerated);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        default:
            if (property_lists_member(
                Binary_Input_Properties_Required, 
                Binary_Input_Properties_Optional, 
                Binary_Input_Properties_Proprietary, 
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
void Binary_Input_Write_Present_Value_Callback_Set(
    binary_input_write_present_value_callback cb)
{
    Binary_Input_Write_Present_Value_Callback = cb;
}

/**
 * @brief Determines a object write-enabled flag state
 * @param object_instance - object-instance number of the object
 * @return  write-enabled status flag
 */
bool Binary_Input_Write_Enabled(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        value = pObject->Write_Enabled;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the write-enabled flag
 * @param object_instance - object-instance number of the object
 */
void Binary_Input_Write_Enable(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        pObject->Write_Enabled = true;
    }
}

/**
 * @brief For a given object instance-number, clears the write-enabled flag
 * @param object_instance - object-instance number of the object
 */
void Binary_Input_Write_Disable(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Binary_Input_Object(object_instance);
    if (pObject) {
        pObject->Write_Enabled = false;
    }
}

/**
 * Creates a Binary Input object
 * @param object_instance - object-instance number of the object
 */
uint32_t Binary_Input_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
    int index = 0;

    pObject = Binary_Input_Object(object_instance);
    if (!pObject) {
        pObject = calloc(1, sizeof(struct object_data));
        if (pObject) {
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
void Binary_Input_Cleanup(void)
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
 * Delete a specific Binary Input object
 * @param object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Binary_Input_Delete(uint32_t object_instance)
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
void Binary_Input_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
