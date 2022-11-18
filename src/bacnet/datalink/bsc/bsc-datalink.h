/**
 * @file
 * @brief BACNet hub connector API.
 * @author Kirill Neznamov
 * @date October 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */

#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/bacenum.h"

/* on Linux, ifname is eth0, ath0, arc0, and others.
   on Windows, ifname is the dotted ip address of the interface
   NULL means all interface.
 */

BACNET_STACK_EXPORT
bool bsc_init(char *ifname);

BACNET_STACK_EXPORT
void bsc_cleanup(void);

BACNET_STACK_EXPORT
int bsc_send_pdu(BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len);

BACNET_STACK_EXPORT
uint16_t bsc_receive(BACNET_ADDRESS *src,
    uint8_t *pdu,
    uint16_t max_pdu,
    unsigned timeout);

BACNET_STACK_EXPORT
void bsc_init_conf(uint32_t instance);
