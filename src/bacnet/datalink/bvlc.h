/**
 * @file
 * @brief BACnet/IP virtual link control module encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 * @defgroup DLBIP BACnet/IP DataLink Network Layer
 * @ingroup DataLink
 */
#ifndef BACNET_BVLC_H
#define BACNET_BVLC_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"

/**
 * BVLL for BACnet/IPv4
 * @{
 */
#define BVLL_TYPE_BACNET_IP (0x81)
/** @} */

/**
 * B/IPv4 BVLL Messages
 * @{
 */
#define BVLC_RESULT 0
#define BVLC_WRITE_BROADCAST_DISTRIBUTION_TABLE 1
#define BVLC_READ_BROADCAST_DIST_TABLE 2
#define BVLC_READ_BROADCAST_DIST_TABLE_ACK 3
#define BVLC_FORWARDED_NPDU 4
#define BVLC_REGISTER_FOREIGN_DEVICE 5
#define BVLC_READ_FOREIGN_DEVICE_TABLE 6
#define BVLC_READ_FOREIGN_DEVICE_TABLE_ACK 7
#define BVLC_DELETE_FOREIGN_DEVICE_TABLE_ENTRY 8
#define BVLC_DISTRIBUTE_BROADCAST_TO_NETWORK 9
#define BVLC_ORIGINAL_UNICAST_NPDU 10
#define BVLC_ORIGINAL_BROADCAST_NPDU 11
#define BVLC_SECURE_BVLL 12
#define BVLC_INVALID 255

/** @} */

/**
 * BVLC Result Code
 * @{
 */
#define BVLC_RESULT_SUCCESSFUL_COMPLETION 0x0000
#define BVLC_RESULT_WRITE_BROADCAST_DISTRIBUTION_TABLE_NAK 0x0010
#define BVLC_RESULT_READ_BROADCAST_DISTRIBUTION_TABLE_NAK 0x0020
#define BVLC_RESULT_REGISTER_FOREIGN_DEVICE_NAK 0X0030
#define BVLC_RESULT_READ_FOREIGN_DEVICE_TABLE_NAK 0x0040
#define BVLC_RESULT_DELETE_FOREIGN_DEVICE_TABLE_ENTRY_NAK 0x0050
#define BVLC_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK 0x0060
#define BVLC_RESULT_INVALID 0xFFFF

/* number of bytes in the IPv4 address */
#define IP_ADDRESS_MAX 4

/**
 * BACnet IPv4 Address
 *
 * Data link layer addressing between B/IPv4 nodes consists of a 32-bit
 * IPv4 address followed by a two-octet UDP port number (both of which
 * shall be transmitted with the most significant octet first).
 * This address shall be referred to as a B/IPv4 address.
 * @{
 */
typedef struct BACnet_IP_Address {
    uint8_t address[IP_ADDRESS_MAX];
    uint16_t port;
} BACNET_IP_ADDRESS;
/* number of bytes in the B/IPv4 address */
#define BIP_ADDRESS_MAX 6

/**
 * BACnet IPv4 Broadcast Distribution Mask
 *
 * The Broadcast Distribution Mask is a 4-octet field that
 * indicates how broadcast messages are to be distributed on
 * the IP subnet served by the BBMD.
 * @{
 */
typedef struct BACnet_IP_Broadcast_Distribution_Mask {
    uint8_t address[IP_ADDRESS_MAX];
} BACNET_IP_BROADCAST_DISTRIBUTION_MASK;
/* number of bytes in the B/IPv4 Broadcast Distribution Mask */
#define BACNET_IP_BDT_MASK_SIZE IP_ADDRESS_MAX
/** @} */

/**
 * BACnet/IP Broadcast Distribution Table (BDT)
 *
 * The BDT consists of one entry for the address of the BBMD
 * for the local IP subnet and an entry for the BBMD on each
 * remote IP subnet to which broadcasts are to be forwarded.
 * Each entry consists of the 6-octet B/IP address with which
 * the BBMD is accessed and a 4-octet broadcast distribution mask.
 * @{
 */
struct BACnet_IP_Broadcast_Distribution_Table_Entry;
typedef struct BACnet_IP_Broadcast_Distribution_Table_Entry {
    /* true if valid entry - false if not */
    bool valid;
    /* BACnet/IP address */
    BACNET_IP_ADDRESS dest_address;
    /* Broadcast Distribution Mask */
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK broadcast_mask;
    struct BACnet_IP_Broadcast_Distribution_Table_Entry *next;
} BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY;
/* number of bytes in a B/IPv4 roadcast Distribution Table entry */
#define BACNET_IP_BDT_ENTRY_SIZE (BIP_ADDRESS_MAX + BACNET_IP_BDT_MASK_SIZE)
/** @} */

/**
 * Foreign Device Table (FDT)
 *
 * Each device that registers as a foreign device shall be placed
 * in an entry in the BBMD's Foreign Device Table (FDT). Each
 * entry shall consist of the 6-octet B/IP address of the registrant;
 * the 2-octet Time-to-Live value supplied at the time of
 * registration; and a 2-octet value representing the number of
 * seconds remaining before the BBMD will purge the registrant's FDT
 * entry if no re-registration occurs. This value will be initialized
 * to the 2-octet Time-to-Live value supplied at the time of
 * registration.
 * @{
 */
struct BACnet_IP_Foreign_Device_Table_Entry;
typedef struct BACnet_IP_Foreign_Device_Table_Entry {
    bool valid;
    /* BACnet/IP address */
    BACNET_IP_ADDRESS dest_address;
    /* requested time-to-live value */
    uint16_t ttl_seconds;
    /* our counter - includes 30 second grace period */
    uint16_t ttl_seconds_remaining;
    struct BACnet_IP_Foreign_Device_Table_Entry *next;
} BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY;
#define BACNET_IP_FDT_ENTRY_SIZE 10
/** @} */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int bvlc_encode_address(
    uint8_t *pdu, uint16_t pdu_size, const BACNET_IP_ADDRESS *ip_address);

BACNET_STACK_EXPORT
int bvlc_decode_address(
    const uint8_t *pdu, uint16_t pdu_len, BACNET_IP_ADDRESS *ip_address);

BACNET_STACK_EXPORT
bool bvlc_address_copy(BACNET_IP_ADDRESS *dst, const BACNET_IP_ADDRESS *src);

BACNET_STACK_EXPORT
bool bvlc_address_different(
    const BACNET_IP_ADDRESS *dst, const BACNET_IP_ADDRESS *src);

BACNET_STACK_EXPORT
bool bvlc_address_from_ascii(BACNET_IP_ADDRESS *dst, const char *addrstr);

BACNET_STACK_EXPORT
bool bvlc_address_port_from_ascii(
    BACNET_IP_ADDRESS *dst, const char *addrstr, const char *portstr);

BACNET_STACK_EXPORT
void bvlc_address_from_network(BACNET_IP_ADDRESS *dst, uint32_t addr);

BACNET_STACK_EXPORT
bool bvlc_address_set(
    BACNET_IP_ADDRESS *addr,
    uint8_t addr0,
    uint8_t addr1,
    uint8_t addr2,
    uint8_t addr3);

BACNET_STACK_EXPORT
bool bvlc_address_get(
    const BACNET_IP_ADDRESS *addr,
    uint8_t *addr0,
    uint8_t *addr1,
    uint8_t *addr2,
    uint8_t *addr3);

BACNET_STACK_EXPORT
bool bvlc_ip_address_to_bacnet_local(
    BACNET_ADDRESS *addr, const BACNET_IP_ADDRESS *ipaddr);

BACNET_STACK_EXPORT
bool bvlc_ip_address_from_bacnet_local(
    BACNET_IP_ADDRESS *ipaddr, const BACNET_ADDRESS *addr);

BACNET_STACK_EXPORT
bool bvlc_ip_address_to_bacnet_remote(
    BACNET_ADDRESS *addr, uint16_t dnet, const BACNET_IP_ADDRESS *ipaddr);

BACNET_STACK_EXPORT
bool bvlc_ip_address_from_bacnet_remote(
    BACNET_IP_ADDRESS *ipaddr, uint16_t *dnet, const BACNET_ADDRESS *addr);

BACNET_STACK_EXPORT
int bvlc_encode_broadcast_distribution_mask(
    uint8_t *pdu,
    uint16_t pdu_size,
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *bd_mask);

BACNET_STACK_EXPORT
int bvlc_decode_broadcast_distribution_mask(
    const uint8_t *pdu,
    uint16_t pdu_len,
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK *bd_mask);

BACNET_STACK_EXPORT
int bvlc_encode_broadcast_distribution_table_entry(
    uint8_t *pdu,
    uint16_t pdu_size,
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry);

BACNET_STACK_EXPORT
int bvlc_decode_broadcast_distribution_table_entry(
    const uint8_t *pdu,
    uint16_t pdu_len,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry);

BACNET_STACK_EXPORT
void bvlc_broadcast_distribution_table_link_array(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list,
    const size_t bdt_array_size);

BACNET_STACK_EXPORT
uint16_t bvlc_broadcast_distribution_table_count(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list);

BACNET_STACK_EXPORT
uint16_t bvlc_broadcast_distribution_table_valid_count(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list);

BACNET_STACK_EXPORT
void bvlc_broadcast_distribution_table_valid_clear(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list);

BACNET_STACK_EXPORT
bool bvlc_broadcast_distribution_table_entry_different(
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *dst,
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *src);

BACNET_STACK_EXPORT
bool bvlc_broadcast_distribution_table_entry_copy(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *dst,
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *src);

BACNET_STACK_EXPORT
bool bvlc_broadcast_distribution_mask_different(
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *dst,
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *src);

BACNET_STACK_EXPORT
bool bvlc_broadcast_distribution_mask_copy(
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK *dst,
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *src);

BACNET_STACK_EXPORT
bool bvlc_broadcast_distribution_table_entry_append(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list,
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry);

BACNET_STACK_EXPORT
bool bvlc_broadcast_distribution_table_entry_set(
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry,
    const BACNET_IP_ADDRESS *addr,
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask);

BACNET_STACK_EXPORT
bool bvlc_broadcast_distribution_table_entry_forward_address(
    BACNET_IP_ADDRESS *addr,
    const BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_entry);

BACNET_STACK_EXPORT
bool bvlc_address_mask(
    BACNET_IP_ADDRESS *dst,
    const BACNET_IP_ADDRESS *src,
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask);

BACNET_STACK_EXPORT
bool bvlc_broadcast_distribution_mask_from_host(
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask, uint32_t broadcast_mask);

BACNET_STACK_EXPORT
bool bvlc_broadcast_distribution_mask_to_host(
    uint32_t *broadcast_mask,
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask);

BACNET_STACK_EXPORT
void bvlc_broadcast_distribution_mask_set(
    BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask,
    uint8_t addr0,
    uint8_t addr1,
    uint8_t addr2,
    uint8_t addr3);

BACNET_STACK_EXPORT
void bvlc_broadcast_distribution_mask_get(
    const BACNET_IP_BROADCAST_DISTRIBUTION_MASK *mask,
    uint8_t *addr0,
    uint8_t *addr1,
    uint8_t *addr2,
    uint8_t *addr3);

BACNET_STACK_EXPORT
int bvlc_broadcast_distribution_table_decode(
    const uint8_t *apdu,
    uint16_t apdu_len,
    BACNET_ERROR_CODE *error_code,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_head);

BACNET_STACK_EXPORT
int bvlc_broadcast_distribution_table_encode(
    uint8_t *apdu,
    uint16_t apdu_size,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_head);

BACNET_STACK_EXPORT
int bvlc_encode_write_broadcast_distribution_table(
    uint8_t *pdu,
    uint16_t pdu_size,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list);

BACNET_STACK_EXPORT
int bvlc_decode_write_broadcast_distribution_table(
    const uint8_t *pdu,
    uint16_t pdu_len,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list);

BACNET_STACK_EXPORT
int bvlc_encode_read_broadcast_distribution_table(
    uint8_t *pdu, uint16_t pdu_size);

BACNET_STACK_EXPORT
int bvlc_encode_read_broadcast_distribution_table_ack(
    uint8_t *pdu,
    uint16_t pdu_size,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list);

BACNET_STACK_EXPORT
int bvlc_decode_read_broadcast_distribution_table_ack(
    const uint8_t *pdu,
    uint16_t pdu_len,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list);

BACNET_STACK_EXPORT
int bvlc_encode_header(
    uint8_t *pdu, uint16_t pdu_size, uint8_t message_type, uint16_t length);

BACNET_STACK_EXPORT
int bvlc_decode_header(
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint8_t *message_type,
    uint16_t *length);

BACNET_STACK_EXPORT
void bvlc_foreign_device_table_maintenance_timer(
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list, uint16_t seconds);

BACNET_STACK_EXPORT
uint16_t bvlc_foreign_device_table_valid_count(
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list);

BACNET_STACK_EXPORT
uint16_t
bvlc_foreign_device_table_count(BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list);

BACNET_STACK_EXPORT
void bvlc_foreign_device_table_link_array(
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list, const size_t array_size);

BACNET_STACK_EXPORT
bool bvlc_foreign_device_table_entry_different(
    const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *dst,
    const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *src);

BACNET_STACK_EXPORT
bool bvlc_foreign_device_table_entry_copy(
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *dst,
    const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *src);

BACNET_STACK_EXPORT
bool bvlc_foreign_device_table_entry_delete(
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list,
    const BACNET_IP_ADDRESS *ip_address);

BACNET_STACK_EXPORT
bool bvlc_foreign_device_table_entry_add(
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list,
    const BACNET_IP_ADDRESS *ip_address,
    uint16_t ttl_seconds);

BACNET_STACK_EXPORT
int bvlc_encode_foreign_device_table_entry(
    uint8_t *pdu,
    uint16_t pdu_size,
    const BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry);

BACNET_STACK_EXPORT
int bvlc_decode_foreign_device_table_entry(
    const uint8_t *pdu,
    uint16_t pdu_len,
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_entry);

BACNET_STACK_EXPORT
int bvlc_foreign_device_table_encode(
    uint8_t *apdu,
    uint16_t apdu_size,
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_head);

BACNET_STACK_EXPORT
int bvlc_encode_read_foreign_device_table(uint8_t *pdu, uint16_t pdu_size);

BACNET_STACK_EXPORT
int bvlc_encode_read_foreign_device_table_ack(
    uint8_t *pdu,
    uint16_t pdu_size,
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list);

BACNET_STACK_EXPORT
int bvlc_decode_read_foreign_device_table_ack(
    const uint8_t *pdu,
    uint16_t pdu_len,
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *fdt_list);

BACNET_STACK_EXPORT
int bvlc_encode_result(uint8_t *pdu, uint16_t pdu_size, uint16_t result_code);

BACNET_STACK_EXPORT
int bvlc_decode_result(
    const uint8_t *pdu, uint16_t pdu_len, uint16_t *result_code);

BACNET_STACK_EXPORT
int bvlc_encode_original_unicast(
    uint8_t *pdu, uint16_t pdu_size, const uint8_t *npdu, uint16_t npdu_len);

BACNET_STACK_EXPORT
int bvlc_decode_original_unicast(
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint8_t *npdu,
    uint16_t npdu_size,
    uint16_t *npdu_len);

BACNET_STACK_EXPORT
int bvlc_encode_original_broadcast(
    uint8_t *pdu, uint16_t pdu_size, const uint8_t *npdu, uint16_t npdu_len);

BACNET_STACK_EXPORT
int bvlc_decode_original_broadcast(
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint8_t *npdu,
    uint16_t npdu_size,
    uint16_t *npdu_len);

BACNET_STACK_EXPORT
int bvlc_encode_forwarded_npdu(
    uint8_t *pdu,
    uint16_t pdu_size,
    const BACNET_IP_ADDRESS *address,
    const uint8_t *npdu,
    uint16_t npdu_len);

BACNET_STACK_EXPORT
int bvlc_decode_forwarded_npdu(
    const uint8_t *pdu,
    uint16_t pdu_len,
    BACNET_IP_ADDRESS *address,
    uint8_t *npdu,
    uint16_t npdu_size,
    uint16_t *npdu_len);

BACNET_STACK_EXPORT
int bvlc_encode_register_foreign_device(
    uint8_t *pdu, uint16_t pdu_size, uint16_t ttl_seconds);

BACNET_STACK_EXPORT
int bvlc_decode_register_foreign_device(
    const uint8_t *pdu, uint16_t pdu_len, uint16_t *ttl_seconds);

BACNET_STACK_EXPORT
int bvlc_encode_delete_foreign_device(
    uint8_t *pdu, uint16_t pdu_size, const BACNET_IP_ADDRESS *ip_address);

BACNET_STACK_EXPORT
int bvlc_decode_delete_foreign_device(
    const uint8_t *pdu, uint16_t pdu_len, BACNET_IP_ADDRESS *ip_address);

BACNET_STACK_EXPORT
int bvlc_encode_secure_bvll(
    uint8_t *pdu, uint16_t pdu_size, const uint8_t *sbuf, uint16_t sbuf_len);

BACNET_STACK_EXPORT
int bvlc_decode_secure_bvll(
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint8_t *sbuf,
    uint16_t sbuf_size,
    uint16_t *sbuf_len);

BACNET_STACK_EXPORT
int bvlc_encode_distribute_broadcast_to_network(
    uint8_t *pdu, uint16_t pdu_size, const uint8_t *npdu, uint16_t npdu_len);

BACNET_STACK_EXPORT
int bvlc_decode_distribute_broadcast_to_network(
    const uint8_t *pdu,
    uint16_t pdu_len,
    uint8_t *npdu,
    uint16_t npdu_size,
    uint16_t *npdu_len);

BACNET_STACK_EXPORT
const char *bvlc_result_code_name(uint16_t result_code);

BACNET_STACK_EXPORT
int bvlc_foreign_device_bbmd_host_address_encode(
    uint8_t *apdu, uint16_t apdu_size, const BACNET_IP_ADDRESS *ip_address);

BACNET_STACK_EXPORT
int bvlc_foreign_device_bbmd_host_address_decode(
    const uint8_t *apdu,
    uint16_t apdu_len,
    BACNET_ERROR_CODE *error_code,
    BACNET_IP_ADDRESS *ip_address);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* */
