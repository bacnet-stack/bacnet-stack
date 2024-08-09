/**
 * @file
 * @brief BACnet DeviceCommunicationControl encode and decode functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/dcc.h"

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
 * @brief Encode DeviceCommunicationControl service
 * @param apdu  Pointer to the APDU buffer used for encoding.
 * @param timeDuration  Optional time duration in minutes.
 * @param enable_disable  Enable/disable communication
 * @param password  Pointer to an optional password.
 *
 * @return Bytes encoded or zero on an error.
 */
int dcc_apdu_encode(uint8_t *apdu,
    uint16_t timeDuration,
    BACNET_COMMUNICATION_ENABLE_DISABLE enable_disable,
    const BACNET_CHARACTER_STRING *password)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    /* optional timeDuration */
    if (timeDuration) {
        len = encode_context_unsigned(apdu, 0, timeDuration);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    /* enable disable */
    len = encode_context_enumerated(apdu, 1, enable_disable);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* optional password */
    if (password) {
        if ((password->length >= 1) && (password->length <= 20)) {
            len = encode_context_character_string(apdu, 2, password);
            apdu_len += len;
        }
    }

    return apdu_len;
}

/**
 * @brief Encode the COVNotification service request
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param timeDuration  Optional time duration in minutes.
 * @param enable_disable  Enable/disable communication
 * @param password  Pointer to an optional password.
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t dcc_service_request_encode(uint8_t *apdu,
    size_t apdu_size,
    uint16_t timeDuration,
    BACNET_COMMUNICATION_ENABLE_DISABLE enable_disable,
    const BACNET_CHARACTER_STRING *password)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = dcc_apdu_encode(NULL, timeDuration, enable_disable, password);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len =
            dcc_apdu_encode(apdu, timeDuration, enable_disable, password);
    }

    return apdu_len;
}

/**
 * Encode service
 *
 * @param apdu  Pointer to the APDU buffer used for encoding.
 * @param invoke_id  Invoke-Id
 * @param timeDuration  Optional time duration in minutes, 0=optional
 * @param enable_disable  Enable/disable communication
 * @param password  Pointer to an optional password, NULL=optional
 * @return Bytes encoded or zero on an error.
 */
int dcc_encode_apdu(uint8_t *apdu,
    uint8_t invoke_id,
    uint16_t timeDuration,
    BACNET_COMMUNICATION_ENABLE_DISABLE enable_disable,
    const BACNET_CHARACTER_STRING *password)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL;
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = dcc_apdu_encode(
        apdu, timeDuration, enable_disable, password);
    apdu_len += len;

    return apdu_len;
}
#endif

/**
 * Decode the service request only
 *
 * @param apdu  Pointer to the received request.
 * @param apdu_len_max  Valid count of bytes in the buffer.
 * @param timeDuration  Pointer to the duration given in minutes [optional]
 * @param enable_disable  Pointer to the variable takingthe communication
 * enable/disable.
 * @param password  Pointer to the password [optional]
 *
 * @return Bytes decoded, or BACNET_STATUS_ABORT or BACNET_STATUS_REJECT
 */
int dcc_decode_service_request(const uint8_t *apdu,
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
    uint32_t password_length = 0;

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
                return BACNET_STATUS_REJECT;
            }
        } else if (len < 0) {
            return BACNET_STATUS_ABORT;
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
                return BACNET_STATUS_ABORT;
            }
        }
        if ((unsigned)apdu_len < apdu_len_max) {
            /* Tag 2: password --optional-- */
            if (!decode_is_context_tag(&apdu[apdu_len], 2)) {
                /* since this is the last value of the packet,
                   if there is data here it must be the specific
                   context tag number or result in an error */
                return BACNET_STATUS_ABORT;
            }
            len = bacnet_tag_number_and_value_decode(&apdu[apdu_len],
                apdu_len_max - apdu_len, &tag_number, &len_value_type);
            if (len > 0) {
                apdu_len += len;
                if ((unsigned)apdu_len < apdu_len_max) {
                    len = bacnet_character_string_decode(&apdu[apdu_len],
                        apdu_len_max - apdu_len, len_value_type, password);
                    if (len > 0) {
                        password_length = len_value_type - 1;
                        if ((password_length >= 1) && (password_length <= 20)) {
                            apdu_len += len;
                        } else {
                            return BACNET_STATUS_REJECT;
                        }
                    } else {
                        return BACNET_STATUS_ABORT;
                    }
                } else {
                    return BACNET_STATUS_ABORT;
                }
            } else {
                return BACNET_STATUS_ABORT;
            }
        } else if (password) {
            /* no optional password - set to NULL */
            characterstring_init_ansi(password, NULL);
        }
    }

    return apdu_len;
}
