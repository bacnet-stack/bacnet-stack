/**
 * @file
 * @brief BACnet Security Wrapper module from Clause 24 of the BACnet Standard.
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015 
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SECURITY_H
#define BACNET_SECURITY_H

#define MAX_AUTH_DATA_LEN 16
#define MD5_KEY_SIZE 16
#define AES_KEY_SIZE 16
#define SHA256_KEY_SIZE 32
#define MAX_KEY_LEN 48
#define MAX_UPDATE_KEY_COUNT 32
#define MAX_INCORRECT_KEYS 255
#define MAX_SUPPORTED_ALGORITHMS 255
#define MAX_PAD_LEN 16
#define SIGNATURE_LEN 16

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

typedef struct BACnet_Security_Wrapper {
    bool payload_net_or_bvll_flag;      /* true if NPDU or BVLL */
    bool encrypted_flag;
    bool authentication_flag;   /* always false for responses */
    bool do_not_unwrap_flag;    /* always true if do-not-encrypt is true */
    bool do_not_decrypt_flag;   /* when encrypted flag is false, it also false */
    bool non_trusted_source_flag;
    bool secured_by_router_flag;
    uint8_t key_revision;       /* 0 for Device-Master key */
    uint16_t key_identifier;
    uint32_t source_device_instance;
    uint32_t message_id;        /* monotonically increased value */
    uint32_t timestamp; /* seconds from UTC 1970-1-1 00:00:00 */
    uint32_t destination_device_instance;
    uint16_t dnet;
    uint8_t dlen;
    uint8_t dadr[MAX_MAC_LEN];
    uint16_t snet;
    uint8_t slen;
    uint8_t sadr[MAX_MAC_LEN];
    uint8_t authentication_mechanism;   /* present when User-Authenticated or
                                         * Application-Specific keys are used with: */
    /* APDU: Confrmed-Request, Unconfirmed-Request */
    /* NPDU: Initialize-Routing-Table, Establish-Connection-To-Network,
     * Disconnect-Connection-To-Network */
    /* BVLL: Write-Broadcast-Distribution-Table, Read-Broadcast-Distribution-Table,
     * Register-Foreign-Device, Read-Foreign-Device-Table,
     * Delete-Foreign-Device-Table-Entry */
    /* 0 is only legitimate value for now. 200-255 are vendor-specific */
    uint16_t user_id;   /* 0 for unknown */
    uint8_t user_role;  /* 0 and 1 are "system users": 0 for device-to-device non-human,
                         * 1 for device-to-device by unknown human */
    uint16_t authentication_data_length;        /* authentication mechanism 1-255 */
    uint16_t vendor_id; /* authentication mechanism 200-255 */
    uint8_t authentication_data[MAX_AUTH_DATA_LEN];     /* other than id, role, length and
                                                         * vendor-id */
    uint16_t service_data_len;  /* case-to-case */
    uint8_t *service_data;
    uint8_t service_type;       /* first octet of service_data */
    uint16_t padding_len;       /* included in padding */
    uint8_t padding[MAX_PAD_LEN];
    uint8_t signature[SIGNATURE_LEN];   /* hmac-md5 or hmac-sha256, first 16 bytes */
} BACNET_SECURITY_WRAPPER;

typedef struct Challenge_Request {
    uint8_t message_challenge;  /* 1 as a response, everything else for other */
    uint32_t orig_message_id;
    uint32_t orig_timestamp;
} BACNET_CHALLENGE_REQUEST;

typedef struct Security_Payload {
    uint16_t payload_length;
    uint8_t *payload;
} BACNET_SECURITY_PAYLOAD;

struct Bad_Timestamp {
    uint32_t expected_timestamp;
};

struct Cannot_Use_Key {
    uint16_t key;
};

struct Incorrect_Key {
    uint8_t number_of_keys;
    uint16_t keys[MAX_INCORRECT_KEYS];
};

struct Unknown_Authentication_Type {
    uint8_t original_authentication_type;
    uint16_t vendor_id;
};

struct Unknown_Key {
    uint16_t original_key;
};

struct Unknown_Key_Revision {
    uint8_t original_key_revision;
};

struct Too_Many_Keys {
    uint8_t max_num_of_keys;
};

struct Invalid_Key_Data {
    uint16_t key;
};

typedef struct Security_Response {
    uint16_t response_code;
    uint32_t orig_message_id;
    uint32_t orig_timestamp;
    union {
        struct Bad_Timestamp bad_timestamp;
        struct Cannot_Use_Key cannot_use_key;
        struct Incorrect_Key incorrect_key;
        struct Unknown_Authentication_Type unknown_authentication_type;
        struct Unknown_Key unknown_key;
        struct Unknown_Key_Revision unknown_key_revision;
        struct Too_Many_Keys too_many_keys;
        struct Invalid_Key_Data invalid_key_data;
    } response;
} BACNET_SECURITY_RESPONSE;

typedef struct Request_Key_Update {
    uint8_t set_1_key_revision;
    uint32_t set_1_activation_time;
    uint32_t set_1_expiration_time;
    uint8_t set_2_key_revision;
    uint32_t set_2_activation_time;
    uint32_t set_2_expiration_time;
    uint8_t distribution_key_revision;
} BACNET_REQUEST_KEY_UPDATE;

typedef struct Key_Entry {
    uint16_t key_identifier;
    uint8_t key_len;
    uint8_t key[MAX_KEY_LEN];
} BACNET_KEY_ENTRY;

typedef struct Update_Key_Set {
    bool set_rae[2], set_ck[2], set_clr[2];
    bool more;
    bool remove;        /* false for add, true for remove */
    uint8_t set_key_revision[2];
    uint32_t set_activation_time[2];
    uint32_t set_expiration_time[2];
    uint8_t set_key_count[2];
    BACNET_KEY_ENTRY set_keys[2][MAX_UPDATE_KEY_COUNT];
} BACNET_UPDATE_KEY_SET;

typedef struct Update_Distribution_Key {
    uint8_t key_revision;
    BACNET_KEY_ENTRY key;
} BACNET_UPDATE_DISTRIBUTION_KEY;

typedef struct Request_Master_Key {
    uint8_t no_supported_algorithms;
    uint8_t es_algorithms[MAX_SUPPORTED_ALGORITHMS];
} BACNET_REQUEST_MASTER_KEY;

typedef struct Set_Master_Key {
    BACNET_KEY_ENTRY key;
} BACNET_SET_MASTER_KEY;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* helper functions */
    BACNET_STACK_EXPORT
    BACNET_KEY_IDENTIFIER_ALGORITHM key_algorithm(uint16_t id);
    BACNET_STACK_EXPORT
    BACNET_KEY_IDENTIFIER_KEY_NUMBER key_number(uint16_t id);

/* key manipulation functions - port specific! */
    BACNET_STACK_EXPORT
    BACNET_SECURITY_RESPONSE_CODE bacnet_master_key_set(BACNET_SET_MASTER_KEY *
        key);
    BACNET_STACK_EXPORT
    BACNET_SECURITY_RESPONSE_CODE
        bacnet_distribution_key_update(BACNET_UPDATE_DISTRIBUTION_KEY * key);
    BACNET_STACK_EXPORT
    BACNET_SECURITY_RESPONSE_CODE bacnet_key_set_update(BACNET_UPDATE_KEY_SET *
        update_key_sets);
    BACNET_STACK_EXPORT
    BACNET_SECURITY_RESPONSE_CODE bacnet_find_key(uint8_t revision,
        BACNET_KEY_ENTRY * key);

/* signing/verification and encryption/decryption - port specific */
    BACNET_STACK_EXPORT
    int key_sign_msg(BACNET_KEY_ENTRY * key,
        uint8_t * msg,
        uint32_t msg_len,
        uint8_t * signature);
    /* BACNET_STACK_EXPORT */
    /* bool key_verify_sign_msg(BACNET_KEY_ENTRY * key, */
    /*     uint8_t * msg, */
    /*     uint32_t msg_len, */
    /*     uint8_t * signature); */
    BACNET_STACK_EXPORT
    int key_encrypt_msg(BACNET_KEY_ENTRY * key,
        uint8_t * msg,
        uint32_t msg_len,
        uint8_t * signature);
    BACNET_STACK_EXPORT
    bool key_decrypt_msg(BACNET_KEY_ENTRY * key,
        uint8_t * msg,
        uint32_t msg_len,
        uint8_t * signature);
    BACNET_STACK_EXPORT
    void key_set_padding(BACNET_KEY_ENTRY * key,
        int enc_len,
        uint16_t * padding_len,
        uint8_t * padding);

/* encoders */
    /* BACNET_STACK_EXPORT */
    /* int encode_security_wrapper(int bytes_before, */
    /*     uint8_t * apdu, */
    /*     BACNET_SECURITY_WRAPPER * wrapper); */
    BACNET_STACK_EXPORT
    int encode_challenge_request(uint8_t * apdu,
        BACNET_CHALLENGE_REQUEST * bc_req);
    BACNET_STACK_EXPORT
    int encode_security_payload(uint8_t * apdu,
        BACNET_SECURITY_PAYLOAD * payload);
    BACNET_STACK_EXPORT
    int encode_security_response(uint8_t * apdu,
        BACNET_SECURITY_RESPONSE * resp);
    BACNET_STACK_EXPORT
    int encode_request_key_update(uint8_t * apdu,
        BACNET_REQUEST_KEY_UPDATE * req);
    BACNET_STACK_EXPORT
    int encode_key_entry(uint8_t * apdu,
        BACNET_KEY_ENTRY * entry);
    BACNET_STACK_EXPORT
    int encode_update_key_set(uint8_t * apdu,
        BACNET_UPDATE_KEY_SET * key_set);
    BACNET_STACK_EXPORT
    int encode_update_distribution_key(uint8_t * apdu,
        BACNET_UPDATE_DISTRIBUTION_KEY * dist_key);
    BACNET_STACK_EXPORT
    int encode_request_master_key(uint8_t * apdu,
        BACNET_REQUEST_MASTER_KEY * req_master_key);
    BACNET_STACK_EXPORT
    int encode_set_master_key(uint8_t * apdu,
        BACNET_SET_MASTER_KEY * set_master_key);

/* safe decoders */
    /* BACNET_STACK_EXPORT */
    /* int decode_security_wrapper_safe(int bytes_before, */
    /*     uint8_t * apdu, */
    /*     uint32_t apdu_len_remaining, */
    /*     BACNET_SECURITY_WRAPPER * wrapper); */
    BACNET_STACK_EXPORT
    int decode_challenge_request_safe(uint8_t * apdu,
        uint32_t apdu_len_remaining,
        BACNET_CHALLENGE_REQUEST * bc_req);
    BACNET_STACK_EXPORT
    int decode_security_payload_safe(uint8_t * apdu,
        uint32_t apdu_len_remaining,
        BACNET_SECURITY_PAYLOAD * payload);
    BACNET_STACK_EXPORT
    int decode_security_response_safe(uint8_t * apdu,
        uint32_t apdu_len_remaining,
        BACNET_SECURITY_RESPONSE * resp);
    BACNET_STACK_EXPORT
    int decode_request_key_update_safe(uint8_t * apdu,
        uint32_t apdu_len_remaining,
        BACNET_REQUEST_KEY_UPDATE * req);
    BACNET_STACK_EXPORT
    int decode_key_entry_safe(uint8_t * apdu,
        uint32_t apdu_len_remaining,
        BACNET_KEY_ENTRY * entry);
    BACNET_STACK_EXPORT
    int decode_update_key_set_safe(uint8_t * apdu,
        uint32_t apdu_len_remaining,
        BACNET_UPDATE_KEY_SET * key_set);
    BACNET_STACK_EXPORT
    int decode_update_distribution_key_safe(uint8_t * apdu,
        uint32_t apdu_len_remaining,
        BACNET_UPDATE_DISTRIBUTION_KEY * dist_key);
    BACNET_STACK_EXPORT
    int decode_request_master_key_safe(uint8_t * apdu,
        uint32_t apdu_len_remaining,
        BACNET_REQUEST_MASTER_KEY * req_master_key);
    BACNET_STACK_EXPORT
    int decode_set_master_key_safe(uint8_t * apdu,
        uint32_t apdu_len_remaining,
        BACNET_SET_MASTER_KEY * set_master_key);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
