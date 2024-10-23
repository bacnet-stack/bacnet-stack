/**
 * @file
 * @brief API for encoding/decoding of BACnet/SC BVLC messages
 * @author Kirill Neznamov
 * @date May 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DATALINK_BSC_BVLC_SC_H
#define BACNET_DATALINK_BSC_BVLC_SC_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"

#ifndef BVLC_SC_NPDU_SIZE_CONF
#define BVLC_SC_NPDU_SIZE 1440
#else
#define BVLC_SC_NPDU_SIZE BVLC_SC_NPDU_SIZE_CONF
#endif

#define BVLC_SC_NPDU_MAX_SIZE \
    61327 /* Table 6-1. NPDU Lengths of BACnet Data Link Layers */
#define BVLC_SC_VMAC_SIZE 6
#define BVLC_SC_UUID_SIZE 16
#define BSC_PRE (2 * BVLC_SC_VMAC_SIZE)

#if !defined(USER_DEFINED_BVLC_SC_HEADER_OPTION_MAX)
#define BVLC_SC_HEADER_OPTION_MAX                                       \
    4 /* though BACNet standard does not limit number of option headers \
     the implementation defines max value */
#else
#define BVLC_SC_HEADER_OPTION_MAX USER_DEFINED_BVLC_SC_HEADER_OPTION_MAX
#endif

#if BVLC_SC_NPDU_SIZE > BVLC_SC_NPDU_MAX_SIZE
#error \
    "Maximum NPDU Length on BACNet/SC Data Link must be <= BVLC_SC_NPDU_MAX_SIZE"
#endif

/*
 * BACnet/SC BVLC Messages (functions) (AB.2 BACnet/SC Virtual Link Layer
 * Messages)
 */

typedef enum BVLC_SC_Message_Type {
    BVLC_SC_RESULT = 0x00,
    BVLC_SC_ENCAPSULATED_NPDU = 0x01,
    BVLC_SC_ADDRESS_RESOLUTION = 0x02,
    BVLC_SC_ADDRESS_RESOLUTION_ACK = 0x03,
    BVLC_SC_ADVERTISIMENT = 0x04,
    BVLC_SC_ADVERTISIMENT_SOLICITATION = 0x05,
    BVLC_SC_CONNECT_REQUEST = 0x06,
    BVLC_SC_CONNECT_ACCEPT = 0x07,
    BVLC_SC_DISCONNECT_REQUEST = 0x08,
    BVLC_SC_DISCONNECT_ACK = 0x09,
    BVLC_SC_HEARTBEAT_REQUEST = 0x0a,
    BVLC_SC_HEARTBEAT_ACK = 0x0b,
    BVLC_SC_PROPRIETARY_MESSAGE = 0x0c
} BVLC_SC_MESSAGE_TYPE;

/*
 * AB.2.2 Control Flags
 */

#define BVLC_SC_CONTROL_DATA_OPTIONS (1 << 0)
#define BVLC_SC_CONTROL_DEST_OPTIONS (1 << 1)
#define BVLC_SC_CONTROL_DEST_VADDR (1 << 2)
#define BVLC_SC_CONTROL_ORIG_VADDR (1 << 3)

/*
 * AB.2.3 Header Options
 */

#define BVLC_SC_HEADER_DATA (1 << 5)
#define BVLC_SC_HEADER_MUST_UNDERSTAND (1 << 6)
#define BVLC_SC_HEADER_MORE (1 << 7)
#define BVLC_SC_HEADER_OPTION_TYPE_MASK (0x1F)

/**
 * BACnet SC VMAC Address
 *
 * B.1.5.2 VMAC Addressing of Nodes
 * For the BVLC message exchange, BACnet/SC nodes are identified by their
 * 6-octet virtual MAC address as defined in Clause H.7.3.
 * For broadcast BVLC messages that need to reach all nodes of the BACnet/SC
 * network, the destination VMAC address shall be the non-EUI-48 value
 * X'FFFFFFFFFFFF', referred to as the Local Broadcast VMAC address.
 * The reserved EUI-48 value X'000000000000' is not used by this data link
 * and therefore can be used internally to indicate that a VMAC is unknown
 * or uninitialized.
 * @{
 */

typedef struct BACnet_SC_VMAC_Address {
    uint8_t address[BVLC_SC_VMAC_SIZE];
} BACNET_SC_VMAC_ADDRESS;
/** @} */

/**
 * BACnet SC UUID
 * AB.1.5.3 Device UUID
 * Every BACnet device that supports one or more BACnet/SC network ports shall
 * have a Universally Unique ID (UUID) as defined in RFC 4122. This UUID
 * identifies the device regardless of its current VMAC address or device
 * instance number and is referred to as the device UUID.
 * This device UUID shall be generated before first deployment of the device in
 * an installation, shall be persistently stored across device restarts, and
 * shall not change over the entire lifetime of a device.
 * If a device is replaced in an installation, the new device is not required
 * to reuse the UUID of the replaced device. For BACnet/SC, it is assumed
 * that existing connections to the device being replaced are all terminated
 * before the new device comes into operation.
 * @{
 */

typedef struct BACnet_SC_Uuid {
    uint8_t uuid[BVLC_SC_UUID_SIZE];
} BACNET_SC_UUID;
/** @} */

/*
 * AB.2.3.1 Secure Path Header Option
 */
typedef enum BVLC_SC_Option_Type {
    BVLC_SC_OPTION_TYPE_SECURE_PATH = 1,
    BVLC_SC_OPTION_TYPE_PROPRIETARY = 31
} BVLC_SC_OPTION_TYPE;

typedef enum BVLC_SC_Direct_Connection_Support {
    BVLC_SC_DIRECT_CONNECTION_ACCEPT_UNSUPPORTED = 0,
    BVLC_SC_DIRECT_CONNECTION_ACCEPT_SUPPORTED = 1,
    BVLC_SC_DIRECT_CONNECTION_SUPPORT_MAX = 1
} BVLC_SC_DIRECT_CONNECTION_SUPPORT;

typedef struct BVLC_SC_Decoded_Hdr {
    uint8_t bvlc_function;
    uint16_t message_id;
    BACNET_SC_VMAC_ADDRESS *origin;
    BACNET_SC_VMAC_ADDRESS *dest;
    uint8_t *dest_options; /* pointer to packed dest options list in message */
    size_t dest_options_len;
    size_t dest_options_num; /* number of filled items in dest_options */
    uint8_t *data_options; /* pointer to packed data options list in message */
    size_t data_options_len;
    size_t data_options_num; /* number of filled items in data_options */
    uint8_t *payload; /* packed payload, points to data in message */
    size_t payload_len;
} BVLC_SC_DECODED_HDR;

typedef struct BVLC_SC_Decoded_Result {
    uint8_t bvlc_function;
    uint8_t result;
    uint8_t error_header_marker;
    uint16_t error_class;
    uint16_t error_code;
    uint8_t *utf8_details_string; /* NOTE!: this is utf 8 string without
                                     trailing zero */
    size_t utf8_details_string_len;
} BVLC_SC_DECODED_RESULT;

typedef struct BVLC_SC_Decoded_Address_Resolution_Ack {
    uint8_t *utf8_websocket_uri_string; /* NOTE!: this is utf 8 string without
                                           trailing zero */
    size_t utf8_websocket_uri_string_len;
} BVLC_SC_DECODED_ADDRESS_RESOLUTION_ACK;

typedef struct BVLC_SC_Decoded_Ecapsulated_NPDU {
    uint8_t *npdu;
    size_t npdu_len;
} BVLC_SC_DECODED_ENCAPSULATED_NPDU;

typedef struct BVLC_SC_Decoded_Advertisiment {
    BACNET_SC_HUB_CONNECTOR_STATE hub_status;
    BVLC_SC_DIRECT_CONNECTION_SUPPORT support;
    uint16_t max_bvlc_len;
    uint16_t max_npdu_len;
} BVLC_SC_DECODED_ADVERTISIMENT;

typedef struct BVLC_SC_Decoded_Connect_Request {
    BACNET_SC_VMAC_ADDRESS *vmac;
    BACNET_SC_UUID *uuid;
    uint16_t max_bvlc_len;
    uint16_t max_npdu_len;
} BVLC_SC_DECODED_CONNECT_REQUEST;

typedef struct BVLC_SC_Decoded_Connect_Accept {
    BACNET_SC_VMAC_ADDRESS *vmac;
    BACNET_SC_UUID *uuid;
    uint16_t max_bvlc_len;
    uint16_t max_npdu_len;
} BVLC_SC_DECODED_CONNECT_ACCEPT;

typedef struct BVLC_SC_Decoded_Proprietary {
    uint16_t vendor_id;
    uint8_t function;
    uint8_t *data;
    size_t data_len;
} BVLC_SC_DECODED_PROPRIETARY;

typedef union BVLC_SC_Decoded_Data {
    BVLC_SC_DECODED_RESULT result;
    BVLC_SC_DECODED_ENCAPSULATED_NPDU encapsulated_npdu;
    BVLC_SC_DECODED_ADDRESS_RESOLUTION_ACK address_resolution_ack;
    BVLC_SC_DECODED_ADVERTISIMENT advertisiment;
    BVLC_SC_DECODED_CONNECT_REQUEST connect_request;
    BVLC_SC_DECODED_CONNECT_ACCEPT connect_accept;
    BVLC_SC_DECODED_PROPRIETARY proprietary;
} BVLC_SC_DECODED_DATA;

typedef struct BVLC_SC_Decoded_Hdr_Proprietary_Option {
    uint16_t vendor_id;
    uint8_t option_type;
    uint8_t *data;
    size_t data_len;
} BVLC_SC_DECODED_HDR_PROPRIETARY_OPTION;

typedef union BVLC_SC_Decoded_Specific_Option_Data {
    BVLC_SC_DECODED_HDR_PROPRIETARY_OPTION proprietary;
} BVLC_SC_DECODED_SPECIFIC_OPTION_DATA;

typedef struct BVLC_SC_Decoded_Hdr_Option {
    uint8_t packed_header_marker;
    BVLC_SC_OPTION_TYPE type;
    bool must_understand;
    BVLC_SC_DECODED_SPECIFIC_OPTION_DATA specific;
} BVLC_SC_DECODED_HDR_OPTION;

typedef struct BVLC_SC_Decoded_Message {
    BVLC_SC_DECODED_HDR hdr;
    BVLC_SC_DECODED_HDR_OPTION data_options[BVLC_SC_HEADER_OPTION_MAX];
    BVLC_SC_DECODED_HDR_OPTION dest_options[BVLC_SC_HEADER_OPTION_MAX];
    BVLC_SC_DECODED_DATA payload;
} BVLC_SC_DECODED_MESSAGE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
size_t bvlc_sc_add_option_to_destination_options(
    uint8_t *out_pdu,
    size_t out_pdu_size,
    uint8_t *pdu,
    size_t pdu_size,
    uint8_t *sc_option,
    size_t sc_option_len);

BACNET_STACK_EXPORT
size_t bvlc_sc_add_option_to_data_options(
    uint8_t *out_pdu,
    size_t out_pdu_size,
    uint8_t *pdu,
    size_t pdu_size,
    uint8_t *sc_option,
    size_t sc_option_len);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_proprietary_option(
    uint8_t *pdu,
    size_t pdu_size,
    bool must_understand,
    uint16_t vendor_id,
    uint8_t proprietary_option_type,
    uint8_t *proprietary_data,
    size_t proprietary_data_len);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_secure_path_option(
    uint8_t *pdu, size_t pdu_size, bool must_understand);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_result(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t bvlc_function,
    uint8_t result_code,
    uint8_t *error_header_marker,
    uint16_t *error_class,
    uint16_t *error_code,
    uint8_t *utf8_details_string);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_encapsulated_npdu(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *npdu,
    size_t npdu_size);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_address_resolution(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_address_resolution_ack(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *web_socket_uris,
    size_t web_socket_uris_len);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_advertisiment(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    BACNET_SC_HUB_CONNECTOR_STATE hub_status,
    BVLC_SC_DIRECT_CONNECTION_SUPPORT support,
    uint16_t max_bvlc_len,
    uint16_t max_npdu_size);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_advertisiment_solicitation(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_connect_request(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *local_vmac,
    BACNET_SC_UUID *local_uuid,
    uint16_t max_bvlc_len,
    uint16_t max_npdu_size);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_connect_accept(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *local_vmac,
    BACNET_SC_UUID *local_uuid,
    uint16_t max_bvlc_len,
    uint16_t max_npdu_len);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_disconnect_request(
    uint8_t *pdu, size_t pdu_len, uint16_t message_id);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_disconnect_ack(
    uint8_t *pdu, size_t pdu_len, uint16_t message_id);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_heartbeat_request(
    uint8_t *out_buf, size_t out_buf_len, uint16_t message_id);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_heartbeat_ack(
    uint8_t *out_buf, size_t out_buf_len, uint16_t message_id);

BACNET_STACK_EXPORT
size_t bvlc_sc_encode_proprietary_message(
    uint8_t *pdu,
    size_t pdu_len,
    uint16_t message_id,
    BACNET_SC_VMAC_ADDRESS *origin,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint16_t vendor_id,
    uint8_t proprietary_function,
    uint8_t *proprietary_data,
    size_t proprietary_data_len);

BACNET_STACK_EXPORT
bool bvlc_sc_decode_message(
    uint8_t *buf,
    size_t buf_len,
    BVLC_SC_DECODED_MESSAGE *message,
    BACNET_ERROR_CODE *error,
    BACNET_ERROR_CLASS *class,
    const char **err_desc);

BACNET_STACK_EXPORT
void bvlc_sc_remove_dest_set_orig(
    uint8_t *pdu, size_t pdu_len, BACNET_SC_VMAC_ADDRESS *orig);

BACNET_STACK_EXPORT
size_t
bvlc_sc_set_orig(uint8_t **ppdu, size_t pdu_len, BACNET_SC_VMAC_ADDRESS *orig);

BACNET_STACK_EXPORT
bool bvlc_sc_is_vmac_broadcast(BACNET_SC_VMAC_ADDRESS *vmac);

BACNET_STACK_EXPORT
bool bvlc_sc_need_send_bvlc_result(BVLC_SC_DECODED_MESSAGE *dm);

BACNET_STACK_EXPORT
bool bvlc_sc_pdu_has_dest_broadcast(uint8_t *pdu, size_t pdu_len);

BACNET_STACK_EXPORT
bool bvlc_sc_pdu_has_no_dest(uint8_t *pdu, size_t pdu_len);

BACNET_STACK_EXPORT
bool bvlc_sc_pdu_get_dest(
    uint8_t *pdu, size_t pdu_len, BACNET_SC_VMAC_ADDRESS *vmac);

BACNET_STACK_EXPORT
size_t bvlc_sc_remove_orig_and_dest(uint8_t **ppdu, size_t pdu_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
