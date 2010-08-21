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
#include "datalink.h"
#include <string.h>
#include "session.h"

/** @file datalink.c  Optional run-time assignment of datalink transport */

#ifndef WITH_MACRO_LINK_FUNCTIONS

void datalink_set(
    struct bacnet_session_object *session_object,
    const char *datalink_string)
{
    /* zero-init */
    session_object->datalink_init = NULL;
    session_object->datalink_send_pdu = NULL;
    session_object->datalink_receive = NULL;
    session_object->datalink_cleanup = NULL;
    session_object->datalink_get_broadcast_address = NULL;
    session_object->datalink_get_my_address = NULL;

#if ( defined(BACDL_ALL) || defined(BACDL_BIP) ) && defined(BIP_H)
    if (!datalink_string || strcasecmp("bip", datalink_string) == 0) {
        session_object->datalink_init = bip_init;
#if defined(BVLC_ENABLED) && BVLC_ENABLED
        session_object->datalink_send_pdu = bvlc_send_pdu;
        session_object->datalink_receive = bvlc_receive;
#else
        session_object->datalink_send_pdu = bip_send_pdu;
        session_object->datalink_receive = bip_receive;
#endif

        session_object->datalink_cleanup = bip_cleanup;
        session_object->datalink_get_broadcast_address =
            bip_get_broadcast_address;
        session_object->datalink_get_my_address = bip_get_my_address;
    } else
#endif
#if ( defined(BACDL_ALL) || defined(BACDL_ETHERNET) ) && defined(ETHERNET_H)
    if (!datalink_string || strcasecmp("ethernet", datalink_string) == 0) {
        session_object->datalink_init = ethernet_init;
        session_object->datalink_send_pdu = ethernet_send_pdu;
        session_object->datalink_receive = ethernet_receive;
        session_object->datalink_cleanup = ethernet_cleanup;
        session_object->datalink_get_broadcast_address =
            ethernet_get_broadcast_address;
        session_object->datalink_get_my_address = ethernet_get_my_address;
    } else
#endif
#if ( defined(BACDL_ALL) || defined(BACDL_ARCNET) ) && defined(ARCNET_H)
    if (!datalink_string || strcasecmp("arcnet", datalink_string) == 0) {
        session_object->datalink_init = arcnet_init;
        session_object->datalink_send_pdu = arcnet_send_pdu;
        session_object->datalink_receive = arcnet_receive;
        session_object->datalink_cleanup = arcnet_cleanup;
        session_object->datalink_get_broadcast_address =
            arcnet_get_broadcast_address;
        session_object->datalink_get_my_address = arcnet_get_my_address;
    } else
#endif /*ARCNET */
#if ( defined(BACDL_ALL) || defined(BACDL_MSTP) ) && defined(DLMSTP_H)
    if (!datalink_string || strcasecmp("mstp", datalink_string) == 0) {
        session_object->datalink_init = dlmstp_init;
        session_object->datalink_send_pdu = dlmstp_send_pdu;
        session_object->datalink_receive = dlmstp_receive;
        session_object->datalink_cleanup = dlmstp_cleanup;
        session_object->datalink_get_broadcast_address =
            dlmstp_get_broadcast_address;
        session_object->datalink_get_my_address = dlmstp_get_my_address;
    } else
#endif /*MSTP */
    {
        /* empty block */
    }
}

#endif /* ! WITH_MACRO_LINK_FUNCTIONS */
