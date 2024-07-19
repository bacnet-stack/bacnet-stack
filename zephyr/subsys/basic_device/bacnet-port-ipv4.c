/**
 * @file
 * @brief The BACnet/IPv4 datalink tasks for handling the device specific 
 *  data link layer
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
/* BACnet definitions */
#include "bacnet/bacdef.h"
/* BACnet library API */
#include "bacnet/basic/object/netport.h"
#include "bacnet/basic/bbmd/h_bbmd.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/datalink/bvlc.h"
#include "bacnet/datalink/datalink.h"
/* me! */
#include "basic_device/bacnet-port-ipv4.h"

/* timer used to renew Foreign Device Registration */
static uint16_t BBMD_Timer_Seconds;
static uint16_t BBMD_TTL_Seconds = 60000;
static BACNET_IP_ADDRESS BBMD_Address;

/**
 * @brief Initialize the datalink network port
 * @param ttl_seconds [in] The time-to-live in seconds for the Foreign Device Registration
 * @param bbmd_address [in] The address of the BBMD
  */
void bacnet_port_ipv4_foreign_device_init(
    const uint16_t ttl_seconds,
    const BACNET_IP_ADDRESS *bbmd_address)
{
    BBMD_TTL_Seconds = ttl_seconds;
    if (bbmd_address) {
        memcpy(&BBMD_Address, bbmd_address, sizeof(BACNET_IP_ADDRESS));
    }
}

/**
 * @brief Renew the Foreign Device Registration
 */
void bacnet_port_ipv4_task(uint16_t elapsed_seconds)
{
    if (BBMD_Timer_Seconds) {
        if (BBMD_Timer_Seconds <= elapsed_seconds) {
            BBMD_Timer_Seconds = 0;
        } else {
            BBMD_Timer_Seconds -= elapsed_seconds;
        }
        if (BBMD_Timer_Seconds == 0) {
            if (BBMD_Address.port > 0) {
                (void)bvlc_register_with_bbmd(&BBMD_Address,
                                                BBMD_TTL_Seconds);
            }
            BBMD_Timer_Seconds = (uint16_t)BBMD_TTL_Seconds;
        }
    }
}

/**
 * Initialize the network port object.
*/
bool bacnet_port_ipv4_init(void)
{
    const uint32_t instance = 1;
    BACNET_IP_ADDRESS addr = { 0 };
    uint8_t prefix = 0;

    if (!bip_init(NULL)) {
        return false;
    }
    Network_Port_Object_Instance_Number_Set(0, instance);
    Network_Port_Name_Set(instance, "BACnet/IP Port");
    Network_Port_Type_Set(instance, PORT_TYPE_BIP);
    bip_get_addr(&addr);
    prefix = bip_get_subnet_prefix();
    Network_Port_BIP_Port_Set(instance, addr.port);
    Network_Port_IP_Address_Set(instance, addr.address[0], addr.address[1],
                                addr.address[2], addr.address[3]);
    Network_Port_IP_Subnet_Prefix_Set(instance, prefix);
    Network_Port_Link_Speed_Set(instance, 0.0);
    /* common NP data */
    Network_Port_Reliability_Set(instance, RELIABILITY_NO_FAULT_DETECTED);
    Network_Port_Out_Of_Service_Set(instance, false);
    Network_Port_Quality_Set(instance, PORT_QUALITY_UNKNOWN);
    Network_Port_APDU_Length_Set(instance, MAX_APDU);
    Network_Port_Network_Number_Set(instance, 0);
    /* last thing - clear pending changes - we don't want to set these
       since they are already set */
    Network_Port_Changes_Pending_Set(instance, false);

    return true;
}
