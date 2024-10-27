/**
 * @file
 * @brief API for basic BACnet data handling
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2013
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_CLIENT_DATA_H
#define BACNET_BASIC_CLIENT_DATA_H

#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/rp.h"

struct bacnet_status_flags_t {
    bool in_alarm : 1;
    bool fault : 1;
    bool overridden : 1;
    bool out_of_service : 1;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void bacnet_data_init(void);
BACNET_STACK_EXPORT
void bacnet_data_task(void);
BACNET_STACK_EXPORT
void bacnet_data_poll_seconds_set(unsigned int seconds);
BACNET_STACK_EXPORT
unsigned int bacnet_data_poll_seconds(void);
BACNET_STACK_EXPORT
void bacnet_data_value_save(
    uint32_t device_instance,
    BACNET_READ_PROPERTY_DATA *rp_data,
    BACNET_APPLICATION_DATA_VALUE *value);
BACNET_STACK_EXPORT
bool bacnet_data_object_add(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool bacnet_data_analog_present_value(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    float *float_value);
BACNET_STACK_EXPORT
bool bacnet_data_multistate_present_value(
    uint32_t device_id,
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t *unsigned_value);
BACNET_STACK_EXPORT
bool bacnet_data_binary_present_value(
    uint32_t device_id,
    uint16_t object_type,
    uint32_t object_instance,
    bool *bool_value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
