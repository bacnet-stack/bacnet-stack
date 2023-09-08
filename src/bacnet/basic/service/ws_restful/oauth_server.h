/**
 * @file
 * @brief OAuth 2.0 and the internal authorization server API.
 * @author Mikhail Antropov
 * @date August 2023
 * @section LICENSE
 *
 * Copyright (C) 2023 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __OAUTH__SRV__INCLUDED__
#define __OAUTH__SRV__INCLUDED__

#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"

/**
 * @brief Apply a Facroty Default state (see W.3.3.1)
 */
void oauth_factory_default_set(void);

/**
 * @brief Retreive a token by pair user/password or error
 *
 * @param user [in] User name
 * @param password [in] Password
 * @param token [out] Buffer for token
 * @param max_size [in] Buffer size
 * @return Number of bytes in the token, negative number on failure.
 */
int oauth_token_get(char *user, char *password, uint8_t *token, size_t max_size);

/**
 * @brief Validate token
 *
 * @param token [in] token
 * @param token_size [in] Token size
 * @return true if the token is valid.
 */
bool oauth_token_check(uint8_t *token, size_t token_size);

/**
 * @brief Set user name
 *
 * @param user [in] User name
 * @return 0 if sucessful, negative number on failure.
 */
int oauth_user_set(char *user);

/**
 * @brief Set user password
 *
 * @param pass [in] Password
 * @return 0 if sucessful, negative number on failure.
 */
int oauth_pass_set(char *pass);

/**
 * @brief Set user ID
 *
 * @param id [in] ID
 * @return 0 if sucessful, negative number on failure.
 */
int oauth_id_set(char *id);

/**
 * @brief Set user secret
 *
 * @param secret [in] secret
 * @return 0 if sucessful, negative number on failure.
 */
int oauth_secret_set(char *secret);

/**
 * @brief Set user secret
 *
 * @param secret [in] secret
 * @return 0 if sucessful, negative number on failure.
 */
bool oauth_is_enable(void);
bool oauth_enable(void);
bool oauth_disable(void);

char *oauth_pri_uri(void);
int oauth_pri_uri_set(char *uri);

int oauth_pri_cert(uint8_t **cert, size_t *size);
int oauth_pri_cert_set(uint8_t *cert, size_t size);
// todo 



#endif