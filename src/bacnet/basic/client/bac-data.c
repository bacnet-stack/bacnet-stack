/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2013
 * @brief Store properties from other BACnet devices
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/sys/mstimer.h"
/* us */
#include "bacnet/basic/client/bac-rw.h"
#include "bacnet/basic/client/bac-data.h"

/* number of objects data stored */
#ifndef BACNET_DATA_OBJECT_MAX
#define BACNET_DATA_OBJECT_MAX 16
#endif
/* Polling interval timer */
static struct mstimer Object_Poll_Timer;
/* property R/W process interval timer */
static struct mstimer Read_Write_Timer;

/* variables for remote BACnet Object Data */
typedef struct bacnet_object_data {
    uint32_t Device_ID;
    uint16_t Object_Type;
    uint32_t Object_ID;
    struct bacnet_present_value {
        /* application tag data type for writing */
        uint8_t tag;
        union {
            bool Boolean;
            float Real;
            uint32_t Unsigned_Int;
            int32_t Signed_Int;
            uint32_t Enumerated;
        } type;
    } Present_Value;
    bool refresh;
} BACNET_DATA_OBJECT;
static BACNET_DATA_OBJECT Object_Table[BACNET_DATA_OBJECT_MAX];

/**
 * @brief Find the index of a BACnet object type of a given instance.
 * @param  device_instance - object-instance number of the device object
 * @param  object_instance - object-instance number of the object
 * @return The index of the object sought, or BACNET_STATUS_ERROR if
 *  not found.
 */
static int bacnet_data_object_index_find(
    uint32_t device_instance, uint16_t object_type, uint32_t object_instance)
{
    BACNET_DATA_OBJECT *object = NULL;
    unsigned i = 0;

    for (i = 0; i < BACNET_DATA_OBJECT_MAX; i++) {
        object = &Object_Table[i];
        if ((object->Device_ID == device_instance) &&
            (object->Object_Type == object_type) &&
            (object->Object_ID == object_instance)) {
            return i;
        }
    }

    return BACNET_STATUS_ERROR;
}

/**
 * @brief Find a free index of an BACnet value object type
 * @return The index of a free element, or BACNET_STATUS_ERROR if
 *  no free elements exist
 */
static int bacnet_data_object_index_find_free(void)
{
    BACNET_DATA_OBJECT *object = NULL;
    unsigned i = 0;

    for (i = 0; i < BACNET_DATA_OBJECT_MAX; i++) {
        object = &Object_Table[i];
        if ((object->Device_ID >= BACNET_MAX_INSTANCE) &&
            (object->Object_Type == MAX_BACNET_OBJECT_TYPE) &&
            (object->Object_ID >= BACNET_MAX_INSTANCE)) {
            return i;
        }
    }

    return BACNET_STATUS_ERROR;
}

/**
 * @brief Initializes the BACnet object data
 */
static void bacnet_data_object_init(void)
{
    unsigned i = 0;
    BACNET_DATA_OBJECT *object = NULL;

    for (i = 0; i < BACNET_DATA_OBJECT_MAX; i++) {
        object = &Object_Table[i];
        object->Device_ID = BACNET_MAX_INSTANCE;
        object->Object_Type = MAX_BACNET_OBJECT_TYPE;
        object->Object_ID = BACNET_MAX_INSTANCE;
    }
}

static void bacnet_data_object_store(int index,
    BACNET_READ_PROPERTY_DATA *rp_data,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    BACNET_DATA_OBJECT *object = NULL;

    assert(rp_data != NULL);
    assert(value != NULL);
    if ((index < BACNET_DATA_OBJECT_MAX) && (!value->context_specific)) {
        object = &Object_Table[index];
        switch (rp_data->object_property) {
            case PROP_PRESENT_VALUE:
                if (value->tag == BACNET_APPLICATION_TAG_REAL) {
                    object->Present_Value.tag = value->tag;
                    object->Present_Value.type.Real = value->type.Real;
                }
                if (value->tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                    object->Present_Value.tag = value->tag;
                    object->Present_Value.type.Unsigned_Int =
                        value->type.Unsigned_Int;
                }
                if (value->tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                    object->Present_Value.tag = value->tag;
                    object->Present_Value.type.Enumerated =
                        value->type.Enumerated;
                }
                break;
            default:
                break;
        }
        object->refresh = false;
    }
}

/**
 * @brief Find the index of an analog value object type of a given instance.
 * @param rp_data [in] Pointer to the BACNET_READ_PROPERTY_DATA structure,
 *  which is packed with the information from the ReadProperty request.
 * @param value [in] pointer to the BACNET_APPLICATION_DATA_VALUE structure
 *  which is packed with the decoded value from the ReadProperty request.
 */
void bacnet_data_value_save(uint32_t device_instance,
    BACNET_READ_PROPERTY_DATA *rp_data,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    int index = 0;

    if (!rp_data) {
        return;
    }
    if (rp_data->error_code != ERROR_CODE_SUCCESS) {
        return;
    }
    if (value) {
        switch (rp_data->object_type) {
            case OBJECT_ANALOG_INPUT:
            case OBJECT_ANALOG_OUTPUT:
            case OBJECT_ANALOG_VALUE:
            case OBJECT_BINARY_INPUT:
            case OBJECT_BINARY_OUTPUT:
            case OBJECT_BINARY_VALUE:
            case OBJECT_MULTI_STATE_INPUT:
            case OBJECT_MULTI_STATE_OUTPUT:
            case OBJECT_MULTI_STATE_VALUE:
                index = bacnet_data_object_index_find(device_instance,
                    rp_data->object_type, rp_data->object_instance);
                if (index != BACNET_STATUS_ERROR) {
                    bacnet_data_object_store(index, rp_data, value);
                }
                break;
            case OBJECT_DEVICE:
            default:
                break;
        }
    }
}

/**
 * @brief Handles the BACnet Data Analog Value processing
 * @param object - BACnet object structure data pointer
 */
static void bacnet_data_object_process(BACNET_DATA_OBJECT *object)
{
    if (object && (object->Device_ID < BACNET_MAX_INSTANCE) &&
        (object->Object_ID < BACNET_MAX_INSTANCE)) {
        bacnet_read_property_queue(object->Device_ID,
            (BACNET_OBJECT_TYPE)object->Object_Type, object->Object_ID,
            PROP_PRESENT_VALUE, BACNET_ARRAY_ALL);
    }
}

/**
 * @brief Adds a BACnet Data remote value point
 * @param device_id - ID of the destination device
 * @param object_type - Type of the object whose property is to be read.
 * @param object_instance - Instance # of the object to be read.
 * @return true if added or existing, false if not added or existing
 */
bool bacnet_data_object_add(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    BACNET_DATA_OBJECT *object = NULL;
    bool status = false;
    int index = 0;

    switch (object_type) {
        case OBJECT_ANALOG_INPUT:
        case OBJECT_ANALOG_OUTPUT:
        case OBJECT_ANALOG_VALUE:
        case OBJECT_BINARY_INPUT:
        case OBJECT_BINARY_OUTPUT:
        case OBJECT_BINARY_VALUE:
        case OBJECT_MULTI_STATE_INPUT:
        case OBJECT_MULTI_STATE_OUTPUT:
        case OBJECT_MULTI_STATE_VALUE:
            index = bacnet_data_object_index_find(
                device_id, object_type, object_instance);
            if (index == BACNET_STATUS_ERROR) {
                index = bacnet_data_object_index_find_free();
                if (index != BACNET_STATUS_ERROR) {
                    object = &Object_Table[index];
                    object->Device_ID = device_id;
                    object->Object_Type = object_type;
                    object->Object_ID = object_instance;
                    object->refresh = true;
                    status = true;
                }
            } else {
                object = &Object_Table[index];
                object->refresh = true;
                status = true;
            }
            break;
        case OBJECT_DEVICE:
        default:
            break;
    }

    return status;
}

/**
 * @brief Reads a Property value that has been stored
 * @param device_id - ID of the destination device
 * @param object_type - BACnet object type
 * @param object_instance - Instance # of the object to be read.
 * @param float_value [out] property value stored if available
 * @return true if found and value loaded
 */
bool bacnet_data_analog_present_value(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    float *float_value)
{
    bool status = false;
    int index = 0;
    BACNET_DATA_OBJECT *object = NULL;

    index =
        bacnet_data_object_index_find(device_id, object_type, object_instance);
    if (index == BACNET_STATUS_ERROR) {
        /* add to our object table if not found */
        bacnet_data_object_add(device_id, object_type, object_instance);
    } else {
        status = true;
        if (float_value) {
            object = &Object_Table[index];
            *float_value = object->Present_Value.type.Real;
        }
    }

    return status;
}

/**
 * @brief Reads a Property value that has been stored
 * @param device_id - ID of the destination device
 * @param object_type - BACnet object type
 * @param object_instance - Instance # of the object to be read.
 * @param bool_value [out] property value stored if available
 * @return true if found and value loaded
 */
bool bacnet_data_binary_present_value(uint32_t device_id,
    uint16_t object_type,
    uint32_t object_instance,
    bool *bool_value)
{
    bool status = false;
    int index = 0;
    BACNET_DATA_OBJECT *object = NULL;

    index =
        bacnet_data_object_index_find(device_id, object_type, object_instance);
    if (index == BACNET_STATUS_ERROR) {
        /* add to our object table if not found */
        bacnet_data_object_add(
            device_id, (BACNET_OBJECT_TYPE)object_type, object_instance);
    } else {
        status = true;
        if (bool_value) {
            object = &Object_Table[index];
            if (object->Present_Value.type.Enumerated == BINARY_INACTIVE) {
                *bool_value = false;
            } else {
                *bool_value = true;
            }
        }
    }

    return status;
}

/**
 * @brief Reads a Property value that has been stored
 * @param device_id - ID of the destination device
 * @param object_type - BACnet object type
 * @param object_instance - Instance # of the object to be read.
 * @param bool_value [out] property value stored if available
 * @return true if found and value loaded
 */
bool bacnet_data_multistate_present_value(uint32_t device_id,
    uint16_t object_type,
    uint32_t object_instance,
    uint32_t *unsigned_value)
{
    bool status = false;
    int index = 0;
    BACNET_DATA_OBJECT *object = NULL;

    index =
        bacnet_data_object_index_find(device_id, object_type, object_instance);
    if (index == BACNET_STATUS_ERROR) {
        /* add to our object table if not found */
        bacnet_data_object_add(
            device_id, (BACNET_OBJECT_TYPE)object_type, object_instance);
    } else {
        status = true;
        if (unsigned_value) {
            object = &Object_Table[index];
            *unsigned_value = object->Present_Value.type.Unsigned_Int;
        }
    }

    return status;
}

/**
 * @brief Handles the BACnet Data repetitive task
 */
void bacnet_data_task(void)
{
    static unsigned object_index = 0;
    BACNET_DATA_OBJECT *object = NULL;
    unsigned i = 0;

    if (mstimer_expired(&Object_Poll_Timer)) {
        mstimer_reset(&Object_Poll_Timer);
        for (i = 0; i < BACNET_DATA_OBJECT_MAX; i++) {
            object = &Object_Table[i];
            object->refresh = true;
        }
    }
    if (mstimer_expired(&Read_Write_Timer)) {
        mstimer_reset(&Read_Write_Timer);
        bacnet_read_write_task();
    }
    if (bacnet_read_write_idle()) {
        object = &Object_Table[object_index];
        if (object->refresh) {
            object->refresh = false;
            bacnet_data_object_process(object);
        }
        object_index++;
        if (object_index >= BACNET_DATA_OBJECT_MAX) {
            object_index = 0;
        }
    }
}

/**
 * @brief Set the BACnet Data Poll seconds
 * @param seconds - number of seconds between polling intervals
 */
void bacnet_data_poll_seconds_set(unsigned int seconds)
{
    mstimer_set(&Object_Poll_Timer, seconds * 1000);
}

/**
 * @brief Get the BACnet Data Poll seconds
 * @return number of seconds between polling intervals
 */
unsigned int bacnet_data_poll_seconds(void)
{
    return mstimer_interval(&Object_Poll_Timer) * 1000;
}

/**
 * @brief Initializes the ReadProperty module
 */
void bacnet_data_init(void)
{
    bacnet_data_object_init();
    bacnet_read_write_init();
    /* start the cyclic poll timer */
    mstimer_set(&Object_Poll_Timer, 1 * 60 * 1000);
    mstimer_set(&Read_Write_Timer, 10);
    bacnet_read_write_value_callback_set(bacnet_data_value_save);
}
