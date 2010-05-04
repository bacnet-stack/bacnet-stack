/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2006 Steve Karg

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
#ifndef REINITIALIZE_DEVICE_H
#define REINITIALIZE_DEVICE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct BACnet_Reinitialize_Device_Data {
    BACNET_REINITIALIZED_STATE state;
    BACNET_CHARACTER_STRING password;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
} BACNET_REINITIALIZE_DEVICE_DATA;

typedef bool(
    *reinitialize_device_function) (
    BACNET_REINITIALIZE_DEVICE_DATA * rd_data);


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* encode service */
    int rd_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_REINITIALIZED_STATE state,
        BACNET_CHARACTER_STRING * password);

/* decode the service request only */
    int rd_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_REINITIALIZED_STATE * state,
        BACNET_CHARACTER_STRING * password);

#ifdef TEST
#include "ctest.h"
    int rd_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_len,
        uint8_t * invoke_id,
        BACNET_REINITIALIZED_STATE * state,
        BACNET_CHARACTER_STRING * password);

    void test_ReinitializeDevice(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
/** @defgroup DMRD Device Management-ReinitializeDevice (DM-RD)
 * @ingroup RDMS
 * 16.4 ReinitializeDevice Service <br>
 * The ReinitializeDevice service is used by a client BACnet-user to instruct 
 * a remote device to reboot itself (cold start), reset itself to some 
 * predefined initial state (warm start), or to control the backup or restore 
 * procedure. Resetting or rebooting a device is primarily initiated by a human 
 * operator for diagnostic purposes. Use of this service during the backup or 
 * restore procedure is usually initiated on behalf of the user by the device 
 * controlling the backup or restore. Due to the sensitive nature of this 
 * service, a password may be required from the responding BACnet-user prior 
 * to executing the service.
 * 
 */
#endif
