/**
 * @file
 * @brief BACnet MSTP COBS encoding for extended frames
 * @author Kerry Lynn <kerlyn@ieee.org>
 * @date 2014
 * @copyright SPDX-License-Identifier: MIT
 * @defgroup DLMSTP BACnet MS/TP DataLink Network Layer
 * @ingroup DataLink
 */
#ifndef BACNET_COBS_H
#define BACNET_COBS_H

#include <stddef.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/* number of bytes needed for COBS encoded CRC */
#define COBS_ENCODED_CRC_SIZE 5
/* inclusive extra bytes needed for APDU */
#define COBS_ENCODED_SIZE(a) ((a)+((a)/254)+1)


/* The first COBS-encoded Frame Type value: 32. */
#define Nmin_COBS_type 32
/* The last COBS-encoded Frame Type value: 127. */
#define Nmax_COBS_type 127
/* The minimum valid Length value of any COBS-encoded frame: 5.
   The theoretical minimum Length is calculated as follows:
   COBS-encoded frames must contain at least one data octet.
   The minimum COBS encoding overhead for the Encoded Data
   field is one octet. The size of the Encoded CRC-32K field
   is always five octets. Adding the lengths of these fields
   and subtracting two (for backward compatibility) results
   in a minimum Length value of five (1 + 1 + 5 - 2). In practice,
   the minimum Length value is determined by the network-layer
   client and is likely to be larger (e.g., for BACnet the minimum
   Length is 502 + 1 + 3 = 506. */
#define Nmin_COBS_length 5
#define Nmin_COBS_length_BACnet 506
/* The maximum valid Length value of any COBS-encoded frame: 2043.
   The theoretical maximum Length is calculated as follows:
   the largest data parameter that any future network client
   may specify is 2032 octets (this is near the limit of the
   CRC-32K's maximum error-detection capability). The worst-case
   COBS encoding overhead for the Encoded Data field would be
   2032 / 254 = 8 octets. Adding in the size adjustment for the
   Encoded CRC-32K results in a maximum Length value of
   2032 + 8 + 3 = 2043. In practice, the maximum Length value is
   determined by the network-layer client and is likely to be
   smaller (e.g., for BACnet the maximum Length is 1497 + 6 + 3 = 1506. */
#define Nmax_COBS_length 2043
#define Nmax_COBS_length_BACnet 1506

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
size_t cobs_encode(
    uint8_t *buffer,
    size_t buffer_size,
    const uint8_t *from,
    size_t length,
    uint8_t mask);

BACNET_STACK_EXPORT
size_t cobs_frame_encode(
    uint8_t *buffer,
    size_t buffer_size,
    const uint8_t *from,
    size_t length);

BACNET_STACK_EXPORT
size_t cobs_decode(
    uint8_t *buffer,
    size_t buffer_size,
    const uint8_t *from,
    size_t length,
    uint8_t mask);

BACNET_STACK_EXPORT
size_t cobs_frame_decode(
    uint8_t *buffer,
    size_t buffer_size,
    const uint8_t *from,
    size_t length);

BACNET_STACK_EXPORT
uint32_t cobs_crc32k(
    uint8_t dataValue,
    uint32_t crc);

BACNET_STACK_EXPORT
size_t cobs_crc32k_encode(
    uint8_t *buffer,
    size_t buffer_size,
    uint32_t crc);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
