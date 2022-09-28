/**
 * @file
 * @brief Implementation of websocket client interface for Zephyr
 * @author Mikhail Antropov
 * @date Sep 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */


#ifndef BSC_HTTP_CLIENT_ASYNC_H_
#define BSC_HTTP_CLIENT_ASYNC_H_

/**
 * @brief processing acync HTTP responce 
 * @defgroup http_client HTTP client API
 * @ingroup networking
 * @{
 */

#include <kernel.h>
#include <net/net_ip.h>
#include <net/http_parser.h>

#ifdef __cplusplus
extern "C" {
#endif

struct http_request;

/**
 * @brief Handles an HTTP response. Used if http_client_req() returns 0 and the
 * errno is equal EAGAIN. The function requires the following fields to be
 * filled in for req values: http_cb, response, recv_buf, recv_buf_len, as in
 * the previous http_client_req() call. The value of other fields does not
 * matter.
 * @param sock Socket id of the connection.
 * @param req Mock of HTTP request information.
 * @param user_data User specified data that is passed to the callback.
 *
 * @return <0 if error, >=0 amount of data sent to the server
 */
int http_wait_data_asyn(int sock, struct http_request *req, void *user_data);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BSC_HTTP_CLIENT_ASYNC_H_ */
