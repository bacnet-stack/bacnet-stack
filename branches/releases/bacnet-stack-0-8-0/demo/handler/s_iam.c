/**************************************************************************
*
* Copyright (C) 2008 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "config.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "address.h"
#include "tsm.h"
#include "dcc.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "datalink.h"
#include "iam.h"
#include "txbuf.h"
/* some demo stuff needed */
#include "handlers.h"
#include "client.h"

/** @file s_iam.c  Send an I-Am message. */

/** Send a I-Am request to a remote network for a specific device.
 * @param target_address [in] BACnet address of target router
 * @param device_id [in] Device Instance 0 - 4194303
 * @param max_apdu [in] Max APDU 0-65535
 * @param segmentation [in] #BACNET_SEGMENTATION enumeration
 * @param vendor_id [in] BACnet vendor ID 0-65535
 */
void Send_I_Am_To_Network(
    BACNET_ADDRESS * target_address,
    uint32_t device_id,
    unsigned int max_apdu,
    int segmentation,
    uint16_t vendor_id)
{
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    datalink_get_my_address(&my_address);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);

    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], target_address,
        &my_address, &npdu_data);
    /* encode the APDU portion of the packet */
    /* encode the APDU portion of the packet */
    len =
        iam_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
        device_id, max_apdu, segmentation, vendor_id);
    pdu_len += len;
    bytes_sent =
        datalink_send_pdu(target_address, &npdu_data,
        &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
#if PRINT_ENABLED
        fprintf(stderr, "Failed to Send I-Am Request (%s)!\n",
            strerror(errno));
#endif
    }

}

/** Encode an I Am message to be broadcast.
 * @param buffer [in,out] The buffer to use for building the message.
 * @param dest [out] The destination address information.
 * @param npdu_data [out] The NPDU structure describing the message.
 * @return The length of the message in buffer[].
 */
int iam_encode_pdu(
    uint8_t * buffer,
    BACNET_ADDRESS * dest,
    BACNET_NPDU_DATA * npdu_data)
{
    int len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS my_address;
    datalink_get_my_address(&my_address);

    datalink_get_broadcast_address(dest);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(&buffer[0], dest, &my_address, npdu_data);

    /* encode the APDU portion of the packet */
    len =
        iam_encode_apdu(&buffer[pdu_len], Device_Object_Instance_Number(),
        MAX_APDU, SEGMENTATION_NONE, Device_Vendor_Identifier());
    pdu_len += len;

    return pdu_len;
}

/** Broadcast an I Am message.
 * @ingroup DMDDB
 *
 * @param buffer [in] The buffer to use for building and sending the message.
 */
void Send_I_Am(
    uint8_t * buffer)
{
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

#if 0
    /* note: there is discussion in the BACnet committee
       that we should allow a device to reply with I-Am
       so that dynamic binding always work.  If the DCC
       initiator loses the MAC address and routing info,
       they can never re-enable DCC because they can't
       find the device with WhoIs/I-Am */
    /* are we are forbidden to send? */
    if (!dcc_communication_enabled())
        return 0;
#endif

    /* encode the data */
    pdu_len = iam_encode_pdu(buffer, &dest, &npdu_data);
    /* send data */
    bytes_sent = datalink_send_pdu(&dest, &npdu_data, &buffer[0], pdu_len);

    if (bytes_sent <= 0) {
#if PRINT_ENABLED
        fprintf(stderr, "Failed to Send I-Am Reply (%s)!\n", strerror(errno));
#endif
    }
}

/** Encode an I Am message to be unicast directly back to the src.
 *
 * @param buffer [in,out] The buffer to use for building the message.
 * @param src [in] The source address information. If the src address is not
 *                 given, the dest address will be a broadcast address.
 * @param dest [out] The destination address information.
 * @param npdu_data [out] The NPDU structure describing the message.
 * @return The length of the message in buffer[].
 */
int iam_unicast_encode_pdu(
    uint8_t * buffer,
    BACNET_ADDRESS * src,
    BACNET_ADDRESS * dest,
    BACNET_NPDU_DATA * npdu_data)
{
    int npdu_len = 0;
    int apdu_len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS my_address;
    /* The destination will be the same as the src, so copy it over. */
    bacnet_address_copy(dest, src);
    /* dest->net = 0; - no, must direct back to src->net to meet BTL tests */

    datalink_get_my_address(&my_address);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    npdu_len = npdu_encode_pdu(&buffer[0], dest, &my_address, npdu_data);
    /* encode the APDU portion of the packet */
    apdu_len =
        iam_encode_apdu(&buffer[npdu_len], Device_Object_Instance_Number(),
        MAX_APDU, SEGMENTATION_NONE, Device_Vendor_Identifier());
    pdu_len = npdu_len + apdu_len;

    return pdu_len;
}

/** Send an I-Am message by unicasting directly back to the src.
 * @ingroup DMDDB
 * @note As of Addendum 135-2008q-1, unicast responses are allowed;
 *  in modern firewalled corporate networks, this may be the
 *  only type of response that will reach the source on
 *  another subnet (without using the BBMD).  <br>
 *  However, some BACnet routers may not correctly handle this message.
 *
 * @param buffer [in] The buffer to use for building and sending the message.
 * @param src [in] The source address information from service handler.
 */
void Send_I_Am_Unicast(
    uint8_t * buffer,
    BACNET_ADDRESS * src)
{
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

#if 0
    /* note: there is discussion in the BACnet committee
       that we should allow a device to reply with I-Am
       so that dynamic binding always work.  If the DCC
       initiator loses the MAC address and routing info,
       they can never re-enable DCC because they can't
       find the device with WhoIs/I-Am */
    /* are we are forbidden to send? */
    if (!dcc_communication_enabled())
        return 0;
#endif

    /* encode the data */
    pdu_len = iam_unicast_encode_pdu(buffer, src, &dest, &npdu_data);
    /* send data */
    bytes_sent = datalink_send_pdu(&dest, &npdu_data, &buffer[0], pdu_len);

    if (bytes_sent <= 0) {
#if PRINT_ENABLED
        fprintf(stderr, "Failed to Send I-Am Reply (%s)!\n", strerror(errno));
#endif
    }
}
