/**
* @file
* @author Steve Karg
* @date 2015
*
* Implementation of the BACnet Virtual Link Layer using IPv6,
* as described in Annex J.
*/
#ifndef BVLC6_H
#define BVLC6_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include "bacdef.h"
#include "npdu.h"

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
#define BVLC6_RESULT_SUCCESSFUL_COMPLETION 0x0000
#define BVLC6_RESULT_ADDRESS_RESOLUTION_NAK 0x0030
#define BVLC6_RESULT_VIRTUAL_ADDRESS_RESOLUTION_NAK 0x0060
#define BVLC6_RESULT_REGISTER_FOREIGN_DEVICE_NAK 0x0090
#define BVLC6_RESULT_DELETE_FOREIGN_DEVICE_NAK 0x00A0
#define BVLC6_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK 0x00C0
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
#define BIP6_MULTICAST_GROUP_ID    0xBAC0
/** @} */

/**
* IANA prefixes
* @{
*/
#define BIP6_MULTICAST_reserved_0  0xFF00
#define BIP6_MULTICAST_NODE_LOCAL  0xFF01
#define BIP6_MULTICAST_LINK_LOCAL  0xFF02
#define BIP6_MULTICAST_reserved_3  0xFF03
#define BIP6_MULTICAST_ADMIN_LOCAL 0xFF04
#define BIP6_MULTICAST_SITE_LOCAL  0xFF05
#define BIP6_MULTICAST_ORG_LOCAL   0xFF08
#define BIP6_MULTICAST_GLOBAL      0xFF0E
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
    int bvlc6_encode_address(
        uint8_t * pdu,
        uint16_t pdu_size,
        BACNET_IP6_ADDRESS * ip6_address);
    int bvlc6_decode_address(
        uint8_t * pdu,
        uint16_t pdu_len,
        BACNET_IP6_ADDRESS * ip6_address);
    bool bvlc6_address_copy(
        BACNET_IP6_ADDRESS * dst,
        BACNET_IP6_ADDRESS * src);
    bool bvlc6_address_different(
        BACNET_IP6_ADDRESS * dst,
        BACNET_IP6_ADDRESS * src);

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

    bool bvlc6_vmac_address_set(
        BACNET_ADDRESS * addr,
        uint32_t device_id);
    bool bvlc6_vmac_address_get(
        BACNET_ADDRESS * addr,
        uint32_t *device_id);

    int bvlc6_encode_header(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint8_t message_type,
        uint16_t length);
    int bvlc6_decode_header(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint8_t * message_type,
        uint16_t * length);

    int bvlc6_encode_result(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac,
        uint16_t result_code);
    int bvlc6_decode_result(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac,
        uint16_t * result_code);

    int bvlc6_encode_original_unicast(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        uint32_t vmac_dst,
        uint8_t * npdu,
        uint16_t npdu_len);
    int bvlc6_decode_original_unicast(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        uint32_t * vmac_dst,
        uint8_t * npdu,
        uint16_t npdu_size,
        uint16_t * npdu_len);

    int bvlc6_encode_original_broadcast(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac,
        uint8_t * npdu,
        uint16_t npdu_len);
    int bvlc6_decode_original_broadcast(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac,
        uint8_t * npdu,
        uint16_t npdu_size,
        uint16_t * npdu_len);

    int bvlc6_encode_address_resolution(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        uint32_t vmac_target);
    int bvlc6_decode_address_resolution(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        uint32_t * vmac_target);

    int bvlc6_encode_forwarded_address_resolution(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        uint32_t vmac_target,
        BACNET_IP6_ADDRESS * bip6_address);
    int bvlc6_decode_forwarded_address_resolution(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        uint32_t * vmac_target,
        BACNET_IP6_ADDRESS * bip6_address);

    int bvlc6_encode_address_resolution_ack(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        uint32_t vmac_dst);
    int bvlc6_decode_address_resolution_ack(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        uint32_t * vmac_dst);

    int bvlc6_encode_virtual_address_resolution(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src);
    int bvlc6_decode_virtual_address_resolution(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src);

    int bvlc6_encode_virtual_address_resolution_ack(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        uint32_t vmac_dst);
    int bvlc6_decode_virtual_address_resolution_ack(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        uint32_t * vmac_dst);

    int bvlc6_encode_forwarded_npdu(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        BACNET_IP6_ADDRESS * address,
        uint8_t * npdu,
        uint16_t npdu_len);
    int bvlc6_decode_forwarded_npdu(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        BACNET_IP6_ADDRESS * address,
        uint8_t * npdu,
        uint16_t npdu_size,
        uint16_t * npdu_len);

    int bvlc6_encode_register_foreign_device(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        uint16_t ttl_seconds);
    int bvlc6_decode_register_foreign_device(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        uint16_t * ttl_seconds);

    int bvlc6_encode_delete_foreign_device(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac_src,
        BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY * fdt_entry);
    int bvlc6_decode_delete_foreign_device(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac_src,
        BACNET_IP6_FOREIGN_DEVICE_TABLE_ENTRY * fdt_entry);

    int bvlc6_encode_secure_bvll(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint8_t * sbuf,
        uint16_t sbuf_len);
    int bvlc6_decode_secure_bvll(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint8_t * sbuf,
        uint16_t sbuf_size,
        uint16_t * sbuf_len);

    int bvlc6_encode_distribute_broadcast_to_network(
        uint8_t * pdu,
        uint16_t pdu_size,
        uint32_t vmac,
        uint8_t * npdu,
        uint16_t npdu_len);
    int bvlc6_decode_distribute_broadcast_to_network(
        uint8_t * pdu,
        uint16_t pdu_len,
        uint32_t * vmac,
        uint8_t * npdu,
        uint16_t npdu_size,
        uint16_t * npdu_len);

    /* user application function prototypes */
    void bvlc6_maintenance_timer(time_t seconds);
    int bvlc6_handler(
        BACNET_IP6_ADDRESS *addr,
        BACNET_ADDRESS * src,
        uint8_t * npdu,
        uint16_t npdu_len);
    int bvlc6_register_with_bbmd(
        BACNET_IP6_ADDRESS *bbmd_addr,
        uint32_t vmac_src,
        uint16_t time_to_live_seconds);
    uint16_t bvlc6_get_last_result(
        void);
    uint8_t bvlc6_get_function_code(
        void);
    void bvlc6_init(void);

#ifdef TEST
#include "ctest.h"
    void test_BVLC6(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* */
