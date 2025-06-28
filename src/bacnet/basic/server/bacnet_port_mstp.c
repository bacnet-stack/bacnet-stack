/**
 * @file
 * @brief The BACnet datalink tasks for handling the device specific
 *  data link layer
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date January 2025
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
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlmstp.h"
/* BACnet Basic network port */
#include "bacnet/basic/server/bacnet_port_mstp.h"

#if defined(BACDL_MSTP)
static struct dlmstp_statistics Statistics = { 0 };
static uint16_t Timer_Seconds;

/**
 * @brief Application Task
 */
void bacnet_port_mstp_task(uint16_t elapsed_seconds)
{
    Timer_Seconds += elapsed_seconds;
    if (Timer_Seconds >= 60) {
        Timer_Seconds = 0;
        dlmstp_fill_statistics(&Statistics);
    }
}

/**
 * Initialize the network port object.
 * @return true if successful
 */
bool bacnet_port_mstp_init(void)
{
    uint32_t instance = 1;
    BACNET_ADDRESS addr = { 0 };
    long baud_rate = 38400;
    long mac_address = 127;
    uint8_t max_master = 127;
    long max_info_frames = 1;

    dlmstp_set_max_info_frames(max_info_frames);
    dlmstp_set_max_master(max_master);
    dlmstp_set_baud_rate(baud_rate);
    dlmstp_set_mac_address(mac_address);
    Network_Port_Object_Instance_Number_Set(0, instance);
    Network_Port_Name_Set(instance, "BACnet MS/TP Port");
    Network_Port_Type_Set(instance, PORT_TYPE_MSTP);
    dlmstp_get_my_address(&addr);
    Network_Port_MAC_Address_Set(instance, &addr.mac[0], addr.mac_len);

    Network_Port_Reliability_Set(instance, RELIABILITY_NO_FAULT_DETECTED);
    Network_Port_Link_Speed_Set(instance, baud_rate);
    Network_Port_Out_Of_Service_Set(instance, false);
    Network_Port_Quality_Set(instance, PORT_QUALITY_UNKNOWN);
    Network_Port_APDU_Length_Set(instance, MAX_APDU);
    Network_Port_Network_Number_Set(instance, 0);
    /* last thing - clear pending changes - we don't want to set these
       since they are already set */
    Network_Port_Changes_Pending_Set(instance, false);

    return true;
}
#endif
