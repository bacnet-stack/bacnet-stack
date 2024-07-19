/**
 * @file
 * @brief Handle Get/Set of BACnet application encoded settings
 * @date May 2024
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <bacnet_settings/bacnet_storage.h>
#include <bacnet_settings/bacnet_settings.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacint.h"

/**
 * @brief Get a BACnet SIGNED INTEGER value from non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param array_index [in] The BACnet array index
 * @param default_value [in] The default value if not found
 * @return stored data length on success 0..N, negative on failure.
 */
int bacnet_settings_value_get(uint16_t object_type, uint32_t object_instance,
			      uint32_t property_id, uint32_t array_index,
			      BACNET_APPLICATION_DATA_VALUE *value)
{
	uint8_t name[BACNET_STORAGE_VALUE_SIZE_MAX + 1] = { 0 };
	BACNET_STORAGE_KEY key = { 0 };
	int stored_len, len;

	bacnet_storage_key_init(&key, object_type, object_instance, property_id,
				array_index);
	stored_len = bacnet_storage_get(&key, name, sizeof(name));
	if (stored_len > 0) {
		len = bacapp_decode_application_data(name, stored_len, value);
		if (len <= 0) {
			if (value) {
				value->tag = MAX_BACNET_APPLICATION_TAG;
			}
		}
	}

	return stored_len;
}

/**
 * @brief Store a BACnet SIGNED INTEGER value in non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param array_index [in] The BACnet array index
 * @param value [in] The value to store
 * @return true on success, false on failure.
 */
bool bacnet_settings_value_set(uint16_t object_type, uint32_t object_instance,
			       uint32_t property_id, uint32_t array_index,
			       BACNET_APPLICATION_DATA_VALUE *value)
{
	uint8_t name[BACNET_STORAGE_VALUE_SIZE_MAX] = { 0 };
	BACNET_STORAGE_KEY key = { 0 };
	int rc, len;

	bacnet_storage_key_init(&key, object_type, object_instance, property_id,
				array_index);
	len = bacapp_encode_application_data(NULL, value);
	if (len <= 0) {
		return false;
	} else if (len > sizeof(name)) {
		return false;
	}
	len = bacapp_encode_application_data(name, value);
	rc = bacnet_storage_set(&key, name, len);

	return rc == 0;
}

/**
 * @brief Get a BACnet REAL value from non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param array_index [in] The BACnet array index
 * @param default_value [in] The default value if not found
 * @return stored data length on success 0..N, negative on failure.
 */
int bacnet_settings_real_get(uint16_t object_type, uint32_t object_instance,
			     uint32_t property_id, uint32_t array_index,
			     float default_value, float *value)
{
	int stored_len;
	BACNET_APPLICATION_DATA_VALUE bvalue = { 0 };

	stored_len =
		bacnet_settings_value_get(object_type, object_instance,
					  property_id, array_index, &bvalue);
	if ((stored_len >= 0) && (bvalue.tag == BACNET_APPLICATION_TAG_REAL)) {
		if (value) {
			*value = bvalue.type.Real;
		}
	} else {
		if (value) {
			*value = default_value;
		}
	}

	return stored_len;
}

/**
 * @brief Store a BACnet REAL value in non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param array_index [in] The BACnet array index
 * @param value [in] The value to store
 * @return true on success, false on failure.
 */
bool bacnet_settings_real_set(uint16_t object_type, uint32_t object_instance,
			      uint32_t property_id, uint32_t array_index,
			      float value)
{
	BACNET_APPLICATION_DATA_VALUE bvalue = { 0 };

	bvalue.context_specific = false;
	bvalue.tag = BACNET_APPLICATION_TAG_REAL;
	bvalue.type.Real = value;

	return bacnet_settings_value_set(object_type, object_instance,
					 property_id, array_index, &bvalue);
}

/**
 * @brief Get a BACnet UNSIGNED value from non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param default_value [in] The default value if not found
 * @return stored data length on success 0..N, negative on failure.
 */
int bacnet_settings_unsigned_get(uint16_t object_type, uint32_t object_instance,
				 uint32_t property_id, uint32_t array_index,
				 BACNET_UNSIGNED_INTEGER default_value,
				 BACNET_UNSIGNED_INTEGER *value)
{
	uint8_t name[BACNET_STORAGE_VALUE_SIZE_MAX + 1] = { 0 };
	BACNET_STORAGE_KEY key = { 0 };
	int stored_len, len;

	bacnet_storage_key_init(&key, object_type, object_instance, property_id,
				array_index);
	stored_len = bacnet_storage_get(&key, name, sizeof(name));
	if (stored_len > 0) {
		len = bacnet_unsigned_application_decode(name, stored_len,
							 value);
		if (len <= 0) {
			if (value) {
				*value = default_value;
			}
		}
	} else {
		if (value) {
			*value = default_value;
		}
	}

	return stored_len;
}

/**
 * @brief Store a BACnet UNSIGNED value in non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param value [int] The value to store
 * @return true on success, false on failure.
 */
bool bacnet_settings_unsigned_set(uint16_t object_type,
				  uint32_t object_instance,
				  uint32_t property_id, uint32_t array_index,
				  BACNET_UNSIGNED_INTEGER value)
{
	BACNET_APPLICATION_DATA_VALUE bvalue = { 0 };

	bvalue.context_specific = false;
	bvalue.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
	bvalue.type.Unsigned_Int = value;

	return bacnet_settings_value_set(object_type, object_instance,
					 property_id, array_index, &bvalue);
}

/**
 * @brief Get a BACnet SIGNED INTEGER value from non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param array_index [in] The BACnet array index
 * @param default_value [in] The default value if not found
 * @return stored data length on success 0..N, negative on failure.
 */
int bacnet_settings_signed_get(uint16_t object_type, uint32_t object_instance,
			       uint32_t property_id, uint32_t array_index,
			       int32_t default_value, int32_t *value)
{
	uint8_t name[BACNET_STORAGE_VALUE_SIZE_MAX + 1] = { 0 };
	BACNET_STORAGE_KEY key = { 0 };
	int stored_len, len;

	bacnet_storage_key_init(&key, object_type, object_instance, property_id,
				array_index);
	stored_len = bacnet_storage_get(&key, name, sizeof(name));
	if (stored_len > 0) {
		len = bacnet_signed_application_decode(name, stored_len, value);
		if (len <= 0) {
			if (value) {
				*value = default_value;
			}
		}
	} else {
		if (value) {
			*value = default_value;
		}
	}

	return stored_len;
}

/**
 * @brief Store a BACnet SIGNED INTEGER value in non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param array_index [in] The BACnet array index
 * @param value [in] The value to store
 * @return true on success, false on failure.
 */
bool bacnet_settings_signed_set(uint16_t object_type, uint32_t object_instance,
				uint32_t property_id, uint32_t array_index,
				int32_t value)
{
	BACNET_APPLICATION_DATA_VALUE bvalue = { 0 };

	bvalue.context_specific = false;
	bvalue.tag = BACNET_APPLICATION_TAG_SIGNED_INT;
	bvalue.type.Signed_Int = value;

	return bacnet_settings_value_set(object_type, object_instance,
					 property_id, array_index, &bvalue);
}

/**
 * @brief Get a BACnet CHARACTER_STRING value from non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param default_value [in] The default value if not found
 * @param value [out] The character string value
 * @return stored data length on success 0..N, negative on failure.
 */
int bacnet_settings_characterstring_get(uint16_t object_type,
					uint32_t object_instance,
					uint32_t property_id,
					uint32_t array_index,
					const char *default_value,
					BACNET_CHARACTER_STRING *value)
{
	uint8_t name[BACNET_STORAGE_VALUE_SIZE_MAX + 1] = { 0 };
	BACNET_STORAGE_KEY key = { 0 };
	int stored_len, len;

	bacnet_storage_key_init(&key, object_type, object_instance, property_id,
				array_index);
	stored_len = bacnet_storage_get(&key, name, sizeof(name));
	if (stored_len > 0) {
		len = bacnet_character_string_application_decode(
			name, stored_len, value);
		if (len <= 0) {
			characterstring_init_ansi(value, default_value);
		}
	} else {
		characterstring_init_ansi(value, default_value);
	}

	return stored_len;
}

/**
 * @brief Store a BACnet CHARACTER_STRING value to non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param default_value [in] The default value if not found
 * @param value [out] The character string value
 * @return true on success, false on failure.
 */
bool bacnet_settings_characterstring_ansi_set(uint16_t object_type,
					      uint32_t object_instance,
					      uint32_t property_id,
					      uint32_t array_index,
					      const char *cstring)
{
	BACNET_APPLICATION_DATA_VALUE bvalue = { 0 };
	bool status;

	bvalue.context_specific = false;
	bvalue.tag = BACNET_APPLICATION_TAG_CHARACTER_STRING;
	status = characterstring_init_ansi(&bvalue.type.Character_String,
					   cstring);
	if (!status) {
		status = bacnet_settings_value_set(object_type, object_instance,
						   property_id, array_index,
						   &bvalue);
	}

	return status;
}

/**
 * @brief Get a C-string value from non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param default_value [in] The default value if not found
 * @param value [out] The string value
 * @param value_size [in] The size of the string value
 * @return stored data length on success 0..N, negative on failure.
 */
int bacnet_settings_string_get(uint16_t object_type, uint32_t object_instance,
			       uint32_t property_id, uint32_t array_index,
			       const char *default_value, char *value,
			       size_t value_size)
{
	BACNET_STORAGE_KEY key = { 0 };
	int rc;

	bacnet_storage_key_init(&key, object_type, object_instance, property_id,
				array_index);
	rc = bacnet_storage_get(&key, value, value_size);
	if (rc <= 0) {
		if (default_value) {
			strncpy(value, default_value, value_size);
			rc = strlen(default_value);
		}
	}

	return rc;
}

/**
 * @brief Get a C-string value from non-volatile storage
 * @param object_type [in] The BACnet object type
 * @param object_instance [in] The BACnet object instance
 * @param property_id [in] The BACnet property id
 * @param default_value [in] The default value if not found
 * @param value [in] The character string value
 * @return true on success, false on failure.
 */
bool bacnet_settings_string_set(uint16_t object_type, uint32_t object_instance,
				uint32_t property_id, uint32_t array_index,
				const char *value)
{
	BACNET_STORAGE_KEY key = { 0 };
	int rc;

	if (!value) {
		return false;
	}
	bacnet_storage_key_init(&key, object_type, object_instance, property_id,
				array_index);
	rc = bacnet_storage_set(&key, (const char *)value, strlen(value) + 1);

	return rc == 0;
}
