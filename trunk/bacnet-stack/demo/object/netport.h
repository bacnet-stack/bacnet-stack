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

#ifndef NETPORT_H
#define NETPORT_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacenum.h"
#include "apdu.h"
#include "rp.h"
#include "wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void Network_Port_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    bool Network_Port_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    bool Network_Port_Name_Set(
        uint32_t object_instance,
        char *new_name);

    char *Network_Port_Description(
        uint32_t instance);
    bool Network_Port_Description_Set(
        uint32_t instance,
        char *new_name);

    BACNET_RELIABILITY Network_Port_Reliability(
        uint32_t object_instance);
    bool Network_Port_Reliability_Set(
        uint32_t object_instance,
        BACNET_RELIABILITY value);

    bool Network_Port_Out_Of_Service(
        uint32_t instance);
    bool Network_Port_Out_Of_Service_Set(
        uint32_t instance,
        bool oos_flag);

    uint8_t Network_Port_Type(
        uint32_t object_instance);
    bool Network_Port_Type_Set(
        uint32_t object_instance,
        uint8_t value);

    uint16_t Network_Port_Network_Number(
        uint32_t object_instance);
    bool Network_Port_Network_Number_Set(
        uint32_t object_instance,
        uint16_t value);

    BACNET_PORT_QUALITY Network_Port_Quality(
        uint32_t object_instance);
    bool Network_Port_Quality_Set(
        uint32_t object_instance,
        BACNET_PORT_QUALITY value);

    bool Network_Port_MAC_Address(
        uint32_t object_instance,
        BACNET_OCTET_STRING *mac_address);
    bool Network_Port_MAC_Address_Set(
        uint32_t object_instance,
        uint8_t *mac_src,
        uint8_t mac_len);

    uint16_t Network_Port_APDU_Length(
        uint32_t object_instance);
    bool Network_Port_APDU_Length_Set(
        uint32_t object_instance,
        uint16_t value);

    uint8_t Network_Port_MSTP_Max_Master(
        uint32_t object_instance);
    bool Network_Port_MSTP_Max_Master_Set(
        uint32_t object_instance,
        uint8_t value);

    uint8_t Network_Port_MSTP_Max_Info_Frames(
        uint32_t object_instance);
    bool Network_Port_MSTP_Max_Info_Frames_Set(
        uint32_t object_instance,
        uint8_t value);

    uint32_t Network_Port_Link_Speed(
        uint32_t object_instance);
    bool Network_Port_Link_Speed_Set(
        uint32_t object_instance,
        uint32_t value);

    bool Network_Port_IP_Address(
        uint32_t object_instance,
        BACNET_OCTET_STRING *ip_address);
    bool Network_Port_IP_Address_Get(
        uint32_t object_instance,
        uint8_t *a, uint8_t *b, uint8_t *c, uint8_t *d);
    bool Network_Port_IP_Address_Set(
        uint32_t object_instance,
        uint8_t a, uint8_t b, uint8_t c, uint8_t d);

    uint8_t Network_Port_IP_Subnet_Prefix(
        uint32_t object_instance);
    bool Network_Port_IP_Subnet_Prefix_Set(
        uint32_t object_instance,
        uint8_t value);

    bool Network_Port_IP_Subnet(
        uint32_t object_instance,
        BACNET_OCTET_STRING *subnet_mask);
    bool Network_Port_IP_Subnet_Get(
        uint32_t object_instance,
        uint8_t *a, uint8_t *b, uint8_t *c, uint8_t *d);
    bool Network_Port_IP_Subnet_Set(
        uint32_t object_instance,
        uint8_t a, uint8_t b, uint8_t c, uint8_t d);

    bool Network_Port_IP_Gateway(
        uint32_t object_instance,
        BACNET_OCTET_STRING *subnet_mask);
    bool Network_Port_IP_Gateway_Get(
        uint32_t object_instance,
        uint8_t *a, uint8_t *b, uint8_t *c, uint8_t *d);
    bool Network_Port_IP_Gateway_Set(
        uint32_t object_instance,
        uint8_t a, uint8_t b, uint8_t c, uint8_t d);

    bool Network_Port_IP_DNS_Server(
        uint32_t object_instance,
        unsigned index,
        BACNET_OCTET_STRING *subnet_mask);
    bool Network_Port_IP_DNS_Server_Get(
        uint32_t object_instance,
        unsigned index,
        uint8_t *a, uint8_t *b, uint8_t *c, uint8_t *d);
    bool Network_Port_IP_DNS_Server_Set(
        uint32_t object_instance,
        unsigned index,
        uint8_t a, uint8_t b, uint8_t c, uint8_t d);

    uint16_t Network_Port_BIP_Port(
        uint32_t object_instance);
    bool Network_Port_BIP_Port_Set(
        uint32_t object_instance,
        uint16_t value);

    BACNET_IP_MODE Network_Port_BIP_Mode(
        uint32_t object_instance);
    bool Network_Port_BIP_Mode_Set(
        uint32_t object_instance,
        BACNET_IP_MODE value);

    bool Network_Port_BBMD_Accept_FD_Registrations(
        uint32_t object_instance);
    bool Network_Port_BBMD_Accept_FD_Registrations_Set(
        uint32_t object_instance,
        bool value);

    bool Network_Port_Changes_Pending(
        uint32_t instance);
    bool Network_Port_Changes_Pending_Set(
        uint32_t instance,
        bool flag);

    bool Network_Port_Valid_Instance(
        uint32_t object_instance);
    unsigned Network_Port_Count(
        void);
    uint32_t Network_Port_Index_To_Instance(
        unsigned find_index);
    unsigned Network_Port_Instance_To_Index(
        uint32_t object_instance);

    bool Network_Port_Set_Network_Port_Instance_ID(
        unsigned index,
        uint32_t object_instance);

    int Network_Port_Read_Range_BDT(
        uint8_t * apdu,
        BACNET_READ_RANGE_DATA * pRequest);
    int Network_Port_Read_Range_FDT(
        uint8_t * apdu,
        BACNET_READ_RANGE_DATA * pRequest);
    bool Network_Port_Read_Range(
        BACNET_READ_RANGE_DATA * pRequest,
        RR_PROP_INFO * pInfo);

    bool Network_Port_Create(
        uint32_t object_instance);
    bool Network_Port_Delete(
        uint32_t object_instance);
    void Network_Port_Cleanup(
        void);
    void Network_Port_Init(
        void);

    /* handling for read property service */
    int Network_Port_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);

    /* handling for write property service */
    bool Network_Port_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
