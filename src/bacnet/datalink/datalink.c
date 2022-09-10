/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2007 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
/** @file datalink.c  Optional run-time assignment of datalink transport */
#include "bacnet/datalink/datalink.h"

#if defined(BACDL_ALL) || defined FOR_DOXYGEN
#include "bacnet/datalink/ethernet.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/datalink/bvlc.h"
#include "bacnet/basic/bbmd/h_bbmd.h"
#include "bacnet/datalink/bip6.h"
#include "bacnet/datalink/bvlc6.h"
#include "bacnet/basic/bbmd6/h_bbmd6.h"
#include "bacnet/datalink/arcnet.h"
#include "bacnet/datalink/dlmstp.h"
#include <strings.h> /* for strcasecmp() */

static enum {
    DATALINK_NONE = 0,
    DATALINK_ARCNET,
    DATALINK_ETHERNET,
    DATALINK_BIP,
    DATALINK_BIP6,
    DATALINK_MSTP
} Datalink_Transport;

void datalink_set(char *datalink_string)
{
    if (strcasecmp("bip", datalink_string) == 0) {
        Datalink_Transport = DATALINK_BIP;
    } else if (strcasecmp("bip6", datalink_string) == 0) {
        Datalink_Transport = DATALINK_BIP6;
    } else if (strcasecmp("ethernet", datalink_string) == 0) {
        Datalink_Transport = DATALINK_ETHERNET;
    } else if (strcasecmp("arcnet", datalink_string) == 0) {
        Datalink_Transport = DATALINK_ARCNET;
    } else if (strcasecmp("mstp", datalink_string) == 0) {
        Datalink_Transport = DATALINK_MSTP;
    } else if (strcasecmp("none", datalink_string) == 0) {
        Datalink_Transport = DATALINK_NONE;
    }
}

bool datalink_init(char *ifname)
{
    bool status = false;

    switch (Datalink_Transport) {
        case DATALINK_NONE:
            status = true;
            break;
        case DATALINK_ARCNET:
            status = arcnet_init(ifname);
            break;
        case DATALINK_ETHERNET:
            status = ethernet_init(ifname);
            break;
        case DATALINK_BIP:
            status = bip_init(ifname);
            break;
        case DATALINK_BIP6:
            status = bip6_init(ifname);
            break;
        case DATALINK_MSTP:
            status = dlmstp_init(ifname);
            break;
        default:
            break;
    }

    return status;
}

int datalink_send_pdu(BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    int bytes = 0;

    switch (Datalink_Transport) {
        case DATALINK_NONE:
            bytes = pdu_len;
            break;
        case DATALINK_ARCNET:
            bytes = arcnet_send_pdu(dest, npdu_data, pdu, pdu_len);
            break;
        case DATALINK_ETHERNET:
            bytes = ethernet_send_pdu(dest, npdu_data, pdu, pdu_len);
            break;
        case DATALINK_BIP:
            bytes = bip_send_pdu(dest, npdu_data, pdu, pdu_len);
            break;
        case DATALINK_BIP6:
            bytes = bip6_send_pdu(dest, npdu_data, pdu, pdu_len);
            break;
        case DATALINK_MSTP:
            bytes = dlmstp_send_pdu(dest, npdu_data, pdu, pdu_len);
            break;
        default:
            break;
    }

    return bytes;
}

uint16_t datalink_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout)
{
    uint16_t bytes = 0;

    switch (Datalink_Transport) {
        case DATALINK_NONE:
            break;
        case DATALINK_ARCNET:
            bytes = arcnet_receive(src, pdu, max_pdu, timeout);
            break;
        case DATALINK_ETHERNET:
            bytes = ethernet_receive(src, pdu, max_pdu, timeout);
            break;
        case DATALINK_BIP:
            bytes = bip_receive(src, pdu, max_pdu, timeout);
            break;
        case DATALINK_BIP6:
            bytes = bip6_receive(src, pdu, max_pdu, timeout);
            break;
        case DATALINK_MSTP:
            bytes = dlmstp_receive(src, pdu, max_pdu, timeout);
            break;
        default:
            break;
    }

    return bytes;
}

void datalink_cleanup(void)
{
    switch (Datalink_Transport) {
        case DATALINK_NONE:
            break;
        case DATALINK_ARCNET:
            arcnet_cleanup();
            break;
        case DATALINK_ETHERNET:
            ethernet_cleanup();
            break;
        case DATALINK_BIP:
            bip_cleanup();
            break;
        case DATALINK_BIP6:
            bip6_cleanup();
            break;
        case DATALINK_MSTP:
            dlmstp_cleanup();
            break;
        default:
            break;
    }
}

void datalink_get_broadcast_address(BACNET_ADDRESS *dest)
{
    switch (Datalink_Transport) {
        case DATALINK_NONE:
            break;
        case DATALINK_ARCNET:
            arcnet_get_broadcast_address(dest);
            break;
        case DATALINK_ETHERNET:
            ethernet_get_broadcast_address(dest);
            break;
        case DATALINK_BIP:
            bip_get_broadcast_address(dest);
            break;
        case DATALINK_BIP6:
            bip6_get_broadcast_address(dest);
            break;
        case DATALINK_MSTP:
            dlmstp_get_broadcast_address(dest);
            break;
        default:
            break;
    }
}

void datalink_get_my_address(BACNET_ADDRESS *my_address)
{
    switch (Datalink_Transport) {
        case DATALINK_NONE:
            break;
        case DATALINK_ARCNET:
            arcnet_get_my_address(my_address);
            break;
        case DATALINK_ETHERNET:
            ethernet_get_my_address(my_address);
            break;
        case DATALINK_BIP:
            bip_get_my_address(my_address);
            break;
        case DATALINK_BIP6:
            bip6_get_my_address(my_address);
            break;
        case DATALINK_MSTP:
            dlmstp_get_my_address(my_address);
            break;
        default:
            break;
    }
}

void datalink_set_interface(char *ifname)
{
    switch (Datalink_Transport) {
        case DATALINK_NONE:
            (void)ifname;
            break;
        case DATALINK_ARCNET:
            (void)ifname;
            break;
        case DATALINK_ETHERNET:
            (void)ifname;
            break;
        case DATALINK_BIP:
            (void)ifname;
            break;
        case DATALINK_BIP6:
            (void)ifname;
            break;
        case DATALINK_MSTP:
            (void)ifname;
            break;
        default:
            break;
    }
}

void datalink_maintenance_timer(uint16_t seconds)
{
    switch (Datalink_Transport) {
        case DATALINK_NONE:
            break;
        case DATALINK_ARCNET:
            break;
        case DATALINK_ETHERNET:
            break;
        case DATALINK_BIP:
            bvlc_maintenance_timer(seconds);
            break;
        case DATALINK_BIP6:
            bvlc6_maintenance_timer(seconds);
            break;
        case DATALINK_MSTP:
            break;
        default:
            break;
    }
}
#endif

#if defined(BACDL_NONE)
bool datalink_init(char *ifname)
{
    (void)ifname;

    return true;
}

int datalink_send_pdu(BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    (void)dest;
    (void)npdu_data;
    (void)pdu;
    (void)pdu_len;

    return 0;
}

uint16_t datalink_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout)
{
    (void)src;
    (void)pdu;
    (void)max_pdu;
    (void)timeout;

    return 0;
}

void datalink_cleanup(void)
{
}

void datalink_get_broadcast_address(BACNET_ADDRESS *dest)
{
    (void)dest;
}

void datalink_get_my_address(BACNET_ADDRESS *my_address)
{
    (void)my_address;
}

void datalink_set_interface(char *ifname)
{
    (void)ifname;
}

void datalink_set(char *datalink_string)
{
    (void)datalink_string;
}

void datalink_maintenance_timer(uint16_t seconds)
{
    (void)seconds;
}
#endif
