/**
 * @file
 * @brief API for Get/Set of BACnet application encoded settings handlers
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SETTINGS_H
#define BACNET_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacint.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int bacnet_settings_value_get(uint16_t object_type, uint32_t object_instance,
			      uint32_t property_id, uint32_t array_index,
			      BACNET_APPLICATION_DATA_VALUE *value);
bool bacnet_settings_value_set(uint16_t object_type, uint32_t object_instance,
			       uint32_t property_id, uint32_t array_index,
			       BACNET_APPLICATION_DATA_VALUE *value);

int bacnet_settings_real_get(uint16_t object_type, uint32_t object_instance,
			     uint32_t property_id, uint32_t array_index,
			     float default_value, float *value);
bool bacnet_settings_real_set(uint16_t object_type, uint32_t object_instance,
			      uint32_t property_id, uint32_t array_index,
			      float value);

int bacnet_settings_unsigned_get(uint16_t object_type, uint32_t object_instance,
				 uint32_t property_id, uint32_t array_index,
				 BACNET_UNSIGNED_INTEGER default_value,
				 BACNET_UNSIGNED_INTEGER *value);
bool bacnet_settings_unsigned_set(uint16_t object_type,
				  uint32_t object_instance,
				  uint32_t property_id, uint32_t array_index,
				  BACNET_UNSIGNED_INTEGER value);

int bacnet_settings_signed_get(uint16_t object_type, uint32_t object_instance,
			       uint32_t property_id, uint32_t array_index,
			       int32_t default_value, int32_t *value);
bool bacnet_settings_signed_set(uint16_t object_type, uint32_t object_instance,
				uint32_t property_id, uint32_t array_index,
				int32_t value);

int bacnet_settings_characterstring_get(uint16_t object_type,
					uint32_t object_instance,
					uint32_t property_id,
					uint32_t array_index,
					const char *default_value,
					BACNET_CHARACTER_STRING *value);

bool bacnet_settings_characterstring_ansi_set(uint16_t object_type,
					      uint32_t object_instance,
					      uint32_t property_id,
					      uint32_t array_index,
					      const char *cstring);

int bacnet_settings_string_get(uint16_t object_type, uint32_t object_instance,
			       uint32_t property_id, uint32_t array_index,
			       const char *default_value, char *value,
			       size_t value_size);

bool bacnet_settings_string_set(uint16_t object_type, uint32_t object_instance,
				uint32_t property_id, uint32_t array_index,
				const char *value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
