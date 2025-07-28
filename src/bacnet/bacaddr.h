/**
 * @file
 * @brief BACnet address encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_ADDRESS_H
#define BACNET_ADDRESS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/* define the max MAC as big as IPv6 + port number */
#ifndef BACNET_VMAC_MAC_MAX
#define BACNET_VMAC_MAC_MAX 18
#endif

/** BACnetVMACEntry ::= SEQUENCE {
 *   virtual-mac-address[0]OctetString, -- maximum size 6 octets
 *   native-mac-address[1]OctetString
 */
typedef struct BACnetVMACEntry {
    BACNET_MAC_ADDRESS virtual_mac_address;
    uint8_t native_mac_address_len;
    uint8_t native_mac_address[BACNET_VMAC_MAC_MAX];
    struct BACnetVMACEntry *next;
} BACNET_VMAC_ENTRY;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void bacnet_address_copy(BACNET_ADDRESS *dest, const BACNET_ADDRESS *src);
BACNET_STACK_EXPORT
bool bacnet_address_same(const BACNET_ADDRESS *dest, const BACNET_ADDRESS *src);
BACNET_STACK_EXPORT
bool bacnet_address_init(
    BACNET_ADDRESS *dest,
    const BACNET_MAC_ADDRESS *mac,
    uint16_t dnet,
    const BACNET_MAC_ADDRESS *adr);

BACNET_STACK_EXPORT
bool bacnet_address_mac_same(
    const BACNET_MAC_ADDRESS *dest, const BACNET_MAC_ADDRESS *src);
BACNET_STACK_EXPORT
void bacnet_address_mac_init(
    BACNET_MAC_ADDRESS *mac, const uint8_t *adr, uint8_t len);
BACNET_STACK_EXPORT
bool bacnet_address_mac_from_ascii(BACNET_MAC_ADDRESS *mac, const char *arg);

BACNET_STACK_EXPORT
int bacnet_address_decode(
    const uint8_t *apdu, uint32_t adpu_size, BACNET_ADDRESS *value);
BACNET_STACK_EXPORT
int bacnet_address_context_decode(
    const uint8_t *apdu,
    uint32_t adpu_size,
    uint8_t tag_number,
    BACNET_ADDRESS *value);

BACNET_STACK_EXPORT
int encode_bacnet_address(uint8_t *apdu, const BACNET_ADDRESS *destination);
BACNET_STACK_EXPORT
int encode_context_bacnet_address(
    uint8_t *apdu, uint8_t tag_number, const BACNET_ADDRESS *destination);

BACNET_STACK_DEPRECATED("Use bacnet_address_decode() instead")
BACNET_STACK_EXPORT
int decode_bacnet_address(const uint8_t *apdu, BACNET_ADDRESS *destination);
BACNET_STACK_DEPRECATED("Use bacnet_address_context_decode() instead")
BACNET_STACK_EXPORT
int decode_context_bacnet_address(
    const uint8_t *apdu, uint8_t tag_number, BACNET_ADDRESS *destination);

BACNET_STACK_EXPORT
int bacnet_vmac_entry_data_encode(
    uint8_t *apdu, const BACNET_VMAC_ENTRY *value);
BACNET_STACK_EXPORT
int bacnet_vmac_entry_encode(
    uint8_t *apdu, uint32_t apdu_size, const BACNET_VMAC_ENTRY *value);
BACNET_STACK_EXPORT
int bacnet_vmac_entry_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_VMAC_ENTRY *value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
