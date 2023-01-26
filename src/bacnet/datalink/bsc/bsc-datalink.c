/**
 * @file
 * @brief BACnet secure connect hub function API.
 * @author Kirill Neznamov
 * @date October 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#include "bacnet/basic/sys/debug.h"
#include <bacnet/basic/sys/fifo.h>
#include "bacnet/datalink/bsc/bsc-conf.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/datalink/bsc/bsc-datalink.h"
#include "bacnet/datalink/bsc/bsc-socket.h"
#include "bacnet/datalink/bsc/bsc-util.h"
#include "bacnet/datalink/bsc/bsc-event.h"
#include "bacnet/datalink/bsc/bsc-hub-function.h"
#include "bacnet/datalink/bsc/bsc-node.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacenum.h"
#include "bacnet/basic/object/netport.h"
#include "bacnet/basic/object/sc_netport.h"
#include "bacnet/basic/object/bacfile.h"

#define DEBUG_BSC_DATALINK 0

#if DEBUG_BSC_DATALINK == 1
#define DEBUG_PRINTF debug_printf
#else
#undef DEBUG_ENABLED
#define DEBUG_PRINTF(...)
#endif

#define BSC_NEXT_POWER_OF_TWO1(v) \
    ((((unsigned int)v) - 1) | ((((unsigned int)v) - 1) >> 1))
#define BSC_NEXT_POWER_OF_TWO2(v) \
    (BSC_NEXT_POWER_OF_TWO1(v) | BSC_NEXT_POWER_OF_TWO1(v) >> 2)
#define BSC_NEXT_POWER_OF_TWO3(v) \
    (BSC_NEXT_POWER_OF_TWO2(v) | BSC_NEXT_POWER_OF_TWO2(v) >> 4)
#define BSC_NEXT_POWER_OF_TWO4(v) \
    (BSC_NEXT_POWER_OF_TWO3(v) | BSC_NEXT_POWER_OF_TWO3(v) >> 8)
#define BSC_NEXT_POWER_OF_TWO(v) \
    ((BSC_NEXT_POWER_OF_TWO4(v) | BSC_NEXT_POWER_OF_TWO4(v) >> 16)) + 1

typedef enum {
    BSC_DATALINK_STATE_IDLE = 0,
    BSC_DATALINK_STATE_STARTING = 1,
    BSC_DATALINK_STATE_STARTED = 2,
    BSC_DATALINK_STATE_STOPPING = 3
} BSC_DATALINK_STATE;

static FIFO_BUFFER bsc_fifo = { 0 };
static uint8_t bsc_fifo_buf[BSC_NEXT_POWER_OF_TWO(BVLC_SC_NPDU_SIZE_CONF * 10)];
static BSC_NODE *bsc_node = NULL;
static BSC_NODE_CONF bsc_conf;
static BSC_DATALINK_STATE bsc_datalink_state = BSC_DATALINK_STATE_IDLE;
static BSC_EVENT *bsc_event = NULL;
static BSC_EVENT *bsc_data_event = NULL;

static void bsc_node_event(BSC_NODE *node,
    BSC_NODE_EVENT ev,
    BACNET_SC_VMAC_ADDRESS *dest,
    uint8_t *pdu,
    uint16_t pdu_len)
{
    DEBUG_PRINTF("bsc_node_event() >>> ev = %d\n", ev);
    bws_dispatch_lock();
    (void)node;
    (void)dest;
    if (ev == BSC_NODE_EVENT_STARTED || ev == BSC_NODE_EVENT_STOPPED) {
        if (bsc_datalink_state != BSC_DATALINK_STATE_IDLE) {
            bsc_event_signal(bsc_event);
        }
    } else if (ev == BSC_NODE_EVENT_RECEIVED_NPDU) {
        if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
            if (FIFO_Available(&bsc_fifo, pdu_len + sizeof(pdu_len))) {
                FIFO_Add(&bsc_fifo, (uint8_t *)&pdu_len, sizeof(pdu_len));
                FIFO_Add(&bsc_fifo, pdu, pdu_len);
                bsc_event_signal(bsc_data_event);
            }
#if DEBUG_ENABLED == 1
            else {
                DEBUG_PRINTF("pdu of size %d\n is dropped\n", pdu_len);
            }
#endif
        }
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_node_event() <<<\n");
}

static void bsc_deinit_resources(void)
{
    if (bsc_event) {
        bsc_event_deinit(bsc_event);
        bsc_event = NULL;
    }
    if (bsc_data_event) {
        bsc_event_deinit(bsc_data_event);
        bsc_data_event = NULL;
    }
}

bool bsc_init(char *ifname)
{
    BSC_SC_RET r;
    bool ret = false;
    DEBUG_PRINTF("bsc_init() >>>\n");

    (void)ifname;

    bws_dispatch_lock();

    if (bsc_datalink_state != BSC_DATALINK_STATE_IDLE) {
        bws_dispatch_unlock();
        DEBUG_PRINTF("bsc_init() <<< ret = %d\n", ret);
        return ret;
    }

    bsc_event = bsc_event_init();
    bsc_data_event = bsc_event_init();

    if (!bsc_event || !bsc_data_event) {
        bsc_deinit_resources();
        bws_dispatch_unlock();
        DEBUG_PRINTF("bsc_init() <<< ret = %d\n", false);
        return false;
    }

    DEBUG_PRINTF("bsc_init() BACNET/SC datalink configured to use input fifo "
                 "of size %d\n",
        sizeof(bsc_fifo_buf));
    FIFO_Init(&bsc_fifo, bsc_fifo_buf, sizeof(bsc_fifo_buf));

    ret = bsc_node_conf_fill_from_netport(&bsc_conf, &bsc_node_event);

    if (!ret) {
        bsc_deinit_resources();
        bws_dispatch_unlock();
        DEBUG_PRINTF("bsc_init() <<< configuration of BACNET/SC datalink "
                     "failed, ret = false\n");
        return ret;
    }

    bsc_datalink_state = BSC_DATALINK_STATE_STARTING;
    r = bsc_node_init(&bsc_conf, &bsc_node);
    if (r == BSC_SC_SUCCESS) {
        r = bsc_node_start(bsc_node);
        if (r == BSC_SC_SUCCESS) {
            bws_dispatch_unlock();
            bsc_event_wait(bsc_event);
            bws_dispatch_lock();
            bsc_datalink_state = BSC_DATALINK_STATE_STARTED;
            bws_dispatch_unlock();
            DEBUG_PRINTF("bsc_init() <<< ret = %d\n", true);
            return true;
        }
    }
    bsc_deinit_resources();
    bsc_node_conf_cleanup(&bsc_conf);
    bsc_datalink_state = BSC_DATALINK_STATE_IDLE;
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_init() <<< ret = %d\n", false);
    return false;
}

void bsc_cleanup(void)
{
    DEBUG_PRINTF("bsc_cleanup() >>>\n");
    bws_dispatch_lock();
    if (bsc_datalink_state != BSC_DATALINK_STATE_IDLE &&
        bsc_datalink_state != BSC_DATALINK_STATE_STOPPING) {
        if (bsc_datalink_state == BSC_DATALINK_STATE_STARTING) {
            bws_dispatch_unlock();
            bsc_event_wait(bsc_event);
            bws_dispatch_lock();
        }
        bsc_datalink_state = BSC_DATALINK_STATE_STOPPING;
        bsc_event_signal(bsc_data_event);
        bsc_node_stop(bsc_node);
        bws_dispatch_unlock();
        bsc_event_wait(bsc_event);
        bsc_event_wait(bsc_data_event);
        bws_dispatch_lock();
        bsc_deinit_resources();
        (void)bsc_node_deinit(bsc_node);
        bsc_node_conf_cleanup(&bsc_conf);
        bsc_node = NULL;
        bsc_datalink_state = BSC_DATALINK_STATE_IDLE;
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_cleanup() <<<\n");
}

int bsc_send_pdu(BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    (void)npdu_data;
    BSC_SC_RET ret;
    BACNET_SC_VMAC_ADDRESS dest_vmac;
    int len = -1;
    static uint8_t buf[BVLC_SC_NPDU_SIZE_CONF];

    /* this datalink doesn't need to know the npdu data */
    (void)npdu_data;

    bws_dispatch_lock();

    if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
        if (dest->net == BACNET_BROADCAST_NETWORK || dest->mac_len == 0) {
            /* broadcast message */
            memset(&dest_vmac.address[0], 0xFF, BVLC_SC_VMAC_SIZE);
        } else if (dest->mac_len == BVLC_SC_VMAC_SIZE) {
            /* unicast */
            memcpy(&dest_vmac.address[0], &dest->mac[0], BVLC_SC_VMAC_SIZE);
        } else {
            bws_dispatch_unlock();
            DEBUG_PRINTF(
                "bsc_send_pdu() <<< ret = -1, incorrect dest mac address\n");
            return len;
        }

        len = bvlc_sc_encode_encapsulated_npdu(buf, sizeof(buf),
            bsc_get_next_message_id(), NULL, &dest_vmac, pdu, pdu_len);

        ret = bsc_node_send(bsc_node, buf, len);
        len = pdu_len;

        if (ret != BSC_SC_SUCCESS) {
            len = -1;
        }
    }

    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_send_pdu() <<< ret = %d\n", len);
    return len;
}

static void bsc_remove_packet(uint16_t packet_size)
{
    int i;
    for (i = 0; i < packet_size; i++) {
        (void)FIFO_Get(&bsc_fifo);
    }
}

uint16_t bsc_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout_ms)
{
    uint16_t pdu_len = 0;
    uint16_t npdu_len = 0;
    BVLC_SC_DECODED_MESSAGE dm;
    BACNET_ERROR_CODE error;
    BACNET_ERROR_CLASS class;
    const char *err_desc = NULL;
    static uint8_t buf[BVLC_SC_NPDU_SIZE_CONF];

    DEBUG_PRINTF("bsc_receive() >>>\n");

    bws_dispatch_lock();

    if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
        if (FIFO_Count(&bsc_fifo) <= sizeof(uint16_t)) {
            bws_dispatch_unlock();
            bsc_event_timedwait(bsc_data_event, timeout_ms);
            bws_dispatch_lock();
        }

        if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED &&
            FIFO_Count(&bsc_fifo) > sizeof(uint16_t)) {
            DEBUG_PRINTF("bsc_receive() processing data...\n");
            FIFO_Pull(&bsc_fifo, (uint8_t *)&npdu_len, sizeof(npdu_len));

            if (sizeof(buf) < npdu_len) {
                DEBUG_PRINTF(
                    "bsc_receive() pdu of size %d is dropped\n", pdu_len);
                bsc_remove_packet(npdu_len);
            } else {
                FIFO_Pull(&bsc_fifo, buf, npdu_len);
                if (!bvlc_sc_decode_message(
                        buf, npdu_len, &dm, &error, &class, &err_desc)) {
                    DEBUG_PRINTF(
                        "bsc_receive() pdu of size %d is dropped because "
                        "of err = %d, class %d, desc = %s\n",
                        npdu_len, error, class, err_desc);
                    bsc_remove_packet(npdu_len);
                } else {
                    if (dm.hdr.origin &&
                        max_pdu >= dm.payload.encapsulated_npdu.npdu_len) {
                        src->mac_len = BVLC_SC_VMAC_SIZE;
                        memcpy(&src->mac[0], &dm.hdr.origin->address[0],
                            BVLC_SC_VMAC_SIZE);
                        memcpy(pdu, dm.payload.encapsulated_npdu.npdu,
                            dm.payload.encapsulated_npdu.npdu_len);
                        pdu_len = dm.payload.encapsulated_npdu.npdu_len;
                    }
#if DEBUG_ENABLED == 1
                    else {
                        DEBUG_PRINTF("bsc_receive() pdu of size %d is dropped "
                                     "because origin addr is absent or output "
                                     "buf of size %d is to small\n",
                            npdu_len, max_pdu);
                    }
#endif
                }
            }
            DEBUG_PRINTF("bsc_receive() pdu_len = %d\n", npdu_len);
        }
    }

    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_receive() <<< ret = %d\n", pdu_len);
    return pdu_len;
}

void bsc_get_broadcast_address(BACNET_ADDRESS *dest)
{
    if (dest) {
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->mac_len = BVLC_SC_VMAC_SIZE;
        memset(&dest->mac[0], 0xFF, BVLC_SC_VMAC_SIZE);
        /* no SADR */
        dest->len = 0;
        memset(dest->adr, 0, sizeof(dest->adr));
    }
}

void bsc_get_my_address(BACNET_ADDRESS *my_address)
{
    if (my_address) {
        memset(my_address, 0, sizeof(*my_address));
    }

    bws_dispatch_lock();
    if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
        my_address->mac_len = BVLC_SC_VMAC_SIZE;
        memcpy(&my_address->mac[0], &bsc_conf.local_vmac->address[0],
            BVLC_SC_VMAC_SIZE);
    }
    bws_dispatch_unlock();
}

BVLC_SC_HUB_CONNECTION_STATUS bsc_hub_connection_status(void)
{
    BVLC_SC_HUB_CONNECTION_STATUS ret = BVLC_SC_HUB_CONNECTION_ABSENT;
    bws_dispatch_lock();
    if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
        ret = bsc_node_hub_connector_status(bsc_node);
    }
    bws_dispatch_unlock();
    return ret;
}

bool bsc_direct_connection_established(
    BACNET_SC_VMAC_ADDRESS *dest, char **urls, size_t urls_cnt)
{
    bool ret = false;
    bws_dispatch_lock();
    if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
        ret = bsc_node_direct_connection_established(
            bsc_node, dest, urls, urls_cnt);
    }
    bws_dispatch_unlock();
    return ret;
}

BSC_SC_RET bsc_connect_direct(
    BACNET_SC_VMAC_ADDRESS *dest, char **urls, size_t urls_cnt)
{
    BSC_SC_RET ret = BSC_SC_INVALID_OPERATION;
    DEBUG_PRINTF(
        "bsc_connect_direct() >>> dest = %p, urls = %p, urls_cnt = %d\n", dest,
        urls, urls_cnt);
    bws_dispatch_lock();
    if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
        ret = bsc_node_connect_direct(bsc_node, dest, urls, urls_cnt);
    }
    bws_dispatch_unlock();
    DEBUG_PRINTF("bsc_connect_direct() ret = %d\n", ret);
    return ret;
}

void bsc_disconnect_direct(BACNET_SC_VMAC_ADDRESS *dest)
{
    bws_dispatch_lock();
    if (bsc_datalink_state == BSC_DATALINK_STATE_STARTED) {
        bsc_node_disconnect_direct(bsc_node, dest);
    }
    bws_dispatch_unlock();
}

void bsc_maintenance_timer(uint16_t seconds)
{
    bsc_node_maintenance_timer(seconds);
}