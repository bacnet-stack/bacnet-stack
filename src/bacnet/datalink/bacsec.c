/**
 * @file
 * @brief BACnet Security Wrapper module from Clause 24 of the BACnet Standard.
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "bacnet/bacdcode.h"
#include "bacnet/datalink/bacsec.h"

BACNET_KEY_IDENTIFIER_ALGORITHM key_algorithm(uint16_t id)
{
    return (BACNET_KEY_IDENTIFIER_ALGORITHM)((id >> 8) & 0xFF);
}

BACNET_KEY_IDENTIFIER_KEY_NUMBER key_number(uint16_t id)
{
    return (BACNET_KEY_IDENTIFIER_KEY_NUMBER)(id & 0xFF);
}

#if 0
/* FIXME: please fix? */
int encode_security_wrapper(
    int bytes_before, uint8_t *apdu, BACNET_SECURITY_WRAPPER *wrapper)
{
    int curr = 0;
    int enc_begin = 0;
    BACNET_KEY_ENTRY key;
    BACNET_SECURITY_RESPONSE_CODE res = SEC_RESP_SUCCESS;

    apdu[curr] = 0;
    /* control byte */
    if (wrapper->payload_net_or_bvll_flag) {
        apdu[curr] |= 1 << 7;
    }
    /* encryption flag will be set after signature calculation */
    /* bit 5 is reserved and shall be 0 */
    if (wrapper->authentication_flag) {
        apdu[curr] |= 1 << 4;
    }
    if (wrapper->do_not_unwrap_flag) {
        apdu[curr] |= 1 << 3;
    }
    if (wrapper->do_not_decrypt_flag) {
        apdu[curr] |= 1 << 2;
    }
    if (wrapper->non_trusted_source_flag) {
        apdu[curr] |= 1 << 1;
    }
    if (wrapper->secured_by_router_flag) {
        apdu[curr] |= 1;
    }
    curr++;
    /* basic integrity checks */
    if (wrapper->do_not_decrypt_flag && !wrapper->do_not_unwrap_flag) {
        return -SEC_RESP_MALFORMED_MESSAGE;
    }
    if (!wrapper->encrypted_flag && wrapper->do_not_decrypt_flag) {
        return -SEC_RESP_MALFORMED_MESSAGE;
    }
    /* key */
    apdu[curr++] = wrapper->key_revision;
    curr += encode_unsigned16(&apdu[curr], wrapper->key_identifier);
    /* find appropriate key */
    key.key_identifier = wrapper->key_identifier;
    res = bacnet_find_key(wrapper->key_revision, &key);
    if (res != SEC_RESP_SUCCESS) {
        return -res;
    }
    /* source device instance */
    curr += encode_unsigned24(&apdu[curr], wrapper->source_device_instance);
    /* message id */
    curr += encode_unsigned32(&apdu[curr], wrapper->message_id);
    /* timestamp */
    curr += encode_unsigned32(&apdu[curr], wrapper->timestamp);
    /* begin encryption starting from destination device instance */
    enc_begin = curr;
    /* destination device instance */
    curr +=
        encode_unsigned24(&apdu[curr], wrapper->destination_device_instance);
    /* dst address */
    curr += encode_unsigned16(&apdu[curr], wrapper->dnet);
    apdu[curr++] = wrapper->dlen;
    memcpy(&apdu[curr], wrapper->dadr, wrapper->dlen);
    curr += wrapper->dlen;
    /* src address */
    curr += encode_unsigned16(&apdu[curr], wrapper->snet);
    apdu[curr++] = wrapper->slen;
    memcpy(&apdu[curr], wrapper->sadr, wrapper->slen);
    curr += wrapper->slen;
    /* authentication */
    if (wrapper->authentication_flag) {
        apdu[curr++] = wrapper->authentication_mechanism;
        /* authentication data */
        curr += encode_unsigned16(&apdu[curr], wrapper->user_id);
        apdu[curr++] = wrapper->user_role;
        if ((wrapper->authentication_mechanism >= 1) &&
            (wrapper->authentication_mechanism <= 199)) {
            curr += encode_unsigned16(
                &apdu[curr], wrapper->authentication_data_length + 5);
            memcpy(&apdu[curr], wrapper->authentication_data,
                wrapper->authentication_data_length);
            curr += wrapper->authentication_data_length;
        } else if (wrapper->authentication_mechanism >= 200) {
            curr += encode_unsigned16(
                &apdu[curr], wrapper->authentication_data_length + 7);
            curr += encode_unsigned16(&apdu[curr], wrapper->vendor_id);
            memcpy(&apdu[curr], wrapper->authentication_data,
                wrapper->authentication_data_length);
            curr += wrapper->authentication_data_length;
        }
    }
    memcpy(&apdu[curr], wrapper->service_data, wrapper->service_data_len);
    curr += wrapper->service_data_len;
    /* signature calculation */
    key_sign_msg(&key, &apdu[-bytes_before], (uint32_t)(bytes_before + curr),
        wrapper->signature);
    /* padding and encryption */
    if (wrapper->encrypted_flag) {
        /* set encryption flag, signing is done */
        apdu[0] |= 1 << 6;
        /* handle padding */
        key_set_padding(
            &key, curr - enc_begin, &wrapper->padding_len, wrapper->padding);
        if (wrapper->padding_len > 2) {
            memcpy(&apdu[curr], wrapper->padding, wrapper->padding_len - 2);
            curr += wrapper->padding_len - 2;
        }
        curr += encode_unsigned16(&apdu[curr], wrapper->padding_len);
        /* encryption */
        key_encrypt_msg(&key, &apdu[enc_begin], (uint32_t)(curr - enc_begin),
            wrapper->signature);
    }
    memcpy(&apdu[curr], wrapper->signature, SIGNATURE_LEN);
    curr += SIGNATURE_LEN;

    return curr;
}
#endif

int encode_challenge_request(
    uint8_t *apdu, const BACNET_CHALLENGE_REQUEST *bc_req)
{
    int curr = 0;

    apdu[curr++] = bc_req->message_challenge;
    curr += encode_unsigned32(&apdu[curr], bc_req->orig_message_id);
    curr += encode_unsigned32(&apdu[curr], bc_req->orig_timestamp);

    return curr;
}

int encode_security_payload(
    uint8_t *apdu, const BACNET_SECURITY_PAYLOAD *payload)
{
    encode_unsigned16(&apdu[0], payload->payload_length);
    memcpy(&apdu[2], payload->payload, payload->payload_length);

    return (int)(2 + payload->payload_length);
}

int encode_security_response(
    uint8_t *apdu, const BACNET_SECURITY_RESPONSE *resp)
{
    int curr = 0;
    int i;

    apdu[curr++] = resp->response_code;
    curr += encode_unsigned32(&apdu[curr], resp->orig_message_id);
    curr += encode_unsigned32(&apdu[curr], resp->orig_timestamp);
    switch ((BACNET_SECURITY_RESPONSE_CODE)resp->response_code) {
        case SEC_RESP_BAD_TIMESTAMP:
            curr += encode_unsigned32(
                &apdu[curr], resp->response.bad_timestamp.expected_timestamp);
            break;
        case SEC_RESP_CANNOT_USE_KEY:
            curr += encode_unsigned16(
                &apdu[curr], resp->response.cannot_use_key.key);
            break;
        case SEC_RESP_INCORRECT_KEY:
            apdu[curr++] = resp->response.incorrect_key.number_of_keys;
            for (i = 0; i < (int)resp->response.incorrect_key.number_of_keys;
                 i++) {
                curr += encode_unsigned16(
                    &apdu[curr], resp->response.incorrect_key.keys[i]);
            }
            break;
        case SEC_RESP_UNKNOWN_AUTHENTICATION_TYPE:
            apdu[curr++] = resp->response.unknown_authentication_type
                               .original_authentication_type;
            curr += encode_unsigned16(
                &apdu[curr],
                resp->response.unknown_authentication_type.vendor_id);
            break;
        case SEC_RESP_UNKNOWN_KEY:
            curr += encode_unsigned16(
                &apdu[curr], resp->response.unknown_key.original_key);
            break;
        case SEC_RESP_UNKNOWN_KEY_REVISION:
            apdu[curr++] =
                resp->response.unknown_key_revision.original_key_revision;
            break;
        case SEC_RESP_TOO_MANY_KEYS:
            apdu[curr++] = resp->response.too_many_keys.max_num_of_keys;
            break;
        case SEC_RESP_INVALID_KEY_DATA:
            curr += encode_unsigned16(
                &apdu[curr], resp->response.invalid_key_data.key);
            break;
        case SEC_RESP_SUCCESS:
        case SEC_RESP_ACCESS_DENIED:
        case SEC_RESP_BAD_DESTINATION_ADDRESS:
        case SEC_RESP_BAD_DESTINATION_DEVICE_ID:
        case SEC_RESP_BAD_SIGNATURE:
        case SEC_RESP_BAD_SOURCE_ADDRESS:
        case SEC_RESP_CANNOT_VERIFY_MESSAGE_ID:
        case SEC_RESP_CORRECT_KEY_REVISION:
        case SEC_RESP_DESTINATION_DEVICE_ID_REQUIRED:
        case SEC_RESP_DUPLICATE_MESSAGE:
        case SEC_RESP_ENCRYPTION_NOT_CONFIGURED:
        case SEC_RESP_ENCRYPTION_REQUIRED:
        case SEC_RESP_KEY_UPDATE_IN_PROGRESS:
        case SEC_RESP_MALFORMED_MESSAGE:
        case SEC_RESP_NOT_KEY_SERVER:
        case SEC_RESP_SECURITY_NOT_CONFIGURED:
        case SEC_RESP_SOURCE_SECURITY_REQUIRED:
        case SEC_RESP_UNKNOWN_SOURCE_MESSAGE:
            break;
        default:
            return -1; /* unknown message type */
    }

    return curr;
}

int encode_request_key_update(
    uint8_t *apdu, const BACNET_REQUEST_KEY_UPDATE *req)
{
    int curr = 0;

    apdu[curr++] = req->set_1_key_revision;
    curr += encode_unsigned32(&apdu[curr], req->set_1_activation_time);
    curr += encode_unsigned32(&apdu[curr], req->set_1_expiration_time);
    apdu[curr++] = req->set_2_key_revision;
    curr += encode_unsigned32(&apdu[curr], req->set_2_activation_time);
    curr += encode_unsigned32(&apdu[curr], req->set_2_expiration_time);
    apdu[curr++] = req->distribution_key_revision;

    return curr;
}

int encode_key_entry(uint8_t *apdu, const BACNET_KEY_ENTRY *entry)
{
    int curr = 0;

    curr += encode_unsigned16(&apdu[curr], entry->key_identifier);
    apdu[curr++] = entry->key_len;
    memcpy(&apdu[curr], entry->key, entry->key_len);
    curr += entry->key_len;

    return curr;
}

int encode_update_key_set(uint8_t *apdu, const BACNET_UPDATE_KEY_SET *key_set)
{
    int curr = 0;
    int i, res;
    apdu[curr] = 0;

    if (key_set->remove) {
        apdu[curr] |= 1;
    }
    if (key_set->more) {
        apdu[curr] |= 1 << 1;
    }
    if (key_set->set_clr[1]) {
        apdu[curr] |= 1 << 2;
    }
    if (key_set->set_ck[1]) {
        apdu[curr] |= 1 << 3;
    }
    if (key_set->set_rae[1]) {
        apdu[curr] |= 1 << 4;
    }
    if (key_set->set_clr[0]) {
        apdu[curr] |= 1 << 5;
    }
    if (key_set->set_ck[0]) {
        apdu[curr] |= 1 << 6;
    }
    if (key_set->set_rae[0]) {
        apdu[curr] |= 1 << 7;
    }
    curr++;
    if (key_set->set_rae[0]) {
        apdu[curr++] = key_set->set_key_revision[0];
        curr += encode_unsigned32(&apdu[curr], key_set->set_activation_time[0]);
        curr += encode_unsigned32(&apdu[curr], key_set->set_expiration_time[0]);
    }
    if (key_set->set_ck[0]) {
        apdu[curr++] = key_set->set_key_count[0];
        if (key_set->set_key_count[0] > MAX_UPDATE_KEY_COUNT) {
            return -1;
        }
        for (i = 0; i < (int)key_set->set_key_count[0]; i++) {
            res = encode_key_entry(&apdu[curr], &key_set->set_keys[0][i]);
            if (res < 0) {
                return -1;
            }
            curr += res;
        }
    }
    if (key_set->set_rae[1]) {
        apdu[curr++] = key_set->set_key_revision[1];
        curr += encode_unsigned32(&apdu[curr], key_set->set_activation_time[1]);
        curr += encode_unsigned32(&apdu[curr], key_set->set_expiration_time[1]);
    }
    if (key_set->set_ck[1]) {
        apdu[curr++] = key_set->set_key_count[1];
        if (key_set->set_key_count[1] > MAX_UPDATE_KEY_COUNT) {
            return -1;
        }
        for (i = 0; i < (int)key_set->set_key_count[1]; i++) {
            res = encode_key_entry(&apdu[curr], &key_set->set_keys[1][i]);
            if (res < 0) {
                return -1;
            }
            curr += res;
        }
    }
    return curr;
}

int encode_update_distribution_key(
    uint8_t *apdu, const BACNET_UPDATE_DISTRIBUTION_KEY *dist_key)
{
    int curr = 0;
    int res;

    apdu[curr++] = dist_key->key_revision;
    res = encode_key_entry(&apdu[curr], &dist_key->key);
    if (res < 0) {
        return -1;
    }

    return curr + res;
}

int encode_request_master_key(
    uint8_t *apdu, const BACNET_REQUEST_MASTER_KEY *req_master_key)
{
    int curr = 0;

    apdu[curr++] = req_master_key->no_supported_algorithms;
    memcpy(
        &apdu[curr], req_master_key->es_algorithms,
        req_master_key->no_supported_algorithms);

    return (int)(curr + req_master_key->no_supported_algorithms);
}

int encode_set_master_key(
    uint8_t *apdu, const BACNET_SET_MASTER_KEY *set_master_key)
{
    return encode_key_entry(apdu, &set_master_key->key);
}

#if 0
/* FIXME: please fix? */
int decode_security_wrapper_safe(int bytes_before,
    uint8_t *apdu,
    uint32_t apdu_len_remaining,
    BACNET_SECURITY_WRAPPER *wrapper)
{
    int curr = 0;
    int enc_begin = 0;
    int real_len = (int)(apdu_len_remaining - SIGNATURE_LEN);
    BACNET_KEY_ENTRY key;
    BACNET_SECURITY_RESPONSE_CODE res = SEC_RESP_SUCCESS;

    if (apdu_len_remaining < 40) {
        return -SEC_RESP_MALFORMED_MESSAGE;
    }
    wrapper->payload_net_or_bvll_flag = ((apdu[curr] & (1 << 7)) != 0);
    wrapper->encrypted_flag = ((apdu[curr] & (1 << 6)) != 0);
    wrapper->authentication_flag = ((apdu[curr] & (1 << 4)) != 0);
    wrapper->do_not_unwrap_flag = ((apdu[curr] & (1 << 3)) != 0);
    wrapper->do_not_decrypt_flag = ((apdu[curr] & (1 << 2)) != 0);
    wrapper->non_trusted_source_flag = ((apdu[curr] & (1 << 1)) != 0);
    wrapper->secured_by_router_flag = ((apdu[curr] & 1) != 0);
    /* basic integrity checks */
    if (wrapper->do_not_decrypt_flag && !wrapper->do_not_unwrap_flag) {
        return -SEC_RESP_MALFORMED_MESSAGE;
    }
    if (!wrapper->encrypted_flag && wrapper->do_not_decrypt_flag) {
        return -SEC_RESP_MALFORMED_MESSAGE;
    }
    /* remove encryption flag for signature validation */
    apdu[curr] &= ~((uint8_t)(1 << 6));
    curr++;
    /* key */
    wrapper->key_revision = apdu[curr++];
    curr += decode_unsigned16(&apdu[curr], &wrapper->key_identifier);
    /* find appropriate key */
    key.key_identifier = wrapper->key_identifier;
    res = bacnet_find_key(wrapper->key_revision, &key);
    if (res != SEC_RESP_SUCCESS) {
        return -res;
    }
    /* source device instance */
    curr += decode_unsigned24(&apdu[curr], &wrapper->source_device_instance);
    /* message id */
    curr += decode_unsigned32(&apdu[curr], &wrapper->message_id);
    /* timestamp */
    curr += decode_unsigned32(&apdu[curr], &wrapper->timestamp);
    /* begin decryption starting from destination device instance */
    enc_begin = curr;
    /* read signature */
    memcpy(wrapper->signature, &apdu[real_len], SIGNATURE_LEN);
    if (wrapper->encrypted_flag) {
        if (!key_decrypt_msg(&key, &apdu[enc_begin],
                (uint32_t)(real_len - enc_begin), wrapper->signature)) {
            return -SEC_RESP_MALFORMED_MESSAGE;
        }
        curr += decode_unsigned16(&apdu[real_len - 2],
        &wrapper->padding_len); real_len -= wrapper->padding_len;
        memcpy(wrapper->padding, &apdu[wrapper->padding_len],
            wrapper->padding_len - 2);
    }
    /* destination device instance */
    curr +=
        decode_unsigned24(&apdu[curr],
        &wrapper->destination_device_instance);
    /* dst address */
    curr += decode_unsigned16(&apdu[curr], &wrapper->dnet);
    wrapper->dlen = apdu[curr++];
    memcpy(wrapper->dadr, &apdu[curr], wrapper->dlen);
    curr += wrapper->dlen;
    /* src address */
    curr += decode_unsigned16(&apdu[curr], &wrapper->snet);
    wrapper->slen = apdu[curr++];
    memcpy(wrapper->sadr, &apdu[curr], wrapper->slen);
    curr += wrapper->slen;
    /* authentication */
    if (wrapper->authentication_flag) {
        wrapper->authentication_mechanism = apdu[curr++];
        /* authentication data */
        curr += decode_unsigned16(&apdu[curr], &wrapper->user_id);
        wrapper->user_role = apdu[curr++];
        if ((wrapper->authentication_mechanism >= 1) &&
            (wrapper->authentication_mechanism <= 199)) {
            curr += decode_unsigned16(
                &apdu[curr], &wrapper->authentication_data_length);
            wrapper->authentication_data_length -= 5;
            memcpy(wrapper->authentication_data, &apdu[curr],
                wrapper->authentication_data_length);
            curr += wrapper->authentication_data_length;
        } else if (wrapper->authentication_mechanism >= 200) {
            curr += decode_unsigned16(
                &apdu[curr], &wrapper->authentication_data_length);
            wrapper->authentication_data_length -= 7;
            curr += decode_unsigned16(&apdu[curr], &wrapper->vendor_id);
            memcpy(wrapper->authentication_data, &apdu[curr],
                wrapper->authentication_data_length);
            curr += wrapper->authentication_data_length;
        }
    }
    wrapper->service_data_len = (uint16_t)(real_len - curr);
    memcpy(wrapper->service_data, &apdu[curr], wrapper->service_data_len);
    curr += wrapper->service_data_len;
    if (!key_verify_sign_msg(&key, &apdu[-bytes_before],
            (uint32_t)(bytes_before + real_len), wrapper->signature)) {
        return -SEC_RESP_BAD_SIGNATURE;
    }

    return curr;
}
#endif

int decode_challenge_request_safe(
    const uint8_t *apdu,
    uint32_t apdu_len_remaining,
    BACNET_CHALLENGE_REQUEST *bc_req)
{
    int curr = 0;

    if (apdu_len_remaining < 9) {
        return -1;
    }
    bc_req->message_challenge = apdu[curr++];
    curr += decode_unsigned32(&apdu[curr], &bc_req->orig_message_id);
    curr += decode_unsigned32(&apdu[curr], &bc_req->orig_timestamp);

    return curr; /* always 9! */
}

int decode_security_payload_safe(
    const uint8_t *apdu,
    uint32_t apdu_len_remaining,
    BACNET_SECURITY_PAYLOAD *payload)
{
    if (apdu_len_remaining < 2) {
        return -1;
    }
    decode_unsigned16(&apdu[0], &payload->payload_length);
    if (apdu_len_remaining - 2 < payload->payload_length) {
        return -1;
    }
    memcpy(payload->payload, &apdu[2], payload->payload_length);
    return (int)(2 + payload->payload_length);
}

int decode_security_response_safe(
    const uint8_t *apdu,
    uint32_t apdu_len_remaining,
    BACNET_SECURITY_RESPONSE *resp)
{
    int curr = 0;
    int i;

    if (apdu_len_remaining < 9) {
        return -1;
    }
    resp->response_code = apdu[curr++];
    curr += decode_unsigned32(&apdu[curr], &resp->orig_message_id);
    curr += decode_unsigned32(&apdu[curr], &resp->orig_timestamp);
    switch ((BACNET_SECURITY_RESPONSE_CODE)resp->response_code) {
        case SEC_RESP_BAD_TIMESTAMP:
            if (apdu_len_remaining < 13) {
                return -1;
            }
            curr += decode_unsigned32(
                &apdu[curr], &resp->response.bad_timestamp.expected_timestamp);
            break;
        case SEC_RESP_CANNOT_USE_KEY:
            if (apdu_len_remaining < 11) {
                return -1;
            }
            curr += decode_unsigned16(
                &apdu[curr], &resp->response.cannot_use_key.key);
            break;
        case SEC_RESP_INCORRECT_KEY:
            if (apdu_len_remaining < 10) {
                return -1;
            }
            resp->response.incorrect_key.number_of_keys = apdu[curr++];
            if (apdu_len_remaining - 10 <
                resp->response.incorrect_key.number_of_keys * 2) {
                return -1;
            }
            for (i = 0; i < (int)resp->response.incorrect_key.number_of_keys;
                 i++) {
                curr += decode_unsigned16(
                    &apdu[curr], &resp->response.incorrect_key.keys[i]);
            }
            break;
        case SEC_RESP_UNKNOWN_AUTHENTICATION_TYPE:
            if (apdu_len_remaining < 12) {
                return -1;
            }
            resp->response.unknown_authentication_type
                .original_authentication_type = apdu[curr++];
            curr += decode_unsigned16(
                &apdu[curr],
                &resp->response.unknown_authentication_type.vendor_id);
            if (resp->response.unknown_authentication_type
                        .original_authentication_type < 200 &&
                resp->response.unknown_authentication_type.vendor_id != 0) {
                return -1;
            }
            break;
        case SEC_RESP_UNKNOWN_KEY:
            if (apdu_len_remaining < 11) {
                return -1;
            }
            curr += decode_unsigned16(
                &apdu[curr], &resp->response.unknown_key.original_key);
            break;
        case SEC_RESP_UNKNOWN_KEY_REVISION:
            if (apdu_len_remaining < 10) {
                return -1;
            }
            resp->response.unknown_key_revision.original_key_revision =
                apdu[curr++];
            break;
        case SEC_RESP_TOO_MANY_KEYS:
            if (apdu_len_remaining < 10) {
                return -1;
            }
            resp->response.too_many_keys.max_num_of_keys = apdu[curr++];
            break;
        case SEC_RESP_INVALID_KEY_DATA:
            if (apdu_len_remaining < 11) {
                return -1;
            }
            curr += decode_unsigned16(
                &apdu[curr], &resp->response.invalid_key_data.key);
            break;
        case SEC_RESP_SUCCESS:
        case SEC_RESP_ACCESS_DENIED:
        case SEC_RESP_BAD_DESTINATION_ADDRESS:
        case SEC_RESP_BAD_DESTINATION_DEVICE_ID:
        case SEC_RESP_BAD_SIGNATURE:
        case SEC_RESP_BAD_SOURCE_ADDRESS:
        case SEC_RESP_CANNOT_VERIFY_MESSAGE_ID:
        case SEC_RESP_CORRECT_KEY_REVISION:
        case SEC_RESP_DESTINATION_DEVICE_ID_REQUIRED:
        case SEC_RESP_DUPLICATE_MESSAGE:
        case SEC_RESP_ENCRYPTION_NOT_CONFIGURED:
        case SEC_RESP_ENCRYPTION_REQUIRED:
        case SEC_RESP_KEY_UPDATE_IN_PROGRESS:
        case SEC_RESP_MALFORMED_MESSAGE:
        case SEC_RESP_NOT_KEY_SERVER:
        case SEC_RESP_SECURITY_NOT_CONFIGURED:
        case SEC_RESP_SOURCE_SECURITY_REQUIRED:
        case SEC_RESP_UNKNOWN_SOURCE_MESSAGE:
            break;
        default:
            return -1; /* unknown message type */
    }
    return curr;
}

int decode_request_key_update_safe(
    const uint8_t *apdu,
    uint32_t apdu_len_remaining,
    BACNET_REQUEST_KEY_UPDATE *req)
{
    int curr = 0;

    if (apdu_len_remaining < 19) {
        return -1;
    }
    req->set_1_key_revision = apdu[curr++];
    curr += decode_unsigned32(&apdu[curr], &req->set_1_activation_time);
    curr += decode_unsigned32(&apdu[curr], &req->set_1_expiration_time);
    req->set_2_key_revision = apdu[curr++];
    curr += decode_unsigned32(&apdu[curr], &req->set_2_activation_time);
    curr += decode_unsigned32(&apdu[curr], &req->set_2_expiration_time);
    req->distribution_key_revision = apdu[curr++];

    return curr;
}

int decode_key_entry_safe(
    const uint8_t *apdu, uint32_t apdu_len_remaining, BACNET_KEY_ENTRY *entry)
{
    int curr = 0;

    if (apdu_len_remaining < 3) {
        return -1;
    }
    curr += decode_unsigned16(&apdu[curr], &entry->key_identifier);
    entry->key_len = apdu[curr++];
    if (apdu_len_remaining - 3 < entry->key_len ||
        entry->key_len > MAX_KEY_LEN) {
        return -1;
    }
    memcpy(entry->key, &apdu[curr], entry->key_len);
    curr += entry->key_len;

    return curr;
}

int decode_update_key_set_safe(
    const uint8_t *apdu,
    uint32_t apdu_len_remaining,
    BACNET_UPDATE_KEY_SET *key_set)
{
    int curr = 0;
    int i, res;

    if (apdu_len_remaining < 1) {
        return -1;
    }
    if (apdu[curr] & 1) {
        key_set->remove = true;
    }
    if ((apdu[curr] >> 1) & 1) {
        key_set->more = true;
    }
    if ((apdu[curr] >> 2) & 1) {
        key_set->set_clr[1] = true;
    }
    if ((apdu[curr] >> 3) & 1) {
        key_set->set_ck[1] = true;
    }
    if ((apdu[curr] >> 4) & 1) {
        key_set->set_rae[1] = true;
    }
    if ((apdu[curr] >> 5) & 1) {
        key_set->set_clr[0] = true;
    }
    if ((apdu[curr] >> 6) & 1) {
        key_set->set_ck[0] = true;
    }
    if ((apdu[curr] >> 7) & 1) {
        key_set->set_rae[0] = true;
    }
    curr++;
    if (key_set->set_rae[0]) {
        if (apdu_len_remaining - curr < 9) {
            return -1;
        }
        key_set->set_key_revision[0] = apdu[curr++];
        curr +=
            decode_unsigned32(&apdu[curr], &key_set->set_activation_time[0]);
        curr +=
            decode_unsigned32(&apdu[curr], &key_set->set_expiration_time[0]);
    }
    if (key_set->set_ck[0]) {
        if (apdu_len_remaining - curr < 1) {
            return -1;
        }
        key_set->set_key_count[0] = apdu[curr++];
        if (key_set->set_key_count[0] > MAX_UPDATE_KEY_COUNT) {
            return -1;
        }
        for (i = 0; i < (int)key_set->set_key_count[0]; i++) {
            res = decode_key_entry_safe(
                apdu + curr, apdu_len_remaining - curr,
                &key_set->set_keys[0][i]);
            if (res < 0) {
                return -1;
            }
            curr += res;
        }
    }
    if (key_set->set_rae[1]) {
        if (apdu_len_remaining - curr < 9) {
            return -1;
        }
        key_set->set_key_revision[1] = apdu[curr++];
        curr +=
            decode_unsigned32(&apdu[curr], &key_set->set_activation_time[1]);
        curr +=
            decode_unsigned32(&apdu[curr], &key_set->set_expiration_time[1]);
    }
    if (key_set->set_ck[1]) {
        if (apdu_len_remaining - curr < 1) {
            return -1;
        }
        key_set->set_key_count[1] = apdu[curr++];
        if (key_set->set_key_count[1] > MAX_UPDATE_KEY_COUNT) {
            return -1;
        }
        for (i = 0; i < (int)key_set->set_key_count[1]; i++) {
            res = decode_key_entry_safe(
                apdu + curr, apdu_len_remaining - curr,
                &key_set->set_keys[1][i]);
            if (res < 0) {
                return -1;
            }
            curr += res;
        }
    }

    return curr;
}

int decode_update_distribution_key_safe(
    const uint8_t *apdu,
    uint32_t apdu_len_remaining,
    BACNET_UPDATE_DISTRIBUTION_KEY *dist_key)
{
    int curr = 0;
    int res;
    if (apdu_len_remaining < 1) {
        return -1;
    }
    dist_key->key_revision = apdu[curr++];
    res = decode_key_entry_safe(
        &apdu[curr], apdu_len_remaining - curr, &dist_key->key);
    if (res < 0) {
        return -1;
    }

    return curr + res;
}

int decode_request_master_key_safe(
    const uint8_t *apdu,
    uint32_t apdu_len_remaining,
    BACNET_REQUEST_MASTER_KEY *req_master_key)
{
    uint32_t curr = 0;

    if (apdu_len_remaining < 1) {
        return -1;
    }
    req_master_key->no_supported_algorithms = apdu[curr++];
    if (apdu_len_remaining < curr + req_master_key->no_supported_algorithms) {
        return -1;
    }
    memcpy(
        req_master_key->es_algorithms, &apdu[curr],
        req_master_key->no_supported_algorithms);

    return (int)(curr + req_master_key->no_supported_algorithms);
}

int decode_set_master_key_safe(
    const uint8_t *apdu,
    uint32_t apdu_len_remaining,
    BACNET_SET_MASTER_KEY *set_master_key)
{
    return decode_key_entry_safe(
        apdu, apdu_len_remaining, &set_master_key->key);
}
