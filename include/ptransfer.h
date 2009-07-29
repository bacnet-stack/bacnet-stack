/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2009 Steve Karg

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
#ifndef PRIVATE_TRANSFER_H
#define PRIVATE_TRANSFER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct BACnet_Private_Transfer_Data {
    uint16_t vendorID;
    uint32_t serviceNumber;
    uint8_t *serviceParameters;
    int serviceParametersLen;
} BACNET_PRIVATE_TRANSFER_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    int ptransfer_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_PRIVATE_TRANSFER_DATA * private_data);
    int ptransfer_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_PRIVATE_TRANSFER_DATA * private_data);

    int ptransfer_error_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_ERROR_CLASS error_class,
        BACNET_ERROR_CODE error_code,
        BACNET_PRIVATE_TRANSFER_DATA * private_data);
    int ptransfer_error_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_ERROR_CLASS * error_class,
        BACNET_ERROR_CODE * error_code,
        BACNET_PRIVATE_TRANSFER_DATA * private_data);

    int ptransfer_ack_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_PRIVATE_TRANSFER_DATA * private_data);
/* ptransfer_ack_decode_service_request() is the same as
       ptransfer_decode_service_request */

#ifdef TEST
#include "ctest.h"
    void test_Private_Transfer_Request(
        Test * pTest);
    void test_Private_Transfer_Ack(
        Test * pTest);
    void test_Private_Transfer_Error(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
