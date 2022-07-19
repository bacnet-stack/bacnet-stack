/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2004 Steve Karg

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

#include <string.h>

#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bits.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacint.h"
#include "bacnet/bacreal.h"

/** @file bacdcode.c  Functions to encode/decode BACnet data types */

/* max-segments-accepted
   B'000'      Unspecified number of segments accepted.
   B'001'      2 segments accepted.
   B'010'      4 segments accepted.
   B'011'      8 segments accepted.
   B'100'      16 segments accepted.
   B'101'      32 segments accepted.
   B'110'      64 segments accepted.
   B'111'      Greater than 64 segments accepted.
*/

/* max-APDU-length-accepted
   B'0000'  Up to MinimumMessageSize (50 octets)
   B'0001'  Up to 128 octets
   B'0010'  Up to 206 octets (fits in a LonTalk frame)
   B'0011'  Up to 480 octets (fits in an ARCNET frame)
   B'0100'  Up to 1024 octets
   B'0101'  Up to 1476 octets (fits in an ISO 8802-3 frame)
   B'0110'  reserved by ASHRAE
   B'0111'  reserved by ASHRAE
   B'1000'  reserved by ASHRAE
   B'1001'  reserved by ASHRAE
   B'1010'  reserved by ASHRAE
   B'1011'  reserved by ASHRAE
   B'1100'  reserved by ASHRAE
   B'1101'  reserved by ASHRAE
   B'1110'  reserved by ASHRAE
   B'1111'  reserved by ASHRAE
*/

/* Encoding of BACNET Length/Value/Type tag
   From clause 20.2.1.3.1

   B'000'   interpreted as Value  = FALSE if application class == BOOLEAN
   B'001'   interpreted as Value  = TRUE  if application class == BOOLEAN

   B'000'   interpreted as Length = 0     if application class != BOOLEAN
   B'001'   interpreted as Length = 1
   B'010'   interpreted as Length = 2
   B'011'   interpreted as Length = 3
   B'100'   interpreted as Length = 4
   B'101'   interpreted as Length > 4
   B'110'   interpreted as Type   = Opening Tag
   B'111'   interpreted as Type   = Closing Tag
*/

/**
 * Encode the max APDU value and return the encoded octed.
 *
 * @param max_segs  from clause 20.1.2.4 max-segments-accepted
 * @param max_apdu  from clause 20.1.2.5 max-APDU-length-accepted
 *
 * @return The encoded octet
 */
uint8_t encode_max_segs_max_apdu(int max_segs, int max_apdu)
{
    uint8_t octet = 0;

    if (max_segs < 2) {
        octet = 0;
    } else if (max_segs < 4) {
        octet = 0x10;
    } else if (max_segs < 8) {
        octet = 0x20;
    } else if (max_segs < 16) {
        octet = 0x30;
    } else if (max_segs < 32) {
        octet = 0x40;
    } else if (max_segs < 64) {
        octet = 0x50;
    } else if (max_segs == 64) {
        octet = 0x60;
    } else {
        octet = 0x70;
    }

    /* max_apdu must be 50 octets minimum */
    if (max_apdu <= 50) {
        octet |= 0x00;
    } else if (max_apdu <= 128) {
        octet |= 0x01;
        /*fits in a LonTalk frame */
    } else if (max_apdu <= 206) {
        octet |= 0x02;
        /*fits in an ARCNET or MS/TP frame */
    } else if (max_apdu <= 480) {
        octet |= 0x03;
    } else if (max_apdu <= 1024) {
        octet |= 0x04;
        /* fits in an ISO 8802-3 frame */
    } else if (max_apdu <= 1476) {
        octet |= 0x05;
    }

    return octet;
}

/**
 * Decode the given octed into a maximum segments value.
 *
 * @param octed  From clause 20.1.2.4 max-segments-accepted
 *               and clause 20.1.2.5 max-APDU-length-accepted
 *
 * @return  Returns the maximum segments value.
 */
int decode_max_segs(uint8_t octet)
{
    int max_segs = 0;

    switch (octet & 0xF0) {
        case 0:
            max_segs = 0;
            break;
        case 0x10:
            max_segs = 2;
            break;
        case 0x20:
            max_segs = 4;
            break;
        case 0x30:
            max_segs = 8;
            break;
        case 0x40:
            max_segs = 16;
            break;
        case 0x50:
            max_segs = 32;
            break;
        case 0x60:
            max_segs = 64;
            break;
        case 0x70:
            max_segs = 65;
            break;
        default:
            break;
    }

    return max_segs;
}

/**
 * Decode the given octed into a maximum APDU value.
 *
 * @param octed  From clause 20.1.2.4 max-segments-accepted
 *               and clause 20.1.2.5 max-APDU-length-accepted
 *
 * @return  Returns the maximum APDU value.
 */
int decode_max_apdu(uint8_t octet)
{
    int max_apdu = 0;

    switch (octet & 0x0F) {
        case 0:
            max_apdu = 50;
            break;
        case 1:
            max_apdu = 128;
            break;
        case 2:
            max_apdu = 206;
            break;
        case 3:
            max_apdu = 480;
            break;
        case 4:
            max_apdu = 1024;
            break;
        case 5:
            max_apdu = 1476;
            break;
        default:
            break;
    }

    return max_apdu;
}

/**
 * Encode a BACnet tag and returns the number of bytes consumed.
 * (From clause 20.2.1 General Rules for Encoding BACnet Tags)
 *
 * @param apdu              Pointer to the encode buffer, or NULL for length
 * @param tag_number        Number of the tag to encode,
 *                          see BACNET_APPLICATION_TAG_X macros.
 * @param context_specific  Indicates to encode in the given context.
 * @param len_value_type    Indicates the length of the tag's content.
 *
 * @return  Returns the number of apdu bytes consumed.
 */
int encode_tag(uint8_t *apdu,
    uint8_t tag_number,
    bool context_specific,
    uint32_t len_value_type)
{
    int len = 1; /* return value */
    uint8_t *apdu_offset = NULL;

    if (apdu) {
        apdu[0] = 0;
    }
    if (context_specific) {
        if (apdu) {
            apdu[0] = BIT(3);
        }
    }

    /* additional tag byte after this byte */
    /* for extended tag byte */
    if (tag_number <= 14) {
        if (apdu) {
            apdu[0] |= (tag_number << 4);
        }
    } else {
        if (apdu) {
            apdu[0] |= 0xF0;
            apdu[1] = tag_number;
        }
        len++;
    }

    /* NOTE: additional len byte(s) after extended tag byte */
    /* if larger than 4 */
    if (len_value_type <= 4) {
        if (apdu) {
            apdu[0] |= len_value_type;
        }
    } else {
        if (apdu) {
            apdu[0] |= 5;
        }
        if (len_value_type <= 253) {
            if (apdu) {
                apdu[len] = (uint8_t)len_value_type;
            }
            len++;
        } else if (len_value_type <= 65535) {
            if (apdu) {
                apdu[len] = 254;
            }
            len++;
            if (apdu) {
                apdu_offset = &apdu[len];
            }
            len += encode_unsigned16(apdu_offset, (uint16_t)len_value_type);
        } else {
            if (apdu) {
                apdu[len] = 255;
            }
            len++;
            if (apdu) {
                apdu_offset = &apdu[len];
            }
            len += encode_unsigned32(apdu_offset, len_value_type);
        }
    }

    return len;
}

/**
 * Encode a BACnet opening tag and returns the number
 * of bytes consumed.
 * (From clause 20.2.1.3.2 Constructed Data.)
 *
 * @param apdu              Pointer to the encode buffer, or NULL for length
 * @param tag_number        Number of the tag to encode,
 *                          see BACNET_APPLICATION_TAG_X macros.
 *
 * @return  Returns the number of apdu bytes consumed.
 */
int encode_opening_tag(uint8_t *apdu, uint8_t tag_number)
{
    int len = 1;

    if (apdu) {
        /* set class field to context specific */
        apdu[0] = BIT(3);
    }
    /* additional tag byte after this byte for extended tag byte */
    if (tag_number <= 14) {
        if (apdu) {
            apdu[0] |= (tag_number << 4);
        }
    } else {
        if (apdu) {
            apdu[0] |= 0xF0;
            apdu[1] = tag_number;
        }
        len++;
    }
    if (apdu) {
        /* set type field to opening tag */
        apdu[0] |= 6;
    }

    return len;
}

/**
 * Encode a BACnet closing tag and returns the number
 * of bytes consumed.
 * (From clause 20.2.1.3.2 Constructed Data.)
 *
 * @param apdu              Pointer to the encode buffer, or NULL for length
 * @param tag_number        Number of the tag to encode,
 *                          see BACNET_APPLICATION_TAG_X macros.
 *
 * @return  Returns the number of apdu bytes consumed.
 */
int encode_closing_tag(uint8_t *apdu, uint8_t tag_number)
{
    int len = 1;

    /* set class field to context specific */
    if (apdu) {
        apdu[0] = BIT(3);
    }
    /* additional tag byte after this byte for extended tag byte */
    if (tag_number <= 14) {
        if (apdu) {
            apdu[0] |= (tag_number << 4);
        }
    } else {
        if (apdu) {
            apdu[0] |= 0xF0;
            apdu[1] = tag_number;
        }
        len++;
    }
    if (apdu) {
        /* set type field to closing tag */
        apdu[0] |= 7;
    }

    return len;
}

/**
 * Decode a BACnet tag and returns the number of bytes consumed.
 *
 * @param apdu              Pointer to the encode buffer
 * @param tag_number        Place holder for number of the tag decoded
 *                          see BACNET_APPLICATION_TAG_X macros.
 *
 * @return  Returns the number of apdu bytes consumed.
 */
int decode_tag_number(uint8_t *apdu, uint8_t *tag_number)
{
    int len = 1; /* return value */

    /* decode the tag number first */
    if (IS_EXTENDED_TAG_NUMBER(apdu[0])) {
        /* extended tag */
        if (tag_number) {
            *tag_number = apdu[1];
        }
        len++;
    } else {
        if (tag_number) {
            *tag_number = (uint8_t)(apdu[0] >> 4);
        }
    }

    return len;
}

/**
 * @brief Decodes from bytes into a BACnet Tag Number
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_len_max - number of bytes in the buffer to decode
 * @param tag_number - BACnet tag number
 *
 * @return  number of bytes decoded, or zero if errors occur
 */
int bacnet_tag_number_decode(
    uint8_t *apdu, uint32_t apdu_len_max, uint8_t *tag_number)
{
    int len = 0; /* return value */

    /* decode the tag number first */
    if (apdu_len_max >= 1) {
        if (IS_EXTENDED_TAG_NUMBER(apdu[0]) && apdu_len_max >= 2) {
            /* extended tag */
            if (tag_number) {
                *tag_number = apdu[1];
            }
            len = 2;
        } else {
            if (tag_number) {
                *tag_number = (uint8_t)(apdu[0] >> 4);
            }
            len = 1;
        }
    }
    return len;
}

/**
 * @brief Returns if at the given pointer a
 * opening tag has been found.
 *
 * @param apdu  Pointer to the tag number.
 *
 * @return  true/false
 */
bool decode_is_opening_tag(uint8_t *apdu)
{
    return (bool)((apdu[0] & 0x07) == 6);
}

/**
 * @brief Returns if at the given pointer a
 * closing tag has been found.
 *
 * @param apdu  Pointer to the tag number.
 *
 * @return  true/false
 */
bool decode_is_closing_tag(uint8_t *apdu)
{
    return (bool)((apdu[0] & 0x07) == 7);
}

/**
 * @brief Decodes the tag number and the value,
 * that the APDU pointer is addressing.
 * (From clause 20.2.1.3.2 Constructed Data)
 *
 * @param apdu  Pointer to the message buffer.
 * @param tag_number  Pointer to a variable
 *                    taking the tag number.
 * @param value       Pointer to a variable
 *                    taking the value.
 *
 * @return  number of bytes decoded, or zero if errors occur
 */
int decode_tag_number_and_value(
    uint8_t *apdu, uint8_t *tag_number, uint32_t *value)
{
    int len = 1;
    uint16_t value16;
    uint32_t value32;

    len = decode_tag_number(&apdu[0], tag_number);
    if (IS_EXTENDED_VALUE(apdu[0])) {
        /* tagged as uint32_t */
        if (apdu[len] == 255) {
            len++;
            value32 = 0;
            len += decode_unsigned32(&apdu[len], &value32);
            if (value) {
                *value = value32;
            }
        }
        /* tagged as uint16_t */
        else if (apdu[len] == 254) {
            len++;
            value16 = 0;
            len += decode_unsigned16(&apdu[len], &value16);
            if (value) {
                *value = value16;
            }
        }
        /* no tag - must be uint8_t */
        else {
            if (value) {
                *value = apdu[len];
            }
            len++;
        }
    } else if (IS_OPENING_TAG(apdu[0]) && value) {
        /* opening tag */
        *value = 0;
    } else if (IS_CLOSING_TAG(apdu[0]) && value) {
        /* closing tag */
        *value = 0;
    } else if (value) {
        /* small value */
        *value = apdu[0] & 0x07;
    }

    return len;
}

/**
 * @brief Decode the BACnet Tag Number and Value
 * as defined in clause 20.2.1.3.2 Constructed Data
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_len_max - number of bytes in the buffer
 * @param tag_number - decoded tag number, if decoded
 * @param value - decoded value, if decoded
 *
 * @return the number of apdu bytes consumed
 */
int bacnet_tag_number_and_value_decode(
    uint8_t *apdu, uint32_t apdu_len_max, uint8_t *tag_number, uint32_t *value)
{
    int len = 0;

    len = bacnet_tag_number_decode(&apdu[0], apdu_len_max, tag_number);
    if (len > 0) {
        apdu_len_max -= len;
        if (IS_EXTENDED_VALUE(apdu[0])) {
            /* tagged as uint32_t */
            if ((apdu[len] == 255) && (apdu_len_max >= 5)) {
                uint32_t value32;
                len++;
                len += decode_unsigned32(&apdu[len], &value32);
                if (value) {
                    *value = value32;
                }
            }
            /* tagged as uint16_t */
            else if ((apdu[len] == 254) && (apdu_len_max >= 3)) {
                uint16_t value16;
                len++;
                len += decode_unsigned16(&apdu[len], &value16);
                if (value) {
                    *value = value16;
                }
            }
            /* no tag - must be uint8_t */
            else if ((apdu[len] < 254) && (apdu_len_max >= 1)) {
                if (value) {
                    *value = apdu[len];
                }
                len++;
            } else {
                /* packet is truncated */
                len = 0;
            }
        } else if (IS_OPENING_TAG(apdu[0]) && value) {
            *value = 0;
        } else if (IS_CLOSING_TAG(apdu[0]) && value) {
            /* closing tag */
            *value = 0;
        } else if (value) {
            /* small value */
            *value = apdu[0] & 0x07;
        }
    }

    return len;
}

/**
 * @brief Returns true if the tag is context specific
 * and matches, as defined in clause 20.2.1.3.2 Constructed
 * Data.
 *
 * @param apdu  Pointer to the tag begin.
 * @param tag_number  Tag number, that has been decoded before.
 *
 * @return true on a match, false otherwise.
 */
bool decode_is_context_tag(uint8_t *apdu, uint8_t tag_number)
{
    uint8_t my_tag_number = 0;

    decode_tag_number(apdu, &my_tag_number);

    return (bool)(IS_CONTEXT_SPECIFIC(*apdu) && (my_tag_number == tag_number));
}

/**
 * @brief Returns true if the tag is context specific
 * and matches, as defined in clause 20.2.1.3.2 Constructed
 * Data. This function returns the tag length as well.
 *
 * @param apdu  Pointer to the tag begin.
 * @param tag_number  Tag number, that has been decoded before.
 * @param tag_length  Pointer to a variable, taking the
 *                    length of the tag in bytes.
 *
 * @return true on a match, false otherwise.
 */
bool decode_is_context_tag_with_length(
    uint8_t *apdu, uint8_t tag_number, int *tag_length)
{
    uint8_t my_tag_number = 0;

    *tag_length = decode_tag_number(apdu, &my_tag_number);

    return (bool)(IS_CONTEXT_SPECIFIC(*apdu) && (my_tag_number == tag_number));
}

/**
 * @brief Returns true if the tag does match and it
 * is an opening tag as well.
 * As defined in clause 20.2.1.3.2 Constructed Data.
 *
 * @param apdu  Pointer to the tag begin.
 * @param tag_number  Tag number, that has been decoded before.
 *
 * @return true on a match, false otherwise.
 */
bool decode_is_opening_tag_number(uint8_t *apdu, uint8_t tag_number)
{
    uint8_t my_tag_number = 0;

    decode_tag_number(apdu, &my_tag_number);

    return (bool)(IS_OPENING_TAG(apdu[0]) && (my_tag_number == tag_number));
}

/**
 * @brief Returns true if the tag does match and it
 * is an closing tag as well.
 * As defined in clause 20.2.1.3.2 Constructed Data.
 *
 * @param apdu  Pointer to the tag begin.
 * @param tag_number  Tag number, that has been decoded before.
 *
 * @return true on a match, false otherwise.
 */
bool decode_is_closing_tag_number(uint8_t *apdu, uint8_t tag_number)
{
    uint8_t my_tag_number = 0;

    decode_tag_number(apdu, &my_tag_number);
    return (bool)(IS_CLOSING_TAG(apdu[0]) && (my_tag_number == tag_number));
}

/**
 * @brief Encode an boolean value.
 * From clause 20.2.3 Encoding of a Boolean Value
 * and 20.2.1 General Rules for Encoding BACnet Tags.
 *
 * @param apdu  Pointer to the encode buffer.
 * @param boolean_value  Boolean value to encode.
 *
 * @return returns the number of apdu bytes consumed
 */
int encode_application_boolean(uint8_t *apdu, bool boolean_value)
{
    int len = 0;
    uint32_t len_value;

    if (boolean_value) {
        len_value = 1;
    } else {
        len_value = 0;
    }
    len =
        encode_tag(apdu, BACNET_APPLICATION_TAG_BOOLEAN, false, len_value);

    return len;
}

/**
 * @brief Encode an boolean value in a context.
 * From clause 20.2.3 Encoding of a Boolean Value
 * and 20.2.1 General Rules for Encoding BACnet Tags.
 *
 * @param apdu  Pointer to the encode buffer.
 * @param tag_number  Tag number in which context
 *                    the value shall be encoded.
 * @param boolean_value  Boolean value to encode.
 *
 * @return returns the number of apdu bytes consumed
 */
int encode_context_boolean(
    uint8_t *apdu, uint8_t tag_number, bool boolean_value)
{
    int len; /* return value */

    len = encode_tag(apdu, (uint8_t)tag_number, true, 1);
    if (apdu) {
        apdu[len] = (bool)(boolean_value ? 1 : 0);
    }
    len++;

    return len;
}

/**
 * @brief Decode an boolean value.
 *
 * @param apdu  Pointer to the encode buffer.
 *
 * @return true/false
 */
bool decode_context_boolean(uint8_t *apdu)
{
    bool boolean_value = false;

    if (apdu[0]) {
        boolean_value = true;
    }

    return boolean_value;
}

/**
 * @brief Decode an boolean value in the context of a tag.
 *
 * @param apdu  Pointer to the encode buffer.
 * @param tag_number  Tag number in which context
 *                    the value shall be encoded.
 * @param boolean_value  Pointer to a boolean variable
 *                       taking the value.
 *
 * @return The count of bytes decoded or BACNET_STATUS_ERROR.
 */
int decode_context_boolean2(
    uint8_t *apdu, uint8_t tag_number, bool *boolean_value)
{
    int len = 0;
    if (decode_is_context_tag_with_length(&apdu[len], tag_number, &len)) {
        if (apdu[len]) {
            *boolean_value = true;
        } else {
            *boolean_value = false;
        }
        len++;
    } else {
        len = BACNET_STATUS_ERROR;
    }
    return len;
}

/**
 * @brief Check the length value and return the boolean meaning.
 * From clause 20.2.3 Encoding of a Boolean Value
 * and 20.2.1 General Rules for Encoding BACnet Tags.
 *
 * @param len_value  Tag length value.
 *
 * @return true/false
 */
bool decode_boolean(uint32_t len_value)
{
    bool boolean_value;

    if (len_value) {
        boolean_value = true;
    } else {
        boolean_value = false;
    }

    return boolean_value;
}

/**
 * @brief Encode a Null value.
 * From clause 20.2.2 Encoding of a Null Value
 * and 20.2.1 General Rules for Encoding BACnet Tags.
 *
 * @param apdu  Pointer to the encode buffer.
 *
 * @return returns the number of apdu bytes consumed.
 */
int encode_application_null(uint8_t *apdu)
{
    return encode_tag(apdu, BACNET_APPLICATION_TAG_NULL, false, 0);
}

/**
 * @brief Encode a Null value in a tag context.
 * From clause 20.2.2 Encoding of a Null Value
 * and 20.2.1 General Rules for Encoding BACnet Tags.
 *
 * @param apdu  Pointer to the encode buffer.
 * @param tag_number  Tag number in which context
 *                    the value shall be encoded.
 *
 * @return returns the number of apdu bytes consumed.
 */
int encode_context_null(uint8_t *apdu, uint8_t tag_number)
{
    return encode_tag(apdu, tag_number, true, 0);
}

/**
 * @brief Reverse the bits of the given byte.
 *
 * @param in_byte  Byte to reverse.
 *
 * @return Byte with reversed bit order.
 */
static uint8_t byte_reverse_bits(uint8_t in_byte)
{
    uint8_t out_byte = 0;

    if (in_byte & BIT(0)) {
        out_byte |= BIT(7);
    }
    if (in_byte & BIT(1)) {
        out_byte |= BIT(6);
    }
    if (in_byte & BIT(2)) {
        out_byte |= BIT(5);
    }
    if (in_byte & BIT(3)) {
        out_byte |= BIT(4);
    }
    if (in_byte & BIT(4)) {
        out_byte |= BIT(3);
    }
    if (in_byte & BIT(5)) {
        out_byte |= BIT(2);
    }
    if (in_byte & BIT(6)) {
        out_byte |= BIT(1);
    }
    if (in_byte & BIT(7)) {
        out_byte |= BIT(0);
    }

    return out_byte;
}

/**
 * @brief Decode a bit string value.
 * (From clause 20.2.10 Encoding of a Bit String Value.)
 *
 * @param apdu  Pointer to the buffer holding the message data.
 * @param len_value  Length of the Data to decode.
 * @param bit_string  Pointer to the bit string variable,
 *                    taking the decoded result.
 *
 * @return Returns the number of apdu bytes consumed.
 */
int decode_bitstring(
    uint8_t *apdu, uint32_t len_value, BACNET_BIT_STRING *bit_string)
{
    int len = 0; /* Return value */
    uint8_t unused_bits;
    uint32_t i;
    uint32_t bytes_used;

    if (apdu && bit_string) {
        /* Init/empty the string. */
        bitstring_init(bit_string);
        if (len_value) {
            /* The first octet contains the unused bits. */
            bytes_used = len_value - 1;
            if (bytes_used <= MAX_BITSTRING_BYTES) {
                len = 1;
                /* Copy the bytes in reversed bit order. */
                for (i = 0; i < bytes_used; i++) {
                    bitstring_set_octet(
                        bit_string, (uint8_t)i, byte_reverse_bits(apdu[len++]));
                }
                /* Erase the remaining unused bits. */
                unused_bits = (uint8_t)(apdu[0] & 0x07);
                bitstring_set_bits_used(
                    bit_string, (uint8_t)bytes_used, unused_bits);
            }
        }
    }
    return len;
}

/**
 * @brief Decode a bit string value in the given context.
 * (From clause 20.2.10 Encoding of a Bit String Value.)
 *
 * @param apdu  Pointer to the buffer holding the message data.
 * @param tag_number  Tag number (context).
 * @param bit_string  Pointer to the bit string variable,
 *                    taking the decoded result.
 *
 * @return Returns the number of bytes dcoded or BACNET_STATUS_ERROR.
 */
int decode_context_bitstring(
    uint8_t *apdu, uint8_t tag_number, BACNET_BIT_STRING *bit_string)
{
    uint32_t len_value;
    int len = 0;

    if (decode_is_context_tag(&apdu[len], tag_number) &&
        !decode_is_closing_tag(&apdu[len])) {
        len += decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        len += decode_bitstring(&apdu[len], len_value, bit_string);
    } else {
        len = BACNET_STATUS_ERROR;
    }
    return len;
}

/* from clause 20.2.10 Encoding of a Bit String Value */
/* returns the number of apdu bytes consumed */
int encode_bitstring(uint8_t *apdu, BACNET_BIT_STRING *bit_string)
{
    int len = 0;
    uint8_t remaining_used_bits = 0;
    uint8_t used_bytes = 0;
    uint8_t i = 0;

    /* if the bit string is empty, then the first octet shall be zero */
    if (bitstring_bits_used(bit_string) == 0) {
        if (apdu) {
            apdu[len] = 0;
        }
        len++;
    } else {
        used_bytes = bitstring_bytes_used(bit_string);
        remaining_used_bits =
            (uint8_t)(bitstring_bits_used(bit_string) - ((used_bytes - 1) * 8));
        /* number of unused bits in the subsequent final octet */
        if (apdu) {
            apdu[len] = (uint8_t)(8 - remaining_used_bits);
        }
        len++;
        for (i = 0; i < used_bytes; i++) {
            if (apdu) {
                apdu[len] = byte_reverse_bits(bitstring_octet(bit_string, i));
            }
            len++;
        }
    }

    return len;
}

int encode_application_bitstring(uint8_t *apdu, BACNET_BIT_STRING *bit_string)
{
    int len = 0;
    uint32_t bit_string_encoded_length = 1; /* 1 for the bits remaining octet */
    uint8_t *apdu_offset = NULL;

    /* bit string may use more than 1 octet for the tag, so find out how many */
    bit_string_encoded_length += bitstring_bytes_used(bit_string);
    len = encode_tag(apdu, BACNET_APPLICATION_TAG_BIT_STRING, false,
        bit_string_encoded_length);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bitstring(apdu_offset, bit_string);

    return len;
}

int encode_context_bitstring(
    uint8_t *apdu, uint8_t tag_number, BACNET_BIT_STRING *bit_string)
{
    int len = 0;
    uint32_t bit_string_encoded_length = 1; /* 1 for the bits remaining octet */
    uint8_t *apdu_offset = NULL;

    /* bit string may use more than 1 octet for the tag, so find out how many */
    bit_string_encoded_length += bitstring_bytes_used(bit_string);
    len = encode_tag(apdu, tag_number, true, bit_string_encoded_length);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bitstring(apdu_offset, bit_string);

    return len;
}

/**
 * @brief Decode the BACnet Object Identifier Value
 * as defined in clause 20.2.14 Encoding of an Object Identifier Value
 *
 * @param apdu - buffer of data to be decoded
 * @param len_value_type - the expected length of the Object Identifier
 * @param object_type - decoded object type, if decoded
 * @param object_instance - decoded object instance, if decoded
 *
 * @return the number of apdu bytes consumed
 */
int decode_object_id_safe(
    uint8_t *apdu,
    uint32_t len_value_type,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *instance)
{
    uint32_t value = 0;
    int len = 0;

    len = decode_unsigned32(apdu, &value);
    if (len_value_type == len) {
        if (apdu) {
            /* value is meaningless if apdu was NULL */
            if (object_type) {
                *object_type = (BACNET_OBJECT_TYPE)(
                    ((value >> BACNET_INSTANCE_BITS) & BACNET_MAX_OBJECT));
            }
            if (instance) {
                *instance = (value & BACNET_MAX_INSTANCE);
            }
        }
    }

    return len;
}

/**
 * @brief Decode the BACnet Object Identifier Value
 * as defined in clause 20.2.14 Encoding of an Object Identifier Value
 *
 * @param apdu - buffer of data to be decoded
 * @param len_value_type - the expected length of the Object Identifier
 * @param object_type - decoded object type, if decoded
 * @param object_instance - decoded object instance, if decoded
 *
 * @return the number of apdu bytes consumed
 */
int decode_object_id(
    uint8_t *apdu,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *instance)
{
    const uint32_t len_value = 4;

    return decode_object_id_safe(apdu, len_value, object_type, instance);
}

/**
 * @brief Decode the BACnet Object Identifier Value
 * as defined in clause 20.2.14 Encoding of an Object Identifier Value
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_len_max - number of bytes in the buffer
 * @param object_type - decoded object type, if decoded
 * @param object_instance - decoded object instance, if decoded
 *
 * @return the number of apdu bytes consumed, or 0 if apdu is too small
 */
int bacnet_object_id_decode(uint8_t *apdu,
    uint16_t apdu_len_max,
    uint32_t len_value_type,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *instance)
{
    int len = 0;

    len = decode_object_id_safe(NULL, len_value_type, object_type, instance);
    if (len <= apdu_len_max) {
        return decode_object_id_safe(apdu, len_value_type, object_type,
            instance);
    }

    return 0;
}

/**
 * @brief Decode the BACnet Object Identifier Value when application encoded
 * as defined in clause 20.2.14 Encoding of an Object Identifier Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_len_max - number of bytes in the buffer
 * @param object_type - decoded object type, if decoded
 * @param object_instance - decoded object instance, if decoded
 *
 * @return the number of apdu bytes consumed, or #BACNET_STATUS_ERROR (-1)
 */
int bacnet_object_id_application_decode(uint8_t *apdu,
    uint16_t apdu_len_max,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *object_instance)
{
    int len = 0;
    int apdu_len = BACNET_STATUS_ERROR;
    uint8_t tag_number;
    uint32_t len_value_type = 0;

    len = bacnet_tag_number_and_value_decode(
        &apdu[len], apdu_len_max, &tag_number, &len_value_type);
    if ((len > 0) && (tag_number == BACNET_APPLICATION_TAG_OBJECT_ID)) {
        apdu_len = len;
        if (apdu_len < apdu_len_max) {
            len = bacnet_object_id_decode(&apdu[len], apdu_len_max - apdu_len,
                len_value_type, object_type, object_instance);
            if (len > 0) {
                apdu_len += len;
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
        } else {
            apdu_len = BACNET_STATUS_ERROR;
        }
    }

    return apdu_len;
}

/**
 * @brief Decode the BACnet Object Identifier Value when context encoded
 * as defined in clause 20.2.14 Encoding of an Object Identifier Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_len_max - number of bytes in the buffer to decode
 * @param tag_value - context tag number expected
 * @param object_type - decoded object type, if decoded
 * @param object_instance - decoded object instance, if decoded
 *
 * @return  number of bytes decoded, zero if wrong tag number,
 * or #BACNET_STATUS_ERROR (-1) if malformed
 */
int bacnet_object_id_context_decode(uint8_t *apdu,
    uint16_t apdu_len_max,
    uint8_t tag_value,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *object_instance)
{
    int apdu_len = 0;
    unsigned len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;

    if (apdu_len_max) {
        if (decode_is_context_tag(&apdu[apdu_len], tag_value) &&
            !decode_is_closing_tag(&apdu[apdu_len])) {
            len = bacnet_tag_number_and_value_decode(&apdu[apdu_len],
                apdu_len_max - apdu_len, &tag_number, &len_value_type);
            if (len > 0) {
                apdu_len += len;
                if (apdu_len < apdu_len_max) {
                    len = bacnet_object_id_decode(&apdu[apdu_len],
                        apdu_len_max - apdu_len, len_value_type, object_type,
                        object_instance);
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
        }
    }

    return apdu_len;
}

int decode_context_object_id(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *instance)
{
    int len = 0;

    if (decode_is_context_tag_with_length(&apdu[len], tag_number, &len)) {
        len += decode_object_id(&apdu[len], object_type, instance);
    } else {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}

/**
 * @brief Encode the BACnet Object Identifier Value
 * as defined in clause 20.2.14 Encoding of an Object Identifier Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param object_type - object type to be encoded
 * @param object_instance - object instance to be encoded
 *
 * @return the number of apdu bytes encoded
 */
int encode_bacnet_object_id(
    uint8_t *apdu, BACNET_OBJECT_TYPE object_type, uint32_t instance)
{
    uint32_t value = 0;
    int len = 0;

    value =
        (((uint32_t)object_type & BACNET_MAX_OBJECT) << BACNET_INSTANCE_BITS) |
        (instance & BACNET_MAX_INSTANCE);
    len = encode_unsigned32(apdu, value);

    return len;
}

/**
 * @brief Encode the BACnet Object Identifier Value as Context Tagged
 * as defined in clause 20.2.14 Encoding of an Object Identifier Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param tag_number - context tag number to be encoded
 * @param object_type - object type to be encoded
 * @param object_instance - object instance to be encoded
 *
 * @return the number of apdu bytes encoded
 */
int encode_context_object_id(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_OBJECT_TYPE object_type,
    uint32_t instance)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    /* length of object id is 4 octets, as per 20.2.14 */
    len = encode_tag(apdu, tag_number, true, 4);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_object_id(apdu_offset, object_type, instance);

    return len;
}

/**
 * @brief Encode the BACnet Object Identifier Value as Application Tagged
 * as defined in clause 20.2.14 Encoding of an Object Identifier Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param object_type - object type to be encoded
 * @param object_instance - object instance to be encoded
 *
 * @return the number of apdu bytes encoded
 */
int encode_application_object_id(
    uint8_t *apdu, BACNET_OBJECT_TYPE object_type, uint32_t instance)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    /* get the length by using NULL APDU */
    len = encode_bacnet_object_id(NULL, object_type, instance);
    len = encode_tag(
        apdu, BACNET_APPLICATION_TAG_OBJECT_ID, false, (uint32_t)len);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_object_id(apdu_offset, object_type, instance);

    return len;
}

#if BACNET_USE_OCTETSTRING

/* from clause 20.2.8 Encoding of an Octet String Value */
/* returns the number of apdu bytes consumed */
int encode_octet_string(uint8_t *apdu, BACNET_OCTET_STRING *octet_string)
{
    int len = 0; /* return value */
    uint8_t *value;
    int i = 0; /* loop counter */

    if (octet_string) {
        /* FIXME: might need to pass in the length of the APDU
           to bounds check since it might not be the only data chunk */
        len = (int)octetstring_length(octet_string);
        value = octetstring_value(octet_string);
        if (value && apdu) {
            for (i = 0; i < len; i++) {
                apdu[i] = value[i];
            }
        }
    }

    return len;
}

/* from clause 20.2.8 Encoding of an Octet String Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
int encode_application_octet_string(
    uint8_t *apdu, BACNET_OCTET_STRING *octet_string)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    if (octet_string) {
        len = encode_tag(apdu, BACNET_APPLICATION_TAG_OCTET_STRING,
            false, octetstring_length(octet_string));
        if (apdu) {
            apdu_offset = &apdu[len];
        }
        len += encode_octet_string(apdu_offset, octet_string);
    }

    return len;
}

/* from clause 20.2.8 Encoding of an Octet String Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
int encode_context_octet_string(
    uint8_t *apdu, uint8_t tag_number, BACNET_OCTET_STRING *octet_string)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    if (octet_string) {
        len = encode_tag(
            apdu, tag_number, true, octetstring_length(octet_string));
        if (apdu) {
            apdu_offset = &apdu[len];
        }
        len += encode_octet_string(apdu_offset, octet_string);
    }

    return len;
}

/**
 * @brief Decode the BACnet Octet String Value
 * from clause 20.2.8 Encoding of an Octet String Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_len_max - number of bytes in the buffer to decode
 * @param len_value - number of bytes in the unsigned value encoding, may be
 * zero
 * @param value - the unsigned value decoded
 *
 * @return  number of bytes decoded, or BACNET_STATUS_ERROR if errors occur
 */
int bacnet_octet_string_decode(uint8_t *apdu,
    uint16_t apdu_len_max,
    uint32_t len_value,
    BACNET_OCTET_STRING *value)
{
    int len = BACNET_STATUS_ERROR;
    bool status = false;

    if (value && (len_value <= apdu_len_max)) {
        if (len_value > 0) {
            status = octetstring_init(value, &apdu[0], len_value);
        } else {
            status = octetstring_init(value, NULL, 0);
        }
        if (status) {
            len = (int)len_value;
        }
    }

    return len;
}

/* from clause 20.2.8 Encoding of an Octet String Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed, or BACNET_STATUS_ERROR */
int decode_octet_string(
    uint8_t *apdu, uint32_t len_value, BACNET_OCTET_STRING *value)
{
    const uint16_t apdu_len_max = MAX_APDU;

    return bacnet_octet_string_decode(apdu, apdu_len_max, len_value, value);
}

int decode_context_octet_string(
    uint8_t *apdu, uint8_t tag_number, BACNET_OCTET_STRING *octet_string)
{
    int len = 0; /* return value */
    bool status = false;
    uint32_t len_value = 0;

    if (decode_is_context_tag(&apdu[len], tag_number) &&
        !decode_is_closing_tag(&apdu[len])) {
        len += decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        if (len_value > 0) {
            status = octetstring_init(octet_string, &apdu[len], len_value);
        } else {
            status = octetstring_init(octet_string, NULL, 0);
        }

        if (status) {
            len += len_value;
        }
    } else {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}

/**
 * @brief Decodes from bytes into a BACnet Octet String application encoding
 * from clause 20.2.8 Encoding of an Octet String Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_len_max - number of bytes in the buffer
 * @param value - decoded value, if decoded
 *
 * @return the number of apdu bytes consumed, or #BACNET_STATUS_ERROR (-1)
 */
int bacnet_octet_string_application_decode(
    uint8_t *apdu, uint16_t apdu_len_max, BACNET_OCTET_STRING *value)
{
    int len = 0;
    int apdu_len = BACNET_STATUS_ERROR;
    uint8_t tag_number;
    uint32_t len_value_type = 0;

    len = bacnet_tag_number_and_value_decode(
        &apdu[len], apdu_len_max, &tag_number, &len_value_type);
    if ((len > 0) && (tag_number == BACNET_APPLICATION_TAG_OCTET_STRING)) {
        apdu_len = len;
        if (apdu_len < apdu_len_max) {
            len = bacnet_octet_string_decode(
                &apdu[len], apdu_len_max - apdu_len, len_value_type, value);
            if (len != BACNET_STATUS_ERROR) {
                apdu_len += len;
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
        } else {
            apdu_len = BACNET_STATUS_ERROR;
        }
    }

    return apdu_len;
}
#endif

/* from clause 20.2.9 Encoding of a Character String Value */
/* returns the number of apdu bytes consumed, or zero if failed */
uint32_t encode_bacnet_character_string_safe(uint8_t *apdu,
    uint32_t max_apdu,
    uint8_t encoding,
    char *pString,
    uint32_t length)
{
    uint32_t apdu_len = 1 /*encoding */;
    uint32_t i;

    apdu_len += length;
    if (apdu_len <= max_apdu) {
        if (apdu) {
            apdu[0] = encoding;
            for (i = 0; i < length; i++) {
                apdu[1 + i] = (uint8_t)pString[i];
            }
        }
    } else {
        apdu_len = 0;
    }

    return apdu_len;
}

int encode_bacnet_character_string(
    uint8_t *apdu, BACNET_CHARACTER_STRING *char_string)
{
    return (int)encode_bacnet_character_string_safe(apdu, MAX_APDU,
        characterstring_encoding(char_string),
        characterstring_value(char_string),
        characterstring_length(char_string));
}

/* from clause 20.2.9 Encoding of a Character String Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
int encode_application_character_string(
    uint8_t *apdu, BACNET_CHARACTER_STRING *char_string)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    len = encode_bacnet_character_string(NULL, char_string);
    len = encode_tag(apdu, BACNET_APPLICATION_TAG_CHARACTER_STRING,
        false, (uint32_t)len);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_character_string(apdu_offset, char_string);

    return len;
}

/* from clause 20.2.9 Encoding of a Character String Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
int encode_context_character_string(
    uint8_t *apdu, uint8_t tag_number, BACNET_CHARACTER_STRING *char_string)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    len = encode_bacnet_character_string(NULL, char_string);
    len = encode_tag(apdu, tag_number, true, (uint32_t)len);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_character_string(apdu_offset, char_string);

    return len;
}

/**
 * @brief Decodes from bytes into a BACnet Character String value
 * from clause 20.2.9 Encoding of a Character String Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_len_max - number of bytes in the buffer to decode
 * @param len_value - number of bytes in the unsigned value encoding
 * @param value - the unsigned value decoded
 *
 * @return  number of bytes decoded, or zero if errors occur
 */
int bacnet_character_string_decode(uint8_t *apdu,
    uint16_t apdu_len_max,
    uint32_t len_value,
    BACNET_CHARACTER_STRING *char_string)
{
    bool status = false;
    char *string_value = NULL;
    int len = 0;

    /* check to see if the APDU is long enough */
    if (len_value <= apdu_len_max) {
        if (len_value > 1) {
            string_value = (char *)&apdu[1];
        }
        status = characterstring_init(
            char_string, apdu[0], string_value, len_value - 1);
        if (status) {
            len = (int)len_value;
        }
    }

    return len;
}

/**
 * @brief Decodes from bytes into a BACnet Character String value
 * from clause 20.2.9 Encoding of a Character String Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param len_value - number of bytes in the unsigned value encoding
 * @param value - the character string value decoded
 *
 * @return  number of bytes decoded, or zero if errors occur
 */
int decode_character_string(
    uint8_t *apdu, uint32_t len_value, BACNET_CHARACTER_STRING *value)
{
    const uint16_t apdu_len_max = MAX_APDU;

    return bacnet_character_string_decode(apdu, apdu_len_max, len_value, value);
}

/**
 * @brief Decodes from bytes into a BACnet Character String value
 * from clause 20.2.9 Encoding of a Character String Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_len_max - number of bytes in the buffer to decode
 * @param tag_value - context tag number expected
 * @param value - the character string value decoded
 *
 * @return  number of bytes decoded, or zero if tag number mismatch, or
 * #BACNET_STATUS_ERROR (-1) if malformed
 */
int bacnet_character_string_context_decode(uint8_t *apdu,
    uint16_t apdu_len_max,
    uint8_t tag_value,
    BACNET_CHARACTER_STRING *value)
{
    int apdu_len = 0;
    unsigned len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;

    if (apdu_len_max) {
        if (decode_is_context_tag(&apdu[apdu_len], tag_value) &&
            !decode_is_closing_tag(&apdu[apdu_len])) {
            len = bacnet_tag_number_and_value_decode(&apdu[apdu_len],
                apdu_len_max - apdu_len, &tag_number, &len_value_type);
            if (len > 0) {
                apdu_len += len;
                if (apdu_len < apdu_len_max) {
                    len = bacnet_character_string_decode(&apdu[apdu_len],
                        apdu_len_max - apdu_len, len_value_type, value);
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
        }
    }

    return apdu_len;
}

/**
 * @brief Decodes from bytes into a BACnet Character String value
 * from clause 20.2.9 Encoding of a Character String Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param tag_value - context tag number expected
 * @param value - the character string value decoded
 *
 * @return  number of bytes decoded, #BACNET_STATUS_ERROR (-1) if
 * wrong tag number, or #BACNET_STATUS_ERROR (-1) if malformed
 */
int decode_context_character_string(
    uint8_t *apdu, uint8_t tag_value, BACNET_CHARACTER_STRING *value)
{
    int len = 0; /* return value */
    const uint16_t apdu_len_max = MAX_APDU;

    len = bacnet_character_string_context_decode(
        apdu, apdu_len_max, tag_value, value);
    if (len == 0) {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}

/**
 * @brief Decodes from bytes into a BACnet Unsigned value
 * from clause 20.2.4 Encoding of an Unsigned Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_len_max - number of bytes in the buffer to decode
 * @param len_value - number of bytes in the unsigned value encoding
 * @param value - the unsigned value decoded
 *
 * @return  number of bytes decoded, or zero if errors occur
 */
int bacnet_unsigned_decode(uint8_t *apdu,
    uint16_t apdu_len_max,
    uint32_t len_value,
    BACNET_UNSIGNED_INTEGER *value)
{
    int len = 0;
    uint16_t unsigned16_value = 0;
    uint32_t unsigned32_value = 0;
#ifdef UINT64_MAX
    uint64_t unsigned64_value = 0;
#endif

    if (value && (len_value <= apdu_len_max)) {
        switch (len_value) {
            case 1:
                *value = apdu[0];
                len = (int)len_value;
                break;
            case 2:
                decode_unsigned16(&apdu[0], &unsigned16_value);
                *value = unsigned16_value;
                len = (int)len_value;
                break;
            case 3:
                decode_unsigned24(&apdu[0], &unsigned32_value);
                *value = unsigned32_value;
                len = (int)len_value;
                break;
            case 4:
                decode_unsigned32(&apdu[0], &unsigned32_value);
                *value = unsigned32_value;
                len = (int)len_value;
                break;
#ifdef UINT64_MAX
            case 5:
                decode_unsigned40(&apdu[0], &unsigned64_value);
                *value = unsigned64_value;
                len = (int)len_value;
                break;
            case 6:
                decode_unsigned48(&apdu[0], &unsigned64_value);
                *value = unsigned64_value;
                len = (int)len_value;
                break;
            case 7:
                decode_unsigned56(&apdu[0], &unsigned64_value);
                *value = unsigned64_value;
                len = (int)len_value;
                break;
            case 8:
                decode_unsigned64(&apdu[0], &unsigned64_value);
                *value = unsigned64_value;
                len = (int)len_value;
                break;
#endif
            default:
                *value = 0;
                break;
        }
    }

    return len;
}

/**
 * @brief Decodes from bytes into a BACnet Unsigned value
 * from clause 20.2.4 Encoding of an Unsigned Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_len_max - number of bytes in the buffer to decode
 * @param tag_value - context tag number expected
 * @param value - the unsigned value decoded
 *
 * @return  number of bytes decoded, zero if wrong tag number,
 * or error (-1) if malformed
 */
int bacnet_unsigned_context_decode(uint8_t *apdu,
    uint16_t apdu_len_max,
    uint8_t tag_value,
    BACNET_UNSIGNED_INTEGER *value)
{
    int apdu_len = 0;
    unsigned len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;

    if (apdu_len_max) {
        if (decode_is_context_tag(&apdu[apdu_len], tag_value) &&
            !decode_is_closing_tag(&apdu[apdu_len])) {
            len = bacnet_tag_number_and_value_decode(&apdu[apdu_len],
                apdu_len_max - apdu_len, &tag_number, &len_value_type);
            if (len > 0) {
                apdu_len += len;
                if (apdu_len < apdu_len_max) {
                    len = bacnet_unsigned_decode(&apdu[apdu_len],
                        apdu_len_max - apdu_len, len_value_type, value);
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
        }
    }

    return apdu_len;
}

/**
 * @brief Decodes from bytes into a BACnet Unsigned value
 * from clause 20.2.4 Encoding of an Unsigned Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_len_max - number of bytes in the buffer
 * @param value - decoded value, if decoded
 *
 * @return the number of apdu bytes consumed, or #BACNET_STATUS_ERROR (-1)
 */
int bacnet_unsigned_application_decode(
    uint8_t *apdu, uint16_t apdu_len_max, BACNET_UNSIGNED_INTEGER *value)
{
    int len = 0;
    int apdu_len = BACNET_STATUS_ERROR;
    uint8_t tag_number;
    uint32_t len_value_type = 0;

    len = bacnet_tag_number_and_value_decode(
        &apdu[len], apdu_len_max, &tag_number, &len_value_type);
    if ((len > 0) && (tag_number == BACNET_APPLICATION_TAG_UNSIGNED_INT)) {
        apdu_len = len;
        if (apdu_len < apdu_len_max) {
            len = bacnet_unsigned_decode(
                &apdu[len], apdu_len_max - apdu_len, len_value_type, value);
            if (len > 0) {
                apdu_len += len;
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
        } else {
            apdu_len = BACNET_STATUS_ERROR;
        }
    }

    return apdu_len;
}

/**
 * @brief Decodes from bytes into a BACnet Unsigned value
 * from clause 20.2.4 Encoding of an Unsigned Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param len_value - number of bytes in the unsigned value encoding
 * @param value - the unsigned value decoded
 *
 * @return  number of bytes decoded, or zero if errors occur
 */
int decode_unsigned(
    uint8_t *apdu, uint32_t len_value, BACNET_UNSIGNED_INTEGER *value)
{
#ifdef UINT64_MAX
    const uint16_t apdu_len_max = 8;
#else
    const uint16_t apdu_len_max = 4;
#endif

    return bacnet_unsigned_decode(apdu, apdu_len_max, len_value, value);
}

/**
 * @brief Decodes from bytes into a BACnet Unsigned value
 * from clause 20.2.4 Encoding of an Unsigned Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param tag_value - context tag number expected
 * @param value - the unsigned value decoded
 *
 * @return  number of bytes decoded, #BACNET_STATUS_ERROR (-1) if
 * wrong tag number, or error (-1) if malformed
 */
int decode_context_unsigned(
    uint8_t *apdu, uint8_t tag_value, BACNET_UNSIGNED_INTEGER *value)
{
#ifdef UINT64_MAX
    const uint16_t apdu_len_max = 3 + 8;
#else
    const uint16_t apdu_len_max = 2 + 4;
#endif
    int len = 0;

    len = bacnet_unsigned_context_decode(apdu, apdu_len_max, tag_value, value);
    if (len == 0) {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}

/**
 * @brief Encode the BACnet Unsigned value
 * as defined in clause 20.2.4 Encoding of an Unsigned Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param value - value to be encoded
 *
 * @return the number of apdu bytes encoded
 */
int encode_bacnet_unsigned(uint8_t *apdu, BACNET_UNSIGNED_INTEGER value)
{
    int len = 0; /* return value */

    len = bacnet_unsigned_length(value);
    if (apdu) {
        if (len == 1) {
            apdu[0] = (uint8_t)value;
        } else if (len == 2) {
            (void)encode_unsigned16(&apdu[0], (uint16_t)value);
        } else if (len == 3) {
            (void)encode_unsigned24(&apdu[0], (uint32_t)value);
        } else {
#ifdef UINT64_MAX
            if (len == 4) {
                (void)encode_unsigned32(&apdu[0], (uint32_t)value);
            } else if (len == 5) {
                (void)encode_unsigned40(&apdu[0], value);
            } else if (len == 6) {
                (void)encode_unsigned48(&apdu[0], value);
            } else if (len == 7) {
                (void)encode_unsigned56(&apdu[0], value);
            } else {
                len = encode_unsigned64(&apdu[0], value);
            }
#else
            len = encode_unsigned32(&apdu[0], value);
#endif
        }
    }

    return len;
}

/**
 * @brief Encode the BACnet Unsigned value as Context Tagged
 * as defined in clause 20.2.4 Encoding of an Unsigned Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param tag_number - context tag number to be encoded
 * @param value - value to be encoded
 *
 * @return the number of apdu bytes encoded
 */
int encode_context_unsigned(
    uint8_t *apdu, uint8_t tag_number, BACNET_UNSIGNED_INTEGER value)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    /* length of unsigned is variable, as per 20.2.4 */
    len = bacnet_unsigned_length(value);
    len = encode_tag(apdu, tag_number, true, (uint32_t)len);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_unsigned(apdu_offset, value);

    return len;
}

/**
 * @brief Encode the BACnet Unsigned value as Application Tagged
 * as defined in clause 20.2.4 Encoding of an Unsigned Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param value - value to be encoded
 *
 * @return the number of apdu bytes encoded
 */
int encode_application_unsigned(uint8_t *apdu, BACNET_UNSIGNED_INTEGER value)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    len = bacnet_unsigned_length(value);
    len = encode_tag(
        apdu, BACNET_APPLICATION_TAG_UNSIGNED_INT, false, (uint32_t)len);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_unsigned(apdu_offset, value);

    return len;
}

/**
 * @brief Decodes from bytes into a BACnet Enumerated value
 * from clause 20.2.11 Encoding of an Enumerated Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_len_max - number of bytes in the buffer to decode
 * @param len_value - number of bytes in the unsigned value encoding
 * @param value - the enumerated value decoded
 *
 * @return  number of bytes decoded, or zero if errors occur
 */
int bacnet_enumerated_decode(
    uint8_t *apdu, uint16_t apdu_len_max, uint32_t len_value, uint32_t *value)
{
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    int len;

    len =
        bacnet_unsigned_decode(apdu, apdu_len_max, len_value, &unsigned_value);
    if (len > 0) {
        *value = unsigned_value;
    }

    return len;
}

/**
 * @brief Decodes from bytes into a BACnet Enumerated value
 * from clause 20.2.11 Encoding of an Enumerated Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param len_value - number of bytes in the unsigned value encoding
 * @param value - the enumerated value decoded
 *
 * @return  number of bytes decoded, or zero if errors occur
 */
int decode_enumerated(uint8_t *apdu, uint32_t len_value, uint32_t *value)
{
    const uint16_t apdu_len_max = 4;

    return bacnet_enumerated_decode(apdu, apdu_len_max, len_value, value);
}

/**
 * @brief Decodes from bytes into a BACnet Enumerated value
 * from clause 20.2.11 Encoding of an Enumerated Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_len_max - number of bytes in the buffer to decode
 * @param tag_value - context tag number expected
 * @param value - the enumerated value decoded
 *
 * @return  number of bytes decoded, or zero if tag number mismatch, or
 * #BACNET_STATUS_ERROR (-1) if malformed
 */
int bacnet_enumerated_context_decode(
    uint8_t *apdu, uint16_t apdu_len_max, uint8_t tag_value, uint32_t *value)
{
    int apdu_len = 0;
    unsigned len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;

    if (apdu_len_max) {
        if (decode_is_context_tag(&apdu[apdu_len], tag_value) &&
            !decode_is_closing_tag(&apdu[apdu_len])) {
            len = bacnet_tag_number_and_value_decode(&apdu[apdu_len],
                apdu_len_max - apdu_len, &tag_number, &len_value_type);
            if (len > 0) {
                apdu_len += len;
                if (apdu_len < apdu_len_max) {
                    len = bacnet_enumerated_decode(&apdu[apdu_len],
                        apdu_len_max - apdu_len, len_value_type, value);
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
        }
    }

    return apdu_len;
}

/**
 * @brief Decodes from bytes into a BACnet Enumerated value
 * from clause 20.2.11 Encoding of an Enumerated Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param tag_value - context tag number expected
 * @param value - the enumerated value decoded
 *
 * @return  number of bytes decoded, #BACNET_STATUS_ERROR (-1) if
 * wrong tag number, or error (-1) if malformed
 */
int decode_context_enumerated(uint8_t *apdu, uint8_t tag_value, uint32_t *value)
{
    const uint16_t apdu_len_max = 6;
    int len = 0;

    len =
        bacnet_enumerated_context_decode(apdu, apdu_len_max, tag_value, value);
    if (len == 0) {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}

/**
 * @brief Encode the BACnet Enumerated Value
 * as defined in clause 20.2.11 Encoding of an Enumerated Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param value - value to be encoded
 *
 * @return the number of apdu bytes encoded
 */
int encode_bacnet_enumerated(uint8_t *apdu, uint32_t value)
{
    return encode_bacnet_unsigned(apdu, value);
}

/**
 * @brief Encode the BACnet Enumerated Value as Application Tagged
 * as defined in clause 20.2.11 Encoding of an Enumerated Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param value - value to be encoded
 *
 * @return the number of apdu bytes encoded
 */
int encode_application_enumerated(uint8_t *apdu, uint32_t value)
{
    int len = 0; /* return value */
    uint8_t *apdu_offset = NULL;

    len = bacnet_unsigned_length(value);
    len = encode_tag(
        apdu, BACNET_APPLICATION_TAG_ENUMERATED, false, (uint32_t)len);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_enumerated(apdu_offset, value);

    return len;
}

/**
 * @brief Encode the BACnet Enumerated Value as Context Tagged
 * as defined in clause 20.2.11 Encoding of an Enumerated Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param tag_number - context tag number to be encoded
 * @param value - value to be encoded
 *
 * @return the number of apdu bytes encoded
 */
int encode_context_enumerated(uint8_t *apdu, uint8_t tag_number, uint32_t value)
{
    int len = 0; /* return value */
    uint8_t *apdu_offset = NULL;

    len = bacnet_unsigned_length(value);
    len = encode_tag(apdu, tag_number, true, (uint32_t)len);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_enumerated(apdu_offset, value);

    return len;
}

#if BACNET_USE_SIGNED
/**
 * @brief Decode the BACnet Signed Integer Value when application encoded
 * as defined in clause 20.2.5 Encoding of a Signed Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_len_max - number of bytes in the buffer to decode
 * @param len_value - number of bytes in the unsigned value encoding
 * @param value - the signed value decoded
 *
 * @return  number of bytes decoded, or zero if errors occur
 */
int bacnet_signed_decode(
    uint8_t *apdu, uint16_t apdu_len_max, uint32_t len_value, int32_t *value)
{
    int len = 0;

    if (apdu && value && (len_value <= apdu_len_max)) {
        switch (len_value) {
            case 1:
                len = decode_signed8(&apdu[0], value);
                break;
            case 2:
                len = decode_signed16(&apdu[0], value);
                break;
            case 3:
                len = decode_signed24(&apdu[0], value);
                break;
            case 4:
                len = decode_signed32(&apdu[0], value);
                break;
            default:
                *value = 0;
                break;
        }
    }

    return len;
}

/**
 * @brief Decodes from bytes into a BACnet Signed Integer
 * as defined in clause 20.2.5 Encoding of a Signed Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_len_max - number of bytes in the buffer to decode
 * @param tag_value - context tag number expected
 * @param value - the signed value decoded
 *
 * @return  number of bytes decoded, zero if wrong tag number,
 * or error (-1) if malformed
 */
int bacnet_signed_context_decode(
    uint8_t *apdu, uint16_t apdu_len_max, uint8_t tag_value, int32_t *value)
{
    int apdu_len = 0;
    unsigned len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;

    if (apdu_len_max) {
        if (decode_is_context_tag(&apdu[apdu_len], tag_value) &&
            !decode_is_closing_tag(&apdu[apdu_len])) {
            len = bacnet_tag_number_and_value_decode(&apdu[apdu_len],
                apdu_len_max - apdu_len, &tag_number, &len_value_type);
            if (len > 0) {
                apdu_len += len;
                if (apdu_len < apdu_len_max) {
                    len = bacnet_signed_decode(&apdu[apdu_len],
                        apdu_len_max - apdu_len, len_value_type, value);
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
        }
    }

    return apdu_len;
}

/**
 * @brief Decode the BACnet Signed Integer Value when application encoded
 * as defined in clause 20.2.5 Encoding of a Signed Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_len_max - number of bytes in the buffer
 * @param value - decoded value, if decoded
 *
 * @return the number of apdu bytes consumed, or #BACNET_STATUS_ERROR (-1)
 */
int bacnet_signed_application_decode(
    uint8_t *apdu, uint16_t apdu_len_max, int32_t *value)
{
    int len = 0;
    int apdu_len = BACNET_STATUS_ERROR;
    uint8_t tag_number;
    uint32_t len_value_type = 0;

    len = bacnet_tag_number_and_value_decode(
        &apdu[len], apdu_len_max, &tag_number, &len_value_type);
    if ((len > 0) && (tag_number == BACNET_APPLICATION_TAG_SIGNED_INT)) {
        apdu_len = len;
        if (apdu_len < apdu_len_max) {
            len = bacnet_signed_decode(
                &apdu[len], apdu_len_max - apdu_len, len_value_type, value);
            if (len > 0) {
                apdu_len += len;
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
        } else {
            apdu_len = BACNET_STATUS_ERROR;
        }
    }

    return apdu_len;
}

/**
 * @brief Decodes from bytes into a BACnet Signed Integer
 * from clause 20.2.5 Encoding of an Signed Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer holding the bytes
 * @param len_value - number of bytes in the unsigned value encoding
 * @param value - the signed value decoded
 *
 * @return  number of bytes decoded, #BACNET_STATUS_ERROR (-1) if
 * wrong tag number, or error (-1) if malformed
 */
int decode_signed(uint8_t *apdu, uint32_t len_value, int32_t *value)
{
    const unsigned apdu_len_max = 4;

    return bacnet_signed_decode(apdu, apdu_len_max, len_value, value);
}

/**
 * @brief Decodes from bytes into a BACnet Signed Integer
 * from clause 20.2.5 Encoding of an Signed Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer holding the bytes
 * @param tag_value - context tag number expected
 * @param value - the signed value decoded
 *
 * @return  number of bytes decoded, #BACNET_STATUS_ERROR (-1) if
 * wrong tag number, or error (-1) if malformed
 */
int decode_context_signed(uint8_t *apdu, uint8_t tag_value, int32_t *value)
{
    const uint16_t apdu_len_max = 6;
    int len = 0;

    len = bacnet_signed_context_decode(apdu, apdu_len_max, tag_value, value);
    if (len == 0) {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}

/**
 * @brief Decodes from bytes into a BACnet Signed Integer
 * from clause 20.2.5 Encoding of an Signed Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer holding the bytes, or NULL for length
 * @param value - the signed value decoded
 *
 * @return  number of bytes encoded
 */
/*  */
int encode_bacnet_signed(uint8_t *apdu, int32_t value)
{
    int len = 0; /* return value */

    len = bacnet_signed_length(value);
    if (apdu) {
        if (len == 1) {
            (void)encode_signed8(&apdu[0], (int8_t)value);
        } else if (len == 2) {
            (void)encode_signed16(&apdu[0], (int16_t)value);
        } else if (len == 3) {
            (void)encode_signed24(&apdu[0], value);
        } else {
            (void)encode_signed32(&apdu[0], value);
        }
    }

    return len;
}

/**
 * @brief Encode the BACnet Signed Integer as Application Tagged
 * as defined in clause 20.2.5 Encoding of a Signed Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param value - value to be encoded
 *
 * @return the number of apdu bytes encoded
 */
int encode_application_signed(uint8_t *apdu, int32_t value)
{
    int len = 0; /* return value */
    uint8_t *apdu_offset = NULL;

    len = bacnet_signed_length(value);
    len = encode_tag(
        apdu, BACNET_APPLICATION_TAG_SIGNED_INT, false, (uint32_t)len);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_signed(apdu_offset, value);

    return len;
}

/**
 * @brief Encode the BACnet Signed Integer as Context Tagged
 * as defined in clause 20.2.5 Encoding of a Signed Integer Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param tag_number - context tag number to be encoded
 * @param value - value to be encoded
 *
 * @return the number of apdu bytes encoded
 */
int encode_context_signed(uint8_t *apdu, uint8_t tag_number, int32_t value)
{
    int len = 0; /* return value */
    uint8_t *apdu_offset = NULL;

    len = bacnet_signed_length(value);
    len = encode_tag(apdu, tag_number, true, (uint32_t)len);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_signed(apdu_offset, value);

    return len;
}
#endif

/**
 * @brief Encode a real floating value. From clause 20.2.6 Encoding of a
 *        Real Number Value and 20.2.1 General Rules for Encoding BACnet Tags.
 *
 * @param apdu  buffer to be encoded, or NULL for length
 * @param value The float value to be encoded.
 *
 * @return the number of apdu bytes consumed.
 */
int encode_application_real(uint8_t *apdu, float value)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    /* length of REAL is 4 octets, as per 20.2.6 */
    len = encode_tag(apdu, BACNET_APPLICATION_TAG_REAL, false, 4);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_real(value, apdu_offset);

    return len;
}

/**
 * @brief Encode a real floating value with a given tag. From clause 20.2.6
 *        Encoding of a Real Number Value and 20.2.1 General Rules for
 *        Encoding BACnet Tags.
 *
 * @param apdu  Transmit buffer
 * @param tag_number  Tag number to be used
 * @param value The float value to be encoded.
 *
 * @return Returns the number of apdu bytes consumed.
 */
int encode_context_real(uint8_t *apdu, uint8_t tag_number, float value)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    /* length of REAL is 4 octets, as per 20.2.6 */
    len = encode_tag(apdu, tag_number, true, 4);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_real(value, apdu_offset);

    return len;
}

int decode_context_real(uint8_t *apdu, uint8_t tag_number, float *real_value)
{
    uint32_t len_value;
    int len = 0;

    if (decode_is_context_tag(&apdu[len], tag_number)) {
        len += decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        len += decode_real(&apdu[len], real_value);
    } else {
        len = -1;
    }
    return len;
}

#if BACNET_USE_DOUBLE
/**
 * @brief Encode a Double Precision Real Number Value as Application Tagged
 *  From clause 20.2.7 Encoding of a Double Precision Real Number Value
 *  and clause 20.2.1 General Rules for Encoding BACnet Tags.
 *
 * @param apdu  buffer to be encoded, or NULL for length
 * @param value The value to be encoded.
 *
 * @return the number of apdu bytes consumed.
 */
int encode_application_double(uint8_t *apdu, double value)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    /* length of DOUBLE is 8 octets, as per 20.2.7 */
    len = encode_tag(apdu, BACNET_APPLICATION_TAG_DOUBLE, false, 8);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_double(value, apdu_offset);

    return len;
}

/**
 * @brief Encode a Double Precision Real Number Value as Context Tagged
 *  From clause 20.2.7 Encoding of a Double Precision Real Number Value
 *  and clause 20.2.1 General Rules for Encoding BACnet Tags.
 *
 * @param apdu  buffer to be encoded, or NULL for length
 * @param tag_number  Tag number to be used
 * @param value The value to be encoded.
 *
 * @return the number of apdu bytes consumed.
 */
int encode_context_double(uint8_t *apdu, uint8_t tag_number, double value)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    /* length of double is 8 octets, as per 20.2.7 */
    len = encode_tag(apdu, tag_number, true, 8);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_double(value, apdu_offset);

    return len;
}

int decode_context_double(
    uint8_t *apdu, uint8_t tag_number, double *double_value)
{
    uint32_t len_value;
    int len = 0;

    if (decode_is_context_tag(&apdu[len], tag_number)) {
        len += decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        len += decode_double(&apdu[len], double_value);
    } else {
        len = -1;
    }
    return len;
}
#endif

/**
 * @brief Encode a Time Value
 *  From clause 20.2.13 Encoding of a Time Value
 *  and clause 20.2.1 General Rules for Encoding BACnet Tags.
 *
 * @param apdu  buffer to be encoded, or NULL for length
 * @param value The value to be encoded.
 *
 * @return the number of apdu bytes consumed.
 */
int encode_bacnet_time(uint8_t *apdu, BACNET_TIME *btime)
{
    if (apdu) {
        apdu[0] = btime->hour;
        apdu[1] = btime->min;
        apdu[2] = btime->sec;
        apdu[3] = btime->hundredths;
    }

    return 4;
}

/**
 * @brief Encode a Time Value as Application Tagged
 *  From clause 20.2.13 Encoding of a Time Value
 *  and clause 20.2.1 General Rules for Encoding BACnet Tags.
 *
 * @param apdu  buffer to be encoded, or NULL for length
 * @param btime The value to be encoded.
 *
 * @return the number of apdu bytes consumed.
 */
int encode_application_time(uint8_t *apdu, BACNET_TIME *btime)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    /* length of Time value is 4 octets, as per 20.2.13 */
    len = encode_tag(apdu, BACNET_APPLICATION_TAG_TIME, false, 4);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_time(apdu_offset, btime);

    return len;
}

/**
 * @brief Encode a Time Value as Context Tagged
 *  From clause 20.2.13 Encoding of a Time Value
 *  and clause 20.2.1 General Rules for Encoding BACnet Tags.
 *
 * @param apdu  buffer to be encoded, or NULL for length
 * @param tag_number  Tag number to be used
 * @param btime The value to be encoded.
 *
 * @return the number of apdu bytes consumed.
 */
int encode_context_time(uint8_t *apdu, uint8_t tag_number, BACNET_TIME *btime)
{
    int len = 0; /* return value */
    uint8_t *apdu_offset = NULL;

    /* length of time is 4 octets, as per 20.2.13 */
    len = encode_tag(apdu, tag_number, true, 4);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_time(apdu_offset, btime);

    return len;
}

/**
 * @brief Decodes from bytes into a BACnet Time Value
 * from clause 20.2.13 Encoding of a Time Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_len_max - number of bytes in the buffer to decode
 * @param len_value - number of bytes encoded
 * @param value - the unsigned value decoded
 *
 * @return  number of bytes decoded, or zero if errors occur
 */
int bacnet_time_decode(uint8_t *apdu,
    uint16_t apdu_len_max,
    uint32_t len_value,
    BACNET_TIME *value)
{
    int len = 0;

    if (value && (len_value <= apdu_len_max) && (len_value == 4)) {
        /* length of time is 4 octets, as per 20.2.13 */
        value->hour = apdu[0];
        value->min = apdu[1];
        value->sec = apdu[2];
        value->hundredths = apdu[3];
        len = (int)len_value;
    }

    return len;
}

/**
 * @brief Decodes from bytes into a BACnet Time Value context encoded
 * from clause 20.2.13 Encoding of a Time Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_len_max - number of bytes in the buffer to decode
 * @param tag_value - context tag number expected
 * @param value - the unsigned value decoded
 *
 * @return  number of bytes decoded, zero if wrong tag number,
 * or error (-1) if malformed
 */
int bacnet_time_context_decode(
    uint8_t *apdu, uint16_t apdu_len_max, uint8_t tag_value, BACNET_TIME *value)
{
    int apdu_len = 0;
    unsigned len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;

    if (apdu_len_max) {
        if (decode_is_context_tag(&apdu[apdu_len], tag_value) &&
            !decode_is_closing_tag(&apdu[apdu_len])) {
            len = bacnet_tag_number_and_value_decode(&apdu[apdu_len],
                apdu_len_max - apdu_len, &tag_number, &len_value_type);
            if (len > 0) {
                apdu_len += len;
                if (apdu_len < apdu_len_max) {
                    len = bacnet_time_decode(&apdu[apdu_len],
                        apdu_len_max - apdu_len, len_value_type, value);
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
        }
    }

    return apdu_len;
}

/**
 * @brief Decodes from bytes into a BACnet Time Value application encoded
 * from clause 20.2.13 Encoding of a Time Value
 * and 20.2.1 General Rules for Encoding BACnet Tags
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_len_max - number of bytes in the buffer
 * @param value - decoded value, if decoded
 *
 * @return the number of apdu bytes consumed, or #BACNET_STATUS_ERROR (-1)
 */
int bacnet_time_application_decode(
    uint8_t *apdu, uint16_t apdu_len_max, BACNET_TIME *value)
{
    int len = 0;
    int apdu_len = BACNET_STATUS_ERROR;
    uint8_t tag_number;
    uint32_t len_value_type = 0;

    len = bacnet_tag_number_and_value_decode(
        &apdu[len], apdu_len_max, &tag_number, &len_value_type);
    if ((len > 0) && (tag_number == BACNET_APPLICATION_TAG_TIME)) {
        apdu_len = len;
        if (apdu_len < apdu_len_max) {
            len = bacnet_time_decode(
                &apdu[len], apdu_len_max - apdu_len, len_value_type, value);
            if (len > 0) {
                apdu_len += len;
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
        } else {
            apdu_len = BACNET_STATUS_ERROR;
        }
    }

    return apdu_len;
}

/* from clause 20.2.13 Encoding of a Time Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
int decode_bacnet_time(uint8_t *apdu, BACNET_TIME *value)
{
    const uint16_t apdu_len_max = 4;
    const uint32_t len_value = 4;

    return bacnet_time_decode(apdu, apdu_len_max, len_value, value);
}

int decode_bacnet_time_safe(
    uint8_t *apdu, uint32_t len_value, BACNET_TIME *btime)
{
    if (len_value != 4) {
        btime->hour = 0;
        btime->hundredths = 0;
        btime->min = 0;
        btime->sec = 0;
        return (int)len_value;
    } else {
        return decode_bacnet_time(apdu, btime);
    }
}

int decode_application_time(uint8_t *apdu, BACNET_TIME *btime)
{
    int len = 0;
    uint8_t tag_number;
    decode_tag_number(&apdu[len], &tag_number);

    if (tag_number == BACNET_APPLICATION_TAG_TIME) {
        len++;
        len += decode_bacnet_time(&apdu[len], btime);
    } else {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}

int decode_context_bacnet_time(
    uint8_t *apdu, uint8_t tag_number, BACNET_TIME *btime)
{
    int len = 0;

    if (decode_is_context_tag_with_length(&apdu[len], tag_number, &len)) {
        len += decode_bacnet_time(&apdu[len], btime);
    } else {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}

/**
 * @brief Encode a Date Value
 *  From clause 20.2.12 Encoding of a Date Value
 *  and clause 20.2.1 General Rules for Encoding BACnet Tags.
 *
 * BACnet Date
 *  year = years since 1900, wildcard=1900+255
 *  month 1=Jan
 *  day = day of month
 *  wday 1=Monday...7=Sunday
 *
 * @param apdu  buffer to be encoded, or NULL for length
 * @param value The value to be encoded.
 *
 * @return the number of apdu bytes consumed.
 */
int encode_bacnet_date(uint8_t *apdu, BACNET_DATE *bdate)
{
    if (apdu) {
        if (bdate->year >= 1900) {
            /* normal encoding, including wildcard */
            apdu[0] = (uint8_t)(bdate->year - 1900);
        } else if (bdate->year < 0x100) {
            /* allow 2 digit years */
            apdu[0] = (uint8_t)bdate->year;
        } else {
            /*
             ** Don't try and guess what the user meant here. Just fail
             */
            return BACNET_STATUS_ERROR;
        }
        apdu[1] = bdate->month;
        apdu[2] = bdate->day;
        apdu[3] = bdate->wday;
    }

    return 4;
}

/**
 * @brief Encode a Date Value as Application Tagged
 *  From clause 20.2.12 Encoding of a Date Value
 *  and clause 20.2.1 General Rules for Encoding BACnet Tags.
 *
 * @param apdu  buffer to be encoded, or NULL for length
 * @param bdate The value to be encoded.
 *
 * @return the number of apdu bytes consumed.
 */
int encode_application_date(uint8_t *apdu, BACNET_DATE *bdate)
{
    int len = 0;
    uint8_t *apdu_offset = NULL;

    /* length of Date value is 4 octets, as per 20.2.12 */
    len = encode_tag(apdu, BACNET_APPLICATION_TAG_DATE, false, 4);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_date(apdu_offset, bdate);

    return len;
}

/**
 * @brief Encode a Date Value as Context Tagged
 *  From clause 20.2.12 Encoding of a Date Value
 *  and clause 20.2.1 General Rules for Encoding BACnet Tags.
 *
 * @param apdu  buffer to be encoded, or NULL for length
 * @param tag_number  Tag number to be used
 * @param bdate The value to be encoded.
 *
 * @return the number of apdu bytes consumed.
 */
int encode_context_date(uint8_t *apdu, uint8_t tag_number, BACNET_DATE *bdate)
{
    int len = 0; /* return value */
    uint8_t *apdu_offset = NULL;

    /* length of date is 4 octets, as per 20.2.12 */
    len = encode_tag(apdu, tag_number, true, 4);
    if (apdu) {
        apdu_offset = &apdu[len];
    }
    len += encode_bacnet_date(apdu_offset, bdate);

    return len;
}

/* from clause 20.2.12 Encoding of a Date Value */
/* and 20.2.1 General Rules for Encoding BACnet Tags */
/* returns the number of apdu bytes consumed */
int decode_date(uint8_t *apdu, BACNET_DATE *bdate)
{
    bdate->year = (uint16_t)apdu[0] + 1900;
    bdate->month = apdu[1];
    bdate->day = apdu[2];
    bdate->wday = apdu[3];

    return 4;
}

int decode_date_safe(uint8_t *apdu, uint32_t len_value, BACNET_DATE *bdate)
{
    if (len_value != 4) {
        bdate->day = 0;
        bdate->month = 0;
        bdate->wday = 0;
        bdate->year = 0;
        return (int)len_value;
    } else {
        return decode_date(apdu, bdate);
    }
}

int decode_application_date(uint8_t *apdu, BACNET_DATE *bdate)
{
    int len = 0;
    uint8_t tag_number;
    decode_tag_number(&apdu[len], &tag_number);

    if (tag_number == BACNET_APPLICATION_TAG_DATE) {
        len++;
        len += decode_date(&apdu[len], bdate);
    } else {
        len = BACNET_STATUS_ERROR;
    }
    return len;
}

int decode_context_date(uint8_t *apdu, uint8_t tag_number, BACNET_DATE *bdate)
{
    int len = 0;

    if (decode_is_context_tag_with_length(&apdu[len], tag_number, &len)) {
        len += decode_date(&apdu[len], bdate);
    } else {
        len = BACNET_STATUS_ERROR;
    }
    return len;
}

/**
 * Encode a simple ACK and returns the number of apdu bytes consumed.
 *
 * @param apdu - buffer to hold encoded data, or NULL for length
 * @param invoke_id  ID invoked
 * @param service_choice  Service being acked
 *
 * @return number of apdu bytes consumed
 */
int encode_simple_ack(uint8_t *apdu, uint8_t invoke_id, uint8_t service_choice)
{
    if (apdu) {
        apdu[0] = PDU_TYPE_SIMPLE_ACK;
        apdu[1] = invoke_id;
        apdu[2] = service_choice;
    }

    return 3;
}

/**
 * Encode a BACnetAddress and returns the number of apdu bytes consumed.
 *
 * @param apdu - buffer to hold encoded data, or NULL for length
 * @param destination  Pointer to the destination address to be encoded.
 *
 * @return number of apdu bytes created
 */
int encode_bacnet_address(uint8_t *apdu, BACNET_ADDRESS *destination)
{
    int apdu_len = 0;
    BACNET_OCTET_STRING mac_addr;

    if (destination) {
        /* network number */
        apdu_len +=
            encode_application_unsigned(apdu, destination->net);
        /* encode mac address as an octet-string */
        if (destination->len != 0) {
            octetstring_init(&mac_addr, destination->adr, destination->len);
        } else {
            octetstring_init(&mac_addr, destination->mac, destination->mac_len);
        }
        if (apdu) {
            apdu += apdu_len;
        }
        apdu_len += encode_application_octet_string(apdu, &mac_addr);
    }
    return apdu_len;
}

/**
 * Decode a BACnetAddress and returns the number of apdu bytes consumed.
 *
 * @param apdu  Receive buffer
 * @param destination  Pointer to the destination address structure to be filled
 * in.
 *
 * @return number of apdu bytes consumed
 */
int decode_bacnet_address(uint8_t *apdu, BACNET_ADDRESS *destination)
{
    int len = 0;
    int tag_len = 0;
    uint32_t len_value_type = 0;
    uint8_t i = 0;
    BACNET_UNSIGNED_INTEGER data_unsigned = 0;
    uint8_t tag_number = 0;
    BACNET_OCTET_STRING mac_addr = { 0 };

    /* network number */
    tag_len =
        decode_tag_number_and_value(&apdu[len], &tag_number, &len_value_type);
    len += tag_len;
    if (tag_number != BACNET_APPLICATION_TAG_UNSIGNED_INT) {
        return BACNET_STATUS_ERROR;
    }
    len += decode_unsigned(&apdu[len], len_value_type, &data_unsigned);
    destination->net = (uint16_t)data_unsigned;
    /* encode mac address as an octet-string */
    tag_len =
        decode_tag_number_and_value(&apdu[len], &tag_number, &len_value_type);
    len += tag_len;
    if (tag_number != BACNET_APPLICATION_TAG_OCTET_STRING) {
        return BACNET_STATUS_ERROR;
    }
    len += decode_octet_string(&apdu[len], len_value_type, &mac_addr);
    destination->mac_len = mac_addr.length;
    /* paranoia : test too big strings */
    if (destination->mac_len > sizeof(destination->mac)) {
        destination->mac_len = sizeof(destination->mac);
    }
    /* copy address */
    for (i = 0; i < destination->mac_len; i++) {
        destination->mac[i] = mac_addr.value[i];
    }

    return len;
}

/* BACnetAddress */
int encode_context_bacnet_address(
    uint8_t *apdu, uint8_t tag_number, BACNET_ADDRESS *destination)
{
    int apdu_len = 0;

    apdu_len += encode_opening_tag(apdu, tag_number);
    if (apdu) {
        apdu += apdu_len;
    }
    apdu_len += encode_bacnet_address(apdu, destination);
    if (apdu) {
        apdu += apdu_len;
    }
    apdu_len += encode_closing_tag(apdu, tag_number);

    return apdu_len;
}

/* BACnetAddress */
int decode_context_bacnet_address(
    uint8_t *apdu, uint8_t tag_number, BACNET_ADDRESS *destination)
{
    int len = 0;
    int section_length;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_length = decode_bacnet_address(&apdu[len], destination);
        if (section_length < 0) {
            len = BACNET_STATUS_ERROR;
        } else {
            len += section_length;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                len = BACNET_STATUS_ERROR;
            }
        }
    } else {
        len = BACNET_STATUS_ERROR;
    }
    return len;
}
