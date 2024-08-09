/**
 * @file
 * @brief Discover all BACnet devices on a destination network
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/bacapp.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/basic/services.h"
#include "bacnet/property.h"
/* us */
#include "bacnet/basic/client/bac-rw.h"
#include "bacnet/basic/client/bac-discover.h"

/* send a Who-Is to discover new devices */
static struct mstimer WhoIs_Timer;
/* property R/W process interval timer */
static struct mstimer Read_Write_Timer;
/* list of devices */
static OS_Keylist Device_List = NULL;
/* discovery destination network */
static uint16_t Target_DNET = 0;
/* re-discovery time */
static unsigned long Discovery_Milliseconds;
/* states of discovery */
typedef enum bacnet_discover_state_enum {
    BACNET_DISCOVER_STATE_INIT = 0,
    BACNET_DISCOVER_STATE_BINDING,
    BACNET_DISCOVER_STATE_OBJECT_LIST_SIZE_REQUEST,
    BACNET_DISCOVER_STATE_OBJECT_LIST_SIZE_RESPONSE,
    BACNET_DISCOVER_STATE_OBJECT_LIST_REQUEST,
    BACNET_DISCOVER_STATE_OBJECT_LIST_RESPONSE,
    BACNET_DISCOVER_STATE_OBJECT_GET_PROPERTY_REQUEST,
    BACNET_DISCOVER_STATE_OBJECT_GET_PROPERTY_RESPONSE,
    BACNET_DISCOVER_STATE_OBJECT_NEXT,
    BACNET_DISCOVER_STATE_DONE
} BACNET_DISCOVER_STATE;

typedef struct bacnet_property_data_t {
    uint8_t *application_data;
    int application_data_len;
} BACNET_PROPERTY_DATA;

typedef struct bacnet_object_data_t {
    OS_Keylist Property_List;
    /* used for discovering object data */
    uint32_t Property_List_Size;
    uint32_t Property_List_Index;
} BACNET_OBJECT_DATA;

typedef struct bacnet_device_data_t {
    OS_Keylist Object_List;
    /* used for discovering device data */
    uint32_t Object_List_Size;
    uint32_t Object_List_Index;
    /* timer and stats */
    struct mstimer Discovery_Timer;
    unsigned long Discovery_Elapsed_Milliseconds;
    BACNET_DISCOVER_STATE Discovery_State;
} BACNET_DEVICE_DATA;

/**
 * @brief Add a ReadProperty reply data value to the property-list
 * @param list - Keylist to add the property to
 * @param key - BACnet property key
 * @return Pointer to the property data structure
 */
static BACNET_PROPERTY_DATA *bacnet_property_data_add(OS_Keylist list, KEY key)
{
    BACNET_PROPERTY_DATA *data = NULL;
    int index;

    data = Keylist_Data(list, key);
    if (!data) {
        data = calloc(1, sizeof(BACNET_PROPERTY_DATA));
        if (data) {
            index = Keylist_Data_Add(list, key, data);
            if (index < 0) {
                free(data);
                data = NULL;
            }
        }
    }

    return data;
}

/**
 * @brief Remove all the property data from the property-list
 * @param list - Keylist to remove the property from
 */
static void bacnet_property_data_cleanup(OS_Keylist list)
{
    BACNET_PROPERTY_DATA *data = NULL;

    do {
        data = Keylist_Data_Pop(list);
        if (data) {
            free(data->application_data);
            free(data);
        }
    } while (data);
    Keylist_Delete(list);
}

/**
 * @brief Add an object to the object list
 * @param list - Keylist to add the object to
 * @param object_type - BACnet object type
 * @param object_instance - BACnet object instance
 * @return Pointer to the object data structure
 */
static BACNET_OBJECT_DATA *bacnet_object_data_add(
    OS_Keylist list, BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    BACNET_OBJECT_DATA *data = NULL;
    KEY key;
    int index;

    key = KEY_ENCODE(object_type, object_instance);
    data = Keylist_Data(list, key);
    if (!data) {
        data = calloc(1, sizeof(BACNET_OBJECT_DATA));
        if (data) {
            data->Property_List = Keylist_Create();
            data->Property_List_Size = 0;
            data->Property_List_Index = 0;
            index = Keylist_Data_Add(list, key, data);
            if (index < 0) {
                free(data);
                data = NULL;
            }
        }
    }

    return data;
}

/**
 * @brief Add an object to the object list
 * @param list - Keylist to add the object to
 * @param object_type - BACnet object type
 * @param object_instance - BACnet object instance
 * @return Pointer to the object data structure
 */
static int bacnet_object_list_index(
    OS_Keylist list, BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    KEY key;

    key = KEY_ENCODE(object_type, object_instance);
    return Keylist_Index(list, key);
}

/**
 * @brief Remove all the property data from the object-list
 * @param list - Keylist to remove the object and properties from
 */
static void bacnet_object_data_cleanup(OS_Keylist list)
{
    BACNET_OBJECT_DATA *data = NULL;

    do {
        data = Keylist_Data_Pop(list);
        if (data) {
            bacnet_property_data_cleanup(data->Property_List);
            free(data);
        }
    } while (data);
    Keylist_Delete(list);
}

/**
 * @brief Add a new device to the device list
 * @param list - Keylist to add the device to
 * @param device_instance - BACnet device instance
 * @return Pointer to the device data structure only if new
 */
static BACNET_DEVICE_DATA *bacnet_device_data_add(uint32_t device_instance)
{
    BACNET_DEVICE_DATA *data = NULL;
    KEY key = device_instance;
    int index;

    if (Device_List) {
        data = Keylist_Data(Device_List, key);
        if (!data) {
            /* device is not in the list */
            data = calloc(1, sizeof(BACNET_DEVICE_DATA));
            if (data) {
                data->Object_List = Keylist_Create();
                data->Discovery_State = BACNET_DISCOVER_STATE_INIT;
                mstimer_set(&data->Discovery_Timer, 0);
                /* other properties are already zeros */
                /* add to list */
                index = Keylist_Data_Add(Device_List, key, data);
                if (index < 0) {
                    free(data);
                    data = NULL;
                }
            }
        }
    }

    return data;
}

/**
 * @brief Get an existing device from the device list
 * @param list - Keylist to get the device from
 * @param device_id - BACnet device instance
 * @return Pointer to the device data structure
 */
static BACNET_DEVICE_DATA *bacnet_device_data(
    OS_Keylist list, uint32_t device_id)
{
    KEY key = device_id;
    BACNET_DEVICE_DATA *device_data;

    device_data = Keylist_Data(list, key);

    return device_data;
}

/**
 * @brief Remove all the device data from the device-list
 */
void bacnet_discover_cleanup(void)
{
    BACNET_DEVICE_DATA *data = NULL;

    do {
        data = Keylist_Data_Pop(Device_List);
        if (data) {
            bacnet_object_data_cleanup(data->Object_List);
            free(data);
        }
    } while (data);
    Keylist_Delete(Device_List);
}

/**
 * @brief get the number of devices discovered
 * @return  the number of devices discovered
 */
int bacnet_discover_device_count(void)
{
    int count;

    count = Keylist_Count(Device_List);

    return count;
}

/**
 * @brief get the device ID at a particular index
 * @param index - 0..N of max devices
 * @return the device ID at a particular index, or UINT32_MAX if not found
 */
uint32_t bacnet_discover_device_instance(unsigned index)
{
    uint32_t instance = UINT32_MAX;
    KEY key;

    if (Keylist_Index_Key(Device_List, index, &key)) {
        instance = key;
    }

    return instance;
}

/**
 * @brief get the number of objects discovered in a device
 * @param device_id - ID of the destination device
 * @return the number of objects discovered in a device
 */
int bacnet_discover_device_object_count(uint32_t device_id)
{
    int count = 0;
    BACNET_DEVICE_DATA *device;
    KEY key = device_id;

    device = Keylist_Data(Device_List, key);
    if (device) {
        count = Keylist_Count(device->Object_List);
    }

    return count;
}

/**
 * @brief get the number of objects discovered in a device
 * @param device_id - ID of the destination device
 * @param index - 0..N of max objects in a device
 * @param object_id - object type and instance if object exists
 * @return true if an object ID was found at this index
 */
bool bacnet_discover_device_object_identifier(
    uint32_t device_id, unsigned index, BACNET_OBJECT_ID *object_id)
{
    bool status = false;
    BACNET_DEVICE_DATA *device;
    KEY key = device_id;

    device = Keylist_Data(Device_List, key);
    if (device) {
        if (Keylist_Index_Key(device->Object_List, index, &key)) {
            if (object_id) {
                object_id->type = KEY_DECODE_TYPE(key);
                object_id->instance = KEY_DECODE_ID(key);
            }
            status = true;
        }
    }

    return status;
}

/**
 * @brief Determine the amount of heap data used by a device
 * @param device_id - BACnet device instance
 * @return the amount of heap data used by a device
 */
size_t bacnet_discover_device_memory(uint32_t device_id)
{
    size_t heap_size = 0;
    size_t object_count = 0, property_count = 0;
    size_t i, j;
    KEY key = device_id;
    BACNET_DEVICE_DATA *device;
    BACNET_OBJECT_DATA *object;
    BACNET_PROPERTY_DATA *property;

    device = Keylist_Data(Device_List, key);
    if (device) {
        heap_size += sizeof(BACNET_DEVICE_DATA);
        object_count = Keylist_Count(device->Object_List);
        heap_size += (object_count * sizeof(BACNET_OBJECT_DATA));
        for (i = 0; i < object_count; i++) {
            object = Keylist_Data_Index(device->Object_List, i);
            if (object) {
                property_count = Keylist_Count(object->Property_List);
                heap_size += (property_count * sizeof(BACNET_PROPERTY_DATA));
                for (j = 0; j < property_count; j++) {
                    property = Keylist_Data_Index(object->Property_List, j);
                    if (property) {
                        heap_size += property->application_data_len;
                    }
                }
            }
        }
    }

    return heap_size;
}

/**
 * @brief get the elapsed time it took to discover a device
 * @param device_id - ID of the destination device
 * @return the elapsed time it took to discover a device
 */
unsigned long bacnet_discover_device_elapsed_milliseconds(uint32_t device_id)
{
    unsigned long milliseconds = 0;
    BACNET_DEVICE_DATA *device;
    KEY key = device_id;

    device = Keylist_Data(Device_List, key);
    if (device) {
        milliseconds = device->Discovery_Elapsed_Milliseconds;
    }

    return milliseconds;
}

/**
 * @brief Get a property value from the device cache
 * @param device_id - ID of the destination device
 * @param object_type - BACnet object type
 * @param object_instance - Instance number of the object to be read.
 * @param object_property - BACnet property identifier
 * @param value property value stored if available (see tag for type)
 * @return true if found and value loaded
 */
bool bacnet_discover_property_value(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    bool status = false;
    BACNET_DEVICE_DATA *device;
    BACNET_OBJECT_DATA *object;
    BACNET_PROPERTY_DATA *property;
    KEY key = device_id;
    int len = 0;

    if (!value) {
        return false;
    }
    device = Keylist_Data(Device_List, key);
    if (device) {
        key = KEY_ENCODE(object_type, object_instance);
        object = Keylist_Data(device->Object_List, key);
        if (object) {
            key = object_property;
            property = Keylist_Data(object->Property_List, key);
            if (property) {
                if (property->application_data_len > 0) {
                    len =
                        bacapp_decode_known_property(property->application_data,
                            property->application_data_len, value, object_type,
                            object_property);
                    if (len > 0) {
                        status = true;
                    }
                } else {
                    bacapp_value_list_init(value, 1);
                    status = true;
                }
            }
        }
    }

    return status;
}

/**
 * @brief Get a name property value from the device object property cache
 * @param device_id - ID of the destination device
 * @param object_type - BACnet object type
 * @param object_instance - Instance number of the object to be read.
 * @param object_property - BACnet property identifier
 * @param buffer [out] Buffer to hold the property name.
 * @param buffer_len [in] Length of the buffer.
 * @param default_string [in] String to use if the property is not found.
 * @return true if found and value copied, else false and default_string copied.
 */
bool bacnet_discover_property_name(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    char *buffer,
    size_t buffer_len,
    const char *default_string)
{
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    bool status = false;

    if (buffer && buffer_len) {
        status = bacnet_discover_property_value(
            device_id, object_type, object_instance, object_property, &value);
        if (status && value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
            if (characterstring_valid(&value.type.Character_String)) {
                strncpy(buffer,
                    characterstring_value(&value.type.Character_String),
                    buffer_len - 1);
            } else {
                status = false;
            }
        }
    }
    if (!status) {
        strncpy(buffer, default_string, buffer_len);
    }

    return status;
}

/**
 * @brief Get the object property count from object property cache
 * @param device_id - ID of the destination device
 * @param object_type - BACnet object type
 * @param object_instance - Instance number of the object to be read.
 * @return number of object properties
 */
unsigned int bacnet_discover_object_property_count(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    unsigned int count = 0;
    BACNET_DEVICE_DATA *device;
    BACNET_OBJECT_DATA *object;
    KEY key = device_id;

    device = Keylist_Data(Device_List, key);
    if (device) {
        key = KEY_ENCODE(object_type, object_instance);
        object = Keylist_Data(device->Object_List, key);
        if (object) {
            count = Keylist_Count(object->Property_List);
        }
    }

    return count;
}

/**
 * @brief get the number of objects discovered in a device
 * @param device_id - ID of the destination device
 * @param object_type - BACnet object type
 * @param object_instance - Instance number of the object to be queried
 * @param index - 0..N of max properties in an object instance
 * @param property_id - property identifier if object exists
 * @return true if an object property ID was found at this index
 */
bool bacnet_discover_object_property_identifier(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    unsigned index,
    uint32_t *property_id)
{
    bool status = false;
    BACNET_DEVICE_DATA *device;
    BACNET_OBJECT_DATA *object;
    KEY key = device_id;

    device = Keylist_Data(Device_List, key);
    if (device) {
        key = KEY_ENCODE(object_type, object_instance);
        object = Keylist_Data(device->Object_List, key);
        if (object) {
            if (Keylist_Index_Key(object->Property_List, index, &key)) {
                if (property_id) {
                    *property_id = key;
                }
                status = true;
            }
        }
    }

    return status;
}

/**
 * @brief add a ReadProperty reply value from a device object property
 * @param device_id [in] Device instance number where data originated
 * @param rp_data [in] Pointer to the BACNET_READ_PROPERTY_DATA structure,
 * which is packed with the information from the ReadProperty request.
 * @param value [in] pointer to the BACNET_APPLICATION_DATA_VALUE structure
 * which is packed with the decoded value from the ReadProperty request.
 * @param device_data [in] Pointer to the device data structure
 */
static void bacnet_device_object_property_add(uint32_t device_id,
    const BACNET_READ_PROPERTY_DATA *rp_data,
    const BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_DEVICE_DATA *device_data)
{
    BACNET_OBJECT_DATA *object_data;
    BACNET_PROPERTY_DATA *property_data;

    if (!rp_data || !value || !device_data) {
        return;
    }
    if ((rp_data->object_type == OBJECT_DEVICE) &&
        (rp_data->object_instance == device_id) &&
        (rp_data->object_property == PROP_OBJECT_LIST)) {
        if (value->tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
            device_data->Object_List_Size = value->type.Unsigned_Int;
            device_data->Object_List_Index = 0;
            if (device_data->Discovery_State ==
                BACNET_DISCOVER_STATE_OBJECT_LIST_SIZE_REQUEST) {
                device_data->Discovery_State =
                    BACNET_DISCOVER_STATE_OBJECT_LIST_SIZE_RESPONSE;
            }
        } else if (value->tag == BACNET_APPLICATION_TAG_OBJECT_ID) {
            if (rp_data->array_index <= device_data->Object_List_Size) {
                object_data = bacnet_object_data_add(device_data->Object_List,
                    value->type.Object_Id.type, value->type.Object_Id.instance);
                debug_printf("add %u object-list[%u] %s-%lu %s.\n", device_id,
                    device_data->Object_List_Index,
                    bactext_object_type_name(value->type.Object_Id.type),
                    (unsigned long)value->type.Object_Id.instance,
                    object_data ? "success" : "fail");
                if (device_data->Discovery_State ==
                    BACNET_DISCOVER_STATE_OBJECT_LIST_REQUEST) {
                    device_data->Discovery_State =
                        BACNET_DISCOVER_STATE_OBJECT_LIST_RESPONSE;
                }
            }
        }
    } else {
        /* move to next state */
        if (device_data->Discovery_State ==
            BACNET_DISCOVER_STATE_OBJECT_GET_PROPERTY_REQUEST) {
            device_data->Discovery_State =
                BACNET_DISCOVER_STATE_OBJECT_GET_PROPERTY_RESPONSE;
        }
        object_data = bacnet_object_data_add(device_data->Object_List,
            rp_data->object_type, rp_data->object_instance);
        if (!object_data) {
            debug_perror("%s-%u object fail to add!\n",
                bactext_object_type_name(rp_data->object_type),
                rp_data->object_instance);
            return;
        }
        property_data = bacnet_property_data_add(
            object_data->Property_List, rp_data->object_property);
        if (!property_data) {
            debug_perror("%s-%u %s property fail to add!\n",
                bactext_object_type_name(rp_data->object_type),
                rp_data->object_instance,
                bactext_property_name(rp_data->object_property));
            return;
        }
        if (rp_data->application_data_len > 0) {
            if (property_data->application_data_len !=
                rp_data->application_data_len) {
                free(property_data->application_data);
                property_data->application_data =
                    calloc(1, rp_data->application_data_len);
            }
            if (property_data->application_data) {
                property_data->application_data_len =
                    rp_data->application_data_len;
                memcpy(property_data->application_data,
                    rp_data->application_data, rp_data->application_data_len);
            } else {
                debug_perror("%s-%u %s property fail to allocate!\n",
                    bactext_object_type_name(rp_data->object_type),
                    rp_data->object_instance,
                    bactext_property_name(rp_data->object_property));
            }
        } else {
            free(property_data->application_data);
            property_data->application_data = NULL;
            property_data->application_data_len = 0;
        }
        if (rp_data->array_index == BACNET_ARRAY_ALL) {
            debug_printf("%u object-list[%d] %s-%lu %s added.\n", device_id,
                bacnet_object_list_index(device_data->Object_List,
                    rp_data->object_type, rp_data->object_instance),
                bactext_object_type_name(rp_data->object_type),
                (unsigned long)rp_data->object_instance,
                bactext_property_name(rp_data->object_property));
        } else {
            debug_printf("%u object-list[%d] %s-%lu %s[%lu] added.\n",
                device_id,
                bacnet_object_list_index(device_data->Object_List,
                    rp_data->object_type, rp_data->object_instance),
                bactext_object_type_name(rp_data->object_type),
                (unsigned long)rp_data->object_instance,
                bactext_property_name(rp_data->object_property),
                (unsigned long)rp_data->array_index);
        }
    }
}

/**
 * @brief Handle the error from a ReadProperty or ReadPropertyMultiple
 * @param device_id - device instance number where data originated
 * @param error_code - BACnet Error code
 */
static void Device_Error_Handler(uint32_t device_id,
    BACNET_ERROR_CODE error_code,
    BACNET_DEVICE_DATA *device_data)
{
    if (device_data) {
        debug_printf(
            "%u - %s\n", device_id, bactext_error_code_name((int)error_code));
        switch (device_data->Discovery_State) {
            case BACNET_DISCOVER_STATE_OBJECT_LIST_REQUEST:
                /* resend request */
                if (device_data->Object_List_Index != 0) {
                    device_data->Object_List_Index--;
                }
                device_data->Discovery_State =
                    BACNET_DISCOVER_STATE_OBJECT_LIST_RESPONSE;
                break;
            case BACNET_DISCOVER_STATE_OBJECT_GET_PROPERTY_REQUEST:
                /* resend request */
                if (device_data->Object_List_Index != 0) {
                    device_data->Object_List_Index--;
                }
                device_data->Discovery_State =
                    BACNET_DISCOVER_STATE_OBJECT_GET_PROPERTY_RESPONSE;
                break;
            default:
                break;
        }
    }
}

/**
 * @brief Reply with the value from the ReadProperty request
 * @param device_id [in] Device instance number
 * @param rp_data [in] Pointer to the BACNET_READ_PROPERTY_DATA structure,
 *  which is packed with the information from the ReadProperty request.
 * @param value [in] pointer to the BACNET_APPLICATION_DATA_VALUE structure
 *  which is packed with the decoded value from the ReadProperty request.
 */
static void bacnet_read_property_reply(uint32_t device_id,
    BACNET_READ_PROPERTY_DATA *rp_data,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    BACNET_DEVICE_DATA *device_data;

    if (!rp_data) {
        return;
    }
    device_data = bacnet_device_data(Device_List, device_id);
    if (!device_data) {
        return;
    }
    if (rp_data->error_code != ERROR_CODE_SUCCESS) {
        Device_Error_Handler(device_id, rp_data->error_code, device_data);
    } else if (value) {
        bacnet_device_object_property_add(
            device_id, rp_data, value, device_data);
    }
}

/**
 * @brief Non-blocking task for running BACnet discover state machine
 * @param device_id - Device ID from discovered device
 * @param device_data - Pointer to the device data structure
 */
static void bacnet_discover_device_fsm(
    uint32_t device_id, BACNET_DEVICE_DATA *device_data)
{
    KEY key = 0;
    BACNET_OBJECT_TYPE object_type = 0;
    uint32_t object_instance = 0;
    bool status = false;

    if (!device_data) {
        return;
    }
    switch (device_data->Discovery_State) {
        case BACNET_DISCOVER_STATE_INIT:
            status = bacnet_read_property_queue(
                device_id, OBJECT_DEVICE, device_id, PROP_OBJECT_LIST, 0);
            if (status) {
                device_data->Discovery_State =
                    BACNET_DISCOVER_STATE_OBJECT_LIST_SIZE_REQUEST;
            } else {
                debug_perror("%u object-list-size fail to queue!\n", device_id);
            }
            break;
        case BACNET_DISCOVER_STATE_OBJECT_LIST_SIZE_REQUEST:
            /* waiting for response */
            return;
        case BACNET_DISCOVER_STATE_OBJECT_LIST_SIZE_RESPONSE:
            device_data->Object_List_Index = 0;
            device_data->Discovery_State =
                BACNET_DISCOVER_STATE_OBJECT_LIST_RESPONSE;
            break;
        case BACNET_DISCOVER_STATE_OBJECT_LIST_REQUEST:
            /* waiting for response */
            break;
        case BACNET_DISCOVER_STATE_OBJECT_LIST_RESPONSE:
            device_data->Object_List_Index++;
            if (device_data->Object_List_Index <=
                device_data->Object_List_Size) {
                debug_printf("%u object-list[%u] size=%u.\n", device_id,
                    device_data->Object_List_Index,
                    device_data->Object_List_Size);
                status = bacnet_read_property_queue(device_id, OBJECT_DEVICE,
                    device_id, PROP_OBJECT_LIST,
                    device_data->Object_List_Index);
                if (status) {
                    device_data->Discovery_State =
                        BACNET_DISCOVER_STATE_OBJECT_LIST_REQUEST;
                    return;
                } else {
                    debug_perror("%u object-list[%u] %s-%u fail to queue!\n",
                        device_id, device_data->Object_List_Index,
                        bactext_object_type_name(object_type),
                        (unsigned)object_instance);
                    device_data->Object_List_Index--;
                }
            } else {
                device_data->Object_List_Index = 0;
                device_data->Discovery_State =
                    BACNET_DISCOVER_STATE_OBJECT_GET_PROPERTY_RESPONSE;
            }
            break;
        case BACNET_DISCOVER_STATE_OBJECT_GET_PROPERTY_REQUEST:
            /* waiting for response */
            break;
        case BACNET_DISCOVER_STATE_OBJECT_GET_PROPERTY_RESPONSE:
            if (device_data->Object_List_Index <
                device_data->Object_List_Size) {
                if (Keylist_Index_Key(device_data->Object_List,
                        device_data->Object_List_Index, &key)) {
                    object_type = KEY_DECODE_TYPE(key);
                    object_instance = KEY_DECODE_ID(key);
                    debug_printf("%u object-list[%u] %s-%u read ALL.\n",
                        device_id, device_data->Object_List_Index,
                        bactext_object_type_name(object_type),
                        (unsigned)object_instance);
                    status = bacnet_read_property_queue(device_id, object_type,
                        object_instance, PROP_ALL, BACNET_ARRAY_ALL);
                }
                if (status) {
                    device_data->Discovery_State =
                        BACNET_DISCOVER_STATE_OBJECT_GET_PROPERTY_REQUEST;
                    device_data->Object_List_Index++;
                } else {
                    debug_perror("%u object-list[%u] %s-%u fail to queue!\n",
                        device_id, device_data->Object_List_Index,
                        bactext_object_type_name(object_type),
                        (unsigned)object_instance);
                }
            } else {
                /* track the duration */
                device_data->Discovery_Elapsed_Milliseconds =
                    mstimer_elapsed(&device_data->Discovery_Timer);
                /* rediscover in the future */
                mstimer_set(
                    &device_data->Discovery_Timer, Discovery_Milliseconds);
                device_data->Discovery_State = BACNET_DISCOVER_STATE_DONE;
            }
            break;
        case BACNET_DISCOVER_STATE_DONE:
            /* finished getting all the object properties */
            if (mstimer_expired(&device_data->Discovery_Timer)) {
                mstimer_set(&device_data->Discovery_Timer, 0);
                device_data->Discovery_State = BACNET_DISCOVER_STATE_INIT;
            }
            break;
        default:
            debug_perror("%u unknown state %u!\n", device_id,
                device_data->Discovery_State);
            break;
    }
}

/**
 * @brief Adds a device to the device list
 * @param device_id - Device ID from discovered device
 * @param src - BACnet address from discovered device
 * @return non-zero device index for device if added or existing
 */
static void bacnet_discover_devices_task(void)
{
    unsigned int device_index = 0;
    unsigned int device_count = 0;
    uint32_t device_id = 0;
    BACNET_DEVICE_DATA *device_data;
    KEY key;

    device_count = Keylist_Count(Device_List);
    for (device_index = 0; device_index < device_count; device_index++) {
        device_data = Keylist_Data_Index(Device_List, device_index);
        if (!device_data) {
            debug_perror("device[%u] is NULL!\n", device_index);
            continue;
        }
        if (Keylist_Index_Key(Device_List, device_index, &key)) {
            device_id = key;
            bacnet_discover_device_fsm(device_id, device_data);
        }
    }
}

/**
 * @brief Iterate a specific device object property list
 * @param callback - function to call for each device object property
 * @param context - pointer to user data
 * @return true if the iteration completed, false if it stopped early
 */
bool bacnet_discover_device_object_property_iterate(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    bacnet_discover_device_callback callback,
    void *context)
{
    size_t property_count = 0;
    size_t device_index = 0, object_index = 0, property_index = 0;
    BACNET_DEVICE_DATA *device;
    BACNET_OBJECT_DATA *object;
    BACNET_PROPERTY_DATA *property;
    BACNET_READ_PROPERTY_DATA rp_data = { 0 };
    bool status = true;
    KEY key = device_id;

    /* device */
    device = Keylist_Data(Device_List, key);
    if (!device) {
        return true;
    }
    device_index = Keylist_Index(Device_List, key);
    /* object */
    key = KEY_ENCODE(object_type, object_instance);
    object = Keylist_Data(device->Object_List, key);
    if (!object) {
        return true;
    }
    object_index = Keylist_Index(device->Object_List, key);
    rp_data.object_type = object_type;
    rp_data.object_instance = object_instance;
    /* property */
    property_count = Keylist_Count(object->Property_List);
    for (property_index = 0; property_index < property_count;
         property_index++) {
        if (Keylist_Index_Key(object->Property_List, property_index, &key)) {
            rp_data.object_property = key;
        } else {
            continue;
        }
        property = Keylist_Data_Index(object->Property_List, property_index);
        if (property) {
            rp_data.error_class = ERROR_CLASS_PROPERTY;
            rp_data.error_code = ERROR_CODE_SUCCESS;
            rp_data.application_data = property->application_data;
            rp_data.application_data_len = property->application_data_len;
            status = callback(device_id, device_index, object_index,
                property_index, &rp_data, context);
            /* callback returns true if the iteration
                should continue, false if it should stop */
            if (!status) {
                return false;
            }
        } else {
            rp_data.application_data = NULL;
            rp_data.application_data_len = 0;
            rp_data.error_class = ERROR_CLASS_PROPERTY;
            rp_data.error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            status = callback(device_id, device_index, object_index,
                property_index, &rp_data, context);
            /* callback returns true if the iteration
                should continue, false if it should stop */
            if (!status) {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Iterate a specific device object list
 * @param callback - function to call for each device object property
 * @param context - pointer to user data
 * @return true if the iteration completed, false if it stopped early
 */
bool bacnet_discover_device_object_iterate(
    uint32_t device_id, bacnet_discover_device_callback callback, void *context)
{
    size_t object_count = 0;
    size_t object_index = 0;
    BACNET_DEVICE_DATA *device;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;
    KEY key = device_id;
    bool status = false;

    device = Keylist_Data(Device_List, key);
    if (!device) {
        return true;
    }
    object_count = Keylist_Count(device->Object_List);
    for (object_index = 0; object_index < object_count; object_index++) {
        if (Keylist_Index_Key(device->Object_List, object_index, &key)) {
            object_type = KEY_DECODE_TYPE(key);
            object_instance = KEY_DECODE_ID(key);
        } else {
            continue;
        }
        status = bacnet_discover_device_object_property_iterate(
            device_id, object_type, object_instance, callback, context);
        if (!status) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Iterate the device list
 * @param callback - function to call for each device object property
 * @param context - pointer to user data
 * @return true if the iteration completed, false if it stopped early
 */
bool bacnet_discover_device_iterate(
    bacnet_discover_device_callback callback, void *context)
{
    size_t device_count = 0;
    size_t device_index = 0;
    uint32_t device_id = 0;
    bool status = true;
    KEY key = 0;

    device_count = Keylist_Count(Device_List);
    for (device_index = 0; device_index < device_count; device_index++) {
        if (Keylist_Index_Key(Device_List, device_index, &key)) {
            device_id = key;
            status = bacnet_discover_device_object_iterate(
                device_id, callback, context);
            if (!status) {
                return false;
            }
        } else {
            continue;
        }
    }

    return true;
}

/**
 * @brief Non-blocking task for running BACnet client tasks
 * @param dest - BACnet address of the destination discovery
 */
void bacnet_discover_task(void)
{
    BACNET_ADDRESS dest = { 0 };

    if (mstimer_expired(&WhoIs_Timer)) {
        mstimer_restart(&WhoIs_Timer);
        dest.net = Target_DNET;
        Send_WhoIs_To_Network(&dest, -1, -1);
    }
    if (mstimer_expired(&Read_Write_Timer)) {
        mstimer_restart(&Read_Write_Timer);
        bacnet_read_write_task();
    }
    if (bacnet_read_write_idle()) {
        bacnet_discover_devices_task();
    }
}

/**
 * @brief Set the BACnet time between discovery in seconds
 * @param seconds - number of seconds between discovery intervals
 */
void bacnet_discover_dnet_set(uint16_t dnet)
{
    Target_DNET = dnet;
}

/**
 * @brief Get the BACnet time between discovery in seconds
 * @return number of seconds between discovery intervals
 */
uint16_t bacnet_discover_dnet(void)
{
    return Target_DNET;
}

/**
 * @brief Sets a Vendor ID filter on I-Am bindings to limit the address
 *  cache usage when we are only reading/writing to a specific vendor ID
 * @param vendor_id - vendor ID to filter, 0=no filter
 */
void bacnet_discover_vendor_id_set(uint16_t vendor_id)
{
    bacnet_read_write_vendor_id_filter_set(vendor_id);
}

/**
 * @brief Gets a Vendor ID filter on I-Am bindings to limit the address
 *  cache usage when we are only reading/writing to a specific vendor ID
 * @return vendor_id - vendor ID to filter, 0=no filter
 */
uint16_t bacnet_discover_vendor_id(void)
{
    return bacnet_read_write_vendor_id_filter();
}

/**
 * @brief Set the BACnet time between discovery in seconds
 * @param seconds - number of seconds between discovery intervals
 */
void bacnet_discover_seconds_set(unsigned int seconds)
{
    Discovery_Milliseconds = seconds * 1000;
}

/**
 * @brief Get the BACnet time between discovery in seconds
 * @return number of seconds between discovery intervals
 */
unsigned int bacnet_discover_seconds(void)
{
    return Discovery_Milliseconds = 1000;
}

/**
 * @brief Set the millisecond timer for the read propcess (default=10ms)
 * @param milliseconds - read process task time
 */
void bacnet_discover_read_process_milliseconds_set(unsigned long milliseconds)
{
    mstimer_set(&Read_Write_Timer, milliseconds);
}

/**
 * @brief Get the millisecond timer for the read propcess (default=10ms)
 * @return read process task time in milliseconds
 */
unsigned long bacnet_discover_read_process_milliseconds(void)
{
    return mstimer_interval(&Read_Write_Timer);
}

/**
 * Save the I-Am service data to a data store
 *
 * @param device_instance [in] device instance number where data originated
 * @param max_apdu [in] maximum APDU size
 * @param segmentation [in] segmentation flag
 * @param vendor_id [in] vendor identifier
 */
void bacnet_discover_device_add(uint32_t device_instance,
    unsigned max_apdu,
    int segmentation,
    uint16_t vendor_id)
{
    BACNET_DEVICE_DATA *device_data;

    (void)max_apdu;
    (void)segmentation;
    device_data = bacnet_device_data_add(device_instance);
    debug_printf("device[%d] %lu - vendor=%u %s.\n",
        Keylist_Index(Device_List, device_instance), device_instance, vendor_id,
        device_data ? "success" : "fail");
}

/**
 * @brief Initializes the ReadProperty module
 */
void bacnet_discover_init(void)
{
    Device_List = Keylist_Create();
    bacnet_read_write_init();
    /* default value in case it is not set */
    if (!mstimer_interval(&WhoIs_Timer)) {
        mstimer_set(&WhoIs_Timer, 5UL * 60UL * 1000UL);
    }
    /* force WhoIs_Timer to be expired to send WhoIs immediately */
    WhoIs_Timer.start = mstimer_now() - WhoIs_Timer.interval;
    /* default value in case it is not set */
    if (!mstimer_interval(&Read_Write_Timer)) {
        mstimer_set(&Read_Write_Timer, 10);
    }
    bacnet_read_write_value_callback_set(bacnet_read_property_reply);
    bacnet_read_write_device_callback_set(bacnet_discover_device_add);
}
