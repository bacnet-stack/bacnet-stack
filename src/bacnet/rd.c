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

#include "bacnet/bacenum.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacdef.h"
#include "bacnet/rd.h"

/** @file rd.c  Encode/Decode Reinitialize Device APDUs */
#if BACNET_SVC_RD_A
/**
 * @brief Encode ReinitializeDevice-Request APDU
 *
 *  ReinitializeDevice-Request ::= SEQUENCE {
 *      reinitialized-state-of-device [0] ENUMERATED {
 *          coldstart (0),
 *          warmstart (1),
 *          start-backup (2),
 *          end-backup (3),
 *          start-restore (4),
 *          end-restore (5),
 *          abort-restore (6),
 *          activate-changes (7)
 *      },
 *      password [1] CharacterString (SIZE (1..20)) OPTIONAL
 * }
 *
 * @param apdu  Pointer to the buffer, or NULL for length
 * @param state  Reinitialization state
 * @param password  Pointer to the pass phrase.
 * @return number of bytes encoded
 */
int reinitialize_device_encode(uint8_t *apdu,
    BACNET_REINITIALIZED_STATE state,
    BACNET_CHARACTER_STRING *password)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    /* reinitialized-state-of-device [0] ENUMERATED */
    len = encode_context_enumerated(apdu, 0, state);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* password [1] CharacterString (SIZE (1..20)) OPTIONAL */
    if (password) {
        if ((password->length >= 1) && (password->length <= 20)) {
            len = encode_context_character_string(apdu, 1, password);
            apdu_len += len;
        }
    }

    return apdu_len;
}

/**
 * @brief Encode the COVNotification service request
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param state  Reinitialization state
 * @param password  Pointer to the pass phrase.
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t reinitialize_device_request_encode(uint8_t *apdu,
    size_t apdu_size,
    BACNET_REINITIALIZED_STATE state,
    BACNET_CHARACTER_STRING *password)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = reinitialize_device_encode(NULL, state, password);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = reinitialize_device_encode(apdu, state, password);
    }

    return apdu_len;
}

/**
 * @brief Encode Reinitialize Device service
 * @param apdu  Pointer to the APDU buffer.
 * @param invoke_id Invoke-Id
 * @param state  Reinitialization state
 * @param password  Pointer to the pass phrase.
 *
 * @return Bytes encoded.
 */
int rd_encode_apdu(uint8_t *apdu,
    uint8_t invoke_id,
    BACNET_REINITIALIZED_STATE state,
    BACNET_CHARACTER_STRING *password)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_REINITIALIZE_DEVICE;
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = reinitialize_device_encode(apdu, state, password);
    apdu_len += len;

    return apdu_len;
}
#endif

/**
 * @brief Decode Reinitialize Device service
 *
 *  ReinitializeDevice-Request ::= SEQUENCE {
 *      reinitialized-state-of-device [0] ENUMERATED {
 *          coldstart (0),
 *          warmstart (1),
 *          start-backup (2),
 *          end-backup (3),
 *          start-restore (4),
 *          end-restore (5),
 *          abort-restore (6),
 *          activate-changes (7)
 *      },
 *      password [1] CharacterString (SIZE (1..20)) OPTIONAL
 * }
 *
 * @param apdu  Pointer to the APDU buffer.
 * @param apdu_size Valid bytes in the buffer
 * @param state  Pointer to the Reinitialization state
 * @param password  Pointer to the pass phrase.
 *
 * @return number of bytes decoded, or #BACNET_STATUS_ERROR if malformed
 */
int rd_decode_service_request(uint8_t *apdu,
    unsigned apdu_size,
    BACNET_REINITIALIZED_STATE *state,
    BACNET_CHARACTER_STRING *password)
{
    int len = 0, apdu_len = 0;
    uint32_t value = 0;

    /* check for value pointers */
    if (apdu) {
        /* Tag 0: reinitializedStateOfDevice */
        len = bacnet_enumerated_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &value);
        if (len > 0) {
            if (state) {
                *state = (BACNET_REINITIALIZED_STATE)value;
            }
            apdu_len = len;
        } else {
            apdu_len = BACNET_STATUS_ERROR;
        }
        /* Tag 1: password - optional */
        if (apdu_len < apdu_size) {
            len = bacnet_character_string_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, 1, password);
            if (len > 0) {
                apdu_len += len;
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
        } else {
            characterstring_init_ansi(password, "");
        }
    }

    return apdu_len;
}
