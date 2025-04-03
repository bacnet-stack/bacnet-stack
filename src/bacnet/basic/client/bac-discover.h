/**
 * @file
 * @brief API for basic BACnet client device discovery
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_CLIENT_DISCOVER_H
#define BACNET_BASIC_CLIENT_DISCOVER_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/rp.h"

/**
 * @brief Callback function for iterating the results of the device discovery.
 * @param device_id [in] The device ID of the data
 * @param device_index [in] The index of the device in the list of discovered
 * devices
 * @param object_index [in] The index of the object in the list of discovered
 * objects in the device
 * @param property_index [in] The index of the property in the list of
 * discovered properties in the object in the device
 * @param rp_data [in] The contents of the device object property
 * @param context_data [in] The context data passed to the discover function
 * @return true if the iteration should continue, false if it should stop
 */
typedef bool (*bacnet_discover_device_callback)(
    uint32_t device_id,
    unsigned device_index,
    unsigned object_index,
    unsigned property_index,
    BACNET_READ_PROPERTY_DATA *rp_data,
    void *context_data);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void bacnet_discover_cleanup(void);
int bacnet_discover_device_count(void);
uint32_t bacnet_discover_device_instance(unsigned index);
int bacnet_discover_device_object_count(uint32_t device_id);
bool bacnet_discover_device_object_identifier(
    uint32_t device_id, unsigned index, BACNET_OBJECT_ID *object_id);
unsigned long bacnet_discover_device_elapsed_milliseconds(uint32_t device_id);
size_t bacnet_discover_device_memory(uint32_t device_id);
unsigned int bacnet_discover_object_property_count(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance);
bool bacnet_discover_object_property_identifier(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    unsigned index,
    uint32_t *property_id);
bool bacnet_discover_property_value(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_APPLICATION_DATA_VALUE *value);
bool bacnet_discover_property_name(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    char *buffer,
    size_t buffer_len,
    const char *default_string);
bool bacnet_discover_device_object_property_iterate(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    bacnet_discover_device_callback callback,
    void *context);
bool bacnet_discover_device_object_iterate(
    uint32_t device_id,
    bacnet_discover_device_callback callback,
    void *context);
bool bacnet_discover_device_iterate(
    bacnet_discover_device_callback callback, void *context);
void bacnet_discover_task(void);
void bacnet_discover_dnet_set(uint16_t dnet);
uint16_t bacnet_discover_dnet(void);
void bacnet_discover_dest_set(const BACNET_ADDRESS *dest);
const BACNET_ADDRESS *bacnet_discover_dest(void);
void bacnet_discover_vendor_id_set(uint16_t vendor_id);
uint16_t bacnet_discover_vendor_id(void);
void bacnet_discover_seconds_set(unsigned int seconds);
unsigned int bacnet_discover_seconds(void);
void bacnet_discover_read_process_milliseconds_set(unsigned long milliseconds);
unsigned long bacnet_discover_read_process_milliseconds(void);
void bacnet_discover_device_add(
    uint32_t device_instance,
    unsigned max_apdu,
    int segmentation,
    uint16_t vendor_id);
void bacnet_discover_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
