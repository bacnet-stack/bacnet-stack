/**
 * @file
 * @brief BACnetDestination complex data type encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2022
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacaddr.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacdest.h"
#include "bacnet/basic/binding/address.h"

/**
 * @brief Initialize the BACnetDestination data structure with defaults
 *
 *  BACnetDestination ::= SEQUENCE {
 *      valid-days                      BACnetDaysOfWeek,
 *      from-time                       Time,
 *      to-time                         Time,
 *      recipient                       BACnetRecipient,
 *      process-identifier              Unsigned32,
 *      issue-confirmed-notifications   BOOLEAN,
 *      transitions                     BACnetEventTransitionBits
 *  }
 *
 * @param destination  BACnetDestination to be initialized
 */
void bacnet_destination_default_init(BACNET_DESTINATION *destination)
{
    unsigned i;

    if (!destination) {
        return;
    }
    /* configure for every day, all day long */
    for (i = 0; i < MAX_BACNET_DAYS_OF_WEEK; i++) {
        bitstring_set_bit(&destination->ValidDays, i, true);
    }
    datetime_set_time(&destination->FromTime, 0, 0, 0, 0);
    datetime_set_time(&destination->ToTime, 23, 59, 59, 99);
    /* initialize Recipient to *wildcard* device instance - invalid! */
    destination->Recipient.tag = BACNET_RECIPIENT_TAG_DEVICE;
    destination->Recipient.type.device.type = OBJECT_DEVICE;
    destination->Recipient.type.device.instance = BACNET_MAX_INSTANCE;
    destination->ProcessIdentifier = 0;
    destination->ConfirmedNotify = false;
    bitstring_set_bit(
        &destination->Transitions, TRANSITION_TO_OFFNORMAL, false);
    bitstring_set_bit(&destination->Transitions, TRANSITION_TO_FAULT, false);
    bitstring_set_bit(&destination->Transitions, TRANSITION_TO_NORMAL, false);
}

/**
 * @brief Compare the BACnetRecipient complex data of r1 and r2
 * @param r1 - BACnetRecipient 1 structure
 * @param r2 - BACnetRecipient 2 structure
 * @return true if r1 and r2 are the same
 */
bool bacnet_recipient_same(
    const BACNET_RECIPIENT *r1, const BACNET_RECIPIENT *r2)
{
    bool status = false;

    if (r1 && r2) {
        if (r1->tag == r2->tag) {
            status = true;
        }
        if (status) {
            if (r1->tag == BACNET_RECIPIENT_TAG_DEVICE) {
                if ((r1->type.device.type == r2->type.device.type) &&
                    (r1->type.device.instance == r2->type.device.instance)) {
                    status = true;
                }
            } else if (r1->tag == BACNET_RECIPIENT_TAG_ADDRESS) {
                status =
                    bacnet_address_same(&r1->type.address, &r2->type.address);
            } else {
                status = false;
            }
        }
    }

    return status;
}

/**
 * @brief Copy the BACnetRecipient complex data from src to dest
 * @param src - BACnetRecipient 1 structure
 * @param dest - BACnetRecipient 2 structure
 */
void bacnet_recipient_copy(BACNET_RECIPIENT *dest, const BACNET_RECIPIENT *src)
{
    if (dest && src) {
        memmove(dest, src, sizeof(BACNET_RECIPIENT));
    }
}

/**
 * @brief Compare the BACnetRecipient data structure device object wildcard
 * @param recipient - BACnetRecipient structure
 * @return true if BACnetRecipient is equal to the device object wildcard
 */
bool bacnet_recipient_device_wildcard(const BACNET_RECIPIENT *recipient)
{
    bool status = false;

    if (recipient) {
        if ((recipient->tag == BACNET_RECIPIENT_TAG_DEVICE) &&
            (recipient->type.device.type == OBJECT_DEVICE) &&
            (recipient->type.device.instance == BACNET_MAX_INSTANCE)) {
            status = true;
        }
    }

    return status;
}

/**
 * @brief Compare the BACnetRecipient data structure to a valid device
 * @param recipient - BACnetRecipient structure
 * @return true if BACnetRecipient is a valid device object instance
 */
bool bacnet_recipient_device_valid(const BACNET_RECIPIENT *recipient)
{
    bool status = false;

    if (recipient) {
        if ((recipient->tag == BACNET_RECIPIENT_TAG_DEVICE) &&
            (recipient->type.device.type == OBJECT_DEVICE) &&
            (recipient->type.device.instance < BACNET_MAX_INSTANCE)) {
            status = true;
        }
    }

    return status;
}

/**
 * @brief Compare the BACnetDestination complex data of dest1 and dest2
 * @param d1 - BACnetDestination 1 structure
 * @param d2 - BACnetDestination 2 structure
 * @return true if dest1 and dest2 are the same
 */
bool bacnet_destination_same(
    const BACNET_DESTINATION *d1, const BACNET_DESTINATION *d2)
{
    bool status = false;

    if (d1 && d2) {
        status = bitstring_same(&d1->ValidDays, &d2->ValidDays);
        if (status) {
            status = (datetime_compare_time(&d1->FromTime, &d2->FromTime) == 0);
        }
        if (status) {
            status = (datetime_compare_time(&d1->ToTime, &d2->ToTime) == 0);
        }
        if (status) {
            status = bacnet_recipient_same(&d1->Recipient, &d2->Recipient);
        }
        if (status) {
            status = (d1->ProcessIdentifier == d2->ProcessIdentifier);
        }
        if (status) {
            status = (d1->ConfirmedNotify == d2->ConfirmedNotify);
        }
        if (status) {
            status = bitstring_same(&d1->Transitions, &d2->Transitions);
        }
    }

    return status;
}

/**
 * @brief Copy the BACnetDestination complex data from src to dest
 * @param dest - BACnetDestination 1 structure
 * @param src - BACnetDestination 2 structure
 */
void bacnet_destination_copy(
    BACNET_DESTINATION *dest, const BACNET_DESTINATION *src)
{
    if (dest && src) {
        memmove(dest, src, sizeof(BACNET_DESTINATION));
    }
}

/**
 * @brief Compare the BACnetDestination data structure to defaults
 * @param d1 - BACnetDestination 1 structure
 * @return true if d1 and d2 (defaults) are the same
 */
bool bacnet_destination_default(const BACNET_DESTINATION *d1)
{
    BACNET_DESTINATION d2 = { 0 };

    bacnet_destination_default_init(&d2);

    return bacnet_destination_same(d1, &d2);
}

/**
 * @brief Encode the BACnetDestination complex data
 *
 *  BACnetDestination ::= SEQUENCE {
 *      valid-days                      BACnetDaysOfWeek,
 *      from-time                       Time,
 *      to-time                         Time,
 *      recipient                       BACnetRecipient,
 *      process-identifier              Unsigned32,
 *      issue-confirmed-notifications   BOOLEAN,
 *      transitions                     BACnetEventTransitionBits
 *  }
 *
 * @param apdu  Pointer to the buffer for encoding.
 * @param destination  Pointer to the property data to be encoded.
 *
 * @return bytes encoded or zero on error.
 */
int bacnet_destination_encode(
    uint8_t *apdu, const BACNET_DESTINATION *destination)
{
    int apdu_len = 0, len = 0;

    if (destination->Recipient.tag < BACNET_RECIPIENT_TAG_MAX) {
        len = encode_application_bitstring(apdu, &destination->ValidDays);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_application_time(apdu, &destination->FromTime);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_application_time(apdu, &destination->ToTime);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        if (destination->Recipient.tag == BACNET_RECIPIENT_TAG_DEVICE) {
            len = encode_context_object_id(
                apdu, 0, OBJECT_DEVICE,
                destination->Recipient.type.device.instance);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
        } else if (destination->Recipient.tag == BACNET_RECIPIENT_TAG_ADDRESS) {
            /* opening tag 1 */
            len = encode_opening_tag(apdu, 1);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_bacnet_address(
                apdu, &destination->Recipient.type.address);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            /* closing tag 1 */
            len = encode_closing_tag(apdu, 1);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
        }
        /* Process Identifier - Unsigned32 */
        len = encode_application_unsigned(apdu, destination->ProcessIdentifier);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* Issue Confirmed Notifications - boolean */
        len = encode_application_boolean(apdu, destination->ConfirmedNotify);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* Transitions - BACnet Event Transition Bits [bitstring] */
        len = encode_application_bitstring(apdu, &destination->Transitions);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode a BACnetDestination complex data type
 * @param apdu - the APDU buffer
 * @param tag_number - context tag number
 * @param destination  Pointer to the property data to be encoded.
 * @return length of the APDU buffer, or 0 if not able to encode
 */
int bacnet_destination_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_DESTINATION *destination)
{
    int len = 0;
    int apdu_len = 0;

    if (destination) {
        len = encode_opening_tag(apdu, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = bacnet_destination_encode(apdu, destination);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_closing_tag(apdu, tag_number);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Decode the BACnetDestination complex data
 *
 *  BACnetDestination ::= SEQUENCE {
 *      valid-days                      BACnetDaysOfWeek,
 *      from-time                       Time,
 *      to-time                         Time,
 *      recipient                       BACnetRecipient,
 *      process-identifier              Unsigned32,
 *      issue-confirmed-notifications   BOOLEAN,
 *      transitions                     BACnetEventTransitionBits
 *  }
 *
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  Count of valid bytes in the buffer.
 * @param destination  Pointer to the property data to be encoded.
 *
 * @return bytes encoded or #BACNET_STATUS_REJECT on error.
 */
int bacnet_destination_decode(
    const uint8_t *apdu, int apdu_size, BACNET_DESTINATION *destination)
{
    int len = 0, apdu_len = 0;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };

    if (!apdu) {
        return BACNET_STATUS_REJECT;
    }
    if (!destination) {
        return BACNET_STATUS_REJECT;
    }
    /* Decode Valid Days */
    len = bacapp_decode_application_data(apdu, apdu_size, &value);
    if ((len == 0) || (len == BACNET_STATUS_ERROR) ||
        (value.tag != BACNET_APPLICATION_TAG_BIT_STRING)) {
        return BACNET_STATUS_REJECT;
    }
    bitstring_copy(&destination->ValidDays, &value.type.Bit_String);
    apdu_len += len;
    apdu += len;
    /* Decode From Time */
    len = bacapp_decode_application_data(apdu, apdu_size, &value);
    if ((len == 0) || (len == BACNET_STATUS_ERROR) ||
        (value.tag != BACNET_APPLICATION_TAG_TIME)) {
        return BACNET_STATUS_REJECT;
    }
    /* store value */
    datetime_copy_time(&destination->FromTime, &value.type.Time);
    apdu_len += len;
    apdu += len;
    /* Decode To Time */
    len = bacapp_decode_application_data(apdu, apdu_size, &value);
    if ((len == 0) || (len == BACNET_STATUS_ERROR) ||
        (value.tag != BACNET_APPLICATION_TAG_TIME)) {
        return BACNET_STATUS_REJECT;
    }
    /* store value */
    datetime_copy_time(&destination->ToTime, &value.type.Time);
    apdu_len += len;
    apdu += len;
    if (decode_is_context_tag(apdu, BACNET_RECIPIENT_TAG_DEVICE)) {
        /* device [0] BACnetObjectIdentifier */
        destination->Recipient.tag = BACNET_RECIPIENT_TAG_DEVICE;
        len = decode_context_object_id(
            apdu, BACNET_RECIPIENT_TAG_DEVICE,
            &destination->Recipient.type.device.type,
            &destination->Recipient.type.device.instance);
        if (len == BACNET_STATUS_ERROR) {
            return BACNET_STATUS_REJECT;
        }
        if (destination->Recipient.type.device.type != OBJECT_DEVICE) {
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;
        apdu += len;
    } else if (decode_is_opening_tag_number(
                   apdu, BACNET_RECIPIENT_TAG_ADDRESS)) {
        /* address [1] BACnetAddress */
        destination->Recipient.tag = BACNET_RECIPIENT_TAG_ADDRESS;
        /* opening tag [1] is len 1 */
        len = 1;
        apdu_len += len;
        apdu += len;
        len = decode_bacnet_address(apdu, &destination->Recipient.type.address);
        if ((len == 0) || (len == BACNET_STATUS_ERROR)) {
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;
        apdu += len;
        /* closing tag [1] */
        if (decode_is_closing_tag_number(apdu, BACNET_RECIPIENT_TAG_ADDRESS)) {
            /* closing tag [1] is len 1 */
            len = 1;
            apdu_len += len;
            apdu += len;
        } else {
            return BACNET_STATUS_REJECT;
        }
    } else {
        return BACNET_STATUS_REJECT;
    }
    /* Process Identifier */
    len = bacapp_decode_application_data(apdu, apdu_size, &value);
    if ((len == 0) || (len == BACNET_STATUS_ERROR) ||
        (value.tag != BACNET_APPLICATION_TAG_UNSIGNED_INT)) {
        return BACNET_STATUS_REJECT;
    }
    /* store value */
    destination->ProcessIdentifier = value.type.Unsigned_Int;
    apdu_len += len;
    apdu += len;
    /* Issue Confirmed Notifications */
    len = bacapp_decode_application_data(apdu, apdu_size, &value);
    if ((len == 0) || (len == BACNET_STATUS_ERROR) ||
        (value.tag != BACNET_APPLICATION_TAG_BOOLEAN)) {
        return BACNET_STATUS_REJECT;
    }
    /* store value */
    destination->ConfirmedNotify = value.type.Boolean;
    apdu_len += len;
    apdu += len;
    /* Transitions */
    len = bacapp_decode_application_data(apdu, apdu_size, &value);
    if ((len == 0) || (len == BACNET_STATUS_ERROR) ||
        (value.tag != BACNET_APPLICATION_TAG_BIT_STRING)) {
        return BACNET_STATUS_REJECT;
    }
    /* store value */
    bitstring_copy(&destination->Transitions, &value.type.Bit_String);
    apdu_len += len;

    return apdu_len;
}

/**
 * Convert BACnet_Destination to ASCII for printing
 *
 * Output format:
 *
 * (
 *  ValidDays=[1,2,5,6,7];
 *  FromTime=0:00:00.0;
 *  ToTime=23:59:59.9;
 *  Recipient=Device(type=8,instance=15);
 *  ProcessIdentifier=0;
 *  ConfirmedNotify=false;
 *  Transitions=[to-offnormal,to-fault,to-normal]
 * )
 *
 * - ValidDays ... array of numbers, 1=Mon through 7=Sun
 * - FromTime, ToTime ... HH:MM:SS.s
 * - Recipient ... two variants: Recipient=Device(type=8,instance=15) or
 * Recipient=Address(net=1234,mac=c0:a8:00:0f)
 *   - type ... bacnet object type enum
 *   - instance ... bacnet object instance
 *   - net ... bacnet network number
 *   - mac ... bacnet MAC address; can be separated by colons or periods.
 * - ProcessIdentifier ... 32bit unsigned int, process ID
 * - ConfirmedNotify ... true or false
 * - Transitions ... array with any of the three items: to-offnormal, to-fault,
 * to-normal
 *
 * @param bacdest - Destination struct to convert to ASCII
 * @param buf - ASCII output buffer
 * @param buf_size - ASCII output buffer capacity
 *
 * @return the number of characters which would be generated for the given
 *  input, excluding the trailing null.
 * @note buf and buf_size may be null and zero to return only the size
 */
int bacnet_destination_to_ascii(
    const BACNET_DESTINATION *bacdest, char *buf, size_t buf_size)
{
    int len = 0;
    int buf_len = 0;
    bool comma;
    int i;

    len = snprintf(buf, buf_size, "(");
    buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
    /*
     BACnetDaysOfWeek ::= BIT STRING {
         monday     (0),
         tuesday    (1),
         wednesday  (2),
         thursday   (3),
         friday     (4),
         saturday   (5),
         sunday     (6)
     }
    */
    /* Use numbers 1-7 (ISO 8601) */
    len = snprintf(buf, buf_size, "ValidDays=[");
    buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
    comma = false;
    for (i = 0; i < 7; i++) {
        if (bitstring_bit(&bacdest->ValidDays, i)) {
            if (comma) {
                len = snprintf(buf, buf_size, ",");
                buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
            }
            len = snprintf(buf, buf_size, "%d", i + 1);
            buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
            comma = true;
        }
    }
    len = snprintf(buf, buf_size, "];");
    buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
    len = snprintf(
        buf, buf_size, "FromTime=%d:%02d:%02d.%02d;", bacdest->FromTime.hour,
        bacdest->FromTime.min, bacdest->FromTime.sec,
        bacdest->FromTime.hundredths);
    buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
    len = snprintf(
        buf, buf_size, "ToTime=%d:%02d:%02d.%02d;", bacdest->ToTime.hour,
        bacdest->ToTime.min, bacdest->ToTime.sec, bacdest->ToTime.hundredths);
    buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
    len = snprintf(buf, buf_size, "Recipient=");
    buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
    if (bacdest->Recipient.tag == BACNET_RECIPIENT_TAG_DEVICE) {
        len = snprintf(
            buf, buf_size, "Device(type=%d,instance=%lu)",
            bacdest->Recipient.type.device.type,
            (unsigned long)bacdest->Recipient.type.device.instance);
        buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
    } else {
        /*
          BACnetAddress ::= SEQUENCE {
              network-number Unsigned16, -- A value of 0 indicates the local
          network mac-address    OCTET STRING -- A string of length 0 indicates
          a broadcast
          }
        */
        len = snprintf(
            buf, buf_size,
            "Address(net=%d,mac=", bacdest->Recipient.type.address.net);
        buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);

        /* TODO determine if it's IPv4+port or Ethernet mac address and print it
         * nicer - how? Both are 6 bytes long. */

        for (i = 0; i < bacdest->Recipient.type.address.mac_len; i++) {
            if (i > 0) {
                len = snprintf(buf, buf_size, ":");
                buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
            }
            len = snprintf(
                buf, buf_size, "%02x", bacdest->Recipient.type.address.mac[i]);
            buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
        }
        len = snprintf(buf, buf_size, ")");
        buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
    }
    len = snprintf(buf, buf_size, ";");
    buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);

    len = snprintf(
        buf, buf_size, "ProcessIdentifier=%lu;",
        (unsigned long)bacdest->ProcessIdentifier);
    buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);

    len = snprintf(
        buf, buf_size, "ConfirmedNotify=%s;",
        bacdest->ConfirmedNotify ? "true" : "false");
    buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);

    /*
     BACnetEventTransitionBits ::= BIT STRING {
         to-offnormal (0),
         to-fault     (1),
         to-normal    (2)
     }
    */
    len = snprintf(buf, buf_size, "Transitions=[");
    buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);

    comma = false;
    /* TODO remove casting when bitstring_bit() has const added - Github issue
     * #320 */
    if (bitstring_bit(&bacdest->Transitions, TRANSITION_TO_OFFNORMAL)) {
        len = snprintf(buf, buf_size, "to-offnormal");
        buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
        comma = true;
    }
    if (bitstring_bit(&bacdest->Transitions, TRANSITION_TO_FAULT)) {
        if (comma) {
            len = snprintf(buf, buf_size, ",");
            buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
        }
        len = snprintf(buf, buf_size, "to-fault");
        buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
        comma = true;
    }
    if (bitstring_bit(&bacdest->Transitions, TRANSITION_TO_NORMAL)) {
        if (comma) {
            len = snprintf(buf, buf_size, ",");
            buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
        }
        len = snprintf(buf, buf_size, "to-normal");
        buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);
    }
    len = snprintf(buf, buf_size, "])"); /* end of the outer paren */
    buf_len += bacapp_snprintf_shift(len, &buf, &buf_size);

    return buf_len;
}

/**
 * Parse BACnet_Destination from ASCII string (as entered by user)
 *
 * @param bacdest - Destination struct to pupulate with data from the ASCII
 * string
 * @param buf - ASCII string, zero terminated
 * @return true on success
 */
bool bacnet_destination_from_ascii(BACNET_DESTINATION *bacdest, const char *buf)
{
    enum ParsePhase {
        PH_START,
        PH_PAIR_SPACER,
        PH_KEYWORD,
        PH_VALUE_SPACER,
        PH_VALUE
    };
    enum ParseKeyword {
        KW_ValidDays = 0,
        KW_FromTime,
        KW_ToTime,
        KW_Recipient,
        KW_ProcessIdentifier,
        KW_ConfirmedNotify,
        KW_Transitions,
        KW_MAX
    };
    enum ParsePhase ph;
    size_t buflen;
    size_t toklen;
    size_t pos;
    char c;
    int i;
    int j;
    int _number_i;
    size_t _must_consume_tmplen;
    uint32_t tmp;
    enum ParseKeyword kw = 0;
    BACNET_TIME *ptime;
    BACNET_MAC_ADDRESS tmpmac;
    static const char *KW_LOOKUP[] = {
        "ValidDays",         "FromTime",        "ToTime",      "Recipient",
        "ProcessIdentifier", "ConfirmedNotify", "Transitions",
    };

    if (bacdest == NULL || buf == NULL) {
        return false;
    }

    bacnet_destination_default_init(bacdest);

    /* Helper macros to simplify the parser ... */

/* true if the character is whitespace */
#define ISWHITE(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')

/* Discard characters while they match a given test. Goes to parse_end on NUL.
 * ctest is a boolean expression where c is the tested character */
#define DISCARD_WHILE(ctest)    \
    do {                        \
        while (1) {             \
            c = buf[pos];       \
            if (c == 0) {       \
                goto parse_end; \
            }                   \
            if ((ctest)) {      \
                pos++;          \
                continue;       \
            }                   \
            break;              \
        }                       \
    } while (0)

/* Discard all whitespace. Goes to parse_end on NUL. */
#define DISCARD_WHITESPACE() DISCARD_WHILE(ISWHITE(c))

/* Must consume a given word; return false otherwise. */
#define MUST_CONSUME(s)                                         \
    do {                                                        \
        _must_consume_tmplen = strlen(s);                       \
        if (0 == strncmp(&buf[pos], s, _must_consume_tmplen)) { \
            pos += _must_consume_tmplen;                        \
        } else {                                                \
            return false;                                       \
        }                                                       \
    } while (0)

/* Collect a decimal number and store the result into tmp; stop on a non-digit.
 * Clobbers "c" and "tmp". TODO replace with strtol? */
#define COLLECT_NUMBER_TMP(maxdigits)                               \
    do {                                                            \
        tmp = 0;                                                    \
        for (_number_i = 0; _number_i < (maxdigits); _number_i++) { \
            c = buf[pos];                                           \
            if (c >= '0' && c <= '9') {                             \
                tmp = (tmp * 10) + (c - '0');                       \
                pos++;                                              \
            } else {                                                \
                break;                                              \
            }                                                       \
        }                                                           \
    } while (0)

    /* Go through all key=value pieces in the string */
    buflen = strlen(buf);
    ph = PH_START;
    pos = 0;
    while (pos < buflen) {
        switch (ph) {
            case PH_START: /* Expect the outer opening paren */
                DISCARD_WHITESPACE();
                MUST_CONSUME("(");
                ph = PH_KEYWORD;
                break;

            case PH_PAIR_SPACER: /* Expect end of string, or semicolon */
                DISCARD_WHILE(c == ')' || c == ']' || ISWHITE(c));
                MUST_CONSUME(";");
                DISCARD_WHITESPACE();
                ph = PH_KEYWORD;
                break;

            case PH_KEYWORD: /* Key */
                DISCARD_WHITESPACE();
                for (i = 0; i < KW_MAX; i++) {
                    toklen = strlen(KW_LOOKUP[i]);
                    if (0 == strncmp(&buf[pos], KW_LOOKUP[i], toklen)) {
                        /* kw matched */
                        kw = i;
                        pos += toklen;
                        ph = PH_VALUE_SPACER;
                        break;
                    }
                }
                if (ph != PH_VALUE_SPACER) {
                    /* Invalid token? */
                    return false;
                }
                break;

            case PH_VALUE_SPACER: /* Equals between key and value, also
                                     consuming opening square bracket if
                                     present. */
                DISCARD_WHITESPACE();
                MUST_CONSUME("=");
                DISCARD_WHILE(c == '[' || ISWHITE(c));
                ph = PH_VALUE;
                break;

            case PH_VALUE: /* Parse the value */
                switch (kw) {
                    case KW_ValidDays:
                        /* Clear all weekdays */
                        for (i = 0; i < MAX_BACNET_DAYS_OF_WEEK; i++) {
                            bitstring_set_bit(&bacdest->ValidDays, i, false);
                        }

                        j = 0; /* 0 = number, 1 = comma */
                        do {
                            DISCARD_WHITESPACE();
                            c = buf[pos];
                            if (c == 0) {
                                goto parse_end;
                            }
                            if (c == ']') {
                                pos++;
                                /* end of numbers */
                                break;
                            }
                            if (j == 0) {
                                if (c >= '1' && c <= '7') {
                                    bitstring_set_bit(
                                        &bacdest->ValidDays, c - '1', true);
                                    pos++;
                                    j = 1;
                                } else {
                                    return false;
                                }
                            } else {
                                MUST_CONSUME(",");
                                j = 0;
                            }
                        } while (1);
                        break;

                    case KW_FromTime:
                    case KW_ToTime:
                        DISCARD_WHITESPACE();

                        if (kw == KW_FromTime) {
                            ptime = &bacdest->FromTime;
                        } else {
                            ptime = &bacdest->ToTime;
                        }

                        /* TODO implemented in bacapp_parse_application_data -
                         * extract & reuse? */

                        /* Hour */
                        COLLECT_NUMBER_TMP(2);
                        ptime->hour = tmp;
                        MUST_CONSUME(":");

                        /* Min */
                        COLLECT_NUMBER_TMP(2);
                        ptime->min = tmp;

                        if (buf[pos] == ':') {
                            /* have seconds */
                            MUST_CONSUME(":");
                            /* Sec */
                            COLLECT_NUMBER_TMP(2);
                            ptime->sec = tmp;

                            /* ? hundredths */
                            c = buf[pos];
                            if (c == '.') {
                                pos++;
                                COLLECT_NUMBER_TMP(2);
                                ptime->hundredths = tmp;
                            } else {
                                ptime->hundredths = 0;
                            }
                        } else {
                            ptime->sec = 0;
                            ptime->hundredths = 0;
                        }
                        break;

                    case KW_ProcessIdentifier:
                        DISCARD_WHITESPACE();
                        /* Collect number */
                        COLLECT_NUMBER_TMP(10);
                        bacdest->ProcessIdentifier = tmp;
                        break;

                    case KW_ConfirmedNotify:
                        DISCARD_WHITESPACE();
                        if (0 == strncmp(&buf[pos], "true", 4)) {
                            bacdest->ConfirmedNotify = true;
                            pos += 4;
                        } else if (0 == strncmp(&buf[pos], "false", 5)) {
                            bacdest->ConfirmedNotify = false;
                            pos += 5;
                        } else {
                            return false;
                        }
                        break;

                    case KW_Transitions:
                        /* Clear all transitions */
                        for (i = 0; i < MAX_BACNET_EVENT_TRANSITION; i++) {
                            bitstring_set_bit(&bacdest->Transitions, i, false);
                        }
                        j = 0; /* 0 = value, 1 = comma */
                        do {
                            DISCARD_WHITESPACE();
                            c = buf[pos];
                            if (c == 0) {
                                goto parse_end;
                            }
                            if (c == ']') {
                                pos++;
                                break;
                            }
                            if (j == 0) {
                                if (0 ==
                                    strncmp(&buf[pos], "to-offnormal", 12)) {
                                    bitstring_set_bit(
                                        &bacdest->Transitions,
                                        TRANSITION_TO_OFFNORMAL, true);
                                    pos += 12;
                                } else if (
                                    0 == strncmp(&buf[pos], "to-fault", 8)) {
                                    bitstring_set_bit(
                                        &bacdest->Transitions,
                                        TRANSITION_TO_FAULT, true);
                                    pos += 8;
                                } else if (
                                    0 == strncmp(&buf[pos], "to-normal", 9)) {
                                    bitstring_set_bit(
                                        &bacdest->Transitions,
                                        TRANSITION_TO_NORMAL, true);
                                    pos += 9;
                                } else {
                                    return false;
                                }
                                j = 1;
                            } else {
                                MUST_CONSUME(",");
                                j = 0;
                            }
                        } while (1);
                        break;

                    case KW_Recipient:
                        if (0 == strncmp(&buf[pos], "Device", 6)) {
                            pos += 6;
                            bacdest->Recipient.tag =
                                BACNET_RECIPIENT_TAG_DEVICE;

                            DISCARD_WHITESPACE();
                            MUST_CONSUME("(");
                            DISCARD_WHITESPACE();
                            MUST_CONSUME("type");
                            DISCARD_WHITESPACE();
                            MUST_CONSUME("=");
                            DISCARD_WHITESPACE();

                            COLLECT_NUMBER_TMP(6);
                            bacdest->Recipient.type.device.type = tmp;

                            DISCARD_WHITESPACE();
                            MUST_CONSUME(",");
                            DISCARD_WHITESPACE();
                            MUST_CONSUME("instance");
                            DISCARD_WHITESPACE();
                            MUST_CONSUME("=");
                            DISCARD_WHITESPACE();

                            COLLECT_NUMBER_TMP(10);
                            bacdest->Recipient.type.device.instance = tmp;

                            DISCARD_WHITESPACE();
                            MUST_CONSUME(")");

                        } else if (0 == strncmp(&buf[pos], "Address", 7)) {
                            pos += 7;
                            bacdest->Recipient.tag =
                                BACNET_RECIPIENT_TAG_ADDRESS;

                            DISCARD_WHITESPACE();
                            MUST_CONSUME("(");
                            DISCARD_WHITESPACE();
                            MUST_CONSUME("net");
                            DISCARD_WHITESPACE();
                            MUST_CONSUME("=");
                            DISCARD_WHITESPACE();

                            COLLECT_NUMBER_TMP(6);
                            bacdest->Recipient.type.address.net = tmp;

                            DISCARD_WHITESPACE();
                            MUST_CONSUME(",");
                            DISCARD_WHITESPACE();
                            MUST_CONSUME("mac");
                            DISCARD_WHITESPACE();
                            MUST_CONSUME("=");
                            DISCARD_WHITESPACE();

                            if (!bacnet_address_mac_from_ascii(
                                    &tmpmac, &buf[pos])) {
                                return false;
                            }
                            bacdest->Recipient.type.address.mac_len =
                                tmpmac.len;
                            memcpy(
                                &bacdest->Recipient.type.address.mac,
                                &tmpmac.adr, MAX_MAC_LEN);

                            /* address_mac_from_ascii doesn't return number of
                             * digits
                             * - we have to discard until ) */

                            DISCARD_WHILE(c != ')');
                            pos++; /* discard the paren */
                        }
                        break;

                    default:
                        return false;
                }
                ph = PH_PAIR_SPACER;
                break;

            default:
                return false;
        }
    }
parse_end:

    return true;
}
