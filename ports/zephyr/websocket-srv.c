/**
 * @file
 * @brief Implementation of server websocket interface MAC OS / BSD.
 * @author Kirill Neznamov
 * @date June 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "bacnet/datalink/bsc/websocket.h"

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

BSC_WEBSOCKET_RET bws_srv_start(
                        BSC_WEBSOCKET_PROTOCOL proto,
                        int port,
                        char* iface,
                        uint8_t *ca_cert,
                        size_t ca_cert_size,
                        uint8_t *cert,
                        size_t cert_size,
                        uint8_t *key,
                        size_t key_size,
                        size_t timeout_s,
                        BSC_WEBSOCKET_SRV_DISPATCH dispatch_func,
                        void* dispatch_func_user_param,
                        BSC_WEBSOCKET_SRV_HANDLE* sh)
{
    LOG_INF("bws_srv_start() >>> proto = %d port = %d", proto, port);
    return BSC_WEBSOCKET_NO_RESOURCES;
}

BSC_WEBSOCKET_RET bws_srv_stop(BSC_WEBSOCKET_SRV_HANDLE sh)
{
    LOG_INF("bws_srv_stop() >>> handle = %p", sh);
    return BSC_WEBSOCKET_NO_RESOURCES;
}

void bws_srv_disconnect(BSC_WEBSOCKET_SRV_HANDLE sh, BSC_WEBSOCKET_HANDLE h)
{
    LOG_INF("bws_srv_disconnect() >>> handle = %p h = %d", sh, h);
}

void bws_srv_send(BSC_WEBSOCKET_SRV_HANDLE sh, BSC_WEBSOCKET_HANDLE h)
{
    LOG_INF("bws_srv_send() >>> handle = %p h = %d", sh, h);
}

BSC_WEBSOCKET_RET bws_srv_dispatch_send(BSC_WEBSOCKET_SRV_HANDLE sh,
                                        BSC_WEBSOCKET_HANDLE h,
                                        uint8_t *payload, size_t payload_size)
{
    LOG_INF(
        "bws_srv_dispatch_send() >>> sh %p, h = %d, payload = %p, size = %d",
        sh, h, payload, payload_size);
    return BSC_WEBSOCKET_NO_RESOURCES;
}
