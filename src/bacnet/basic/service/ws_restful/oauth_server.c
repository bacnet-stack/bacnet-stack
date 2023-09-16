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
    return true;
}

bool oauth_enable(void)
{
    return true;
}

bool oauth_disable(void)
{
    return true;
}

char *oauth_pri_uri(void)
{
    return NULL;
}

int oauth_pri_uri_set(char *uri)
{
    (void)uri;
    return 0;
}

int oauth_pri_cert(uint8_t **cert, size_t *size)
{
    (void)cert;
    *size = 0;
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
    (void)key;
    *size = 0;
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
    return NULL;
}

int oauth_sec_uri_set(char *uri)
{
    (void)uri;
    return 0;
}

int oauth_sec_cert(uint8_t **cert, size_t *size)
{
    (void)cert;
    *size = 0;
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
    (void)key;
    *size = 0;
    return 0;
}

int oauth_sec_pubkey_set(uint8_t *key, size_t size)
{
    (void)key;
    (void)size;
    return 0;
}
