/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
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
*
*********************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacstr.h"
#include "bacenum.h"
#include "apdu.h"
#include "dlmstp.h"
#include "rs485.h"
#include "version.h"
/* objects */
#include "device.h"
#include "ai.h"
#include "av.h"
#include "bi.h"
#include "bv.h"
#include "wp.h"
#include "dcc.h"

/* note: you really only need to define variables for
   properties that are writable or that may change.
   The properties that are constant can be hard coded
   into the read-property encoding. */
static uint32_t Object_Instance_Number = 12345;
static char Object_Name[32] = "ARM7 Device";
static BACNET_DEVICE_STATUS System_Status = STATUS_OPERATIONAL;

BACNET_REINITIALIZED_STATE_OF_DEVICE Reinitialize_State =
    REINITIALIZED_STATE_IDLE;

void Device_Reinit(void)
{
    dcc_set_status_duration(COMMUNICATION_ENABLE, 0);
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
}

void Device_Init(void)
{
    Reinitialize_State = REINITIALIZED_STATE_IDLE;
    dcc_set_status_duration(COMMUNICATION_ENABLE, 0);
    /* FIXME: Get the data from the eeprom */
    /* I2C_Read_Block(EEPROM_DEVICE_ADDRESS,
       (char *)&Object_Instance_Number,
       sizeof(Object_Instance_Number),
       EEPROM_BACNET_ID_ADDR); */
}

/* methods to manipulate the data */
uint32_t Device_Object_Instance_Number(void)
{
    return Object_Instance_Number;
}

bool Device_Set_Object_Instance_Number(uint32_t object_id)
{
    bool status = true;         /* return value */

    if (object_id <= BACNET_MAX_INSTANCE) {
        Object_Instance_Number = object_id;
        /* FIXME: Write the data to the eeprom */
        /* I2C_Write_Block(
           EEPROM_DEVICE_ADDRESS,
           (char *)&Object_Instance_Number,
           sizeof(Object_Instance_Number),
           EEPROM_BACNET_ID_ADDR); */
    } else
        status = false;

    return status;
}

bool Device_Valid_Object_Instance_Number(uint32_t object_id)
{
    /* BACnet allows for a wildcard instance number */
    return ((Object_Instance_Number == object_id) ||
        (object_id == BACNET_MAX_INSTANCE));
}

BACNET_DEVICE_STATUS Device_System_Status(void)
{
    return System_Status;
}

void Device_Set_System_Status(BACNET_DEVICE_STATUS status)
{
    if (status < MAX_DEVICE_STATUS)
        System_Status = status;
}

uint16_t Device_Vendor_Identifier(void)
{
    return BACNET_VENDOR_ID;
}

uint8_t Device_Protocol_Version(void)
{
    return 1;
}

uint8_t Device_Protocol_Revision(void)
{
    return 5;
}

/* FIXME: MAX_APDU is defined in config.ini - set it! */
uint16_t Device_Max_APDU_Length_Accepted(void)
{
    return MAX_APDU;
}

BACNET_SEGMENTATION Device_Segmentation_Supported(void)
{
    return SEGMENTATION_NONE;
}

uint16_t Device_APDU_Timeout(void)
{
    return 60000;
}


uint8_t Device_Number_Of_APDU_Retries(void)
{
    return 0;
}

uint8_t Device_Database_Revision(void)
{
    return 0;
}

unsigned Device_Object_List_Count(void)
{
    unsigned count = 1;         /* at least 1 for device object */

    /* FIXME: add objects as needed */
    count += Binary_Input_Count();
    count += Binary_Value_Count();
    count += Analog_Input_Count();
    count += Analog_Value_Count();

    return count;
}

bool Device_Object_List_Identifier(unsigned array_index,
    int *object_type, uint32_t * instance)
{
    bool status = false;
    unsigned object_index = 0;
    unsigned object_count = 0;

    /* device object */
    if (array_index == 1) {
        *object_type = OBJECT_DEVICE;
        *instance = Object_Instance_Number;
        status = true;
    }
    /* normalize the index since
       we know it is not the previous objects */
    /* array index starts at 1 */
    object_index = array_index - 1;
    /* 1 for the device object */
    object_count = 1;
    /* FIXME: add objects as needed */
    /* binary value objects */
    if (!status) {
        object_index -= object_count;
        object_count = Binary_Value_Count();
        /* is it a valid index for this object? */
        if (object_index < object_count) {
            *object_type = OBJECT_BINARY_VALUE;
            *instance = Binary_Value_Index_To_Instance(object_index);
            status = true;
        }
    }
    /* analog input objects */
    if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index -= object_count;
        object_count = Analog_Value_Count();
        if (object_index < object_count) {
            *object_type = OBJECT_ANALOG_VALUE;
            *instance = Analog_Value_Index_To_Instance(object_index);
            status = true;
        }
    }
    /* analog input objects */
    if (!status) {
        /* array index starts at 1, and 1 for the device object */
        object_index -= object_count;
        object_count = Analog_Input_Count();
        if (object_index < object_count) {
            *object_type = OBJECT_ANALOG_INPUT;
            *instance = Analog_Input_Index_To_Instance(object_index);
            status = true;
        }
    }
    /* binary input objects */
    if (!status) {
        /* normalize the index since
           we know it is not the previous objects */
        object_index -= object_count;
        object_count = Binary_Input_Count();
        /* is it a valid index for this object? */
        if (object_index < object_count) {
            *object_type = OBJECT_BINARY_INPUT;
            *instance = Binary_Input_Index_To_Instance(object_index);
            status = true;
        }
    }

    return status;
}

/* return the length of the apdu encoded or -1 for error */
int Device_Encode_Property_APDU(uint8_t * apdu,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    BACNET_ERROR_CLASS * error_class, BACNET_ERROR_CODE * error_code)
{
    int apdu_len = 0;           /* return value */
    int len = 0;                /* apdu len intermediate value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned i = 0;
    int object_type = 0;
    uint32_t instance = 0;
    unsigned count = 0;
    BACNET_TIME local_time;
    BACNET_DATE local_date;

    /* FIXME: change the hardcoded names to suit your application */
    switch (property) {
    case PROP_OBJECT_IDENTIFIER:
        apdu_len = encode_tagged_object_id(&apdu[0], OBJECT_DEVICE,
            Object_Instance_Number);
        break;
    case PROP_OBJECT_NAME:
        characterstring_init_ansi(&char_string, Object_Name);
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_OBJECT_TYPE:
        apdu_len = encode_tagged_enumerated(&apdu[0], OBJECT_DEVICE);
        break;
    case PROP_DESCRIPTION:
        characterstring_init_ansi(&char_string, "BACnet Demo");
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_SYSTEM_STATUS:
        apdu_len =
            encode_tagged_enumerated(&apdu[0], Device_System_Status());
        break;
    case PROP_VENDOR_NAME:
        characterstring_init_ansi(&char_string, BACNET_VENDOR_NAME);
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_VENDOR_IDENTIFIER:
        apdu_len =
            encode_tagged_unsigned(&apdu[0], Device_Vendor_Identifier());
        break;
    case PROP_MODEL_NAME:
        characterstring_init_ansi(&char_string, "GNU Demo");
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_FIRMWARE_REVISION:
        characterstring_init_ansi(&char_string, BACnet_Version);
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_APPLICATION_SOFTWARE_VERSION:
        characterstring_init_ansi(&char_string, "1.0");
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_LOCATION:
        characterstring_init_ansi(&char_string, "USA");
        apdu_len = encode_tagged_character_string(&apdu[0], &char_string);
        break;
    case PROP_PROTOCOL_VERSION:
        apdu_len =
            encode_tagged_unsigned(&apdu[0], Device_Protocol_Version());
        break;
    case PROP_PROTOCOL_REVISION:
        apdu_len =
            encode_tagged_unsigned(&apdu[0], Device_Protocol_Revision());
        break;
        /* BACnet Legacy Support */
    case PROP_PROTOCOL_CONFORMANCE_CLASS:
        apdu_len = encode_tagged_unsigned(&apdu[0], 1);
        break;
    case PROP_PROTOCOL_SERVICES_SUPPORTED:
        /* Note: list of services that are executed, not initiated. */
        bitstring_init(&bit_string);
        for (i = 0; i < MAX_BACNET_SERVICES_SUPPORTED; i++) {
            /* automatic lookup based on handlers set */
            bitstring_set_bit(&bit_string, (uint8_t) i,
                apdu_service_supported(i));
        }
        apdu_len = encode_tagged_bitstring(&apdu[0], &bit_string);
        break;
    case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
        /* Note: this is the list of objects that can be in this device,
           not a list of objects that this device can access */
        bitstring_init(&bit_string);
        for (i = 0; i < MAX_ASHRAE_OBJECT_TYPE; i++) {
            /* FIXME: if ReadProperty used an array of Functions... */
            /* initialize all the object types to not-supported */
            bitstring_set_bit(&bit_string, (uint8_t) i, false);
        }
        /* FIXME: indicate the objects that YOU support */
        bitstring_set_bit(&bit_string, OBJECT_DEVICE, true);
        bitstring_set_bit(&bit_string, OBJECT_ANALOG_VALUE, true);
        bitstring_set_bit(&bit_string, OBJECT_BINARY_VALUE, true);
        bitstring_set_bit(&bit_string, OBJECT_ANALOG_INPUT, true);
        bitstring_set_bit(&bit_string, OBJECT_BINARY_INPUT, true);
        apdu_len = encode_tagged_bitstring(&apdu[0], &bit_string);
        break;
    case PROP_OBJECT_LIST:
        count = Device_Object_List_Count();
        /* Array element zero is the number of objects in the list */
        if (array_index == 0)
            apdu_len = encode_tagged_unsigned(&apdu[0], count);
        /* if no index was specified, then try to encode the entire list */
        /* into one packet.  Note that more than likely you will have */
        /* to return an error if the number of encoded objects exceeds */
        /* your maximum APDU size. */
        else if (array_index == BACNET_ARRAY_ALL) {
            for (i = 1; i <= count; i++) {
                if (Device_Object_List_Identifier(i, &object_type,
                        &instance)) {
                    len =
                        encode_tagged_object_id(&apdu[apdu_len],
                        object_type, instance);
                    apdu_len += len;
                    /* assume next one is the same size as this one */
                    /* can we all fit into the APDU? */
                    if ((apdu_len + len) >= MAX_APDU) {
                        *error_class = ERROR_CLASS_SERVICES;
                        *error_code = ERROR_CODE_NO_SPACE_FOR_OBJECT;
                        apdu_len = -1;
                        break;
                    }
                } else {
                    /* error: internal error? */
                    *error_class = ERROR_CLASS_SERVICES;
                    *error_code = ERROR_CODE_OTHER;
                    apdu_len = -1;
                    break;
                }
            }
        } else {
            if (Device_Object_List_Identifier(array_index, &object_type,
                    &instance))
                apdu_len =
                    encode_tagged_object_id(&apdu[0], object_type,
                    instance);
            else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = -1;
            }
        }
        break;
    case PROP_MAX_APDU_LENGTH_ACCEPTED:
        apdu_len = encode_tagged_unsigned(&apdu[0],
            Device_Max_APDU_Length_Accepted());
        break;
    case PROP_SEGMENTATION_SUPPORTED:
        apdu_len = encode_tagged_enumerated(&apdu[0],
            Device_Segmentation_Supported());
        break;
    case PROP_APDU_TIMEOUT:
        apdu_len = encode_tagged_unsigned(&apdu[0], Device_APDU_Timeout());
        break;
    case PROP_NUMBER_OF_APDU_RETRIES:
        apdu_len =
            encode_tagged_unsigned(&apdu[0],
            Device_Number_Of_APDU_Retries());
        break;
    case PROP_DEVICE_ADDRESS_BINDING:
        /* FIXME: encode the list here, if it exists */
        break;
    case PROP_DATABASE_REVISION:
        apdu_len =
            encode_tagged_unsigned(&apdu[0], Device_Database_Revision());
        break;
    case PROP_MAX_INFO_FRAMES:
        apdu_len =
            encode_tagged_unsigned(&apdu[0], dlmstp_max_info_frames());
        break;
    case PROP_MAX_MASTER:
        apdu_len = encode_tagged_unsigned(&apdu[0], dlmstp_max_master());
        break;
    case PROP_LOCAL_TIME:
        /* FIXME: if you support time */
        local_time.hour = 0;
        local_time.min = 0;
        local_time.sec = 0;
        local_time.hundredths = 0;
        apdu_len = encode_tagged_time(&apdu[0], &local_time);
        break;
    case PROP_UTC_OFFSET:
        /* Note: BACnet Time Zone is inverse of everybody else */
        apdu_len = encode_tagged_signed(&apdu[0], 5 /* EST */ );
        break;
    case PROP_LOCAL_DATE:
        /* FIXME: if you support date */
        local_date.year = 2006; /* AD */
        local_date.month = 4;   /* Jan=1..Dec=12 */
        local_date.day = 11;    /* 1..31 */
        local_date.wday = 0;    /* 1=Mon..7=Sun */
        apdu_len = encode_tagged_date(&apdu[0], &local_date);
        break;
    case PROP_DAYLIGHT_SAVINGS_STATUS:
        /* FIXME: if you support time/date */
        apdu_len = encode_tagged_boolean(&apdu[0], false);
        break;
    case 9600:
        apdu_len = encode_tagged_unsigned(&apdu[0], RS485_Get_Baud_Rate());
        break;
    default:
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
        apdu_len = -1;
        break;
    }

    return apdu_len;
}

bool Device_Write_Property(BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class, BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    if (!Device_Valid_Object_Instance_Number(wp_data->object_instance)) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    /* decode the some of the request */
    len = bacapp_decode_application_data(wp_data->application_data,
        wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    /* FIXME: len == 0: unable to decode? */
    switch (wp_data->object_property) {
    case PROP_OBJECT_IDENTIFIER:
        if (value.tag == BACNET_APPLICATION_TAG_OBJECT_ID) {
            if ((value.type.Object_Id.type == OBJECT_DEVICE) &&
                (Device_Set_Object_Instance_Number(value.type.
                        Object_Id.instance))) {
                /* we could send an I-Am broadcast to let the world know */
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;
    case PROP_MAX_INFO_FRAMES:
        if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
            if (value.type.Unsigned_Int <= 255) {
                dlmstp_set_max_info_frames(value.type.Unsigned_Int);
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;
    case PROP_MAX_MASTER:
        if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
            if ((value.type.Unsigned_Int > 0) &&
                (value.type.Unsigned_Int <= 127)) {
                dlmstp_set_max_master(value.type.Unsigned_Int);
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;
    case PROP_OBJECT_NAME:
        if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
            uint8_t encoding;
            size_t len;

            encoding =
                characterstring_encoding(&value.type.Character_String);
            len = characterstring_length(&value.type.Character_String);
            if (encoding == CHARACTER_ANSI_X34) {
                if (len <= 20) {
                    /* FIXME: set the name */
                    /* Display_Set_Name(
                       characterstring_value(&value.type.Character_String)); */
                    /* FIXME:  All the object names in a device must be unique.
                       Disallow setting the Device Object Name to any objects in
                       the device. */
                } else {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                }
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;
    case 9600:
        if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
            if (value.type.Unsigned_Int > 115200) {
                RS485_Set_Baud_Rate(value.type.Unsigned_Int);
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
        break;
    default:
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        break;
    }

    return status;
}
