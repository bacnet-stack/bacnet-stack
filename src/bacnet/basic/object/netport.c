/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2016
 * @brief A basic BACnet Network Port object provides access to the
 * configuration and properties of any network ports of a device.
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/bacint.h"
#include "bacnet/bacdcode.h"
#include "bacnet/datetime.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/datalink/bvlc.h"
#include "bacnet/datalink/bvlc6.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
/* me */
#include "bacnet/basic/object/netport.h"
#include <bacnet/basic/object/netport_internal.h>

#ifndef BBMD_ENABLED
#define BBMD_ENABLED 1
#endif

#define IPV4_ADDR_SIZE 4
#define BIP_DNS_MAX 3
struct bacnet_ipv4_port {
    uint8_t IP_Address[IPV4_ADDR_SIZE];
    uint8_t IP_Subnet_Prefix;
    uint8_t IP_Gateway[IPV4_ADDR_SIZE];
    uint8_t IP_DNS_Server[BIP_DNS_MAX][IPV4_ADDR_SIZE];
    uint16_t Port;
    BACNET_IP_MODE Mode;
    bool IP_DHCP_Enable;
    uint32_t IP_DHCP_Lease_Seconds;
    uint32_t IP_DHCP_Lease_Seconds_Start;
    uint8_t IP_DHCP_Server[IPV4_ADDR_SIZE];
    bool IP_NAT_Traversal;
    uint32_t IP_Global_Address[IPV4_ADDR_SIZE];
    bool BBMD_Accept_FD_Registrations;
    void *BBMD_BD_Table;
    void *BBMD_FD_Table;
    /* used for foreign device registration to remote BBMD */
    BACNET_HOST_N_PORT_MINIMAL BBMD_Address;
    uint16_t BBMD_Lifetime;
};

#define IPV6_ADDR_SIZE 16
#define ZONE_INDEX_SIZE 16
struct bacnet_ipv6_port {
    uint8_t MAC_Address[3];
    uint8_t IP_Address[IPV6_ADDR_SIZE];
    uint8_t IP_Subnet_Prefix;
    uint8_t IP_Gateway[IPV6_ADDR_SIZE];
    uint8_t IP_DNS_Server[BIP_DNS_MAX][IPV6_ADDR_SIZE];
    uint8_t IP_Multicast_Address[IPV6_ADDR_SIZE];
    bool IP_DHCP_Enable;
    uint8_t IP_DHCP_Server[IPV6_ADDR_SIZE];
    uint32_t IP_DHCP_Lease_Seconds;
    uint32_t IP_DHCP_Lease_Seconds_Start;
    uint16_t Port;
    BACNET_IP_MODE Mode;
    char Zone_Index[ZONE_INDEX_SIZE];
    bool BBMD_Accept_FD_Registrations;
    void *BBMD_BD_Table;
    void *BBMD_FD_Table;
    /* used for foreign device registration to remote BBMD */
    BACNET_HOST_N_PORT_MINIMAL BBMD_Address;
    uint16_t BBMD_Lifetime;
};

struct ethernet_port {
    uint8_t MAC_Address[6];
};

struct mstp_port {
    uint8_t MAC_Address;
    uint8_t Max_Master;
    uint8_t Max_Info_Frames;
};

struct bsc_port {
    uint8_t MAC_Address[6];
    BACNET_SC_PARAMS Parameters;
};

struct object_data {
    uint32_t Instance_Number;
    const char *Object_Name;
    const char *Description;
    BACNET_RELIABILITY Reliability;
    bool Out_Of_Service : 1;
    bool Changes_Pending : 1;
    uint8_t Network_Type;
    uint16_t Network_Number;
    BACNET_PORT_QUALITY Quality;
    uint16_t APDU_Length;
    float Link_Speed;
    bacnet_network_port_activate_changes Activate_Changes;
    bacnet_network_port_discard_changes Discard_Changes;
    union {
        struct bacnet_ipv4_port IPv4;
        struct bacnet_ipv6_port IPv6;
        struct ethernet_port Ethernet;
        struct mstp_port MSTP;
        struct bsc_port BSC;
    } Network;
};

#ifndef BACNET_NETWORK_PORTS_MAX
#define BACNET_NETWORK_PORTS_MAX 1
#endif

static struct object_data Object_List[BACNET_NETWORK_PORTS_MAX];

#ifndef MSTP_LINK_SPEED_LIST
#define MSTP_LINK_SPEED_LIST 9600, 19200, 38400, 57600, 76800, 115200
#endif
static unsigned MSTP_LINK_SPEEDS[] = { MSTP_LINK_SPEED_LIST };
static unsigned MSTP_LINK_SPEEDS_MAX =
    sizeof MSTP_LINK_SPEEDS / sizeof(unsigned);

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Network_Port_Properties_Required[] = {
    /* unordered list of required properties */
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_STATUS_FLAGS,
    PROP_RELIABILITY,
    PROP_OUT_OF_SERVICE,
    PROP_NETWORK_TYPE,
    PROP_PROTOCOL_LEVEL,
    PROP_CHANGES_PENDING,
#if (BACNET_PROTOCOL_REVISION < 24)
    PROP_APDU_LENGTH,
    PROP_NETWORK_NUMBER,
    PROP_NETWORK_NUMBER_QUALITY,
    PROP_LINK_SPEED,
#endif
    -1
};

static const int Ethernet_Port_Properties_Optional[] = {
    /* unordered list of optional properties */
    PROP_DESCRIPTION,
    PROP_MAC_ADDRESS,
#if (BACNET_PROTOCOL_REVISION >= 24)
    PROP_APDU_LENGTH,
    PROP_NETWORK_NUMBER,
    PROP_NETWORK_NUMBER_QUALITY,
    PROP_LINK_SPEED,
#endif
    -1
};

static const int MSTP_Port_Properties_Optional[] = {
    /* unordered list of optional properties */
    PROP_DESCRIPTION,
    PROP_MAC_ADDRESS,
    PROP_MAX_MASTER,
    PROP_MAX_INFO_FRAMES,
#if (BACNET_PROTOCOL_REVISION >= 24)
    PROP_APDU_LENGTH,
    PROP_NETWORK_NUMBER,
    PROP_NETWORK_NUMBER_QUALITY,
    PROP_LINK_SPEED,
    PROP_LINK_SPEEDS,
#endif
    -1
};

static const int BIP_Port_Properties_Optional[] = {
    /* unordered list of optional properties */
    PROP_DESCRIPTION,
    PROP_MAC_ADDRESS,
    PROP_BACNET_IP_MODE,
    PROP_IP_ADDRESS,
    PROP_BACNET_IP_UDP_PORT,
    PROP_IP_SUBNET_MASK,
    PROP_IP_DEFAULT_GATEWAY,
    PROP_IP_DNS_SERVER,
#if defined(BACDL_BIP) && (BACNET_NETWORK_PORT_IP_DHCP_ENABLED)
    PROP_IP_DHCP_ENABLE,
    PROP_IP_DHCP_LEASE_TIME,
    PROP_IP_DHCP_LEASE_TIME_REMAINING,
    PROP_IP_DHCP_SERVER,
#endif
#if defined(BACDL_BIP) && (BBMD_ENABLED)
    PROP_BBMD_ACCEPT_FD_REGISTRATIONS,
    PROP_BBMD_BROADCAST_DISTRIBUTION_TABLE,
    PROP_BBMD_FOREIGN_DEVICE_TABLE,
#endif
#if defined(BACDL_BIP) && (BBMD_CLIENT_ENABLED)
    PROP_FD_BBMD_ADDRESS,
    PROP_FD_SUBSCRIPTION_LIFETIME,
#endif
#if (BACNET_PROTOCOL_REVISION >= 24)
    PROP_APDU_LENGTH,
    PROP_NETWORK_NUMBER,
    PROP_NETWORK_NUMBER_QUALITY,
    PROP_LINK_SPEED,
#endif
    -1
};

static const int BIP6_Port_Properties_Optional[] = {
    /* unordered list of optional properties */
    PROP_DESCRIPTION,
    PROP_MAC_ADDRESS,
    PROP_BACNET_IPV6_MODE,
    PROP_IPV6_ADDRESS,
    PROP_IPV6_PREFIX_LENGTH,
    PROP_BACNET_IPV6_UDP_PORT,
    PROP_IPV6_DEFAULT_GATEWAY,
    PROP_BACNET_IPV6_MULTICAST_ADDRESS,
    PROP_IPV6_DNS_SERVER,
#if defined(BACDL_BIP6) && (BACNET_NETWORK_PORT_IP_DHCP_ENABLED)
    PROP_IPV6_AUTO_ADDRESSING_ENABLE,
    PROP_IPV6_DHCP_LEASE_TIME,
    PROP_IPV6_DHCP_LEASE_TIME_REMAINING,
    PROP_IPV6_DHCP_SERVER,
#endif
    PROP_IPV6_ZONE_INDEX,
#if defined(BACDL_BIP6) && (BBMD_ENABLED)
    PROP_BBMD_ACCEPT_FD_REGISTRATIONS,
    PROP_BBMD_BROADCAST_DISTRIBUTION_TABLE,
    PROP_BBMD_FOREIGN_DEVICE_TABLE,
#endif
#if defined(BACDL_BIP6) && (BBMD_CLIENT_ENABLED)
    PROP_FD_BBMD_ADDRESS,
    PROP_FD_SUBSCRIPTION_LIFETIME,
#endif
#if (BACNET_PROTOCOL_REVISION >= 24)
    PROP_APDU_LENGTH,
    PROP_NETWORK_NUMBER,
    PROP_NETWORK_NUMBER_QUALITY,
    PROP_LINK_SPEED,
#endif
    -1
};

static const int BSC_Port_Properties_Optional[] = {
    PROP_NETWORK_NUMBER,
    PROP_NETWORK_NUMBER_QUALITY,
    PROP_APDU_LENGTH,
    PROP_MAC_ADDRESS,
    PROP_BACNET_IP_MODE,
    PROP_IP_ADDRESS,
    PROP_BACNET_IP_UDP_PORT,
    PROP_IP_SUBNET_MASK,
    PROP_IP_DEFAULT_GATEWAY,
    PROP_IP_DNS_SERVER,
    PROP_MAX_BVLC_LENGTH_ACCEPTED,
    PROP_MAX_NPDU_LENGTH_ACCEPTED,
    PROP_SC_PRIMARY_HUB_URI,
    PROP_SC_FAILOVER_HUB_URI,
    PROP_SC_MINIMUM_RECONNECT_TIME,
    PROP_SC_MAXIMUM_RECONNECT_TIME,
    PROP_SC_CONNECT_WAIT_TIMEOUT,
    PROP_SC_DISCONNECT_WAIT_TIMEOUT,
    PROP_SC_HEARTBEAT_TIMEOUT,
    PROP_SC_HUB_CONNECTOR_STATE,
    PROP_OPERATIONAL_CERTIFICATE_FILE,
    PROP_ISSUER_CERTIFICATE_FILES,
    PROP_CERTIFICATE_SIGNING_REQUEST_FILE,
/*SC optional*/
#ifdef BACNET_SECURE_CONNECT_ROUTING_TABLE
    PROP_ROUTING_TABLE,
#endif /* BACNET_SECURE_CONNECT_ROUTING_TABLE */
#if BSC_CONF_HUB_FUNCTIONS_NUM != 0
    PROP_SC_PRIMARY_HUB_CONNECTION_STATUS,
    PROP_SC_FAILOVER_HUB_CONNECTION_STATUS,
    PROP_SC_HUB_FUNCTION_ENABLE,
    PROP_SC_HUB_FUNCTION_ACCEPT_URIS,
    PROP_SC_HUB_FUNCTION_BINDING,
    PROP_SC_HUB_FUNCTION_CONNECTION_STATUS,
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM != 0
    PROP_SC_DIRECT_CONNECT_INITIATE_ENABLE,
    PROP_SC_DIRECT_CONNECT_ACCEPT_ENABLE,
    PROP_SC_DIRECT_CONNECT_ACCEPT_URIS,
    PROP_SC_DIRECT_CONNECT_BINDING,
    PROP_SC_DIRECT_CONNECT_CONNECTION_STATUS,
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */
    PROP_SC_FAILED_CONNECTION_REQUESTS,
    -1
};

static const int Network_Port_Properties_Proprietary[] = { -1 };

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
void Network_Port_Property_List(
    uint32_t object_instance,
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    unsigned index = 0;

    if (pRequired) {
        *pRequired = Network_Port_Properties_Required;
    }
    if (pOptional) {
        index = Network_Port_Instance_To_Index(object_instance);
        if (index < BACNET_NETWORK_PORTS_MAX) {
            switch (Object_List[index].Network_Type) {
                case PORT_TYPE_MSTP:
                    *pOptional = MSTP_Port_Properties_Optional;
                    break;
                case PORT_TYPE_BIP:
                    *pOptional = BIP_Port_Properties_Optional;
                    break;
                case PORT_TYPE_BSC:
                    *pOptional = BSC_Port_Properties_Optional;
                    break;
                case PORT_TYPE_BIP6:
                    *pOptional = BIP6_Port_Properties_Optional;
                    break;
                case PORT_TYPE_ETHERNET:
                default:
                    *pOptional = Ethernet_Port_Properties_Optional;
                    break;
            }
        }
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
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    Network_Port_Property_List(
        Object_List[0].Instance_Number, pRequired, pOptional, pProprietary);
}

/**
 * @brief Determine if the object property is a member of this object instance
 * @param object_instance - object-instance number of the object
 * @param object_property - object-property to be checked
 * @return true if the property is a member of this object instance
 */
static bool Property_List_Member(uint32_t object_instance, int object_property)
{
    bool found = false;
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;

    Network_Port_Property_List(
        object_instance, &pRequired, &pOptional, &pProprietary);
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
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
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
bool Network_Port_Name_Set(uint32_t object_instance, const char *new_name)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        Object_List[index].Object_Name = new_name;
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, returns the ASCII object-name
 * @param  object_instance - object-instance number of the object
 * @return ASCII C string object name, or NULL if not found or not set.
 */
const char *Network_Port_Object_Name_ASCII(uint32_t object_instance)
{
    unsigned index = 0; /* offset from instance lookup */

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        return Object_List[index].Object_Name;
    }

    return NULL;
}

/**
 * @brief For a given object instance-number, returns the ASCII description
 * @param  object_instance - object-instance number of the object
 * @return ASCII C string object name, or NULL if not found or not set.
 */
const char *Network_Port_Description(uint32_t instance)
{
    unsigned index = 0; /* offset from instance lookup */

    index = Network_Port_Instance_To_Index(instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        return Object_List[index].Description;
    }

    return NULL;
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
bool Network_Port_Description_Set(uint32_t instance, const char *new_name)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        Object_List[index].Description = new_name;
        status = true;
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
    unsigned index = 0; /* offset from instance lookup */

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
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
        return Object_List[index].Instance_Number;
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
    unsigned index = 0;

    for (index = 0; index < BACNET_NETWORK_PORTS_MAX; index++) {
        if (Object_List[index].Instance_Number == object_instance) {
            return index;
        }
    }

    return BACNET_NETWORK_PORTS_MAX;
}

/**
 * For the Network Port object, set the instance number.
 *
 * @param  index - 0..BACNET_NETWORK_PORTS_MAX value
 * @param  object_instance - object-instance number of the object
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_Object_Instance_Number_Set(
    unsigned index, uint32_t object_instance)
{
    bool status = false; /* return value */

    if (index < BACNET_NETWORK_PORTS_MAX) {
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
bool Network_Port_Out_Of_Service(uint32_t object_instance)
{
    bool oos_flag = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
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
bool Network_Port_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
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
BACNET_RELIABILITY Network_Port_Reliability(uint32_t object_instance)
{
    BACNET_RELIABILITY reliability = RELIABILITY_NO_FAULT_DETECTED;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
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
bool Network_Port_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        Object_List[index].Reliability = value;
        status = true;
    }

    return status;
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
    uint8_t port_type = PORT_TYPE_NON_BACNET;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        port_type = Object_List[index].Network_Type;
#if (BACNET_PROTOCOL_REVISION >= 17) && (BACNET_PROTOCOL_REVISION <= 23)
        /*  For BACnet/SC network port implementations with
            a protocol revision Protocol_Revision 17 and higher through 23,
            BACnet/SC network ports shall be represented by a Network Port
            object at the BACNET_APPLICATION protocol level with
            a proprietary network type value. */
        if (port_type == PORT_TYPE_BSC) {
            port_type = PORT_TYPE_BSC_INTERIM;
        }
#endif
    }

    return port_type;
}

/**
 * For a given object instance-number, sets the BACnet port type
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - network port type 0-63 are defined by ASHRAE.
 *  Values from 64-255 may be used by others subject to the
 *  procedures and constraints described in Clause 23.
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_Type_Set(uint32_t object_instance, uint8_t value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
#if (BACNET_PROTOCOL_REVISION >= 17) && (BACNET_PROTOCOL_REVISION <= 23)
        /*  For BACnet/SC network port implementations with
            a protocol revision Protocol_Revision 17 and higher through 23,
            BACnet/SC network ports shall be represented by a Network Port
            object at the BACNET_APPLICATION protocol level with
            a proprietary network type value. */
        if (value == PORT_TYPE_BSC_INTERIM) {
            value = PORT_TYPE_BSC;
        }
#endif
        Object_List[index].Network_Type = value;

        status = true;
    }

    return status;
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
    uint16_t network_number = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        network_number = Object_List[index].Network_Number;
    }

    return network_number;
}

/**
 * For a given object instance-number, sets the BACnet Network Number
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - network number 0..65534
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_Network_Number_Set(uint32_t object_instance, uint16_t value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        Object_List[index].Network_Number = value;
        status = true;
    }

    return status;
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
    BACNET_PORT_QUALITY value = PORT_QUALITY_UNKNOWN;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        value = Object_List[index].Quality;
    }

    return value;
}

/**
 * For a given object instance-number, sets the quality property
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - quality enumerated value
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_Quality_Set(
    uint32_t object_instance, BACNET_PORT_QUALITY value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        Object_List[index].Quality = value;
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, loads the mac-address into
 * a buffer and returns the length of the mac-address.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  mac_address - holds the mac-address retrieved, or NULL for length
 * @param  mac_size - size of the mac-address buffer, or 0 for length
 *
 * @return the length of the mac-address, or zero if not found
 */
uint8_t Network_Port_MAC_Address_Value(
    uint32_t object_instance, uint8_t *mac_address, size_t mac_size)
{
    unsigned index = 0; /* offset from instance lookup */
    uint8_t *mac = NULL;
    uint8_t ip_mac[4 + 2] = { 0 };
    size_t mac_len = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        switch (Object_List[index].Network_Type) {
            case PORT_TYPE_ETHERNET:
                mac = &Object_List[index].Network.Ethernet.MAC_Address[0];
                mac_len =
                    sizeof(Object_List[index].Network.Ethernet.MAC_Address);
                break;
            case PORT_TYPE_MSTP:
                mac = &Object_List[index].Network.MSTP.MAC_Address;
                mac_len = sizeof(Object_List[index].Network.MSTP.MAC_Address);
                break;
            case PORT_TYPE_BIP:
                memcpy(
                    &ip_mac[0], &Object_List[index].Network.IPv4.IP_Address, 4);
                /* convert port from host-byte-order to network-byte-order */
                encode_unsigned16(
                    &ip_mac[4], Object_List[index].Network.IPv4.Port);
                mac = &ip_mac[0];
                mac_len = sizeof(ip_mac);
                break;
            case PORT_TYPE_BIP6:
                mac = &Object_List[index].Network.IPv6.MAC_Address[0];
                mac_len = sizeof(Object_List[index].Network.IPv6.MAC_Address);
                break;
            case PORT_TYPE_BSC:
                mac = &Object_List[index].Network.BSC.MAC_Address[0];
                mac_len = sizeof(Object_List[index].Network.BSC.MAC_Address);
                break;
            default:
                break;
        }
        if (mac_len > 0) {
            if ((mac_address) && (mac_size >= mac_len)) {
                memcpy(mac_address, mac, mac_len);
            }
        }
    }

    return mac_len;
}

/**
 * For a given object instance-number, loads the mac-address into
 * an octet string.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  mac_address - holds the mac-address retrieved
 *
 * @return  true if mac-address was retrieved
 */
bool Network_Port_MAC_Address(
    uint32_t object_instance, BACNET_OCTET_STRING *mac_address)
{
    uint8_t mac_len = 0;

    if (mac_address) {
        mac_len = Network_Port_MAC_Address_Value(
            object_instance, mac_address->value, sizeof(mac_address->value));
        mac_address->length = mac_len;
    }

    return mac_len > 0;
}

/**
 * For a given object instance-number, sets the mac-address and it's length
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  new_name - holds the object-name to be written
 *         Expecting a pointer to a static ANSI C string for zero copy.
 *
 * @return  true if object-name was set
 */
bool Network_Port_MAC_Address_Set(
    uint32_t object_instance, const uint8_t *mac_src, uint8_t mac_len)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;
    size_t mac_size = 0;
    uint8_t *mac_dest = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        switch (Object_List[index].Network_Type) {
            case PORT_TYPE_ETHERNET:
                mac_dest = &Object_List[index].Network.Ethernet.MAC_Address[0];
                mac_size =
                    sizeof(Object_List[index].Network.Ethernet.MAC_Address);
                break;
            case PORT_TYPE_MSTP:
                mac_dest = &Object_List[index].Network.MSTP.MAC_Address;
                mac_size = sizeof(Object_List[index].Network.MSTP.MAC_Address);
                break;
            case PORT_TYPE_BIP:
                if (mac_len >= 6) {
                    memcpy(
                        &Object_List[index].Network.IPv4.IP_Address,
                        &mac_src[0], 4);
                    /* convert from network-byte-order to host-byte-order */
                    decode_unsigned16(
                        &mac_src[4], &Object_List[index].Network.IPv4.Port);
                    status = true;
                }
                break;
            case PORT_TYPE_BIP6:
                mac_dest = &Object_List[index].Network.IPv6.MAC_Address[0];
                mac_size = sizeof(Object_List[index].Network.IPv6.MAC_Address);
                break;
            case PORT_TYPE_BSC:
                mac_dest = &Object_List[index].Network.BSC.MAC_Address[0];
                mac_size = sizeof(Object_List[index].Network.BSC.MAC_Address);
                break;
            default:
                break;
        }
        if (mac_src && mac_dest && (mac_len == mac_size)) {
            Object_List[index].Changes_Pending = true;
            memcpy(mac_dest, mac_src, mac_size);
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the APDU length.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return APDU length for this network port
 */
uint16_t Network_Port_APDU_Length(uint32_t object_instance)
{
    uint16_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        value = Object_List[index].APDU_Length;
    }

    return value;
}

/**
 * For a given object instance-number, sets the APDU length
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - APDU length 0..65535
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_APDU_Length_Set(uint32_t object_instance, uint16_t value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        Object_List[index].APDU_Length = value;
        status = true;
    }

    return status;
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
    float value = 0.0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        value = Object_List[index].Link_Speed;
    }

    return value;
}

/**
 * Validate the Link_Speed
 *
 * @param  value Link_Speed value in bits-per-second
 * @return  true if values are in MSTP_Link_Speeds
 */
static bool MSTP_Valid_Link_Speed(const float value)
{
    for (int i = 0; i < MSTP_LINK_SPEEDS_MAX; ++i) {
        if ((unsigned)value == MSTP_LINK_SPEEDS[i]) {
            return true;
        }
    }
    return false;
}

/**
 * For a given object instance-number, sets the Link_Speed
 *
 * @param  object_instance - object-instance number of the object
 * @param  value Link_Speed value in bits-per-second
 * @return  true if values are within range and property is set.
 */
bool Network_Port_Link_Speed_Set(uint32_t object_instance, float value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type != PORT_TYPE_MSTP ||
            MSTP_Valid_Link_Speed(value)) {
            Object_List[index].Link_Speed = value;
            Object_List[index].Changes_Pending = true;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, returns the changes-pending
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
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
 * For a given object instance-number, sets the changes-pending property value
 *
 * @param object_instance - object-instance number of the object
 * @param value - boolean changes-pending value
 *
 * @return true if the changes-pending property value was set
 */
bool Network_Port_Changes_Pending_Set(uint32_t object_instance, bool value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        Object_List[index].Changes_Pending = value;
        if (value == false) {
            Network_Port_Changes_Pending_Discard(object_instance);
        }
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, activates any pending changes
 * @param object_instance - object-instance number of the object
 */
void Network_Port_Changes_Pending_Activate(uint32_t object_instance)
{
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Activate_Changes) {
            Object_List[index].Activate_Changes(object_instance);
        }
    }
}

/**
 * @brief For a given object instance-number, sets the callback function
 * to activate any pending changes
 */
void Network_Port_Changes_Pending_Activate_Callback_Set(
    uint32_t object_instance, bacnet_network_port_activate_changes callback)

{
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        Object_List[index].Activate_Changes = callback;
    }
}

/**
 * @brief For a given object instance-number, discards any pending changes
 * @param object_instance - object-instance number of the object
 */
void Network_Port_Changes_Pending_Discard(uint32_t object_instance)
{
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Discard_Changes) {
            Object_List[index].Discard_Changes(object_instance);
        }
    }
}

/**
 * @brief For a given object instance-number, sets the callback function
 * to discard any pending changes
 */
void Network_Port_Changes_Pending_Discard_Callback_Set(
    uint32_t object_instance, bacnet_network_port_discard_changes callback)

{
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        Object_List[index].Discard_Changes = callback;
    }
}

/**
 * For a given object instance-number, gets the MS/TP Max_Master value
 * Note: depends on Network_Type being set to PORT_TYPE_MSTP for this object
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return MS/TP Max_Master value
 */
uint8_t Network_Port_MSTP_MAC_Address(uint32_t object_instance)
{
    uint8_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_MSTP) {
            value = Object_List[index].Network.MSTP.MAC_Address;
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the MS/TP Max_Master value
 * Note: depends on Network_Type being set to PORT_TYPE_MSTP for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - MS/TP Max_Master value 0..127
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_MSTP_MAC_Address_Set(uint32_t object_instance, uint8_t value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_MSTP) {
            if (value <= 127) {
                if (Object_List[index].Network.MSTP.MAC_Address != value) {
                    Object_List[index].Changes_Pending = true;
                }
                Object_List[index].Network.MSTP.MAC_Address = value;
                status = true;
            }
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the MS/TP Max_Master value
 * Note: depends on Network_Type being set to PORT_TYPE_MSTP for this object
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return MS/TP Max_Master value
 */
uint8_t Network_Port_MSTP_Max_Master(uint32_t object_instance)
{
    uint8_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_MSTP) {
            value = Object_List[index].Network.MSTP.Max_Master;
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the MS/TP Max_Master value
 * Note: depends on Network_Type being set to PORT_TYPE_MSTP for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - MS/TP Max_Master value 0..127
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_MSTP_Max_Master_Set(uint32_t object_instance, uint8_t value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_MSTP) {
            if (value <= 127) {
                if (Object_List[index].Network.MSTP.Max_Master != value) {
                    Object_List[index].Changes_Pending = true;
                }
                Object_List[index].Network.MSTP.Max_Master = value;
                status = true;
            }
        }
    }

    return status;
}

/**
 * For a given object instance-number, loads the ip-address into
 * an octet string.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  ip_address - holds the mac-address retrieved
 *
 * @return  true if ip-address was retrieved
 */
bool Network_Port_IP_Address(
    uint32_t object_instance, BACNET_OCTET_STRING *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            status = octetstring_init(
                ip_address, &Object_List[index].Network.IPv4.IP_Address[0], 4);
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the ip-address
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  a - ip-address first octet
 * @param  b - ip-address second octet
 * @param  c - ip-address third octet
 * @param  d - ip-address fourth octet
 *
 * @return  true if ip-address was set
 */
bool Network_Port_IP_Address_Set(
    uint32_t object_instance, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            Object_List[index].Network.IPv4.IP_Address[0] = a;
            Object_List[index].Network.IPv4.IP_Address[1] = b;
            Object_List[index].Network.IPv4.IP_Address[2] = c;
            Object_List[index].Network.IPv4.IP_Address[3] = d;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, loads the subnet-mask-address into
 * an octet string.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  subnet-mask - holds the mac-address retrieved
 *
 * @return  true if ip-address was retrieved
 */
bool Network_Port_IP_Subnet(
    uint32_t object_instance, BACNET_OCTET_STRING *subnet_mask)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;
    uint32_t mask = 0;
    uint32_t prefix = 0;
    uint8_t ip_mask[4] = { 255, 255, 255, 255 };

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            prefix = Object_List[index].Network.IPv4.IP_Subnet_Prefix;
            if ((prefix > 0) && (prefix <= 32)) {
                mask = (0xFFFFFFFF << (32 - prefix)) & 0xFFFFFFFF;
                encode_unsigned32(ip_mask, mask);
            }
            status = octetstring_init(subnet_mask, ip_mask, sizeof(ip_mask));
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the BACnet/IP Subnet prefix value
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return BACnet/IP subnet prefix value
 */
uint8_t Network_Port_IP_Subnet_Prefix(uint32_t object_instance)
{
    uint8_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            value = Object_List[index].Network.IPv4.IP_Subnet_Prefix;
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the BACnet/IP Subnet prefix value
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACnet/IP Subnet prefix value 1..32
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_IP_Subnet_Prefix_Set(uint32_t object_instance, uint8_t value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            if ((value > 0) && (value <= 32)) {
                if (Object_List[index].Network.IPv4.IP_Subnet_Prefix != value) {
                    Object_List[index].Changes_Pending = true;
                }
                Object_List[index].Network.IPv4.IP_Subnet_Prefix = value;
                status = true;
            }
        }
    }

    return status;
}

/**
 * For a given object instance-number, loads the gateway ip-address into
 * an octet string.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  ip_address - holds the ip-address retrieved
 *
 * @return  true if ip-address was retrieved
 */
bool Network_Port_IP_Gateway(
    uint32_t object_instance, BACNET_OCTET_STRING *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            status = octetstring_init(
                ip_address, &Object_List[index].Network.IPv4.IP_Gateway[0], 4);
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the gateway ip-address
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  a - ip-address first octet
 * @param  b - ip-address second octet
 * @param  c - ip-address third octet
 * @param  d - ip-address fourth octet
 *
 * @return  true if ip-address was set
 */
bool Network_Port_IP_Gateway_Set(
    uint32_t object_instance, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            Object_List[index].Network.IPv4.IP_Gateway[0] = a;
            Object_List[index].Network.IPv4.IP_Gateway[1] = b;
            Object_List[index].Network.IPv4.IP_Gateway[2] = c;
            Object_List[index].Network.IPv4.IP_Gateway[3] = d;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, returns the IP_DHCP_Enable
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  IP_DHCP_Enable property value
 */
bool Network_Port_IP_DHCP_Enable(uint32_t object_instance)
{
    bool dhcp_enable = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            dhcp_enable = Object_List[index].Network.IPv4.IP_DHCP_Enable;
        } else if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            dhcp_enable = Object_List[index].Network.IPv6.IP_DHCP_Enable;
        }
    }

    return dhcp_enable;
}

/**
 * For a given object instance-number, sets the IP_DHCP_Enable property value
 *
 * @param object_instance - object-instance number of the object
 * @param value - boolean IP_DHCP_Enable value
 *
 * @return true if the IP_DHCP_Enable property value was set
 */
bool Network_Port_IP_DHCP_Enable_Set(uint32_t object_instance, bool value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            if (Object_List[index].Network.IPv4.IP_DHCP_Enable != value) {
                Object_List[index].Changes_Pending = true;
            }
            Object_List[index].Network.IPv4.IP_DHCP_Enable = value;
            status = true;
        } else if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            if (Object_List[index].Network.IPv6.IP_DHCP_Enable != value) {
                Object_List[index].Changes_Pending = true;
            }
            Object_List[index].Network.IPv6.IP_DHCP_Enable = value;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number and dns_index, loads the ip-address into
 * an octet string.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  dns_index - 0=primary, 1=secondary, 3=tertierary
 * @param  ip_address - holds the mac-address retrieved
 *
 * @return  true if ip-address was retrieved
 */
bool Network_Port_IP_DNS_Server(
    uint32_t object_instance,
    unsigned dns_index,
    BACNET_OCTET_STRING *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            if (dns_index < BIP_DNS_MAX) {
                status = octetstring_init(
                    ip_address,
                    &Object_List[index]
                         .Network.IPv4.IP_DNS_Server[dns_index][0],
                    4);
            }
        }
    }

    return status;
}

/**
 * @brief Encode a BACnetARRAY property element; a function template
 * @param object_instance [in] BACnet network port object instance number
 * @param index [in] array index requested:
 *    0 to (array size - 1) for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or
 *    NULL to return the length of buffer if it had been built
 * @return The length of the apdu encoded, or
 *    BACNET_STATUS_ERROR for an invalid array index
 */
static int Network_Port_IP_DNS_Server_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = 0;
    BACNET_OCTET_STRING ip_address = { 0 };

    if (index >= BIP_DNS_MAX) {
        apdu_len = BACNET_STATUS_ERROR;
    } else {
        if (Network_Port_IP_DNS_Server(object_instance, index, &ip_address)) {
            apdu_len = encode_application_octet_string(apdu, &ip_address);
        }
    }

    return apdu_len;
}

/**
 * For a given object instance-number, sets the ip-address
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  index - 0=primary, 1=secondary, 3=tertierary
 * @param  a - ip-address first octet
 * @param  b - ip-address second octet
 * @param  c - ip-address third octet
 * @param  d - ip-address fourth octet
 *
 * @return  true if ip-address was set
 */
bool Network_Port_IP_DNS_Server_Set(
    uint32_t object_instance,
    unsigned dns_index,
    uint8_t a,
    uint8_t b,
    uint8_t c,
    uint8_t d)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;
    uint8_t *dns_server = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            if (dns_index < BIP_DNS_MAX) {
                dns_server = &Object_List[index]
                                  .Network.IPv4.IP_DNS_Server[dns_index][0];
                if ((dns_server[0] != a) || (dns_server[1] != b) ||
                    (dns_server[2] != c) || (dns_server[3] != d)) {
                    /* octets are different, set changes pending */
                    Object_List[index].Changes_Pending = true;
                }
                Object_List[index].Network.IPv4.IP_DNS_Server[dns_index][0] = a;
                Object_List[index].Network.IPv4.IP_DNS_Server[dns_index][1] = b;
                Object_List[index].Network.IPv4.IP_DNS_Server[dns_index][2] = c;
                Object_List[index].Network.IPv4.IP_DNS_Server[dns_index][3] = d;
                status = true;
            }
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the BACnet/IP UDP Port number
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return BACnet/IP UDP Port number
 */
uint16_t Network_Port_BIP_Port(uint32_t object_instance)
{
    uint16_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            value = Object_List[index].Network.IPv4.Port;
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the BACnet/IP UDP Port number
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACnet/IP UDP Port number (default=0xBAC0)
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_BIP_Port_Set(uint32_t object_instance, uint16_t value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            if (Object_List[index].Network.IPv4.Port != value) {
                Object_List[index].Changes_Pending = true;
            }
            Object_List[index].Network.IPv4.Port = value;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the BACnet/IP UDP Port number
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return BACnet/IP UDP Port number
 */
BACNET_IP_MODE Network_Port_BIP_Mode(uint32_t object_instance)
{
    BACNET_IP_MODE value = BACNET_IP_MODE_NORMAL;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            value = Object_List[index].Network.IPv4.Mode;
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the BACnet/IP UDP Port number
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACnet/IP UDP Port number (default=0xBAC0)
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_BIP_Mode_Set(uint32_t object_instance, BACNET_IP_MODE value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            if (Object_List[index].Network.IPv4.Mode != value) {
                Object_List[index].Changes_Pending = true;
            }
            Object_List[index].Network.IPv4.Mode = value;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, returns the BBMD-Accept-FD-Registrations
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  BBMD-Accept-FD-Registrations property value
 */
bool Network_Port_BBMD_Accept_FD_Registrations(uint32_t object_instance)
{
    bool flag = false;
    unsigned index = 0;
    struct bacnet_ipv4_port *ipv4 = NULL;
    struct bacnet_ipv6_port *ipv6 = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            ipv4 = &Object_List[index].Network.IPv4;
            flag = ipv4->BBMD_Accept_FD_Registrations;
        } else if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            ipv6 = &Object_List[index].Network.IPv6;
            flag = ipv6->BBMD_Accept_FD_Registrations;
        }
    }

    return flag;
}

/**
 * For a given object instance-number, sets the BBMD-Accept-FD-Registrations
 * property value
 *
 * @param object_instance - object-instance number of the object
 * @param flag - boolean changes-pending flag
 *
 * @return true if the BBMD-Accept-FD-Registrations property value was set
 */
bool Network_Port_BBMD_Accept_FD_Registrations_Set(
    uint32_t object_instance, bool flag)
{
    bool status = false;
    unsigned index = 0;
    struct bacnet_ipv4_port *ipv4 = NULL;
    struct bacnet_ipv6_port *ipv6 = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            ipv4 = &Object_List[index].Network.IPv4;
            if (flag != ipv4->BBMD_Accept_FD_Registrations) {
                ipv4->BBMD_Accept_FD_Registrations = flag;
                Object_List[index].Changes_Pending = true;
            }
            status = true;
        } else if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            ipv6 = &Object_List[index].Network.IPv6;
            if (flag != ipv6->BBMD_Accept_FD_Registrations) {
                ipv6->BBMD_Accept_FD_Registrations = flag;
                Object_List[index].Changes_Pending = true;
            }
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, returns the BBMD-BD-Table head
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  BBMD-Accept-FD-Registrations property value
 */
void *Network_Port_BBMD_BD_Table(uint32_t object_instance)
{
    void *bdt_head = NULL;
    unsigned index = 0;
    struct bacnet_ipv4_port *ipv4 = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        ipv4 = &Object_List[index].Network.IPv4;
        bdt_head = ipv4->BBMD_BD_Table;
    }

    return bdt_head;
}

/**
 * @brief For a given object instance-number, encodes the BBMD-BD-Table property
 * value
 * @param object_instance - object-instance number of the object
 * @param apdu - buffer to encode the property value into, or NULL for length
 * @return number of bytes encoded
 */
static int BBMD_Broadcast_Distribution_Table_Encode(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    unsigned index = 0;
    struct bacnet_ipv4_port *ipv4 = NULL;
    struct bacnet_ipv6_port *ipv6 = NULL;
    int apdu_len = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            ipv4 = &Object_List[index].Network.IPv4;
            apdu_len = bvlc_broadcast_distribution_table_encode(
                apdu, apdu_size, ipv4->BBMD_BD_Table);
        } else if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            ipv6 = &Object_List[index].Network.IPv6;
            apdu_len = bvlc6_broadcast_distribution_table_encode(
                apdu, apdu_size, ipv6->BBMD_BD_Table);
        }
    }

    return apdu_len;
}

/**
 * @brief Get the BACnetLIST capacity
 * @param object_instance [in] BACnet network port object instance number
 * @return capacity of the BACnetList (number of possible entries) or
 *  zero on error
 */
static size_t
BBMD_Broadcast_Distribution_Table_Capacity(uint32_t object_instance)
{
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list;
    size_t capacity = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            bdt_list = Object_List[index].Network.IPv4.BBMD_BD_Table;
            capacity = bvlc_broadcast_distribution_table_count(bdt_list);
        }
    }

    return capacity;
}

/**
 * @brief Decode a BACnetLIST property element to determine the element length
 * @param object_instance [in] BACnet network port object instance number
 * @param apdu [in] Buffer in which the APDU contents are extracted
 * @param apdu_size [in] The size of the APDU buffer
 * @return The length of the decoded apdu, or BACNET_STATUS_ERROR on error
 */
static int BBMD_Broadcast_Distribution_Table_Element_Length(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    int len = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            len = bvlc_decode_broadcast_distribution_table_entry(
                apdu, apdu_size, NULL);
        }
    }

    return len;
}

/**
 * @brief Write a value to a BACnetLIST property element value
 *  using a BACnetARRAY write utility function
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index to write:
 *    0=array size, 1 to N for individual array members
 * @param application_data [in] encoded element value
 * @param application_data_len [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE BBMD_Broadcast_Distribution_Table_Element_Write(
    uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    uint8_t *application_data,
    size_t application_data_len)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY bdt_entry = { 0 };
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list;
    uint16_t capacity = 0;
    int len;
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            bdt_list = Object_List[index].Network.IPv4.BBMD_BD_Table;
            capacity = bvlc_broadcast_distribution_table_count(bdt_list);
            if (array_index == 0) {
                error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
            } else if (array_index <= capacity) {
                len = bvlc_decode_broadcast_distribution_table_entry(
                    application_data, application_data_len, &bdt_entry);
                if (len > 0) {
                    status = bvlc_broadcast_distribution_table_entry_insert(
                        bdt_list, &bdt_entry, array_index);
                    if (status) {
                        error_code = ERROR_CODE_SUCCESS;
                    } else {
                        error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                } else {
                    error_code = ERROR_CODE_INVALID_DATA_TYPE;
                }
            } else {
                error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
        } else {
            error_code = ERROR_CODE_ABORT_OTHER;
        }
    }

    return error_code;
}

/**
 * For a given object instance-number, sets the BBMD-BD-Table head
 * property value
 *
 * @param object_instance - object-instance number of the object
 * @param bdt_head - Broadcast Distribution Table linked list head
 *
 * @return true if the Broadcast Distribution Table linked list head
 *  property value was set
 */
bool Network_Port_BBMD_BD_Table_Set(uint32_t object_instance, void *bdt_head)
{
    bool status = false;
    unsigned index = 0;
    struct bacnet_ipv4_port *ipv4 = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        ipv4 = &Object_List[index].Network.IPv4;
        if (bdt_head != ipv4->BBMD_BD_Table) {
            ipv4->BBMD_BD_Table = bdt_head;
            Object_List[index].Changes_Pending = true;
        }
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, returns the BBMD-FD-Table head
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  BBMD-Accept-FD-Registrations property value
 */
void *Network_Port_BBMD_FD_Table(uint32_t object_instance)
{
    void *fdt_head = NULL;
    unsigned index = 0;
    struct bacnet_ipv4_port *ipv4 = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        ipv4 = &Object_List[index].Network.IPv4;
        fdt_head = ipv4->BBMD_FD_Table;
    }

    return fdt_head;
}

/**
 * For a given object instance-number, sets the BBMD-FD-Table head
 * property value
 *
 * @param object_instance - object-instance number of the object
 * @param fdt_head - Foreign Device Table linked list head
 *
 * @return true if the BBMD-Accept-FD-Registrations property value was set
 */
bool Network_Port_BBMD_FD_Table_Set(uint32_t object_instance, void *fdt_head)
{
    bool status = false;
    unsigned index = 0;
    struct bacnet_ipv4_port *ipv4 = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        ipv4 = &Object_List[index].Network.IPv4;
        if (fdt_head != ipv4->BBMD_FD_Table) {
            ipv4->BBMD_FD_Table = fdt_head;
            Object_List[index].Changes_Pending = true;
        }
        status = true;
    }

    return status;
}

/**
 * @brief For a given object instance-number, encodes the BBMD-BD-Table property
 * value
 * @param object_instance - object-instance number of the object
 * @param apdu - buffer to encode the property value into, or NULL for length
 * @return number of bytes encoded
 */
static int BBMD_Foreign_Device_Table_Encode(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    unsigned index = 0;
    struct bacnet_ipv4_port *ipv4 = NULL;
    struct bacnet_ipv6_port *ipv6 = NULL;
    int apdu_len = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            ipv4 = &Object_List[index].Network.IPv4;
            apdu_len = bvlc_foreign_device_table_encode(
                apdu, apdu_size, ipv4->BBMD_FD_Table);
        } else if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            ipv6 = &Object_List[index].Network.IPv6;
            apdu_len = bvlc6_foreign_device_table_encode(
                apdu, apdu_size, ipv6->BBMD_FD_Table);
        }
    }

    return apdu_len;
}

/**
 * @brief Get the BACnetLIST capacity
 * @param object_instance [in] BACnet network port object instance number
 * @return capacity of the BACnetList (number of possible entries) or
 *  zero on error
 */
static size_t BBMD_Foreign_Device_Table_Capacity(uint32_t object_instance)
{
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list;
    size_t capacity = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            fdt_list = Object_List[index].Network.IPv4.BBMD_FD_Table;
            capacity = bvlc_foreign_device_table_count(fdt_list);
        }
    }

    return capacity;
}

/**
 * @brief Decode a BACnetLIST property element to determine the element length
 * @param object_instance [in] BACnet network port object instance number
 * @param apdu [in] Buffer in which the APDU contents are extracted
 * @param apdu_size [in] The size of the APDU buffer
 * @return The length of the decoded apdu, or BACNET_STATUS_ERROR on error
 */
static int BBMD_Foreign_Device_Table_Element_Length(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    int len = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            len = bvlc_decode_foreign_device_table_entry(apdu, apdu_size, NULL);
        }
    }

    return len;
}

/**
 * @brief Write a value to a BACnetLIST property element value
 *  using a BACnetARRAY write utility function
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index to write:
 *    0=array size, 1 to N for individual array members
 * @param application_data [in] encoded element value
 * @param application_data_len [in] The size of the encoded element value
 * @return BACNET_ERROR_CODE value
 */
static BACNET_ERROR_CODE BBMD_Foreign_Device_Table_Element_Write(
    uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    uint8_t *application_data,
    size_t application_data_len)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY fdt_entry = { 0 };
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list;
    uint16_t capacity = 0;
    int len;
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            fdt_list = Object_List[index].Network.IPv4.BBMD_FD_Table;
            capacity = bvlc_foreign_device_table_count(fdt_list);
            if (array_index == 0) {
                error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
            } else if (array_index <= capacity) {
                len = bvlc_decode_foreign_device_table_entry(
                    application_data, application_data_len, &fdt_entry);
                if (len > 0) {
                    status = bvlc_foreign_device_table_entry_insert(
                        fdt_list, &fdt_entry, array_index - 1);
                    if (status) {
                        error_code = ERROR_CODE_SUCCESS;
                    } else {
                        error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                } else {
                    error_code = ERROR_CODE_INVALID_DATA_TYPE;
                }
            } else {
                error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
        } else {
            error_code = ERROR_CODE_ABORT_OTHER;
        }
    }

    return error_code;
}

/**
 * @brief For a given object instance-number, gets the HostNPort
 * @note depends on Network_Type being set for this object
 * @param object_instance - object-instance number of the object
 * @param bbmd_address - BACNET_HOST_N_PORT structure
 * @return true if BBMD Address was copied
 */
bool Network_Port_Remote_BBMD_Address(
    uint32_t object_instance, BACNET_HOST_N_PORT *bbmd_address)
{
    bool status = false;
    unsigned index;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            status = host_n_port_from_minimal(
                bbmd_address, &Object_List[index].Network.IPv4.BBMD_Address);
        } else if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            status = host_n_port_from_minimal(
                bbmd_address, &Object_List[index].Network.IPv6.BBMD_Address);
        }
    }

    return status;
}

/**
 * @brief For a given object instance-number, sets the FD BBMD Address:
 * either as IP address or a hostname.
 * @note depends on Network_Type being set for this object
 * @param  object_instance - object-instance number of the object
 * @param  bbmd_address - BACNET_HOST_N_PORT FD_BBMD_Address
 * @return  true if BBMD Address was set
 */
bool Network_Port_Remote_BBMD_Address_Set(
    uint32_t object_instance, const BACNET_HOST_N_PORT *bbmd_address)
{
    bool status = false;
    BACNET_HOST_N_PORT_MINIMAL bbmd_address_minimal = { 0 };
    BACNET_HOST_N_PORT_MINIMAL *dest_bbmd_address = NULL;
    unsigned index;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            dest_bbmd_address = &Object_List[index].Network.IPv4.BBMD_Address;
        } else if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            dest_bbmd_address = &Object_List[index].Network.IPv6.BBMD_Address;
        }
        if (dest_bbmd_address) {
            status =
                host_n_port_to_minimal(&bbmd_address_minimal, bbmd_address);
            if (status) {
                if (!host_n_port_minimal_same(
                        dest_bbmd_address, &bbmd_address_minimal)) {
                    host_n_port_to_minimal(dest_bbmd_address, bbmd_address);
                    Object_List[index].Changes_Pending = true;
                }
            }
        }
    }

    return status;
}

/**
 * For a given object instance-number, loads the ip-address into
 * an octet string.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  a - ip-address first octet
 * @param  b - ip-address first octet
 * @param  c - ip-address first octet
 * @param  d - ip-address first octet
 *
 * @return  true if ip-address was retrieved
 */
bool Network_Port_Remote_BBMD_IP_Address(
    uint32_t object_instance, uint8_t *a, uint8_t *b, uint8_t *c, uint8_t *d)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;
    BACNET_HOST_N_PORT_MINIMAL *address;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            address = &Object_List[index].Network.IPv4.BBMD_Address;
            if (address->tag == BACNET_HOST_ADDRESS_TAG_IP_ADDRESS) {
                if (a) {
                    *a = address->host.ip_address.address[0];
                }
                if (b) {
                    *b = address->host.ip_address.address[1];
                }
                if (c) {
                    *c = address->host.ip_address.address[2];
                }
                if (d) {
                    *d = address->host.ip_address.address[3];
                }
                status = true;
            }
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the ip-address
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  a - ip-address first octet
 * @param  b - ip-address first octet
 * @param  c - ip-address first octet
 * @param  d - ip-address first octet
 *
 * @return  true if ip-address was set
 */
bool Network_Port_Remote_BBMD_IP_Address_Set(
    uint32_t object_instance, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;
    BACNET_HOST_N_PORT_MINIMAL *address;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            address = &Object_List[index].Network.IPv4.BBMD_Address;
            if ((address->host.ip_address.address[0] != a) ||
                (address->host.ip_address.address[1] != b) ||
                (address->host.ip_address.address[2] != c) ||
                (address->host.ip_address.address[3] != d) ||
                (address->tag != BACNET_HOST_ADDRESS_TAG_IP_ADDRESS)) {
                Object_List[index].Changes_Pending = true;
            }
            address->host.ip_address.address[0] = a;
            address->host.ip_address.address[1] = b;
            address->host.ip_address.address[2] = c;
            address->host.ip_address.address[3] = d;
            address->host.ip_address.length = 4;
            address->tag = BACNET_HOST_ADDRESS_TAG_IP_ADDRESS;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the BBMD UDP Port number
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return BBMD UDP Port number
 */
uint16_t Network_Port_Remote_BBMD_BIP_Port(uint32_t object_instance)
{
    uint16_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            value = Object_List[index].Network.IPv4.BBMD_Address.port;
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the BBMD UDP Port number
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BBMD UDP Port number (default=0xBAC0)
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_Remote_BBMD_BIP_Port_Set(
    uint32_t object_instance, uint16_t value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            if (Object_List[index].Network.IPv4.BBMD_Address.port != value) {
                Object_List[index].Changes_Pending = true;
            }
            Object_List[index].Network.IPv4.BBMD_Address.port = value;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the BBMD lifetime seconds
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return BBMD lifetime seconds
 */
uint16_t Network_Port_Remote_BBMD_BIP_Lifetime(uint32_t object_instance)
{
    uint16_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            value = Object_List[index].Network.IPv4.BBMD_Lifetime;
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the BBMD lifetime seconds
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BBMD lifetime seconds
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_Remote_BBMD_BIP_Lifetime_Set(
    uint32_t object_instance, uint16_t value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            if (Object_List[index].Network.IPv4.BBMD_Lifetime != value) {
                Object_List[index].Changes_Pending = true;
            }
            Object_List[index].Network.IPv4.BBMD_Lifetime = value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Get the foreign device subscription lifetime seconds
 * @param object_instance [in] BACnet network port object instance number
 * @return foreign device subscription BBMD lifetime seconds
 */
static uint16_t Foreign_Device_Subscription_Lifetime(uint32_t object_instance)
{
    uint16_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        switch (Object_List[index].Network_Type) {
            case PORT_TYPE_BIP:
                value = Object_List[index].Network.IPv4.BBMD_Lifetime;
                break;
            case PORT_TYPE_BIP6:
                value = Object_List[index].Network.IPv6.BBMD_Lifetime;
                break;
            default:
                break;
        }
    }

    return value;
}

/**
 * For a given object instance-number, returns the
 * BBMD-Accept-FD-Registrations property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  BBMD-Accept-FD-Registrations property value
 */
bool Network_Port_BBMD_IP6_Accept_FD_Registrations(uint32_t object_instance)
{
    bool flag = false;
    unsigned index = 0;
    struct bacnet_ipv6_port *ipv6 = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        ipv6 = &Object_List[index].Network.IPv6;
        flag = ipv6->BBMD_Accept_FD_Registrations;
    }

    return flag;
}

/**
 * For a given object instance-number, sets the BBMD-Accept-FD-Registrations
 * property value
 *
 * @param object_instance - object-instance number of the object
 * @param flag - boolean changes-pending flag
 *
 * @return true if the BBMD-Accept-FD-Registrations property value was set
 */
bool Network_Port_BBMD_IP6_Accept_FD_Registrations_Set(
    uint32_t object_instance, bool flag)
{
    return Network_Port_BBMD_Accept_FD_Registrations_Set(object_instance, flag);
}

/**
 * For a given object instance-number, returns the BBMD-BD-Table head
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  BBMD-Accept-FD-Registrations property value
 */
void *Network_Port_BBMD_IP6_BD_Table(uint32_t object_instance)
{
    void *bdt_head = NULL;
    unsigned index = 0;
    struct bacnet_ipv6_port *ipv6 = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        ipv6 = &Object_List[index].Network.IPv6;
        bdt_head = ipv6->BBMD_BD_Table;
    }

    return bdt_head;
}

/**
 * For a given object instance-number, sets the BBMD-BD-Table head
 * property value
 *
 * @param object_instance - object-instance number of the object
 * @param bdt_head - Broadcast Distribution Table linked list head
 *
 * @return true if the Broadcast Distribution Table linked list head
 *  property value was set
 */
bool Network_Port_BBMD_IP6_BD_Table_Set(
    uint32_t object_instance, void *bdt_head)
{
    bool status = false;
    unsigned index = 0;
    struct bacnet_ipv6_port *ipv6 = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        ipv6 = &Object_List[index].Network.IPv6;
        if (bdt_head != ipv6->BBMD_BD_Table) {
            ipv6->BBMD_BD_Table = bdt_head;
            Object_List[index].Changes_Pending = true;
        }
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, returns the BBMD-FD-Table head
 * property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  BBMD-Accept-FD-Registrations property value
 */
void *Network_Port_BBMD_IP6_FD_Table(uint32_t object_instance)
{
    void *fdt_head = NULL;
    unsigned index = 0;
    struct bacnet_ipv6_port *ipv6 = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        ipv6 = &Object_List[index].Network.IPv6;
        fdt_head = ipv6->BBMD_FD_Table;
    }

    return fdt_head;
}

/**
 * For a given object instance-number, sets the BBMD-FD-Table head
 * property value
 *
 * @param object_instance - object-instance number of the object
 * @param fdt_head - Foreign Device Table linked list head
 *
 * @return true if the BBMD-Accept-FD-Registrations property value was set
 */
bool Network_Port_BBMD_IP6_FD_Table_Set(
    uint32_t object_instance, void *fdt_head)
{
    bool status = false;
    unsigned index = 0;
    struct bacnet_ipv6_port *ipv6 = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        ipv6 = &Object_List[index].Network.IPv6;
        if (fdt_head != ipv6->BBMD_FD_Table) {
            ipv6->BBMD_FD_Table = fdt_head;
            Object_List[index].Changes_Pending = true;
        }
        status = true;
    }

    return status;
}

/**
 * For a given object instance-number, gets the ip-address and port
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  addr - holds the ip-address and port retrieved
 *
 * @return number of bytes encoded, or BACNET_STATUS_ERROR if error
 */
static int Foreign_Device_BBMD_Address_Encode(
    uint32_t object_instance, uint8_t *apdu, size_t apdu_size)
{
    int apdu_len = 0;
    BACNET_HOST_N_PORT bbmd_address = { 0 };

    Network_Port_Remote_BBMD_Address(object_instance, &bbmd_address);
    apdu_len = host_n_port_encode(NULL, &bbmd_address);
    if (apdu_len > apdu_size) {
        apdu_len = BACNET_STATUS_ERROR;
    } else {
        apdu_len = host_n_port_encode(apdu, &bbmd_address);
    }

    return apdu_len;
}

/**
 * For a given object instance-number, loads the ip-address into
 * an octet string.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  addr - pointer to IP6_ADDRESS_MAX = 16 octets buffer
 *
 * @return  true if ip-address was retrieved
 */
bool Network_Port_Remote_BBMD_IP6_Address(
    uint32_t object_instance, uint8_t *addr)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;
    BACNET_HOST_N_PORT_MINIMAL *address;
    size_t i;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            address = &Object_List[index].Network.IPv6.BBMD_Address;
            if (address->tag == BACNET_HOST_ADDRESS_TAG_IP_ADDRESS) {
                if (addr) {
                    for (i = 0; i < IP6_ADDRESS_MAX; i++) {
                        addr[i] = address->host.ip_address.address[i];
                    }
                }
                status = true;
            }
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the ip-address
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  addr - pointer to IP6_ADDRESS_MAX = 16 octets buffer
 *
 * @return  true if ip-address was set
 */
bool Network_Port_Remote_BBMD_IP6_Address_Set(
    uint32_t object_instance, const uint8_t *addr)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;
    BACNET_HOST_N_PORT_MINIMAL bbmd_address = { 0 };
    uint16_t port;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            port = Object_List[index].Network.IPv6.BBMD_Address.port;
            host_n_port_minimal_ip_init(
                &bbmd_address, port, addr, IP6_ADDRESS_MAX);
            if (!host_n_port_minimal_same(
                    &Object_List[index].Network.IPv6.BBMD_Address,
                    &bbmd_address)) {
                Object_List[index].Changes_Pending = true;
            }
            status = host_n_port_minimal_copy(
                &Object_List[index].Network.IPv6.BBMD_Address, &bbmd_address);
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the BBMD UDP Port number
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return BBMD UDP Port number
 */
uint16_t Network_Port_Remote_BBMD_BIP6_Port(uint32_t object_instance)
{
    uint16_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            value = Object_List[index].Network.IPv6.BBMD_Address.port;
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the BBMD UDP Port number
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BBMD UDP Port number (default=0xBAC0)
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_Remote_BBMD_BIP6_Port_Set(
    uint32_t object_instance, uint16_t value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            if (Object_List[index].Network.IPv6.BBMD_Address.port != value) {
                Object_List[index].Changes_Pending = true;
            }
            Object_List[index].Network.IPv6.BBMD_Address.port = value;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the BBMD lifetime seconds
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return BBMD lifetime seconds
 */
uint16_t Network_Port_Remote_BBMD_BIP6_Lifetime(uint32_t object_instance)
{
    uint16_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            value = Object_List[index].Network.IPv6.BBMD_Lifetime;
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the BBMD lifetime seconds
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BBMD lifetime seconds
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_Remote_BBMD_BIP6_Lifetime_Set(
    uint32_t object_instance, uint16_t value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            if (Object_List[index].Network.IPv6.BBMD_Lifetime != value) {
                Object_List[index].Changes_Pending = true;
            }
            Object_List[index].Network.IPv6.BBMD_Lifetime = value;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the BACnet/IP UDP Port number
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return BACnet/IP UDP Port number
 */
BACNET_IP_MODE Network_Port_BIP6_Mode(uint32_t object_instance)
{
    BACNET_IP_MODE value = BACNET_IP_MODE_NORMAL;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            value = Object_List[index].Network.IPv6.Mode;
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the BACnet/IP UDP Port number
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACnet/IP UDP Port number (default=0xBAC0)
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_BIP6_Mode_Set(uint32_t object_instance, BACNET_IP_MODE value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            if (Object_List[index].Network.IPv4.Mode != value) {
                Object_List[index].Changes_Pending = true;
            }
            Object_List[index].Network.IPv6.Mode = value;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, loads the ip-address into
 * an octet string.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  ip_address - holds the mac-address retrieved
 *
 * @return  true if ip-address was retrieved
 */
bool Network_Port_IPv6_Address(
    uint32_t object_instance, BACNET_OCTET_STRING *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            status = octetstring_init(
                ip_address, &Object_List[index].Network.IPv6.IP_Address[0],
                IPV6_ADDR_SIZE);
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the ip-address
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  ip-address - 16-byte IPv6 Address
 *
 * @return  true if ip-address was set
 */
bool Network_Port_IPv6_Address_Set(
    uint32_t object_instance, const uint8_t *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;
    unsigned i = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if ((Object_List[index].Network_Type == PORT_TYPE_BIP6) &&
            (ip_address)) {
            for (i = 0; i < IPV6_ADDR_SIZE; i++) {
                Object_List[index].Network.IPv6.IP_Address[i] = ip_address[i];
            }
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the BACnet/IP Subnet prefix
 * value Note: depends on Network_Type being set to PORT_TYPE_BIP for this
 * object
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return BACnet/IP subnet prefix value
 */
uint8_t Network_Port_IPv6_Subnet_Prefix(uint32_t object_instance)
{
    uint8_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            value = Object_List[index].Network.IPv6.IP_Subnet_Prefix;
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the BACnet/IP Subnet prefix
 * value Note: depends on Network_Type being set to PORT_TYPE_BIP for this
 * object
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACnet/IP Subnet prefix value 1..128
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_IPv6_Subnet_Prefix_Set(
    uint32_t object_instance, uint8_t value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            if ((value > 0) && (value <= 128)) {
                if (Object_List[index].Network.IPv6.IP_Subnet_Prefix != value) {
                    Object_List[index].Changes_Pending = true;
                }
                Object_List[index].Network.IPv6.IP_Subnet_Prefix = value;
                status = true;
            }
        }
    }

    return status;
}

/**
 * For a given object instance-number, loads the gateway ip-address into
 * an octet string.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  ip_address - holds the ip-address retrieved
 *
 * @return  true if ip-address was retrieved
 */
bool Network_Port_IPv6_Gateway(
    uint32_t object_instance, BACNET_OCTET_STRING *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            status = octetstring_init(
                ip_address, &Object_List[index].Network.IPv6.IP_Gateway[0],
                IPV6_ADDR_SIZE);
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the gateway ip-address
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  ip_address - 16 byte IPv6 address
 *
 * @return  true if ip-address was set
 */
bool Network_Port_IPv6_Gateway_Set(
    uint32_t object_instance, const uint8_t *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;
    unsigned i = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if ((Object_List[index].Network_Type == PORT_TYPE_BIP6) &&
            (ip_address)) {
            for (i = 0; i < IPV6_ADDR_SIZE; i++) {
                Object_List[index].Network.IPv6.IP_Gateway[i] = ip_address[i];
            }
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, loads the subnet-mask-address into
 * an octet string.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  dns_index - 0=primary, 1=secondary, 3=tertierary
 * @param  ip_address - holds the mac-address retrieved
 *
 * @return  true if ip-address was retrieved
 */
bool Network_Port_IPv6_DNS_Server(
    uint32_t object_instance,
    unsigned dns_index,
    BACNET_OCTET_STRING *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            if (dns_index < BIP_DNS_MAX) {
                status = octetstring_init(
                    ip_address,
                    &Object_List[index]
                         .Network.IPv6.IP_DNS_Server[dns_index][0],
                    IPV6_ADDR_SIZE);
            }
        }
    }

    return status;
}

/**
 * @brief Encode a BACnetARRAY property element; a function template
 * @param object_instance [in] BACnet network port object instance number
 * @param index [in] array index requested:
 *    0 to (array size - 1) for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or
 *    NULL to return the length of buffer if it had been built
 * @return The length of the apdu encoded, or
 *    BACNET_STATUS_ERROR for an invalid array index
 */
static int Network_Port_IPv6_DNS_Server_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX index, uint8_t *apdu)
{
    int apdu_len = 0;
    BACNET_OCTET_STRING ip_address = { 0 };

    if (index >= BIP_DNS_MAX) {
        apdu_len = BACNET_STATUS_ERROR;
    } else {
        if (Network_Port_IPv6_DNS_Server(object_instance, index, &ip_address)) {
            apdu_len = encode_application_octet_string(apdu, &ip_address);
        }
    }

    return apdu_len;
}

/**
 * For a given object instance-number, sets the ip-address
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  index - 0=primary, 1=secondary, 3=tertierary
 * @param  ip_address - 16 byte IPv6 address
 *
 * @return  true if ip-address was set
 */
bool Network_Port_IPv6_DNS_Server_Set(
    uint32_t object_instance, unsigned dns_index, const uint8_t *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;
    unsigned i = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if ((Object_List[index].Network_Type == PORT_TYPE_BIP6) &&
            (dns_index < BIP_DNS_MAX) && (ip_address)) {
            for (i = 0; i < IPV6_ADDR_SIZE; i++) {
                Object_List[index].Network.IPv6.IP_DNS_Server[dns_index][i] =
                    ip_address[i];
            }
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, loads the multicast ip-address into
 * an octet string.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  ip_address - holds the ip-address retrieved
 *
 * @return  true if ip-address was retrieved
 */
bool Network_Port_IPv6_Multicast_Address(
    uint32_t object_instance, BACNET_OCTET_STRING *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            status = octetstring_init(
                ip_address,
                &Object_List[index].Network.IPv6.IP_Multicast_Address[0],
                IPV6_ADDR_SIZE);
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the multicast ip-address
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  ip_address - 16 byte IPv6 address
 *
 * @return  true if ip-address was set
 */
bool Network_Port_IPv6_Multicast_Address_Set(
    uint32_t object_instance, const uint8_t *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;
    unsigned i = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if ((Object_List[index].Network_Type == PORT_TYPE_BIP6) &&
            (ip_address)) {
            for (i = 0; i < IPV6_ADDR_SIZE; i++) {
                Object_List[index].Network.IPv6.IP_Multicast_Address[i] =
                    ip_address[i];
            }
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, loads the DHCP server ip-address into
 * an octet string.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  ip_address - holds the ip-address retrieved
 *
 * @return  true if ip-address was retrieved
 */
bool Network_Port_IPv6_DHCP_Server(
    uint32_t object_instance, BACNET_OCTET_STRING *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            status = octetstring_init(
                ip_address, &Object_List[index].Network.IPv6.IP_DHCP_Server[0],
                IPV6_ADDR_SIZE);
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the DHCP server ip-address
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  ip_address - 16 byte IPv6 address
 *
 * @return  true if ip-address was set
 */
bool Network_Port_IPv6_DHCP_Server_Set(
    uint32_t object_instance, const uint8_t *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;
    unsigned i = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if ((Object_List[index].Network_Type == PORT_TYPE_BIP6) &&
            (ip_address)) {
            for (i = 0; i < IPV6_ADDR_SIZE; i++) {
                Object_List[index].Network.IPv6.IP_DHCP_Server[i] =
                    ip_address[i];
            }
            status = true;
        }
    }

    return status;
}

/**
 * @brief Get the current time from the Device object
 * @return current time in epoch seconds
 */
static bacnet_time_t Network_Port_Epoch_Seconds_Now(void)
{
    BACNET_DATE_TIME bdatetime = { 0 };

    datetime_local(&bdatetime.date, &bdatetime.time, NULL, NULL);
    return datetime_seconds_since_epoch(&bdatetime);
}

/**
 * For a given object instance-number, sets the IPv4_DHCP_Lease_Time
 * or IPv6_DHCP_Lease_Time property value in seconds.
 *
 * @note depends on Network_Type being set to
 * PORT_TYPE_BIP or PORT_TYPE_BIP6 for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - 32 bit IPv4 DHCP Lease Time in seconds
 *
 * @return IPv4_DHCP_Lease_Time
 */
bool Network_Port_IP_DHCP_Lease_Time_Set(
    uint32_t object_instance, const uint32_t value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        switch (Object_List[index].Network_Type) {
            case PORT_TYPE_BIP:
                if (Object_List[index].Network.IPv4.IP_DHCP_Lease_Seconds !=
                    value) {
                    Object_List[index].Changes_Pending = true;
                }
                Object_List[index].Network.IPv4.IP_DHCP_Lease_Seconds = value;
                Object_List[index].Network.IPv4.IP_DHCP_Lease_Seconds_Start =
                    Network_Port_Epoch_Seconds_Now();
                status = true;
                break;
            case PORT_TYPE_BIP6:
                if (Object_List[index].Network.IPv6.IP_DHCP_Lease_Seconds !=
                    value) {
                    Object_List[index].Changes_Pending = true;
                }
                Object_List[index].Network.IPv6.IP_DHCP_Lease_Seconds = value;
                Object_List[index].Network.IPv6.IP_DHCP_Lease_Seconds_Start =
                    Network_Port_Epoch_Seconds_Now();
                status = true;
                break;
            default:
                break;
        }
    }

    return status;
}

/**
 * @brief Get the DHCP lease time in seconds
 * @param object_instance [in] BACnet network port object instance number
 * @return DHCP lease time in seconds
 */
uint32_t Network_Port_IP_DHCP_Lease_Time(uint32_t object_instance)
{
    uint16_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        switch (Object_List[index].Network_Type) {
            case PORT_TYPE_BIP:
                value = Object_List[index].Network.IPv4.IP_DHCP_Lease_Seconds;
                break;
            case PORT_TYPE_BIP6:
                value = Object_List[index].Network.IPv6.IP_DHCP_Lease_Seconds;
                break;
            default:
                break;
        }
    }

    return value;
}

/**
 * @brief Get the DHCP lease time in seconds
 * @param object_instance [in] BACnet network port object instance number
 * @return DHCP lease time in seconds
 */
uint32_t Network_Port_IP_DHCP_Lease_Time_Remaining(uint32_t object_instance)
{
    uint32_t value = 0, elapsed_seconds = 0, seconds = 0, start_seconds = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        switch (Object_List[index].Network_Type) {
            case PORT_TYPE_BIP:
                seconds = Object_List[index].Network.IPv4.IP_DHCP_Lease_Seconds;
                if (seconds) {
                    start_seconds =
                        Object_List[index]
                            .Network.IPv4.IP_DHCP_Lease_Seconds_Start;
                    elapsed_seconds =
                        Network_Port_Epoch_Seconds_Now() - start_seconds;
                    if (elapsed_seconds < seconds) {
                        value = seconds - elapsed_seconds;
                    }
                }
                break;
            case PORT_TYPE_BIP6:
                seconds = Object_List[index].Network.IPv6.IP_DHCP_Lease_Seconds;
                if (seconds) {
                    start_seconds =
                        Object_List[index]
                            .Network.IPv6.IP_DHCP_Lease_Seconds_Start;
                    elapsed_seconds =
                        Network_Port_Epoch_Seconds_Now() - start_seconds;
                    if (elapsed_seconds < seconds) {
                        value = seconds - elapsed_seconds;
                    }
                }
                break;
            default:
                break;
        }
    }

    return value;
}

/**
 * @brief Get the the address of the DHCP server from which the last DHCP
 *  lease was obtained for the port. If the address of the DHCP server
 *  cannot be determined, the value of this property shall be X'00000000'.
 * @param object_instance [in] BACnet network port object instance number
 * @param ip_address [out] pointer to the IP address
 */
void Network_Port_IP_DHCP_Server(
    uint32_t object_instance, BACNET_OCTET_STRING *ip_address)
{
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        switch (Object_List[index].Network_Type) {
            case PORT_TYPE_BIP:
                octetstring_init(
                    ip_address,
                    &Object_List[index].Network.IPv4.IP_DHCP_Server[0],
                    IPV4_ADDR_SIZE);
                break;
            case PORT_TYPE_BIP6:
                octetstring_init(
                    ip_address,
                    &Object_List[index].Network.IPv6.IP_DHCP_Server[0],
                    IPV6_ADDR_SIZE);
                break;
            default:
                break;
        }
    }
}

/**
 * @brief For a given object instance-number, sets the DHCP server ip-address
 * @note depends on Network_Type being set for this object
 * @param  object_instance - object-instance number of the object
 * @param  ip_address - octet string of the DHCP server address
 * @return  true if ip-address was set
 */
bool Network_Port_IP_DHCP_Server_Set(
    uint32_t object_instance, BACNET_OCTET_STRING *ip_address)
{
    bool status = false;
    unsigned index = 0;
    BACNET_OCTET_STRING my_address = { 0 };

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        switch (Object_List[index].Network_Type) {
            case PORT_TYPE_BIP:
                /* check for changes */
                octetstring_init(
                    &my_address,
                    &Object_List[index].Network.IPv4.IP_DHCP_Server[0],
                    IPV4_ADDR_SIZE);
                if (!octetstring_value_same(&my_address, ip_address)) {
                    Object_List[index].Changes_Pending = true;
                }
                octetstring_copy_value(
                    &Object_List[index].Network.IPv4.IP_DHCP_Server[0],
                    IPV4_ADDR_SIZE, ip_address);
                status = true;
                break;
            case PORT_TYPE_BIP6:
                octetstring_init(
                    &my_address,
                    &Object_List[index].Network.IPv6.IP_DHCP_Server[0],
                    IPV6_ADDR_SIZE);
                if (!octetstring_value_same(&my_address, ip_address)) {
                    Object_List[index].Changes_Pending = true;
                }
                octetstring_copy_value(
                    &Object_List[index].Network.IPv6.IP_DHCP_Server[0],
                    IPV6_ADDR_SIZE, ip_address);
                status = true;
                break;
            default:
                break;
        }
    }

    return status;
}

/**
 * For a given object instance-number, gets the BACnet/IP UDP Port number
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return BACnet/IP UDP Port number
 */
uint16_t Network_Port_BIP6_Port(uint32_t object_instance)
{
    uint16_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            value = Object_List[index].Network.IPv6.Port;
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the BACnet/IP UDP Port number
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACnet/IP UDP Port number (default=0xBAC0)
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_BIP6_Port_Set(uint32_t object_instance, uint16_t value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            if (Object_List[index].Network.IPv6.Port != value) {
                Object_List[index].Changes_Pending = true;
            }
            Object_List[index].Network.IPv6.Port = value;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, loads the zone index string into
 * an character string. Zone index could be "eth0" or some other name.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  zone_index - holds the zone_index character string
 *
 * @return  true if zone_index was retrieved
 */
bool Network_Port_IPv6_Zone_Index(
    uint32_t object_instance, BACNET_CHARACTER_STRING *zone_index)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            status = characterstring_init_ansi(
                zone_index, &Object_List[index].Network.IPv6.Zone_Index[0]);
        }
    }

    return status;
}

/**
 * For a given object instance-number, returns the Zone index ASCII.
 * The Zone index could be "eth0" or some other name.
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @return  Zone index ASCII string
 */
const char *Network_Port_IPv6_Zone_Index_ASCII(uint32_t object_instance)
{
    const char *p = NULL;
    unsigned index = 0; /* offset from instance lookup */

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            p = &Object_List[index].Network.IPv6.Zone_Index[0];
        }
    }

    return p;
}

/**
 * For a given object instance-number, returns the BACnet IPv6 Auto
 * Addressing Enable property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return auto-Addressing-Enable property value
 */
bool Network_Port_IPv6_Auto_Addressing_Enable(uint32_t object_instance)
{
    bool flag = false;
    unsigned index = 0;
    struct bacnet_ipv6_port *ipv6 = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        ipv6 = &Object_List[index].Network.IPv6;
        flag = ipv6->IP_DHCP_Enable;
    }

    return flag;
}

/**
 * For a given object instance-number, sets the BACnet/IP6 Auto Addressing
 * Enable Note: depends on Network_Type being set to PORT_TYPE_BIP6 for this
 * object
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACnet/IP6 Audo Addressing Enable (default false)
 *
 * @return  true if values are within range and property is set.
 */
bool Network_Port_IPv6_Auto_Addressing_Enable_Set(
    uint32_t object_instance, bool value)
{
    bool status = false;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            if (Object_List[index].Network.IPv6.IP_DHCP_Enable != value) {
                Object_List[index].Changes_Pending = true;
            }
            Object_List[index].Network.IPv6.IP_DHCP_Enable = value;
            status = true;
        }
    }

    return status;
}

/**
 * For a given object instance-number, sets the gateway ip-address
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  ip_address - 16 byte IPv6 address
 *
 * @return  true if ip-address was set
 */
bool Network_Port_IPv6_Gateway_Zone_Index_Set(
    uint32_t object_instance, char *zone_index)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if ((Object_List[index].Network_Type == PORT_TYPE_BIP6) &&
            (zone_index)) {
            snprintf(
                &Object_List[index].Network.IPv6.Zone_Index[0], ZONE_INDEX_SIZE,
                "%s", zone_index);
            status = true;
        }
    }

    return status;
}

/**
 * @brief Write the FD BBMD Address
 * @param object_instance [in] BACnet network port object instance number
 * @param value [in] BACnet IP address and port
 * @param error_class [out] BACnet error class
 * @param error_code [out] BACnet error code
 * @return true if the value was written
 */
static bool Network_Port_FD_BBMD_Address_Write(
    uint32_t object_instance,
    const BACNET_HOST_N_PORT *value,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;

    if (!error_class || !error_code) {
        return status;
    }
    if (!value) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_INVALID_DATA_TYPE;
        return status;
    }
    switch (Network_Port_Type(object_instance)) {
        case PORT_TYPE_BIP:
            if (Network_Port_BIP_Mode(object_instance) !=
                BACNET_IP_MODE_FOREIGN) {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                break;
            } else if (
                ((value->host_ip_address) &&
                 (value->host.ip_address.length == 4)) ||
                ((value->host_name) &&
                 (bacnet_is_valid_hostname(&value->host.name)))) {
                status = Network_Port_Remote_BBMD_Address_Set(
                    object_instance, value);
            }
            if (!status) {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
            break;
        case PORT_TYPE_BIP6:
            if (Network_Port_BIP6_Mode(object_instance) !=
                BACNET_IP_MODE_FOREIGN) {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                break;
            } else if (
                ((value->host_ip_address) &&
                 (value->host.ip_address.length == 16)) ||
                ((value->host_name) &&
                 (bacnet_is_valid_hostname(&value->host.name)))) {
                status = Network_Port_Remote_BBMD_Address_Set(
                    object_instance, value);
            }
            if (!status) {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
            break;
        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }

    return status;
}

/**
 * @brief Write the FD Subscription Lifetime
 * @param object_instance [in] BACnet network port object instance number
 * @param value [in] BACnet IP address and port
 * @param error_class [out] BACnet error class
 * @param error_code [out] BACnet error code
 * @return true if the value was written
 */
static bool Network_Port_FD_Subscription_Lifetime_Write(
    uint32_t object_instance,
    BACNET_UNSIGNED_INTEGER value,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    uint16_t lifetime = 0;

    if (!error_class || !error_code) {
        return status;
    }
    if (value > UINT16_MAX) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return status;
    }
    lifetime = (uint16_t)value;
    switch (Network_Port_Type(object_instance)) {
        case PORT_TYPE_BIP:
            if (Network_Port_BIP_Mode(object_instance) ==
                BACNET_IP_MODE_FOREIGN) {
                status = Network_Port_Remote_BBMD_BIP_Lifetime_Set(
                    object_instance, lifetime);
                if (!status) {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
            break;
        case PORT_TYPE_BIP6:
            if (Network_Port_BIP6_Mode(object_instance) ==
                BACNET_IP_MODE_FOREIGN) {
                status = Network_Port_Remote_BBMD_BIP6_Lifetime_Set(
                    object_instance, lifetime);
                if (!status) {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
            break;
        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }

    return status;
}

/**
 * For a given object instance-number, gets the MS/TP Max_Info_Frames value
 * Note: depends on Network_Type being set to PORT_TYPE_MSTP for this object
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return MS/TP Max_Info_Frames value
 */
uint8_t Network_Port_MSTP_Max_Info_Frames(uint32_t object_instance)
{
    uint8_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_MSTP) {
            value = Object_List[index].Network.MSTP.Max_Info_Frames;
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the MS/TP Max_Info_Frames value
 * Note: depends on Network_Type being set to PORT_TYPE_MSTP for this object
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
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_MSTP) {
            if (Object_List[index].Network.MSTP.Max_Info_Frames != value) {
                Object_List[index].Changes_Pending = true;
            }
            Object_List[index].Network.MSTP.Max_Info_Frames = value;
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
static int MSTP_Link_Speeds_Element_Encode(
    const uint32_t object_instance,
    const BACNET_ARRAY_INDEX array_index,
    uint8_t *const apdu)
{
    (void)object_instance;
    int apdu_len = BACNET_STATUS_ERROR;

    if (array_index < MSTP_LINK_SPEEDS_MAX) {
        apdu_len =
            encode_application_real(apdu, (float)MSTP_LINK_SPEEDS[array_index]);
    }

    return apdu_len;
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
    int apdu_len = 0;
    int apdu_size = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_OCTET_STRING octet_string;
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    if (!Property_List_Member(
            rpdata->object_instance, rpdata->object_property)) {
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
                &apdu[0], BACNET_PROTOCOL_LEVEL_BACNET_APPLICATION);
            break;
        case PROP_NETWORK_NUMBER:
            apdu_len = encode_application_unsigned(
                &apdu[0], Network_Port_Network_Number(rpdata->object_instance));
            break;
        case PROP_NETWORK_NUMBER_QUALITY:
            apdu_len = encode_application_enumerated(
                &apdu[0], Network_Port_Quality(rpdata->object_instance));
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(
                &char_string,
                Network_Port_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_MAC_ADDRESS:
            Network_Port_MAC_Address(rpdata->object_instance, &octet_string);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_LINK_SPEED:
            apdu_len = encode_application_real(
                &apdu[0], Network_Port_Link_Speed(rpdata->object_instance));
            break;
        case PROP_LINK_SPEEDS:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                MSTP_Link_Speeds_Element_Encode, MSTP_LINK_SPEEDS_MAX, apdu,
                apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_CHANGES_PENDING:
            apdu_len = encode_application_boolean(
                &apdu[0],
                Network_Port_Changes_Pending(rpdata->object_instance));
            break;
        case PROP_APDU_LENGTH:
            apdu_len = encode_application_unsigned(
                &apdu[0], Network_Port_APDU_Length(rpdata->object_instance));
            break;
        case PROP_MAX_MASTER:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_MSTP_Max_Master(rpdata->object_instance));
            break;
        case PROP_MAX_INFO_FRAMES:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_MSTP_Max_Info_Frames(rpdata->object_instance));
            break;
        case PROP_BACNET_IP_MODE:
            apdu_len = encode_application_enumerated(
                &apdu[0], Network_Port_BIP_Mode(rpdata->object_instance));
            break;
        case PROP_IP_ADDRESS:
            Network_Port_IP_Address(rpdata->object_instance, &octet_string);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_BACNET_IP_UDP_PORT:
            apdu_len = encode_application_unsigned(
                &apdu[0], Network_Port_BIP_Port(rpdata->object_instance));
            break;
        case PROP_IP_SUBNET_MASK:
            Network_Port_IP_Subnet(rpdata->object_instance, &octet_string);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_IP_DEFAULT_GATEWAY:
            Network_Port_IP_Gateway(rpdata->object_instance, &octet_string);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_IP_DHCP_ENABLE:
            apdu_len = encode_application_boolean(
                &apdu[0], Network_Port_IP_DHCP_Enable(rpdata->object_instance));
            break;
        case PROP_IP_DHCP_LEASE_TIME:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_IP_DHCP_Lease_Time(rpdata->object_instance));
            break;
        case PROP_IP_DHCP_LEASE_TIME_REMAINING:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_IP_DHCP_Lease_Time_Remaining(
                    rpdata->object_instance));
            break;
        case PROP_IP_DHCP_SERVER:
            Network_Port_IP_DHCP_Server(rpdata->object_instance, &octet_string);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_IP_DNS_SERVER:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Network_Port_IP_DNS_Server_Encode, BIP_DNS_MAX, apdu,
                apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
        case PROP_BBMD_ACCEPT_FD_REGISTRATIONS:
            apdu_len = encode_application_boolean(
                &apdu[0],
                Network_Port_BBMD_Accept_FD_Registrations(
                    rpdata->object_instance));
            break;
        case PROP_BBMD_BROADCAST_DISTRIBUTION_TABLE:
            /* BACnetLIST */
            apdu_len = BBMD_Broadcast_Distribution_Table_Encode(
                rpdata->object_instance, apdu, apdu_size);
            break;
        case PROP_BBMD_FOREIGN_DEVICE_TABLE:
            /* BACnetLIST */
            apdu_len = BBMD_Foreign_Device_Table_Encode(
                rpdata->object_instance, apdu, apdu_size);
            break;
        case PROP_FD_BBMD_ADDRESS:
            apdu_len = Foreign_Device_BBMD_Address_Encode(
                rpdata->object_instance, apdu, apdu_size);
            break;
        case PROP_FD_SUBSCRIPTION_LIFETIME:
            apdu_len = encode_application_unsigned(
                apdu,
                Foreign_Device_Subscription_Lifetime(rpdata->object_instance));
            break;
        case PROP_BACNET_IPV6_MODE:
            apdu_len = encode_application_enumerated(
                &apdu[0], Network_Port_BIP6_Mode(rpdata->object_instance));
            break;
        case PROP_IPV6_ADDRESS:
            Network_Port_IPv6_Address(rpdata->object_instance, &octet_string);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_IPV6_PREFIX_LENGTH:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_IPv6_Subnet_Prefix(rpdata->object_instance));
            break;
        case PROP_BACNET_IPV6_UDP_PORT:
            apdu_len = encode_application_unsigned(
                &apdu[0], Network_Port_BIP6_Port(rpdata->object_instance));
            break;
        case PROP_IPV6_DEFAULT_GATEWAY:
            Network_Port_IPv6_Gateway(rpdata->object_instance, &octet_string);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_BACNET_IPV6_MULTICAST_ADDRESS:
            Network_Port_IPv6_Multicast_Address(
                rpdata->object_instance, &octet_string);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_IPV6_DNS_SERVER:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Network_Port_IPv6_DNS_Server_Encode, BIP_DNS_MAX, apdu,
                apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
        case PROP_IPV6_AUTO_ADDRESSING_ENABLE:
            apdu_len = encode_application_boolean(
                &apdu[0],
                Network_Port_IPv6_Auto_Addressing_Enable(
                    rpdata->object_instance));
            break;
        case PROP_IPV6_DHCP_LEASE_TIME:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_IP_DHCP_Lease_Time(rpdata->object_instance));
            break;
        case PROP_IPV6_DHCP_LEASE_TIME_REMAINING:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_IP_DHCP_Lease_Time_Remaining(
                    rpdata->object_instance));
            break;
        case PROP_IPV6_DHCP_SERVER:
            Network_Port_IPv6_DHCP_Server(
                rpdata->object_instance, &octet_string);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_IPV6_ZONE_INDEX:
            Network_Port_IPv6_Zone_Index(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
#ifdef BACDL_BSC
        case PROP_MAX_BVLC_LENGTH_ACCEPTED:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_Max_BVLC_Length_Accepted(rpdata->object_instance));
            break;
        case PROP_MAX_NPDU_LENGTH_ACCEPTED:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_Max_NPDU_Length_Accepted(rpdata->object_instance));
            break;
        case PROP_SC_PRIMARY_HUB_URI:
            Network_Port_SC_Primary_Hub_URI(
                rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_SC_FAILOVER_HUB_URI:
            Network_Port_SC_Failover_Hub_URI(
                rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_SC_MINIMUM_RECONNECT_TIME:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_SC_Minimum_Reconnect_Time(
                    rpdata->object_instance));
            break;
        case PROP_SC_MAXIMUM_RECONNECT_TIME:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_SC_Maximum_Reconnect_Time(
                    rpdata->object_instance));
            break;
        case PROP_SC_CONNECT_WAIT_TIMEOUT:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_SC_Connect_Wait_Timeout(rpdata->object_instance));
            break;
        case PROP_SC_DISCONNECT_WAIT_TIMEOUT:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_SC_Disconnect_Wait_Timeout(
                    rpdata->object_instance));
            break;
        case PROP_SC_HEARTBEAT_TIMEOUT:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_SC_Heartbeat_Timeout(rpdata->object_instance));
            break;
        case PROP_SC_HUB_CONNECTOR_STATE:
            apdu_len = encode_application_enumerated(
                &apdu[0],
                Network_Port_SC_Hub_Connector_State(rpdata->object_instance));
            break;
        case PROP_OPERATIONAL_CERTIFICATE_FILE:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_Operational_Certificate_File(
                    rpdata->object_instance));
            break;
        case PROP_ISSUER_CERTIFICATE_FILES:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Network_Port_Issuer_Certificate_File_Encode,
                BACNET_ISSUER_CERT_FILE_MAX, apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_CERTIFICATE_SIGNING_REQUEST_FILE:
            apdu_len = encode_application_unsigned(
                &apdu[0],
                Network_Port_Certificate_Signing_Request_File(
                    rpdata->object_instance));
            break;
            /* SC optionals */
#if BACNET_SECURE_CONNECT_ROUTING_TABLE
        case PROP_ROUTING_TABLE:
            apdu_len = Network_Port_Routing_Table_Encode(
                rpdata->object_instance, apdu, apdu_size);
            break;
#endif /* BACNET_SECURE_CONNECT_ROUTING_TABLE */
#if BSC_CONF_HUB_FUNCTIONS_NUM != 0
        case PROP_SC_PRIMARY_HUB_CONNECTION_STATUS:
            apdu_len = bacapp_encode_SCHubConnection(
                &apdu[0],
                Network_Port_SC_Primary_Hub_Connection_Status(
                    rpdata->object_instance));
            break;
        case PROP_SC_FAILOVER_HUB_CONNECTION_STATUS:
            apdu_len = bacapp_encode_SCHubConnection(
                &apdu[0],
                Network_Port_SC_Failover_Hub_Connection_Status(
                    rpdata->object_instance));
            break;
        case PROP_SC_HUB_FUNCTION_ENABLE:
            apdu_len = encode_application_boolean(
                &apdu[0],
                Network_Port_SC_Hub_Function_Enable(rpdata->object_instance));
            break;
        case PROP_SC_HUB_FUNCTION_ACCEPT_URIS:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Network_Port_SC_Hub_Function_Accept_URI_Encode,
                BACNET_SC_DIRECT_ACCEPT_URI_MAX, apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_SC_HUB_FUNCTION_BINDING:
            Network_Port_SC_Hub_Function_Binding(
                rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_SC_HUB_FUNCTION_CONNECTION_STATUS:
            apdu_len = Network_Port_SC_Hub_Function_Connection_Status_Encode(
                rpdata->object_instance, apdu, apdu_size);
            break;
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM != 0
        case PROP_SC_DIRECT_CONNECT_INITIATE_ENABLE:
            apdu_len = encode_application_boolean(
                &apdu[0],
                Network_Port_SC_Direct_Connect_Initiate_Enable(
                    rpdata->object_instance));
            break;
        case PROP_SC_DIRECT_CONNECT_ACCEPT_ENABLE:
            apdu_len = encode_application_boolean(
                &apdu[0],
                Network_Port_SC_Direct_Connect_Accept_Enable(
                    rpdata->object_instance));
            break;
        case PROP_SC_DIRECT_CONNECT_ACCEPT_URIS:
            apdu_len = bacnet_array_encode(
                rpdata->object_instance, rpdata->array_index,
                Network_Port_SC_Direct_Connect_Accept_URI_Encode,
                BACNET_SC_DIRECT_ACCEPT_URI_MAX, apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_SC_DIRECT_CONNECT_BINDING:
            Network_Port_SC_Direct_Connect_Binding(
                rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_SC_DIRECT_CONNECT_CONNECTION_STATUS:
            apdu_len = Network_Port_SC_Direct_Connect_Connection_Status_Encode(
                rpdata->object_instance, apdu, apdu_size);
            break;
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */
        case PROP_SC_FAILED_CONNECTION_REQUESTS:
            apdu_len = Network_Port_SC_Failed_Connection_Requests_Encode(
                rpdata->object_instance, apdu, apdu_size);
            break;
#endif /* BACDL_BSC */
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
bool Network_Port_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    uint32_t capacity;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    if (!Network_Port_Valid_Instance(wp_data->object_instance)) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }
    if (!Property_List_Member(
            wp_data->object_instance, wp_data->object_property)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
        return false;
    }
    /* decode the some of the request */
#if defined(BACAPP_COMPLEX_TYPES)
    len = bacapp_decode_known_property(
        wp_data->application_data, wp_data->application_data_len, &value,
        wp_data->object_type, wp_data->object_property);
#else
    len = bacapp_decode_application_data(
        wp_data->application_data, wp_data->application_data_len, &value);
#endif
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    /* FIXME: len < application_data_len: more data? */
    switch (wp_data->object_property) {
        case PROP_MAC_ADDRESS:
            if (Network_Port_Type(wp_data->object_instance) == PORT_TYPE_MSTP) {
                status = write_property_type_valid(
                    wp_data, &value, BACNET_APPLICATION_TAG_OCTET_STRING);
                if (status) {
                    status = Network_Port_MAC_Address_Set(
                        wp_data->object_instance, value.type.Octet_String.value,
                        value.type.Octet_String.length);
                    if (!status) {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
            break;
        case PROP_MAX_MASTER:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
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
            }
            break;
        case PROP_MAX_INFO_FRAMES:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
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
            }
            break;
        case PROP_LINK_SPEED:
            if (Network_Port_Type(wp_data->object_instance) == PORT_TYPE_MSTP) {
                status = write_property_type_valid(
                    wp_data, &value, BACNET_APPLICATION_TAG_REAL);
                if (status) {
                    status = Network_Port_Link_Speed_Set(
                        wp_data->object_instance, value.type.Real);
                    if (!status) {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                }
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
            break;
        case PROP_FD_BBMD_ADDRESS:
            if (write_property_type_valid(
                    wp_data, &value, BACNET_APPLICATION_TAG_HOST_N_PORT)) {
                status = Network_Port_FD_BBMD_Address_Write(
                    wp_data->object_instance, &value.type.Host_Address,
                    &wp_data->error_class, &wp_data->error_code);
            }
            break;
        case PROP_FD_SUBSCRIPTION_LIFETIME:
            if (write_property_type_valid(
                    wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT)) {
                status = Network_Port_FD_Subscription_Lifetime_Write(
                    wp_data->object_instance, value.type.Unsigned_Int,
                    &wp_data->error_class, &wp_data->error_code);
            }
            break;
        case PROP_BBMD_ACCEPT_FD_REGISTRATIONS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                status = Network_Port_BBMD_Accept_FD_Registrations_Set(
                    wp_data->object_instance, value.type.Boolean);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_BBMD_BROADCAST_DISTRIBUTION_TABLE:
            /* BACnetLIST */
            capacity = BBMD_Broadcast_Distribution_Table_Capacity(
                wp_data->object_instance);
            wp_data->error_code = bacnet_array_write(
                wp_data->object_instance, wp_data->array_index,
                BBMD_Broadcast_Distribution_Table_Element_Length,
                BBMD_Broadcast_Distribution_Table_Element_Write, capacity,
                wp_data->application_data, wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
            }
            break;
        case PROP_BBMD_FOREIGN_DEVICE_TABLE:
            /* BACnetLIST */
            capacity =
                BBMD_Foreign_Device_Table_Capacity(wp_data->object_instance);
            wp_data->error_code = bacnet_array_write(
                wp_data->object_instance, wp_data->array_index,
                BBMD_Foreign_Device_Table_Element_Length,
                BBMD_Foreign_Device_Table_Element_Write, capacity,
                wp_data->application_data, wp_data->application_data_len);
            if (wp_data->error_code == ERROR_CODE_SUCCESS) {
                status = true;
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
    if (!status && (wp_data->error_code == ERROR_CODE_OTHER)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
    }

    return status;
}

/**
 * ReadRange service handler for the BACnet/IP BDT.
 *
 * @param  apdu - place to encode the data
 * @param  pRequest - BACNET_READ_RANGE_DATA data
 *
 * @return number of bytes encoded
 */
int Network_Port_Read_Range_BDT(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest)
{
    (void)apdu;
    (void)pRequest;
    return 0;
}

/**
 * ReadRange service handler for the BACnet/IP FDT.
 *
 * @param  apdu - place to encode the data
 * @param  pRequest - BACNET_READ_RANGE_DATA data
 *
 * @return number of bytes encoded
 */
int Network_Port_Read_Range_FDT(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest)
{
    (void)apdu;
    (void)pRequest;
    return 0;
}

/**
 * ReadRange service handler
 *
 * @param  apdu - place to encode the data
 * @param  pInfo - RR_PROP_INFO data
 *
 * @return number of bytes encoded
 */
bool Network_Port_Read_Range(
    BACNET_READ_RANGE_DATA *pRequest, RR_PROP_INFO *pInfo)
{
    /* return value */
    bool status = false;

    if (Property_List_Member(
            pRequest->object_instance, pRequest->object_property)) {
        if (property_list_bacnet_list_member(
                OBJECT_NETWORK_PORT, pRequest->object_property)) {
            switch (pRequest->object_property) {
                case PROP_BBMD_BROADCAST_DISTRIBUTION_TABLE:
                    pInfo->RequestTypes = RR_BY_POSITION;
                    pInfo->Handler = Network_Port_Read_Range_BDT;
                    status = true;
                    break;
                case PROP_BBMD_FOREIGN_DEVICE_TABLE:
                    pInfo->RequestTypes = RR_BY_POSITION;
                    pInfo->Handler = Network_Port_Read_Range_FDT;
                    status = true;
                    break;
                default:
                    pRequest->error_class = ERROR_CLASS_PROPERTY;
                    pRequest->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
                    break;
            }
        } else {
            pRequest->error_class = ERROR_CLASS_SERVICES;
            pRequest->error_code = ERROR_CODE_PROPERTY_IS_NOT_A_LIST;
        }
    } else {
        pRequest->error_class = ERROR_CLASS_PROPERTY;
        pRequest->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
    }

    return status;
}

/**
 * @brief Activate any of the changes pending for all network port objects
 */
void Network_Port_Changes_Activate(void)
{
    unsigned i = 0;

    for (i = 0; i < BACNET_NETWORK_PORTS_MAX; i++) {
        if (Object_List[i].Changes_Pending) {
            Network_Port_Changes_Pending_Activate(i);
            Object_List[i].Changes_Pending = false;
        }
    }
}

/**
 * @brief Discard any of the changes pending for all network port objects
 */
void Network_Port_Changes_Discard(void)
{
    unsigned i = 0;

    for (i = 0; i < BACNET_NETWORK_PORTS_MAX; i++) {
        if (Object_List[i].Changes_Pending) {
            Network_Port_Changes_Pending_Discard(i);
            Object_List[i].Changes_Pending = false;
        }
    }
}

/**
 * @brief Cleanup - useful if network port object are allocated on the heap
 */
void Network_Port_Cleanup(void)
{
#if defined(BACDL_BSC) && defined(BACNET_SECURE_CONNECT_ROUTING_TABLE)
    unsigned index = 0;
    for (index = 0; index < BACNET_NETWORK_PORTS_MAX; index++) {
        BACNET_SC_PARAMS *sc = &Object_List[index].Network.BSC.Parameters;
        if (sc->Routing_Table) {
            Keylist_Delete(sc->Routing_Table);
            sc->Routing_Table = NULL;
        }
    }
#endif
}

/**
 * Initializes the Network Port object data
 */
void Network_Port_Init(void)
{
    unsigned index = 0;

#ifdef BACDL_BSC
    BACNET_SC_PARAMS *sc;
#endif /* BACDL_BSC */

    /* do something interesting */

    for (index = 0; index < BACNET_NETWORK_PORTS_MAX; index++) {
        memset(&Object_List[index], 0, sizeof(Object_List[index]));
#ifdef BACDL_BSC
        Object_List[index].Network_Type = PORT_TYPE_BSC;
        sc = &Object_List[index].Network.BSC.Parameters;
        Object_List[index].Activate_Changes =
            Network_Port_SC_Pending_Params_Apply;
        Object_List[index].Discard_Changes =
            Network_Port_SC_Pending_Params_Discard;
#ifdef BACNET_SECURE_CONNECT_ROUTING_TABLE
        sc->Routing_Table = Keylist_Create();
#endif
        sc->SC_Failed_Connection_Requests_Count = 0;
#if BSC_CONF_HUB_FUNCTIONS_NUM != 0
        sc->SC_Hub_Function_Connection_Status_Count = 0;
#endif
#if BSC_CONF_HUB_CONNECTORS_NUM != 0
        sc->SC_Direct_Connect_Connection_Status_Count = 0;
#endif
        (void)sc;
#endif /* BACDL_BSC */
    }
}

#ifdef BACDL_BSC
/**
 * For a given object instance-number, gets SC parameters structure
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return SC params structure
 */
BACNET_SC_PARAMS *Network_Port_SC_Params(uint32_t object_instance)
{
    BACNET_SC_PARAMS *param = NULL;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BSC) {
            param = &Object_List[index].Network.BSC.Parameters;
        }
    }

    return param;
}
#endif
