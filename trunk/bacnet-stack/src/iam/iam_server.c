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

/* encode I-Am service */
int iam_encode_apdu(uint8_t * apdu,
    uint32_t device_id,
    unsigned max_apdu, int segmentation, uint16_t vendor_id)
{
    int len = 0;                /* length of each encoding */
    int apdu_len = 0;           /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_I_AM;     /* service choice */
        apdu_len = 2;
        len = encode_application_object_id(&apdu[apdu_len],
            OBJECT_DEVICE, device_id);
        apdu_len += len;
        len = encode_application_unsigned(&apdu[apdu_len], max_apdu);
        apdu_len += len;
        len = encode_application_enumerated(&apdu[apdu_len], segmentation);
        apdu_len += len;
        len = encode_application_unsigned(&apdu[apdu_len], vendor_id);
        apdu_len += len;
    }

    return apdu_len;
}
