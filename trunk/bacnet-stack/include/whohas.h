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
#ifndef WHOHAS_H
#define WHOHAS_H

#include <stdint.h>
#include <stdbool.h>
#include "bacstr.h"

typedef struct BACnet_Who_Has_Data {
    int32_t low_limit;  /* deviceInstanceRange */
    int32_t high_limit;
    bool is_object_name;   /* true if a string */
    union {
        BACNET_OBJECT_ID identifier;
        BACNET_CHARACTER_STRING name;
    } object;
} BACNET_WHO_HAS_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* encode service  - use -1 for limit if you want unlimited */
    int whohas_encode_apdu(
        uint8_t * apdu,
        BACNET_WHO_HAS_DATA * data);

    int whohas_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_WHO_HAS_DATA * data);

    int whohas_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_WHO_HAS_DATA * data);

#ifdef TEST
#include "ctest.h"
    void testWhoHas(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
/** @defgroup DMDOB Device Management-Dynamic Object Binding (DM-DOB)
 * @ingroup RDMS
 * 16.9 Who-Has and I-Have Services <br>
 * The Who-Has service is used by a sending BACnet-user to identify the device
 * object identifiers and network addresses of other BACnet devices whose local
 * databases contain an object with a given Object_Name or a given Object_Identifier.
 * The I-Have service is used to respond to Who-Has service requests or to
 * advertise the existence of an object with a given Object_Name or
 * Object_Identifier. The I-Have service request may be issued at any time and
 * does not need to be preceded by the receipt of a Who-Has service request.
 * The Who-Has and I-Have services are unconfirmed services.
 *
 */
#endif
