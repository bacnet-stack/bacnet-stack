/**
 * @file
 * @author Nikola Jelic
 * @date 2014
 * @brief Command objects, customize for your use
 *
 * @section DESCRIPTION
 *
 * The Command object type defines a standardized object whose
 * properties represent the externally visible characteristics of a
 * multi-action command procedure. A Command object is used to
 * write a set of values to a group of object properties, based on
 * the "action code" that is written to the Present_Value of the
 * Command object. Whenever the Present_Value property of the
 * Command object is written to, it triggers the Command object
 * to take a set of actions that change the values of a set of other
 * objects' properties.
 *
 * @section LICENSE
 *
 * Copyright (C) 2014 Nikola Jelic <nikola.jelic@euroicc.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bactext.h"
#include "bacnet/config.h" /* the custom stuff */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/lighting.h"
#include "bacnet/proplist.h"
#include "bacnet/timestamp.h"
#include "bacnet/basic/object/command.h"

/*BACnetActionCommand ::= SEQUENCE {
deviceIdentifier [0] BACnetObjectIdentifier OPTIONAL,
objectIdentifier [1] BACnetObjectIdentifier,
propertyIdentifier [2] BACnetPropertyIdentifier,
propertyArrayIndex [3] Unsigned OPTIONAL, --used only with array datatype
propertyValue [4] ABSTRACT-SYNTAX.&Type,
priority [5] Unsigned (1..16) OPTIONAL, --used only when property is commandable
postDelay [6] Unsigned OPTIONAL,
quitOnFailure [7] BOOLEAN,
writeSuccessful [8] BOOLEAN
}*/

int cl_encode_apdu(uint8_t *apdu, BACNET_ACTION_LIST *bcl)
{
    int len = 0;
    int apdu_len = 0;

    if (bcl->Device_Id.instance <= BACNET_MAX_INSTANCE) {
        len = encode_context_object_id(
            &apdu[apdu_len], 0, bcl->Device_Id.type, bcl->Device_Id.instance);
        if (len < 0) {
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;
    }
    /* TODO: Check for object type and instance limits */
    len = encode_context_object_id(
        &apdu[apdu_len], 1, bcl->Object_Id.type, bcl->Object_Id.instance);
    if (len < 0) {
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    len =
        encode_context_enumerated(&apdu[apdu_len], 2, bcl->Property_Identifier);
    if (len < 0) {
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    if (bcl->Property_Array_Index != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(
            &apdu[apdu_len], 3, bcl->Property_Array_Index);
        if (len < 0) {
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;
    }

    /* 	BACnet Testing Observed Incident oi00108
            Command Action not correctly formatted
            Revealed by BACnet Test Client v1.8.16 (
       www.bac-test.com/bacnet-test-client-download ) BITS: BIT00031 BC
       135.1: 9.20.1.7 BC 135.1: 9.20.1.9 Any discussions can be directed to
       edward@bac-test.com Please feel free to remove this comment when my
       changes have been reviewed by all interested parties. Say 6 months ->
       September 2016 */

    len = encode_opening_tag(&apdu[apdu_len], 4);
    if (len < 0) {
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    len = bacapp_encode_application_data(&apdu[apdu_len], &bcl->Value);
    if (len < 0) {
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 4);
    if (len < 0) {
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;

    if (bcl->Priority != BACNET_NO_PRIORITY) {
        len = encode_context_unsigned(&apdu[apdu_len], 5, bcl->Priority);
        if (len < 0) {
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;
    }
    if (bcl->Post_Delay != 0xFFFFFFFFU) {
        len = encode_context_unsigned(&apdu[apdu_len], 6, bcl->Post_Delay);
        if (len < 0) {
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;
    }
    len = encode_context_boolean(&apdu[apdu_len], 7, bcl->Quit_On_Failure);
    if (len < 0) {
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    len = encode_context_boolean(&apdu[apdu_len], 8, bcl->Write_Successful);
    if (len < 0) {
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;

    return apdu_len;
}

int cl_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    BACNET_APPLICATION_TAG tag,
    BACNET_ACTION_LIST *bcl)
{
    int len = 0;
    int dec_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    uint32_t enum_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    if (decode_is_context_tag(&apdu[dec_len], 0)) {
        /* Tag 0: Device ID */
        dec_len++;
        len = decode_object_id(
            &apdu[dec_len], &bcl->Device_Id.type, &bcl->Device_Id.instance);
        if (len < 0) {
            return BACNET_STATUS_REJECT;
        }
        dec_len += len;
    }
    if (!decode_is_context_tag(&apdu[dec_len++], 1)) {
        return BACNET_STATUS_REJECT;
    }
    len = decode_object_id(
        &apdu[dec_len], &bcl->Object_Id.type, &bcl->Object_Id.instance);
    if (len < 0) {
        return BACNET_STATUS_REJECT;
    }
    dec_len += len;
    len = decode_tag_number_and_value(
        &apdu[dec_len], &tag_number, &len_value_type);
    if (len < 0) {
        return BACNET_STATUS_REJECT;
    }
    dec_len += len;
    if (tag_number != 2) {
        return BACNET_STATUS_REJECT;
    }
    len = decode_enumerated(&apdu[dec_len], len_value_type, &enum_value);
    if (len < 0) {
        return BACNET_STATUS_REJECT;
    }
    bcl->Property_Identifier = enum_value;
    dec_len += len;
    if (decode_is_context_tag(&apdu[dec_len], 3)) {
        len = decode_tag_number_and_value(
            &apdu[dec_len], &tag_number, &len_value_type);
        dec_len += len;
        len = decode_unsigned(&apdu[dec_len], len_value_type, &unsigned_value);
        if (len < 0) {
            return BACNET_STATUS_REJECT;
        }
        bcl->Property_Array_Index = unsigned_value;
        dec_len += len;
    } else {
        bcl->Property_Array_Index = BACNET_ARRAY_ALL;
    }
    if (!decode_is_context_tag(&apdu[dec_len], 4)) {
        return BACNET_STATUS_REJECT;
    }
    bcl->Value.context_specific = true;
    bcl->Value.context_tag = 4;
    bcl->Value.tag = tag;
    switch (tag) {
        case BACNET_APPLICATION_TAG_NULL:
            len = 1;
            break;
        case BACNET_APPLICATION_TAG_BOOLEAN:
            len = decode_context_boolean2(
                &apdu[dec_len], 4, &bcl->Value.type.Boolean);
            if (len < 0) {
                return BACNET_STATUS_REJECT;
            }
            break;
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            len = decode_context_unsigned(&apdu[dec_len], 4, &unsigned_value);
            if (len < 0) {
                return BACNET_STATUS_REJECT;
            }
            bcl->Value.type.Unsigned_Int = unsigned_value;
            break;
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            len = decode_context_signed(
                &apdu[dec_len], 4, &bcl->Value.type.Signed_Int);
            break;
        case BACNET_APPLICATION_TAG_REAL:
            len = decode_context_real(&apdu[dec_len], 4, &bcl->Value.type.Real);
            break;
        case BACNET_APPLICATION_TAG_DOUBLE:
            len = decode_context_double(
                &apdu[dec_len], 4, &bcl->Value.type.Double);
            break;
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            len = decode_context_octet_string(
                &apdu[dec_len], 4, &bcl->Value.type.Octet_String);
            break;
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            len = decode_context_character_string(
                &apdu[dec_len], 4, &bcl->Value.type.Character_String);
            break;
        case BACNET_APPLICATION_TAG_BIT_STRING:
            len = decode_context_bitstring(
                &apdu[dec_len], 4, &bcl->Value.type.Bit_String);
            if (len < 0) {
                return BACNET_STATUS_REJECT;
            }
            break;
        case BACNET_APPLICATION_TAG_ENUMERATED:
            len = decode_context_enumerated(
                &apdu[dec_len], 4, &bcl->Value.type.Enumerated);
            break;
        case BACNET_APPLICATION_TAG_DATE:
            len = decode_context_date(&apdu[dec_len], 4, &bcl->Value.type.Date);
            break;
        case BACNET_APPLICATION_TAG_TIME:
            len = decode_context_bacnet_time(
                &apdu[dec_len], 4, &bcl->Value.type.Time);
            break;
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            len = decode_context_object_id(&apdu[dec_len], 4,
                &bcl->Value.type.Object_Id.type,
                &bcl->Value.type.Object_Id.instance);
            break;
#if defined(BACAPP_TYPES_EXTRA)
        case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
            len = lighting_command_decode(&apdu[dec_len], apdu_len - dec_len,
                &bcl->Value.type.Lighting_Command);
            break;
#endif
        default:
            return BACNET_STATUS_REJECT;
    }
    if (len > 0) {
        dec_len += len;
    }
    if (decode_is_context_tag(&apdu[dec_len], 5)) {
        len = decode_tag_number_and_value(
            &apdu[dec_len], &tag_number, &len_value_type);
        dec_len += len;
        len = decode_unsigned(&apdu[dec_len], len_value_type, &unsigned_value);
        if (len < 0) {
            return BACNET_STATUS_REJECT;
        }
        bcl->Priority = (uint8_t)unsigned_value;
        dec_len += len;
    } else {
        bcl->Priority = BACNET_NO_PRIORITY;
    }
    if (decode_is_context_tag(&apdu[dec_len], 6)) {
        len = decode_tag_number_and_value(
            &apdu[dec_len], &tag_number, &len_value_type);
        dec_len += len;
        len = decode_unsigned(&apdu[dec_len], len_value_type, &unsigned_value);
        if (len < 0) {
            return BACNET_STATUS_REJECT;
        }
        bcl->Post_Delay = unsigned_value;
        dec_len += len;
    } else {
        bcl->Post_Delay = 0xFFFFFFFFU;
    }
    if (!decode_is_context_tag(&apdu[dec_len], 7)) {
        return BACNET_STATUS_REJECT;
    }
    len = decode_context_boolean2(&apdu[dec_len], 7, &bcl->Quit_On_Failure);
    if (len < 0) {
        return BACNET_STATUS_REJECT;
    }
    dec_len += len;
    if (!decode_is_context_tag(&apdu[dec_len], 8)) {
        return BACNET_STATUS_REJECT;
    }
    len = decode_context_boolean2(&apdu[dec_len], 8, &bcl->Write_Successful);
    if (len < 0) {
        return BACNET_STATUS_REJECT;
    }
    dec_len += len;
    if (dec_len < apdu_len) {
        return BACNET_STATUS_REJECT;
    }

    return dec_len;
}

COMMAND_DESCR Command_Descr[MAX_COMMANDS];

/* These arrays are used by the ReadPropertyMultiple handler */
static const int Command_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_PRESENT_VALUE, PROP_IN_PROCESS,
    PROP_ALL_WRITES_SUCCESSFUL, PROP_ACTION, -1 };

static const int Command_Properties_Optional[] = { PROP_DESCRIPTION, -1 };

static const int Command_Properties_Proprietary[] = { -1 };

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
void Command_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Command_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Command_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Command_Properties_Proprietary;
    }

    return;
}

/**
 * @brief Determine if the object property is a member of this object instance
 * @param object_instance - object-instance number of the object
 * @param object_property - object-property to be checked
 * @return true if the property is a member of this object instance
 */
static bool Property_List_Member(
    uint32_t object_instance, int object_property)
{
    bool found = false;
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;

    (void)object_instance;
    Command_Property_Lists(
        &pRequired, &pOptional, &pProprietary);
    found = property_list_member(pRequired, object_property);
    if (!found) {
        found = property_list_member(pOptional, object_property);
    }
    if (!found) {
        found = property_list_member(pProprietary, object_property);
    }

    return found;
}

/**
 * Initializes the Command object data
 */
void Command_Init(void)
{
    unsigned i;
    for (i = 0; i < MAX_COMMANDS; i++) {
        Command_Descr[i].Present_Value = 0;
        Command_Descr[i].In_Process = false;
        Command_Descr[i].All_Writes_Successful = true; /* Optimistic default */
    }
}

/**
 * Determines if a given object instance is valid
 *
 * @param object_instance - object-instance number of the object
 *
 * @return true if the instance is valid, and false if not
 */
bool Command_Valid_Instance(uint32_t object_instance)
{
    unsigned int index;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        return true;
    }

    return false;
}

/**
 * Determines the number of this object instances
 *
 * @return Number of objects
 */
unsigned Command_Count(void)
{
    return MAX_COMMANDS;
}

/**
 * Determines the object instance-number for a given 0..N index
 * of objects where N is the total number of this object instances.
 *
 * @param index - 0..total number of this object instances
 *
 * @return object instance-number for the given index
 */
uint32_t Command_Index_To_Instance(unsigned index)
{
    return index;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of this object where N is the total number of this object instances
 *
 * @param object_instance - object-instance number of the object.
 *
 * @return index for the given instance-number, or
 * the total number of this object instances if not valid.
 */
unsigned Command_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_COMMANDS;

    if (object_instance < MAX_COMMANDS) {
        index = object_instance;
    }

    return index;
}

/**
 * For a given object instance-number, determines the present-value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  present-value of the object
 */
uint32_t Command_Present_Value(uint32_t object_instance)
{
    uint32_t value = 0;
    unsigned int index;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        value = Command_Descr[index].Present_Value;
    }

    return value;
}

/**
 * For a given object instance-number, sets the present-value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - present-value to set
 *
 * @return  true if values are within range and present-value is set.
 */
bool Command_Present_Value_Set(uint32_t object_instance, uint32_t value)
{
    bool status = false;
    unsigned int index;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        Command_Descr[index].Present_Value = value;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, determines if the command
 * is in-process.  This true value indicates that the Command object has
 * begun processing one of a set of action sequences. Once all of the
 * writes have been attempted by the Command object, the In_Process
 * property shall be set back to false.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if this object-instance is in-process.
 */
bool Command_In_Process(uint32_t object_instance)
{
    bool value = false;
    unsigned int index;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        value = Command_Descr[index].In_Process;
    }

    return value;
}

/**
 * For a given object instance-number, sets the command in-process value.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - true or false value to set
 *
 * @return  true if values are within range and in-process flag is set.
 */
bool Command_In_Process_Set(uint32_t object_instance, bool value)
{
    bool status = false;
    unsigned int index;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        Command_Descr[index].In_Process = value;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, indicates the success or failure
 * of the sequence of actions that are triggered when the Present_Value
 * property is written to.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if all writes were successful for this object-instance
 */
bool Command_All_Writes_Successful(uint32_t object_instance)
{
    bool value = false;
    unsigned int index;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        value = Command_Descr[index].All_Writes_Successful;
    }

    return value;
}

/**
 * For a given object instance-number, sets the all-writes-successful value.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - true or false value to set
 *
 * @return  true if values are within range and all-writes-succcessful is set.
 */
bool Command_All_Writes_Successful_Set(uint32_t object_instance, bool value)
{
    bool status = false;
    unsigned int index;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        Command_Descr[index].All_Writes_Successful = value;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, loads the object-name into
 * a characterstring.
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - holds the object-name retrieved
 *
 * @return  true if object-name was retrieved
 */
bool Command_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32] = ""; /* okay for single thread */
    unsigned int index;
    bool status = false;

    index = Command_Instance_To_Index(object_instance);
    if (index < MAX_COMMANDS) {
        sprintf(text_string, "COMMAND %lu", (unsigned long)index);
        status = characterstring_init_ansi(object_name, text_string);
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
int Command_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    int len = 0;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index = 0;
    uint8_t *apdu = NULL;
    uint16_t apdu_max = 0;
    COMMAND_DESCR *CurrentCommand;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu_max = rpdata->application_data_len;
    object_index = Command_Instance_To_Index(rpdata->object_instance);
    if (object_index < MAX_COMMANDS) {
        CurrentCommand = &Command_Descr[object_index];
    } else {
        return false;
    }

    apdu = rpdata->application_data;
    switch ((int)rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_COMMAND, rpdata->object_instance);
            break;

        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Command_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;

        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_COMMAND);
            break;

        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_unsigned(
                &apdu[0], Command_Present_Value(rpdata->object_instance));
            break;
        case PROP_IN_PROCESS:
            apdu_len = encode_application_boolean(
                &apdu[0], Command_In_Process(rpdata->object_instance));
            break;
        case PROP_ALL_WRITES_SUCCESSFUL:
            apdu_len = encode_application_boolean(&apdu[0],
                Command_All_Writes_Successful(rpdata->object_instance));
            break;
        case PROP_ACTION:
            /* TODO */
            if (rpdata->array_index == 0) {
                apdu_len =
                    encode_application_unsigned(&apdu[0], MAX_COMMAND_ACTIONS);
            } else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                int i;
                for (i = 0; i < MAX_COMMAND_ACTIONS; i++) {
                    BACNET_ACTION_LIST *Curr_CL_Member =
                        &CurrentCommand->Action[0];
                    /* another loop, for additional actions in the list */
                    for (; Curr_CL_Member != NULL;
                         Curr_CL_Member = Curr_CL_Member->next) {
                        len = cl_encode_apdu(
                            &apdu[apdu_len], &CurrentCommand->Action[0]);
                        apdu_len += len;
                        /* assume the next one is of the same length, which need
                         * not be the case */
                        if ((i != MAX_COMMAND_ACTIONS - 1) &&
                            (apdu_len + len) >= apdu_max) {
                            rpdata->error_code =
                                ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                            apdu_len = BACNET_STATUS_ABORT;
                            break;
                        }
                    }
                }
            } else {
                if (rpdata->array_index < MAX_COMMAND_ACTIONS) {
                    BACNET_ACTION_LIST *Curr_CL_Member =
                        &CurrentCommand->Action[rpdata->array_index];
                    /* another loop, for additional actions in the list */
                    for (; Curr_CL_Member != NULL;
                         Curr_CL_Member = Curr_CL_Member->next) {
                        len = cl_encode_apdu(
                            &apdu[apdu_len], &CurrentCommand->Action[0]);
                        apdu_len += len;
                        /* assume the next one is of the same length, which need
                         * not be the case */
                        if ((apdu_len + len) >= apdu_max) {
                            rpdata->error_code =
                                ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                            apdu_len = BACNET_STATUS_ABORT;
                            break;
                        }
                    }
                } else {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_ACTION) &&
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
bool Command_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    unsigned int object_index = 0;
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
    if ((wp_data->object_property != PROP_ACTION) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    object_index = Command_Instance_To_Index(wp_data->object_instance);
    if (object_index >= MAX_COMMANDS) {
        return false;
    }

    switch ((int)wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                if (value.type.Unsigned_Int >= MAX_COMMAND_ACTIONS) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    return false;
                }
                Command_Present_Value_Set(
                    wp_data->object_instance, value.type.Unsigned_Int);
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                status = false;
            }
            break;
        default:
            if (Property_List_Member(
                    wp_data->object_instance, wp_data->object_property)) {
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

void Command_Intrinsic_Reporting(uint32_t object_instance)
{
    (void)object_instance;
}
