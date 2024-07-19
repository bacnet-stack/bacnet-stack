/**
 * @file
 * @brief Implementation of the BACnet Virtual Link Layer using IPv6
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2015
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 * @defgroup DLBIP BACnet/IP DataLink Network Layer
 * @ingroup DataLink
 */
#ifndef BACNET_BVLC6_H
#define BACNET_BVLC6_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"

/**
* BVLL for BACnet/IPv6
* @{
*/
#define BVLL_TYPE_BACNET_IP6 (0x82)
/** @} */

/**
* B/IPv6 BVLL Messages
* @{
*/
#define BVLC6_RESULT 0x00
#define BVLC6_ORIGINAL_UNICAST_NPDU 0x01
#define BVLC6_ORIGINAL_BROADCAST_NPDU 0x02
#define BVLC6_ADDRESS_RESOLUTION 0x03
#define BVLC6_FORWARDED_ADDRESS_RESOLUTION 0x04
#define BVLC6_ADDRESS_RESOLUTION_ACK 0x05
#define BVLC6_VIRTUAL_ADDRESS_RESOLUTION 0x06
#define BVLC6_VIRTUAL_ADDRESS_RESOLUTION_ACK 0x07
#define BVLC6_FORWARDED_NPDU 0x08
#define BVLC6_REGISTER_FOREIGN_DEVICE 0x09
#define BVLC6_DELETE_FOREIGN_DEVICE 0x0A
#define BVLC6_SECURE_BVLL 0x0B
#define BVLC6_DISTRIBUTE_BROADCAST_TO_NETWORK 0x0C
/** @} */

/**
* BVLC Result Code
* @{
*/
#define BVLC6_RESULT_SUCCESSFUL_COMPLETION 0x0000U
#define BVLC6_RESULT_ADDRESS_RESOLUTION_NAK 0x0030U
#define BVLC6_RESULT_VIRTUAL_ADDRESS_RESOLUTION_NAK 0x0060U
#define BVLC6_RESULT_REGISTER_FOREIGN_DEVICE_NAK 0x0090U
#define BVLC6_RESULT_DELETE_FOREIGN_DEVICE_NAK 0x00A0U
#define BVLC6_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK 0x00C0U
#define BVLC6_RESULT_INVALID 0xFFFFU
/** @} */

/**
* BACnet IPv6 Multicast Group ID
* BACnet broadcast messages shall be delivered by IPv6 multicasts
* as opposed to using IP broadcasting. Broadcasting in
* IPv6 is subsumed by multicasting to the all-nodes link
* group FF02::1; however, the use of the all-nodes group is not
* recommended, and BACnet/IPv6 uses an IANA permanently assigned
* multicast group identifier to avoid disturbing
* every interface in the network.
*
* The IANA assigned BACnet/IPv6 variable scope multicast address
* is FF0X:0:0:0:0:0:0:BAC0 (FF0X::BAC0) which indicates the multicast
* group identifier X'BAC0'. The following multicast scopes are
* defined for B/IPv6.
* @{
*/
#define BIP6_MULTICAST_GROUP_ID    0xBAC0U
/** @} */

/**
* IANA prefixes
* @{
*/
#define BIP6_MULTICAST_reserved_0  0xFF00U
#define BIP6_MULTICAST_NODE_LOCAL  0xFF01U
#define BIP6_MULTICAST_LINK_LOCAL  0xFF02U
#define BIP6_MULTICAST_reserved_3  0xFF03U
#define BIP6_MULTICAST_ADMIN_LOCAL 0xFF04U
#define BIP6_MULTICAST_SITE_LOCAL  0xFF05U
#define BIP6_MULTICAST_ORG_LOCAL   0xFF08U
#define BIP6_MULTICAST_GLOBAL      0xFF0EU
/** @} */

/* number of bytes in the IPv6 address */
#define IP6_ADDRESS_MAX 16
/* number of bytes in the B/IPv6 address */
#define BIP6_ADDRESS_MAX 18

/**
* BACnet IPv6 Address
*
* Data link layer addressing between B/IPv6 nodes consists of a 128-bit
* IPv6 address followed by a two-octet UDP port number (both of which
* shall be transmitted with the most significant octet first).
* This address shall be referred to as a B/IPv6 address.
* @{
*/
typedef struct BACnet_IP6_Address {
    uint8_t address[IP6_ADDRESS_MAX];
    uint16_t port;
} BACNET_IP6_ADDRESS;
/** @} */

/**
* BACnet /IPv6 Broadcast Distribution Table Format
*
* The BDT shall consist of either the eighteen-octet B/IPv6 address
* of the peer BBMD or the combination of the fully qualified
* domain name service (DNS) entry and UDP port that resolves to
* the B/IPv6 address of the peer BBMD. The Broadcast
* Distribution Table shall not contain an entry for the BBMD in
* which the BDT resides.
* @{
*/
struct BACnet_IP6_Broadcast_Distribution_Table_Entry;
typedef struct BACnet_IP6_Broadcast_Distribution_Table_Entry {
    /* true if valid entry - false if not */
    bool valid;
    /* BACnet/IPv6 address */
    BACNET_IP6_ADDRESS bip6_address;
    struct BACnet_IP6_Broadcast_Distribution_Table_Entry *next;
} BACNET_IP6_BROADCAST_DISTRIBUTION_TABLE_ENTRY;
/** @} */

/**
* Foreign Device Table (FDT)
*
* Each entry shall contain the B/IPv6 address and the TTL of the
* registered foreign device.
*
* Each entry shall consist of the eighteen-octet B/IPv6 address of the
* registrant; the 2-octet Time-to-Live value supplied at the time of
* registration; and a 2-octet value representing the number of seconds
* remaining before the BBMD will purge the registrant's FDT entry if no
* re-registration occurs. The number of seconds remaining shall be
* initialized to the 2-octet Time-to-Live value supplied at the time
* of registration plus 30 seconds (see U.4.5.2), with a maximum of 65535.
* @{
*/
struct BACnet_IP6_Foreign_Device_Table_Entry;
typedef struct BACnet_IP6_Foreign_Device_Table_Entry {
    /* true if valid entry - false if not */
    bool valid;
    /* BACnet/IPv6 address */
    BACNET_IP6_ADDRESS bip6_address;
    /* requested time-to-live value */
    uint16_t ttl_seconds;
    /*  number of seconds remaining */
    uint16_t ttl_seconds_remaining;
    struct BACnet_IP6_Foreign_Device_Table_Entry *next;
} BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY;
/** @} */

#ifdef __cplusplus
extern "C" {

#endif /* __cplusplus */
    BACNET_STACK_EXPORT
    int bvlc6_encode_address(
        uint8_t * pdu,
        uint16_t pdu_size,
        BACNET_IP6_ADDRESS * ip6_address);
    BACNET_STACK_EXPORT
    int bvlc6_decode_address(
        uint8_t * pdu,
        uint16_t pdu_len,
        BACNET_IP6_ADDRESS * ip6_address);
    BACNET_STACK_EXPORT
    bool bvlc6_address_copy(
        BACNET_IP6_ADDRESS * dst,
        BACNET_IP6_ADDRESS * src);
    BACNET_STACK_EXPORT
    bool bvlc6_address_different(
        BACNET_IP6_ADDRESS * dst,
        BACNET_IP6_ADDRESS * src);

    BACNET_STACK_EXPORT
    int bvlc6_address_to_ascii(BACNET_IP6_ADDRESS *addr, char *buf,
        size_t buf_size);
    BACNET_STACK_EXPORT
    bool bvlc6_address_from_ascii(
        BACNET_IP6_ADDRESS *addr,
        const char *addrstr);

    BACNET_STACK_EXPORT
    bool bvlc6_address_set(
        BACNET_IP6_ADDRESS * addr,
        uint16_t addr0,
        uint16_t addr1,
        uint16_t addr2,
        uint16_t addr3,
        uint16_t addr4,
        uint16_t addr5,
        uint16_t addr6,
        uint16_t addr7);
    BACNET_STACK_EXPORT
    bool bvlc6_address_get(
        BACNET_IP6_ADDRESS * addr,
        uint16_t *addr0,
        uint16_t *addr1,
        uint16_t *addr2,
        uint16_t *addr3,
        uint16_t *addr4,
        uint16_t *addr5,
        uint16_t *addr6,
        uint16_t *addr7);

    BACNET_STACK_EXPORT
    bool bvlc6_vmac_address_set(
        BACNET_ADDRESS * addr,
        uint32_t device_id);
    BACNET_STACK_EXPORT
    bool bvlc6_vmac_address_get(
        BACNET_ADDRESS * addr,
        uint32_t *device_id);

    BACNET_STACK_EXPORT
   int bvlc6_encode_header(
       uint8_t * pdu,
       uint16_t pdu_size,
       uint8_t message_type,
       uint16_t length);
    BACNET_STACK_EXPORT
   int bvlc6_decode_header(
       uint8_t * pdu,
       uint16_t pdu_len,
       uint8_t * message_type,
       uint16_t * length);

   BACNET_STACK_EXPORT
   int bvlc6_encode_result(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac,
        uint16_t result_code);
    BACNET_STACK_EXPORT
    int bvlc6_decode_result(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac,
        uint16_t * result_code);

    BACNET_STACK_EXPORT
   int bvlc6_encode_original_unicast(
       uint8_t * pdu,
       uint16_t pdu_size,
       uint32_t vmac_src,
       uint32_t vmac_dst,
       uint8_t * npdu,
       uint16_t npdu_len);
    BACNET_STACK_EXPORT
    int bvlc6_decode_original_unicast(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        uint32_t * vmac_dst,
        uint8_t * npdu,
        uint16_t npdu_size,
        uint16_t * npdu_len);

    BACNET_STACK_EXPORT
    int bvlc6_encode_original_broadcast(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac,
        uint8_t * npdu,
        uint16_t npdu_len);
    BACNET_STACK_EXPORT
    int bvlc6_decode_original_broadcast(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac,
        uint8_t * npdu,
        uint16_t npdu_size,
        uint16_t * npdu_len);

    BACNET_STACK_EXPORT
    int bvlc6_encode_address_resolution(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        uint32_t vmac_target);
    BACNET_STACK_EXPORT
    int bvlc6_decode_address_resolution(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        uint32_t * vmac_target);

    BACNET_STACK_EXPORT
    int bvlc6_encode_forwarded_address_resolution(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        uint32_t vmac_target,
        BACNET_IP6_ADDRESS * bip6_address);
    BACNET_STACK_EXPORT
    int bvlc6_decode_forwarded_address_resolution(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        uint32_t * vmac_target,
        BACNET_IP6_ADDRESS * bip6_address);

    BACNET_STACK_EXPORT
    int bvlc6_encode_address_resolution_ack(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        uint32_t vmac_dst);
    BACNET_STACK_EXPORT
    int bvlc6_decode_address_resolution_ack(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        uint32_t * vmac_dst);

    BACNET_STACK_EXPORT
    int bvlc6_encode_virtual_address_resolution(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src);
    BACNET_STACK_EXPORT
    int bvlc6_decode_virtual_address_resolution(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src);

    BACNET_STACK_EXPORT
    int bvlc6_encode_virtual_address_resolution_ack(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        uint32_t vmac_dst);
    BACNET_STACK_EXPORT
    int bvlc6_decode_virtual_address_resolution_ack(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        uint32_t * vmac_dst);

    BACNET_STACK_EXPORT
    int bvlc6_encode_forwarded_npdu(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        BACNET_IP6_ADDRESS * address,
        uint8_t * npdu,
        uint16_t npdu_len);
    BACNET_STACK_EXPORT
    int bvlc6_decode_forwarded_npdu(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        BACNET_IP6_ADDRESS * address,
        uint8_t * npdu,
        uint16_t npdu_size,
        uint16_t * npdu_len);

    BACNET_STACK_EXPORT
    int bvlc6_encode_register_foreign_device(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        uint16_t ttl_seconds);
    BACNET_STACK_EXPORT
    int bvlc6_decode_register_foreign_device(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        uint16_t * ttl_seconds);

    BACNET_STACK_EXPORT
    int bvlc6_encode_delete_foreign_device(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        BACNET_IP6_ADDRESS *bip6_address);

    BACNET_STACK_EXPORT
    int bvlc6_decode_delete_foreign_device(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        BACNET_IP6_ADDRESS *bip6_address);

    BACNET_STACK_EXPORT
    int bvlc6_encode_secure_bvll(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint8_t * sbuf,
        uint16_t sbuf_len);
    BACNET_STACK_EXPORT
    int bvlc6_decode_secure_bvll(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint8_t * sbuf,
        uint16_t sbuf_size,
        uint16_t * sbuf_len);

    BACNET_STACK_EXPORT
    int bvlc6_encode_distribute_broadcast_to_network(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac,
        uint8_t * npdu,
        uint16_t npdu_len);
    BACNET_STACK_EXPORT
    int bvlc6_decode_distribute_broadcast_to_network(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac,
        uint8_t * npdu,
        uint16_t npdu_size,
        uint16_t * npdu_len);
    BACNET_STACK_EXPORT
    int bvlc6_foreign_device_bbmd_host_address_encode(uint8_t *apdu,
        uint16_t apdu_size,
        BACNET_IP6_ADDRESS *ip6_address);

    BACNET_STACK_EXPORT
    int bvlc6_broadcast_distribution_table_entry_encode(uint8_t *apdu,
        BACNET_IP6_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry);
    BACNET_STACK_EXPORT
    int bvlc6_broadcast_distribution_table_list_encode(uint8_t *apdu,
        BACNET_IP6_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_head);
    BACNET_STACK_EXPORT
    int bvlc6_broadcast_distribution_table_encode(uint8_t *apdu,
        uint16_t apdu_size,
        BACNET_IP6_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_head);

    BACNET_STACK_EXPORT
    int bvlc6_foreign_device_table_entry_encode(uint8_t *apdu,
        BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry);
    BACNET_STACK_EXPORT
    int bvlc6_foreign_device_table_list_encode(uint8_t *apdu,
        BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY *fdt_head);
    BACNET_STACK_EXPORT
    int bvlc6_foreign_device_table_encode(uint8_t *apdu,
        uint16_t apdu_size,
        BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY *fdt_head);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* */
