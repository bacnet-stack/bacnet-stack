/**
 * @file
 * @brief BACnet bitstring, octectstring, and characterstring encode 
 *  and decode functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_STRING_H
#define BACNET_STRING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/* bit strings
   They could be as large as 256/8=32 octets */
typedef struct BACnet_Bit_String {
    uint8_t bits_used;
    uint8_t value[MAX_BITSTRING_BYTES];
} BACNET_BIT_STRING;

typedef struct BACnet_Character_String {
    size_t length;
    uint8_t encoding;
    char value[MAX_CHARACTER_STRING_BYTES];
} BACNET_CHARACTER_STRING;

typedef struct BACnet_Octet_String {
    size_t length;
    uint8_t value[MAX_OCTET_STRING_BYTES];
} BACNET_OCTET_STRING;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void bitstring_init(
        BACNET_BIT_STRING * bit_string);
    BACNET_STACK_EXPORT
    void bitstring_set_bit(
        BACNET_BIT_STRING * bit_string,
        uint8_t bit_number,
        bool value);
    BACNET_STACK_EXPORT
    bool bitstring_bit(
        BACNET_BIT_STRING * bit_string,
        uint8_t bit_number);
    BACNET_STACK_EXPORT
    uint8_t bitstring_bits_used(
        BACNET_BIT_STRING * bit_string);
/* returns the number of bytes that a bit string is using */
    BACNET_STACK_EXPORT
    uint8_t bitstring_bytes_used(
        BACNET_BIT_STRING * bit_string);
    BACNET_STACK_EXPORT
    unsigned bitstring_bits_capacity(
        BACNET_BIT_STRING * bit_string);
/* used for encoding and decoding from the APDU */
    BACNET_STACK_EXPORT
    uint8_t bitstring_octet(
        BACNET_BIT_STRING * bit_string,
        uint8_t octet_index);
    BACNET_STACK_EXPORT
    bool bitstring_set_octet(
        BACNET_BIT_STRING * bit_string,
        uint8_t index,
        uint8_t octet);
    BACNET_STACK_EXPORT
    bool bitstring_set_bits_used(
        BACNET_BIT_STRING * bit_string,
        uint8_t bytes_used,
        uint8_t unused_bits);
    BACNET_STACK_EXPORT
    bool bitstring_copy(
        BACNET_BIT_STRING * dest,
        BACNET_BIT_STRING * src);
    BACNET_STACK_EXPORT
    bool bitstring_same(
        BACNET_BIT_STRING * bitstring1,
        BACNET_BIT_STRING * bitstring2);
    BACNET_STACK_EXPORT
    bool bitstring_init_ascii(
        BACNET_BIT_STRING * bit_string,
        const char *ascii);

/* returns false if the string exceeds capacity
   initialize by using length=0 */
    BACNET_STACK_EXPORT
    bool characterstring_init(
        BACNET_CHARACTER_STRING * char_string,
        uint8_t encoding,
        const char *value,
        size_t length);
/* used for ANSI C-Strings */
    BACNET_STACK_EXPORT
    bool characterstring_init_ansi(
        BACNET_CHARACTER_STRING * char_string,
        const char *value);
    bool characterstring_init_ansi_safe(
        BACNET_CHARACTER_STRING * char_string,
        const char *value,
        size_t tmax);
    bool characterstring_copy(
        BACNET_CHARACTER_STRING * dest,
        BACNET_CHARACTER_STRING * src);
    BACNET_STACK_EXPORT
    bool characterstring_ansi_copy(
        char *dest,
        size_t dest_max_len,
        BACNET_CHARACTER_STRING * src);
/* returns true if the strings are the same length, encoding, value */
    BACNET_STACK_EXPORT
    bool characterstring_same(
        BACNET_CHARACTER_STRING * dest,
        BACNET_CHARACTER_STRING * src);
    BACNET_STACK_EXPORT
    bool characterstring_ansi_same(
        BACNET_CHARACTER_STRING * dest,
        const char *src);
/* returns false if the string exceeds capacity */
    BACNET_STACK_EXPORT
    bool characterstring_append(
        BACNET_CHARACTER_STRING * char_string,
        const char *value,
        size_t length);
/* This function sets a new length without changing the value.
   If length exceeds capacity, no modification happens and
   function returns false.  */
    BACNET_STACK_EXPORT
    bool characterstring_truncate(
        BACNET_CHARACTER_STRING * char_string,
        size_t length);
    BACNET_STACK_EXPORT
    bool characterstring_set_encoding(
        BACNET_CHARACTER_STRING * char_string,
        uint8_t encoding);
/* Returns the value */
    BACNET_STACK_EXPORT
    char *characterstring_value(
        BACNET_CHARACTER_STRING * char_string);
/* returns the length */
    BACNET_STACK_EXPORT
    size_t characterstring_length(
        BACNET_CHARACTER_STRING * char_string);
    BACNET_STACK_EXPORT
    uint8_t characterstring_encoding(
        BACNET_CHARACTER_STRING * char_string);
    BACNET_STACK_EXPORT
    size_t characterstring_capacity(
        BACNET_CHARACTER_STRING * char_string);
    BACNET_STACK_EXPORT
    bool characterstring_printable(
        BACNET_CHARACTER_STRING * char_string);
    BACNET_STACK_EXPORT
    bool characterstring_valid(
        BACNET_CHARACTER_STRING * char_string);
    BACNET_STACK_EXPORT
    bool utf8_isvalid(
        const char *str,
        size_t length);

    /* returns false if the string exceeds capacity
       initialize by using length=0 */
    BACNET_STACK_EXPORT
    bool octetstring_init(
        BACNET_OCTET_STRING * octet_string,
        uint8_t * value,
        size_t length);
    /* converts an null terminated ASCII Hex string to an octet string.
       returns true if successfully converted and fits; false if too long */
    BACNET_STACK_EXPORT
    bool octetstring_init_ascii_hex(
        BACNET_OCTET_STRING * octet_string,
        const char *ascii_hex);
    BACNET_STACK_EXPORT
    bool octetstring_copy(
        BACNET_OCTET_STRING * dest,
        BACNET_OCTET_STRING * src);
    BACNET_STACK_EXPORT
    size_t octetstring_copy_value(
        uint8_t * dest,
        size_t length,
        BACNET_OCTET_STRING * src);
/* returns false if the string exceeds capacity */
    BACNET_STACK_EXPORT
    bool octetstring_append(
        BACNET_OCTET_STRING * octet_string,
        uint8_t * value,
        size_t length);
/* This function sets a new length without changing the value.
   If length exceeds capacity, no modification happens and
   function returns false.  */
    BACNET_STACK_EXPORT
    bool octetstring_truncate(
        BACNET_OCTET_STRING * octet_string,
        size_t length);
/* Returns the value */
    BACNET_STACK_EXPORT
    uint8_t *octetstring_value(
        BACNET_OCTET_STRING * octet_string);
/* Returns the length.*/
    BACNET_STACK_EXPORT
    size_t octetstring_length(
        BACNET_OCTET_STRING * octet_string);
    BACNET_STACK_EXPORT
    size_t octetstring_capacity(
        BACNET_OCTET_STRING * octet_string);
    /* returns true if the same length and contents */
    BACNET_STACK_EXPORT
    bool octetstring_value_same(
        BACNET_OCTET_STRING * octet_string1,
        BACNET_OCTET_STRING * octet_string2);

    BACNET_STACK_EXPORT
    int bacnet_stricmp(const char *s1, const char *s2);
    BACNET_STACK_EXPORT
    size_t bacnet_strnlen(const char *s, size_t maxlen);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
