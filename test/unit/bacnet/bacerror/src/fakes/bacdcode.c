/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>

#include "fakes/bacdcode.h"

#if 0
DEFINE_FAKE_VALUE_FUNC(int, encode_tag, uint8_t *, uint8_t, bool, uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, encode_opening_tag, uint8_t *, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, encode_closing_tag, uint8_t *, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, decode_tag_number, uint8_t *, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_tag_number_decode, uint8_t *, uint32_t, uint8_t *);
#endif
DEFINE_FAKE_VALUE_FUNC(
    int, decode_tag_number_and_value, uint8_t *, uint8_t *, uint32_t *);
#if 0
DEFINE_FAKE_VALUE_FUNC(int, bacnet_tag_number_and_value_decode, uint8_t *, uint32_t, uint8_t *, uint32_t *);
DEFINE_FAKE_VALUE_FUNC(bool, decode_is_opening_tag_number, uint8_t *, uint8_t);
DEFINE_FAKE_VALUE_FUNC(bool, decode_is_closing_tag_number, uint8_t *, uint8_t);
DEFINE_FAKE_VALUE_FUNC(bool, decode_is_context_tag, uint8_t *, uint8_t);
DEFINE_FAKE_VALUE_FUNC(bool, decode_is_context_tag_with_length, uint8_t *, uint8_t, int *);
DEFINE_FAKE_VALUE_FUNC(bool, decode_is_opening_tag, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(bool, decode_is_closing_tag, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(int, encode_application_null, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(int, encode_context_null, uint8_t *, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, encode_application_boolean, uint8_t *, bool);
DEFINE_FAKE_VALUE_FUNC(bool, decode_boolean, uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, encode_context_boolean, uint8_t *, uint8_t, bool);
DEFINE_FAKE_VALUE_FUNC(bool, decode_context_boolean, uint8_t *);
DEFINE_FAKE_VALUE_FUNC(int, decode_context_boolean2, uint8_t *, uint8_t, bool *);
DEFINE_FAKE_VALUE_FUNC(int, decode_bitstring, uint8_t *, uint32_t, BACNET_BIT_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, decode_context_bitstring, uint8_t *, uint8_t, BACNET_BIT_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, encode_bitstring, uint8_t *, BACNET_BIT_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, encode_application_bitstring, uint8_t *, BACNET_BIT_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, encode_context_bitstring, uint8_t *, uint8_t, BACNET_BIT_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, encode_application_real, uint8_t *, float);
DEFINE_FAKE_VALUE_FUNC(int, encode_context_real, uint8_t *, uint8_t, float);
DEFINE_FAKE_VALUE_FUNC(int, decode_context_real, uint8_t *, uint8_t, float *);
DEFINE_FAKE_VALUE_FUNC(int, encode_application_double, uint8_t *, double);
DEFINE_FAKE_VALUE_FUNC(int, encode_context_double, uint8_t *, uint8_t, double);
DEFINE_FAKE_VALUE_FUNC(int, decode_context_double, uint8_t *, uint8_t, double *);
DEFINE_FAKE_VALUE_FUNC(int, decode_object_id, uint8_t *, BACNET_OBJECT_TYPE *, uint32_t *);
DEFINE_FAKE_VALUE_FUNC(int, decode_object_id_safe, uint8_t *, uint32_t, BACNET_OBJECT_TYPE *, uint32_t *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_object_id_decode, uint8_t *, uint16_t, uint32_t, BACNET_OBJECT_TYPE *, uint32_t *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_object_id_application_decode, uint8_t *, uint16_t, BACNET_OBJECT_TYPE *, uint32_t *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_object_id_context_decode, uint8_t *, uint16_t, uint8_t, BACNET_OBJECT_TYPE *, uint32_t *);
DEFINE_FAKE_VALUE_FUNC(int, decode_context_object_id, uint8_t *, uint8_t, BACNET_OBJECT_TYPE *, uint32_t *);
DEFINE_FAKE_VALUE_FUNC(int, encode_bacnet_object_id, uint8_t *, BACNET_OBJECT_TYPE, uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, encode_context_object_id, uint8_t *, uint8_t, BACNET_OBJECT_TYPE, uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, encode_application_object_id, uint8_t *, BACNET_OBJECT_TYPE, uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, encode_octet_string, uint8_t *, BACNET_OCTET_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, encode_application_octet_string, uint8_t *, BACNET_OCTET_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, encode_context_octet_string, uint8_t *, uint8_t, BACNET_OCTET_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, decode_octet_string, uint8_t *, uint32_t, BACNET_OCTET_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, decode_context_octet_string, uint8_t *, uint8_t, BACNET_OCTET_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_octet_string_decode, uint8_t *, uint16_t, uint32_t, BACNET_OCTET_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_octet_string_application_decode, uint8_t *, uint16_t, BACNET_OCTET_STRING *);
DEFINE_FAKE_VALUE_FUNC(uint32_t, encode_bacnet_character_string_safe, uint8_t *apdu, uint32_t, uint8_t, char *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, encode_bacnet_character_string, uint8_t *, BACNET_CHARACTER_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, encode_application_character_string, uint8_t *, BACNET_CHARACTER_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, encode_context_character_string, uint8_t *, uint8_t, BACNET_CHARACTER_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, decode_character_string, uint8_t *, uint32_t, BACNET_CHARACTER_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, decode_context_character_string, uint8_t *, uint8_t, BACNET_CHARACTER_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_character_string_decode, uint8_t *, uint16_t, uint32_t, BACNET_CHARACTER_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_character_string_context_decode, uint8_t *, uint16_t, uint8_t, BACNET_CHARACTER_STRING *);
DEFINE_FAKE_VALUE_FUNC(int, encode_bacnet_unsigned, uint8_t *, BACNET_UNSIGNED_INTEGER);
DEFINE_FAKE_VALUE_FUNC(int, encode_context_unsigned, uint8_t *, uint8_t, BACNET_UNSIGNED_INTEGER);
DEFINE_FAKE_VALUE_FUNC(int, encode_application_unsigned, uint8_t *, BACNET_UNSIGNED_INTEGER);
DEFINE_FAKE_VALUE_FUNC(int, decode_unsigned, uint8_t *, uint32_t, BACNET_UNSIGNED_INTEGER *);
DEFINE_FAKE_VALUE_FUNC(int, decode_context_unsigned, uint8_t *, uint8_t, BACNET_UNSIGNED_INTEGER *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_unsigned_decode, uint8_t *, uint16_t, uint32_t, BACNET_UNSIGNED_INTEGER *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_unsigned_application_decode, uint8_t *, uint16_t, BACNET_UNSIGNED_INTEGER *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_unsigned_context_decode, uint8_t *, uint16_t, uint8_t, BACNET_UNSIGNED_INTEGER *);
DEFINE_FAKE_VALUE_FUNC(int, encode_bacnet_signed, uint8_t *, int32_t);
DEFINE_FAKE_VALUE_FUNC(int, encode_application_signed, uint8_t *, int32_t);
DEFINE_FAKE_VALUE_FUNC(int, encode_context_signed, uint8_t *, uint8_t, int32_t);
DEFINE_FAKE_VALUE_FUNC(int, decode_signed, uint8_t *, uint32_t, int32_t *);
DEFINE_FAKE_VALUE_FUNC(int, decode_context_signed, uint8_t *, uint8_t, int32_t *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_signed_decode, uint8_t *, uint16_t, uint32_t, int32_t *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_signed_context_decode, uint8_t *, uint16_t, uint8_t, int32_t *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_signed_application_decode, uint8_t *, uint16_t, int32_t *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_enumerated_decode, uint8_t *, uint16_t, uint32_t, uint32_t *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_enumerated_context_decode, uint8_t *, uint16_t, uint8_t, uint32_t *);
#endif
DEFINE_FAKE_VALUE_FUNC(int, decode_enumerated, uint8_t *, uint32_t, uint32_t *);
#if 0
DEFINE_FAKE_VALUE_FUNC(int, decode_context_enumerated, uint8_t *, uint8_t, uint32_t *);
DEFINE_FAKE_VALUE_FUNC(int, encode_bacnet_enumerated, uint8_t *, uint32_t);
#endif
DEFINE_FAKE_VALUE_FUNC(int, encode_application_enumerated, uint8_t *, uint32_t);
#if 0
DEFINE_FAKE_VALUE_FUNC(int, encode_context_enumerated, uint8_t *, uint8_t, uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, encode_bacnet_time, uint8_t *, BACNET_TIME *);
DEFINE_FAKE_VALUE_FUNC(int, encode_application_time, uint8_t *, BACNET_TIME *);
DEFINE_FAKE_VALUE_FUNC(int, decode_bacnet_time, uint8_t *, BACNET_TIME *);
DEFINE_FAKE_VALUE_FUNC(int, decode_bacnet_time_safe, uint8_t *, uint32_t, BACNET_TIME *);
DEFINE_FAKE_VALUE_FUNC(int, encode_context_time, uint8_t *, uint8_t, BACNET_TIME *);
DEFINE_FAKE_VALUE_FUNC(int, decode_application_time, uint8_t *, BACNET_TIME *);
DEFINE_FAKE_VALUE_FUNC(int, decode_context_bacnet_time, uint8_t *, uint8_t, BACNET_TIME *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_time_decode, uint8_t *, uint16_t, uint32_t, BACNET_TIME *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_time_context_decode, uint8_t *, uint16_t, uint8_t, BACNET_TIME *);
DEFINE_FAKE_VALUE_FUNC(int, bacnet_time_application_decode, uint8_t *, uint16_t, BACNET_TIME *);
DEFINE_FAKE_VALUE_FUNC(int, encode_bacnet_date, uint8_t *, BACNET_DATE *);
DEFINE_FAKE_VALUE_FUNC(int, encode_application_date, uint8_t *, BACNET_DATE *);
DEFINE_FAKE_VALUE_FUNC(int, encode_context_date, uint8_t *, uint8_t, BACNET_DATE *);
DEFINE_FAKE_VALUE_FUNC(int, decode_date, uint8_t *, BACNET_DATE *);
DEFINE_FAKE_VALUE_FUNC(int, decode_date_safe, uint8_t *, uint32_t, BACNET_DATE *);
DEFINE_FAKE_VALUE_FUNC(int, decode_application_date, uint8_t *, BACNET_DATE *);
DEFINE_FAKE_VALUE_FUNC(int, decode_context_date, uint8_t *, uint8_t, BACNET_DATE *);
DEFINE_FAKE_VALUE_FUNC(uint8_t, encode_max_segs_max_apdu, int, int);
DEFINE_FAKE_VALUE_FUNC(int, decode_max_segs, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, decode_max_apdu, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, encode_simple_ack, uint8_t *, uint8_t, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, encode_bacnet_address, uint8_t *, BACNET_ADDRESS *);
DEFINE_FAKE_VALUE_FUNC(int, decode_bacnet_address, uint8_t *, BACNET_ADDRESS *);
DEFINE_FAKE_VALUE_FUNC(int, encode_context_bacnet_address, uint8_t *, uint8_t, BACNET_ADDRESS *);
DEFINE_FAKE_VALUE_FUNC(int, decode_context_bacnet_address, uint8_t *, uint8_t, BACNET_ADDRESS *);
#endif
