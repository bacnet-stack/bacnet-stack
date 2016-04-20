/**************************************************************************
*
* Copyright (C) 2015 Nikola Jelic <nikola.jelic@euroicc.com>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/
#include "bacsec.h"
#include <string.h>
#include <time.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#define TMP_BUF_LEN 2048

/* structures for keys - we use existing structures */

BACNET_KEY_ENTRY master_key;

BACNET_UPDATE_DISTRIBUTION_KEY distribution_key;

BACNET_UPDATE_KEY_SET key_sets;

static bool rand_set = false;
static HMAC_CTX hmac_ctx;
static EVP_CIPHER_CTX evp_ctx;
static uint8_t tmp_buf[TMP_BUF_LEN];

static uint16_t next_mult_of_16(uint16_t arg)
{
    if ((arg & 0xF) == 0)
        return arg;
    else
        return ((arg >> 4) + 1) << 4;
}

/* signing/verification and encryption/decryption - port specific */
int key_sign_msg(BACNET_KEY_ENTRY * key,
    uint8_t * msg,
    uint32_t msg_len,
    uint8_t * signature)
{
    uint8_t full_signature[32]; /* longest case */
    HMAC_CTX_init(&hmac_ctx);
    switch (key_algorithm(key->key_identifier)) {
        case KIA_AES_MD5:
            HMAC_Init_ex(&hmac_ctx, &key->key[16], 16, EVP_md5(), NULL);
            break;
        case KIA_AES_SHA256:
            HMAC_Init_ex(&hmac_ctx, &key->key[16], 32, EVP_sha256(), NULL);
            break;
        default:
            return -1;
    }
    HMAC_Update(&hmac_ctx, msg, msg_len);
    HMAC_Final(&hmac_ctx, full_signature, NULL);        /* we ignore the signature size */
    HMAC_CTX_cleanup(&hmac_ctx);
    memcpy(signature, full_signature, SIGNATURE_LEN);
    return 0;
}

bool key_verify_sign_msg(BACNET_KEY_ENTRY * key,
    uint8_t * msg,
    uint32_t msg_len,
    uint8_t * signature)
{
    uint8_t full_signature[32]; /* longest case */
    HMAC_CTX_init(&hmac_ctx);
    switch (key_algorithm(key->key_identifier)) {
        case KIA_AES_MD5:
            HMAC_Init_ex(&hmac_ctx, &key->key[16], 16, EVP_md5(), NULL);
            break;
        case KIA_AES_SHA256:
            HMAC_Init_ex(&hmac_ctx, &key->key[16], 32, EVP_sha256(), NULL);
            break;
        default:
            return false;
    }
    HMAC_Update(&hmac_ctx, msg, msg_len);
    HMAC_Final(&hmac_ctx, full_signature, NULL);        /* we ignore the signature size */
    HMAC_CTX_cleanup(&hmac_ctx);
    return (memcmp(signature, full_signature,
            SIGNATURE_LEN) == 0 ? true : false);
}

int key_encrypt_msg(BACNET_KEY_ENTRY * key,
    uint8_t * msg,
    uint32_t msg_len,
    uint8_t * signature)
{
    int outlen, outlen2;
    switch (key_algorithm(key->key_identifier)) {
        case KIA_AES_MD5:
        case KIA_AES_SHA256:
            EVP_EncryptInit_ex(&evp_ctx, EVP_aes_128_cbc(), NULL, key->key,
                signature);
            break;
        default:
            return -1;
    }
    EVP_EncryptUpdate(&evp_ctx, tmp_buf, &outlen, msg, msg_len);
    EVP_EncryptFinal(&evp_ctx, &msg[outlen], &outlen2);
    EVP_CIPHER_CTX_cleanup(&evp_ctx);
    if (outlen2 != 0)
        return -1;
    memcpy(msg, tmp_buf, msg_len);
    return 0;
}

bool key_decrypt_msg(BACNET_KEY_ENTRY * key,
    uint8_t * msg,
    uint32_t msg_len,
    uint8_t * signature)
{
    int outlen, outlen2;
    switch (key_algorithm(key->key_identifier)) {
        case KIA_AES_MD5:
        case KIA_AES_SHA256:
            EVP_DecryptInit_ex(&evp_ctx, EVP_aes_128_cbc(), NULL, key->key,
                signature);
            break;
        default:
            return false;
    }
    if (EVP_DecryptUpdate(&evp_ctx, tmp_buf, &outlen, msg, msg_len) == 0)
        return false;
    if (EVP_DecryptFinal(&evp_ctx, &msg[outlen], &outlen2) == 0)
        return false;
    EVP_CIPHER_CTX_cleanup(&evp_ctx);
    if (outlen2 != 0)
        return false;
    memcpy(msg, tmp_buf, msg_len);
    return true;
}

void key_set_padding(BACNET_KEY_ENTRY * key,
    int enc_len,
    uint16_t * padding_len,
    uint8_t * padding)
{
    /* in the future, we should check for the block size, but for now it is always 16 */
    int i;
    uint16_t padlen = next_mult_of_16(enc_len + 2);
    (void) key;
    (void) padding_len;
    if (!rand_set) {
        srand(time(NULL));
        rand_set = true;
    }
    if (padlen > 2)
        for (i = 0; i < padlen - 2; i++)
            padding[i] = rand();
}

BACNET_SECURITY_RESPONSE_CODE bacnet_master_key_set(BACNET_SET_MASTER_KEY *
    key)
{
    memcpy(&master_key, &key->key, sizeof(BACNET_KEY_ENTRY));

    return SEC_RESP_SUCCESS;
}

BACNET_SECURITY_RESPONSE_CODE
bacnet_distribution_key_update(BACNET_UPDATE_DISTRIBUTION_KEY * key)
{
    memcpy(&distribution_key, key, sizeof(BACNET_KEY_ENTRY));

    return SEC_RESP_SUCCESS;
}

BACNET_SECURITY_RESPONSE_CODE bacnet_key_set_update(BACNET_UPDATE_KEY_SET *
    update_key_sets)
{
    int i, j, k, l;
    bool found;
    for (i = 0; i < 2; i++) {
        if (update_key_sets->set_rae[i]) {
            found = false;
            /* try with a valid set */
            for (j = 0; j < 2; j++)
                if ((key_sets.set_key_revision[j] ==
                        update_key_sets->set_key_revision[i]) &&
                    key_sets.set_rae[j]) {
                    found = true;
                    break;
                }
            /* try with an empty set */
            if (!found) {
                for (j = 0; j < 2; j++)
                    if (!key_sets.set_rae[j]) {
                        found = true;
                        break;
                    }
            }
            /* failure */
            if (!found)
                return -SEC_RESP_UNKNOWN_KEY_REVISION;
            /* just in case we're writing over an empty set */
            key_sets.set_key_revision[j] =
                update_key_sets->set_key_revision[i];
            /* update revision activation and expiration time */
            key_sets.set_activation_time[j] =
                update_key_sets->set_activation_time[i];
            key_sets.set_expiration_time[j] =
                update_key_sets->set_expiration_time[i];
            /* should we clear the key set? */
            if (update_key_sets->set_clr[i]) {
                key_sets.set_key_count[j] = 0;
            }
            for (k = 0; k < update_key_sets->set_key_count[i]; k++) {
                found = false;
                for (l = 0; l < key_sets.set_key_count[j]; l++)
                    if (update_key_sets->set_keys[i][k].key_identifier ==
                        key_sets.set_keys[j][l].key_identifier) {
                        found = true;
                        break;
                    }

                if (!found) {
                    if (!update_key_sets->remove) {     /* add key */
                        /* check for available space */
                        if (key_sets.set_key_count[j] == MAX_UPDATE_KEY_COUNT)
                            return -SEC_RESP_TOO_MANY_KEYS;
                        memcpy(&key_sets.set_keys[j][key_sets.
                                set_key_count[j]],
                            &update_key_sets->set_keys[i][k],
                            sizeof(BACNET_KEY_ENTRY));
                        key_sets.set_key_count[j]++;
                    }   /* else do nothing, successfuly */
                } else {
                    if (!update_key_sets->remove) {     /* update key */
                        memcpy(&key_sets.set_keys[j][l],
                            &update_key_sets->set_keys[i][k],
                            sizeof(BACNET_KEY_ENTRY));
                    } else {    /* remove key */
                        memmove(&key_sets.set_keys[j][l],
                            &key_sets.set_keys[j][l + 1],
                            sizeof(BACNET_KEY_ENTRY) *
                            (key_sets.set_key_count[j] - l));
                        key_sets.set_key_count[j]--;
                    }
                }
            }
        }

    }
    return SEC_RESP_SUCCESS;
}

BACNET_SECURITY_RESPONSE_CODE bacnet_find_key(uint8_t revision,
    BACNET_KEY_ENTRY * key)
{
    int i, j;
    unsigned int current_time = time(NULL);
    switch (key_number(key->key_identifier)) {
        case KIKN_DEVICE_MASTER:
            if (revision != 0)
                return -SEC_RESP_UNKNOWN_KEY_REVISION;
            else
                memcpy(key, &master_key, sizeof(BACNET_KEY_ENTRY));
            break;
        case KIKN_DISTRIBUTION:
            if (revision != distribution_key.key_revision)
                return -SEC_RESP_UNKNOWN_KEY_REVISION;
            else
                memcpy(key, &distribution_key.key, sizeof(BACNET_KEY_ENTRY));
            break;
        default:       /* all other keys must be in a key set */
            for (i = 0; i < 2; i++) {
                if ((revision == key_sets.set_key_revision[i]) &&
                    (key_sets.set_activation_time[i] <= current_time) &&
                    (current_time <= key_sets.set_expiration_time[i])) {
                    for (j = 0; j < key_sets.set_key_count[i]; j++)
                        if (key->key_identifier ==
                            key_sets.set_keys[i][j].key_identifier) {
                            memcpy(key,
                                &key_sets.set_keys[i][j].key_identifier,
                                sizeof(BACNET_KEY_ENTRY));
                            return SEC_RESP_SUCCESS;
                        }
                }
            }
            return -SEC_RESP_UNKNOWN_KEY_REVISION;
    }
    return SEC_RESP_SUCCESS;
}
