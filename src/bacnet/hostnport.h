/**
 * @file
 * @brief API for BACnetHostNPort complex data type encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef BACNET_HOST_N_PORT_H
#define BACNET_HOST_N_PORT_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacstr.h"
#include "bacnet/datalink/bvlc.h"

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
    bool host_ip_address:1;
    bool host_name:1;
    union BACnetHostAddress {
        /* none = host_ip_address AND host_name are FALSE */
        BACNET_OCTET_STRING ip_address;
        BACNET_CHARACTER_STRING name;
    } host;
    uint16_t port;
} BACNET_HOST_N_PORT;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int host_n_port_address_encode(
        uint8_t *apdu, 
        BACNET_HOST_N_PORT *address);
    BACNET_STACK_EXPORT
    int host_n_port_encode(
        uint8_t * apdu,
        BACNET_HOST_N_PORT *address);
    BACNET_STACK_EXPORT
    int host_n_port_context_encode(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_HOST_N_PORT *address);
    BACNET_STACK_EXPORT
    int host_n_port_address_decode(uint8_t *apdu,
        uint32_t apdu_size,
        BACNET_ERROR_CODE *error_code,
        BACNET_HOST_N_PORT *address);
    BACNET_STACK_EXPORT
    int host_n_port_decode(uint8_t *apdu,
        uint32_t apdu_len,
        BACNET_ERROR_CODE *error_code,
        BACNET_HOST_N_PORT *address);
    BACNET_STACK_EXPORT
    int host_n_port_context_decode(uint8_t *apdu,
        uint32_t apdu_size,
        uint8_t tag_number,
        BACNET_ERROR_CODE *error_code,
        BACNET_HOST_N_PORT *address);
    BACNET_STACK_EXPORT
    bool host_n_port_copy(
        BACNET_HOST_N_PORT * dst,
        BACNET_HOST_N_PORT * src);
    BACNET_STACK_EXPORT
    bool host_n_port_same(
        BACNET_HOST_N_PORT * dst,
        BACNET_HOST_N_PORT * src);
    BACNET_STACK_EXPORT
    bool host_n_port_from_ascii(
        BACNET_HOST_N_PORT *value, 
        const char *argv);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
