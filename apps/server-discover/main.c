/**
 * @file
 * @author Steve Karg
 * @date 2022
 * @brief Application to acquire devices and their object list from a BACnet
 * network.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

/* core library */
#include "bacnet/config.h"
#include "bacnet/apdu.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/bacstr.h"
#include "bacnet/bactext.h"
#include "bacnet/dcc.h"
#include "bacnet/iam.h"
#include "bacnet/npdu.h"
#include "bacnet/version.h"
#include "bacnet/whois.h"
/* basic services */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/client/bac-rw.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"

#if BACNET_SVC_SERVER
#error "App requires server-only features disabled! Set BACNET_SVC_SERVER=0"
#endif

/* current version of the BACnet stack */
static const char *BACnet_Version = BACNET_VERSION_TEXT;
/** Buffer used for receiving */
static uint8_t Rx_Buf[MAX_MPDU];
/* task timer for various BACnet timeouts */
static struct mstimer BACnet_Task_Timer;
/* task timer for TSM timeouts */
static struct mstimer BACnet_TSM_Timer;
/* task timer for sending Who-Is */
static struct mstimer Discover_Timer;
/* property R/W process interval timer */
static struct mstimer Read_Write_Timer;
/* discovery destination network */
static uint16_t Target_DNET = 0;
/* list of devices */
static OS_Keylist Device_List = NULL;

typedef enum bacnet_discover_state_enum {
    BACNET_DISCOVER_STATE_INIT = 0,
    BACNET_DISCOVER_STATE_BINDING,
    BACNET_DISCOVER_STATE_DEVICE_NAME_REQUEST,
    BACNET_DISCOVER_STATE_DEVICE_NAME_RESPONSE,
    BACNET_DISCOVER_STATE_DEVICE_MODEL_REQUEST,
    BACNET_DISCOVER_STATE_DEVICE_MODEL_RESPONSE,
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
    uint16_t Vendor_Identifier;
    BACNET_CHARACTER_STRING Object_Name;
    BACNET_CHARACTER_STRING Model_Name;
    OS_Keylist Object_List;
    /* used for discovering device data */
    BACNET_DISCOVER_STATE Discovery_State;
    uint32_t Object_List_Size;
    uint32_t Object_List_Index;
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
            data->application_data_len = 0;
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
void bacnet_device_list_cleanup(void)
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
 * @brief Non-blocking task for running BACnet server tasks
 */
static void bacnet_server_task(void)
{
    static bool initialized = false;
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    const unsigned timeout_ms = 5;

    if (!initialized) {
        initialized = true;
        /* broadcast an I-Am on startup */
        Send_I_Am(&Handler_Transmit_Buffer[0]);
    }
    /* input */
    /* returns 0 bytes on timeout */
    pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout_ms);
    /* process */
    if (pdu_len) {
        npdu_handler(&src, &Rx_Buf[0], pdu_len);
    }
    /* 1 second tasks */
    if (mstimer_expired(&BACnet_Task_Timer)) {
        mstimer_reset(&BACnet_Task_Timer);
        dcc_timer_seconds(1);
        datalink_maintenance_timer(1);
        dlenv_maintenance_timer(1);
    }
    if (mstimer_expired(&BACnet_TSM_Timer)) {
        mstimer_reset(&BACnet_TSM_Timer);
        tsm_timer_milliseconds(mstimer_interval(&BACnet_TSM_Timer));
    }
}

/**
 * @brief Initialize the handlers for this server device
 */
static void bacnet_server_init(void)
{
    Device_Init(NULL);
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* we need to handle who-has to support dynamic object binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    mstimer_set(&BACnet_Task_Timer, 1000);
    mstimer_set(&BACnet_TSM_Timer, 50);
}

/**
 * @brief add a ReadProperty reply value from a device object property
 * @param rp_data [in] Pointer to the BACNET_READ_PROPERTY_DATA structure,
 * which is packed with the information from the ReadProperty request.
 * @param value [in] pointer to the BACNET_APPLICATION_DATA_VALUE structure
 * which is packed with the decoded value from the ReadProperty request.
 * @param device_data [in] Pointer to the device data structure
 */
static void bacnet_device_object_property_add(
    BACNET_READ_PROPERTY_DATA *rp_data,
    BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_DEVICE_DATA *device_data)
{
    BACNET_OBJECT_DATA *object_data;
    BACNET_PROPERTY_DATA *property_data;

    if (!rp_data) {
        return;
    }
    if (!value) {
        return;
    }
    if (!device_data) {
        return;
    }
    if (device_data->Discovery_State ==
        BACNET_DISCOVER_STATE_OBJECT_GET_PROPERTY_REQUEST) {
        device_data->Discovery_State =
            BACNET_DISCOVER_STATE_OBJECT_GET_PROPERTY_RESPONSE;
    }
    object_data = bacnet_object_data_add(device_data->Object_List,
        rp_data->object_type, rp_data->object_instance);
    if (!object_data) {
        return;
    }
    property_data = bacnet_property_data_add(
        object_data->Property_List, rp_data->object_property);
    if (!property_data) {
        return;
    }
    if (property_data->application_data_len == 0) {
        if (rp_data->application_data_len > 0) {
            property_data->application_data =
                calloc(1, rp_data->application_data_len);
        }
        property_data->application_data_len = rp_data->application_data_len;
    } else if (property_data->application_data_len !=
        rp_data->application_data_len) {
        if (property_data->application_data_len > 0) {
            property_data->application_data = realloc(
                property_data->application_data, rp_data->application_data_len);
        } else {
            free(property_data->application_data);
            property_data->application_data = NULL;
        }
        property_data->application_data_len = rp_data->application_data_len;
    }
    if (property_data->application_data_len == rp_data->application_data_len) {
        if (property_data->application_data) {
            memcpy(property_data->application_data, rp_data->application_data,
                rp_data->application_data_len);
        }
    }
}

/**
 * @brief save the Device values from the ReadProperty request
 * @param device_id [in] Device instance number
 * @param rp_data [in] Pointer to the BACNET_READ_PROPERTY_DATA structure,
 *  which is packed with the information from the ReadProperty request.
 * @param value [in] pointer to the BACNET_APPLICATION_DATA_VALUE structure
 *  which is packed with the decoded value from the ReadProperty request.
 */
static void bacnet_device_property_add(uint32_t device_id,
    BACNET_READ_PROPERTY_DATA *rp_data,
    BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_DEVICE_DATA *device_data)
{
    BACNET_OBJECT_DATA *object_data;

    if (!rp_data) {
        return;
    }
    if (!value) {
        return;
    }
    if (!device_data) {
        return;
    }
    switch (rp_data->object_property) {
        case PROP_OBJECT_NAME:
            if (value->tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
                characterstring_copy(
                    &device_data->Object_Name, &value->type.Character_String);
                device_data->Discovery_State =
                    BACNET_DISCOVER_STATE_DEVICE_NAME_RESPONSE;
            }
            break;
        case PROP_MODEL_NAME:
            if (value->tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
                characterstring_copy(
                    &device_data->Model_Name, &value->type.Character_String);
                device_data->Discovery_State =
                    BACNET_DISCOVER_STATE_DEVICE_MODEL_RESPONSE;
            }
            break;
        case PROP_OBJECT_LIST:
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
                    object_data = bacnet_object_data_add(
                        device_data->Object_List, value->type.Object_Id.type,
                        value->type.Object_Id.instance);
                    if (device_data->Discovery_State ==
                        BACNET_DISCOVER_STATE_OBJECT_LIST_REQUEST) {
                        debug_printf("add %u object-list[%u] %s-%lu %s.\n",
                            device_id, device_data->Object_List_Index,
                            bactext_object_type_name(
                                value->type.Object_Id.type),
                            (unsigned long)value->type.Object_Id.instance,
                            object_data ? "success" : "fail");
                        device_data->Discovery_State =
                            BACNET_DISCOVER_STATE_OBJECT_LIST_RESPONSE;
                    }
                }
            }
            break;
        default:
            break;
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
        if (device_data->Discovery_State ==
            BACNET_DISCOVER_STATE_DEVICE_NAME_REQUEST) {
            /* go back to previous step */
            device_data->Discovery_State = BACNET_DISCOVER_STATE_INIT;
        }
        if (device_data->Discovery_State ==
            BACNET_DISCOVER_STATE_DEVICE_MODEL_REQUEST) {
            /* go back to previous step */
            device_data->Discovery_State =
                BACNET_DISCOVER_STATE_DEVICE_NAME_RESPONSE;
        }
        if (device_data->Discovery_State ==
            BACNET_DISCOVER_STATE_OBJECT_LIST_SIZE_REQUEST) {
            /* go back to previous step */
            device_data->Discovery_State =
                BACNET_DISCOVER_STATE_DEVICE_MODEL_RESPONSE;
        }
        if (device_data->Discovery_State ==
            BACNET_DISCOVER_STATE_OBJECT_LIST_REQUEST) {
            /* resend request */
            if (device_data->Object_List_Index != 0) {
                device_data->Object_List_Index--;
            }
            device_data->Discovery_State =
                BACNET_DISCOVER_STATE_OBJECT_LIST_RESPONSE;
        }
        if (device_data->Discovery_State ==
            BACNET_DISCOVER_STATE_OBJECT_GET_PROPERTY_REQUEST) {
            /* resend request */
            if (device_data->Object_List_Index != 0) {
                device_data->Object_List_Index--;
            }
            device_data->Discovery_State =
                BACNET_DISCOVER_STATE_OBJECT_GET_PROPERTY_RESPONSE;
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
        switch (rp_data->object_type) {
            case OBJECT_DEVICE:
                bacnet_device_property_add(
                    device_id, rp_data, value, device_data);
                break;
            default:
                bacnet_device_object_property_add(rp_data, value, device_data);
                break;
        }
    }
}

/**
 * @brief Non-blocking task for running BACnet discover state machine
 * @param device_id - Device ID from discovered device
 * @param device_data - Pointer to the device data structure
 */
void bacnet_client_device_discover_fsm(
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
            status = bacnet_read_property_queue(device_id, OBJECT_DEVICE,
                device_id, PROP_OBJECT_NAME, BACNET_ARRAY_ALL);
            if (status) {
                device_data->Discovery_State =
                    BACNET_DISCOVER_STATE_DEVICE_NAME_REQUEST;
            } else {
                debug_perror("%u object-name fail to queue!\n", device_id);
            }
            break;
        case BACNET_DISCOVER_STATE_DEVICE_NAME_REQUEST:
            /* waiting for response */
            break;
        case BACNET_DISCOVER_STATE_DEVICE_NAME_RESPONSE:
            status = bacnet_read_property_queue(device_id, OBJECT_DEVICE,
                device_id, PROP_MODEL_NAME, BACNET_ARRAY_ALL);
            if (status) {
                device_data->Discovery_State =
                    BACNET_DISCOVER_STATE_DEVICE_MODEL_REQUEST;
            } else {
                debug_perror("%u model-name fail to queue!\n", device_id);
            }
            break;
        case BACNET_DISCOVER_STATE_DEVICE_MODEL_REQUEST:
            /* waiting for response */
            break;
        case BACNET_DISCOVER_STATE_DEVICE_MODEL_RESPONSE:
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
                key = Keylist_Key(
                    device_data->Object_List, device_data->Object_List_Index);
                object_type = KEY_DECODE_TYPE(key);
                object_instance = KEY_DECODE_ID(key);
                if (object_type == OBJECT_DEVICE) {
                    /* skip device object */
                    device_data->Object_List_Index++;
                    break;
                }
                debug_printf("%u object-list[%u] %s-%u read ALL.\n", device_id,
                    device_data->Object_List_Index,
                    bactext_object_type_name(object_type),
                    (unsigned)object_instance);
                status = bacnet_read_property_queue(device_id, object_type,
                    object_instance, PROP_ALL, BACNET_ARRAY_ALL);
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
                device_data->Discovery_State = BACNET_DISCOVER_STATE_DONE;
            }
            break;
        case BACNET_DISCOVER_STATE_DONE:
            /* finished getting all the object properties */
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
static void bacnet_client_device_discover_task(void)
{
    unsigned int device_index = 0;
    unsigned int device_count = 0;
    uint32_t device_id = 0;
    BACNET_DEVICE_DATA *device_data;

    device_count = Keylist_Count(Device_List);
    for (device_index = 0; device_index < device_count; device_index++) {
        device_data = Keylist_Data_Index(Device_List, device_index);
        if (!device_data) {
            debug_perror("device[%u] is NULL!\n", device_index);
            continue;
        }
        device_id = Keylist_Key(Device_List, device_index);
        bacnet_client_device_discover_fsm(device_id, device_data);
    }
}

/**
 * @brief Print the device list to the debug console
 */
void print_device_list(void)
{
    unsigned int device_index = 0;
    unsigned int device_count = 0;
    uint32_t device_id = 0;
    BACNET_DEVICE_DATA *device_data;

    device_count = Keylist_Count(Device_List);
    debug_printf("----list of %u devices ----\n", device_count);
    for (device_index = 0; device_index < device_count; device_index++) {
        device_data = Keylist_Data_Index(Device_List, device_index);
        if (!device_data) {
            debug_perror("device[%u] is NULL!\n", device_index);
            continue;
        }
        device_id = Keylist_Key(Device_List, device_index);
        debug_printf("device[%u] %7u vendor-id=%4d object_list[%d]\n", 
            device_index, device_id,
            device_data->Vendor_Identifier,
            Keylist_Count(device_data->Object_List));
    }
}

/**
 * @brief Non-blocking task for running BACnet client tasks
 * @param dest - BACnet address of the destination discovery
 */
static void bacnet_client_task(BACNET_ADDRESS *dest)
{
    if (mstimer_expired(&Discover_Timer)) {
        mstimer_reset(&Discover_Timer);
        Send_WhoIs_To_Network(dest, -1, -1);
        print_device_list();
    }
    if (mstimer_expired(&Read_Write_Timer)) {
        mstimer_reset(&Read_Write_Timer);
        bacnet_read_write_task();
    }
    if (bacnet_read_write_idle()) {
        bacnet_client_device_discover_task();
    }
}

/**
 * Save the I-Am service data to a data store
 *
 * @param device_instance [in] device instance number where data originated
 * @param max_apdu [in] maximum APDU size
 * @param segmentation [in] segmentation flag
 * @param vendor_id [in] vendor identifier
 */
void bacnet_client_device_add(uint32_t device_instance,
    unsigned max_apdu,
    int segmentation,
    uint16_t vendor_id)
{
    BACNET_DEVICE_DATA *device_data;

    (void)max_apdu;
    (void)segmentation;
    device_data = bacnet_device_data_add(device_instance);
    if (device_data) {
        device_data->Vendor_Identifier = vendor_id;
    }
    debug_printf("device[%d] %lu - vendor=%u %s\n",
        Keylist_Index(Device_List, device_instance), device_instance, vendor_id,
        device_data ? "success" : "fail");
}

/**
 * @brief Initializes the ReadProperty module
 */
static void bacnet_client_init(unsigned long discover_seconds)
{
    Device_List = Keylist_Create();
    bacnet_read_write_init();
    mstimer_set(&Discover_Timer, 1UL * discover_seconds * 1000UL);
    mstimer_set(&Read_Write_Timer, 10);
    bacnet_read_write_value_callback_set(bacnet_read_property_reply);
    bacnet_read_write_device_callback_set(bacnet_client_device_add);
}

static void print_usage(const char *filename)
{
    printf("Usage: %s [--discover-seconds][--dnet]\n", filename);
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf("Simulate a BACnet server-discovery device.\n");
    printf("--discover-seconds:\n"
           "Number of seconds to wait before initiating the next discovery.\n");
    printf("--dnet N\n"
           "Optional BACnet network number N for directed requests.\n"
           "Valid range is from 0 to 65535 where 0 is the local connection\n"
           "and 65535 is network broadcast.\n");
    (void)filename;
}

/**
 * @brief Main function of server-client demo.
 * @param argc [in] Arg count.
 * @param argv [in] Takes one argument: the Device Instance #.
 * @return 0 on success.
 */
int main(int argc, char *argv[])
{
    int argi = 0;
    unsigned int target_args = 0;
    const char *filename = NULL;
    uint32_t device_id = BACNET_MAX_INSTANCE;
    long long_value = -1;
    BACNET_ADDRESS dest = { 0 };
    /* data from the command line */
    unsigned long discover_seconds = 60;

    filename = filename_remove_path(argv[0]);
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--help") == 0) {
            print_usage(filename);
            print_help(filename);
            return 0;
        }
        if (strcmp(argv[argi], "--version") == 0) {
            printf("%s %s\n", filename, BACNET_VERSION_TEXT);
            printf("Copyright (C) 2024 by Steve Karg and others.\n"
                   "This is free software; see the source for copying "
                   "conditions.\n"
                   "There is NO warranty; not even for MERCHANTABILITY or\n"
                   "FITNESS FOR A PARTICULAR PURPOSE.\n");
            return 0;
        }
        if (strcmp(argv[argi], "--discover-seconds") == 0) {
            if (++argi < argc) {
                discover_seconds = strtol(argv[argi], NULL, 0);
            }
        } else if (strcmp(argv[argi], "--dnet") == 0) {
            if (++argi < argc) {
                long_value = strtol(argv[argi], NULL, 0);
                if ((long_value >= 0) && (long_value <= UINT16_MAX)) {
                    Target_DNET = (uint16_t)long_value;
                }
            }
        } else {
            if (target_args == 0) {
                device_id = strtol(argv[argi], NULL, 0);
                target_args++;
            }
        }
    }
    if (device_id > BACNET_MAX_INSTANCE) {
        debug_perror("device-instance=%u - it must be less than %u\n",
            device_id, BACNET_MAX_INSTANCE);
        return 1;
    }
    Device_Set_Object_Instance_Number(device_id);
    debug_aprintf("BACnet Server-Discovery Demo\n"
                  "BACnet Stack Version %s\n"
                  "BACnet Device ID: %u\n"
                  "DNET: %u every %lu seconds\n"
                  "Max APDU: %d\n",
        BACnet_Version, Device_Object_Instance_Number(), Target_DNET,
        discover_seconds, MAX_APDU);
    dlenv_init();
    atexit(datalink_cleanup);
    bacnet_server_init();
    bacnet_client_init(discover_seconds);
    atexit(bacnet_device_list_cleanup);
    /* send the first Who-Is */
    bacnet_address_init(&dest, NULL, Target_DNET, NULL);
    Send_WhoIs_To_Network(&dest, -1, -1);
    /* loop forever */
    for (;;) {
        bacnet_server_task();
        bacnet_client_task(&dest);
    }

    return 0;
}
