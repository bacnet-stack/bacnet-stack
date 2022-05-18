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

#define BSC_MPDU_MAX  1440
#define BSC_VMAC_SIZE 6
#define BSC_DEVICE_UUID_SIZE 16

#if !defined(USER_DEFINED_BSC_HEADER_OPTION_MAX)
#define BSC_HEADER_OPTION_MAX 4 /* though BACNet standard does not limit number of option headers
                                   the implementation defines max value */
#else 
#define BSC_HEADER_OPTION_MAX USER_DEFINED_BSC_HEADER_OPTION_MAX
#endif

#if BSC_MPDU_MAX > 61327  /* Table 6-1. NPDU Lengths of BACnet Data Link Layers */
#error "Maximum NPDU Length on BACNet/SC Data Link must be <= 61327"
#endif

/*
 * BACnet/SC BVLC Messages (funcitons) (AB.2 BACnet/SC Virtual Link Layer Messages)
 */

typedef enum {
    BSC_BVLC_RESULT                      = 0x00,
    BSC_BVLC_ENCAPSULATED_NPDU           = 0x01,
    BSC_BVLC_ADDRESS_RESOLUTION          = 0x02,
    BSC_BVLC_ADDRESS_RESOLUTION_ACK      = 0x03,
    BSC_BVLC_ADVERTISIMENT               = 0x04,
    BSC_BVLC_ADVERTISIMENT_SOLICITATION  = 0x05,
    BSC_BVLC_CONNECT_REQUEST             = 0x06,
    BSC_BVLC_CONNECT_ACCEPT              = 0x07,
    BSC_BVLC_DISCONNECT_REQUEST          = 0x08,
    BSC_BVLC_DISCONNECT_ACK              = 0x09,
    BSC_BVLC_HEARTBEAT_REQUEST           = 0x0a,
    BSC_BVLC_HEARTBEAT_ACK               = 0x0b,
    BSC_BVLC_PROPRIETARY_MESSAGE         = 0x0c
} bsc_bvlc_message_type_t;


/*
 * AB.2.2 Control Flags
 */

#define BSC_BVLC_CONTROL_DATA_OPTIONS        (1<<0)
#define BSC_BVLC_CONTROL_DEST_OPTIONS        (1<<1)
#define BSC_BVLC_CONTROL_DEST_VADDR          (1<<2)
#define BSC_BVLC_CONTROL_ORIG_VADDR          (1<<3)

/*
 * AB.2.3 Header Options
 */

#define BSC_BVLC_HEADER_DATA                 (1 << 5)
#define BSC_BVLC_HEADER_MUST_UNDERSTAND      (1 << 6)
#define BSC_BVLC_HEADER_MORE                 (1 << 7)
#define BSL_BVLC_HEADER_OPTION_TYPE_MASK     (0x1F)

/*
 * AB.2.3.1 Secure Path Header Option
 */
typedef enum
{
   BSC_BVLC_OPTION_TYPE_SECURE_PATH = 1,
   BSC_BVLC_OPTION_TYPE_PROPRIETARY = 31
} bsc_bvlc_hdr_option_type_t;

typedef enum 
{
  BSC_BVLC_HUB_CONNECTION_ABSENT = 0,
  BSC_BVLC_HUB_CONNECTION_PRIMARY_HUB_CONNECTED = 1,
  BSC_BVLC_HUB_CONNECTION_FAILOVER_HUB_CONNECTED = 2
} bsc_bvlc_hub_connection_status_t;

typedef enum 
{
  BSC_BVLC_DIRECT_CONNECTIONS_ACCEPT_UNSUPPORTED = 0,
  BSC_BVLC_DIRECT_CONNECTIONS_ACCEPT_SUPPORTED  = 1
} bsc_bvlc_direct_connection_support_t;

typedef struct
{
  uint8_t         bvlc_function;
  uint16_t        message_id;
  BACNET_ADDRESS  origin;
  BACNET_ADDRESS  dest;
  uint8_t        *dest_options;     /* pointer to packed dest options list in message */
  uint16_t        dest_options_len;
  uint16_t        dest_options_num; /* number of filled items in dest_options */
  uint8_t        *data_options;     /* pointer to packed data options list in message */
  uint16_t        data_options_len;
  uint16_t        data_options_num; /* number of filled items in data_options */
  uint8_t        *payload;          /* packed payload, points to data in message */
  uint16_t        payload_len;
} bsc_bvlc_unpacked_hdr_t;

typedef struct
{
  uint8_t        bvlc_function;
  uint8_t        result;
  uint8_t        error_header_marker;
  uint16_t       error_class;
  uint16_t       error_code;
  uint8_t       *utf8_details_string;     /* NOTE!: this is utf 8 string without trailing zero */
  uint8_t        utf8_details_string_len; 
} bsc_bvlc_unpacked_result_t;

typedef struct 
{
  uint8_t       *utf8_websocket_uri_string;      /* NOTE!: this is utf 8 string without trailing zero */
  uint8_t        utf8_websocket_uri_string_len; 
} bsc_bvlc_unpacked_address_resolution_ack_t;

typedef struct
{
  uint8_t        *npdu;
  uint16_t        npdu_len;
} bsc_bvlc_unpacked_encapsulated_npdu_t;

typedef struct
{
  bsc_bvlc_hub_connection_status_t     hub_status;
  bsc_bvlc_direct_connection_support_t support;
  uint16_t                             max_blvc_len;
  uint16_t                             max_npdu_len;
} bsc_bvlc_unpacked_advertisiment_t;

typedef struct
{
  uint8_t  (*local_vmac)[BSC_VMAC_SIZE];
  uint8_t  (*local_uuid)[BSC_DEVICE_UUID_SIZE];
  uint16_t max_blvc_len;
  uint16_t max_npdu_len;
} bsc_bvlc_unpacked_conect_request_t;

typedef struct
{
  uint8_t  (*local_vmac)[BSC_VMAC_SIZE];
  uint8_t  (*local_uuid)[BSC_DEVICE_UUID_SIZE];
  uint16_t max_blvc_len;
  uint16_t max_npdu_len;
} bsc_bvlc_unpacked_conect_accept_t;

typedef struct
{
  uint16_t         vendor_id;
  uint8_t          proprietary_function;
  uint8_t         *proprietary_data;
  uint16_t         proprietary_data_len;
} bsc_bvlc_unpacked_proprietary_t;

typedef union {
  bsc_bvlc_unpacked_result_t                  result;
  bsc_bvlc_unpacked_encapsulated_npdu_t       encapsulated_npdu;
  bsc_bvlc_unpacked_address_resolution_ack_t  address_resolution_ack;
  bsc_bvlc_unpacked_advertisiment_t           advertisiment;
  bsc_bvlc_unpacked_conect_request_t          connect_request;
  bsc_bvlc_unpacked_conect_accept_t           connect_accept;
  bsc_bvlc_unpacked_proprietary_t             proprietary;
} bsc_bvlc_unpacked_data_t;

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
  bsc_bvlc_hdr_option_type_t                 type;
  bool                                       must_understand;
  bsc_blvc_unpacked_specific_option_data_t   specific;
} bsc_blvc_unpacked_hdr_option_t;

typedef struct
{
  bsc_bvlc_unpacked_hdr_t        hdr;
  bsc_blvc_unpacked_hdr_option_t data_options[BSC_HEADER_OPTION_MAX];
  bsc_blvc_unpacked_hdr_option_t dest_options[BSC_HEADER_OPTION_MAX];
  bsc_bvlc_unpacked_data_t       payload;
} bsc_bvlc_unpacked_message_t;


uint16_t bsc_bvlc_add_sc_option_to_destination_options(uint8_t* outbuf,
                                                       uint16_t outbuf_len,
                                                       uint8_t* blvc_message,
                                                       uint16_t blvc_message_len,
                                                       uint8_t* sc_option,
                                                       uint16_t sc_option_len);


uint16_t bsc_bvlc_add_sc_option_to_data_options(uint8_t* outbuf,
                                                uint16_t outbuf_len,
                                                uint8_t* blvc_message,
                                                uint16_t blvc_message_len,
                                                uint8_t* sc_option,
                                                uint16_t sc_option_len);

uint16_t bsc_bvlc_encode_proprietary_option(uint8_t* outbuf,
                                            uint16_t outbuf_len,
                                            bool     must_understand,
                                            uint16_t vendor_id,
                                            uint8_t  proprietary_option_type,
                                            uint8_t* proprietary_data,
                                            uint16_t proprietary_data_len);

uint16_t bsc_bvlc_encode_secure_path_option(uint8_t* outbuf,
                                            uint16_t outbuf_len,
                                            bool     must_understand);

unsigned int bsc_bvlc_encode_result(uint8_t        *out_buf,
                                    int             out_buf_len,
                                    uint16_t        message_id,
                                    BACNET_ADDRESS *origin,
                                    BACNET_ADDRESS *dest,
                                    uint8_t         bvlc_function,
                                    uint8_t         result_code,
                                    uint8_t        *error_header_marker,
                                    uint16_t       *error_class,
                                    uint16_t       *error_code,
                                    uint8_t        *utf8_details_string );

unsigned int bsc_bvlc_encode_encapsulated_npdu(uint8_t        *out_buf,
                                               int             out_buf_len,
                                               uint16_t        message_id,
                                               BACNET_ADDRESS *origin,
                                               BACNET_ADDRESS *dest,
                                               uint8_t        *npdu,
                                               uint16_t        npdu_len);

unsigned int bsc_bvlc_encode_address_resolution(uint8_t        *out_buf,
                                               int             out_buf_len,
                                               uint16_t        message_id,
                                               BACNET_ADDRESS *origin,
                                               BACNET_ADDRESS *dest);

unsigned int bsc_bvlc_encode_address_resolution_ack(uint8_t        *out_buf,
                                                    int             out_buf_len,
                                                    uint16_t        message_id,
                                                    BACNET_ADDRESS *origin,
                                                    BACNET_ADDRESS *dest,
                                                    uint8_t        *web_socket_uris,
                                                    uint16_t        web_socket_uris_len);

unsigned int bsc_bvlc_encode_advertisiment(uint8_t                              *out_buf,
                                           int                                   out_buf_len,
                                           uint16_t                              message_id,
                                           BACNET_ADDRESS                       *origin,
                                           BACNET_ADDRESS                       *dest,
                                           bsc_bvlc_hub_connection_status_t      hub_status,
                                           bsc_bvlc_direct_connection_support_t  support,
                                           uint16_t                              max_blvc_len,
                                           uint16_t                              max_npdu_len);

unsigned int bsc_bvlc_encode_advertisiment_solicitation(uint8_t        *out_buf,
                                                        int             out_buf_len,
                                                        uint16_t        message_id,
                                                        BACNET_ADDRESS *origin,
                                                        BACNET_ADDRESS *dest );

unsigned int bsc_bvlc_encode_connect_request(uint8_t        *out_buf,
                                             int             out_buf_len,
                                             uint16_t        message_id,
                                             uint8_t         (*local_vmac)[BSC_VMAC_SIZE],
                                             uint8_t         (*local_uuid)[BSC_DEVICE_UUID_SIZE],
                                             uint16_t        max_blvc_len,
                                             uint16_t        max_npdu_len);

unsigned int bsc_bvlc_encode_connect_accept(uint8_t        *out_buf,
                                            int             out_buf_len,
                                            uint16_t        message_id,
                                            uint8_t         (*local_vmac)[BSC_VMAC_SIZE],
                                            uint8_t         (*local_uuid)[BSC_DEVICE_UUID_SIZE],
                                            uint16_t        max_blvc_len,
                                            uint16_t        max_npdu_len);

unsigned int bsc_bvlc_encode_disconnect_request(uint8_t *out_buf,
                                                int      out_buf_len,
                                                uint16_t message_id);

unsigned int bsc_bvlc_encode_heartbeat_request(uint8_t *out_buf,
                                               int      out_buf_len,
                                               uint16_t message_id);

unsigned int bsc_bvlc_encode_heartbeat_ack(uint8_t *out_buf,
                                           int      out_buf_len,
                                           uint16_t message_id);

unsigned int bsc_bvlc_encode_proprietary_message(uint8_t         *out_buf,
                                                 int              out_buf_len,
                                                 uint16_t         message_id,
                                                 BACNET_ADDRESS  *origin,
                                                 BACNET_ADDRESS  *dest,
                                                 uint16_t         vendor_id,
                                                 uint8_t          proprietary_function,
                                                 uint8_t         *proprietary_data,
                                                 uint16_t         proprietary_data_len);

bool bsc_bvlc_decode_message(uint8_t                     *buf,
                             uint16_t                     buf_len,
                             bsc_bvlc_unpacked_message_t *message,
                             BACNET_ERROR_CODE           *error,
                             BACNET_ERROR_CLASS          *class);

#endif