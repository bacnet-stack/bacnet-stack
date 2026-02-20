/**
 * @file
 * @brief The Loop object type defines a standardized object whose
 *  properties represent the externally visible characteristics of
 *  any form of feedback control loop. Flexibility is achieved by
 *  providing three independent gain constants with no assumed
 *  values for units. The appropriate gain units are determined
 *  by the details of the control algorithm, which is a local matter.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date November 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/bactext.h"
#include "bacnet/datetime.h"
#include "bacnet/proplist.h"
#include "bacnet/timer_value.h"
/* basic objects and services */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "bacnet/basic/object/loop.h"

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List = NULL;
/* common object type */
static const BACNET_OBJECT_TYPE Object_Type = OBJECT_LOOP;
/* handling for manipulated and reference properties */
static write_property_function Write_Property_Internal_Callback;
static read_property_function Read_Property_Internal_Callback;
/* Write Property notification callbacks for logging or other purposes */
static struct loop_write_property_notification Write_Property_Notification_Head;

struct object_data {
    /* internal variables for PID calculations */
    uint32_t Update_Timer;
    float Integral_Sum;
    float Error;
    /* variables for object properties */
    uint32_t Update_Interval;
    float Present_Value;
    BACNET_ENGINEERING_UNITS Output_Units;
    BACNET_OBJECT_PROPERTY_REFERENCE Manipulated_Variable_Reference;
    BACNET_ENGINEERING_UNITS Controlled_Variable_Units;
    float Controlled_Variable_Value;
    BACNET_OBJECT_PROPERTY_REFERENCE Controlled_Variable_Reference;
    float Setpoint;
    BACNET_OBJECT_PROPERTY_REFERENCE Setpoint_Reference;
    BACNET_ACTION Action;
    float Proportional_Constant;
    BACNET_ENGINEERING_UNITS Proportional_Constant_Units;
    float Integral_Constant;
    BACNET_ENGINEERING_UNITS Integral_Constant_Units;
    float Derivative_Constant;
    BACNET_ENGINEERING_UNITS Derivative_Constant_Units;
    float Bias;
    float Maximum_Output;
    float Minimum_Output;
    float COV_Increment;
    uint8_t Priority_For_Writing;
    const char *Description;
    const char *Object_Name;
    BACNET_RELIABILITY Reliability;
    bool Out_Of_Service : 1;
    bool Changed : 1;
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
    PROP_OUTPUT_UNITS,
    PROP_MANIPULATED_VARIABLE_REFERENCE,
    PROP_CONTROLLED_VARIABLE_REFERENCE,
    PROP_CONTROLLED_VARIABLE_VALUE,
    PROP_CONTROLLED_VARIABLE_UNITS,
    PROP_SETPOINT_REFERENCE,
    PROP_SETPOINT,
    PROP_ACTION,
    PROP_PRIORITY_FOR_WRITING,
    -1
};

static const int32_t Properties_Optional[] = {
    /* unordered list of optional properties */
    PROP_DESCRIPTION,
    PROP_RELIABILITY,
    PROP_PROPORTIONAL_CONSTANT,
    PROP_PROPORTIONAL_CONSTANT_UNITS,
    PROP_INTEGRAL_CONSTANT,
    PROP_INTEGRAL_CONSTANT_UNITS,
    PROP_DERIVATIVE_CONSTANT,
    PROP_DERIVATIVE_CONSTANT_UNITS,
    PROP_BIAS,
    PROP_MAXIMUM_OUTPUT,
    PROP_MINIMUM_OUTPUT,
    PROP_COV_INCREMENT,
    PROP_UPDATE_INTERVAL,
    -1
};

/* handling for proprietary properties */
static const int32_t Properties_Proprietary[] = { -1 };

/* Every object shall have a Writable Property_List property
   which is a BACnetARRAY of property identifiers,
   one property identifier for each property within this object
   that is always writable.  */
static const int32_t Writable_Properties[] = {
    PROP_PRESENT_VALUE,
    PROP_OUT_OF_SERVICE,
    PROP_ACTION,
    PROP_UPDATE_INTERVAL,
    PROP_OUTPUT_UNITS,
    PROP_CONTROLLED_VARIABLE_VALUE,
    PROP_CONTROLLED_VARIABLE_UNITS,
    PROP_PROPORTIONAL_CONSTANT,
    PROP_PROPORTIONAL_CONSTANT_UNITS,
    PROP_INTEGRAL_CONSTANT,
    PROP_INTEGRAL_CONSTANT_UNITS,
    PROP_DERIVATIVE_CONSTANT,
    PROP_DERIVATIVE_CONSTANT_UNITS,
    PROP_BIAS,
    PROP_SETPOINT,
    PROP_MINIMUM_OUTPUT,
    PROP_MAXIMUM_OUTPUT,
    PROP_PRIORITY_FOR_WRITING,
    PROP_MANIPULATED_VARIABLE_REFERENCE,
    PROP_CONTROLLED_VARIABLE_REFERENCE,
    PROP_SETPOINT_REFERENCE,
    PROP_COV_INCREMENT,
    -1
};

/**
 * Returns the list of required, optional, and proprietary properties.
 * Used by ReadPropertyMultiple service.
 *
 * @param pRequired - pointer to list of int terminated by -1, of
 * BACnet required properties for this object.
 * @param pOptional - pointer to list of int terminated by -1, of
 * BACnet optional properties for this object.
 * @param pProprietary - pointer to list of int terminated by -1, of
 * BACnet proprietary properties for this object.
 */
void Loop_Property_Lists(
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
 * @brief Get the list of writable properties for a Loop object
 * @param  object_instance - object-instance number of the object
 * @param  properties - Pointer to the pointer of writable properties.
 */
void Loop_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties)
{
    (void)object_instance;
    if (properties) {
        *properties = Writable_Properties;
    }
}

/**
 * @brief Determine if the property is a member of this object
 * @param object_property - object-property to be checked
 * @return true if the property is a member of any of these lists
 */
static bool Loop_Property_Lists_Member(int object_property)
{
    const int32_t *pRequired;
    const int32_t *pOptional;
    const int32_t *pProprietary;

    Loop_Property_Lists(&pRequired, &pOptional, &pProprietary);
    return property_lists_member(
        pRequired, pOptional, pProprietary, object_property);
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
 * Determines if a given Loop instance is valid
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if the instance is valid, and false if not
 */
bool Loop_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject = Object_Data(object_instance);

    return (pObject != NULL);
}

/**
 * Determines the number of Loop objects
 *
 * @return  Number of Loop objects
 */
unsigned Loop_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Loop objects where N is Loop_Count().
 *
 * @param  index - 0..MAX_PROGRAMS value
 *
 * @return  object instance-number for the given index
 */
uint32_t Loop_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Loop objects where N is Loop_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or MAX_PROGRAMS
 * if not valid.
 */
unsigned Loop_Instance_To_Index(uint32_t object_instance)
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
bool Loop_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32] = "";
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(
                text, sizeof(text), "LOOP-%lu", (unsigned long)object_instance);
            status = characterstring_init_ansi(object_name, text);
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
bool Loop_Name_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
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
const char *Loop_Name_ASCII(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        name = pObject->Object_Name;
    }

    return name;
}

/**
 * For a given object instance-number, return the description.
 * @param  object_instance - object-instance number of the object
 * @param  description - description pointer
 * @return  true/false
 */
bool Loop_Description(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Description) {
            status =
                characterstring_init_ansi(description, pObject->Description);
        } else {
            status = characterstring_init_ansi(description, "");
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the description
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the description to be set
 * @return  true if string was set
 */
bool Loop_Description_Set(uint32_t object_instance, const char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        status = true;
        pObject->Description = new_name;
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the description
 * @param  object_instance - object-instance number of the object
 * @return description text or NULL if not found
 */
const char *Loop_Description_ANSI(uint32_t object_instance)
{
    const char *name = NULL;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (pObject->Description == NULL) {
            name = "";
        } else {
            name = pObject->Description;
        }
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
bool Loop_Out_Of_Service(uint32_t object_instance)
{
    struct object_data *pObject;
    bool value = false;

    pObject = Object_Data(object_instance);
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
void Loop_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->Out_Of_Service = value;
    }
}

/**
 * @brief For a given object instance-number, gets the reliability.
 * @param  object_instance - object-instance number of the object
 * @return reliability value
 */
BACNET_RELIABILITY Loop_Reliability(uint32_t object_instance)
{
    BACNET_RELIABILITY reliability = RELIABILITY_NO_FAULT_DETECTED;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        reliability = pObject->Reliability;
    }

    return reliability;
}

/**
 * @brief For a given object instance-number, gets the Fault status flag
 * @param  object_instance - object-instance number of the object
 * @return  true the status flag is in Fault
 */
static bool Loop_Fault(uint32_t object_instance)
{
    struct object_data *pObject;
    bool fault = false;

    pObject = Keylist_Data(Object_List, object_instance);
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
 * @param  value - reliability enumerated value
 * @return  true if values are within range and property is set.
 */
bool Loop_Reliability_Set(uint32_t object_instance, BACNET_RELIABILITY value)
{
    struct object_data *pObject;
    bool status = false;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value <= 255) {
            pObject->Reliability = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief This property indicates the current output value
 *  of the loop algorithm in units of the Output_Units property.
 * @param object_instance [in] BACnet object instance number
 * @return the present-value for a specific object instance
 */
float Loop_Present_Value(uint32_t object_instance)
{
    float value = 0.0f;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Present_Value;
    }

    return value;
}

/**
 * @brief This property sets the current output value
 *  of the loop algorithm in units of the Output_Units property.
 * The Present_Value property shall be writable when Out_Of_Service is TRUE.
 * @param object_instance [in] BACnet object instance number
 * @return true if the present-value for a specific object instance was set
 */
bool Loop_Present_Value_Set(uint32_t object_instance, float value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (isfinite(value)) {
            pObject->Present_Value = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief This property, of type Unsigned, indicates the interval
 *  in milliseconds at which the loop algorithm updates the output
 *  (Present_Value property).
 * @param object_instance [in] BACnet object instance number
 * @return the update-interval for a specific object instance
 */
uint32_t Loop_Update_Interval(uint32_t object_instance)
{
    uint32_t value = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Update_Interval;
    }

    return value;
}

/**
 * @brief This property, of type Unsigned, sets the interval
 *  in milliseconds at which the loop algorithm updates the output
 *  (Present_Value property).
 * @param object_instance [in] BACnet object instance number
 * @param value [in] the update-interval for a specific object instance
 * @return true if the update-interval for a specific object instance was set
 */
bool Loop_Update_Interval_Set(uint32_t object_instance, uint32_t value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Update_Interval = value;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, returns the output-units property value
 * @param  object_instance - object-instance number of the object
 * @return  output-units property value
 */
BACNET_ENGINEERING_UNITS Loop_Output_Units(uint32_t object_instance)
{
    BACNET_ENGINEERING_UNITS units = UNITS_NO_UNITS;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        units = pObject->Output_Units;
    }

    return units;
}

/**
 * For a given object instance-number, sets the output-units property value
 * @param object_instance - object-instance number of the object
 * @param units - units property value
 * @return true if the output-units property value was set
 */
bool Loop_Output_Units_Set(
    uint32_t object_instance, BACNET_ENGINEERING_UNITS units)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Output_Units = units;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, determines if the reference is empty
 * A BACnetObjectPropertyReference containing an object instance number
 * equal to 4194303 is considered to be 'empty' or 'uninitialized'.
 * @param  value - object property reference
 * @return true if the reference is empty
 */
static bool
Object_Property_Reference_Empty(const BACNET_OBJECT_PROPERTY_REFERENCE *value)
{
    bool status = false;

    if (value) {
        if (value->object_identifier.instance == BACNET_MAX_INSTANCE) {
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the reference to be empty
 * A BACnetObjectPropertyReference containing an object instance number
 * equal to 4194303 is considered to be 'empty' or 'uninitialized'.
 * @param  value - object property reference
 * @return true if the reference is empty
 */
static void Object_Property_Reference_Set(
    BACNET_OBJECT_PROPERTY_REFERENCE *value,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property_id,
    BACNET_ARRAY_INDEX array_index)
{
    if (value) {
        value->object_identifier.type = object_type;
        value->object_identifier.instance = object_instance;
        value->property_identifier = property_id;
        value->property_array_index = array_index;
    }
}

/**
 * @brief This property is of type BACnetObjectPropertyReference.
 *  The output (Present_Value) of the control loop is written to the
 *  object and property designated by the Manipulated_Variable_Reference.
 *  It is normally the Present_Value of an Analog Output object
 *  used to position a device, but it could also be another object
 *  or property, such as that used to stage multiple Binary Outputs.
 * @param object_instance - object-instance number of the object
 * @param value - [out] BACnetObjectPropertyReference
 */
bool Loop_Manipulated_Variable_Reference(
    uint32_t object_instance, BACNET_OBJECT_PROPERTY_REFERENCE *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = bacnet_object_property_reference_copy(
            value, &pObject->Manipulated_Variable_Reference);
    }

    return status;
}

/**
 * @brief This property is of type BACnetObjectPropertyReference.
 *  The output (Present_Value) of the control loop is written to the
 *  object and property designated by the Manipulated_Variable_Reference.
 *  It is normally the Present_Value of an Analog Output object
 *  used to position a device, but it could also be another object
 *  or property, such as that used to stage multiple Binary Outputs.
 * @param object_instance - object-instance number of the object
 * @param value - [in] BACnetObjectPropertyReference
 * @return true if the property value was set
 */
bool Loop_Manipulated_Variable_Reference_Set(
    uint32_t object_instance, BACNET_OBJECT_PROPERTY_REFERENCE *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = bacnet_object_property_reference_copy(
            &pObject->Manipulated_Variable_Reference, value);
    }

    return status;
}

/**
 * @brief This property is of type BACnetObjectPropertyReference.
 *  The Controlled_Variable_Reference identifies the property used
 *  to set the Controlled_Variable_Value property of the Loop object.
 *  It is normally the Present_Value property of an Analog Input object
 *  used for measuring a process variable, temperature, for example,
 *  but it could also be another object, such as an Analog Value,
 *  which calculates a minimum or maximum of a group of Analog Inputs
 *  for use in discriminator control.
 * @param object_instance - object-instance number of the object
 * @param value - [in] BACnetObjectPropertyReference
 * @return true if the property value was retrieved
 */
bool Loop_Controlled_Variable_Reference(
    uint32_t object_instance, BACNET_OBJECT_PROPERTY_REFERENCE *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = bacnet_object_property_reference_copy(
            value, &pObject->Controlled_Variable_Reference);
    }

    return status;
}

/**
 * @brief Sets the Controlled_Variable_Reference property value
 * @param object_instance - object-instance number of the object
 * @param value - [in] BACnetObjectPropertyReference
 * @return true if the property value was set
 */
bool Loop_Controlled_Variable_Reference_Set(
    uint32_t object_instance, BACNET_OBJECT_PROPERTY_REFERENCE *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = bacnet_object_property_reference_copy(
            &pObject->Controlled_Variable_Reference, value);
    }

    return status;
}

/**
 * @brief This property, of type Real, is the value of the property
 *  of the object referenced by the Controlled_Variable_Reference property.
 *  This control loop compares the Controlled_Variable_Value with
 *  the Setpoint to calculate the error.
 * @param object_instance [in] BACnet object instance number
 * @return the Controlled_Variable_Value for a specific object instance
 */
float Loop_Controlled_Variable_Value(uint32_t object_instance)
{
    float value = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Controlled_Variable_Value;
    }

    return value;
}

/**
 * @brief This property sets the loop-controlled-variable value
 *  of the loop algorithm in units of the Output_Units property.
 * The Present_Value property shall be writable when Out_Of_Service is TRUE.
 * @param object_instance [in] BACnet object instance number
 * @return true if the present-value for a specific object instance was set
 */
bool Loop_Controlled_Variable_Value_Set(uint32_t object_instance, float value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (isfinite(value)) {
            pObject->Controlled_Variable_Value = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the
 *  controlled-variable-units property value
 * @param  object_instance - object-instance number of the object
 * @return  output-units property value
 */
BACNET_ENGINEERING_UNITS
Loop_Controlled_Variable_Units(uint32_t object_instance)
{
    BACNET_ENGINEERING_UNITS units = UNITS_NO_UNITS;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        units = pObject->Controlled_Variable_Units;
    }

    return units;
}

/**
 * @brief For a given object instance-number, sets the
 *  controlled-variable-units property value
 * @param object_instance - object-instance number of the object
 * @param units - units property value
 * @return true if the output-units property value was set
 */
bool Loop_Controlled_Variable_Units_Set(
    uint32_t object_instance, BACNET_ENGINEERING_UNITS units)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Controlled_Variable_Units = units;
        status = true;
    }

    return status;
}

/**
 * @brief This property, of type BACnetSetpointReference, contains zero
 *  or one references. The absence of a reference indicates that the
 *  setpoint for this control loop is fixed and is contained in the
 *  Setpoint property. The presence of a reference indicates that
 *  the property of another object contains the setpoint value used
 *  for this Loop object and the reference specifies that property.
 * @param object_instance - object-instance number of the object
 * @param value - [in] BACnetObjectPropertyReference
 * @return true if the property value was retrieved
 */
bool Loop_Setpoint_Reference(
    uint32_t object_instance, BACNET_OBJECT_PROPERTY_REFERENCE *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = bacnet_object_property_reference_copy(
            value, &pObject->Setpoint_Reference);
    }

    return status;
}

/**
 * @brief Sets the Setpoint_Reference property value
 * @param object_instance - object-instance number of the object
 * @param value - [in] BACnetObjectPropertyReference
 * @return true if the property value was set
 */
bool Loop_Setpoint_Reference_Set(
    uint32_t object_instance, BACNET_OBJECT_PROPERTY_REFERENCE *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        status = bacnet_object_property_reference_copy(
            &pObject->Setpoint_Reference, value);
    }

    return status;
}

/**
 * @brief This property, of type Real, is the value of the loop setpoint
 *  or of the property of the object referenced by the Setpoint_Reference,
 *  expressed in units of the Controlled_Variable_Units property.
 * @param object_instance [in] BACnet object instance number
 * @return the Controlled_Variable_Value for a specific object instance
 */
float Loop_Setpoint(uint32_t object_instance)
{
    float value = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Setpoint;
    }

    return value;
}

/**
 * @brief This property sets the setpoint value
 * @param object_instance [in] BACnet object instance number
 * @param value [in] property value to set
 * @return true if the property value was set
 */
bool Loop_Setpoint_Set(uint32_t object_instance, float value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (isfinite(value)) {
            pObject->Setpoint = value;
            status = true;
        }
    }

    return status;
}

BACNET_ACTION Loop_Action(uint32_t object_instance)
{
    BACNET_ACTION value = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Action;
    }

    return value;
}

/**
 * @brief This property sets the action value
 * @param object_instance [in] BACnet object instance number
 * @param value [in] property value to set
 * @return true if the property value was set
 */
bool Loop_Action_Set(uint32_t object_instance, BACNET_ACTION value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value < BACNET_ACTION_MAX) {
            pObject->Action = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Gets the property value for a given object instance
 * @param  object_instance - object-instance number of the object
 * @return the property value for a given object instance
 */
float Loop_Proportional_Constant(uint32_t object_instance)
{
    float value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Proportional_Constant;
    }

    return value;
}

/**
 * @brief Sets the property value for a given object instance
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 * @return true if the property value was set
 */
bool Loop_Proportional_Constant_Set(uint32_t object_instance, float value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (isfinite(value)) {
            pObject->Proportional_Constant = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the units property value
 * @param object_instance - object-instance number of the object
 * @return units property value
 */
BACNET_ENGINEERING_UNITS
Loop_Proportional_Constant_Units(uint32_t object_instance)
{
    BACNET_ENGINEERING_UNITS units = UNITS_NO_UNITS;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        units = pObject->Proportional_Constant_Units;
    }

    return units;
}

/**
 * @brief For a given object instance-number, sets the units property value
 * @param object_instance - object-instance number of the object
 * @param units - units property value
 * @return true if the units property value was set
 */
bool Loop_Proportional_Constant_Units_Set(
    uint32_t object_instance, BACNET_ENGINEERING_UNITS units)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Proportional_Constant_Units = units;
        status = true;
    }

    return status;
}

/**
 * @brief Gets the property value for a given object instance
 * @param  object_instance - object-instance number of the object
 * @return the property value for a given object instance
 */
float Loop_Integral_Constant(uint32_t object_instance)
{
    float value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Integral_Constant;
    }

    return value;
}

/**
 * @brief Sets the property value for a given object instance
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 * @return true if the property value was set
 */
bool Loop_Integral_Constant_Set(uint32_t object_instance, float value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (isfinite(value)) {
            pObject->Integral_Constant = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the units property value
 * @param object_instance - object-instance number of the object
 * @return units property value
 */
BACNET_ENGINEERING_UNITS Loop_Integral_Constant_Units(uint32_t object_instance)
{
    BACNET_ENGINEERING_UNITS units = UNITS_NO_UNITS;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        units = pObject->Integral_Constant_Units;
    }

    return units;
}

/**
 * @brief For a given object instance-number, sets the units property value
 * @param object_instance - object-instance number of the object
 * @param units - units property value
 * @return true if the units property value was set
 */
bool Loop_Integral_Constant_Units_Set(
    uint32_t object_instance, BACNET_ENGINEERING_UNITS units)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Integral_Constant_Units = units;
        status = true;
    }

    return status;
}

/**
 * @brief Gets the property value for a given object instance
 * @param  object_instance - object-instance number of the object
 * @return the property value for a given object instance
 */
float Loop_Derivative_Constant(uint32_t object_instance)
{
    float value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Derivative_Constant;
    }

    return value;
}

/**
 * @brief Sets the property value for a given object instance
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 * @return true if the property value was set
 */
bool Loop_Derivative_Constant_Set(uint32_t object_instance, float value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (isfinite(value)) {
            pObject->Derivative_Constant = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the units property value
 * @param object_instance - object-instance number of the object
 * @return units property value
 */
BACNET_ENGINEERING_UNITS
Loop_Derivative_Constant_Units(uint32_t object_instance)
{
    BACNET_ENGINEERING_UNITS units = UNITS_NO_UNITS;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        units = pObject->Derivative_Constant_Units;
    }

    return units;
}

/**
 * @brief For a given object instance-number, sets the units property value
 * @param object_instance - object-instance number of the object
 * @param units - units property value
 * @return true if the units property value was set
 */
bool Loop_Derivative_Constant_Units_Set(
    uint32_t object_instance, BACNET_ENGINEERING_UNITS units)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Derivative_Constant_Units = units;
        status = true;
    }

    return status;
}

/**
 * @brief Gets the property value for a given object instance
 * @param  object_instance - object-instance number of the object
 * @return the property value for a given object instance
 */
float Loop_Bias(uint32_t object_instance)
{
    float value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Bias;
    }

    return value;
}

/**
 * @brief Sets the property value for a given object instance
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 * @return true if the property value was set
 */
bool Loop_Bias_Set(uint32_t object_instance, float value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (isfinite(value)) {
            pObject->Bias = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Gets the property value for a given object instance
 * @param  object_instance - object-instance number of the object
 * @return the property value for a given object instance
 */
float Loop_Maximum_Output(uint32_t object_instance)
{
    float value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Maximum_Output;
    }

    return value;
}

/**
 * @brief Sets the property value for a given object instance
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 * @return true if the property value was set
 */
bool Loop_Maximum_Output_Set(uint32_t object_instance, float value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (isfinite(value)) {
            pObject->Maximum_Output = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Gets the property value for a given object instance
 * @param  object_instance - object-instance number of the object
 * @return the property value for a given object instance
 */
float Loop_Minimum_Output(uint32_t object_instance)
{
    float value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Minimum_Output;
    }

    return value;
}

/**
 * @brief Sets the property value for a given object instance
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 * @return true if the property value was set
 */
bool Loop_Minimum_Output_Set(uint32_t object_instance, float value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (isfinite(value)) {
            pObject->Minimum_Output = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Gets the priority-for-writing property value for a given object
 * instance
 * @param  object_instance - object-instance number of the object
 * @return priority-for-writing property value
 */
uint8_t Loop_Priority_For_Writing(uint32_t object_instance)
{
    uint8_t value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->Priority_For_Writing;
    }

    return value;
}

/**
 * @brief Sets the priority-for-writing property value for a given object
 * instance
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 * @return true if the priority-for-writing property value was in range and set
 */
bool Loop_Priority_For_Writing_Set(uint32_t object_instance, uint8_t value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        /* Unsigned(1..16) */
        if ((value >= BACNET_MIN_PRIORITY) && (value <= BACNET_MAX_PRIORITY)) {
            pObject->Priority_For_Writing = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Gets the property value for a given object instance
 * @param  object_instance - object-instance number of the object
 * @return the property value for a given object instance
 */
float Loop_COV_Increment(uint32_t object_instance)
{
    float value = 0;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        value = pObject->COV_Increment;
    }

    return value;
}

/**
 * @brief Sets the property value for a given object instance
 * @param object_instance - object-instance number of the object
 * @param value - property value to set
 * @return true if the property value was set
 */
bool Loop_COV_Increment_Set(uint32_t object_instance, float value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        if (isfinite(value) && isgreater(value, 0.0)) {
            pObject->COV_Increment = value;
            status = true;
        }
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
 * @return number of APDU bytes in the response, zero if no data, or
 * BACNET_STATUS_ERROR on error.
 */
int Loop_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string = { 0 };
    BACNET_CHARACTER_STRING char_string = { 0 };
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_OBJECT_PROPERTY_REFERENCE reference_value = { 0 };
    float real_value = 0.0f;
    uint8_t *apdu = NULL;
    uint32_t enum_value = 0;
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
            Loop_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], Object_Type);
            break;
        case PROP_PRESENT_VALUE:
            real_value = Loop_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            state = Loop_Fault(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, state);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Loop_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Loop_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_OUTPUT_UNITS:
            enum_value = Loop_Output_Units(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enum_value);
            break;
        case PROP_MANIPULATED_VARIABLE_REFERENCE:
            Loop_Manipulated_Variable_Reference(
                rpdata->object_instance, &reference_value);
            apdu_len =
                bacapp_encode_obj_property_ref(&apdu[0], &reference_value);
            break;
        case PROP_CONTROLLED_VARIABLE_REFERENCE:
            Loop_Controlled_Variable_Reference(
                rpdata->object_instance, &reference_value);
            apdu_len =
                bacapp_encode_obj_property_ref(&apdu[0], &reference_value);
            break;
        case PROP_CONTROLLED_VARIABLE_VALUE:
            real_value =
                Loop_Controlled_Variable_Value(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_CONTROLLED_VARIABLE_UNITS:
            enum_value =
                Loop_Controlled_Variable_Units(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enum_value);
            break;
        case PROP_SETPOINT_REFERENCE:
            Loop_Setpoint_Reference(rpdata->object_instance, &reference_value);
            apdu_len = bacapp_encode_obj_property_ref(apdu, &reference_value);
            break;
        case PROP_SETPOINT:
            real_value = Loop_Setpoint(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_UPDATE_INTERVAL:
            unsigned_value = Loop_Update_Interval(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], unsigned_value);
            break;
        case PROP_ACTION:
            enum_value = Loop_Action(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enum_value);
            break;
        case PROP_PRIORITY_FOR_WRITING:
            unsigned_value = Loop_Priority_For_Writing(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], unsigned_value);
            break;
        case PROP_DESCRIPTION:
            if (Loop_Description(rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;
        case PROP_RELIABILITY:
            enum_value = Loop_Reliability(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enum_value);
            break;

        case PROP_PROPORTIONAL_CONSTANT:
            real_value = Loop_Proportional_Constant(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_PROPORTIONAL_CONSTANT_UNITS:
            enum_value =
                Loop_Proportional_Constant_Units(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enum_value);
            break;
        case PROP_INTEGRAL_CONSTANT:
            real_value = Loop_Integral_Constant(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_INTEGRAL_CONSTANT_UNITS:
            enum_value = Loop_Integral_Constant_Units(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enum_value);
            break;
        case PROP_DERIVATIVE_CONSTANT:
            real_value = Loop_Derivative_Constant(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_DERIVATIVE_CONSTANT_UNITS:
            enum_value =
                Loop_Derivative_Constant_Units(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], enum_value);
            break;
        case PROP_BIAS:
            real_value = Loop_Bias(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_MAXIMUM_OUTPUT:
            real_value = Loop_Maximum_Output(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_MINIMUM_OUTPUT:
            real_value = Loop_Minimum_Output(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
            break;
        case PROP_COV_INCREMENT:
            real_value = Loop_COV_Increment(rpdata->object_instance);
            apdu_len = encode_application_real(&apdu[0], real_value);
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
bool Loop_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    /* decode the some of the request */
    len = bacapp_decode_known_array_property(
        wp_data->application_data, wp_data->application_data_len, &value,
        wp_data->object_type, wp_data->object_property, wp_data->array_index);
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                status = Loop_Present_Value_Set(
                    wp_data->object_instance, value.type.Real);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Loop_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_ACTION:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                status = Loop_Action_Set(
                    wp_data->object_instance, value.type.Enumerated);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_UPDATE_INTERVAL:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                status = Loop_Update_Interval_Set(
                    wp_data->object_instance, value.type.Unsigned_Int);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_OUTPUT_UNITS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Unsigned_Int <= UINT16_MAX) {
                    status = Loop_Output_Units_Set(
                        wp_data->object_instance, value.type.Enumerated);
                } else {
                    status = false;
                }
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_CONTROLLED_VARIABLE_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                status = Loop_Controlled_Variable_Value_Set(
                    wp_data->object_instance, value.type.Real);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_CONTROLLED_VARIABLE_UNITS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Unsigned_Int <= UINT16_MAX) {
                    status = Loop_Controlled_Variable_Units_Set(
                        wp_data->object_instance, value.type.Enumerated);
                } else {
                    status = false;
                }
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_PROPORTIONAL_CONSTANT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                status = Loop_Proportional_Constant_Set(
                    wp_data->object_instance, value.type.Real);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_PROPORTIONAL_CONSTANT_UNITS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Unsigned_Int <= UINT16_MAX) {
                    status = Loop_Proportional_Constant_Units_Set(
                        wp_data->object_instance, value.type.Enumerated);
                } else {
                    status = false;
                }
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_INTEGRAL_CONSTANT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                status = Loop_Integral_Constant_Set(
                    wp_data->object_instance, value.type.Real);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_INTEGRAL_CONSTANT_UNITS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Unsigned_Int <= UINT16_MAX) {
                    status = Loop_Integral_Constant_Units_Set(
                        wp_data->object_instance, value.type.Enumerated);
                } else {
                    status = false;
                }
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_DERIVATIVE_CONSTANT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                status = Loop_Derivative_Constant_Set(
                    wp_data->object_instance, value.type.Real);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_DERIVATIVE_CONSTANT_UNITS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Unsigned_Int <= UINT16_MAX) {
                    status = Loop_Derivative_Constant_Units_Set(
                        wp_data->object_instance, value.type.Enumerated);
                } else {
                    status = false;
                }
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_BIAS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                status =
                    Loop_Bias_Set(wp_data->object_instance, value.type.Real);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_SETPOINT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                status = Loop_Setpoint_Set(
                    wp_data->object_instance, value.type.Real);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_MINIMUM_OUTPUT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                status = Loop_Minimum_Output_Set(
                    wp_data->object_instance, value.type.Real);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_MAXIMUM_OUTPUT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                status = Loop_Maximum_Output_Set(
                    wp_data->object_instance, value.type.Real);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_PRIORITY_FOR_WRITING:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                if (value.type.Unsigned_Int <= UINT8_MAX) {
                    status = Loop_Priority_For_Writing_Set(
                        wp_data->object_instance, value.type.Unsigned_Int);
                } else {
                    status = false;
                }
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_MANIPULATED_VARIABLE_REFERENCE:
            status = write_property_type_valid(
                wp_data, &value,
                BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE);
            if (status) {
                status = Loop_Manipulated_Variable_Reference_Set(
                    wp_data->object_instance,
                    &value.type.Object_Property_Reference);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_CONTROLLED_VARIABLE_REFERENCE:
            status = write_property_type_valid(
                wp_data, &value,
                BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE);
            if (status) {
                status = Loop_Controlled_Variable_Reference_Set(
                    wp_data->object_instance,
                    &value.type.Object_Property_Reference);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_SETPOINT_REFERENCE:
            status = write_property_type_valid(
                wp_data, &value,
                BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE);
            if (status) {
                status = Loop_Setpoint_Reference_Set(
                    wp_data->object_instance,
                    &value.type.Object_Property_Reference);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_COV_INCREMENT:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_REAL);
            if (status) {
                status = Loop_COV_Increment_Set(
                    wp_data->object_instance, value.type.Real);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        default:
            if (Loop_Property_Lists_Member(wp_data->object_property)) {
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
 * @brief Set the context used with load, unload, run, halt, and restart
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void *Loop_Context_Get(uint32_t object_instance)
{
    struct object_data *pObject = Object_Data(object_instance);

    if (pObject) {
        return pObject->Context;
    }

    return NULL;
}

/**
 * @brief Set the context used for vendor specific extensions
 * @param object_instance [in] BACnet object instance number
 * @param context [in] pointer to the context
 */
void Loop_Context_Set(uint32_t object_instance, void *context)
{
    struct object_data *pObject;

    pObject = Object_Data(object_instance);
    if (pObject) {
        pObject->Context = context;
    }
}

/**
 * @brief Sets a callback used when the loop reads from BACnet Object
 *  reference value
 * @param cb - callback used to provide indications
 */
void Loop_Read_Property_Internal_Callback_Set(read_property_function cb)
{
    Read_Property_Internal_Callback = cb;
}

/**
 * @brief For a given object, reads a BACnet Object Property reference
 * @param value - application value
 * @param priority - BACnet priority 0=none,1..16
 * @return  true if values are within range and value is written.
 */
static bool Loop_Read_Variable_Reference_Update(
    const BACNET_OBJECT_PROPERTY_REFERENCE *reference, float *value)
{
    BACNET_READ_PROPERTY_DATA data = { 0 };
    uint8_t apdu[32] = { 0 };
    int apdu_len = 0, len = 0;
    bool status = false;

    if (!Object_Property_Reference_Empty(reference)) {
        data.object_type = reference->object_identifier.type;
        data.object_instance = reference->object_identifier.instance;
        data.object_property = reference->property_identifier;
        data.array_index = reference->property_array_index;
        data.application_data = apdu;
        data.application_data_len = sizeof(apdu);
        data.error_class = ERROR_CLASS_PROPERTY;
        data.error_code = ERROR_CODE_UNKNOWN_PROPERTY;
        if (Read_Property_Internal_Callback) {
            apdu_len = Read_Property_Internal_Callback(&data);
        }
        if (apdu_len > 0) {
            /* expecting only application tagged REAL values */
            len = bacnet_real_application_decode(apdu, apdu_len, value);
            if (len > 0) {
                status = true;
            }
        }
    }

    return status;
}

/**
 * @brief Sets a callback used when the loop is written from BACnet
 * @param cb - callback used to provide indications
 */
void Loop_Write_Property_Internal_Callback_Set(write_property_function cb)
{
    Write_Property_Internal_Callback = cb;
}

/**
 * @brief Add a Loop notification callback
 * @param notification - pointer to the notification structure
 */
void Loop_Write_Property_Notification_Add(
    struct loop_write_property_notification *notification)
{
    struct loop_write_property_notification *head;

    head = &Write_Property_Notification_Head;
    do {
        if (head->next == notification) {
            /* already here! */
            break;
        } else if (!head->next) {
            /* first available node */
            head->next = notification;
            break;
        }
        head = head->next;
    } while (head);
}

/**
 * @brief Calls all registered Loop write property notification callbacks
 * @param instance - object instance number
 * @param status - write property status
 * @param wp_data - write property data
 */
void Loop_Write_Property_Notify(
    uint32_t instance, bool status, BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    struct loop_write_property_notification *head;

    head = &Write_Property_Notification_Head;
    do {
        if (head->callback) {
            head->callback(instance, status, wp_data);
        }
        head = head->next;
    } while (head);
}

/**
 * @brief For a given object, writes to the manipulated-variable-reference
 * @param pObject - object instance data
 * @param object_instance - object-instance number of the object
 * @param value - application value
 * @param priority - BACnet priority 0=none,1..16
 * @return  true if values are within range and value is written.
 */
static bool Loop_Write_Manipulated_Variable(
    struct object_data *pObject,
    uint32_t object_instance,
    float value,
    uint8_t priority)
{
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    BACNET_OBJECT_PROPERTY_REFERENCE *member;
    bool status = false;

    if (pObject) {
        member = &pObject->Manipulated_Variable_Reference;
        if ((member->object_identifier.type == OBJECT_LOOP) &&
            (member->object_identifier.instance == object_instance)) {
            /* self - perform simulation by setting the controlled variable */
            pObject->Controlled_Variable_Value = value;
        } else if (!Object_Property_Reference_Empty(member)) {
            wp_data.object_type = member->object_identifier.type;
            wp_data.object_instance = member->object_identifier.instance;
            wp_data.object_property = member->property_identifier;
            wp_data.array_index = member->property_array_index;
            wp_data.error_class = ERROR_CLASS_PROPERTY;
            wp_data.error_code = ERROR_CODE_SUCCESS;
            wp_data.priority = priority;
            wp_data.application_data_len =
                encode_application_real(wp_data.application_data, value);
            if (Write_Property_Internal_Callback) {
                status = write_property_bacnet_array_valid(&wp_data);
                if (status) {
                    status = Write_Property_Internal_Callback(&wp_data);
                    if (status) {
                        wp_data.error_code = ERROR_CODE_SUCCESS;
                    }
                }
            }
            Loop_Write_Property_Notify(object_instance, status, &wp_data);
        }
    }

    return status;
}

/**
 * @brief PID algorithm
 * @param pObject - object instance data
 * @param elapsed_milliseconds - number of milliseconds elapsed
 * @return computed PID output value
 */
static float
Loop_PID_Algorithm(struct object_data *pObject, uint32_t elapsed_milliseconds)
{
    float output, error, elapsed_seconds;
    float integral_sum, integral_min, integral_max;
    float proportional, integral, derivative;

    if (elapsed_milliseconds == 0) {
        return pObject->Bias;
    }
    error = pObject->Setpoint - pObject->Controlled_Variable_Value;
    if (pObject->Action == ACTION_REVERSE) {
        /* In reverse action, an increase in the process variable
           above the setpoint requires a decrease in the controller output
           to bring the process variable back to the setpoint. */
        error = -error;
    }
    proportional = pObject->Proportional_Constant * error;
    elapsed_seconds = (float)elapsed_milliseconds;
    elapsed_seconds /= 1000.0f;
    integral_sum = error * elapsed_seconds;
    pObject->Integral_Sum += integral_sum;
    if (islessgreater(pObject->Integral_Constant, 0.0f)) {
        /* clamp integral sum to prevent windup */
        integral_max = pObject->Maximum_Output / pObject->Integral_Constant;
        if (isgreater(pObject->Integral_Sum, integral_max)) {
            pObject->Integral_Sum = integral_max;
        }
        integral_min = pObject->Minimum_Output / pObject->Integral_Constant;
        if (isless(pObject->Integral_Sum, integral_min)) {
            pObject->Integral_Sum = integral_min;
        }
    }
    integral = pObject->Integral_Constant * pObject->Integral_Sum;
    derivative = pObject->Derivative_Constant *
        ((error - pObject->Error) / elapsed_seconds);
    pObject->Error = error;
    output = proportional + integral + derivative + pObject->Bias;
    /* clamp the output within limits */
    if (isgreater(output, pObject->Maximum_Output)) {
        output = pObject->Maximum_Output;
    }
    if (isless(output, pObject->Minimum_Output)) {
        output = pObject->Minimum_Output;
    }

    return output;
}

/**
 * @brief Updates the object loop operation
 * @param object_instance - object-instance number of the object
 * @param elapsed_milliseconds - number of milliseconds elapsed
 */
void Loop_Timer(uint32_t object_instance, uint16_t elapsed_milliseconds)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        /* update any variable references */
        Loop_Read_Variable_Reference_Update(
            &pObject->Controlled_Variable_Reference,
            &pObject->Controlled_Variable_Value);
        Loop_Read_Variable_Reference_Update(
            &pObject->Setpoint_Reference, &pObject->Setpoint);
        /* loop algorithm updates the present-value */
        if (!pObject->Out_Of_Service) {
            /* When Out_Of_Service is TRUE:
                (a) the Present_Value property shall be
                    decoupled from the algorithm;
            */
            pObject->Present_Value =
                Loop_PID_Algorithm(pObject, elapsed_milliseconds);
        }
        if (pObject->Update_Interval) {
            pObject->Update_Timer += elapsed_milliseconds;
            /*  NOTE: No property that represents the interval at which
                the process variable is sampled or the algorithm is executed
                is part of this object.
                The Update_Interval value may be the same as these other values
                but could also be different depending on the algorithm utilized.
                The sampling or execution interval is a local matter and need
                not be represented as part of this object.*/
            if (pObject->Update_Timer >= pObject->Update_Interval) {
                pObject->Update_Timer -= pObject->Update_Interval;
                /*  The property referenced by Manipulated_Variable_Reference
                    and other functions that depend on the state of the
                    Present_Value or Reliability properties shall
                    respond to changes made to these properties,
                    as if those changes had been made by the algorithm.*/
                Loop_Write_Manipulated_Variable(
                    pObject, object_instance, pObject->Present_Value,
                    pObject->Priority_For_Writing);
            }
        }
    }
}

/**
 * @brief Creates a Loop object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Loop_Create(uint32_t object_instance)
{
    struct object_data *pObject = NULL;
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
    if (pObject) {
        /* already exists - signal success but don't change data */
        return object_instance;
    }
    pObject = calloc(1, sizeof(struct object_data));
    if (!pObject) {
        /* no RAM available - signal failure */
        return BACNET_MAX_INSTANCE;
    }
    index = Keylist_Data_Add(Object_List, object_instance, pObject);
    if (index < 0) {
        /* unable to add to list - signal failure */
        free(pObject);
        return BACNET_MAX_INSTANCE;
    }
    /* only need to set property values that are non-zero */
    pObject->Maximum_Output = 10.0f;
    pObject->Minimum_Output = 0.0f;
    pObject->Output_Units = UNITS_NO_UNITS;
    pObject->Controlled_Variable_Units = UNITS_NO_UNITS;
    pObject->Proportional_Constant = 1.0f;
    pObject->Proportional_Constant_Units = UNITS_NO_UNITS;
    pObject->Integral_Constant = 0.2f;
    pObject->Integral_Constant_Units = UNITS_NO_UNITS;
    pObject->Derivative_Constant = 0.05f;
    pObject->Derivative_Constant_Units = UNITS_NO_UNITS;
    pObject->Action = ACTION_DIRECT;
    pObject->Update_Interval = 1000;
    /* set the references to self */
    Object_Property_Reference_Set(
        &pObject->Manipulated_Variable_Reference, OBJECT_LOOP, object_instance,
        PROP_CONTROLLED_VARIABLE_VALUE, BACNET_ARRAY_ALL);
    Object_Property_Reference_Set(
        &pObject->Controlled_Variable_Reference, OBJECT_LOOP, object_instance,
        PROP_CONTROLLED_VARIABLE_VALUE, BACNET_ARRAY_ALL);
    Object_Property_Reference_Set(
        &pObject->Setpoint_Reference, OBJECT_LOOP, object_instance,
        PROP_SETPOINT, BACNET_ARRAY_ALL);
    pObject->Priority_For_Writing = BACNET_MAX_PRIORITY;
    pObject->COV_Increment = 1.0f;
    pObject->Description = NULL;
    pObject->Object_Name = NULL;
    pObject->Reliability = RELIABILITY_NO_FAULT_DETECTED;
    pObject->Out_Of_Service = false;
    pObject->Changed = false;
    pObject->Context = NULL;

    return object_instance;
}

/**
 * @brief Deletes an object-instance
 * @param object_instance - object-instance number of the object
 * @return true if the object-instance was deleted
 */
bool Loop_Delete(uint32_t object_instance)
{
    bool status = false;
    struct object_data *pObject =
        Keylist_Data_Delete(Object_List, object_instance);

    if (pObject) {
        free(pObject);
        status = true;
    }

    return status;
}

/**
 * @brief Deletes all the objects and their data
 */
void Loop_Cleanup(void)
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
 * @brief Returns the approximate size of each Loop object data
 */
size_t Loop_Size(void)
{
    return sizeof(struct object_data);
}

/**
 * Initializes the object data
 */
void Loop_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();
    }
}
