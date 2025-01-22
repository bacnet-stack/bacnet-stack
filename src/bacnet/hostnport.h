/**
 * @file
 * @brief API for BACnetHostNPort complex data type encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_HOST_N_PORT_H
#define BACNET_HOST_N_PORT_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacstr.h"

/**
 *  BACnetHostNPort ::= SEQUENCE {
 *      host [0] BACnetHostAddress,
 *          BACnetHostAddress ::= CHOICE {
 *              none [0] NULL,
 *              ip-address [1] OCTET STRING,
 *              name [2] CharacterString
 *          }
 *      port [1] Unsigned16
 *  }
 */
typedef struct BACnetHostNPort {
    bool host_ip_address : 1;
    bool host_name : 1;
    union BACnetHostAddress {
        /* none = host_ip_address AND host_name are FALSE */
        BACNET_OCTET_STRING ip_address;
        BACNET_CHARACTER_STRING name;
    } host;
    uint16_t port;
} BACNET_HOST_N_PORT;

/**
 *  BACnetBDTEntry ::= SEQUENCE {
 *      bbmd-address [0] BACnetHostNPort,
 *      broadcast-mask [1] OCTET STRING OPTIONAL
 *      -- shall be present if BACnet/IP, and absent for BACnet/IPv6
 *  }
 */
typedef struct BACnetBDTEntry {
    BACNET_HOST_N_PORT bbmd_address;
    BACNET_OCTET_STRING broadcast_mask;
} BACNET_BDT_ENTRY;

/**
 *  BACnetFDTEntry ::= SEQUENCE {
 *      bacnetip-address [0] OCTET STRING,
 *      -- the 6-octet B/IP or 18-octet B/IPv6 address of the registrant
 *      time-to-live [1] Unsigned16,
 *      -- time to live in seconds at the time of registration
 *      remaining-time-to-live [2] Unsigned16
 *      -- remaining time to live in seconds, incl. grace period
 * }
 */
typedef struct BACnetFDTEntry {
    BACNET_OCTET_STRING bacnetip_address;
    uint16_t time_to_live;
    uint16_t remaining_time_to_live;
} BACNET_FDT_ENTRY;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int host_n_port_address_encode(
    uint8_t *apdu, const BACNET_HOST_N_PORT *address);
BACNET_STACK_EXPORT
int host_n_port_encode(uint8_t *apdu, const BACNET_HOST_N_PORT *address);
BACNET_STACK_EXPORT
int host_n_port_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_HOST_N_PORT *address);
BACNET_STACK_EXPORT
int host_n_port_address_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_ERROR_CODE *error_code,
    BACNET_HOST_N_PORT *address);
BACNET_STACK_EXPORT
int host_n_port_decode(
    const uint8_t *apdu,
    uint32_t apdu_len,
    BACNET_ERROR_CODE *error_code,
    BACNET_HOST_N_PORT *address);
BACNET_STACK_EXPORT
int host_n_port_context_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_ERROR_CODE *error_code,
    BACNET_HOST_N_PORT *address);
BACNET_STACK_EXPORT
bool host_n_port_copy(BACNET_HOST_N_PORT *dst, const BACNET_HOST_N_PORT *src);
BACNET_STACK_EXPORT
bool host_n_port_same(
    const BACNET_HOST_N_PORT *dst, const BACNET_HOST_N_PORT *src);
BACNET_STACK_EXPORT
bool host_n_port_from_ascii(BACNET_HOST_N_PORT *value, const char *argv);

BACNET_STACK_EXPORT
int bacnet_bdt_entry_encode(uint8_t *apdu, const BACNET_BDT_ENTRY *entry);
BACNET_STACK_EXPORT
int bacnet_bdt_entry_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_BDT_ENTRY *entry);
BACNET_STACK_EXPORT
int bacnet_bdt_entry_decode(
    const uint8_t *apdu,
    uint32_t apdu_len,
    BACNET_ERROR_CODE *error_code,
    BACNET_BDT_ENTRY *address);
BACNET_STACK_EXPORT
int bacnet_bdt_entry_context_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_ERROR_CODE *error_code,
    BACNET_BDT_ENTRY *address);
BACNET_STACK_EXPORT
bool bacnet_bdt_entry_copy(BACNET_BDT_ENTRY *dst, const BACNET_BDT_ENTRY *src);
BACNET_STACK_EXPORT
bool bacnet_bdt_entry_same(
    const BACNET_BDT_ENTRY *dst, const BACNET_BDT_ENTRY *src);
BACNET_STACK_EXPORT
bool bacnet_bdt_entry_from_ascii(BACNET_BDT_ENTRY *value, const char *argv);
BACNET_STACK_EXPORT
int bacnet_bdt_entry_to_ascii(
    char *str, size_t str_size, const BACNET_BDT_ENTRY *value);

BACNET_STACK_EXPORT
int bacnet_fdt_entry_encode(uint8_t *apdu, const BACNET_FDT_ENTRY *entry);
BACNET_STACK_EXPORT
int bacnet_fdt_entry_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_FDT_ENTRY *entry);
BACNET_STACK_EXPORT
int bacnet_fdt_entry_decode(
    const uint8_t *apdu,
    uint32_t apdu_len,
    BACNET_ERROR_CODE *error_code,
    BACNET_FDT_ENTRY *address);
BACNET_STACK_EXPORT
int bacnet_fdt_entry_context_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_ERROR_CODE *error_code,
    BACNET_FDT_ENTRY *address);
BACNET_STACK_EXPORT
bool bacnet_fdt_entry_copy(BACNET_FDT_ENTRY *dst, const BACNET_FDT_ENTRY *src);
BACNET_STACK_EXPORT
bool bacnet_fdt_entry_same(
    const BACNET_FDT_ENTRY *dst, const BACNET_FDT_ENTRY *src);
BACNET_STACK_EXPORT
bool bacnet_fdt_entry_from_ascii(BACNET_FDT_ENTRY *value, const char *argv);
BACNET_STACK_EXPORT
int bacnet_fdt_entry_to_ascii(
    char *str, size_t str_size, const BACNET_FDT_ENTRY *value);
BACNET_STACK_EXPORT
bool bacnet_is_valid_hostname(const BACNET_CHARACTER_STRING *const hostname);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
