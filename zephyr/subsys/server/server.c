/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdalign.h> /*TODO: Not std until C11! */
#include <device.h>
#include <init.h>
#include <kernel.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include <net/net_ip.h>
#include <sys/printk.h>
/* some BACnet modules we use */
#include "bacnet/bacdef.h"
#include "bacnet/config.h"
#include "bacnet/bactext.h"
#include "bacnet/bacerror.h"
#include "bacnet/iam.h"
#include "bacnet/arf.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/version.h"
/* some demo modules we use */
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/services.h"
/* our datalink layers */
#include "bacnet/datalink/bip.h"
#include "bacnet/datalink/bvlc.h"
/* include the device object */
#include "bacnet/basic/object/device.h"

/* Logging module registration is already done in ports/zephyr/main.c */
#include <logging/log.h>
LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

enum bacnet_server_msg_type {
    SERVER_MSG_TYPE_INVALID = 0,
    SERVER_MSG_TYPE_IPV4_EVENT,
};

struct bacnet_server_msg {
    uint8_t type;
    uint8_t dummy[3];
    uint32_t parm_u32;
    void *parm_ptr;
};

K_MSGQ_DEFINE(bacnet_server_msgq, sizeof(struct bacnet_server_msg), 8,
          alignof(struct bacnet_server_msg));

#define SERVER_IPV4_EVENTS_MASK                                                \
    (NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL)

static struct k_thread server_thread_data;
static K_THREAD_STACK_DEFINE(server_thread_stack,
                 CONFIG_BACNETSTACK_BACNET_SERVER_STACK_SIZE);

/* Keep a reference to the Ethernet interface */
static struct net_mgmt_event_callback mgmt_cb;

/* track our directly connected ports network number */
static uint16_t BIP_Net;
/* buffer for receiving packets from the directly connected ports */
static uint8_t BIP_Rx_Buffer[MAX_MPDU];

/** Initialize the handlers we will utilize.
 * @see Device_Init, apdu_set_unconfirmed_handler, apdu_set_confirmed_handler
 */
static void service_handlers_init(void)
{
    Device_Init(NULL);
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS,
                     handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS,
                     handler_who_has);
    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(
        handler_unrecognized_service);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
                   handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE,
                   handler_read_property_multiple);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY,
                   handler_write_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE,
                   handler_write_property_multiple);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
                   handler_reinitialize_device);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
}

/* TODO: Update copyright as this code pattern copied from
 *       conn_mgr_ipv4_events_handler()
 */
static void ipv4_events_handler(struct net_mgmt_event_callback *cb,
                u32_t mgmt_event, struct net_if *iface)
{
    static int counter = 0;
    printk("\n*** Handler[%d]: IPv4 event %08x received on iface %p ***\n",
           ++counter, mgmt_event, iface);

    if ((mgmt_event & SERVER_IPV4_EVENTS_MASK) != mgmt_event) {
        printk("\n*** Handler[%d]: ignoring event %08x on iface %p ***\n",
               counter, mgmt_event, iface);
        return;
    }

    struct bacnet_server_msg msg = {
        .type = SERVER_MSG_TYPE_IPV4_EVENT,
        .parm_u32 = mgmt_event,
        .parm_ptr = iface,
    };

    int ret = k_msgq_put(&bacnet_server_msgq, &msg, K_NO_WAIT);
    if (ret != 0) {
        printk("\n*** Handler[%d]: queue full, type %u event 0x%08x on iface %p dropped! ***\n",
               counter, msg.type, msg.parm_u32, msg.parm_ptr);
    }
}

/**
 * @brief BACnet Server Thread
 */
static void server_thread(void)
{
    LOG_INF("Server: started");
    service_handlers_init();

    bip_init("Server: from init");
    BIP_Net = 1;

    net_mgmt_init_event_callback(&mgmt_cb, ipv4_events_handler,
                     SERVER_IPV4_EVENTS_MASK);
    net_mgmt_add_event_callback(&mgmt_cb);

    while (1) {
        const s32_t sleep_ms = K_FOREVER;

        struct bacnet_server_msg msg = {
            .type = SERVER_MSG_TYPE_INVALID,
        };
        int ret = k_msgq_get(&bacnet_server_msgq, &msg, sleep_ms);

        /* Waiting period timed out */
        if (-EAGAIN == ret) {
            BACNET_ADDRESS src = {
                0
            }; /* address where message came from */
            /* input */
            /* returns 0 bytes on timeout */
            uint16_t pdu_len = bip_receive(&src, &BIP_Rx_Buffer[0],
                               MAX_MPDU, 5);

            /* process */
            if (pdu_len) {
                LOG_INF("Server: BIP received %u bytes.",
                    (unsigned)pdu_len);
            }
        }

        /* Message received */
        else if (0 == ret) {
            switch (msg.type) {
#if defined(CONFIG_NET_IPV4)
            case SERVER_MSG_TYPE_IPV4_EVENT: {
                LOG_INF("Server: MSG_TYPE_IPV4_EVENT u32: %08x ptr: %p",
                    msg.parm_u32, msg.parm_ptr);
                const u32_t mgmt_event = msg.parm_u32;
                //TODO: const struct net_if *iface = msg.parm_ptr;

                /* Handle events */
                if ((mgmt_event & SERVER_IPV4_EVENTS_MASK) !=
                    mgmt_event) {
                    LOG_INF("Server: thread ignoring event");
                    break;
                }

                switch (NET_MGMT_GET_COMMAND(mgmt_event)) {
#if 0
                case NET_EVENT_IPV4_CMD_ADDR_ADD:
                    LOG_INF("Server: IPv4 addr activated");
                    bip_init("Server: from CMD_ADDR_ADD");
                    BIP_Net = bip_valid() ? 1 : 0;
                    break;
                case NET_EVENT_IPV4_CMD_ADDR_DEL:
                    LOG_INF("Server: IPv4 addr deactivated");
                    bip_cleanup();
                    BIP_Net = 0;
                    break;
#endif
                default:
                    LOG_INF("Server: Unsupported event %u",
                        mgmt_event);
                    break;
                }

            } break;
#endif /* defined(CONFIG_NET_IPV4) */

            default:
                LOG_WRN("Server: Dropping unsupported type %u",
                    msg.type);
                break;
            }
        }

        /* Returned without waiting and without message - why? */
        else {
            LOG_WRN("Server: Msgq returned w/o timeout or msg! req = %d",
                ret);
        }
    }
}

static int server_init(struct device *dev)
{
    ARG_UNUSED(dev);

    k_thread_create(&server_thread_data, server_thread_stack,
            K_THREAD_STACK_SIZEOF(server_thread_stack),
            (k_thread_entry_t)server_thread, NULL, NULL, NULL,
            K_PRIO_PREEMPT(CONFIG_BACNETSTACK_BACNET_SERVER_PRIO), 0,
            K_NO_WAIT);
    k_thread_name_set(&server_thread_data, "BACserver");

    return 0;
}

SYS_INIT(server_init, APPLICATION, CONFIG_BACNETSTACK_BACNET_SERVER_APP_PRIORITY);
