/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

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
#ifndef WHOIS_H
#define WHOIS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* encode service  - use -1 for limit if you want unlimited */
    int whois_encode_apdu(
        uint8_t * apdu,
        int32_t low_limit,
        int32_t high_limit);

    int whois_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        int32_t * pLow_limit,
        int32_t * pHigh_limit);

#ifdef TEST
    int whois_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_len,
        int32_t * pLow_limit,
        int32_t * pHigh_limit);

    void testWhoIs(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
/** @defgroup DMDDB Device Management-Dynamic Device Binding (DM-DDB)
 * @ingroup RDMS
 * 16.10 Who-Is and I-Am Services <br>
 * The Who-Is service is used by a sending BACnet-user to determine the device 
 * object identifier, the network address, or both, of other BACnet devices 
 * that share the same internetwork. 
 * The Who-Is service is an unconfirmed service. The Who-Is service may be used 
 * to determine the device object identifier and network addresses of all devices 
 * on the network, or to determine the network address of a specific device whose 
 * device object identifier is known, but whose address is not. <br> 
 * The I-Am service is also an unconfirmed service. The I-Am service is used to 
 * respond to Who-Is service requests. However, the I-Am service request may be 
 * issued at any time. It does not need to be preceded by the receipt of a 
 * Who-Is service request. In particular, a device may wish to broadcast an I-Am 
 * service request when it powers up. The network address is derived either
 * from the MAC address associated with the I-Am service request, if the device 
 * issuing the request is on the local network, or from the NPCI if the device 
 * is on a remote network.
 */
#endif
