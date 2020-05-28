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
#include <stdint.h>
#include "bacnet/bacenum.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacdef.h"
#include "bacnet/dcc.h"

/** @file dcc.c  Enable/Disable Device Communication Control (DCC) */

/* note: the disable and time are not expected to survive
   over a power cycle or reinitialization. */
/* note: time duration is given in Minutes, but in order to be accurate,
   we need to count down in seconds. */
/* infinite time duration is defined as 0 */
static uint32_t DCC_Time_Duration_Seconds = 0;
static BACNET_COMMUNICATION_ENABLE_DISABLE DCC_Enable_Disable =
    COMMUNICATION_ENABLE;
/* password is optionally supported */

/**
 * Returns, the network communications enable/disable status.
 *
 * @return BACnet communication status
 */
BACNET_COMMUNICATION_ENABLE_DISABLE dcc_enable_status(void)
{
    return DCC_Enable_Disable;
}

/**
 * Returns, if network communications is enabled.
 *
 * @return true, if communication has been enabled.
 */
bool dcc_communication_enabled(void)
{
    return (DCC_Enable_Disable == COMMUNICATION_ENABLE);
}

/**
 * When network communications are completely disabled,
 * only DeviceCommunicationControl and ReinitializeDevice APDUs
 * shall be processed and no messages shall be initiated.
 *
 * @return true, if communication has been disabled, false otherwise.
 */
bool dcc_communication_disabled(void)
{
    return (DCC_Enable_Disable == COMMUNICATION_DISABLE);
}

/**
 * When the initiation of communications is disabled,
 * all APDUs shall be processed and responses returned as
 * required and no messages shall be initiated with the
 * exception of I-Am requests, which shall be initiated only in
 * response to Who-Is messages. In this state, a device that
 * supports I-Am request initiation shall send one I-Am request
 * for any Who-Is request that is received if and only if
 * the Who-Is request does not contain an address range or
 * the device is included in the address range.
 *
 * @return true, if disabling initiation is set, false otherwise.
 */
bool dcc_communication_initiation_disabled(void)
{
    return (DCC_Enable_Disable == COMMUNICATION_DISABLE_INITIATION);
}

/**
 * Returns the time duration in seconds.
 * Note: 0 indicates either expired, or infinite duration.
 *
 * @return time in seconds
 */
uint32_t dcc_duration_seconds(void)
{
    return DCC_Time_Duration_Seconds;
}

/**
 * Called every second or so.  If more than one second,
 * then seconds should be the number of seconds to tick away.
 *
 * @param seconds  Time passed in seconds, since last call.
 */
void dcc_timer_seconds(uint32_t seconds)
{
    if (DCC_Time_Duration_Seconds) {
        if (DCC_Time_Duration_Seconds > seconds) {
            DCC_Time_Duration_Seconds -= seconds;
        } else {
            DCC_Time_Duration_Seconds = 0;
        }
        /* just expired - do something */
        if (DCC_Time_Duration_Seconds == 0) {
            DCC_Enable_Disable = COMMUNICATION_ENABLE;
        }
    }
}

/**
 * Set DCC status using duration.
 *
 * @param status Enable/disable communication
 * @param minutes  Duration in minutes
 *
 * @return true/false
 */
bool dcc_set_status_duration(
    BACNET_COMMUNICATION_ENABLE_DISABLE status, uint16_t minutes)
{
    bool valid = false;

    /* valid? */
    if (status < MAX_BACNET_COMMUNICATION_ENABLE_DISABLE) {
        DCC_Enable_Disable = status;
        if (status == COMMUNICATION_ENABLE) {
            DCC_Time_Duration_Seconds = 0;
        } else {
            DCC_Time_Duration_Seconds = minutes * 60;
        }
        valid = true;
    }

    return valid;
}

#if BACNET_SVC_DCC_A
/**
 * Encode service
 *
 * @param apdu  Pointer to the APDU buffer used for encoding.
 * @param invoke_id  Invoke-Id
 * @param timeDuration  Optional time duration in minutes.
 * @param enable_disable  Enable/disable communication
 * @param password  Pointer to an optional password.
 *
 * @return Bytes encoded or zero on an error.
 */
int dcc_encode_apdu(uint8_t *apdu,
    uint8_t invoke_id,
    uint16_t timeDuration, /* 0=optional */
    BACNET_COMMUNICATION_ENABLE_DISABLE enable_disable,
    BACNET_CHARACTER_STRING *password)
{ /* NULL=optional */
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL;
        apdu_len = 4;
        /* optional timeDuration */
        if (timeDuration) {
            len = encode_context_unsigned(&apdu[apdu_len], 0, timeDuration);
            apdu_len += len;
        }
        /* enable disable */
        len = encode_context_enumerated(&apdu[apdu_len], 1, enable_disable);
        apdu_len += len;
        /* optional password */
        if (password) {
            if ((password->length >= 1) && (password->length <= 20)) {
                len = encode_context_character_string(&apdu[apdu_len], 2, password);
                apdu_len += len;
            }
        }
    }

    return apdu_len;
}
#endif

/**
 * Decode the service request only
 *
 * @param apdu  Pointer to the received request.
 * @param apdu_len_max  Valid count of bytes in the buffer.
 * @param timeDuration  Pointer to the duration given in minutes [optional]
 * @param enable_disable  Pointer to the variable takingthe communication enable/disable.
 * @param password  Pointer to the password [optional]
 *
 * @return Bytes decoded.
 */
int dcc_decode_service_request(uint8_t *apdu,
    unsigned apdu_len_max,
    uint16_t *timeDuration,
    BACNET_COMMUNICATION_ENABLE_DISABLE *enable_disable,
    BACNET_CHARACTER_STRING *password)
{
    int apdu_len = 0;
    int len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    BACNET_UNSIGNED_INTEGER decoded_unsigned = 0;
    uint32_t decoded_enum = 0;

    if (apdu && apdu_len_max) {
        /* Tag 0: timeDuration, in minutes --optional-- */
        len = bacnet_unsigned_context_decode(
            &apdu[apdu_len], apdu_len_max - apdu_len, 0, &decoded_unsigned);
        if (len > 0) {
            apdu_len += len;
            if (decoded_unsigned <= UINT16_MAX) {
                if (timeDuration) {
                    *timeDuration = (uint16_t)decoded_unsigned;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else if (len < 0) {
            return BACNET_STATUS_ERROR;
        } else if (timeDuration) {
            /* zero indicates infinite duration and
               results in no timeout */
            *timeDuration = 0;
        }
        if ((unsigned)apdu_len < apdu_len_max) {
            /* Tag 1: enable_disable */
            len = bacnet_enumerated_context_decode(
                &apdu[apdu_len], apdu_len_max - apdu_len, 1, &decoded_enum);
            if (len > 0) {
                apdu_len += len;
                if (enable_disable) {
                    *enable_disable =
                        (BACNET_COMMUNICATION_ENABLE_DISABLE)decoded_enum;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        }
        if ((unsigned)apdu_len < apdu_len_max) {
            /* Tag 2: password --optional-- */
            if (!decode_is_context_tag(&apdu[apdu_len], 2)) {
                /* since this is the last value of the packet,
                   if there is data here it must be the specific
                   context tag number or result in an error */
                return BACNET_STATUS_ERROR;
            }
            len = bacnet_tag_number_and_value_decode(&apdu[apdu_len],
                apdu_len_max - apdu_len, &tag_number, &len_value_type);
            if (len > 0) {
                apdu_len += len;
                if ((unsigned)apdu_len < apdu_len_max) {
                    len = bacnet_character_string_decode(&apdu[apdu_len],
                        apdu_len_max - apdu_len, len_value_type, password);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                } else {
                    return BACNET_STATUS_ERROR;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else if (password) {
            /* no optional password - set to NULL */
            characterstring_init_ansi(password, NULL);
        }
    }

    return apdu_len;
}

#ifdef BAC_TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

int dcc_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    uint16_t *timeDuration,
    BACNET_COMMUNICATION_ENABLE_DISABLE *enable_disable,
    BACNET_CHARACTER_STRING *password)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST)
        return -1;
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
    if (apdu[3] != SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL)
        return -1;
    offset = 4;

    if (apdu_len > offset) {
        len = dcc_decode_service_request(&apdu[offset], apdu_len - offset,
            timeDuration, enable_disable, password);
    }

    return len;
}

void test_DeviceCommunicationControlData(Test *pTest,
    uint8_t invoke_id,
    uint16_t timeDuration,
    BACNET_COMMUNICATION_ENABLE_DISABLE enable_disable,
    BACNET_CHARACTER_STRING *password)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    uint8_t test_invoke_id = 0;
    uint16_t test_timeDuration = 0;
    BACNET_COMMUNICATION_ENABLE_DISABLE test_enable_disable;
    BACNET_CHARACTER_STRING test_password;

    len = dcc_encode_apdu(
        &apdu[0], invoke_id, timeDuration, enable_disable, password);
    ct_test(pTest, len != 0);
    apdu_len = len;

    len = dcc_decode_apdu(&apdu[0], apdu_len, &test_invoke_id,
        &test_timeDuration, &test_enable_disable, &test_password);
    ct_test(pTest, len != -1);
    ct_test(pTest, test_invoke_id == invoke_id);
    ct_test(pTest, test_timeDuration == timeDuration);
    ct_test(pTest, test_enable_disable == enable_disable);
    ct_test(pTest, characterstring_same(&test_password, password));
}

void test_DeviceCommunicationControl(Test *pTest)
{
    uint8_t invoke_id = 128;
    uint16_t timeDuration = 0;
    BACNET_COMMUNICATION_ENABLE_DISABLE enable_disable;
    BACNET_CHARACTER_STRING password;

    timeDuration = 0;
    enable_disable = COMMUNICATION_DISABLE_INITIATION;
    characterstring_init_ansi(&password, "John 3:16");
    test_DeviceCommunicationControlData(
        pTest, invoke_id, timeDuration, enable_disable, &password);

    timeDuration = 12345;
    enable_disable = COMMUNICATION_DISABLE;
    test_DeviceCommunicationControlData(
        pTest, invoke_id, timeDuration, enable_disable, NULL);

    return;
}

void test_DeviceCommunicationControlMalformedData(Test *pTest)
{
    /* payload with enable-disable, and password with wrong characterstring
     * length */
    uint8_t payload_1[] = { 0x19, 0x00, 0x2a, 0x00, 0x41 };
    /* payload with enable-disable, and password with wrong characterstring
     * length */
    uint8_t payload_2[] = { 0x19, 0x00, 0x2d, 0x55, 0x00, 0x66, 0x69, 0x73,
        0x74, 0x65, 0x72 };
    /* payload with enable-disable - wrong context tag number for password */
    uint8_t payload_3[] = { 0x19, 0x01, 0x3d, 0x09, 0x00, 0x66, 0x69, 0x73,
        0x74, 0x65, 0x72 };
    /* payload with duration, enable-disable, and password */
    uint8_t payload_4[] = { 0x00, 0x05, 0xf1, 0x11, 0x0a, 0x00, 0x19, 0x00,
        0x2d, 0x09, 0x00, 0x66, 0x69, 0x73, 0x74, 0x65, 0x72 };
    /* payload submitted with bug report */
    uint8_t payload_5[] = { 0x0d, 0xff, 0x80, 0x00, 0x03, 0x1a, 0x0a, 0x19,
        0x00, 0x2a, 0x00, 0x41 };
    int len = 0;
    uint8_t test_invoke_id = 0;
    uint16_t test_timeDuration = 0;
    BACNET_COMMUNICATION_ENABLE_DISABLE test_enable_disable;
    BACNET_CHARACTER_STRING test_password;

    len = dcc_decode_apdu(&payload_1[0], sizeof(payload_1), &test_invoke_id,
        &test_timeDuration, &test_enable_disable, &test_password);
    ct_test(pTest, len == BACNET_STATUS_ERROR);
    len = dcc_decode_apdu(&payload_2[0], sizeof(payload_2), &test_invoke_id,
        &test_timeDuration, &test_enable_disable, &test_password);
    ct_test(pTest, len == BACNET_STATUS_ERROR);
    len = dcc_decode_apdu(&payload_3[0], sizeof(payload_3), &test_invoke_id,
        &test_timeDuration, &test_enable_disable, &test_password);
    ct_test(pTest, len == BACNET_STATUS_ERROR);
    len = dcc_decode_apdu(&payload_4[0], sizeof(payload_4), &test_invoke_id,
        &test_timeDuration, &test_enable_disable, &test_password);
    ct_test(pTest, len == BACNET_STATUS_ERROR);
    len = dcc_decode_apdu(&payload_5[0], sizeof(payload_5), &test_invoke_id,
        &test_timeDuration, &test_enable_disable, &test_password);
    ct_test(pTest, len == BACNET_STATUS_ERROR);
}

#ifdef TEST_DEVICE_COMMUNICATION_CONTROL
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet DeviceCommunicationControl", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, test_DeviceCommunicationControl);
    assert(rc);
    rc =
        ct_addTestFunction(pTest, test_DeviceCommunicationControlMalformedData);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void)ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_DEVICE_COMMUNICATION_CONTROL */
#endif /* BAC_TEST */
