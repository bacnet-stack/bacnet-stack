#ifndef BVLCSC_H
#define BVLCSC_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacenum.h"

#define BVLC_SC_NPDU_MAX  1440
#define BVLC_SC_VMAC_SIZE 6
#define BVLC_SC_UUID_SIZE 16

#if !defined(USER_DEFINED_BVLC_SC_HEADER_OPTION_MAX)
#define BVLC_SC_HEADER_OPTION_MAX 4 /* though BACNet standard does not limit number of option headers
                                   the implementation defines max value */
#else 
#define BVLC_SC_HEADER_OPTION_MAX USER_DEFINED_BVLC_SC_HEADER_OPTION_MAX
#endif

#if BVLC_SC_NPDU_MAX > 61327  /* Table 6-1. NPDU Lengths of BACnet Data Link Layers */
#error "Maximum NPDU Length on BACNet/SC Data Link must be <= 61327"
#endif

/*
 * BACnet/SC BVLC Messages (funcitons) (AB.2 BACnet/SC Virtual Link Layer Messages)
 */

typedef enum {
    BVLC_SC_RESULT                      = 0x00,
    BVLC_SC_ENCAPSULATED_NPDU           = 0x01,
    BVLC_SC_ADDRESS_RESOLUTION          = 0x02,
    BVLC_SC_ADDRESS_RESOLUTION_ACK      = 0x03,
    BVLC_SC_ADVERTISIMENT               = 0x04,
    BVLC_SC_ADVERTISIMENT_SOLICITATION  = 0x05,
    BVLC_SC_CONNECT_REQUEST             = 0x06,
    BVLC_SC_CONNECT_ACCEPT              = 0x07,
    BVLC_SC_DISCONNECT_REQUEST          = 0x08,
    BVLC_SC_DISCONNECT_ACK              = 0x09,
    BVLC_SC_HEARTBEAT_REQUEST           = 0x0a,
    BVLC_SC_HEARTBEAT_ACK               = 0x0b,
    BVLC_SC_PROPRIETARY_MESSAGE         = 0x0c
} bvlc_sc_message_type_t;


/*
 * AB.2.2 Control Flags
 */

#define BVLC_SC_CONTROL_DATA_OPTIONS        (1<<0)
#define BVLC_SC_CONTROL_DEST_OPTIONS        (1<<1)
#define BVLC_SC_CONTROL_DEST_VADDR          (1<<2)
#define BVLC_SC_CONTROL_ORIG_VADDR          (1<<3)

/*
 * AB.2.3 Header Options
 */

#define BVLC_SC_HEADER_DATA                 (1 << 5)
#define BVLC_SC_HEADER_MUST_UNDERSTAND      (1 << 6)
#define BVLC_SC_HEADER_MORE                 (1 << 7)
#define BSL_BVLC_HEADER_OPTION_TYPE_MASK    (0x1F)

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
* to re-use the UUID of the replaced device. For BACnet/SC, it is assumed
* that existing connections to the device being replaced are all terminated
* before the new device comes into operation.
* @{
*/

typedef struct BACnet_SC_Uuid {
    uint8_t address[BVLC_SC_UUID_SIZE];
} BACNET_SC_UUID;
/** @} */

/*
 * AB.2.3.1 Secure Path Header Option
 */
typedef enum 
{
   BVLC_SC_OPTION_TYPE_SECURE_PATH = 1,
   BVLC_SC_OPTION_TYPE_PROPRIETARY = 31
} bvlc_sc_hdr_option_type_t;

typedef enum 
{
  BVLC_SC_HUB_CONNECTION_ABSENT = 0,
  BVLC_SC_HUB_CONNECTION_PRIMARY_HUB_CONNECTED = 1,
  BVLC_SC_HUB_CONNECTION_FAILOVER_HUB_CONNECTED = 2
} bvlc_sc_hub_connection_status_t;

typedef enum 
{
  BVLC_SC_DIRECT_CONNECTIONS_ACCEPT_UNSUPPORTED = 0,
  BVLC_SC_DIRECT_CONNECTIONS_ACCEPT_SUPPORTED  = 1
} bvlc_sc_direct_connection_support_t;

typedef struct
{
  uint8_t                 bvlc_function;
  uint16_t                message_id;
  BACNET_SC_VMAC_ADDRESS *origin;
  BACNET_SC_VMAC_ADDRESS *dest;
  uint8_t                *dest_options;     /* pointer to packed dest options list in message */
  uint16_t                dest_options_len;
  uint16_t                dest_options_num; /* number of filled items in dest_options */
  uint8_t                *data_options;     /* pointer to packed data options list in message */
  uint16_t                data_options_len;
  uint16_t                data_options_num; /* number of filled items in data_options */
  uint8_t                *payload;          /* packed payload, points to data in message */
  uint16_t                payload_len;
} bvlc_sc_unpacked_hdr_t;

typedef struct
{
  uint8_t        bvlc_function;
  uint8_t        result;
  uint8_t        error_header_marker;
  uint16_t       error_class;
  uint16_t       error_code;
  uint8_t       *utf8_details_string;     /* NOTE!: this is utf 8 string without trailing zero */
  uint8_t        utf8_details_string_len; 
} bvlc_sc_unpacked_result_t;

typedef struct 
{
  uint8_t       *utf8_websocket_uri_string;      /* NOTE!: this is utf 8 string without trailing zero */
  uint8_t        utf8_websocket_uri_string_len; 
} bvlc_sc_unpacked_address_resolution_ack_t;

typedef struct
{
  uint8_t        *npdu;
  uint16_t        npdu_len;
} bvlc_sc_unpacked_encapsulated_npdu_t;

typedef struct
{
  bvlc_sc_hub_connection_status_t     hub_status;
  bvlc_sc_direct_connection_support_t support;
  uint16_t                             max_blvc_len;
  uint16_t                             max_npdu_len;
} bvlc_sc_unpacked_advertisiment_t;

typedef struct
{
  BACNET_SC_VMAC_ADDRESS *local_vmac;
  BACNET_SC_UUID         *local_uuid;
  uint16_t                max_blvc_len;
  uint16_t                max_npdu_len;
} bvlc_sc_unpacked_conect_request_t;

typedef struct
{
  BACNET_SC_VMAC_ADDRESS *local_vmac;
  BACNET_SC_UUID         *local_uuid;
  uint16_t                max_blvc_len;
  uint16_t                max_npdu_len;
} bvlc_sc_unpacked_conect_accept_t;

typedef struct
{
  uint16_t         vendor_id;
  uint8_t          proprietary_function;
  uint8_t         *proprietary_data;
  uint16_t         proprietary_data_len;
} bvlc_sc_unpacked_proprietary_t;

typedef union {
  bvlc_sc_unpacked_result_t                  result;
  bvlc_sc_unpacked_encapsulated_npdu_t       encapsulated_npdu;
  bvlc_sc_unpacked_address_resolution_ack_t  address_resolution_ack;
  bvlc_sc_unpacked_advertisiment_t           advertisiment;
  bvlc_sc_unpacked_conect_request_t          connect_request;
  bvlc_sc_unpacked_conect_accept_t           connect_accept;
  bvlc_sc_unpacked_proprietary_t             proprietary;
} bvlc_sc_unpacked_data_t;

typedef struct 
{
  uint16_t  vendor_id;
  uint8_t   option_type;
  uint8_t  *data;
  uint16_t  data_len;
} bsc_blvc_unpacked_hdr_proprietary_option_t;

typedef union {
  bsc_blvc_unpacked_hdr_proprietary_option_t proprietary;
} bsc_blvc_unpacked_specific_option_data_t;

typedef struct
{
  uint8_t                                    packed_header_marker;
  bvlc_sc_hdr_option_type_t                 type;
  bool                                       must_understand;
  bsc_blvc_unpacked_specific_option_data_t   specific;
} bsc_blvc_unpacked_hdr_option_t;

typedef struct //BACnet_SC_Unpacked_Message
{
  bvlc_sc_unpacked_hdr_t        hdr;
  bsc_blvc_unpacked_hdr_option_t data_options[BVLC_SC_HEADER_OPTION_MAX];
  bsc_blvc_unpacked_hdr_option_t dest_options[BVLC_SC_HEADER_OPTION_MAX];
  bvlc_sc_unpacked_data_t       payload;
} bvlc_sc_unpacked_message_t;
//BACNET_SC_UNPACKED_MESSAGE;
//bvlc_sc_unpacked_message_t;


uint16_t bvlc_sc_add_option_to_destination_options(uint8_t* outbuf,
                                                   uint16_t outbuf_len,
                                                   uint8_t* blvc_message,
                                                   uint16_t blvc_message_len,
                                                   uint8_t* sc_option,
                                                   uint16_t sc_option_len);


uint16_t bvlc_sc_add_option_to_data_options(uint8_t* outbuf,
                                            uint16_t outbuf_len,
                                            uint8_t* blvc_message,
                                            uint16_t blvc_message_len,
                                            uint8_t* sc_option,
                                            uint16_t sc_option_len);

uint16_t bvlc_sc_encode_proprietary_option(uint8_t* outbuf,
                                           uint16_t outbuf_len,
                                           bool     must_understand,
                                           uint16_t vendor_id,
                                           uint8_t  proprietary_option_type,
                                           uint8_t* proprietary_data,
                                           uint16_t proprietary_data_len);

uint16_t bvlc_sc_encode_secure_path_option(uint8_t* outbuf,
                                           uint16_t outbuf_len,
                                           bool     must_understand);

unsigned int bvlc_sc_encode_result(uint8_t        *out_buf,
                                   int             out_buf_len,
                                   uint16_t        message_id,
                                   BACNET_SC_VMAC_ADDRESS *origin,
                                   BACNET_SC_VMAC_ADDRESS *dest,
                                   uint8_t         bvlc_function,
                                   uint8_t         result_code,
                                   uint8_t        *error_header_marker,
                                   uint16_t       *error_class,
                                   uint16_t       *error_code,
                                   uint8_t        *utf8_details_string );

unsigned int bvlc_sc_encode_encapsulated_npdu(uint8_t        *out_buf,
                                              int             out_buf_len,
                                              uint16_t        message_id,
                                              BACNET_SC_VMAC_ADDRESS *origin,
                                              BACNET_SC_VMAC_ADDRESS *dest,
                                              uint8_t        *npdu,
                                              uint16_t        npdu_len);

unsigned int bvlc_sc_encode_address_resolution(uint8_t        *out_buf,
                                               int             out_buf_len,
                                               uint16_t        message_id,
                                               BACNET_SC_VMAC_ADDRESS *origin,
                                               BACNET_SC_VMAC_ADDRESS *dest);

unsigned int bvlc_sc_encode_address_resolution_ack(uint8_t        *out_buf,
                                                   int             out_buf_len,
                                                   uint16_t        message_id,
                                                   BACNET_SC_VMAC_ADDRESS *origin,
                                                   BACNET_SC_VMAC_ADDRESS *dest,
                                                   uint8_t        *web_socket_uris,
                                                   uint16_t        web_socket_uris_len);

unsigned int bvlc_sc_encode_advertisiment(uint8_t                              *out_buf,
                                          int                                   out_buf_len,
                                          uint16_t                              message_id,
                                          BACNET_SC_VMAC_ADDRESS                       *origin,
                                          BACNET_SC_VMAC_ADDRESS                       *dest,
                                          bvlc_sc_hub_connection_status_t      hub_status,
                                          bvlc_sc_direct_connection_support_t  support,
                                          uint16_t                              max_blvc_len,
                                          uint16_t                              max_npdu_len);

unsigned int bvlc_sc_encode_advertisiment_solicitation(uint8_t        *out_buf,
                                                       int             out_buf_len,
                                                       uint16_t        message_id,
                                                       BACNET_SC_VMAC_ADDRESS *origin,
                                                       BACNET_SC_VMAC_ADDRESS *dest );

unsigned int bvlc_sc_encode_connect_request(uint8_t                *out_buf,
                                            int                     out_buf_len,
                                            uint16_t                message_id,
                                            BACNET_SC_VMAC_ADDRESS *local_vmac,
                                            BACNET_SC_UUID         *local_uuid,
                                            uint16_t                max_blvc_len,
                                            uint16_t                max_npdu_len);

unsigned int bvlc_sc_encode_connect_accept(uint8_t        *out_buf,
                                           int             out_buf_len,
                                           uint16_t        message_id,
                                           BACNET_SC_VMAC_ADDRESS *local_vmac,
                                           BACNET_SC_UUID         *local_uuid,
                                           uint16_t        max_blvc_len,
                                           uint16_t        max_npdu_len);

unsigned int bvlc_sc_encode_disconnect_request(uint8_t *out_buf,
                                               int      out_buf_len,
                                               uint16_t message_id);

unsigned int bvlc_sc_encode_heartbeat_request(uint8_t *out_buf,
                                              int      out_buf_len,
                                              uint16_t message_id);

unsigned int bvlc_sc_encode_heartbeat_ack(uint8_t *out_buf,
                                          int      out_buf_len,
                                          uint16_t message_id);

unsigned int bvlc_sc_encode_proprietary_message(uint8_t         *out_buf,
                                                int              out_buf_len,
                                                uint16_t         message_id,
                                                BACNET_SC_VMAC_ADDRESS  *origin,
                                                BACNET_SC_VMAC_ADDRESS  *dest,
                                                uint16_t         vendor_id,
                                                uint8_t          proprietary_function,
                                                uint8_t         *proprietary_data,
                                                uint16_t         proprietary_data_len);

bool bvlc_sc_decode_message(uint8_t                     *buf,
                            uint16_t                     buf_len,
                            bvlc_sc_unpacked_message_t *message,
                            BACNET_ERROR_CODE           *error,
                            BACNET_ERROR_CLASS          *class);

#endif