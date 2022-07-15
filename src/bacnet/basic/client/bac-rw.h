/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2013
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef BAC_RW_H
#define BAC_RW_H

#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacapp.h"
#include "bacnet/rp.h"
#include "bacnet/bacnet_stack_exports.h"

/**
 * Save the requested ReadProperty data to a data store
 *
 * @param device_instance [in] device instance number where data originated
 * @param rp_data [in] Pointer to the BACNET_READ_PROPERTY_DATA structure,
 *  which is packed with the information from the ReadProperty request.
 * @param value [in] pointer to the BACNET_APPLICATION_DATA_VALUE structure
 *  which is packed with the decoded value from the ReadProperty request.
 */
typedef void (*bacnet_read_write_value_callback_t)(uint32_t device_instance,
    BACNET_READ_PROPERTY_DATA *rp_data,
    BACNET_APPLICATION_DATA_VALUE *value);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void bacnet_read_write_init(void);
BACNET_STACK_EXPORT
void bacnet_read_write_task(void);
BACNET_STACK_EXPORT
bool bacnet_read_write_idle(void);
BACNET_STACK_EXPORT
bool bacnet_read_write_busy(void);
BACNET_STACK_EXPORT
bool bacnet_read_property_queue(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint32_t array_index);
BACNET_STACK_EXPORT
bool bacnet_write_property_real_queue(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    float value,
    uint8_t priority,
    uint32_t array_index);
BACNET_STACK_EXPORT
bool bacnet_write_property_null_queue(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint8_t priority,
    uint32_t array_index);
BACNET_STACK_EXPORT
bool bacnet_write_property_enumerated_queue(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    unsigned int value,
    uint8_t priority,
    uint32_t array_index);
BACNET_STACK_EXPORT
bool bacnet_write_property_unsigned_queue(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    unsigned int value,
    uint8_t priority,
    uint32_t array_index);
BACNET_STACK_EXPORT
bool bacnet_write_property_signed_queue(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    signed int value,
    uint8_t priority,
    uint32_t array_index);
BACNET_STACK_EXPORT
bool bacnet_write_property_boolean_queue(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    bool value,
    uint8_t priority,
    uint32_t array_index);
BACNET_STACK_EXPORT
void bacnet_read_write_value_callback_set(
    bacnet_read_write_value_callback_t callback);
BACNET_STACK_EXPORT
void bacnet_read_write_vendor_id_filter_set(uint16_t vendor_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
