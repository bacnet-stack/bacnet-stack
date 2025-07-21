/**
 * @file
 * @brief API for BACnet Who-Is service encoder and decoder
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_WHOIS_H
#define BACNET_WHOIS_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* encode service  - use -1 for limit if you want unlimited */
BACNET_STACK_EXPORT
int whois_request_encode(uint8_t *apdu, int32_t low_limit, int32_t high_limit);
BACNET_STACK_EXPORT
int whois_encode_apdu(uint8_t *apdu, int32_t low_limit, int32_t high_limit);

BACNET_STACK_EXPORT
int whois_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_len,
    int32_t *pLow_limit,
    int32_t *pHigh_limit);

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
 * to determine the device object identifier and network addresses of all
 * devices on the network, or to determine the network address of a specific
 * device whose device object identifier is known, but whose address is not.
 * <br> The I-Am service is also an unconfirmed service. The I-Am service is
 * used to respond to Who-Is service requests. However, the I-Am service request
 * may be issued at any time. It does not need to be preceded by the receipt of
 * a Who-Is service request. In particular, a device may wish to broadcast an
 * I-Am service request when it powers up. The network address is derived either
 * from the MAC address associated with the I-Am service request, if the device
 * issuing the request is on the local network, or from the NPCI if the device
 * is on a remote network.
 */
#endif
