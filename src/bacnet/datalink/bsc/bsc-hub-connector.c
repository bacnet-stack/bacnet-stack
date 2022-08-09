/**
 * @file
 * @brief BACNet hub connector API.
 * @author Kirill Neznamov
 * @date July 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */

#include "bacnet/datalink/bsc/bsc-hub-connector.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/bsc/bsc-util.h"
#include "bacnet/datalink/bsc/bsc-connection-private.h"

typedef enum
{
  BSC_HUB_CONNECTOR_STATE_IDLE = 0,
  BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY = 1,
  BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER = 2,
  BSC_HUB_CONNECTOR_STATE_WAIT_FOR_RECONNECT = 3
} BSC_HUB_CONNECTOR_STATE;

struct BSC_Hub_Connector {
    BSC_HUB_CONNECTOR *next;
    BSC_HUB_CONNECTOR *last;
    BSC_CONNECTION primary;
    BSC_CONNECTION failover;
    BSC_HUB_CONNECTOR_STATE state;
    unsigned int reconnect_timeout_s;
    char* primary_url;
    char* failover_url;
    unsigned long time_stamp;
};

static BSC_HUB_CONNECTOR *bsc_hub_conn_list_head = NULL;
static BSC_HUB_CONNECTOR *bsc_hub_conn_list_tail = NULL;

static void bsc_hub_connector_remove(BSC_HUB_CONNECTOR *c)
{
    debug_printf("bsc_hub_connector_remove() >>> c = %p, head = %p, tail = %p\n",
        c, bsc_hub_conn_list_head, bsc_hub_conn_list_tail);

    c->state = BSC_HUB_CONNECTOR_STATE_IDLE;

    if (bsc_hub_conn_list_head == bsc_hub_conn_list_tail) {
        bsc_hub_conn_list_head = NULL;
        bsc_hub_conn_list_tail = NULL;
    } else if (!c->last) {
        bsc_hub_conn_list_head = c->next;
        bsc_hub_conn_list_head->last = NULL;
    } else if (!c->next) {
        bsc_hub_conn_list_tail = c->last;
        bsc_hub_conn_list_tail->next = NULL;
    } else {
        c->next->last = c->last;
        c->last->next = c->next;
    }

    debug_printf("bsc_hub_connector_remove() <<<\n");
}

static void bsc_hub_connector_add(BSC_HUB_CONNECTOR* c)
{
    debug_printf("bsc_hub_connector_add() >>> c = %p\n", c);

    if (bsc_hub_conn_list_head == NULL && bsc_hub_conn_list_tail == NULL) {
        bsc_hub_conn_list_head = c;
        bsc_hub_conn_list_tail = c;
        c->next = NULL;
        c->last = NULL;
    } else if (bsc_hub_conn_list_head == bsc_hub_conn_list_tail) {
        c->last = bsc_hub_conn_list_head;
        c->next = NULL;
        bsc_hub_conn_list_tail = c;
        bsc_hub_conn_list_head->next = c;
    } else {
        c->last = bsc_hub_conn_list_tail;
        c->next = NULL;
        bsc_hub_conn_list_tail->next = c;
        bsc_hub_conn_list_tail = c;
    }
    debug_printf("bsc_hub_connector_add() <<< \n");
}

static bool bsc_hub_connector_connect(BSC_HUB_CONNECTOR *c) 
{
  bool ret;

  debug_printf("bsc_hub_connector_connect() >>> c = %p\n", c);

  ret = bsc_connect(&c->primary, c->primary_url, BACNET_WEBSOCKET_HUB_CONNECTION);

  if(!ret) {
    debug_printf("bsc_hub_connector_connect() connection to primary hub failed, trying failover hub...\n");
    ret = bsc_connect(&c->failover, c->failover_url, BACNET_WEBSOCKET_HUB_CONNECTION);
    if(!ret) {
      debug_printf("bsc_hub_connector_connect() connection to failover hub failed\n");
      debug_printf("bsc_hub_connector_connect() <<< ret = 0\n");
      return false;
    }
    else {
      c->state = BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER;
    }
  }
  else {
    c->state = BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY;
    bsc_hub_connector_add(c);
  }
  debug_printf("bsc_hub_connector_connect() <<< ret = 1\n");
  return true;
}

bool bsc_hub_connector_start(BSC_HUB_CONNECTOR *c,
                             char* primaryURL,
                             char* failoverURL,
                             unsigned int reconnect_timeout_s)
{
  bool ret;

  debug_printf("bsc_hub_connector_start() >>> c = %p, primaryURL = %s, failoverURL = %s, reconnnect_timeout_s = %d\n",
               c, primaryURL, failoverURL, reconnect_timeout_s);

  memset(c, 0, sizeof(*c));
  c->primary_url = malloc(strlen(primaryURL) + 1);
  c->failover_url = malloc(strlen(failoverURL) + 1);
  c->reconnect_timeout_s = reconnect_timeout_s;

  if(!c->primary_url || !c->failover_url) {
    debug_printf("bsc_hub_connector_start() memory allocation failed\n");
    if(c->primary_url) {
      free(c->primary_url);
    }
    if(c->failover_url) {
      free(c->failover_url);
    }
    debug_printf("bsc_hub_connector_start() <<< ret = 0\n");
    return false;
  }

  strcpy(c->primary_url, primaryURL);
  strcpy(c->failover_url, failoverURL);
  bsc_hub_connector_add(c);

  ret = bsc_hub_connector_connect(c);

  if(!ret) {
      free(c->primary_url);
      free(c->failover_url);
      bsc_hub_connector_remove(c);
  }

  debug_printf("bsc_hub_connector_start() <<< ret = %d\n", ret);
  return ret;
}

void bsc_hub_connector_stop(BSC_HUB_CONNECTOR *c)
{
  debug_printf("bsc_hub_connector_stop() >>> c = %p\n", c);

  if(c->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY) {
    bsc_disconnect(&c->primary);
  }
  else if(c->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER) {
    bsc_disconnect(&c->failover);
  }

  free(c->primary_url);
  free(c->failover_url);
  bsc_hub_connector_remove(c);

  debug_printf("bsc_hub_connector_stop() <<<\n");
}

static void bsc_hub_process_wait_for_reconnect(BSC_HUB_CONNECTOR *c)
{
   bool ret;
   debug_printf("bsc_hub_process_wait_for_reconnect() >>> c = %p, state= %d\n", c, c->state);

   if(c->state == BSC_HUB_CONNECTOR_STATE_WAIT_FOR_RECONNECT) {
     if(!bsc_seconds_left(c->time_stamp, c->reconnect_timeout_s)) {
        debug_printf("bsc_hub_process_wait_for_reconnect() re-connect wait timeout of %d seconds is elapsed for hub connector %p \n", c->reconnect_timeout_s, c);
        ret = bsc_hub_connector_connect(c);
        if(!ret) {
          debug_printf("bsc_hub_process_wait_for_reconnect() re-connect attempt failed, wait for %d seconds\n", c->reconnect_timeout_s);
          c->time_stamp = mstimer_now();
        }
        else {
          debug_printf("bsc_hub_process_wait_for_reconnect() re-connect attempt succeded, state = %d\n", c->state);
        }
     }
    }
   debug_printf("bsc_hub_process_wait_for_reconnect() <<<\n", c, c->state);
}

bool bsc_hub_connector_send(BSC_HUB_CONNECTOR *c,
     uint8_t *pdu,
     unsigned pdu_len)
{
  int ret;

  debug_printf("bsc_hub_connector_send() >>> c = %p, pdu = %p, pdu_len = %d\n", c, pdu, pdu_len);

  if(c->state == BSC_HUB_CONNECTOR_STATE_IDLE) {
    debug_printf("bsc_hub_connector_send() hub connector is in idle state, pdu is dropped\n");
    debug_printf("bsc_hub_connector_send() <<< ret = false\n");
    return false;
  }
  
  bsc_hub_process_wait_for_reconnect(c);

  if(c->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY) {
    ret = bsc_send(&c->primary, pdu, pdu_len);
  }
  else if(c->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER) {
    ret = bsc_send(&c->failover, pdu, pdu_len);
  }

  if(ret < 0)
  {
    debug_printf("bsc_hub_connector_send() send failed, try wait and reconnect\n");
    c->state = BSC_HUB_CONNECTOR_STATE_WAIT_FOR_RECONNECT;
    c->time_stamp = mstimer_now();
    debug_printf("bsc_hub_connector_send() <<< ret = false\n");
    return false;
  }
  else if(ret == 0) {
    debug_printf("bsc_hub_connector_send() <<< ret = false\n");
    return false;
  }

  debug_printf("bsc_hub_connector_send() <<< ret = true\n");
  return true;
}

int bsc_hub_connector_recv(BSC_HUB_CONNECTOR *c,
                   uint8_t *pdu,
                   uint16_t max_pdu,
                   unsigned int timeout)
{
  int ret = 0;
  debug_printf("bsc_hub_connector_recv() >>> c = %p, pdu = %p, max_pdu = %d, timeout = %d\n", c, pdu, max_pdu, timeout);

  if(c->state == BSC_HUB_CONNECTOR_STATE_IDLE) {
    debug_printf("bsc_hub_connector_recv() hub connector is in idle state, recv failed\n");
    debug_printf("bsc_hub_connector_recv() <<< ret = 0\n");
    return ret;
  }

  bsc_hub_process_wait_for_reconnect(c);

  if(c->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_PRIMARY) {
    ret = bsc_recv(&c->primary, pdu, max_pdu, timeout);
  }
  else if(c->state == BSC_HUB_CONNECTOR_STATE_CONNECTED_FAILOVER) {
    ret = bsc_recv(&c->failover, pdu, max_pdu, timeout);
  }

  if(ret < 0)
  {
    debug_printf("bsc_hub_connector_recv() recv failed, try wait and reconnect\n");
    c->state = BSC_HUB_CONNECTOR_STATE_WAIT_FOR_RECONNECT;
    c->time_stamp = mstimer_now();
    debug_printf("bsc_hub_connector_recv() <<< ret = 0\n");
    return 0;
  }
  else if(ret == 0) {
    debug_printf("bsc_hub_connector_recv() <<< ret =0\n");
    return 0;
  }

  debug_printf("bsc_hub_connector_recv() <<< ret = %d\n", ret);
  return ret;
}
