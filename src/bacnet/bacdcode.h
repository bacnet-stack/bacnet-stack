/**
 * @file
 * @brief BACnet primitive data encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_CODEC_H
#define BACNET_CODEC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/datetime.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacint.h"
#include "bacnet/bacreal.h"

/**
 * @brief Encode a BACnetARRAY property element; a function template
 * @param object_instance [in] BACnet network port object instance number
 * @param array_index [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
typedef int (*bacnet_array_property_element_encode_function)(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu);

typedef struct BACnetTag {
    uint8_t number;
    bool application : 1;
    bool context : 1;
    bool opening : 1;
    bool closing : 1;
    uint32_t len_value_type;
} BACNET_TAG;

/* max size of a BACnet tag */
#define BACNET_TAG_SIZE 7

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int encode_tag(uint8_t *apdu,
    uint8_t tag_number,
    bool context_specific,
    uint32_t len_value_type);
BACNET_STACK_EXPORT
int encode_opening_tag(uint8_t *apdu, uint8_t tag_number);
BACNET_STACK_EXPORT
int encode_closing_tag(uint8_t *apdu, uint8_t tag_number);

BACNET_STACK_EXPORT
int bacnet_tag_encode(uint8_t *apdu, uint32_t apdu_size, BACNET_TAG *tag);
BACNET_STACK_EXPORT
int bacnet_tag_decode(uint8_t *apdu, uint32_t apdu_size, BACNET_TAG *tag);

BACNET_STACK_EXPORT
bool bacnet_is_opening_tag(uint8_t *apdu, uint32_t apdu_size);
BACNET_STACK_EXPORT
bool bacnet_is_closing_tag(uint8_t *apdu, uint32_t apdu_size);
BACNET_STACK_EXPORT
bool bacnet_is_context_specific(uint8_t *apdu, uint32_t apdu_size);

BACNET_STACK_EXPORT
bool bacnet_is_context_tag_number(
    uint8_t *apdu, uint32_t apdu_size, uint8_t tag_number, int *tag_length,
    uint32_t *len_value_type);
BACNET_STACK_EXPORT
int bacnet_tag_number_decode(
    uint8_t *apdu, uint32_t apdu_size, uint8_t *tag_number);
BACNET_STACK_EXPORT
bool bacnet_is_opening_tag_number(
    uint8_t *apdu, uint32_t apdu_size, uint8_t tag_number, int *tag_length);
BACNET_STACK_EXPORT
bool bacnet_is_closing_tag_number(
    uint8_t *apdu, uint32_t apdu_size, uint8_t tag_number, int *tag_length);

BACNET_STACK_EXPORT
int bacnet_application_data_length(
    uint8_t tag_number, uint32_t len_value_type);
BACNET_STACK_EXPORT
int bacnet_enclosed_data_length(
    uint8_t *apdu, size_t apdu_size);

BACNET_STACK_DEPRECATED("Use bacnet_tag_decode() instead")
BACNET_STACK_EXPORT
int bacnet_tag_number_and_value_decode(
    uint8_t *apdu, uint32_t apdu_size, uint8_t *tag_number, uint32_t *value);
BACNET_STACK_DEPRECATED("Use bacnet_tag_number_decode() instead")
BACNET_STACK_EXPORT
int decode_tag_number(uint8_t *apdu, uint8_t *tag_number);
BACNET_STACK_DEPRECATED("Use bacnet_tag_decode() instead")
BACNET_STACK_EXPORT
int decode_tag_number_and_value(
    uint8_t *apdu, uint8_t *tag_number, uint32_t *value);
BACNET_STACK_DEPRECATED("Use bacnet_is_opening_tag_number() instead")
BACNET_STACK_EXPORT
bool decode_is_opening_tag_number(uint8_t *apdu, uint8_t tag_number);
BACNET_STACK_DEPRECATED("Use bacnet_is_closing_tag_number() instead")
BACNET_STACK_EXPORT
bool decode_is_closing_tag_number(uint8_t *apdu, uint8_t tag_number);
BACNET_STACK_DEPRECATED("Use bacnet_is_context_tag_number() instead")
BACNET_STACK_EXPORT
bool decode_is_context_tag(uint8_t *apdu, uint8_t tag_number);
BACNET_STACK_DEPRECATED("Use bacnet_is_context_tag_number() instead")
BACNET_STACK_EXPORT
bool decode_is_context_tag_with_length(
    uint8_t *apdu, uint8_t tag_number, int *tag_length);
BACNET_STACK_DEPRECATED("Use bacnet_is_opening_tag() instead")
BACNET_STACK_EXPORT
bool decode_is_opening_tag(uint8_t *apdu);
BACNET_STACK_DEPRECATED("Use bacnet_is_closing_tag() instead")
BACNET_STACK_EXPORT
bool decode_is_closing_tag(uint8_t *apdu);

BACNET_STACK_EXPORT
int encode_application_null(uint8_t *apdu);
BACNET_STACK_EXPORT
int encode_context_null(uint8_t *apdu, uint8_t tag_number);

/* from clause 20.2.3 Encoding of a Boolean Value */
BACNET_STACK_EXPORT
int encode_application_boolean(uint8_t *apdu, bool boolean_value);
BACNET_STACK_EXPORT
bool decode_boolean(uint32_t len_value);
BACNET_STACK_EXPORT
int encode_context_boolean(
    uint8_t *apdu, uint8_t tag_number, bool boolean_value);
BACNET_STACK_DEPRECATED("Use bacnet_boolean_context_decode() instead")
BACNET_STACK_EXPORT
bool decode_context_boolean(uint8_t *apdu);
BACNET_STACK_EXPORT
BACNET_STACK_DEPRECATED("Use bacnet_boolean_context_decode() instead")
int decode_context_boolean2(
    uint8_t *apdu, uint8_t tag_number, bool *boolean_value);

BACNET_STACK_EXPORT
int bacnet_boolean_application_encode(
    uint8_t *apdu, uint32_t apdu_size, bool value);
BACNET_STACK_EXPORT
int bacnet_boolean_application_decode(
    uint8_t *apdu, uint32_t apdu_len_max, bool *value);
BACNET_STACK_EXPORT
int bacnet_boolean_context_decode(uint8_t *apdu,
    uint32_t apdu_len_max,
    uint8_t tag_value,
    bool *boolean_value);

BACNET_STACK_EXPORT
int encode_bitstring(uint8_t *apdu, BACNET_BIT_STRING *bit_string);
BACNET_STACK_EXPORT
int encode_application_bitstring(uint8_t *apdu, BACNET_BIT_STRING *bit_string);
BACNET_STACK_EXPORT
int encode_context_bitstring(
    uint8_t *apdu, uint8_t tag_number, BACNET_BIT_STRING *bit_string);
BACNET_STACK_DEPRECATED("Use bacnet_bitstring_decode() instead")
BACNET_STACK_EXPORT
int decode_bitstring(
    uint8_t *apdu, uint32_t len_value, BACNET_BIT_STRING *bit_string);
BACNET_STACK_DEPRECATED("Use bacnet_bitstring_context_decode() instead")
BACNET_STACK_EXPORT
int decode_context_bitstring(
    uint8_t *apdu, uint8_t tag_number, BACNET_BIT_STRING *bit_string);
BACNET_STACK_EXPORT
int bacnet_bitstring_decode(uint8_t *apdu,
    uint32_t apdu_len_max,
    uint32_t len_value,
    BACNET_BIT_STRING *value);

BACNET_STACK_EXPORT
int bacnet_bitstring_application_encode(
    uint8_t *apdu, uint32_t apdu_size, BACNET_BIT_STRING *value);
BACNET_STACK_EXPORT
int bacnet_bitstring_application_decode(
    uint8_t *apdu, uint32_t apdu_len_max, BACNET_BIT_STRING *value);
BACNET_STACK_EXPORT
int bacnet_bitstring_context_decode(
    uint8_t *apdu,
    uint32_t apdu_len_max,
    uint8_t tag_value,
    BACNET_BIT_STRING *value);

BACNET_STACK_EXPORT
int encode_application_real(uint8_t *apdu, float value);
BACNET_STACK_EXPORT
int encode_context_real(uint8_t *apdu, uint8_t tag_number, float value);
BACNET_STACK_DEPRECATED("Use bacnet_real_context_decode() instead")
BACNET_STACK_EXPORT
int decode_context_real(uint8_t *apdu, uint8_t tag_number, float *real_value);
BACNET_STACK_EXPORT
int bacnet_real_decode(
    uint8_t *apdu, uint32_t apdu_len_max, uint32_t len_value, float *value);
BACNET_STACK_EXPORT
int bacnet_real_context_decode(
    uint8_t *apdu, uint32_t apdu_len_max, uint8_t tag_value, float *value);
BACNET_STACK_EXPORT
int bacnet_real_application_encode(
    uint8_t *apdu, uint32_t apdu_size, float value);
BACNET_STACK_EXPORT
int bacnet_real_application_decode(
    uint8_t *apdu, uint32_t apdu_len_max, float *value);

BACNET_STACK_EXPORT
int encode_application_double(uint8_t *apdu, double value);
BACNET_STACK_EXPORT
int encode_context_double(uint8_t *apdu, uint8_t tag_number, double value);
BACNET_STACK_DEPRECATED("Use bacnet_double_context_decode() instead")
BACNET_STACK_EXPORT
int decode_context_double(
    uint8_t *apdu, uint8_t tag_number, double *double_value);
BACNET_STACK_EXPORT
int bacnet_double_decode(
    uint8_t *apdu, uint32_t apdu_len_max, uint32_t len_value, double *value);
BACNET_STACK_EXPORT
int bacnet_double_context_decode(
    uint8_t *apdu, uint32_t apdu_len_max, uint8_t tag_value, double *value);
BACNET_STACK_EXPORT
int bacnet_double_application_encode(
    uint8_t *apdu, uint32_t apdu_size, double value);
BACNET_STACK_EXPORT
int bacnet_double_application_decode(
    uint8_t *apdu, uint32_t apdu_len_max, double *value);

BACNET_STACK_EXPORT
int encode_bacnet_object_id(
    uint8_t *apdu, BACNET_OBJECT_TYPE object_type, uint32_t instance);
BACNET_STACK_EXPORT
int encode_context_object_id(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_OBJECT_TYPE object_type,
    uint32_t instance);
BACNET_STACK_EXPORT
int encode_application_object_id(
    uint8_t *apdu, BACNET_OBJECT_TYPE object_type, uint32_t instance);
BACNET_STACK_DEPRECATED("Use bacnet_object_id_decode() instead")
BACNET_STACK_EXPORT
int decode_object_id(
    uint8_t *apdu, BACNET_OBJECT_TYPE *object_type, uint32_t *object_instance);
BACNET_STACK_DEPRECATED("Use bacnet_object_id_context_decode() instead")
BACNET_STACK_EXPORT
int decode_context_object_id(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *instance);
BACNET_STACK_EXPORT
int decode_object_id_safe(uint8_t *apdu,
    uint32_t len_value,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *object_instance);
BACNET_STACK_EXPORT
int bacnet_object_id_decode(uint8_t *apdu,
    uint32_t apdu_len_max,
    uint32_t len_value,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *instance);

BACNET_STACK_EXPORT
int bacnet_object_id_application_encode(
    uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance);
BACNET_STACK_EXPORT
int bacnet_object_id_application_decode(
    uint8_t *apdu,
    uint32_t apdu_len_max,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *object_instance);
BACNET_STACK_EXPORT
int bacnet_object_id_context_decode(
    uint8_t *apdu,
    uint32_t apdu_len_max,
    uint8_t tag_value,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *instance);

BACNET_STACK_EXPORT
int encode_octet_string(uint8_t *apdu, BACNET_OCTET_STRING *octet_string);
BACNET_STACK_EXPORT
int encode_application_octet_string(
    uint8_t *apdu, BACNET_OCTET_STRING *octet_string);
BACNET_STACK_EXPORT
int encode_context_octet_string(
    uint8_t *apdu, uint8_t tag_number, BACNET_OCTET_STRING *octet_string);
BACNET_STACK_EXPORT
BACNET_STACK_DEPRECATED("Use bacnet_octet_string_decode() instead")
int decode_octet_string(
    uint8_t *apdu, uint32_t len_value, BACNET_OCTET_STRING *octet_string);
BACNET_STACK_DEPRECATED("Use bacnet_octet_string_context_decode() instead")
BACNET_STACK_EXPORT
int decode_context_octet_string(
    uint8_t *apdu, uint8_t tag_number, BACNET_OCTET_STRING *octet_string);
BACNET_STACK_EXPORT
int bacnet_octet_string_decode(uint8_t *apdu,
    uint32_t apdu_len_max,
    uint32_t len_value,
    BACNET_OCTET_STRING *value);

BACNET_STACK_EXPORT
int bacnet_octet_string_application_encode(
    uint8_t *apdu, uint32_t apdu_size, BACNET_OCTET_STRING *value);
BACNET_STACK_EXPORT
int bacnet_octet_string_application_decode(
    uint8_t *apdu, uint32_t apdu_len_max, BACNET_OCTET_STRING *value);
BACNET_STACK_EXPORT
int bacnet_octet_string_context_decode(
    uint8_t *apdu,
    uint32_t apdu_len_max,
    uint8_t tag_value,
    BACNET_OCTET_STRING *value);

BACNET_STACK_EXPORT
uint32_t encode_bacnet_character_string_safe(uint8_t *apdu,
    uint32_t max_apdu,
    uint8_t encoding,
    char *pString,
    uint32_t length);
BACNET_STACK_EXPORT
int encode_bacnet_character_string(
    uint8_t *apdu, BACNET_CHARACTER_STRING *char_string);
BACNET_STACK_EXPORT
int encode_application_character_string(
    uint8_t *apdu, BACNET_CHARACTER_STRING *char_string);
BACNET_STACK_EXPORT
int encode_context_character_string(
    uint8_t *apdu, uint8_t tag_number, BACNET_CHARACTER_STRING *char_string);
BACNET_STACK_DEPRECATED("Use bacnet_character_string_decode() instead")
BACNET_STACK_EXPORT
int decode_character_string(
    uint8_t *apdu, uint32_t len_value, BACNET_CHARACTER_STRING *char_string);
BACNET_STACK_DEPRECATED("Use bacnet_character_string_context_decode() instead")
BACNET_STACK_EXPORT
int decode_context_character_string(
    uint8_t *apdu, uint8_t tag_number, BACNET_CHARACTER_STRING *char_string);
BACNET_STACK_EXPORT
int bacnet_character_string_decode(uint8_t *apdu,
    uint32_t apdu_len_max,
    uint32_t len_value,
    BACNET_CHARACTER_STRING *char_string);

BACNET_STACK_EXPORT
int bacnet_character_string_application_encode(
    uint8_t *apdu, uint32_t apdu_size, BACNET_CHARACTER_STRING *value);
BACNET_STACK_EXPORT
int bacnet_character_string_application_decode(
    uint8_t *apdu, uint32_t apdu_len_max, BACNET_CHARACTER_STRING *value);
BACNET_STACK_EXPORT
int bacnet_character_string_context_decode(
    uint8_t *apdu,
    uint32_t apdu_len_max,
    uint8_t tag_value,
    BACNET_CHARACTER_STRING *value);

BACNET_STACK_EXPORT
int encode_bacnet_unsigned(uint8_t *apdu, BACNET_UNSIGNED_INTEGER value);
BACNET_STACK_EXPORT
int encode_context_unsigned(
    uint8_t *apdu, uint8_t tag_number, BACNET_UNSIGNED_INTEGER value);
BACNET_STACK_EXPORT
int encode_application_unsigned(uint8_t *apdu, BACNET_UNSIGNED_INTEGER value);
BACNET_STACK_DEPRECATED("Use bacnet_unsigned_decode() instead")
BACNET_STACK_EXPORT
int decode_unsigned(
    uint8_t *apdu, uint32_t len_value, BACNET_UNSIGNED_INTEGER *value);
BACNET_STACK_DEPRECATED("Use bacnet_unsigned_context_decode() instead")
BACNET_STACK_EXPORT
int decode_context_unsigned(
    uint8_t *apdu, uint8_t tag_number, BACNET_UNSIGNED_INTEGER *value);
BACNET_STACK_EXPORT
int bacnet_unsigned_decode(uint8_t *apdu,
    uint32_t apdu_max_len,
    uint32_t len_value,
    BACNET_UNSIGNED_INTEGER *value);

BACNET_STACK_EXPORT
int bacnet_unsigned_application_encode(
    uint8_t *apdu, uint32_t apdu_size, BACNET_UNSIGNED_INTEGER value);
BACNET_STACK_EXPORT
int bacnet_unsigned_application_decode(
    uint8_t *apdu, uint32_t apdu_len_max, BACNET_UNSIGNED_INTEGER *value);
BACNET_STACK_EXPORT
int bacnet_unsigned_context_decode(
    uint8_t *apdu,
    uint32_t apdu_len_max,
    uint8_t tag_number,
    BACNET_UNSIGNED_INTEGER *value);

BACNET_STACK_EXPORT
int encode_bacnet_signed(uint8_t *apdu, int32_t value);
BACNET_STACK_EXPORT
int encode_application_signed(uint8_t *apdu, int32_t value);
BACNET_STACK_EXPORT
int encode_context_signed(uint8_t *apdu, uint8_t tag_number, int32_t value);
BACNET_STACK_DEPRECATED("Use bacnet_signed_decode() instead")
BACNET_STACK_EXPORT
int decode_signed(uint8_t *apdu, uint32_t len_value, int32_t *value);
BACNET_STACK_DEPRECATED("Use bacnet_signed_context_decode() instead")
BACNET_STACK_EXPORT
int decode_context_signed(uint8_t *apdu, uint8_t tag_number, int32_t *value);

BACNET_STACK_EXPORT
int bacnet_signed_decode(
    uint8_t *apdu, uint32_t apdu_size, uint32_t len_value, int32_t *value);
BACNET_STACK_EXPORT
int bacnet_signed_context_decode(
    uint8_t *apdu, uint32_t apdu_size, uint8_t tag_value, int32_t *value);
BACNET_STACK_EXPORT
int bacnet_signed_application_encode(
    uint8_t *apdu, uint32_t apdu_size, int32_t value);
BACNET_STACK_EXPORT
int bacnet_signed_application_decode(
    uint8_t *apdu, uint32_t apdu_size, int32_t *value);

BACNET_STACK_EXPORT
int encode_bacnet_enumerated(uint8_t *apdu, uint32_t value);
BACNET_STACK_EXPORT
int encode_application_enumerated(uint8_t *apdu, uint32_t value);
BACNET_STACK_EXPORT
int encode_context_enumerated(
    uint8_t *apdu, uint8_t tag_number, uint32_t value);
BACNET_STACK_DEPRECATED("Use bacnet_enumerated_decode() instead")
BACNET_STACK_EXPORT
int decode_enumerated(uint8_t *apdu, uint32_t len_value, uint32_t *value);
BACNET_STACK_DEPRECATED("Use bacnet_enumerated_context_decode() instead")
BACNET_STACK_EXPORT
int decode_context_enumerated(
    uint8_t *apdu, uint8_t tag_value, uint32_t *value);
BACNET_STACK_EXPORT
int bacnet_enumerated_decode(
    uint8_t *apdu, uint32_t apdu_max_len, uint32_t len_value, uint32_t *value);


BACNET_STACK_EXPORT
int bacnet_enumerated_application_encode(
    uint8_t *apdu, uint32_t apdu_size, uint32_t value);
BACNET_STACK_EXPORT
int bacnet_enumerated_application_decode(
    uint8_t *apdu, uint32_t apdu_len_max, uint32_t *value);
BACNET_STACK_EXPORT
int bacnet_enumerated_context_decode(
    uint8_t *apdu, uint32_t apdu_len_max, uint8_t tag_value, uint32_t *value);

BACNET_STACK_EXPORT
int encode_bacnet_time(uint8_t *apdu, BACNET_TIME *btime);
BACNET_STACK_EXPORT
int encode_application_time(uint8_t *apdu, BACNET_TIME *btime);
BACNET_STACK_EXPORT
int encode_context_time(uint8_t *apdu, uint8_t tag_number, BACNET_TIME *btime);
BACNET_STACK_DEPRECATED("Use bacnet_time_decode() instead")
BACNET_STACK_EXPORT
int decode_bacnet_time(uint8_t *apdu, BACNET_TIME *btime);
BACNET_STACK_DEPRECATED("Use bacnet_time_decode() instead")
BACNET_STACK_EXPORT
int decode_bacnet_time_safe(
    uint8_t *apdu, uint32_t len_value, BACNET_TIME *btime);
BACNET_STACK_DEPRECATED("Use bacnet_time_application_decode() instead")
BACNET_STACK_EXPORT
int decode_application_time(uint8_t *apdu, BACNET_TIME *btime);
BACNET_STACK_DEPRECATED("Use bacnet_time_context_decode() instead")
BACNET_STACK_EXPORT
int decode_context_bacnet_time(
    uint8_t *apdu, uint8_t tag_number, BACNET_TIME *btime);
BACNET_STACK_EXPORT
int bacnet_time_decode(uint8_t *apdu,
    uint32_t apdu_len_max,
    uint32_t len_value,
    BACNET_TIME *value);
BACNET_STACK_EXPORT
int bacnet_time_context_decode(uint8_t *apdu,
    uint32_t apdu_len_max,
    uint8_t tag_value,
    BACNET_TIME *value);
BACNET_STACK_EXPORT
int bacnet_time_application_encode(
    uint8_t *apdu, uint32_t apdu_size, BACNET_TIME *value);
BACNET_STACK_EXPORT
int bacnet_time_application_decode(
    uint8_t *apdu, uint32_t apdu_len_max, BACNET_TIME *value);

BACNET_STACK_EXPORT
int encode_bacnet_date(uint8_t *apdu, BACNET_DATE *bdate);
BACNET_STACK_EXPORT
int encode_application_date(uint8_t *apdu, BACNET_DATE *bdate);
BACNET_STACK_EXPORT
int encode_context_date(uint8_t *apdu, uint8_t tag_number, BACNET_DATE *bdate);
BACNET_STACK_DEPRECATED("Use bacnet_date_decode() instead")
BACNET_STACK_EXPORT
int decode_date(uint8_t *apdu, BACNET_DATE *bdate);
BACNET_STACK_DEPRECATED("Use bacnet_date_decode() instead")
BACNET_STACK_EXPORT
int decode_date_safe(uint8_t *apdu, uint32_t len_value, BACNET_DATE *bdate);
BACNET_STACK_DEPRECATED("Use bacnet_date_application_decode() instead")
BACNET_STACK_EXPORT
int decode_application_date(uint8_t *apdu, BACNET_DATE *bdate);
BACNET_STACK_DEPRECATED("Use bacnet_date_context_decode() instead")
BACNET_STACK_EXPORT
int decode_context_date(uint8_t *apdu, uint8_t tag_number, BACNET_DATE *bdate);
BACNET_STACK_EXPORT
int bacnet_date_decode(uint8_t *apdu,
    uint32_t apdu_len_max,
    uint32_t len_value,
    BACNET_DATE *value);

BACNET_STACK_EXPORT
int bacnet_date_application_encode(
    uint8_t *apdu, uint32_t apdu_size, BACNET_DATE *value);
BACNET_STACK_EXPORT
int bacnet_date_application_decode(
    uint8_t *apdu, uint32_t apdu_len_max, BACNET_DATE *value);
BACNET_STACK_EXPORT
int bacnet_date_context_decode(uint8_t *apdu,
    uint32_t apdu_len_max,
    uint8_t tag_value,
    BACNET_DATE *value);

/* from clause 20.1.2.4 max-segments-accepted */
/* and clause 20.1.2.5 max-APDU-length-accepted */
/* returns the encoded octet */
BACNET_STACK_EXPORT
uint8_t encode_max_segs_max_apdu(int max_segs, int max_apdu);
BACNET_STACK_EXPORT
int decode_max_segs(uint8_t octet);
BACNET_STACK_EXPORT
int decode_max_apdu(uint8_t octet);

/* returns the number of apdu bytes consumed */
BACNET_STACK_EXPORT
int encode_simple_ack(uint8_t *apdu, uint8_t invoke_id, uint8_t service_choice);

BACNET_STACK_EXPORT
int bacnet_array_encode(uint32_t object_instance,
    BACNET_ARRAY_INDEX array_index,
    bacnet_array_property_element_encode_function encoder,
    BACNET_UNSIGNED_INTEGER array_size,
    uint8_t *apdu,
    int max_apdu);

/* from clause 20.2.1.2 Tag Number */
/* true if extended tag numbering is used */
#define IS_EXTENDED_TAG_NUMBER(x) (((x)&0xF0) == 0xF0)

/* from clause 20.2.1.3.1 Primitive Data */
/* true if the extended value is used */
#define IS_EXTENDED_VALUE(x) (((x)&0x07) == 5)

/* from clause 20.2.1.1 Class */
/* true if the tag is context specific */
#define IS_CONTEXT_SPECIFIC(x) (((x)&BIT(3)) == BIT(3))

/* from clause 20.2.1.3.2 Constructed Data */
/* true if the tag is an opening tag */
#define IS_OPENING_TAG(x) (((x)&0x07) == 6)

/* from clause 20.2.1.3.2 Constructed Data */
/* true if the tag is a closing tag */
#define IS_CLOSING_TAG(x) (((x)&0x07) == 7)

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
