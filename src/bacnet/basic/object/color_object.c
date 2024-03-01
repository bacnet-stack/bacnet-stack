/**
 * @file
 * @author Steve Karg
 * @date June 2022
 * @brief Color objects, customize for your use
 *
 * @section DESCRIPTION
 *
 * The Color object is an object with a present-value that
 * uses an x,y color single precision floating point data type.
 *
 * @section LICENSE
 *
 * Copyright (C) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacerror.h"
#include "bacnet/bacapp.h"
#include "bacnet/bactext.h"
#include "bacnet/cov.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/lighting.h"
#include "bacnet/reject.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/basic/sys/linear.h"
/* me! */
#include "bacnet/basic/object/color_object.h"

struct object_data {
    bool Changed : 1;
    bool Write_Enabled : 1;
    /* indicate the target color value for the color output */
    BACNET_XY_COLOR Present_Value;
    /* indicates the components of the object's actual color output */
    BACNET_XY_COLOR Tracking_Value;
    /* used to request specific behaviors */
    BACNET_COLOR_COMMAND Color_Command;
    /* indicates that there may be processes in the color object that may
        cause the Tracking_Value and Present_Value to differ temporarily. */
    BACNET_COLOR_OPERATION_IN_PROGRESS In_Progress;
    /* the color to be used for the color output when the device is restarted
       until such time as Present_Value or Color_Command are written */
    BACNET_XY_COLOR Default_Color;
    /* indicates the amount of time in milliseconds over which changes
       to the color output reflected in the Tracking_Value property */
    uint32_t Default_Fade_Time;
    /* The transition may be NONE or FADE. */
    BACNET_COLOR_TRANSITION Transition;
    const char *Object_Name;
    const char *Description;
};
/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
/* callback for present value writes */
static color_write_present_value_callback Color_Write_Present_Value_Callback;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Color_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_PRESENT_VALUE, PROP_TRACKING_VALUE,
    PROP_COLOR_COMMAND, PROP_IN_PROGRESS, PROP_DEFAULT_COLOR,
    PROP_DEFAULT_FADE_TIME, -1 };

static const int Color_Properties_Optional[] = { PROP_DESCRIPTION,
    PROP_TRANSITION, -1 };

static const int Color_Properties_Proprietary[] = { -1 };

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
void Color_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Color_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Color_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Color_Properties_Proprietary;
    }

    return;
}

/**
 * Determines if a given Color instance is valid
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if the instance is valid, and false if not
 */
bool Color_Valid_Instance(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        return true;
    }

    return false;
}

/**
 * Determines the number of Color objects
 *
 * @return  Number of Color objects
 */
unsigned Color_Count(void)
{
    return Keylist_Count(Object_List);
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Color objects where N is Color_Count().
 *
 * @param  index - 0..N where N is Color_Count()
 *
 * @return  object instance-number for the given index
 */
uint32_t Color_Index_To_Instance(unsigned index)
{
    KEY key = UINT32_MAX;

    Keylist_Index_Key(Object_List, index, &key);

    return key;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Color objects where N is Color_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or Color_Count()
 * if not valid.
 */
unsigned Color_Instance_To_Index(uint32_t object_instance)
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
bool Color_Present_Value(uint32_t object_instance, BACNET_XY_COLOR *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        xy_color_copy(value, &pObject->Present_Value);
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, sets the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - floating point Color
 *
 * @return  true if values are within range and present-value is set.
 */
bool Color_Present_Value_Set(uint32_t object_instance, BACNET_XY_COLOR *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        xy_color_copy(&pObject->Present_Value, value);
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, writes to the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Color_Present_Value_Write(uint32_t object_instance,
    BACNET_XY_COLOR *value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        (void)priority;
        xy_color_copy(&pObject->Present_Value, value);
        /* configure the color-command to perform the transition */
        if (pObject->Transition == BACNET_COLOR_TRANSITION_FADE) {
            pObject->Color_Command.transit.fade_time =
                pObject->Default_Fade_Time;
        } else {
            pObject->Color_Command.transit.fade_time = 0;
        }
        pObject->Color_Command.operation = BACNET_COLOR_OPERATION_FADE_TO_COLOR;
        xy_color_copy(&pObject->Color_Command.target.color, value);
        status = true;
    } else {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
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
bool Color_Tracking_Value(uint32_t object_instance, BACNET_XY_COLOR *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        xy_color_copy(value, &pObject->Tracking_Value);
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, sets the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - floating point Color
 *
 * @return  true if values are within range and present-value is set.
 */
bool Color_Tracking_Value_Set(uint32_t object_instance, BACNET_XY_COLOR *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        xy_color_copy(&pObject->Tracking_Value, value);
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, gets the property value
 *
 * @param object_instance - object-instance number of the object
 * @param value - color command data
 * @return  true if the property value is copied
 */
bool Color_Command(uint32_t object_instance, BACNET_COLOR_COMMAND *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject && value) {
        color_command_copy(value, &pObject->Color_Command);
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, sets the property value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - color command data
 * @return  true if values are within range and value is set.
 */
bool Color_Command_Set(uint32_t object_instance, BACNET_COLOR_COMMAND *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject && value) {
        color_command_copy(&pObject->Color_Command, value);
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, writes to the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Color_Command_Write(uint32_t object_instance,
    BACNET_COLOR_COMMAND *value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        (void)priority;
        if (pObject->Write_Enabled) {
            color_command_copy(&pObject->Color_Command, value);
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        }
    } else {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/**
 * For a given object instance-number, gets the property value
 *
 * @param object_instance - object-instance number of the object
 * @return property value
 */
BACNET_COLOR_OPERATION_IN_PROGRESS Color_In_Progress(uint32_t object_instance)
{
    BACNET_COLOR_OPERATION_IN_PROGRESS value =
        BACNET_COLOR_OPERATION_IN_PROGRESS_MAX;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->In_Progress;
    }

    return value;
}

/**
 * For a given object instance-number, sets the property value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACNET_COLOR_OPERATION_IN_PROGRESS
 * @return  true if values are within range and value is set.
 */
bool Color_In_Progress_Set(
    uint32_t object_instance, BACNET_COLOR_OPERATION_IN_PROGRESS value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value < BACNET_COLOR_OPERATION_IN_PROGRESS_MAX) {
            pObject->In_Progress = value;
            status = true;
        }
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
bool Color_Default_Color(uint32_t object_instance, BACNET_XY_COLOR *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        xy_color_copy(value, &pObject->Default_Color);
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, sets the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - floating point Color
 *
 * @return  true if values are within range and present-value is set.
 */
bool Color_Default_Color_Set(uint32_t object_instance, BACNET_XY_COLOR *value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        xy_color_copy(&pObject->Default_Color, value);
        status = true;
    }

    return status;
}

/**
 * Handle a WriteProperty to a specific property.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Color_Default_Color_Write(uint32_t object_instance,
    BACNET_XY_COLOR *value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        (void)priority;
        if (pObject->Write_Enabled) {
            xy_color_copy(&pObject->Default_Color, value);
            status = true;
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        }
    } else {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/**
 * For a given object instance-number, gets the property value
 *
 * @param object_instance - object-instance number of the object
 * @return the amount of time in milliseconds over which changes
 *  to the Color reflected in the Tracking_Value property
 */
uint32_t Color_Default_Fade_Time(uint32_t object_instance)
{
    uint32_t value = 0;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Default_Fade_Time;
    }

    return value;
}

/**
 * For a given object instance-number, sets the property value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - the amount of time in milliseconds over which changes
 *  to the Color reflected in the Tracking_Value property
 * @return  true if values are within range and value is set.
 */
bool Color_Default_Fade_Time_Set(uint32_t object_instance, uint32_t value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if ((value == 0) ||
            ((value >= BACNET_COLOR_FADE_TIME_MIN) &&
                (value <= BACNET_COLOR_FADE_TIME_MAX))) {
            pObject->Default_Fade_Time = value;
        }
        status = true;
    }

    return status;
}

/**
 * Handle a WriteProperty to a specific property.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Color_Default_Fade_Time_Write(uint32_t object_instance,
    BACNET_UNSIGNED_INTEGER value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        (void)priority;
        if (pObject->Write_Enabled) {
            if ((value == 0) ||
                ((value >= BACNET_COLOR_FADE_TIME_MIN) &&
                    (value <= BACNET_COLOR_FADE_TIME_MAX))) {
                pObject->Default_Fade_Time = value;
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        }
    } else {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
    }

    return status;
}

/**
 * For a given object instance-number, gets the property value
 *
 * @param object_instance - object-instance number of the object
 * @return property value
 */
BACNET_COLOR_TRANSITION Color_Transition(uint32_t object_instance)
{
    BACNET_COLOR_TRANSITION value = BACNET_COLOR_TRANSITION_NONE;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Transition;
    }

    return value;
}

/**
 * For a given object instance-number, sets the property value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACNET_COLOR_TRANSITION
 * @return  true if values are within range and value is set.
 */
bool Color_Transition_Set(
    uint32_t object_instance, BACNET_COLOR_TRANSITION value)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (value < BACNET_COLOR_TRANSITION_MAX) {
            pObject->Transition = value;
            status = true;
        }
    }

    return status;
}

/**
 * Handle a WriteProperty to a specific property.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - property value to be written
 * @param  priority - priority-array index value 1..16
 * @param  error_class - the BACnet error class
 * @param  error_code - BACnet Error code
 *
 * @return  true if values are within range and present-value is set.
 */
static bool Color_Transition_Write(uint32_t object_instance,
    uint32_t value,
    uint8_t priority,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        (void)priority;
        if (pObject->Write_Enabled) {
            if (value < BACNET_COLOR_TRANSITION_MAX) {
                pObject->Transition = value;
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        }
    } else {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
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
bool Color_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    struct object_data *pObject;
    char name_text[24] = "COLOR-4194303";

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Object_Name) {
            status =
                characterstring_init_ansi(object_name, pObject->Object_Name);
        } else {
            snprintf(name_text, sizeof(name_text), "COLOR-%u", object_instance);
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
bool Color_Name_Set(uint32_t object_instance, char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject && new_name) {
        pObject->Object_Name = new_name;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, returns the description
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return description text or NULL if not found
 */
char *Color_Description(uint32_t object_instance)
{
    char *name = NULL;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        if (pObject->Description) {
            name = (char *)pObject->Description;
        } else {
            name = "";
        }
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
bool Color_Description_Set(uint32_t object_instance, char *new_name)
{
    bool status = false; /* return value */
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject && new_name) {
        status = true;
        pObject->Description = new_name;
    }

    return status;
}

/**
 * Updates the color object tracking value while fading
 *
 * Transitioning from one color to another is supported by writing a
 * FADE_TO_COLOR command to the property Color_Command.
 * The current color is always indicated in the
 * Tracking_Value property. If a color command is
 * currently in progress and the Present_Value is written,
 * the color command shall be halted.
 *
 * @param  object_instance - object-instance number of the object
 * @param milliseconds - number of milliseconds elapsed
 */
static void Color_Fade_To_Color_Handler(
    uint32_t object_instance, uint16_t milliseconds)
{
    BACNET_XY_COLOR old_value;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (!pObject) {
        return;
    }
    xy_color_copy(&old_value, &pObject->Tracking_Value);
    if (milliseconds >= pObject->Color_Command.transit.fade_time) {
        /* stop fading */
        xy_color_copy(
            &pObject->Tracking_Value, &pObject->Color_Command.target.color);
        pObject->In_Progress = BACNET_COLOR_OPERATION_IN_PROGRESS_IDLE;
        pObject->Color_Command.operation = BACNET_COLOR_OPERATION_STOP;
        pObject->Color_Command.transit.fade_time = 0;
    } else {
        if (xy_color_same(&old_value, &pObject->Color_Command.target.color)) {
            /* stop fading */
            xy_color_copy(
                &pObject->Tracking_Value, &pObject->Color_Command.target.color);
            pObject->In_Progress = BACNET_COLOR_OPERATION_IN_PROGRESS_IDLE;
            pObject->Color_Command.operation = BACNET_COLOR_OPERATION_STOP;
            pObject->Color_Command.transit.fade_time = 0;
        } else {
            /* fading */
            pObject->Tracking_Value.x_coordinate = linear_interpolate(0,
                milliseconds, pObject->Color_Command.transit.fade_time,
                old_value.x_coordinate,
                pObject->Color_Command.target.color.x_coordinate);
            pObject->Tracking_Value.y_coordinate = linear_interpolate(0,
                milliseconds, pObject->Color_Command.transit.fade_time,
                old_value.y_coordinate,
                pObject->Color_Command.target.color.y_coordinate);
            pObject->Color_Command.transit.fade_time -= milliseconds;
            pObject->In_Progress =
                BACNET_COLOR_OPERATION_IN_PROGRESS_FADE_ACTIVE;
        }
    }
    if (Color_Write_Present_Value_Callback) {
        Color_Write_Present_Value_Callback(
            object_instance, &old_value, &pObject->Tracking_Value);
    }
}

/**
 * Updates the color object tracking value per ramp or fade
 *
 * @param  object_instance - object-instance number of the object
 * @param milliseconds - number of milliseconds elapsed
 */
void Color_Timer(uint32_t object_instance, uint16_t milliseconds)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        switch (pObject->Color_Command.operation) {
            case BACNET_COLOR_OPERATION_NONE:
                pObject->In_Progress = BACNET_COLOR_OPERATION_IN_PROGRESS_IDLE;
                break;
            case BACNET_COLOR_OPERATION_FADE_TO_COLOR:
                Color_Fade_To_Color_Handler(
                    object_instance, milliseconds);
                break;
            case BACNET_COLOR_OPERATION_STOP:
                pObject->In_Progress = BACNET_COLOR_OPERATION_IN_PROGRESS_IDLE;
                break;
            default:
                break;
        }
    }
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
int Color_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;
    BACNET_XY_COLOR color_value = { 0.0, 0.0 };
    BACNET_COLOR_COMMAND color_command = { 0 };

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }

    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], rpdata->object_type, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Color_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], rpdata->object_type);
            break;
        case PROP_PRESENT_VALUE:
            if (Color_Present_Value(rpdata->object_instance, &color_value)) {
                apdu_len = xy_color_encode(apdu, &color_value);
            }
            break;
        case PROP_TRACKING_VALUE:
            if (Color_Tracking_Value(rpdata->object_instance, &color_value)) {
                apdu_len = xy_color_encode(apdu, &color_value);
            }
            break;
        case PROP_COLOR_COMMAND:
            if (Color_Command(rpdata->object_instance, &color_command)) {
                apdu_len = color_command_encode(apdu, &color_command);
            }
            break;
        case PROP_IN_PROGRESS:
            apdu_len = encode_application_enumerated(
                apdu, Color_In_Progress(rpdata->object_instance));
            break;
        case PROP_DEFAULT_COLOR:
            if (Color_Default_Color(rpdata->object_instance, &color_value)) {
                apdu_len = xy_color_encode(apdu, &color_value);
            }
            break;
        case PROP_DEFAULT_FADE_TIME:
            apdu_len = encode_application_unsigned(
                apdu, Color_Default_Fade_Time(rpdata->object_instance));
            break;
        case PROP_TRANSITION:
            apdu_len = encode_application_enumerated(
                apdu, Color_Transition(rpdata->object_instance));
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string, Color_Description(rpdata->object_instance));
            apdu_len = encode_application_character_string(apdu, &char_string);
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_PRIORITY_ARRAY) &&
        (rpdata->object_property != PROP_EVENT_TIME_STAMPS) &&
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
bool Color_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    int apdu_size = 0;
    uint8_t *apdu = NULL;

    /* decode the some of the request */
    apdu = wp_data->application_data;
    apdu_size = wp_data->application_data_len;
    len = bacapp_decode_known_property(apdu, apdu_size, &value,
        wp_data->object_type, wp_data->object_property);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    if ((wp_data->object_property != PROP_PRIORITY_ARRAY) &&
        (wp_data->object_property != PROP_EVENT_TIME_STAMPS) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_XY_COLOR);
            if (status) {
                status = Color_Present_Value_Write(wp_data->object_instance,
                    &value.type.XY_Color, wp_data->priority,
                    &wp_data->error_class, &wp_data->error_code);
            }
            break;
        case PROP_COLOR_COMMAND:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_COLOR_COMMAND);
            if (status) {
                status = Color_Command_Write(wp_data->object_instance,
                    &value.type.Color_Command, wp_data->priority,
                    &wp_data->error_class, &wp_data->error_code);
            }
            break;
        case PROP_DEFAULT_COLOR:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_XY_COLOR);
            if (status) {
                status = Color_Default_Color_Write(wp_data->object_instance,
                    &value.type.XY_Color, wp_data->priority,
                    &wp_data->error_class, &wp_data->error_code);
            }
            break;
        case PROP_DEFAULT_FADE_TIME:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                status = Color_Default_Fade_Time_Write(wp_data->object_instance,
                    value.type.Unsigned_Int, wp_data->priority,
                    &wp_data->error_class, &wp_data->error_code);
            }
            break;
        case PROP_TRANSITION:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                status = Color_Transition_Write(wp_data->object_instance,
                    value.type.Enumerated, wp_data->priority,
                    &wp_data->error_class, &wp_data->error_code);
            }
            break;
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_TYPE:
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
        case PROP_TRACKING_VALUE:
        case PROP_IN_PROGRESS:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}

/**
 * @brief Sets a callback used when present-value is written from BACnet
 * @param cb - callback used to provide indications
 */
void Color_Write_Present_Value_Callback_Set(
    color_write_present_value_callback cb)
{
    Color_Write_Present_Value_Callback = cb;
}

/**
 * @brief Determines a object write-enabled flag state
 * @param object_instance - object-instance number of the object
 * @return  write-enabled status flag
 */
bool Color_Write_Enabled(uint32_t object_instance)
{
    bool value = false;
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        value = pObject->Write_Enabled;
    }

    return value;
}

/**
 * @brief For a given object instance-number, sets the write-enabled flag
 * @param object_instance - object-instance number of the object
 */
void Color_Write_Enable(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Write_Enabled = true;
    }
}

/**
 * @brief For a given object instance-number, clears the write-enabled flag
 * @param object_instance - object-instance number of the object
 */
void Color_Write_Disable(uint32_t object_instance)
{
    struct object_data *pObject;

    pObject = Keylist_Data(Object_List, object_instance);
    if (pObject) {
        pObject->Write_Enabled = false;
    }
}

/**
 * @brief Creates a Color object
 * @param object_instance - object-instance number of the object
 * @return the object-instance that was created, or BACNET_MAX_INSTANCE
 */
uint32_t Color_Create(uint32_t object_instance)
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
            pObject->Object_Name = NULL;
            /* color defaults */
            xy_color_set(&pObject->Present_Value, 0.0, 0.0);
            xy_color_set(&pObject->Tracking_Value, 0.0, 0.0);
            xy_color_set(&pObject->Default_Color, 1.0, 1.0);
            pObject->Default_Fade_Time = BACNET_COLOR_FADE_TIME_MIN;
            /* at powerup - fade to default color */
            xy_color_copy(
                &pObject->Color_Command.target.color, &pObject->Default_Color);
            pObject->Color_Command.operation =
                BACNET_COLOR_OPERATION_FADE_TO_COLOR;
            pObject->Color_Command.transit.fade_time =
                pObject->Default_Fade_Time;
            /* initialize all the status */
            pObject->In_Progress = BACNET_COLOR_OPERATION_IN_PROGRESS_IDLE;
            pObject->Transition = BACNET_COLOR_TRANSITION_FADE;
            pObject->Changed = false;
            pObject->Write_Enabled = false;
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
 * Deletes an Color object
 * @param object_instance - object-instance number of the object
 * @return true if the object is deleted
 */
bool Color_Delete(uint32_t object_instance)
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
 * Deletes all the Colors and their data
 */
void Color_Cleanup(void)
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
 * Initializes the Color object data
 */
void Color_Init(void)
{
    Object_List = Keylist_Create();
}
