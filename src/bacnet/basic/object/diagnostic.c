/**
 * @file
 * @author Edward Hague / edward@bac-test.com
 * @date 2023
 * @brief Diagnostic Object
 *
 * @section DESCRIPTION
 *
 * yada
 *
 * @section LICENSE
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
//#include <stdbool.h>
//#include <stdint.h>
//#include <stdio.h>
//#include <stddef.h>
//#include <stdlib.h>
//#include <string.h>
//#include "bacnet/config.h"
//#include "bacnet/basic/binding/address.h"
//#include "bacnet/bacdef.h"
//#include "bacnet/bacapp.h"
//#include "bacnet/bacint.h"
//#include "bacnet/bacdcode.h"
//#include "bacnet/npdu.h"
//#include "bacnet/apdu.h"
//#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/diagnostic.h"
#include "bacnet/readrange.h"


struct object_data {
    uint32_t Instance_Number;
    char *Object_Name;
    BACNET_RELIABILITY Reliability;
    bool Out_Of_Service : 1;
};

#define BACNET_DIAGNOSTIC_OBJECTS_MAX   1

static struct object_data Object_List[BACNET_DIAGNOSTIC_OBJECTS_MAX];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Diagnostic_Properties_Required[] = { 
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_TYPE,
    PROP_OBJECT_NAME, 
    PROP_STATUS_FLAGS, 
    PROP_RELIABILITY,
    PROP_OUT_OF_SERVICE, 
    -1 };

static const int Diagnostic_Properties_Optional[] = { -1 };

static const int Diagnostic_Properties_Proprietary[] = { -1 };

/**
 * Returns the list of required, optional, and proprietary properties.
 * Used by ReadPropertyMultiple service.
 *
 * @param object_instance - object-instance number of the object
 * @param pRequired - pointer to list of int terminated by -1, of
 * BACnet required properties for this object.
 * @param pOptional - pointer to list of int terminated by -1, of
 * BACnet optkional properties for this object.
 * @param pProprietary - pointer to list of int terminated by -1, of
 * BACnet proprietary properties for this object.
 */
void Diagnostic_Property_List(uint32_t object_instance,
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    unsigned index = 0;

    if (pRequired) {
        *pRequired = Diagnostic_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Diagnostic_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Diagnostic_Properties_Proprietary;
    }
}


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
void Diagnostic_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    Diagnostic_Property_List(
        Object_List[0].Instance_Number, pRequired, pOptional, pProprietary);
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
bool Diagnostic_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Diagnostic_Instance_To_Index(object_instance);
    if (index < BACNET_DIAGNOSTIC_OBJECTS_MAX) {
        status = characterstring_init_ansi(
            object_name, Object_List[index].Object_Name);
    }

    return status;
}

/**
 * For a given object instance-number, sets the object-name
 * Note that the object name must be unique within this device.
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be written
 *         Expecting a pointer to a static ANSI C string for zero copy.
 *
 * @return  true if object-name was set
 */
bool Diagnostic_Name_Set(uint32_t object_instance, char *new_name)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Diagnostic_Instance_To_Index(object_instance);
    if (index < BACNET_DIAGNOSTIC_OBJECTS_MAX) {
        Object_List[index].Object_Name = new_name;
    }

    return status;
}

/**
 * Determines if a given Network Port instance is valid
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if the instance is valid, and false if not
 */
bool Diagnostic_Valid_Instance(uint32_t object_instance)
{
    unsigned index = 0; /* offset from instance lookup */

    index = Diagnostic_Instance_To_Index(object_instance);
    if (index < BACNET_DIAGNOSTIC_OBJECTS_MAX) {
        return true;
    }

    return false;
}

/**
 * Determines the number of Network Port objects
 *
 * @return  Number of Network Port objects
 */
unsigned Diagnostic_Count(void)
{
    return BACNET_DIAGNOSTIC_OBJECTS_MAX;
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Network Port objects where N is Diagnostic_Count().
 *
 * @param index - 0..BACNET_NETWORK_PORTS_MAX value
 *
 * @return object instance-number for the given index, or BACNET_MAX_INSTANCE
 * for an invalid index.
 */
uint32_t Diagnostic_Index_To_Instance(unsigned index)
{
    if (index < BACNET_DIAGNOSTIC_OBJECTS_MAX) {
        return Object_List[index].Instance_Number;
    }

    return BACNET_MAX_INSTANCE;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Network Port objects where N is Diagnostic_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or BACNET_NETWORK_PORTS_MAX
 * if not valid.
 */
unsigned Diagnostic_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = 0;

    for (index = 0; index < BACNET_DIAGNOSTIC_OBJECTS_MAX; index++) {
        if (Object_List[index].Instance_Number == object_instance) {
            return index;
        }
    }

    return BACNET_DIAGNOSTIC_OBJECTS_MAX;
}

/**
 * For the Network Port object, set the instance number.
 *
 * @param  index - 0..BACNET_NETWORK_PORTS_MAX value
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if values are within range and property is set.
 */
bool Diagnostic_Object_Instance_Number_Set(
    unsigned index, uint32_t object_instance)
{
    bool status = false; /* return value */

    if (index < BACNET_DIAGNOSTIC_OBJECTS_MAX) {
        if (object_instance <= BACNET_MAX_INSTANCE) {
            Object_List[index].Instance_Number = object_instance;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, returns the out-of-service
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  out-of-service property value
 */
bool Diagnostic_Out_Of_Service(uint32_t object_instance)
{
    bool oos_flag = false;
    unsigned index = 0;

    index = Diagnostic_Instance_To_Index(object_instance);
    if (index < BACNET_DIAGNOSTIC_OBJECTS_MAX) {
        oos_flag = Object_List[index].Out_Of_Service;
    }

    return oos_flag;
}

/**
 * For a given object instance-number, sets the out-of-service property value
 *
 * @param object_instance - object-instance number of the object
 * @param value - boolean out-of-service value
 *
 * @return true if the out-of-service property value was set
 */
bool Diagnostic_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    bool status = false;
    unsigned index = 0;

    index = Diagnostic_Instance_To_Index(object_instance);
    if (index < BACNET_DIAGNOSTIC_OBJECTS_MAX) {
        Object_List[index].Out_Of_Service = value;
        status = true;
    }

    return status;
}


/**
 * For a given object instance-number, gets the reliability.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return reliability value
 */
BACNET_RELIABILITY Diagnostic_Reliability(uint32_t object_instance)
{
    BACNET_RELIABILITY reliability = RELIABILITY_NO_FAULT_DETECTED;
    unsigned index = 0;

    index = Diagnostic_Instance_To_Index(object_instance);
    if (index < BACNET_DIAGNOSTIC_OBJECTS_MAX) {
        reliability = Object_List[index].Reliability;
    }

    return reliability;
}


/**
 * For a given object instance-number, sets the reliability
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - reliability enumerated value
 *
 * @return  true if values are within range and property is set.
 */
bool Diagnostic_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value)
{
    bool status = false;
    unsigned index = 0;

    index = Diagnostic_Instance_To_Index(object_instance);
    if (index < BACNET_DIAGNOSTIC_OBJECTS_MAX) {
        Object_List[index].Reliability = value;
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
int Diagnostic_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0;
    int apdu_size ;
    BACNET_BIT_STRING bit_string;
//    BACNET_OCTET_STRING octet_string;
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu;
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    Diagnostic_Property_List(
        rpdata->object_instance, &pRequired, &pOptional, &pProprietary);
    if ((!property_list_member(pRequired, rpdata->object_property)) &&
        (!property_list_member(pOptional, rpdata->object_property)) &&
        (!property_list_member(pProprietary, rpdata->object_property))) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
        return BACNET_STATUS_ERROR;
    }

    apdu = rpdata->application_data;
    apdu_size = rpdata->application_data_len;

    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_NETWORK_PORT, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Diagnostic_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_NETWORK_PORT);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            if (Diagnostic_Reliability(rpdata->object_instance) ==
                RELIABILITY_NO_FAULT_DETECTED) {
                bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            } else {
                bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, true);
            }
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            if (Diagnostic_Out_Of_Service(rpdata->object_instance)) {
                bitstring_set_bit(
                    &bit_string, STATUS_FLAG_OUT_OF_SERVICE, true);
            } else {
                bitstring_set_bit(
                    &bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            }
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_RELIABILITY:
            apdu_len = encode_application_enumerated(
                &apdu[0], Diagnostic_Reliability(rpdata->object_instance));
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(
                &apdu[0], Diagnostic_Out_Of_Service(rpdata->object_instance));
            break;

        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            (void)apdu_size;
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
bool Diagnostic_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    if (!Diagnostic_Valid_Instance(wp_data->object_instance)) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    /* decode the some of the request */
    len = bacapp_decode_application_data(
        wp_data->application_data, wp_data->application_data_len, &value);
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    if ((wp_data->object_property != PROP_LINK_SPEEDS) &&
        (wp_data->object_property != PROP_IP_DNS_SERVER) &&
        (wp_data->object_property != PROP_IPV6_DNS_SERVER) &&
        (wp_data->object_property != PROP_EVENT_MESSAGE_TEXTS) &&
        (wp_data->object_property != PROP_EVENT_MESSAGE_TEXTS_CONFIG) &&
        (wp_data->object_property != PROP_TAGS) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    /* FIXME: len < application_data_len: more data? */
    switch (wp_data->object_property) 
    {
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_RELIABILITY:
        case PROP_OUT_OF_SERVICE:
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
 * ReadRange service handler
 *
 * @param  apdu - place to encode the data
 * @param  apdu - BACNET_READ_RANGE_DATA data
 *
 * @return number of bytes encoded
 */
int Diagnostic_Read_Range_XXX(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest)
{
    (void)apdu;
    (void)pRequest;
    return 0;
}

bool Diagnostic_Read_Range(
    BACNET_READ_RANGE_DATA *pRequest, RR_PROP_INFO *pInfo)
{
    /* return value */
    bool status = false;

    switch (pRequest->object_property) {
        /* required properties */
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_RELIABILITY:
        case PROP_OUT_OF_SERVICE:
            pRequest->error_class = ERROR_CLASS_SERVICES;
            pRequest->error_code = ERROR_CODE_PROPERTY_IS_NOT_A_LIST;
            break;
        case PROP_BBMD_FOREIGN_DEVICE_TABLE:
            pInfo->RequestTypes = RR_BY_POSITION;
            pInfo->Handler = Diagnostic_Read_Range_XXX;
            status = true;
            break;
        default:
            pRequest->error_class = ERROR_CLASS_PROPERTY;
            pRequest->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}


/**
 * Initializes the Diagnostic Object data
 */
void Diagnostic_Init(void)
{
    /* do something interesting */
}
