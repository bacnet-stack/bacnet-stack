/**
 * @file
 * @brief API to read properties from other BACnet devices
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_CLIENT_READ_WRITE_H
#define BACNET_BASIC_CLIENT_READ_WRITE_H
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/rp.h"

/**
 * Callback for successful WriteProperty response
 *
 * @param device_instance [in] device instance number where data originated
 */
typedef void (*bacnet_read_write_success_callback_t)(uint32_t device_instance);
/**
 * Save the requested ReadProperty data to a data store
 *
 * @param device_instance [in] device instance number where data originated
 * @param rp_data [in] Pointer to the BACNET_READ_PROPERTY_DATA structure,
 *  which is packed with the information from the ReadProperty request.
 * @param value [in] pointer to the BACNET_APPLICATION_DATA_VALUE structure
 *  which is packed with the decoded value from the ReadProperty request.
 */
typedef void (*bacnet_read_write_value_callback_t)(
    uint32_t device_instance,
    BACNET_READ_PROPERTY_DATA *rp_data,
    BACNET_APPLICATION_DATA_VALUE *value);

/**
 * Save the I-Am service data to a data store
 *
 * @param device_instance [in] device instance number where data originated
 * @param max_apdu [in] maximum APDU size
 * @param segmentation [in] segmentation flag
 * @param vendor_id [in] vendor identifier
 */
typedef void (*bacnet_read_write_device_callback_t)(
    uint32_t device_instance,
    unsigned max_apdu,
    int segmentation,
    uint16_t vendor_id);

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
bool bacnet_read_property_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint32_t array_index);
BACNET_STACK_EXPORT
bool bacnet_write_property_real_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    float value,
    uint8_t priority,
    uint32_t array_index);
BACNET_STACK_EXPORT
bool bacnet_write_property_null_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint8_t priority,
    uint32_t array_index);
BACNET_STACK_EXPORT
bool bacnet_write_property_enumerated_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    unsigned int value,
    uint8_t priority,
    uint32_t array_index);
BACNET_STACK_EXPORT
bool bacnet_write_property_unsigned_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    unsigned int value,
    uint8_t priority,
    uint32_t array_index);
BACNET_STACK_EXPORT
bool bacnet_write_property_signed_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    signed int value,
    uint8_t priority,
    uint32_t array_index);
BACNET_STACK_EXPORT
bool bacnet_write_property_boolean_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    bool value,
    uint8_t priority,
    uint32_t array_index);
/**
 * Queue a WriteProperty request using abstract-syntax encoded application data.
 *
 * @param device_id [in] target device instance number
 * @param object_type [in] target object type
 * @param object_instance [in] target object instance
 * @param object_property [in] target property identifier
 * @param value [in] pointer to the encoded application data buffer
 * @param value_length [in] length of the encoded application data buffer
 * @param priority [in] BACnet write priority
 * @param array_index [in] target array index
 *
 * The buffer referenced by @p value is treated as read-only. Ownership remains
 * with the caller, and this API does not deep-copy the buffer when queuing the
 * request. Therefore, @p value must remain valid and unchanged until the queued
 * write operation has been processed/completed.
 */
BACNET_STACK_EXPORT
bool bacnet_write_property_abstract_syntax_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    const uint8_t *value,
    uint16_t value_length,
    uint8_t priority,
    uint32_t array_index);

BACNET_STACK_EXPORT
void bacnet_read_write_success_callback_set(
    bacnet_read_write_success_callback_t callback);
BACNET_STACK_EXPORT
bool bacnet_write_property_abstract_syntax_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint8_t *value,
    uint16_t value_length,
    uint8_t priority,
    uint32_t array_index);

BACNET_STACK_EXPORT
void bacnet_read_write_success_callback_set(
    bacnet_read_write_success_callback_t callback);
BACNET_STACK_EXPORT
void bacnet_read_write_value_callback_set(
    bacnet_read_write_value_callback_t callback);
BACNET_STACK_EXPORT
void bacnet_read_write_device_callback_set(
    bacnet_read_write_device_callback_t callback);
BACNET_STACK_EXPORT
void bacnet_read_write_vendor_id_filter_set(uint16_t vendor_id);
BACNET_STACK_EXPORT
uint16_t bacnet_read_write_vendor_id_filter(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
