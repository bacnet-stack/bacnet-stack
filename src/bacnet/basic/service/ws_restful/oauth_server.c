/**
 * @file
 * @brief Mock of OAuth server.
 * @author Mikhail Antropov
 * @date Sep 2023
 * @section LICENSE
 *
 * Copyright (C) 2023 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <bacnet/basic/service/ws_restful/oauth_server.h>

typedef struct
{
    char *uri; /* null terminated string without leading and trailing '/' */
    uint8_t *cert;
    size_t certSize;
    uint8_t *key;
    size_t keySize;
} BACNET_TRUST_SERVER;

BACNET_TRUST_SERVER primary_server = {0};
BACNET_TRUST_SERVER secondary_server = {0};
bool m_enable = true;

void oauth_factory_default_set(void)
{

}

int oauth_token_get(char *user, char *password, uint8_t *token, size_t max_size)
{
    (void)user;
    (void)password;
    (void)token;
    (void)max_size;
    return 0;
}

bool oauth_token_check(uint8_t *token, size_t token_size)
{
    (void)token;
    (void)token_size;
    return true;
}

int oauth_user_set(char *user)
{
    (void)user;
    return 0;
}

int oauth_pass_set(char *pass)
{
    (void)pass;
    return 0;
}

int oauth_id_set(char *id)
{
    (void)id;
    return 0;
}

int oauth_secret_set(char *secret)
{
    (void)secret;
    return 0;
}

bool oauth_is_enable(void)
{
    return m_enable;
}

bool oauth_enable(void)
{
    m_enable = true;
    return true;
}

bool oauth_disable(void)
{
    m_enable = false;
    return true;
}

void oauth_server_pri_init(
    char *uri, uint8_t *cert, size_t certSize, uint8_t *key, size_t keySize)
{
    primary_server.uri = uri;
    primary_server.cert = cert;
    primary_server.certSize = certSize;
    primary_server.key = key;
    primary_server.keySize = keySize;
}

void oauth_server_sec_init(
    char *uri, uint8_t *cert, size_t certSize, uint8_t *key, size_t keySize)
{
    secondary_server.uri = uri;
    secondary_server.cert = cert;
    secondary_server.certSize = certSize;
    secondary_server.key = key;
    secondary_server.keySize = keySize;
}

char *oauth_pri_uri(void)
{
    return primary_server.uri;
}

int oauth_pri_uri_set(char *uri)
{
    (void)uri;
    return 0;
}

int oauth_pri_cert(uint8_t **cert, size_t *size)
{
    *cert = primary_server.cert ? primary_server.cert : NULL;
    *size = primary_server.certSize;
    return 0;
}

int oauth_pri_cert_set(uint8_t *cert, size_t size)
{
    (void)cert;
    (void)size;
    return 0;
}

int oauth_pri_pubkey(uint8_t **key, size_t *size)
{
    *key = primary_server.key ? primary_server.key : NULL;
    *size = primary_server.keySize;
    return 0;
}

int oauth_pri_pubkey_set(uint8_t *key, size_t size)
{
    (void)key;
    (void)size;
    return 0;
}

char *oauth_sec_uri(void)
{
    return secondary_server.uri;
}

int oauth_sec_uri_set(char *uri)
{
    (void)uri;
    return 0;
}

int oauth_sec_cert(uint8_t **cert, size_t *size)
{
    *cert = secondary_server.cert ? secondary_server.cert : NULL;
    *size = secondary_server.certSize;
    return 0;
}

int oauth_sec_cert_set(uint8_t *cert, size_t size)
{
    (void)cert;
    (void)size;
    return 0;
}

int oauth_sec_pubkey(uint8_t **key, size_t *size)
{
    *key = secondary_server.key ? secondary_server.key : NULL;
    *size = secondary_server.keySize;
    return 0;
}

int oauth_sec_pubkey_set(uint8_t *key, size_t size)
{
    (void)key;
    (void)size;
    return 0;
}
