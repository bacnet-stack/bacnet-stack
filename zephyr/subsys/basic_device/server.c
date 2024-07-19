/**
 * @file
 * @brief BACnet Stack server initialization and task handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date March 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdlib.h>
#include <stdalign.h> /*TODO: Not std until C11! */
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>
#include <bacnet_settings/bacnet_storage.h>
#include "bacnet/datalink/datalink.h"
#include "basic_device/bacnet-port.h"
#include "basic_device/bacnet.h"
#if defined(CONFIG_BACNETSTACK_BACNET_SETTINGS)
#include "bacnet_settings/bacnet_storage.h"
#endif

/* note: stack is minimally 2x to 3x of MAX_APDU */
#ifndef CONFIG_BACNETSTACK_BACNET_SERVER_STACK_SIZE
#define CONFIG_BACNETSTACK_BACNET_SERVER_STACK_SIZE 4096
#endif

#ifndef CONFIG_BACNETSTACK_BACNET_SERVER_PRIO
#define CONFIG_BACNETSTACK_BACNET_SERVER_PRIO 10
#endif

#ifndef CONFIG_BACNETSTACK_BACNET_SERVER_APP_PRIORITY
#define CONFIG_BACNETSTACK_BACNET_SERVER_APP_PRIORITY 90
#endif

#ifndef CONFIG_BACNETSTACK_LOG_LEVEL
#define CONFIG_BACNETSTACK_LOG_LEVEL LOG_LEVEL_INF
#endif

/* Logging module registration is already done in ports/zephyr/main.c */
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

static struct k_thread server_thread_data;
static K_THREAD_STACK_DEFINE(server_thread_stack,
                             CONFIG_BACNETSTACK_BACNET_SERVER_STACK_SIZE);

/**
 * @brief BACnet Server Thread
 */
static void server_thread(void)
{
    LOG_INF("BACnet Server: started");

#if defined(CONFIG_BACNETSTACK_BACNET_SETTINGS)
    bacnet_storage_init();
#endif
    bacnet_init();
    for (;;) {
        if (!bacnet_port_init()) {
            LOG_ERR("BACnet Server: port initialization failed");
            k_sleep(K_MSEC(1000));
        }
    }
    LOG_INF("BACnet Server: initialized");
    for (;;) {
        k_sleep(K_MSEC(10));
        bacnet_task();
        bacnet_port_task();
    }
}

/**
 * @brief BACnet Server Thread initialization
 */
static int server_init(void)
{
    k_thread_create(&server_thread_data, server_thread_stack,
                    K_THREAD_STACK_SIZEOF(server_thread_stack),
                    (k_thread_entry_t)server_thread, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(CONFIG_BACNETSTACK_BACNET_SERVER_PRIO), 0,
                    K_NO_WAIT);
    k_thread_name_set(&server_thread_data, "bacnet_server");

    return 0;
}

SYS_INIT(server_init, APPLICATION,
         CONFIG_BACNETSTACK_BACNET_SERVER_APP_PRIORITY);
