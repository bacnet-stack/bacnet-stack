/**
 * @file
 * @brief BACnet bitstring, octectstring, and characterstring encode
 *  and decode functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacstr.h"

#ifndef BACNET_USE_OCTETSTRING /* Do we need any octet strings? */
#define BACNET_USE_OCTETSTRING 1
#endif

#ifndef BACNET_STRING_UTF8_VALIDATION
#define BACNET_STRING_UTF8_VALIDATION 1
#endif

/* check the limits of bitstring capacity */
#if ((MAX_BITSTRING_BYTES * 8) > (UINT8_MAX + 1))
#error "MAX_BITSTRING_BYTES cannot exceed 32!"
#endif
#if (((MAX_BITSTRING_BYTES * 8) > UINT8_MAX) && (UINT_MAX <= UINT8_MAX))
#error "MAX_BITSTRING_BYTES cannot exceed 31!"
#endif

/**
 * Initialize a bit string.
 *
 * @param bit_string  Pointer to the bit string structure.
 */
void bitstring_init(BACNET_BIT_STRING *bit_string)
{
    unsigned i;

    if (bit_string) {
        bit_string->bits_used = 0;
        for (i = 0; i < MAX_BITSTRING_BYTES; i++) {
            bit_string->value[i] = 0;
        }
    }
}

/**
 * Set bits in the bit string.
 *
 * @param bit_string  Pointer to the bit string structure.
 * @param bit_number  Number of the bit [0..(MAX_BITSTRING_BYTES*8)-1]
 * @param value       Value 0/1
 */
void bitstring_set_bit(
    BACNET_BIT_STRING *bit_string, uint8_t bit_number, bool value)
{
    unsigned byte_number = bit_number / 8;
    uint8_t bit_mask = 1;

    if (bit_string) {
        if (byte_number < MAX_BITSTRING_BYTES) {
            /* set max bits used */
            if (bit_string->bits_used < (bit_number + 1)) {
                bit_string->bits_used = bit_number + 1;
            }
            bit_mask = bit_mask << (bit_number - (byte_number * 8));
            if (value) {
                bit_string->value[byte_number] |= bit_mask;
            } else {
                bit_string->value[byte_number] &= (~(bit_mask));
            }
        }
    }
}

/**
 * Return the value of a single bit
 * out of the bit string.
 *
 * @param bit_string  Pointer to the bit string structure.
 * @param bit_number  Number of the bit [0..(MAX_BITSTRING_BYTES*8)-1]
 *
 * @return Value 0/1
 */
bool bitstring_bit(const BACNET_BIT_STRING *bit_string, uint8_t bit_number)
{
    bool value = false;
    unsigned byte_number = bit_number / 8;
    uint8_t bit_mask = 1;

    if (bit_string) {
        if (bit_number < (MAX_BITSTRING_BYTES * 8)) {
            bit_mask = bit_mask << (bit_number - (byte_number * 8));
            if (bit_string->value[byte_number] & bit_mask) {
                value = true;
            }
        }
    }

    return value;
}

/**
 * Return the number of bits used.
 *
 * @param bit_string  Pointer to the bit string structure.
 *
 * @return Bits used [0..(MAX_BITSTRING_BYTES*8)-1]
 */
uint8_t bitstring_bits_used(const BACNET_BIT_STRING *bit_string)
{
    return (bit_string ? bit_string->bits_used : 0);
}

/**
 * @brief Write the amount of bits used in the bit string structure.
 * @param bit_string  Pointer to the bit string structure.
 * @param bits_used Number of bits in this bitstring
 * @return true on success or false on error.
 */
bool bitstring_bits_used_set(BACNET_BIT_STRING *bit_string, uint8_t bits_used)
{
    bool status = false;

    if (bit_string) {
        bit_string->bits_used = bits_used;
        status = true;
    }

    return status;
}

/**
 * Returns the number of bytes that a bit string is using.
 *
 * @param bit_string  Pointer to the bit string structure.
 *
 * @return Bytes used [0..MAX_BITSTRING_BYTES]
 */
uint8_t bitstring_bytes_used(const BACNET_BIT_STRING *bit_string)
{
    uint8_t len = 0; /* return value */
    uint8_t used_bytes = 0;
    uint8_t last_bit = 0;

    if (bit_string && bit_string->bits_used) {
        last_bit = bit_string->bits_used - 1;
        used_bytes = last_bit / 8;
        /* add one for the first byte */
        used_bytes++;
        len = used_bytes;
    }

    return len;
}

/**
 * Returns an octet at the given bit position.
 *
 * @param bit_string  Pointer to the bit string structure.
 * @param octet_index Byte index of the octet [0..MAX_BITSTRING_BYTES-1]
 *
 * @return Value of the octet.
 */
uint8_t
bitstring_octet(const BACNET_BIT_STRING *bit_string, uint8_t octet_index)
{
    uint8_t octet = 0;

    if (bit_string) {
        if (octet_index < MAX_BITSTRING_BYTES) {
            octet = bit_string->value[octet_index];
        }
    }

    return octet;
}

/**
 * Set an octet at the given bit position.
 *
 * @param bit_string  Pointer to the bit string structure.
 * @param index Byte index of the octet [0..MAX_BITSTRING_BYTES-1]
 * @param octet Octet value
 *
 * @return true on success, false otherwise.
 */
bool bitstring_set_octet(
    BACNET_BIT_STRING *bit_string, uint8_t index, uint8_t octet)
{
    bool status = false;

    if (bit_string) {
        if (index < MAX_BITSTRING_BYTES) {
            bit_string->value[index] = octet;
            status = true;
        }
    }

    return status;
}

/**
 * Write the amount of bits used in the bit
 * string structure.
 *
 * @param bit_string  Pointer to the bit string structure.
 * @param bytes_used  Count of bytes used.
 * @param unused_bits Count of remaining unused bits in
 *                    the last byte.
 *
 * @return true on success or false on error.
 */
bool bitstring_set_bits_used(
    BACNET_BIT_STRING *bit_string, uint8_t bytes_used, uint8_t unused_bits)
{
    bool status = false;

    if (bit_string && bytes_used) {
        bit_string->bits_used = bytes_used * 8;
        bit_string->bits_used -= unused_bits;
        status = true;
    }

    return status;
}

/**
 * Return the capacity of the bit string.
 *
 * @param bit_string  Pointer to the bit string structure.
 *
 * @return Capacitiy in bits [0..(MAX_BITSTRING_BYTES*8)]
 */
unsigned bitstring_bits_capacity(const BACNET_BIT_STRING *bit_string)
{
    if (bit_string) {
        return min((MAX_BITSTRING_BYTES * 8), (UINT8_MAX + 1));
    } else {
        return 0;
    }
}

/**
 * Copy bits from one bit string to another.
 *
 * @param dest  Pointer to the destination bit string structure.
 * @param src  Pointer to the source bit string structure.
 *
 * @return true on success, false otherwise.
 */
bool bitstring_copy(BACNET_BIT_STRING *dest, const BACNET_BIT_STRING *src)
{
    unsigned i;
    bool status = false;

    if (dest && src) {
        dest->bits_used = src->bits_used;
        for (i = 0; i < MAX_BITSTRING_BYTES; i++) {
            dest->value[i] = src->value[i];
        }
        status = true;
    }

    return status;
}

/**
 * Returns true if the same length and contents.
 *
 * @param bitstring1  Pointer to the first bit string structure.
 * @param bitstring2  Pointer to the second bit string structure.
 *
 * @return true if the content of both bit strings are
 *         the same, false otherwise.
 */
bool bitstring_same(
    const BACNET_BIT_STRING *bitstring1, const BACNET_BIT_STRING *bitstring2)
{
    int i; /* loop counter */
    int bytes_used = 0;
    uint8_t compare_mask = 0;

    if (bitstring1 && bitstring2) {
        bytes_used = (int)(bitstring1->bits_used / 8);
        if ((bitstring1->bits_used == bitstring2->bits_used) &&
            (bytes_used <= MAX_BITSTRING_BYTES)) {
            /* compare fully used bytes */
            for (i = 0; i < bytes_used; i++) {
                if (bitstring1->value[i] != bitstring2->value[i]) {
                    return false;
                }
            }
            /* compare only the relevant bits of last partly used byte */
            compare_mask = 0xFF >> (8 - (bitstring1->bits_used % 8));
            if ((bitstring1->value[bytes_used] & compare_mask) !=
                (bitstring2->value[bytes_used] & compare_mask)) {
                return false;
            } else {
                return true;
            }
        }
    }

    return false;
}

/**
 * Converts an null terminated ASCII string to an bitstring.
 *
 * Expects "1,0,1,0,1,1" or "101011" as the bits
 *
 * @param bit_string  Pointer to the bit string structure.
 * @param ascii  Pointer to a zero terminated string, made up from
 *               '0' and '1' like "010010011", that shall be
 *               converted into a bit string.
 *
 * @return true if successfully converted and fits; false if too long.
 */
bool bitstring_init_ascii(BACNET_BIT_STRING *bit_string, const char *ascii)
{
    bool status = false; /* return value */
    unsigned index = 0; /* offset into buffer */
    uint8_t bit_number = 0;

    if (bit_string) {
        bitstring_init(bit_string);
        if (ascii[0] == 0) {
            /* nothing to decode, so success! */
            status = true;
        } else {
            while (ascii[index] != 0) {
                if (bit_number >= bitstring_bits_capacity(bit_string)) {
                    /* too long of a string */
                    status = false;
                    break;
                }
                if (ascii[index] == '1') {
                    bitstring_set_bit(bit_string, bit_number, true);
                    bit_number++;
                    status = true;
                } else if (ascii[index] == '0') {
                    bitstring_set_bit(bit_string, bit_number, false);
                    bit_number++;
                    status = true;
                } else {
                    /* skip non-numeric or alpha */
                    index++;
                    continue;
                }
                /* next character */
                index++;
            }
        }
    }

    return status;
}

#define CHARACTER_STRING_CAPACITY (MAX_CHARACTER_STRING_BYTES - 1)
/**
 * Initialize a BACnet character string.
 * Returns false if the string exceeds capacity.
 * Initialize by using value=NULL
 *
 * @param char_string  Pointer to the BACnet string
 * @param encoding  Encoding that shall be used
 *                  like CHARACTER_UTF8
 * @param value  C-string used to initialize the object
 * @param length  C-String length in characters.
 *
 * @return true on success, false if the string exceeds capacity.
 */
bool characterstring_init(
    BACNET_CHARACTER_STRING *char_string,
    uint8_t encoding,
    const char *value,
    size_t length)
{
    bool status = false; /* return value */
    size_t i; /* counter */

    if (char_string) {
        char_string->length = 0;
        char_string->encoding = encoding;
        /* save a byte at the end for NULL -
           note: assumes printable characters */
        if (length <= CHARACTER_STRING_CAPACITY) {
            if (value) {
                for (i = 0; i < MAX_CHARACTER_STRING_BYTES; i++) {
                    if (i < length) {
                        char_string->value[char_string->length] = value[i];
                        char_string->length++;
                    } else {
                        char_string->value[i] = 0;
                    }
                }
            } else {
                for (i = 0; i < MAX_CHARACTER_STRING_BYTES; i++) {
                    char_string->value[i] = 0;
                }
            }
            status = true;
        }
    }

    return status;
}

/**
 * Initialize a BACnet character string.
 * Returns false if the string exceeds capacity.
 * Initialize by using value=NULL
 *
 * @param char_string  Pointer to the BACnet string
 * @param value  C-string used to initialize the object
 * @param tmax  C-String length in characters.
 *
 * @return true/false
 */
bool characterstring_init_ansi_safe(
    BACNET_CHARACTER_STRING *char_string, const char *value, size_t tmax)
{
    return characterstring_init(
        char_string, CHARACTER_ANSI_X34, value,
        value ? bacnet_strnlen(value, tmax) : 0);
}

/**
 * Initialize a BACnet character string.
 * Returns false if the string exceeds capacity.
 * Initialize by using value=NULL
 *
 * @param char_string  Pointer to the BACnet string
 * @param value  C-string used to initialize the object
 *
 * @return true/false
 */
bool characterstring_init_ansi(
    BACNET_CHARACTER_STRING *char_string, const char *value)
{
    return characterstring_init(
        char_string, CHARACTER_ANSI_X34, value, value ? strlen(value) : 0);
}

/**
 * Copy a character string.
 *
 * @param dest  Pointer to the destination string.
 * @param src  Pointer to the source string.
 *
 * @return true/false
 */
bool characterstring_copy(
    BACNET_CHARACTER_STRING *dest, const BACNET_CHARACTER_STRING *src)
{
    if (dest && src) {
        return characterstring_init(
            dest, characterstring_encoding(src), characterstring_value(src),
            characterstring_length(src));
    }

    return false;
}

/**
 * Copy a character string into a C-string.
 *
 * @param dest  Pointer to the destination C-string buffer.
 * @param dest_max_len  Size of the destination C-string buffer.
 * @param src  Pointer to the source BACnet string.
 *
 * @return true/false
 */
bool characterstring_ansi_copy(
    char *dest, size_t dest_max_len, const BACNET_CHARACTER_STRING *src)
{
    size_t i; /* counter */

    if (dest && src) {
        if ((src->encoding == CHARACTER_ANSI_X34) &&
            (src->length < dest_max_len)) {
            for (i = 0; i < dest_max_len; i++) {
                if (i < src->length) {
                    dest[i] = src->value[i];
                } else {
                    dest[i] = 0;
                }
            }
            return true;
        }
    }

    return false;
}

/**
 * @brief Copy a BACnetCharacterString into a buffer with null padding
 * @param dest  Pointer to the destination buffer.
 * @param dest_max_len  Size of the destination buffer.
 * @param src  Pointer to the source BACnetCharacterString.
 * @return Length of the copied BACnetCharacterString, or 0 if the string
 *  exceeds the destination buffer size.
 */
size_t characterstring_copy_value(
    char *dest, size_t dest_max_len, const BACNET_CHARACTER_STRING *src)
{
    size_t i = 0, length = 0;

    if (dest && src) {
        if (src->length < dest_max_len) {
            length = src->length;
            for (i = 0; i < dest_max_len; i++) {
                if (i < src->length) {
                    dest[i] = src->value[i];
                } else {
                    dest[i] = 0;
                }
            }
        }
    }

    return length;
}

/**
 * Returns true if the character encoding and string
 * contents are the same.
 *
 * @param dest  Pointer to the first string to test.
 * @param src  Pointer to the second string to test.
 *
 * @return true if the character encoding and string contents are the same
 */
bool characterstring_same(
    const BACNET_CHARACTER_STRING *dest, const BACNET_CHARACTER_STRING *src)
{
    size_t i; /* counter */
    bool same_status = false;

    if (src && dest) {
        if ((src->encoding == dest->encoding) &&
            (src->length == dest->length) &&
            (src->length <= MAX_CHARACTER_STRING_BYTES)) {
            same_status = true;
            for (i = 0; i < src->length; i++) {
                if (src->value[i] != dest->value[i]) {
                    same_status = false;
                    break;
                }
            }
        }
    } else if (src) {
        if (src->length == 0) {
            same_status = true;
        }
    } else if (dest) {
        if (dest->length == 0) {
            same_status = true;
        }
    }

    return same_status;
}

/**
 * Returns true if the BACnet string and the C-string
 * contents are the same.
 *
 * @param dest  Pointer to the first string to test.
 * @param src  Pointer to the second string to test.
 *
 * @return true if the character encoding and string contents are the same
 */
bool characterstring_ansi_same(
    const BACNET_CHARACTER_STRING *src1, const char *src2)
{
    size_t i; /* counter */
    bool same_status = false;

    if (src1 && src2) {
        if ((src1->encoding == CHARACTER_ANSI_X34) &&
            (src1->length == strlen(src2)) &&
            (src1->length <= MAX_CHARACTER_STRING_BYTES)) {
            same_status = true;
            for (i = 0; i < src1->length; i++) {
                if (src2[i] != src1->value[i]) {
                    same_status = false;
                    break;
                }
            }
        }
    } else if (src2) {
        /* NULL matches an empty string in our world */
        if (strlen(src2) == 0) {
            same_status = true;
        }
    } else if (src1) {
        if (src1->length == 0) {
            same_status = true;
        }
    }

    return same_status;
}

/**
 * Returns number of UTF8 code points in a character string.
 *
 * @param dest  Pointer to the string to count the UTF8 code points.
 *
 * @return Length of the character string in utf8 codepoints
 */
size_t characterstring_utf8_length(const BACNET_CHARACTER_STRING *str)
{
    size_t count = 0;
    int i = 0;

    while ((i < MAX_CHARACTER_STRING_BYTES) && (str->value[i] != '\0')) {
        if ((str->value[i] & 0xc0) != 0x80) {
            count++;
        }

        i++;
    }

    return count;
}

/**
 * Append some characters to the end of the characterstring
 *
 * @param char_string  Pointer to the BACnet string to which
 *                     the content of the C-string shall be added.
 * @param value  Pointer to the C-String to be added.
 * @param length  Count of characters to add.
 *
 * @param src  Pointer to the first string to test.
 *
 * @return false if the string exceeds capacity.
 */
bool characterstring_append(
    BACNET_CHARACTER_STRING *char_string, const char *value, size_t length)
{
    size_t i; /* counter */
    bool status = false; /* return value */

    if (char_string) {
        if ((length + char_string->length) <= CHARACTER_STRING_CAPACITY) {
            for (i = 0; i < length; i++) {
                char_string->value[char_string->length] = value[i];
                char_string->length++;
            }
            status = true;
        }
    }

    return status;
}

/**
 * @brief This function sets a new length without changing
 * the value. If length exceeds capacity, no modification
 * happens and function returns false.
 *
 * @return true on success, false if the string exceeds
 * capacity.
 */
bool characterstring_truncate(
    BACNET_CHARACTER_STRING *char_string, size_t length)
{
    bool status = false; /* return value */

    if (char_string) {
        if (length <= CHARACTER_STRING_CAPACITY) {
            char_string->length = length;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Returns the pointer to the C-string for the given BACnet string.
 *
 * @param char_string  Pointer to the character string.
 *
 * @return Pointer to a zero-terminated C-string.
 */
const char *characterstring_value(const BACNET_CHARACTER_STRING *char_string)
{
    const char *value = NULL;

    if (char_string) {
        value = char_string->value;
    }

    return value;
}

/**
 * @brief Returns the length for the given BACnet string.
 *
 * @param char_string  Pointer to the character string.
 *
 * @return Length of the character string, but
 *         maximum MAX_CHARACTER_STRING_BYTES.
 */
size_t characterstring_length(const BACNET_CHARACTER_STRING *char_string)
{
    size_t length = 0;

    if (char_string) {
        length = char_string->length;

        /* Length within bounds? */
        if (length > CHARACTER_STRING_CAPACITY) {
            length = CHARACTER_STRING_CAPACITY;
        }
    }

    return length;
}

/**
 * @brief Returns the possible capacity for the given BACnet string.
 *
 * @param char_string  Pointer to the character string.
 *
 * @return MAX_CHARACTER_STRING_BYTES
 */
size_t characterstring_capacity(const BACNET_CHARACTER_STRING *char_string)
{
    size_t length = 0;

    if (char_string) {
        length = CHARACTER_STRING_CAPACITY;
    }

    return length;
}

/**
 * @brief Returns the character encoding for the given BACnet string.
 *
 * @param char_string  Pointer to the character string.
 *
 * @return Encoding, like CHARACTER_ANSI_X34
 */
uint8_t characterstring_encoding(const BACNET_CHARACTER_STRING *char_string)
{
    uint8_t encoding = 0;

    if (char_string) {
        encoding = char_string->encoding;
    }

    return encoding;
}

/**
 * @brief Set the character encoding for the given BACnet string.
 *
 * @param char_string  Pointer to the character string.
 * @param Encoding, like CHARACTER_ANSI_X34
 *
 * @return true/false on error
 */
bool characterstring_set_encoding(
    BACNET_CHARACTER_STRING *char_string, uint8_t encoding)
{
    bool status = false;

    if (char_string) {
        char_string->encoding = encoding;
        status = true;
    }

    return status;
}

/**
 * @brief Returns true if string is printable.
 *
 * Used to assist in the requirement that
 * "The set of characters used in the Object_Name shall be
 * restricted to printable characters."
 *
 * Printable character: a character that represents a printable
 * symbol as opposed to a device control character. These
 * include, but are not limited to, upper- and lowercase letters,
 * punctuation marks, and mathematical symbols. The exact set
 * depends upon the character set being used. In ANSI X3.4 the
 * printable characters are represented by single octets in the range
 * X'20' - X'7E'.
 *
 * @param char_string  Pointer to the character string.
 *
 * @return true/false on error
 */
bool characterstring_printable(const BACNET_CHARACTER_STRING *char_string)
{
    bool status = false; /* return value */
    size_t i; /* counter */
    size_t imax;
    char chr;

    if (char_string) {
        if (char_string->encoding == CHARACTER_ANSI_X34) {
            status = true;
            imax = char_string->length;
            if (imax > CHARACTER_STRING_CAPACITY) {
                imax = CHARACTER_STRING_CAPACITY;
            }
            for (i = 0; i < imax; i++) {
                chr = char_string->value[i];
                if ((chr < 0x20) || (chr > 0x7E)) {
                    status = false;
                    break;
                }
            }
        } else {
            status = true;
        }
    }

    return status;
}

#if BACNET_STRING_UTF8_VALIDATION
/* Basic UTF-8 manipulation routines
 * by Jeff Bezanson
 * placed in the public domain Fall 2005 */
static const char trailingBytesForUTF8[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
};

/**
 * @brief Based on the valid_utf8 routine from the PCRE library by Philip Hazel
 * length is in bytes, since without knowing whether the string is valid
 * it's hard to know how many characters there are!
 *
 * @param str  Pointer to the character string.
 * @param length  Count of bytes to check. The count of bytes
 *                does not necessarily match the count of chars.
 *
 * @return true if the string is valid, false otherwise.
 */
bool utf8_isvalid(const char *str, size_t length)
{
    const unsigned char *p, *pend;
    unsigned char c;
    size_t ab;

    /* An empty string is valid. */
    if (length == 0) {
        return true;
    }
    /* Check pointer. */
    if (!str) {
        return false;
    }
    /* Check characters. */
    pend = (const unsigned char *)str + length;
    for (p = (const unsigned char *)str; p < pend; p++) {
        c = *p;
        /* null in middle of string */
        if (c == 0) {
            return false;
        }
        /* ASCII character */
        if (c < 128) {
            continue;
        }
        if ((c & 0xc0) != 0xc0) {
            return false;
        }
        ab = (size_t)trailingBytesForUTF8[c];
        if (length < ab) {
            return false;
        }
        length -= ab;

        p++;
        /* Check top bits in the second byte */
        if ((*p & 0xc0) != 0x80) {
            return false;
        }
        /* Check for overlong sequences for each different length */
        switch (ab) {
                /* Check for xx00 000x */
            case 1:
                if ((c & 0x3e) == 0) {
                    return false;
                }
                continue; /* We know there aren't any more bytes to check */

                /* Check for 1110 0000, xx0x xxxx */
            case 2:
                if (c == 0xe0 && (*p & 0x20) == 0) {
                    return false;
                }
                break;

                /* Check for 1111 0000, xx00 xxxx */
            case 3:
                if (c == 0xf0 && (*p & 0x30) == 0) {
                    return false;
                }
                break;

                /* Check for 1111 1000, xx00 0xxx */
            case 4:
                if (c == 0xf8 && (*p & 0x38) == 0) {
                    return false;
                }
                break;

                /* Check for leading 0xfe or 0xff,
                   and then for 1111 1100, xx00 00xx */
            case 5:
                if (c == 0xfe || c == 0xff || (c == 0xfc && (*p & 0x3c) == 0)) {
                    return false;
                }
                break;
            default:
                break;
        }

        /* Check for valid bytes after the 2nd, if any; all must start 10 */
        while (--ab > 0) {
            if ((*(++p) & 0xc0) != 0x80) {
                return false;
            }
        }
    }

    return true;
}
#else
bool utf8_isvalid(const char *str, size_t length)
{
    (void)str;
    (void)length;
    return true;
}
#endif

/**
 * Check if the character string is valid or not.
 *
 * @param char_string  Pointer to the character string.
 *
 * @return true if the string is valid, false otherwise.
 */
bool characterstring_valid(const BACNET_CHARACTER_STRING *char_string)
{
    bool valid = false; /* return value */

    if (char_string) {
        if (char_string->encoding < MAX_CHARACTER_STRING_ENCODING) {
            if (char_string->encoding == CHARACTER_UTF8) {
                /*UTF8 check*/
                if (utf8_isvalid(char_string->value, char_string->length)) {
                    valid = true;
                }
            } else {
                /*non UTF8*/
                valid = true;
            }
        }
    }
    return valid;
}

#if BACNET_USE_OCTETSTRING
/**
 * @brief Initialize an octet string with the given bytes or
 * zeros, if NULL for the value is provided.
 *
 * @param octet_string  Pointer to the octet string.
 * @param value  Pointer to the bytes to be copied to the octet
 *               string or NULL to initialize the octet string.
 * @param length  Count of bytes used to fill the octet string.
 *
 * @return true on success, false if the string exceeds capacity.
 */
bool octetstring_init(
    BACNET_OCTET_STRING *octet_string, const uint8_t *value, size_t length)
{
    bool status = false; /* return value */
    size_t i; /* counter */
    uint8_t *pb = NULL;

    if (octet_string && (length <= MAX_OCTET_STRING_BYTES)) {
        octet_string->length = 0;
        if (value) {
            pb = octet_string->value;
            for (i = 0; i < MAX_OCTET_STRING_BYTES; i++) {
                if (i < length) {
                    *pb = value[i];
                } else {
                    *pb = 0;
                }
                pb++;
            }
            octet_string->length = length;
        } else {
            memset(octet_string->value, 0, MAX_OCTET_STRING_BYTES);
        }
        status = true;
    }

    return status;
}

/** @brief Converts an null terminated ASCII Hex string to an octet string.
 *
 * @param octet_string  Pointer to the octet string.
 * @param ascii_hex  Pointer to the HEX-ASCII string.
 *
 * @return true if successfully converted and fits; false if too long */
bool octetstring_init_ascii_hex(
    BACNET_OCTET_STRING *octet_string, const char *ascii_hex)
{
    bool status = false; /* return value */
    unsigned index = 0; /* offset into buffer */
    uint8_t value = 0;
    char hex_pair_string[3] = "";

    if (octet_string && ascii_hex) {
        octet_string->length = 0;
        if (ascii_hex[0] == 0) {
            /* nothing to decode, so success! */
            status = true;
        } else {
            while (ascii_hex[index] != 0) {
                if (!isalnum((int)ascii_hex[index])) {
                    /* skip non-numeric or alpha */
                    index++;
                    continue;
                }
                if (ascii_hex[index + 1] == 0) {
                    /* not a hex pair */
                    status = false;
                    break;
                }
                hex_pair_string[0] = ascii_hex[index];
                hex_pair_string[1] = ascii_hex[index + 1];
                value = (uint8_t)strtol(hex_pair_string, NULL, 16);
                if (octet_string->length <= MAX_OCTET_STRING_BYTES) {
                    octet_string->value[octet_string->length] = value;
                    octet_string->length++;
                    /* at least one pair was decoded */
                    status = true;
                } else {
                    /* too long */
                    status = false;
                    break;
                }
                /* set up for next pair */
                index += 2;
            }
        }
    }

    return status;
}

/**
 * Copy an octet string from source to destination.
 *
 * @param dest  Pointer to the destination octet string.
 * @param src   Pointer to the source octet string.
 *
 * @return true on success, false otherwise.
 */
bool octetstring_copy(BACNET_OCTET_STRING *dest, const BACNET_OCTET_STRING *src)
{
    return octetstring_init(
        dest, octetstring_value((BACNET_OCTET_STRING *)src),
        octetstring_length(src));
}

/**
 * @brief Copy bytes from the octet string to a byte buffer.
 *
 * @param dest    Pointer to the byte buffer.
 * @param length  Bytes to be copied from the
 *                octet string to the buffer.
 * @param src     Pointer to the octet string.
 *
 * @return Returns the number of bytes copied, or 0 if
 * the dest cannot hold entire octetstring value.
 */
size_t octetstring_copy_value(
    uint8_t *dest, size_t length, const BACNET_OCTET_STRING *src)
{
    size_t bytes_copied = 0;
    size_t i; /* counter */

    if (src && dest) {
        if (src->length <= length) {
            for (i = 0; i < src->length; i++) {
                dest[i] = src->value[i];
            }
            bytes_copied = src->length;
        }
    }

    return bytes_copied;
}

/**
 * @brief Append bytes to the end of the octet string.
 *
 * @param octet_string  Pointer to the octet string.
 * @param value    Pointer to the byte buffer to be appended.
 * @param length  Bytes to be appended.
 *
 * @return false if the string exceeds capacity.
 */
bool octetstring_append(
    BACNET_OCTET_STRING *octet_string, const uint8_t *value, size_t length)
{
    size_t i; /* counter */
    bool status = false; /* return value */

    if (octet_string) {
        if ((length + octet_string->length) <= MAX_OCTET_STRING_BYTES) {
            for (i = 0; i < length; i++) {
                octet_string->value[octet_string->length] = value[i];
                octet_string->length++;
            }
            status = true;
        }
    }

    return status;
}

/**
 * @brief This function sets a new length without changing the value.
 * If length exceeds capacity, no modification happens and the
 * function returns false.
 *
 * @param octet_string  Pointer to the octet string.
 * @param length  New length the octet string is truncated to.
 *
 * @return tur on success, false otherwise.
 */
bool octetstring_truncate(BACNET_OCTET_STRING *octet_string, size_t length)
{
    bool status = false; /* return value */

    if (octet_string) {
        if (length <= MAX_OCTET_STRING_BYTES) {
            octet_string->length = length;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Returns a pointer to the value (data) of
 * the given octet string.
 *
 * @param octet_string  Pointer to the octet string.
 *
 * @return Value as a pointer to a byte array or NULL on error.
 */
uint8_t *octetstring_value(BACNET_OCTET_STRING *octet_string)
{
    uint8_t *value = NULL;

    if (octet_string) {
        value = octet_string->value;
    }

    return value;
}

/**
 * @brief Returns the length in bytes of
 * the given octet string.
 *
 * @param octet_string  Pointer to the octet string.
 *
 * @return Length in bytes. Returns always 0 on error.
 */
size_t octetstring_length(const BACNET_OCTET_STRING *octet_string)
{
    size_t length = 0;

    if (octet_string) {
        length = octet_string->length;
        /* Force length to be within bounds. */
        if (length > MAX_OCTET_STRING_BYTES) {
            length = MAX_OCTET_STRING_BYTES;
        }
    }

    return length;
}

/**
 * @brief Returns the maximum capacity of an octet string.
 *
 * @param octet_string  Pointer to the octet string.
 *
 * @return Capacity in bytes. Returns always 0 on error.
 */
size_t octetstring_capacity(const BACNET_OCTET_STRING *octet_string)
{
    size_t length = 0;

    if (octet_string) {
        length = MAX_OCTET_STRING_BYTES;
    }

    return length;
}

/**
 * @brief Returns true if the same length and contents.
 *
 * @param octet_string1  Pointer to the first octet string.
 * @param octet_string2  Pointer to the second octet string.
 *
 * @return true if the octet strings are the same, false otherwise.
 */
bool octetstring_value_same(
    const BACNET_OCTET_STRING *octet_string1,
    const BACNET_OCTET_STRING *octet_string2)
{
    size_t i = 0; /* loop counter */

    if (octet_string1 && octet_string2) {
        if ((octet_string1->length == octet_string2->length) &&
            (octet_string1->length <= MAX_OCTET_STRING_BYTES)) {
            for (i = 0; i < octet_string1->length; i++) {
                if (octet_string1->value[i] != octet_string2->value[i]) {
                    return false;
                }
            }
            return true;
        }
    }

    return false;
}
#endif

/**
 * @brief Compare two strings, case insensitive
 * @param a - first string
 * @param b - second string
 * @return 0 if the strings are equal, non-zero if not
 * @note The stricmp() function is not included in the C standard.
 */
int bacnet_stricmp(const char *a, const char *b)
{
    int twin_a, twin_b;

    if (a == NULL) {
        return -1;
    }
    if (b == NULL) {
        return 1;
    }
    do {
        twin_a = *(const unsigned char *)a;
        twin_b = *(const unsigned char *)b;
        twin_a = tolower(toupper(twin_a));
        twin_b = tolower(toupper(twin_b));
        a++;
        b++;
    } while ((twin_a == twin_b) && (twin_a != '\0'));

    return twin_a - twin_b;
}

/**
 * @brief Compare two strings, case insensitive, with length limit
 * @details The strnicmp() function compares, at most, the first n characters
 *  of string1 and string2 without sensitivity to case.
 *
 *  The function operates on null terminated strings.
 *  The string arguments to the function are expected to contain
 *  a null character (\0) marking the end of the string.
 *
 * @param a - first string
 * @param b - second string
 * @param length - maximum length to compare
 * @return 0 if the strings are equal, non-zero if not
 * @note The strnicmp() function is not included in the C standard.
 */
int bacnet_strnicmp(const char *a, const char *b, size_t length)
{
    int twin_a, twin_b;

    if (length == 0) {
        return 0;
    }
    if (a == NULL) {
        return -1;
    }
    if (b == NULL) {
        return 1;
    }
    do {
        twin_a = *(const unsigned char *)a;
        twin_b = *(const unsigned char *)b;
        twin_a = tolower(toupper(twin_a));
        twin_b = tolower(toupper(twin_b));
        a++;
        b++;
        length--;
    } while ((twin_a == twin_b) && (twin_a != '\0') && (length > 0));

    return twin_a - twin_b;
}

/**
 * @brief Return the length of a string, within a maximum length
 * @note The strnlen function is non-standard and not available in
 * all libc implementations.  This function is a workaround for that.
 * @details The strnlen function computes the smaller of the number
 * of characters in the array pointed to by s, not including any
 * terminating null character, or the value of the maxlen argument.
 * The strnlen function examines no more than maxlen bytes of the
 * array pointed to by s.
 * @param s - string to check
 * @param maxlen - maximum length to check
 * @return The strnlen function returns the number of bytes that
 * precede the first null character in the array pointed to by s,
 * if s contains a null character within the first maxlen characters;
 * otherwise, it returns maxlen.
 */
size_t bacnet_strnlen(const char *str, size_t maxlen)
{
    const char *p = memchr(str, 0, maxlen);
    if (p == NULL) {
        return maxlen;
    }
    return (p - str);
}

/**
 * @brief Attempt to convert a numeric string into a unsigned long value
 * @param str - string to convert
 * @param long_value - where to put the converted value
 * @return true if converted and value is set
 * @return false if not converted and value is not set
 */
bool bacnet_strtoul(const char *str, unsigned long *long_value)
{
    char *endptr;
    unsigned long value;

    value = strtoul(str, &endptr, 0);
    if (endptr == str) {
        /* No digits found */
        return false;
    }
    if (value == ULONG_MAX) {
        /* If the correct value is outside the range of representable values,
           {ULONG_MAX} shall be returned */
        return false;
    }
    if (*endptr != '\0') {
        /* Extra text found */
        return false;
    }
    if (long_value) {
        *long_value = value;
    }

    return true;
}

/**
 * @brief Attempt to convert a numeric string into a signed long integer
 * @param str - string to convert
 * @param long_value - where to put the converted value
 * @return true if converted and value is set
 * @return false if not converted and value is not set
 */
bool bacnet_strtol(const char *str, long *long_value)
{
    char *endptr;
    long value;

    value = strtol(str, &endptr, 0);
    if (endptr == str) {
        /* No digits found */
        return false;
    }
    if (value == LONG_MAX || value == LONG_MIN) {
        /* If the correct value is outside the range of representable values,
           {LONG_MAX} or {LONG_MIN} shall be returned */
        return false;
    }
    if (*endptr != '\0') {
        /* Extra text found */
        return false;
    }
    if (long_value) {
        *long_value = value;
    }

    return true;
}

/**
 * @brief Attempt to convert a numeric string into a finite floating point value
 * @param str - string to convert
 * @param float_value - where to put the converted value
 * @return true if converted and finite value is set
 * @return false if not converted and finite value is not set
 */
bool bacnet_strtof(const char *str, float *float_value)
{
    char *endptr;
    float value;

    value = strtof(str, &endptr);
    if (endptr == str) {
        /* No digits found */
        return false;
    }
    if (!isfinite(value)) {
        /* the given floating-point number arg is not a finite value
           i.e. it is infinite or NaN.  */
        return false;
    }
    if (*endptr != '\0') {
        /* Extra text found */
        return false;
    }
    if (float_value) {
        *float_value = value;
    }

    return true;
}

/**
 * @brief Attempt to convert a numeric string into a finite double precision
 *  floating point value
 * @param str - string to convert
 * @param double_value - where to put the converted value
 * @return true if converted and finite value is set
 * @return false if not converted and finite value is not set
 */
bool bacnet_strtod(const char *str, double *double_value)
{
    char *endptr;
    double value;

    value = strtod(str, &endptr);
    if (endptr == str) {
        /* No digits found */
        return false;
    }
    if (!isfinite(value)) {
        /* the given floating-point number arg is not a finite value
           i.e. it is infinite or NaN.  */
        return false;
    }
    if (*endptr != '\0') {
        /* Extra text found */
        return false;
    }
    if (double_value) {
        *double_value = value;
    }

    return true;
}

/**
 * @brief Attempt to convert a numeric string into a finite double precision
 *  floating point value
 * @param str - string to convert
 * @param long_double_value - where to put the converted value
 * @return true if converted and finite value is set
 * @return false if not converted and finite value is not set
 */
bool bacnet_strtold(const char *str, long double *long_double_value)
{
    char *endptr;
    long double value;

    value = strtold(str, &endptr);
    if (endptr == str) {
        /* No digits found */
        return false;
    }
    if (!isfinite(value)) {
        /* the given floating-point number arg is not a finite value
           i.e. it is infinite or NaN.  */
        return false;
    }
    if (*endptr != '\0') {
        /* Extra text found */
        return false;
    }
    if (long_double_value) {
        *long_double_value = value;
    }

    return true;
}

/**
 * @brief Attempt to convert a numeric string into a uint8_t value
 * @param str - string to convert
 * @param uint32_value - where to put the converted value
 * @return true if converted and value is set
 * @return false if not converted and value is not set
 */
bool bacnet_strtouint8(const char *str, uint8_t *uint8_value)
{
    char *endptr;
    unsigned long value;

    value = strtoul(str, &endptr, 0);
    if (endptr == str) {
        /* No digits found */
        return false;
    }
    if (value == ULONG_MAX) {
        /* If the value is outside the range of representable values,
           {ULONG_MAX} shall be returned */
        return false;
    }
    if (value > UINT8_MAX) {
        /* If the value is outside the range of this datatype */
        return false;
    }
    if (*endptr != '\0') {
        /* Extra text found */
        return false;
    }
    if (uint8_value) {
        *uint8_value = (uint8_t)value;
    }

    return true;
}

/**
 * @brief Attempt to convert a numeric string into a uint16_t value
 * @param str - string to convert
 * @param uint32_value - where to put the converted value
 * @return true if converted and value is set
 * @return false if not converted and value is not set
 */
bool bacnet_strtouint16(const char *str, uint16_t *uint16_value)
{
    char *endptr;
    unsigned long value;

    value = strtoul(str, &endptr, 0);
    if (endptr == str) {
        /* No digits found */
        return false;
    }
    if (value == ULONG_MAX) {
        /* If the value is outside the range of representable values,
           {ULONG_MAX} shall be returned */
        return false;
    }
    if (value > UINT16_MAX) {
        /* If the value is outside the range of this datatype */
        return false;
    }
    if (*endptr != '\0') {
        /* Extra text found */
        return false;
    }
    if (uint16_value) {
        *uint16_value = (uint16_t)value;
    }

    return true;
}

/**
 * @brief Attempt to convert a numeric string into a uint32_t value
 * @param str - string to convert
 * @param uint32_value - where to put the converted value
 * @return true if converted and value is set
 * @return false if not converted and value is not set
 */
bool bacnet_strtouint32(const char *str, uint32_t *uint32_value)
{
    char *endptr;
    unsigned long value;

    value = strtoul(str, &endptr, 0);
    if (endptr == str) {
        /* No digits found */
        return false;
    }
    if (value == ULONG_MAX) {
        /* If the value is outside the range of representable values,
           {ULONG_MAX} shall be returned */
        return false;
    }
    if (value > UINT32_MAX) {
        /* If the value is outside the range of this datatype */
        return false;
    }
    if (*endptr != '\0') {
        /* Extra text found */
        return false;
    }
    if (uint32_value) {
        *uint32_value = (uint32_t)value;
    }

    return true;
}

/**
 * @brief Attempt to convert a numeric string into an int32_t value
 * @param str - string to convert
 * @param int32_value - where to put the converted value
 * @return true if converted and value is set
 * @return false if not converted and value is not set
 */
bool bacnet_strtoint32(const char *str, int32_t *int32_value)
{
    char *endptr;
    long value;

    value = strtol(str, &endptr, 0);
    if (endptr == str) {
        /* No digits found */
        return false;
    }
    if (value == LONG_MAX || value == LONG_MIN) {
        /* If the value is outside the range of representable values,
           {LONG_MAX} or {LONG_MIN} shall be returned */
        return false;
    }
    if (value < INT32_MIN || value > INT32_MAX) {
        /* The correct value is outside the range of this data type */
        return false;
    }
    if (*endptr != '\0') {
        /* Extra text found */
        return false;
    }
    if (int32_value) {
        *int32_value = (int32_t)value;
    }
    return true;
}

/**
 * @brief Attempt to convert a numeric string into an int32_t value
 * @param str - string to convert
 * @param int32_value - where to put the converted value
 * @return true if converted and value is set
 * @return false if not converted and value is not set
 */
bool bacnet_strtobool(const char *str, bool *bool_value)
{
    bool status = false;
    long long_value = 0;

    if (bacnet_stricmp(str, "true") == 0 ||
        bacnet_stricmp(str, "active") == 0) {
        status = true;
        if (bool_value) {
            *bool_value = true;
        }
    } else if (
        bacnet_stricmp(str, "false") == 0 ||
        bacnet_stricmp(str, "inactive") == 0) {
        status = true;
        if (bool_value) {
            *bool_value = false;
        }
    } else {
        status = bacnet_strtol(str, &long_value);
        if (!status) {
            return false;
        }
        if (bool_value) {
            *bool_value = true;
        } else {
            *bool_value = false;
        }
    }

    return status;
}
