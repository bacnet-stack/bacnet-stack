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
#include <assert.h>
#include <string.h>
#include "ctest.h"

int iam_decode_apdu(uint8_t * apdu,
    uint32_t * pDevice_id,
    unsigned *pMax_apdu, int *pSegmentation, uint16_t * pVendor_id)
{
    int apdu_len = 0;           /* total length of the apdu, return value */

    /* valid data? */
    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST)
        return -1;
    if (apdu[1] != SERVICE_UNCONFIRMED_I_AM)
        return -1;
    apdu_len = iam_decode_service_request(&apdu[2],
        pDevice_id, pMax_apdu, pSegmentation, pVendor_id);

    return apdu_len;
}

void testIAm(Test * pTest)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    uint32_t device_id = 42;
    unsigned max_apdu = 480;
    int segmentation = SEGMENTATION_NONE;
    uint16_t vendor_id = 42;
    uint32_t test_device_id = 0;
    unsigned test_max_apdu = 0;
    int test_segmentation = 0;
    uint16_t test_vendor_id = 0;

    len = iam_encode_apdu(&apdu[0],
        device_id, max_apdu, segmentation, vendor_id);
    ct_test(pTest, len != 0);

    len = iam_decode_apdu(&apdu[0],
        &test_device_id,
        &test_max_apdu, &test_segmentation, &test_vendor_id);

    ct_test(pTest, len != -1);
    ct_test(pTest, test_device_id == device_id);
    ct_test(pTest, test_vendor_id == vendor_id);
    ct_test(pTest, test_max_apdu == max_apdu);
    ct_test(pTest, test_segmentation == segmentation);
}

#ifdef TEST_IAM
/* dummy function stubs */
int datalink_send_pdu(BACNET_ADDRESS * dest,
    BACNET_NPDU_DATA * npdu_data, uint8_t * pdu, unsigned pdu_len)
{
    (void) dest;
    (void) npdu_data;
    (void) pdu;
    (void) pdu_len;

    return 0;
}

void datalink_get_broadcast_address(BACNET_ADDRESS * dest)
{                               /* destination address */
    (void) dest;
}

uint16_t Device_Vendor_Identifier(void)
{
    return 0;
}

uint32_t Device_Object_Instance_Number(void)
{
    return 0;
}

void address_add_binding(uint32_t device_id,
    unsigned max_apdu, BACNET_ADDRESS * src)
{
    (void) device_id;
    (void) max_apdu;
    (void) src;
}

/* dummy for apdu dependency */
void tsm_free_invoke_id(uint8_t invokeID)
{
    /* dummy stub for testing */
    (void) invokeID;
}


int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet I-Am", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testIAm);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_IAM */
