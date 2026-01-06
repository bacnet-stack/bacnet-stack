/**
 * @file
 * @author Steve Karg
 * @date 2016
 * @brief Network port objects, customize for your use
 *
 * @section DESCRIPTION
 *
 * The Network Port object provides access to the configuration
 * and properties of network ports of a device.
 *
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "bacnet/config.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/basic/object/device.h"
/* MS/TP specific */
#include "bacnet/datalink/dlmstp.h"
#include "rs485.h"
//#include "nvdata.h"
/* me */
#include "bacnet/basic/object/netport.h"

/* our local object data */
struct object_data {
    bool Changes_Pending : 1;
    uint8_t MAC_Address[1];
    uint8_t Max_Master;
    uint8_t Max_Info_Frames;
    float Link_Speed;
};

/* this object example only supports 1 instance */
#define BACNET_NETWORK_PORTS_MAX 1
struct object_data Object_List[BACNET_NETWORK_PORTS_MAX];
/* instance number - can be overridden */
#ifndef BACNET_NETWORK_PORT_INSTANCE
#define BACNET_NETWORK_PORT_INSTANCE 1
#endif

/* BACnetARRAY of REAL, is an array of the link speeds
   supported by this network port */
static uint32_t Link_Speeds[] = {9600, 19200, 38400, 57600, 76800, 115200 };

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int32_t Network_Port_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_STATUS_FLAGS, PROP_RELIABILITY,
    PROP_OUT_OF_SERVICE, PROP_NETWORK_TYPE, PROP_PROTOCOL_LEVEL,
    PROP_CHANGES_PENDING,
#if (BACNET_PROTOCOL_REVISION < 24)
    PROP_APDU_LENGTH,
    PROP_NETWORK_NUMBER,
    PROP_NETWORK_NUMBER_QUALITY,
    PROP_LINK_SPEED,
#endif
    -1
};

static const int32_t Network_Port_Properties_Optional[] = { PROP_MAC_ADDRESS,
    PROP_MAX_MASTER, PROP_MAX_INFO_FRAMES, PROP_LINK_SPEEDS,
#if (BACNET_PROTOCOL_REVISION >= 24)
    PROP_APDU_LENGTH,
    PROP_NETWORK_NUMBER,
    PROP_NETWORK_NUMBER_QUALITY,
    PROP_LINK_SPEED,
#endif
    -1 };

static const int32_t Network_Port_Properties_Proprietary[] = { -1 };

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
void Network_Port_Property_List(uint32_t object_instance,
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary)
{
    (void)object_instance;
    if (pRequired) {
        *pRequired = Network_Port_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Network_Port_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Network_Port_Properties_Proprietary;
    }

    return;
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
void Network_Port_Property_Lists(
    const int32_t **pRequired, const int32_t **pOptional, const int32_t **pProprietary)
{
    Network_Port_Property_List(
        BACNET_NETWORK_PORT_INSTANCE, pRequired, pOptional, pProprietary);
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
bool Network_Port_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;

    if (object_instance == BACNET_NETWORK_PORT_INSTANCE) {
        status = characterstring_init_ansi(object_name, "NP-1");
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
bool Network_Port_Valid_Instance(uint32_t object_instance)
{
    if (object_instance == BACNET_NETWORK_PORT_INSTANCE) {
        return true;
    }

    return false;
}

/**
 * Determines the number of Network Port objects
 *
 * @return  Number of Network Port objects
 */
unsigned Network_Port_Count(void)
{
    return BACNET_NETWORK_PORTS_MAX;
}

/**
 * Determines the object instance-number for a given 0..N index
 * of Network Port objects where N is Network_Port_Count().
 *
 * @param index - 0..BACNET_NETWORK_PORTS_MAX value
 *
 * @return object instance-number for the given index, or BACNET_MAX_INSTANCE
 * for an invalid index.
 */
uint32_t Network_Port_Index_To_Instance(unsigned index)
{
    if (index < BACNET_NETWORK_PORTS_MAX) {
        return BACNET_NETWORK_PORT_INSTANCE;
    }

    return BACNET_MAX_INSTANCE;
}

/**
 * For a given object instance-number, determines a 0..N index
 * of Network Port objects where N is Network_Port_Count().
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  index for the given instance-number, or BACNET_NETWORK_PORTS_MAX
 * if not valid.
 */
unsigned Network_Port_Instance_To_Index(uint32_t object_instance)
{
    if (object_instance == BACNET_NETWORK_PORT_INSTANCE) {
        return 0;
    }

    return BACNET_NETWORK_PORTS_MAX;
}

/**
 * For a given object instance-number, returns the out-of-service
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  out-of-service property value
 */
bool Network_Port_Out_Of_Service(uint32_t object_instance)
{
    (void)object_instance;
    return false;
}

/**
 * For a given object instance-number, gets the reliability.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return reliability value
 */
BACNET_RELIABILITY Network_Port_Reliability(uint32_t object_instance)
{
    (void)object_instance;
    return RELIABILITY_NO_FAULT_DETECTED;
}

/**
 * For a given object instance-number, gets the BACnet Network Type.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return BACnet network type value
 */
uint8_t Network_Port_Type(uint32_t object_instance)
{
    (void)object_instance;
    return PORT_TYPE_MSTP;
}

/**
 * For a given object instance-number, gets the BACnet Network Number.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return BACnet network type value
 */
uint16_t Network_Port_Network_Number(uint32_t object_instance)
{
    (void)object_instance;
    /* A Network_Number of 0 indicates that the Network_Number
       is not known or cannot be determined. */
    return 0;
}

/**
 * For a given object instance-number, gets the quality property
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return quality property value
 */
BACNET_PORT_QUALITY Network_Port_Quality(uint32_t object_instance)
{
    (void)object_instance;
    return PORT_QUALITY_CONFIGURED;
}

/**
 * For a given object instance-number, loads the mac-address into
 * an octet string.
 *
 * @param  object_instance - object-instance number of the object
 * @param  mac_address - holds the mac-address retrieved
 *
 * @return  true if mac-address was retrieved
 */
bool Network_Port_MAC_Address(
    uint32_t object_instance, BACNET_OCTET_STRING *mac_address)
{
    (void)object_instance;
    Object_List[0].MAC_Address[0] = dlmstp_mac_address();

    return octetstring_init(mac_address, Object_List[0].MAC_Address, 1);
}

/**
 * @brief For a given object instance-number, set the mac-address and length
 * @param  object_instance - object-instance number of the object
 * @param  mac_src - holds the MAC address to be written
 * @param  mac_len - number of bytes in the MAC address
 * @return  true if object-name was set
 */
bool Network_Port_MAC_Address_Set(
    uint32_t object_instance, const uint8_t *mac_src, uint8_t mac_len)
{
    if (mac_len == 1) {
        Object_List[0].MAC_Address[0] = mac_src[0];
        Object_List[0].Changes_Pending = true;
        return true;
    }

    return false;
}

/**
 * For a given object instance-number, gets the BACnet Network Number.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return APDU length for this network port
 */
uint16_t Network_Port_APDU_Length(uint32_t object_instance)
{
    (void)object_instance;
    return MAX_APDU;
}

/**
 * For a given object instance-number, gets the network communication rate
 * as the number of bits per second. A value of 0 indicates an unknown
 * communication rate.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return Link_Speed for this network port, or 0 if unknown
 */
float Network_Port_Link_Speed(uint32_t object_instance)
{
    (void)object_instance;
    Object_List[0].Link_Speed = rs485_baud_rate();

    return Object_List[0].Link_Speed;
}

/**
 * @brief Get the number of Link speeds supported by this object
 * @param object_instance [in] BACnet network port object instance number
 * @return number of link-speed values supported by this object
 */
static unsigned Network_Port_Link_Speeds_Count(uint32_t object_instance)
{
    return ARRAY_SIZE(Link_Speeds);
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
static int Network_Port_Link_Speeds_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    float link_speed;

    (void)object_instance;
    if (array_index < ARRAY_SIZE(Link_Speeds)) {
        link_speed = Link_Speeds[array_index];
        apdu_len = encode_application_real(apdu, link_speed);
    }

    return apdu_len;
}

/**
 * @brief Set the device link speed (baud rate)
 * @param object_instance   The object instance number of the object
 * @param value             The new link speed value
 * @return  true if value was set
 */
bool Network_Port_Link_Speed_Set(uint32_t object_instance, float value)
{
    bool status = false;
    uint32_t baud = (uint32_t)value;
    unsigned i;

    (void)object_instance;
    for (i = 0; i < ARRAY_SIZE(Link_Speeds); i++) {
       if (Link_Speeds[i] == baud) {
            Object_List[0].Link_Speed = value;
            Object_List[0].Changes_Pending = true;
            status = true;
            break;
       }
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the changes-pending flag
 * @param  object_instance - object-instance number of the object
 * @return  changes-pending property value
 */
bool Network_Port_Changes_Pending(uint32_t object_instance)
{
    bool flag = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        flag = Object_List[index].Changes_Pending;
    }

    return flag;
}

/**
 * @brief For a given object instance-number, sets the changes-pending flag
 * @param object_instance - object-instance number of the object
 * @param flag - true=changes pending, false=no changes pending
 * @return true if flag was set
 */
bool Network_Port_Changes_Pending_Set(uint32_t instance, bool flag)
{
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        Object_List[index].Changes_Pending = flag;
    } else
        return false;

    return true;
}

/**
 * @brief For a given object instance-number, gets the MS/TP Max_Master value
 * @param  object_instance - object-instance number of the object
 * @return MS/TP Max_Master value
 */
uint8_t Network_Port_MSTP_Max_Master(uint32_t object_instance)
{
    (void)object_instance;
    Object_List[0].Max_Master = dlmstp_max_master();

    return Object_List[0].Max_Master;
}

/**
 * For a given object instance-number, sets the MS/TP Max_Master value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - MS/TP Max_Master value 0..127
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_MSTP_Max_Master_Set(uint32_t object_instance, uint8_t value)
{
    bool status = false;

    (void)object_instance;
    if (value <= dlmstp_max_master_limit()) {
        Object_List[0].Max_Master = value;
        Object_List[0].Changes_Pending = true;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, gets the MS/TP Max_Info_Frames value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return MS/TP Max_Info_Frames value
 */
uint8_t Network_Port_MSTP_Max_Info_Frames(uint32_t object_instance)
{
    (void)object_instance;
    Object_List[0].Max_Info_Frames = dlmstp_max_info_frames();

    return Object_List[0].Max_Info_Frames;
}

/**
 * For a given object instance-number, sets the MS/TP Max_Info_Frames value
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - MS/TP Max_Info_Frames value 0..255
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_MSTP_Max_Info_Frames_Set(
    uint32_t object_instance, uint8_t value)
{
    bool status = false;

    (void)object_instance;
    if (value <= dlmstp_max_info_frames_limit()) {
        Object_List[0].Max_Info_Frames = value;
        Object_List[0].Changes_Pending = true;
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
int Network_Port_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0, apdu_max;
    BACNET_BIT_STRING bit_string;
    BACNET_OCTET_STRING octet_string;
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;
    unsigned count;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    apdu_max = rpdata->application_data_len;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_NETWORK_PORT, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            Network_Port_Object_Name(rpdata->object_instance, &char_string);
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
            if (Network_Port_Reliability(rpdata->object_instance) ==
                RELIABILITY_NO_FAULT_DETECTED) {
                bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            } else {
                bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, true);
            }
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            if (Network_Port_Out_Of_Service(rpdata->object_instance)) {
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
                &apdu[0], Network_Port_Reliability(rpdata->object_instance));
            break;
        case PROP_OUT_OF_SERVICE:
            apdu_len = encode_application_boolean(
                &apdu[0], Network_Port_Out_Of_Service(rpdata->object_instance));
            break;
        case PROP_NETWORK_TYPE:
            apdu_len = encode_application_enumerated(
                &apdu[0], Network_Port_Type(rpdata->object_instance));
            break;
        case PROP_PROTOCOL_LEVEL:
            apdu_len = encode_application_enumerated(
                &apdu[0], BACNET_PROTOCOL_LEVEL_PHYSICAL);
            break;
        case PROP_NETWORK_NUMBER:
            apdu_len = encode_application_unsigned(
                &apdu[0], Network_Port_Network_Number(rpdata->object_instance));
            break;
        case PROP_NETWORK_NUMBER_QUALITY:
            apdu_len = encode_application_enumerated(
                &apdu[0], Network_Port_Quality(rpdata->object_instance));
            break;
        case PROP_MAC_ADDRESS:
            Network_Port_MAC_Address(rpdata->object_instance, &octet_string);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_MAX_APDU_LENGTH_ACCEPTED:
            apdu_len = encode_application_unsigned(
                &apdu[0], Network_Port_APDU_Length(rpdata->object_instance));
            break;
        case PROP_LINK_SPEED:
            apdu_len = encode_application_real(
                &apdu[0], Network_Port_Link_Speed(rpdata->object_instance));
            break;
        case PROP_LINK_SPEEDS:
            count = Network_Port_Link_Speeds_Count(rpdata->object_instance);
            apdu_len = bacnet_array_encode(rpdata->object_instance,
                rpdata->array_index,
                Network_Port_Link_Speeds_Encode,
                count, apdu, apdu_max);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_CHANGES_PENDING:
            apdu_len = encode_application_boolean(&apdu[0],
                Network_Port_Changes_Pending(rpdata->object_instance));
            break;
        case PROP_APDU_LENGTH:
            apdu_len = encode_application_unsigned(
                &apdu[0], Network_Port_APDU_Length(rpdata->object_instance));
            break;
        case PROP_MAX_MASTER:
            apdu_len = encode_application_unsigned(&apdu[0],
                Network_Port_MSTP_Max_Master(rpdata->object_instance));
            break;
        case PROP_MAX_INFO_FRAMES:
            apdu_len = encode_application_unsigned(&apdu[0],
                Network_Port_MSTP_Max_Info_Frames(rpdata->object_instance));
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
bool Network_Port_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    if (!Network_Port_Valid_Instance(wp_data->object_instance)) {
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
    /* FIXME: len < application_data_len: more data? */
    switch (wp_data->object_property) {
        case PROP_MAX_MASTER:
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if (value.type.Unsigned_Int <= 255) {
                    status = Network_Port_MSTP_Max_Master_Set(
                        wp_data->object_instance, value.type.Unsigned_Int);
                    if (!status) {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
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
                    status = Network_Port_MSTP_Max_Info_Frames_Set(
                        wp_data->object_instance, value.type.Unsigned_Int);
                    if (!status) {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
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
        case PROP_LINK_SPEED:
            if (value.tag == BACNET_APPLICATION_TAG_REAL) {
                if (!Network_Port_Link_Speed_Set(
                        wp_data->object_instance, value.type.Real)) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_MAC_ADDRESS:
            if (value.tag == BACNET_APPLICATION_TAG_OCTET_STRING) {
                if (!Network_Port_MAC_Address_Set(wp_data->object_instance,
                        value.type.Octet_String.value,
                        value.type.Octet_String.length)) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_RELIABILITY:
        case PROP_OUT_OF_SERVICE:
        case PROP_NETWORK_TYPE:
        case PROP_PROTOCOL_LEVEL:
        case PROP_NETWORK_NUMBER:
        case PROP_NETWORK_NUMBER_QUALITY:
        case PROP_MAX_APDU_LENGTH_ACCEPTED:
        case PROP_CHANGES_PENDING:
        case PROP_APDU_LENGTH:
        case PROP_LINK_SPEEDS:
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
 * Initializes the Network Port object data
 */
void Network_Port_Init(void)
{
    Object_List[0].Changes_Pending = false;
    Object_List[0].MAC_Address[0] = dlmstp_mac_address();
    Object_List[0].Max_Master = dlmstp_max_master();
    Object_List[0].Max_Info_Frames = dlmstp_max_info_frames();
    Object_List[0].Link_Speed = rs485_baud_rate();
}
