/**
 * @brief This module manages the BACnet Device object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacenum.h"
#include "bacnet/apdu.h"
#include "bacnet/dcc.h"
#include "bacnet/datalink/dlmstp.h"
#include "rs485.h"
#include "bacnet/version.h"
/* objects */
#include "bacnet/basic/services.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bv.h"
#include "bacnet/wp.h"

/* note: you really only need to define variables for
   properties that are writable or that may change.
   The properties that are constant can be hard coded
   into the read-property encoding. */
static uint32_t Object_Instance_Number = 260001;
static char Object_Name[30] = "AVR Device";
static BACNET_DEVICE_STATUS System_Status = STATUS_OPERATIONAL;
static const char *Model_Name = "ATmega328 Uno R3 Device";

bool Device_Object_Name_ANSI_Init(const char *object_name)
{
    bool status = false; /* return value */

    if (object_name) {
        status = true;
        strncpy(Object_Name, object_name, sizeof(Object_Name));
    }

    return status;
}

char *Device_Object_Name_ANSI(void)
{
    return Object_Name;
}

/* methods to manipulate the data */
uint32_t Device_Object_Instance_Number(void)
{
    return Object_Instance_Number;
}

bool Device_Set_Object_Instance_Number(uint32_t object_id)
{
    bool status = true; /* return value */

    if (object_id <= BACNET_MAX_INSTANCE) {
        Object_Instance_Number = object_id;
        /* FIXME: Write the data to the eeprom */
        /* I2C_Write_Block(
           EEPROM_DEVICE_ADDRESS,
           (char *)&Object_Instance_Number,
           sizeof(Object_Instance_Number),
           EEPROM_BACNET_ID_ADDR); */
    } else {
        status = false;
    }

    return status;
}

bool Device_Valid_Object_Instance_Number(uint32_t object_id)
{
    /* BACnet allows for a wildcard instance number */
    return (Object_Instance_Number == object_id);
}

uint16_t Device_Vendor_Identifier(void)
{
    return BACNET_VENDOR_ID;
}

unsigned Device_Object_List_Count(void)
{
    unsigned count = 1; /* at least 1 for device object */

    /* FIXME: add objects as needed */
    count += Analog_Value_Count();
    count += Binary_Value_Count();

    return count;
}

bool Device_Object_List_Identifier(
    uint32_t array_index, BACNET_OBJECT_TYPE *object_type, uint32_t *instance)
{
    bool status = false;
    uint32_t object_index = 0;
    uint32_t object_count = 0;

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
    /* analog value objects */
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

    return status;
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
int Device_Object_List_Element_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_OBJECT_TYPE object_type;
    uint32_t instance;
    bool found;

    if (object_instance == Device_Object_Instance_Number()) {
        /* single element is zero based, add 1 for BACnetARRAY which is one
         * based */
        array_index++;
        found =
            Device_Object_List_Identifier(array_index, &object_type, &instance);
        if (found) {
            apdu_len =
                encode_application_object_id(apdu, object_type, instance);
        }
    }

    return apdu_len;
}

/* return the length of the apdu encoded or -1 for error */
int Device_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    uint8_t *apdu;
    int apdu_len = 0; /* return value */
    int apdu_max = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    uint32_t i = 0;
    uint32_t count = 0;

    apdu = rpdata->application_data;
    apdu_max = rpdata->application_data_len;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_DEVICE, Object_Instance_Number);
            break;
        case PROP_OBJECT_NAME:
            characterstring_init_ansi(&char_string, Object_Name);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_DEVICE);
            break;
        case PROP_SYSTEM_STATUS:
            apdu_len = encode_application_enumerated(&apdu[0], System_Status);
            break;
        case PROP_VENDOR_NAME:
            characterstring_init_ansi(&char_string, BACNET_VENDOR_NAME);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_VENDOR_IDENTIFIER:
            apdu_len = encode_application_unsigned(
                &apdu[0], Device_Vendor_Identifier());
            break;
        case PROP_MODEL_NAME:
            characterstring_init_ansi(&char_string, Model_Name);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_FIRMWARE_REVISION:
            characterstring_init_ansi(&char_string, BACNET_VERSION_TEXT);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_APPLICATION_SOFTWARE_VERSION:
            characterstring_init_ansi(&char_string, "1.0");
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_PROTOCOL_VERSION:
            apdu_len =
                encode_application_unsigned(&apdu[0], BACNET_PROTOCOL_VERSION);
            break;
        case PROP_PROTOCOL_REVISION:
            apdu_len =
                encode_application_unsigned(&apdu[0], BACNET_PROTOCOL_REVISION);
            break;
        case PROP_PROTOCOL_SERVICES_SUPPORTED:
            /* Note: list of services that are executed, not initiated. */
            bitstring_init(&bit_string);
            for (i = 0; i < MAX_BACNET_SERVICES_SUPPORTED; i++) {
                /* automatic lookup based on handlers set */
                bitstring_set_bit(
                    &bit_string, (uint8_t)i,
                    apdu_service_supported((BACNET_SERVICES_SUPPORTED)i));
            }
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
            /* Note: this is the list of objects that can be in this device,
               not a list of objects that this device can access */
            bitstring_init(&bit_string);
            /* must have the bit string as big as it can be */
            for (i = 0; i < MAX_ASHRAE_OBJECT_TYPE; i++) {
                /* initialize all the object types to not-supported */
                bitstring_set_bit(&bit_string, (uint8_t)i, false);
            }
            /* FIXME: indicate the objects that YOU support */
            bitstring_set_bit(&bit_string, OBJECT_DEVICE, true);
            bitstring_set_bit(&bit_string, OBJECT_ANALOG_VALUE, true);
            bitstring_set_bit(&bit_string, OBJECT_BINARY_VALUE, true);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_OBJECT_LIST:
            count = Device_Object_List_Count();
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Device_Object_List_Element_Encode, count, apdu, apdu_max);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_MAX_APDU_LENGTH_ACCEPTED:
            apdu_len = encode_application_unsigned(&apdu[0], MAX_APDU);
            break;
        case PROP_SEGMENTATION_SUPPORTED:
            apdu_len =
                encode_application_enumerated(&apdu[0], SEGMENTATION_NONE);
            break;
        case PROP_APDU_TIMEOUT:
            apdu_len = encode_application_unsigned(&apdu[0], 60000);
            break;
        case PROP_NUMBER_OF_APDU_RETRIES:
            apdu_len = encode_application_unsigned(&apdu[0], 0);
            break;
        case PROP_DEVICE_ADDRESS_BINDING:
            /* FIXME: encode the list here, if it exists */
            break;
        case PROP_DATABASE_REVISION:
            apdu_len = encode_application_unsigned(&apdu[0], 0);
            break;
        case PROP_MAX_INFO_FRAMES:
            apdu_len =
                encode_application_unsigned(&apdu[0], dlmstp_max_info_frames());
            break;
        case PROP_MAX_MASTER:
            apdu_len =
                encode_application_unsigned(&apdu[0], dlmstp_max_master());
            break;
        case (BACNET_PROPERTY_ID)9600:
            apdu_len =
                encode_application_unsigned(&apdu[0], RS485_Get_Baud_Rate());
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = -1;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_OBJECT_LIST) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

bool Device_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    if (!Device_Valid_Object_Instance_Number(wp_data->object_instance)) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
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
    if ((wp_data->object_property != PROP_OBJECT_LIST) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            if (value.tag == BACNET_APPLICATION_TAG_OBJECT_ID) {
                if ((value.type.Object_Id.type == OBJECT_DEVICE) &&
                    (Device_Set_Object_Instance_Number(
                        value.type.Object_Id.instance))) {
                    /* we could send an I-Am broadcast to let the world know */
                    status = true;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_MAX_INFO_FRAMES:
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if (value.type.Unsigned_Int <= 255) {
                    dlmstp_set_max_info_frames(value.type.Unsigned_Int);
                    status = true;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_MAX_MASTER:
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if ((value.type.Unsigned_Int > 0) &&
                    (value.type.Unsigned_Int <= 127)) {
                    dlmstp_set_max_master(value.type.Unsigned_Int);
                    status = true;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_OBJECT_NAME:
            if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
                uint8_t encoding;

                encoding =
                    characterstring_encoding(&value.type.Character_String);
                if (encoding == CHARACTER_UTF8) {
                    if (characterstring_ansi_copy(
                            &Object_Name[0], sizeof(Object_Name),
                            &value.type.Character_String)) {
                        status = true;
                    } else {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code =
                            ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                    }
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code =
                        ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED;
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_NUMBER_OF_APDU_RETRIES:
        case PROP_APDU_TIMEOUT:
        case PROP_VENDOR_IDENTIFIER:
        case PROP_SYSTEM_STATUS:
        case PROP_LOCATION:
        case PROP_DESCRIPTION:
        case PROP_MODEL_NAME:
        case PROP_VENDOR_NAME:
        case PROP_FIRMWARE_REVISION:
        case PROP_APPLICATION_SOFTWARE_VERSION:
        case PROP_LOCAL_TIME:
        case PROP_UTC_OFFSET:
        case PROP_LOCAL_DATE:
        case PROP_DAYLIGHT_SAVINGS_STATUS:
        case PROP_PROTOCOL_VERSION:
        case PROP_PROTOCOL_REVISION:
        case PROP_PROTOCOL_SERVICES_SUPPORTED:
        case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
        case PROP_OBJECT_LIST:
        case PROP_MAX_APDU_LENGTH_ACCEPTED:
        case PROP_SEGMENTATION_SUPPORTED:
        case PROP_DEVICE_ADDRESS_BINDING:
        case PROP_DATABASE_REVISION:
        case PROP_ACTIVE_COV_SUBSCRIPTIONS:
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
