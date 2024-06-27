/**
 * @file
 * @brief API for Network Protocol Data Unit (NPDU) encode and decode functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_NPDU_H
#define BACNET_NPDU_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/** Hop count default is required by BTL to be maximum */
#ifndef HOP_COUNT_DEFAULT
#define HOP_COUNT_DEFAULT 255
#endif

/* an NPDU structure keeps the parameter stack to a minimum */
typedef struct bacnet_npdu_data_t {
    uint8_t protocol_version;
    /* parts of the control octet: */
    bool data_expecting_reply;
    bool network_layer_message; /* false if APDU */
    BACNET_MESSAGE_PRIORITY priority;
    /* optional network message info */
    BACNET_NETWORK_MESSAGE_TYPE network_message_type;   /* optional */
    uint16_t vendor_id; /* optional, if net message type is > 0x80 */
    uint8_t hop_count;
} BACNET_NPDU_DATA;

struct router_port_t;
/** The info[] string has no agreed-upon purpose, hence it is useless.
 * Keeping it short here. This size could be 0-255. */
#define ROUTER_PORT_INFO_LEN 2
/** Port Info structure used by Routers for their routing table. */
typedef struct router_port_t {
    uint16_t dnet;              /**< The DNET number that identifies this port. */
    uint8_t id;                 /**< Either 0 or some ill-defined, meaningless value. */
    uint8_t info[ROUTER_PORT_INFO_LEN];  /**< Info like 'modem dialing string' */
    uint8_t info_len;   /**< Length of info[]. */
    struct router_port_t *next;         /**< Point to next in linked list */
} BACNET_ROUTER_PORT;

#define NETWORK_NUMBER_LEARNED 0
#define NETWORK_NUMBER_CONFIGURED 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    uint8_t npdu_encode_max_seg_max_apdu(
        int max_segs,
        int max_apdu);

    BACNET_STACK_EXPORT
    int npdu_encode_pdu(
        uint8_t * npdu,
        BACNET_ADDRESS * dest,
        BACNET_ADDRESS * src,
        BACNET_NPDU_DATA * npdu_data);

    BACNET_STACK_EXPORT
    int bacnet_npdu_encode_pdu(
        uint8_t * pdu,
        uint16_t pdu_size,
        BACNET_ADDRESS * dest,
        BACNET_ADDRESS * src,
        BACNET_NPDU_DATA * npdu_data);

    BACNET_STACK_EXPORT
    void npdu_encode_npdu_data(
        BACNET_NPDU_DATA * npdu,
        bool data_expecting_reply,
        BACNET_MESSAGE_PRIORITY priority);

    BACNET_STACK_EXPORT
    void npdu_encode_npdu_network(
        BACNET_NPDU_DATA *npdu_data,
        BACNET_NETWORK_MESSAGE_TYPE network_message_type,
        bool data_expecting_reply,
        BACNET_MESSAGE_PRIORITY priority);

    BACNET_STACK_EXPORT
    void npdu_copy_data(
        BACNET_NPDU_DATA * dest,
        BACNET_NPDU_DATA * src);

    BACNET_STACK_DEPRECATED("Use bacnet_npdu_decode() instead")
    BACNET_STACK_EXPORT
    int npdu_decode(
        uint8_t * npdu,
        BACNET_ADDRESS * dest,
        BACNET_ADDRESS * src,
        BACNET_NPDU_DATA * npdu_data);

    BACNET_STACK_EXPORT
    int bacnet_npdu_decode(
        uint8_t * npdu,
        uint16_t pdu_len,
        BACNET_ADDRESS * dest,
        BACNET_ADDRESS * src,
        BACNET_NPDU_DATA * npdu_data);

    BACNET_STACK_EXPORT
    bool npdu_confirmed_service(
        uint8_t *pdu,
        uint16_t pdu_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
