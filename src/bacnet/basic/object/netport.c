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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "bacnet/config.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacint.h"
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/object/device.h"
/* me */
#include "bacnet/basic/object/netport.h"
#include <bacnet/basic/object/netport_internal.h>

#ifndef BBMD_ENABLED
#define BBMD_ENABLED 1
#endif

#define BIP_DNS_MAX 3
struct bacnet_ipv4_port {
    uint8_t IP_Address[4];
    uint8_t IP_Subnet_Prefix;
    uint8_t IP_Gateway[4];
    uint8_t IP_DNS_Server[BIP_DNS_MAX][4];
    uint16_t Port;
    BACNET_IP_MODE Mode;
    bool IP_DHCP_Enable;
    uint32_t IP_DHCP_Lease_Seconds;
    uint32_t IP_DHCP_Lease_Seconds_Remaining;
    uint8_t IP_DHCP_Server[4];
    bool IP_NAT_Traversal;
    uint32_t IP_Global_Address[4];
    bool BBMD_Accept_FD_Registrations;
    void *BBMD_BD_Table;
    void *BBMD_FD_Table;
    /* used for foreign device registration to remote BBMD */
    uint8_t BBMD_IP_Address[4];
    uint16_t BBMD_Port;
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
    uint8_t IP_DHCP_Server[IPV6_ADDR_SIZE];
    uint16_t Port;
    BACNET_IP_MODE Mode;
    char Zone_Index[ZONE_INDEX_SIZE];
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
    char *Object_Name;
    BACNET_RELIABILITY Reliability;
    bool Out_Of_Service : 1;
    bool Changes_Pending : 1;
    uint8_t Network_Type;
    uint16_t Network_Number;
    BACNET_PORT_QUALITY Quality;
    uint16_t APDU_Length;
    float Link_Speed;
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

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Network_Port_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_STATUS_FLAGS, PROP_RELIABILITY,
    PROP_OUT_OF_SERVICE, PROP_NETWORK_TYPE, PROP_PROTOCOL_LEVEL,
    PROP_CHANGES_PENDING, -1 };

static const int Ethernet_Port_Properties_Optional[] = { PROP_NETWORK_NUMBER,
    PROP_NETWORK_NUMBER_QUALITY, PROP_APDU_LENGTH, PROP_MAC_ADDRESS, -1 };

static const int MSTP_Port_Properties_Optional[] = { PROP_NETWORK_NUMBER,
    PROP_NETWORK_NUMBER_QUALITY, PROP_APDU_LENGTH, PROP_MAC_ADDRESS,
    PROP_MAX_MASTER, PROP_MAX_INFO_FRAMES, -1 };

static const int BIP_Port_Properties_Optional[] = { PROP_NETWORK_NUMBER,
    PROP_NETWORK_NUMBER_QUALITY, PROP_APDU_LENGTH, PROP_MAC_ADDRESS,
    PROP_BACNET_IP_MODE, PROP_IP_ADDRESS, PROP_BACNET_IP_UDP_PORT,
    PROP_IP_SUBNET_MASK, PROP_IP_DEFAULT_GATEWAY, PROP_IP_DNS_SERVER,
#if defined(BACDL_BIP) && BBMD_ENABLED
    PROP_BBMD_ACCEPT_FD_REGISTRATIONS, PROP_BBMD_BROADCAST_DISTRIBUTION_TABLE,
    PROP_BBMD_FOREIGN_DEVICE_TABLE, PROP_FD_BBMD_ADDRESS,
    PROP_FD_SUBSCRIPTION_LIFETIME,
#endif
    -1 };

static const int BIP6_Port_Properties_Optional[] = { PROP_NETWORK_NUMBER,
    PROP_NETWORK_NUMBER_QUALITY, PROP_APDU_LENGTH, PROP_MAC_ADDRESS,
    PROP_BACNET_IPV6_MODE, PROP_IPV6_ADDRESS, PROP_IPV6_PREFIX_LENGTH,
    PROP_BACNET_IPV6_UDP_PORT, PROP_IPV6_DEFAULT_GATEWAY,
    PROP_BACNET_IPV6_MULTICAST_ADDRESS, PROP_IPV6_DNS_SERVER,
    PROP_IPV6_AUTO_ADDRESSING_ENABLE, PROP_IPV6_DHCP_LEASE_TIME,
    PROP_IPV6_DHCP_LEASE_TIME_REMAINING, PROP_IPV6_DHCP_SERVER,
    PROP_IPV6_ZONE_INDEX, -1 };

static const int BSC_Port_Properties_Optional[] = { PROP_NETWORK_NUMBER,
    PROP_NETWORK_NUMBER_QUALITY, PROP_APDU_LENGTH, PROP_MAC_ADDRESS,
    PROP_BACNET_IP_MODE, PROP_IP_ADDRESS, PROP_BACNET_IP_UDP_PORT,
    PROP_IP_SUBNET_MASK, PROP_IP_DEFAULT_GATEWAY, PROP_IP_DNS_SERVER,
    PROP_MAX_BVLC_LENGTH_ACCEPTED, PROP_MAX_NPDU_LENGTH_ACCEPTED,
    PROP_SC_PRIMARY_HUB_URI, PROP_SC_FAILOVER_HUB_URI,
    PROP_SC_MINIMUM_RECONNECT_TIME, PROP_SC_MAXIMUM_RECONNECT_TIME,
    PROP_SC_CONNECT_WAIT_TIMEOUT, PROP_SC_DISCONNECT_WAIT_TIMEOUT,
    PROP_SC_HEARTBEAT_TIMEOUT, PROP_SC_HUB_CONNECTOR_STATE,
    PROP_OPERATIONAL_CERTIFICATE_FILE, PROP_ISSUER_CERTIFICATE_FILES,
    PROP_CERTIFICATE_SIGNING_REQUEST_FILE,
/*SC optional*/
#ifdef BACNET_SECURE_CONNECT_ROUTING_TABLE
    PROP_ROUTING_TABLE,
#endif /* BACNET_SECURE_CONNECT_ROUTING_TABLE */
#if BSC_CONF_HUB_FUNCTIONS_NUM != 0
    PROP_SC_PRIMARY_HUB_CONNECTION_STATUS,
    PROP_SC_FAILOVER_HUB_CONNECTION_STATUS, PROP_SC_HUB_FUNCTION_ENABLE,
    PROP_SC_HUB_FUNCTION_ACCEPT_URIS, PROP_SC_HUB_FUNCTION_BINDING,
    PROP_SC_HUB_FUNCTION_CONNECTION_STATUS,
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM != 0
    PROP_SC_DIRECT_CONNECT_INITIATE_ENABLE,
    PROP_SC_DIRECT_CONNECT_ACCEPT_ENABLE, PROP_SC_DIRECT_CONNECT_ACCEPT_URIS,
    PROP_SC_DIRECT_CONNECT_BINDING, PROP_SC_DIRECT_CONNECT_CONNECTION_STATUS,
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */
    PROP_SC_FAILED_CONNECTION_REQUESTS, -1 };

static const int Network_Port_Properties_Proprietary[] = { -1 };

unsigned Network_Port_Object_Number(void)
{
    return BACNET_NETWORK_PORTS_MAX;
}

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

const char *Network_Port_Object_Name_char(uint32_t object_instance)
{
    unsigned index = 0; /* offset from instance lookup */

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        return Object_List[index].Object_Name;
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
bool Network_Port_Name_Set(uint32_t object_instance, char *new_name)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
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
bool Network_Port_Valid_Instance(uint32_t object_instance)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        status = true;
    }

    return status;
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
    uint32_t instance = BACNET_MAX_INSTANCE;

    if (index < BACNET_NETWORK_PORTS_MAX) {
        instance = Object_List[index].Instance_Number;
    }

    return instance;
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
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;
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
        status = octetstring_init(mac_address, mac, mac_len);
    }

    return status;
}

uint8_t *Network_Port_MAC_Address_pointer(uint32_t object_instance)
{
    unsigned index = 0; /* offset from instance lookup */
    uint8_t *mac = NULL;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        switch (Object_List[index].Network_Type) {
            case PORT_TYPE_ETHERNET:
                mac = &Object_List[index].Network.Ethernet.MAC_Address[0];
                break;
            case PORT_TYPE_MSTP:
                mac = &Object_List[index].Network.MSTP.MAC_Address;
                break;
            case PORT_TYPE_BIP:
                break;
            case PORT_TYPE_BIP6:
                mac = &Object_List[index].Network.IPv6.MAC_Address[0];
                break;
            case PORT_TYPE_BSC:
                mac = &Object_List[index].Network.BSC.MAC_Address[0];
                break;
            default:
                break;
        }
    }

    return mac;
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
    uint32_t object_instance, uint8_t *mac_src, uint8_t mac_len)
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
                /* no need to set - created from IP address and UPD Port */
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
            memcpy(mac_dest, mac_src, mac_size);
            status = true;
        }
    }

    return status;
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
    uint16_t value = 0;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        value = Object_List[index].APDU_Length;
    }

    return value;
}

/**
 * For a given object instance-number, sets the BACnet Network Number
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
        Object_List[index].Link_Speed = value;
        status = true;
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
            Network_Port_Pending_Params_Discard(object_instance);
        }
        status = true;
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
 * @param  b - ip-address first octet
 * @param  c - ip-address first octet
 * @param  d - ip-address first octet
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
    uint8_t ip_mask[4] = { 0 };

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            prefix = Object_List[index].Network.IPv4.IP_Subnet_Prefix;
            if ((prefix > 0) && (prefix <= 32)) {
                mask = (0xFFFFFFFF << (32 - prefix)) & 0xFFFFFFFF;
                encode_unsigned32(ip_mask, mask);
                status =
                    octetstring_init(subnet_mask, ip_mask, sizeof(ip_mask));
            }
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
 * @param  b - ip-address first octet
 * @param  c - ip-address first octet
 * @param  d - ip-address first octet
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
bool Network_Port_IP_DNS_Server(uint32_t object_instance,
    unsigned dns_index,
    BACNET_OCTET_STRING *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            if (dns_index < BIP_DNS_MAX) {
                status = octetstring_init(ip_address,
                    &Object_List[index]
                         .Network.IPv4.IP_DNS_Server[dns_index][0],
                    4);
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
 * @param  index - 0=primary, 1=secondary, 3=tertierary
 * @param  a - ip-address first octet
 * @param  b - ip-address first octet
 * @param  c - ip-address first octet
 * @param  d - ip-address first octet
 *
 * @return  true if ip-address was set
 */
bool Network_Port_IP_DNS_Server_Set(uint32_t object_instance,
    unsigned dns_index,
    uint8_t a,
    uint8_t b,
    uint8_t c,
    uint8_t d)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            if (dns_index < BIP_DNS_MAX) {
                Object_List[index].Network.IPv4.IP_DNS_Server[dns_index][0] = a;
                Object_List[index].Network.IPv4.IP_DNS_Server[dns_index][1] = b;
                Object_List[index].Network.IPv4.IP_DNS_Server[dns_index][2] = c;
                Object_List[index].Network.IPv4.IP_DNS_Server[dns_index][3] = d;
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
int Network_Port_IP_DNS_Server_Encode(
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

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        ipv4 = &Object_List[index].Network.IPv4;
        flag = ipv4->BBMD_Accept_FD_Registrations;
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

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        ipv4 = &Object_List[index].Network.IPv4;
        if (flag != ipv4->BBMD_Accept_FD_Registrations) {
            ipv4->BBMD_Accept_FD_Registrations = flag;
            Object_List[index].Changes_Pending = true;
        }
        status = true;
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

#if defined(BACDL_BIP) && BBMD_ENABLED
/**
 * For a given object instance-number, gets the ip-address and port
 * Note: depends on Network_Type being set for this object
 *
 * @param  object_instance - object-instance number of the object
 * @param  addr - holds the ip-address and port retrieved
 *
 * @return  true if ip-address and port were retrieved
 */
static bool Network_Port_Remote_BBMD_IP_Address_And_Port(
    uint32_t object_instance, BACNET_IP_ADDRESS *addr)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    if (addr) {
        index = Network_Port_Instance_To_Index(object_instance);
        if (index < BACNET_NETWORK_PORTS_MAX) {
            if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
                bvlc_address_set(addr,
                    Object_List[index].Network.IPv4.BBMD_IP_Address[0],
                    Object_List[index].Network.IPv4.BBMD_IP_Address[1],
                    Object_List[index].Network.IPv4.BBMD_IP_Address[2],
                    Object_List[index].Network.IPv4.BBMD_IP_Address[3]);
                addr->port = Object_List[index].Network.IPv4.BBMD_Port;
                status = true;
            }
        }
    }

    return status;
}
#endif

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

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            if (a) {
                *a = Object_List[index].Network.IPv4.BBMD_IP_Address[0];
            }
            if (b) {
                *b = Object_List[index].Network.IPv4.BBMD_IP_Address[1];
            }
            if (c) {
                *c = Object_List[index].Network.IPv4.BBMD_IP_Address[2];
            }
            if (d) {
                *d = Object_List[index].Network.IPv4.BBMD_IP_Address[3];
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

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP) {
            Object_List[index].Network.IPv4.BBMD_IP_Address[0] = a;
            Object_List[index].Network.IPv4.BBMD_IP_Address[1] = b;
            Object_List[index].Network.IPv4.BBMD_IP_Address[2] = c;
            Object_List[index].Network.IPv4.BBMD_IP_Address[3] = d;
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
            value = Object_List[index].Network.IPv4.BBMD_Port;
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
            if (Object_List[index].Network.IPv4.BBMD_Port != value) {
                Object_List[index].Changes_Pending = true;
            }
            Object_List[index].Network.IPv4.BBMD_Port = value;
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
            status = octetstring_init(ip_address,
                &Object_List[index].Network.IPv6.IP_Address[0], IPV6_ADDR_SIZE);
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
    uint32_t object_instance, uint8_t *ip_address)
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
 * For a given object instance-number, sets the BACnet/IP Subnet prefix value
 * Note: depends on Network_Type being set to PORT_TYPE_BIP for this object
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
            status = octetstring_init(ip_address,
                &Object_List[index].Network.IPv6.IP_Gateway[0], IPV6_ADDR_SIZE);
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
    uint32_t object_instance, uint8_t *ip_address)
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
bool Network_Port_IPv6_DNS_Server(uint32_t object_instance,
    unsigned dns_index,
    BACNET_OCTET_STRING *ip_address)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            if (dns_index < BIP_DNS_MAX) {
                status = octetstring_init(ip_address,
                    &Object_List[index]
                         .Network.IPv6.IP_DNS_Server[dns_index][0],
                    IPV6_ADDR_SIZE);
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
 * @param  index - 0=primary, 1=secondary, 3=tertierary
 * @param  ip_address - 16 byte IPv6 address
 *
 * @return  true if ip-address was set
 */
bool Network_Port_IPv6_DNS_Server_Set(
    uint32_t object_instance, unsigned dns_index, uint8_t *ip_address)
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
int Network_Port_IPv6_DNS_Server_Encode(
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
            status = octetstring_init(ip_address,
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
    uint32_t object_instance, uint8_t *ip_address)
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
            status = octetstring_init(ip_address,
                &Object_List[index].Network.IPv6.IP_DHCP_Server[0],
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
    uint32_t object_instance, uint8_t *ip_address)
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

const char *Network_Port_IPv6_Zone_Index_char(uint32_t object_instance)
{
    const char *p = NULL;
    unsigned index = 0;

    index = Network_Port_Instance_To_Index(object_instance);
    if (index < BACNET_NETWORK_PORTS_MAX) {
        if (Object_List[index].Network_Type == PORT_TYPE_BIP6) {
            p = &Object_List[index].Network.IPv6.Zone_Index[0];
        }
    }

    return p;
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
            snprintf(&Object_List[index].Network.IPv6.Zone_Index[0],
                ZONE_INDEX_SIZE, "%s", zone_index);
            status = true;
        }
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

/**
 * Determine if the object property is a BACnetARRAY datatype
 * @param  object_property [in] BACnet object property
 * @return true if the object property is a BACnetARRAY datatype
 */
bool Network_Port_BACnetArray_Property(BACNET_PROPERTY_ID object_property)
{
    bool status = false;

    switch (object_property) {
        case PROP_EVENT_TIME_STAMPS:
        case PROP_EVENT_MESSAGE_TEXTS:
        case PROP_EVENT_MESSAGE_TEXTS_CONFIG:
        case PROP_PROPERTY_LIST:
        case PROP_TAGS:
        case PROP_LINK_SPEEDS:
        case PROP_IP_DNS_SERVER:
        case PROP_IPV6_DNS_SERVER:
        case PROP_ISSUER_CERTIFICATE_FILES:
        case PROP_SC_HUB_FUNCTION_ACCEPT_URIS:
        case PROP_SC_DIRECT_CONNECT_ACCEPT_URIS:
            status = true;
            break;
        default:
            break;
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
    int apdu_len = 0;
    int apdu_size = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_OCTET_STRING octet_string;
    BACNET_CHARACTER_STRING char_string;
#if defined(BACDL_BIP) && BBMD_ENABLED
    BACNET_IP_ADDRESS ip_address;
#endif
    uint8_t *apdu = NULL;
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    Network_Port_Property_List(
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
        case PROP_MAC_ADDRESS:
            Network_Port_MAC_Address(rpdata->object_instance, &octet_string);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_LINK_SPEED:
            apdu_len = encode_application_real(
                &apdu[0], Network_Port_Link_Speed(rpdata->object_instance));
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
        case PROP_IP_DNS_SERVER:
            apdu_len = bacnet_array_encode(rpdata->object_instance,
                rpdata->array_index, Network_Port_IP_DNS_Server_Encode,
                BIP_DNS_MAX, apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
#if defined(BACDL_BIP) && BBMD_ENABLED
        case PROP_BBMD_ACCEPT_FD_REGISTRATIONS:
            apdu_len = encode_application_boolean(&apdu[0],
                Network_Port_BBMD_Accept_FD_Registrations(
                    rpdata->object_instance));
            break;
        case PROP_BBMD_BROADCAST_DISTRIBUTION_TABLE:
            apdu_len = bvlc_broadcast_distribution_table_encode(&apdu[0],
                rpdata->application_data_len,
                Network_Port_BBMD_BD_Table(rpdata->object_instance));
            break;
        case PROP_BBMD_FOREIGN_DEVICE_TABLE:
            apdu_len = bvlc_foreign_device_table_encode(&apdu[0],
                rpdata->application_data_len,
                Network_Port_BBMD_FD_Table(rpdata->object_instance));
            break;
        case PROP_FD_BBMD_ADDRESS:
            Network_Port_Remote_BBMD_IP_Address_And_Port(
                rpdata->object_instance, &ip_address);
            apdu_len = bvlc_foreign_device_bbmd_host_address_encode(
                &apdu[0], apdu_size, &ip_address);
            break;
        case PROP_FD_SUBSCRIPTION_LIFETIME:
            apdu_len = encode_application_unsigned(&apdu[0],
                Network_Port_Remote_BBMD_BIP_Lifetime(rpdata->object_instance));
            break;
#endif
        case PROP_BACNET_IPV6_MODE:
            apdu_len = encode_application_enumerated(
                &apdu[0], Network_Port_BIP6_Mode(rpdata->object_instance));
            break;
        case PROP_IPV6_ADDRESS:
            Network_Port_IPv6_Address(rpdata->object_instance, &octet_string);
            apdu_len = encode_application_octet_string(&apdu[0], &octet_string);
            break;
        case PROP_IPV6_PREFIX_LENGTH:
            apdu_len = encode_application_unsigned(&apdu[0],
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
            apdu_len = bacnet_array_encode(rpdata->object_instance,
                rpdata->array_index, Network_Port_IPv6_DNS_Server_Encode,
                BIP_DNS_MAX, apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_IPV6_AUTO_ADDRESSING_ENABLE:
            apdu_len = encode_application_boolean(&apdu[0], false);
            break;
        case PROP_IPV6_DHCP_LEASE_TIME:
            apdu_len = encode_application_unsigned(&apdu[0], 0);
            break;
        case PROP_IPV6_DHCP_LEASE_TIME_REMAINING:
            apdu_len = encode_application_unsigned(&apdu[0], 0);
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
            apdu_len = encode_application_unsigned(&apdu[0],
                Network_Port_Max_BVLC_Length_Accepted(rpdata->object_instance));
            break;
        case PROP_MAX_NPDU_LENGTH_ACCEPTED:
            apdu_len = encode_application_unsigned(&apdu[0],
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
            apdu_len = encode_application_unsigned(&apdu[0],
                Network_Port_SC_Minimum_Reconnect_Time(
                    rpdata->object_instance));
            break;
        case PROP_SC_MAXIMUM_RECONNECT_TIME:
            apdu_len = encode_application_unsigned(&apdu[0],
                Network_Port_SC_Maximum_Reconnect_Time(
                    rpdata->object_instance));
            break;
        case PROP_SC_CONNECT_WAIT_TIMEOUT:
            apdu_len = encode_application_unsigned(&apdu[0],
                Network_Port_SC_Connect_Wait_Timeout(rpdata->object_instance));
            break;
        case PROP_SC_DISCONNECT_WAIT_TIMEOUT:
            apdu_len = encode_application_unsigned(&apdu[0],
                Network_Port_SC_Disconnect_Wait_Timeout(
                    rpdata->object_instance));
            break;
        case PROP_SC_HEARTBEAT_TIMEOUT:
            apdu_len = encode_application_unsigned(&apdu[0],
                Network_Port_SC_Heartbeat_Timeout(rpdata->object_instance));
            break;
        case PROP_SC_HUB_CONNECTOR_STATE:
            apdu_len = encode_application_enumerated(&apdu[0],
                Network_Port_SC_Hub_Connector_State(rpdata->object_instance));
            break;
        case PROP_OPERATIONAL_CERTIFICATE_FILE:
            apdu_len = encode_application_unsigned(&apdu[0],
                Network_Port_Operational_Certificate_File(
                    rpdata->object_instance));
            break;
        case PROP_ISSUER_CERTIFICATE_FILES:
            apdu_len = bacnet_array_encode(rpdata->object_instance,
                rpdata->array_index,
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
            apdu_len = encode_application_unsigned(&apdu[0],
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
            apdu_len = bacapp_encode_SCHubConnection(&apdu[0],
                Network_Port_SC_Primary_Hub_Connection_Status(
                    rpdata->object_instance));
            break;
        case PROP_SC_FAILOVER_HUB_CONNECTION_STATUS:
            apdu_len = bacapp_encode_SCHubConnection(&apdu[0],
                Network_Port_SC_Failover_Hub_Connection_Status(
                    rpdata->object_instance));
            break;
        case PROP_SC_HUB_FUNCTION_ENABLE:
            apdu_len = encode_application_boolean(&apdu[0],
                Network_Port_SC_Hub_Function_Enable(rpdata->object_instance));
            break;
        case PROP_SC_HUB_FUNCTION_ACCEPT_URIS:
            apdu_len = bacnet_array_encode(rpdata->object_instance,
                rpdata->array_index,
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
            apdu_len = encode_application_boolean(&apdu[0],
                Network_Port_SC_Direct_Connect_Initiate_Enable(
                    rpdata->object_instance));
            break;
        case PROP_SC_DIRECT_CONNECT_ACCEPT_ENABLE:
            apdu_len = encode_application_boolean(&apdu[0],
                Network_Port_SC_Direct_Connect_Accept_Enable(
                    rpdata->object_instance));
            break;
        case PROP_SC_DIRECT_CONNECT_ACCEPT_URIS:
            apdu_len = bacnet_array_encode(rpdata->object_instance,
                rpdata->array_index,
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
    /*  only array properties can have optional array indices */
    if ((apdu_len >= 0) &&
        (!Network_Port_BACnetArray_Property(rpdata->object_property)) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

static bool Network_Port_MSTP_Max_Master_setter(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    return Network_Port_MSTP_Max_Master_Set(object_instance, (uint8_t)value);
}

static bool Network_Port_MSTP_Max_Info_Frames_setter(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value)
{
    return Network_Port_MSTP_Max_Info_Frames_Set(
        object_instance, (uint8_t)value);
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
    BACNET_APPLICATION_DATA_VALUE value;

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
    switch (wp_data->object_property) {
        case PROP_MAX_MASTER:
            write_property_unsigned_decode(wp_data, &value, 
                Network_Port_MSTP_Max_Master_setter, 255);
            break;
        case PROP_MAX_INFO_FRAMES:
            write_property_unsigned_decode(wp_data, &value, 
                Network_Port_MSTP_Max_Info_Frames_setter, 255);
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
        case PROP_MAC_ADDRESS:
        case PROP_LINK_SPEED:
        case PROP_CHANGES_PENDING:
        case PROP_APDU_LENGTH:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
#ifdef BACDL_BSC
        case PROP_MAX_BVLC_LENGTH_ACCEPTED:
            write_property_unsigned_decode(wp_data, &value,
                Network_Port_Max_BVLC_Length_Accepted_Dirty_Set,
                BACNET_UNSIGNED_INTEGER_MAX);
            break;
        case PROP_MAX_NPDU_LENGTH_ACCEPTED:
            write_property_unsigned_decode(wp_data, &value,
                Network_Port_Max_NPDU_Length_Accepted_Dirty_Set,
                BACNET_UNSIGNED_INTEGER_MAX);
            break;
        case PROP_SC_MINIMUM_RECONNECT_TIME:
            write_property_unsigned_decode(wp_data, &value,
                Network_Port_SC_Minimum_Reconnect_Time_Dirty_Set,
                BACNET_UNSIGNED_INTEGER_MAX);
            break;
        case PROP_SC_MAXIMUM_RECONNECT_TIME:
            write_property_unsigned_decode(wp_data, &value,
                Network_Port_SC_Maximum_Reconnect_Time_Dirty_Set,
                BACNET_UNSIGNED_INTEGER_MAX);
            break;
        case PROP_SC_CONNECT_WAIT_TIMEOUT:
            write_property_unsigned_decode(wp_data, &value,
                Network_Port_SC_Connect_Wait_Timeout_Dirty_Set,
                BACNET_UNSIGNED_INTEGER_MAX);
            break;
        case PROP_SC_DISCONNECT_WAIT_TIMEOUT:
            write_property_unsigned_decode(wp_data, &value,
                Network_Port_SC_Disconnect_Wait_Timeout_Dirty_Set,
                BACNET_UNSIGNED_INTEGER_MAX);
            break;
        case PROP_SC_HEARTBEAT_TIMEOUT:
            write_property_unsigned_decode(wp_data, &value,
                Network_Port_SC_Heartbeat_Timeout_Dirty_Set,
                BACNET_UNSIGNED_INTEGER_MAX);
            break;
            /* SC optionals */
#if BSC_CONF_HUB_FUNCTIONS_NUM != 0
        case PROP_SC_PRIMARY_HUB_URI:
            status = write_property_empty_string_valid(
                wp_data, &value, MAX_DEV_DESC_LEN);
            if (status) {
                status = Network_Port_SC_Primary_Hub_URI_Dirty_Set(
                    wp_data->object_instance,
                    characterstring_value(&value.type.Character_String));
            }
            break;
        case PROP_SC_FAILOVER_HUB_URI:
            status = write_property_empty_string_valid(
                wp_data, &value, MAX_DEV_DESC_LEN);
            if (status) {
                status = Network_Port_SC_Failover_Hub_URI_Dirty_Set(
                    wp_data->object_instance,
                    characterstring_value(&value.type.Character_String));
            }
            break;
        case PROP_SC_HUB_FUNCTION_ENABLE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                status = Network_Port_SC_Hub_Function_Enable_Dirty_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_SC_HUB_FUNCTION_ACCEPT_URIS:
            status = write_property_empty_string_valid(
                wp_data, &value, BACNET_URI_LENGTH);
            if (status) {
                status = Network_Port_SC_Hub_Function_Accept_URI_Dirty_Set(
                    wp_data->object_instance, wp_data->array_index - 1,
                    characterstring_value(&value.type.Character_String));
            }
            if (!status) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
            break;
        case PROP_SC_HUB_FUNCTION_BINDING:
            status = write_property_empty_string_valid(
                wp_data, &value, MAX_DEV_DESC_LEN);
            if (status) {
                status = Network_Port_SC_Hub_Function_Binding_Dirty_Set(
                    wp_data->object_instance,
                    characterstring_value(&value.type.Character_String));
            }
            break;
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM != 0
        case PROP_SC_DIRECT_CONNECT_INITIATE_ENABLE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                status =
                    Network_Port_SC_Direct_Connect_Initiate_Enable_Dirty_Set(
                        wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_SC_DIRECT_CONNECT_ACCEPT_ENABLE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                status = Network_Port_SC_Direct_Connect_Accept_Enable_Dirty_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_SC_DIRECT_CONNECT_ACCEPT_URIS:
            status = write_property_empty_string_valid(
                wp_data, &value, BACNET_URI_LENGTH);
            if (status) {
                status = Network_Port_SC_Direct_Connect_Accept_URI_Dirty_Set(
                    wp_data->object_instance, wp_data->array_index - 1,
                    characterstring_value(&value.type.Character_String));
            }
            if (!status) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
            break;
        case PROP_SC_DIRECT_CONNECT_BINDING:
            status = write_property_empty_string_valid(
                wp_data, &value, MAX_DEV_DESC_LEN);
            if (status) {
                status = Network_Port_SC_Direct_Connect_Binding_Dirty_Set(
                    wp_data->object_instance,
                    characterstring_value(&value.type.Character_String));
            }
            break;
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */
        case PROP_SC_HUB_CONNECTOR_STATE:
        case PROP_OPERATIONAL_CERTIFICATE_FILE:
        case PROP_ISSUER_CERTIFICATE_FILES:
        case PROP_CERTIFICATE_SIGNING_REQUEST_FILE:
        case PROP_ROUTING_TABLE:
        case PROP_SC_PRIMARY_HUB_CONNECTION_STATUS:
        case PROP_SC_FAILOVER_HUB_CONNECTION_STATUS:
        case PROP_SC_FAILED_CONNECTION_REQUESTS:
        case PROP_SC_HUB_FUNCTION_CONNECTION_STATUS:
        case PROP_SC_DIRECT_CONNECT_CONNECTION_STATUS:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
#endif /* BACDL_BSC */
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
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
 * @param  apdu - BACNET_READ_RANGE_DATA data
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

    switch (pRequest->object_property) {
        /* required properties */
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
        case PROP_CHANGES_PENDING:
        case PROP_APDU_LENGTH:
        case PROP_LINK_SPEED:
        /* optional properties */
        case PROP_MAC_ADDRESS:
#if defined(BACDL_MSTP)
        case PROP_MAX_MASTER:
        case PROP_MAX_INFO_FRAMES:
#endif
#if defined(BACDL_BIP)
        case PROP_BACNET_IP_MODE:
        case PROP_IP_ADDRESS:
        case PROP_BACNET_IP_UDP_PORT:
        case PROP_IP_SUBNET_MASK:
        case PROP_IP_DEFAULT_GATEWAY:
        case PROP_IP_DNS_SERVER:
#endif
#if defined(BACDL_BIP) && BBMD_ENABLED
        case PROP_BBMD_ACCEPT_FD_REGISTRATIONS:
#endif
            (void)pInfo;
            pRequest->error_class = ERROR_CLASS_SERVICES;
            pRequest->error_code = ERROR_CODE_PROPERTY_IS_NOT_A_LIST;
            break;
        case PROP_BBMD_BROADCAST_DISTRIBUTION_TABLE:
#if defined(BACDL_BIP) && BBMD_ENABLED
            pInfo->RequestTypes = RR_BY_POSITION;
            pInfo->Handler = Network_Port_Read_Range_BDT;
            status = true;
#else
            (void)pInfo;
            pRequest->error_class = ERROR_CLASS_PROPERTY;
            pRequest->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
#endif
            break;
        case PROP_BBMD_FOREIGN_DEVICE_TABLE:
#if defined(BACDL_BIP) && BBMD_ENABLED
            pInfo->RequestTypes = RR_BY_POSITION;
            pInfo->Handler = Network_Port_Read_Range_FDT;
            status = true;
#else
            (void)pInfo;
            pRequest->error_class = ERROR_CLASS_PROPERTY;
            pRequest->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
#endif
            break;
        default:
            (void)pInfo;
            pRequest->error_class = ERROR_CLASS_PROPERTY;
            pRequest->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
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

void Network_Port_Pending_Params_Apply(uint32_t object_instance)
{
#ifdef BACDL_BSC
    Network_Port_SC_Pending_Params_Apply(object_instance);
#else
    (void)object_instance;
#endif /* BACDL_BSC */
}

void Network_Port_Pending_Params_Discard(uint32_t object_instance)
{
#ifdef BACDL_BSC
    Network_Port_SC_Pending_Params_Discard(object_instance);
#else
    (void)object_instance;
#endif /* BACDL_BSC */
}
