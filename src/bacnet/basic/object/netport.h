/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2016
 * @brief API for basic BACnet Network port object
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_NETPORT_H
#define BACNET_BASIC_OBJECT_NETPORT_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/readrange.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Network_Port_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);
BACNET_STACK_EXPORT
void Network_Port_Property_List(
    uint32_t object_instance,
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary);

BACNET_STACK_EXPORT
bool Network_Port_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Network_Port_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Network_Port_Object_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
char *Network_Port_Description(uint32_t instance);
BACNET_STACK_EXPORT
bool Network_Port_Description_Set(uint32_t instance, const char *new_name);

BACNET_STACK_EXPORT
BACNET_RELIABILITY Network_Port_Reliability(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Reliability_Set(
    uint32_t object_instance, BACNET_RELIABILITY value);

BACNET_STACK_EXPORT
bool Network_Port_Out_Of_Service(uint32_t instance);
BACNET_STACK_EXPORT
bool Network_Port_Out_Of_Service_Set(uint32_t instance, bool oos_flag);

BACNET_STACK_EXPORT
uint8_t Network_Port_Type(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Type_Set(uint32_t object_instance, uint8_t value);

BACNET_STACK_EXPORT
uint16_t Network_Port_Network_Number(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Network_Number_Set(uint32_t object_instance, uint16_t value);

BACNET_STACK_EXPORT
BACNET_PORT_QUALITY Network_Port_Quality(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Quality_Set(
    uint32_t object_instance, BACNET_PORT_QUALITY value);

BACNET_STACK_EXPORT
bool Network_Port_MAC_Address(
    uint32_t object_instance, BACNET_OCTET_STRING *mac_address);
BACNET_STACK_EXPORT
uint8_t Network_Port_MAC_Address_Value(
    uint32_t object_instance, uint8_t *mac_address, size_t mac_size);
BACNET_STACK_EXPORT
bool Network_Port_MAC_Address_Set(
    uint32_t object_instance, const uint8_t *mac_src, uint8_t mac_len);

BACNET_STACK_EXPORT
uint16_t Network_Port_APDU_Length(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_APDU_Length_Set(uint32_t object_instance, uint16_t value);

BACNET_STACK_EXPORT
uint8_t Network_Port_MSTP_MAC_Address(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_MSTP_MAC_Address_Set(uint32_t object_instance, uint8_t value);

BACNET_STACK_EXPORT
uint8_t Network_Port_MSTP_Max_Master(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_MSTP_Max_Master_Set(uint32_t object_instance, uint8_t value);

BACNET_STACK_EXPORT
uint8_t Network_Port_MSTP_Max_Info_Frames(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_MSTP_Max_Info_Frames_Set(
    uint32_t object_instance, uint8_t value);

BACNET_STACK_EXPORT
float Network_Port_Link_Speed(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Link_Speed_Set(uint32_t object_instance, float value);

BACNET_STACK_EXPORT
bool Network_Port_IP_Address(
    uint32_t object_instance, BACNET_OCTET_STRING *ip_address);
BACNET_STACK_EXPORT
bool Network_Port_IP_Address_Get(
    uint32_t object_instance, uint8_t *a, uint8_t *b, uint8_t *c, uint8_t *d);
BACNET_STACK_EXPORT
bool Network_Port_IP_Address_Set(
    uint32_t object_instance, uint8_t a, uint8_t b, uint8_t c, uint8_t d);

BACNET_STACK_EXPORT
uint8_t Network_Port_IP_Subnet_Prefix(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_IP_Subnet_Prefix_Set(uint32_t object_instance, uint8_t value);

BACNET_STACK_EXPORT
bool Network_Port_IP_Subnet(
    uint32_t object_instance, BACNET_OCTET_STRING *subnet_mask);
BACNET_STACK_EXPORT
bool Network_Port_IP_Subnet_Get(
    uint32_t object_instance, uint8_t *a, uint8_t *b, uint8_t *c, uint8_t *d);
BACNET_STACK_EXPORT
bool Network_Port_IP_Subnet_Set(
    uint32_t object_instance, uint8_t a, uint8_t b, uint8_t c, uint8_t d);

BACNET_STACK_EXPORT
bool Network_Port_IP_Gateway(
    uint32_t object_instance, BACNET_OCTET_STRING *subnet_mask);
BACNET_STACK_EXPORT
bool Network_Port_IP_Gateway_Get(
    uint32_t object_instance, uint8_t *a, uint8_t *b, uint8_t *c, uint8_t *d);
BACNET_STACK_EXPORT
bool Network_Port_IP_Gateway_Set(
    uint32_t object_instance, uint8_t a, uint8_t b, uint8_t c, uint8_t d);

BACNET_STACK_EXPORT
bool Network_Port_IP_DHCP_Enable(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_IP_DHCP_Enable_Set(uint32_t object_instance, bool value);

BACNET_STACK_EXPORT
bool Network_Port_IP_DNS_Server(
    uint32_t object_instance, unsigned index, BACNET_OCTET_STRING *subnet_mask);
BACNET_STACK_EXPORT
bool Network_Port_IP_DNS_Server_Get(
    uint32_t object_instance,
    unsigned index,
    uint8_t *a,
    uint8_t *b,
    uint8_t *c,
    uint8_t *d);
BACNET_STACK_EXPORT
bool Network_Port_IP_DNS_Server_Set(
    uint32_t object_instance,
    unsigned index,
    uint8_t a,
    uint8_t b,
    uint8_t c,
    uint8_t d);

BACNET_STACK_EXPORT
uint16_t Network_Port_BIP_Port(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_BIP_Port_Set(uint32_t object_instance, uint16_t value);
BACNET_STACK_EXPORT
uint32_t Network_Port_IPv6_DHCP_Lease_Time(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_IPv6_DHCP_Lease_Time_Set(
    uint32_t object_instance, const uint32_t value);

BACNET_STACK_EXPORT
BACNET_IP_MODE Network_Port_BIP_Mode(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_BIP_Mode_Set(uint32_t object_instance, BACNET_IP_MODE value);

BACNET_STACK_EXPORT
bool Network_Port_BBMD_Accept_FD_Registrations(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_BBMD_Accept_FD_Registrations_Set(
    uint32_t object_instance, bool value);

BACNET_STACK_EXPORT
void *Network_Port_BBMD_BD_Table(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_BBMD_BD_Table_Set(uint32_t object_instance, void *bdt_head);
BACNET_STACK_EXPORT
void *Network_Port_BBMD_FD_Table(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_BBMD_FD_Table_Set(uint32_t object_instance, void *fdt_head);

BACNET_STACK_EXPORT
bool Network_Port_Remote_BBMD_IP_Address(
    uint32_t object_instance, uint8_t *a, uint8_t *b, uint8_t *c, uint8_t *d);
BACNET_STACK_EXPORT
bool Network_Port_Remote_BBMD_IP_Address_Set(
    uint32_t object_instance, uint8_t a, uint8_t b, uint8_t c, uint8_t d);
BACNET_STACK_EXPORT
uint16_t Network_Port_Remote_BBMD_BIP_Port(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Remote_BBMD_BIP_Port_Set(
    uint32_t object_instance, uint16_t value);
BACNET_STACK_EXPORT
uint16_t Network_Port_Remote_BBMD_BIP_Lifetime(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Remote_BBMD_BIP_Lifetime_Set(
    uint32_t object_instance, uint16_t value);

#if defined(BACDL_BIP6)
BACNET_STACK_EXPORT
bool Network_Port_BBMD_IP6_Accept_FD_Registrations(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_BBMD_IP6_Accept_FD_Registrations_Set(
    uint32_t object_instance, bool value);

BACNET_STACK_EXPORT
void *Network_Port_BBMD_IP6_BD_Table(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_BBMD_IP6_BD_Table_Set(
    uint32_t object_instance, void *bdt_head);
BACNET_STACK_EXPORT
void *Network_Port_BBMD_IP6_FD_Table(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_BBMD_IP6_FD_Table_Set(
    uint32_t object_instance, void *fdt_head);

BACNET_STACK_EXPORT
bool Network_Port_Remote_BBMD_IP6_Address(
    uint32_t object_instance, uint8_t *addr);
BACNET_STACK_EXPORT
bool Network_Port_Remote_BBMD_IP6_Address_Set(
    uint32_t object_instance, const uint8_t *addr);
BACNET_STACK_EXPORT
uint16_t Network_Port_Remote_BBMD_BIP6_Port(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Remote_BBMD_BIP6_Port_Set(
    uint32_t object_instance, uint16_t value);
BACNET_STACK_EXPORT
uint16_t Network_Port_Remote_BBMD_BIP6_Lifetime(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Remote_BBMD_BIP6_Lifetime_Set(
    uint32_t object_instance, uint16_t value);
#endif

BACNET_STACK_EXPORT
BACNET_IP_MODE Network_Port_BIP6_Mode(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_BIP6_Mode_Set(uint32_t object_instance, BACNET_IP_MODE value);

BACNET_STACK_EXPORT
bool Network_Port_IPv6_Address(
    uint32_t object_instance, BACNET_OCTET_STRING *ip_address);
BACNET_STACK_EXPORT
bool Network_Port_IPv6_Address_Set(
    uint32_t object_instance, const uint8_t *ip_address);

BACNET_STACK_EXPORT
bool Network_Port_IPv6_Multicast_Address(
    uint32_t object_instance, BACNET_OCTET_STRING *ip_address);
BACNET_STACK_EXPORT
bool Network_Port_IPv6_Multicast_Address_Set(
    uint32_t object_instance, const uint8_t *ip_address);

BACNET_STACK_EXPORT
uint8_t Network_Port_IPv6_Subnet_Prefix(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_IPv6_Subnet_Prefix_Set(
    uint32_t object_instance, uint8_t value);

BACNET_STACK_EXPORT
bool Network_Port_IPv6_Gateway(
    uint32_t object_instance, BACNET_OCTET_STRING *ip_address);
BACNET_STACK_EXPORT
bool Network_Port_IPv6_Gateway_Set(
    uint32_t object_instance, const uint8_t *ip_address);

BACNET_STACK_EXPORT
bool Network_Port_IPv6_DNS_Server(
    uint32_t object_instance,
    unsigned dns_index,
    BACNET_OCTET_STRING *ip_address);
BACNET_STACK_EXPORT
bool Network_Port_IPv6_DNS_Server_Set(
    uint32_t object_instance, unsigned dns_index, const uint8_t *ip_address);

BACNET_STACK_EXPORT
bool Network_Port_IPv6_DHCP_Server(
    uint32_t object_instance, BACNET_OCTET_STRING *ip_address);
BACNET_STACK_EXPORT
bool Network_Port_IPv6_DHCP_Server_Set(
    uint32_t object_instance, const uint8_t *ip_address);

BACNET_STACK_EXPORT
bool Network_Port_IPv6_Zone_Index(
    uint32_t object_instance, BACNET_CHARACTER_STRING *zone_index);
BACNET_STACK_EXPORT
bool Network_Port_IPv6_Gateway_Zone_Index_Set(
    uint32_t object_instance, char *zone_index);

BACNET_STACK_EXPORT
bool Network_Port_IPv6_Auto_Addressing_Enable(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_IPv6_Auto_Addressing_Enable_Set(
    uint32_t object_instance, bool value);

BACNET_STACK_EXPORT
uint16_t Network_Port_BIP6_Port(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_BIP6_Port_Set(uint32_t object_instance, uint16_t value);

BACNET_STACK_EXPORT
bool Network_Port_Changes_Pending(uint32_t instance);
BACNET_STACK_EXPORT
bool Network_Port_Changes_Pending_Set(uint32_t instance, bool flag);
BACNET_STACK_EXPORT
void Network_Port_Changes_Pending_Activate(uint32_t instance);
BACNET_STACK_EXPORT
void Network_Port_Changes_Pending_Discard(uint32_t instance);

BACNET_STACK_EXPORT
bool Network_Port_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Network_Port_Count(void);
BACNET_STACK_EXPORT
uint32_t Network_Port_Index_To_Instance(unsigned find_index);
BACNET_STACK_EXPORT
unsigned Network_Port_Instance_To_Index(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Network_Port_Object_Instance_Number_Set(
    unsigned index, uint32_t object_instance);

BACNET_STACK_EXPORT
int Network_Port_Read_Range_BDT(
    uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest);
BACNET_STACK_EXPORT
int Network_Port_Read_Range_FDT(
    uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest);
BACNET_STACK_EXPORT
bool Network_Port_Read_Range(
    BACNET_READ_RANGE_DATA *pRequest, RR_PROP_INFO *pInfo);

BACNET_STACK_EXPORT
uint32_t Network_Port_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Network_Port_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Network_Port_Changes_Activate(void);
BACNET_STACK_EXPORT
void Network_Port_Changes_Discard(void);
BACNET_STACK_EXPORT
void Network_Port_Cleanup(void);
BACNET_STACK_EXPORT
void Network_Port_Init(void);

/* handling for read property service */
BACNET_STACK_EXPORT
int Network_Port_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

/* handling for write property service */
BACNET_STACK_EXPORT
bool Network_Port_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
