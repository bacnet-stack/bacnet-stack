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
#include <stdint.h>
#include "bacenum.h"
#include "bacdef.h"
#include "npdu.h"
#include "dcc.h"
#include "datalink.h"
#include "device.h"
#include "bacdcode.h"
#include "address.h"
#include "iam.h"

int iam_send(uint8_t * buffer)
{
    int len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

    /* if we are forbidden to send, don't send! */
    if (!dcc_communication_enabled())
        return 0;

    /* I-Am is a global broadcast */
    datalink_get_broadcast_address(&dest);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(&buffer[0], &dest, NULL, &npdu_data);
    /* encode the APDU portion of the packet */
    len = iam_encode_apdu(&buffer[pdu_len],
        Device_Object_Instance_Number(),
        MAX_APDU, SEGMENTATION_NONE, Device_Vendor_Identifier());
    pdu_len += len;
    /* send data */
    bytes_sent = datalink_send_pdu(&dest, &npdu_data, &buffer[0], pdu_len);

    return bytes_sent;
}
