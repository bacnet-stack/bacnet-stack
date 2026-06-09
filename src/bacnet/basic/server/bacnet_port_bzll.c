/**
 * @file
 * @brief The BACnet datalink tasks for handling the device specific
 *  data link layer
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date March 2026
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
/* BACnet definitions */
#include "bacnet/bacdef.h"
/* BACnet library API */
#include "bacnet/basic/object/netport.h"
#include "bacnet/basic/bzll/bzllvmac.h"
#include "bacnet/datalink/bzll.h"
/* me! */
#include "bacnet/basic/server/bacnet_port_bzll.h"

#if defined(BACDL_ZIGBEE)

/**
 * @brief Periodic tasks for the BACnet datalink layer
 */
void bacnet_port_bzll_task(uint16_t elapsed_seconds)
{
    (void)elapsed_seconds;
    /* nothing to do */
}

/**
 * Initialize the network port object.
 *
 * From Table 12-71.11.
 * Additional Properties of the Network Port Object Type
 * if Network_Type is ZIGBEE
 *
 * Property Identifier          Property Datatype
 * ----------------------       -----------------
 * Network_Number               Unsigned16
 * Network_Number_Quality       BACnetNetworkNumberQuality
 * APDU_Length                  Unsigned
 * Virtual_MAC_Address_Table    BACnetLIST of BACnetVMACEntry
 * Routing_Table                BACnetLIST of BACnetRouterEntry
 * MAC_Address                  OctetString
 * Link_Speed                   Real
 * Link_Speeds                  BACnetARRAY[N] of Real
 * Link_Speed_Autonegotiate     Boolean
 * Network_Interface_Name       CharacterString
 * Device_Address_Proxy_Enable  Boolean
 * Device_Address_Proxy_Timeout Unsigned
 *
 * @return true if successful
 */
bool bacnet_port_bzll_init(void)
{
    uint32_t instance = 1;
    BACNET_ADDRESS addr = { 0 };

    if (!bzll_init(NULL)) {
        return false;
    }
    Network_Port_Object_Instance_Number_Set(0, instance);
    Network_Port_Name_Set(instance, "BACnet/Zigbee Port");
    Network_Port_Type_Set(instance, PORT_TYPE_ZIGBEE);
    bzll_get_my_address(&addr);
    Network_Port_MAC_Address_Set(instance, &addr.mac[0], addr.mac_len);

    Network_Port_Reliability_Set(instance, RELIABILITY_NO_FAULT_DETECTED);
    Network_Port_Out_Of_Service_Set(instance, false);
    Network_Port_Network_Number_Set(instance, 0);
    Network_Port_Quality_Set(instance, PORT_QUALITY_UNKNOWN);
    Network_Port_APDU_Length_Set(instance, MAX_APDU);
    Network_Port_Link_Speed_Set(instance, 0.0);
    /* last thing - clear pending changes - we don't want to set these
       since they are already set */
    Network_Port_Changes_Pending_Set(instance, false);

    return true;
}
#endif
