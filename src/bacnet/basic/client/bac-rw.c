/**
 * @file
 * @brief Read properties from other BACnet devices, and store their values
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "bacnet/abort.h"
#include "bacnet/apdu.h"
#include "bacnet/iam.h"
#include "bacnet/reject.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/ringbuf.h"
#include "bacnet/basic/tsm/tsm.h"
/* me */
#include "bacnet/basic/client/bac-rw.h"

/* timer for address cache */
static struct mstimer Cache_Timer;
#define CACHE_CYCLE_SECONDS 60
/* timeout timer for read-write task */
static struct mstimer Read_Write_Timer;
/* where the data from the read is stored */
static bacnet_read_write_value_callback_t bacnet_read_write_value_callback;
/* where the data from the I-Am is called */
static bacnet_read_write_device_callback_t bacnet_read_write_device_callback;

/* states for client task */
typedef enum {
    BACNET_CLIENT_IDLE,
    BACNET_CLIENT_BIND,
    BACNET_CLIENT_BINDING,
    BACNET_CLIENT_SEND,
    BACNET_CLIENT_WAITING,
    BACNET_CLIENT_FINISHED
} BACNET_CLIENT_STATE;
/* data queue */
typedef struct target_data_t {
    bool write_property;
    uint32_t device_id;
    uint32_t object_instance;
    BACNET_OBJECT_TYPE object_type;
    BACNET_PROPERTY_ID object_property;
    int32_t array_index;
    uint8_t priority;
    /* application tag data type for writing */
    uint8_t tag;
    union {
        /* only supporting limited values for writing */
        bool Boolean;
        float Real;
        uint32_t Enumerated;
        uint32_t Unsigned_Int;
        int32_t Signed_Int;
    } type;
} TARGET_DATA;
#define TARGET_DATA_QUEUE_SIZE (sizeof(struct target_data_t))
/* count must be a power of 2 for ringbuf library */
#ifndef TARGET_DATA_QUEUE_COUNT
#define TARGET_DATA_QUEUE_COUNT 8
#endif
static TARGET_DATA Target_Data_Buffer[TARGET_DATA_QUEUE_COUNT];
static RING_BUFFER Target_Data_Queue;
/* local storage - keeps it off the c-stack */
static BACNET_APPLICATION_DATA_VALUE Target_Decoded_Property_Value;
/* the invoke id is needed to filter incoming messages */
static uint8_t Request_Invoke_ID;
static BACNET_ADDRESS Target_Address;
static uint32_t Target_Device_ID;
static uint16_t Target_Vendor_ID;
static bool Error_Detected = false;
static BACNET_ERROR_CLASS Error_Class;
static BACNET_ERROR_CODE Error_Code;
static BACNET_CLIENT_STATE RW_State = BACNET_CLIENT_IDLE;

/**
 * @brief Handler for an Error PDU.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param invoke_id [in] the invokeID from the rejected message
 * @param error_class [in] the error class
 * @param error_code [in] the error code
 */
static void MyErrorHandler(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        Error_Detected = true;
        Error_Class = error_class;
        Error_Code = error_code;
    }
}

/**
 * @brief Handler for an Abort PDU.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param invoke_id [in] the invokeID from the rejected message
 * @param abort_reason [in] the reason for the message abort
 * @param server
 */
static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    (void)server;
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        Error_Detected = true;
        Error_Class = ERROR_CLASS_SERVICES;
        Error_Code = abort_convert_to_error_code(abort_reason);
    }
}

/**
 * @brief Handler for a Reject PDU.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param invoke_id [in] the invokeID from the rejected message
 * @param reject_reason [in] the reason for the rejection
 */
static void
MyRejectHandler(BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        Error_Detected = true;
        Error_Class = ERROR_CLASS_SERVICES;
        Error_Code = reject_convert_to_error_code(reject_reason);
    }
}

/**
 * @brief Handler for I-Am responses
 * @param service_request [in] The received message to be handled.
 * @param service_len [in] Length of the service_request message.
 * @param src [in] The BACNET_ADDRESS of the message's source.
 */
static void My_I_Am_Bind(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len = 0;
    uint32_t device_id = 0;
    unsigned max_apdu = 0;
    int segmentation = 0;
    uint16_t vendor_id = 0;
    bool found = false;
    bool bind = false;

    (void)service_len;
    len = iam_decode_service_request(
        service_request, &device_id, &max_apdu, &segmentation, &vendor_id);
    if (len > 0) {
        found = address_bind_request(device_id, NULL, NULL);
        if (!found) {
            if (Target_Vendor_ID != 0) {
                if (Target_Vendor_ID == vendor_id) {
                    /* limit binding to specific vendor ID */
                    bind = true;
                }
            } else {
                bind = true;
            }
            if (bind) {
                address_add_binding(device_id, max_apdu, src);
                if (bacnet_read_write_device_callback) {
                    bacnet_read_write_device_callback(
                        device_id, max_apdu, segmentation, vendor_id);
                }
            }
        }
    }

    return;
}

/** Handler for a Simple ACK PDU.
 *
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param invoke_id [in] the invokeID from the rejected message
 */
static void
MyWritePropertySimpleAckHandler(BACNET_ADDRESS *src, uint8_t invoke_id)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        /* nothing to do */
    }
}

/**
 * @brief Process a ReadProperty-ACK message
 * @param device_id [in] The device ID of the source of the message
 * @param rp_data [in] The contents of the service request.
 */
static void bacnet_read_property_ack_process(
    uint32_t device_id, BACNET_READ_PROPERTY_DATA *rp_data)
{
    BACNET_APPLICATION_DATA_VALUE *value;
    uint8_t *apdu;
    int apdu_len, len;
    BACNET_ARRAY_INDEX array_index = 0;

    if (rp_data) {
        value = &Target_Decoded_Property_Value;
        /* check for property error */
        if (rp_data->error_code != ERROR_CODE_SUCCESS) {
            if (bacnet_read_write_value_callback) {
                bacnet_read_write_value_callback(device_id, rp_data, NULL);
            }
            return;
        }
        /* check for empty list */
        if (rp_data->application_data_len == 0) {
            bacapp_value_list_init(value, 1);
            value->tag = BACNET_APPLICATION_TAG_EMPTYLIST;
            rp_data->error_class = ERROR_CLASS_SERVICES;
            rp_data->error_code = ERROR_CODE_SUCCESS;
            if (bacnet_read_write_value_callback) {
                bacnet_read_write_value_callback(device_id, rp_data, value);
            }
            return;
        }
        apdu = rp_data->application_data;
        apdu_len = rp_data->application_data_len;
        while (apdu_len) {
            bacapp_value_list_init(value, 1);
            len = bacapp_decode_known_array_property(
                apdu, (unsigned)apdu_len, value, rp_data->object_type,
                rp_data->object_property, rp_data->array_index);
            if (len > 0) {
                if ((len < apdu_len) &&
                    (rp_data->array_index == BACNET_ARRAY_ALL)) {
                    /* assume that since there is more data that this
                       is an array and split full array of elements
                       into separate RP Acks */
                    array_index = 1;
                }
                rp_data->error_class = ERROR_CLASS_SERVICES;
                rp_data->error_code = ERROR_CODE_SUCCESS;
                if (array_index) {
                    rp_data->array_index = array_index;
                }
                if (bacnet_read_write_value_callback) {
                    bacnet_read_write_value_callback(device_id, rp_data, value);
                }
                /* see if there is any more data */
                if (len < apdu_len) {
                    apdu += len;
                    apdu_len -= len;
                    if (array_index) {
                        array_index++;
                    }
                } else {
                    break;
                }
            } else {
                rp_data->error_class = ERROR_CLASS_SERVICES;
                if (len < 0) {
                    rp_data->error_code = ERROR_CODE_OTHER;
                } else {
                    rp_data->error_code = ERROR_CODE_SUCCESS;
                }
                if (bacnet_read_write_value_callback) {
                    bacnet_read_write_value_callback(device_id, rp_data, NULL);
                }
                break;
            }
        }
    }
}

/** Handler for a ReadProperty ACK.
 *  Saves the data from a matching read-property request
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 * decoded from the APDU header of this message.
 */
static void My_Read_Property_Ack_Handler(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    int len = 0;
    BACNET_READ_PROPERTY_DATA rp_data = { 0 };
    uint32_t device_id = 0;

    if (address_match(&Target_Address, src) &&
        (service_data->invoke_id == Request_Invoke_ID)) {
        address_get_device_id(src, &device_id);
        rp_data.error_code = ERROR_CODE_SUCCESS;
        len = rp_ack_decode_service_request(
            service_request, service_len, &rp_data);
        if (len < 0) {
            /* unable to decode value */
            Error_Detected = true;
            Error_Class = ERROR_CLASS_SERVICES;
            Error_Code = ERROR_CODE_INTERNAL_ERROR;
        } else {
            bacnet_read_property_ack_process(device_id, &rp_data);
        }
    }
}

/** Handler for a ReadPropertyMultiple ACK.
 *  Saves the data from a matching read-property-multiple request
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 * decoded from the APDU header of this message.
 */
static void My_Read_Property_Multiple_Ack_Handler(
    uint8_t *apdu,
    uint16_t apdu_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    BACNET_READ_PROPERTY_DATA rp_data = { 0 };
    uint32_t device_id = 0;

    address_get_device_id(src, &device_id);
    if (address_match(&Target_Address, src) &&
        (service_data->invoke_id == Request_Invoke_ID)) {
        rp_data.error_code = ERROR_CODE_SUCCESS;
        rpm_ack_object_property_process(
            apdu, apdu_len, device_id, &rp_data,
            bacnet_read_property_ack_process);
    }
}

/**
 * @brief Sends a ReadPropertyMultiple service request
 * @param device_id [in] The contents of the service request.
 * @param object_type [in] The contents of the service request.
 * @param object_instance [in] The contents of the service request.
 * @return invoke_id of request
 */
static uint8_t Send_RPM_All_Request(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    BACNET_READ_ACCESS_DATA read_access_data = { 0 };
    BACNET_PROPERTY_REFERENCE property_list = { 0 };
    uint8_t pdu[MAX_PDU] = { 0 };

    /* configure the property list */
    property_list.error.error_class = ERROR_CLASS_DEVICE;
    property_list.error.error_code = ERROR_CODE_OTHER;
    property_list.value = NULL;
    property_list.propertyArrayIndex = BACNET_ARRAY_ALL;
    property_list.propertyIdentifier = PROP_ALL;
    property_list.next = NULL;
    /* configure the read access data */
    read_access_data.listOfProperties = &property_list;
    read_access_data.object_instance = object_instance;
    read_access_data.object_type = object_type;
    read_access_data.next = NULL;

    return Send_Read_Property_Multiple_Request(
        pdu, sizeof(pdu), device_id, &read_access_data);
}

/**
 * @brief Handles the ReadProperty process
 * @param service_request [in] The contents of the service request.
 * @return true if the process is finished
 */
static bool bacnet_read_write_process(const TARGET_DATA *target)
{
    bool found = false;
    unsigned max_apdu = 0;
    uint8_t application_data[16] = { 0 };
    int application_data_len = 0;
    bool valid_tag = false;

    switch (RW_State) {
        case BACNET_CLIENT_IDLE:
            mstimer_set(&Read_Write_Timer, apdu_timeout());
            if (target->device_id < BACNET_MAX_INSTANCE) {
                Error_Detected = false;
                RW_State = BACNET_CLIENT_BIND;
            } else {
                RW_State = BACNET_CLIENT_FINISHED;
            }
            break;
        case BACNET_CLIENT_BIND:
            /* exclude our device - in case our ID changed */
            address_own_device_id_set(Device_Object_Instance_Number());
            /* try to bind with the device */
            found = address_bind_request(
                target->device_id, &max_apdu, &Target_Address);
            if (found) {
                Target_Device_ID = target->device_id;
                RW_State = BACNET_CLIENT_SEND;
            } else {
                Send_WhoIs(target->device_id, target->device_id);
                RW_State = BACNET_CLIENT_BINDING;
            }
            break;
        case BACNET_CLIENT_BINDING:
            found = address_bind_request(
                target->device_id, &max_apdu, &Target_Address);
            if (found) {
                Target_Device_ID = target->device_id;
                mstimer_set(&Read_Write_Timer, apdu_timeout());
                RW_State = BACNET_CLIENT_SEND;
            } else if (mstimer_expired(&Read_Write_Timer)) {
                /* unable to bind within APDU timeout */
                Error_Detected = true;
                Error_Class = ERROR_CLASS_SERVICES;
                Error_Code = ERROR_CODE_TIMEOUT;
                RW_State = BACNET_CLIENT_FINISHED;
            }
            break;
        case BACNET_CLIENT_SEND:
            if (target->write_property) {
                switch (target->tag) {
                    case BACNET_APPLICATION_TAG_NULL:
                        application_data_len =
                            encode_application_null(&application_data[0]);
                        valid_tag = true;
                        break;
                    case BACNET_APPLICATION_TAG_BOOLEAN:
                        application_data_len = encode_application_boolean(
                            &application_data[0], target->type.Boolean);
                        valid_tag = true;
                        break;
                    case BACNET_APPLICATION_TAG_REAL:
                        application_data_len = encode_application_real(
                            &application_data[0], target->type.Real);
                        valid_tag = true;
                        break;
                    case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                        application_data_len = encode_application_unsigned(
                            &application_data[0], target->type.Unsigned_Int);
                        valid_tag = true;
                        break;
                    case BACNET_APPLICATION_TAG_SIGNED_INT:
                        application_data_len = encode_application_signed(
                            &application_data[0], target->type.Signed_Int);
                        valid_tag = true;
                        break;
                    case BACNET_APPLICATION_TAG_ENUMERATED:
                        application_data_len = encode_application_enumerated(
                            &application_data[0], target->type.Enumerated);
                        valid_tag = true;
                        break;
                    default:
                        break;
                }
                if (valid_tag) {
                    Request_Invoke_ID = Send_Write_Property_Request_Data(
                        target->device_id, target->object_type,
                        target->object_instance, target->object_property,
                        &application_data[0], application_data_len,
                        target->priority, target->array_index);
                }
            } else {
                if (target->object_property == PROP_ALL) {
                    Request_Invoke_ID = Send_RPM_All_Request(
                        target->device_id, target->object_type,
                        target->object_instance);
                } else {
                    Request_Invoke_ID = Send_Read_Property_Request(
                        target->device_id, target->object_type,
                        target->object_instance, target->object_property,
                        target->array_index);
                }
            }
            if (Request_Invoke_ID == 0) {
                if (mstimer_expired(&Read_Write_Timer)) {
                    /* TSM Timeout - no invokeIDs available */
                    Error_Detected = true;
                    Error_Class = ERROR_CLASS_SERVICES;
                    Error_Code = ERROR_CODE_TIMEOUT;
                    RW_State = BACNET_CLIENT_FINISHED;
                }
            } else {
                RW_State = BACNET_CLIENT_WAITING;
            }
            break;
        case BACNET_CLIENT_WAITING:
            if (Error_Detected) {
                RW_State = BACNET_CLIENT_FINISHED;
            } else if (tsm_invoke_id_free(Request_Invoke_ID)) {
                Error_Detected = false;
                RW_State = BACNET_CLIENT_FINISHED;
            } else if (tsm_invoke_id_failed(Request_Invoke_ID)) {
                Error_Detected = true;
                Error_Class = ERROR_CLASS_SERVICES;
                Error_Code = ERROR_CODE_ABORT_TSM_TIMEOUT;
                RW_State = BACNET_CLIENT_FINISHED;
                tsm_free_invoke_id(Request_Invoke_ID);
            }
            break;
        case BACNET_CLIENT_FINISHED:
            RW_State = BACNET_CLIENT_IDLE;
            break;
        default:
            break;
    }

    return (RW_State == BACNET_CLIENT_FINISHED);
}

/**
 * @brief Sets the callback for when a read-property returns data
 *
 * @param callback - function for callback
 */
void bacnet_read_write_value_callback_set(
    bacnet_read_write_value_callback_t callback)
{
    bacnet_read_write_value_callback = callback;
}

/**
 * @brief Sets the callback for when an I-Am returns device data
 *
 * @param callback - function for callback
 */
void bacnet_read_write_device_callback_set(
    bacnet_read_write_device_callback_t callback)
{
    bacnet_read_write_device_callback = callback;
}

/**
 * @brief Handles the ReadProperty repetitive task
 */
void bacnet_read_write_task(void)
{
    TARGET_DATA *target;
    bool status = false;
    BACNET_READ_PROPERTY_DATA rp_data;

    if (!Ringbuf_Empty(&Target_Data_Queue)) {
        target = (TARGET_DATA *)Ringbuf_Peek(&Target_Data_Queue);
        status = bacnet_read_write_process(target);
        if (status) {
            if (Error_Detected) {
                if (bacnet_read_write_value_callback) {
                    rp_data.error_class = Error_Class;
                    rp_data.error_code = Error_Code;
                    rp_data.object_type = target->object_type;
                    rp_data.object_instance = target->object_instance;
                    rp_data.object_property = target->object_property;
                    rp_data.array_index = target->array_index;
                    bacnet_read_write_value_callback(
                        target->device_id, &rp_data, NULL);
                }
            }
            Ringbuf_Pop(&Target_Data_Queue, NULL);
        }
    }
    if (mstimer_expired(&Cache_Timer)) {
        mstimer_reset(&Cache_Timer);
        address_cache_timer(CACHE_CYCLE_SECONDS);
    }
}

/**
 * @brief Adds a Read Property request remote data point
 * @param device_id - ID of the destination device
 * @param object_type - Type of the object whose property is to be read.
 * @param object_instance - Instance # of the object to be read.
 * @param object_property - Property to be read, but not ALL, REQUIRED, or
 * OPTIONAL.
 * @param array_index [in] Optional: if the Property is an array,
 *   - 0 for the array size
 *   - 1 to n for individual array members
 *   - BACNET_ARRAY_ALL (~0) for the full array to be read.
 * @return true if added, false if not added
 */
bool bacnet_read_property_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint32_t array_index)
{
    bool status = false;
    TARGET_DATA target;

    target.write_property = false;
    target.device_id = device_id;
    target.object_type = object_type;
    target.object_instance = object_instance;
    target.object_property = object_property;
    target.array_index = array_index;
    status = Ringbuf_Put(&Target_Data_Queue, (uint8_t *)&target);

    return status;
}

/**
 * @brief Adds a WriteProperty request to a remote data point - REAL
 * @param device_id - ID of the destination device
 * @param object_type - Type of the object whose property is to be read.
 * @param object_instance - Instance # of the object to be read.
 * @param object_property - Property to be read, but not ALL, REQUIRED, or
 * OPTIONAL.
 * @param value - property value of type REAL (float)
 * @param priority - BACnet priority for writing 1..16, or 0 if not set
 * @param array_index [in] Optional: if the Property is an array,
 *   - 0 for the array size
 *   - 1 to n for individual array members
 *   - BACNET_ARRAY_ALL (~0) for the full array to be read.
 * @return true if added, false if not added
 */
bool bacnet_write_property_real_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    float value,
    uint8_t priority,
    uint32_t array_index)
{
    bool status = false;
    TARGET_DATA target = { 0 };

    target.write_property = true;
    target.device_id = device_id;
    target.object_type = object_type;
    target.object_instance = object_instance;
    target.object_property = object_property;
    target.tag = BACNET_APPLICATION_TAG_REAL;
    target.type.Real = value;
    target.priority = priority;
    target.array_index = array_index;
    status = Ringbuf_Put(&Target_Data_Queue, (uint8_t *)&target);

    return status;
}

/**
 * @brief Adds a Write Property request to a remote data point
 * @param device_id - ID of the destination device
 * @param object_type - Type of the object whose property is to be read.
 * @param object_instance - Instance # of the object to be read.
 * @param object_property - Property to be read, but not ALL, REQUIRED, or
 * OPTIONAL.
 * @param value - property value of type NULL
 * @param priority - BACnet priority for writing 1..16, or 0 if not set
 * @param array_index [in] Optional: if the Property is an array,
 *   - 0 for the array size
 *   - 1 to n for individual array members
 *   - BACNET_ARRAY_ALL (~0) for the full array to be read.
 * @return true if added, false if not added
 */
bool bacnet_write_property_null_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    uint8_t priority,
    uint32_t array_index)
{
    bool status = false;
    TARGET_DATA target = { 0 };

    target.write_property = true;
    target.device_id = device_id;
    target.object_type = object_type;
    target.object_instance = object_instance;
    target.object_property = object_property;
    target.tag = BACNET_APPLICATION_TAG_NULL;
    target.priority = priority;
    target.array_index = array_index;
    status = Ringbuf_Put(&Target_Data_Queue, (uint8_t *)&target);

    return status;
}

/**
 * @brief Adds a Write Property request to a remote data point
 * @param device_id - ID of the destination device
 * @param object_type - Type of the object whose property is to be read.
 * @param object_instance - Instance # of the object to be read.
 * @param object_property - Property to be read, but not ALL, REQUIRED, or
 * OPTIONAL.
 * @param value - property value of type ENUMERATED
 * @param priority - BACnet priority for writing 1..16, or 0 if not set
 * @param array_index [in] Optional: if the Property is an array,
 *   - 0 for the array size
 *   - 1 to n for individual array members
 *   - BACNET_ARRAY_ALL (~0) for the full array to be read.
 * @return true if added, false if not added
 */
bool bacnet_write_property_enumerated_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    unsigned int value,
    uint8_t priority,
    uint32_t array_index)
{
    bool status = false;
    TARGET_DATA target;

    target.write_property = true;
    target.device_id = device_id;
    target.object_type = object_type;
    target.object_instance = object_instance;
    target.object_property = object_property;
    target.tag = BACNET_APPLICATION_TAG_ENUMERATED;
    target.type.Enumerated = value;
    target.priority = priority;
    target.array_index = array_index;
    status = Ringbuf_Put(&Target_Data_Queue, (uint8_t *)&target);

    return status;
}

/**
 * @brief Adds a Write Property request to a remote data point
 * @param device_id - ID of the destination device
 * @param object_type - Type of the object whose property is to be read.
 * @param object_instance - Instance # of the object to be read.
 * @param object_property - Property to be read, but not ALL, REQUIRED, or
 * OPTIONAL.
 * @param value - property value of type UNSIGNED INT
 * @param priority - BACnet priority for writing 1..16, or 0 if not set
 * @param array_index [in] Optional: if the Property is an array,
 *   - 0 for the array size
 *   - 1 to n for individual array members
 *   - BACNET_ARRAY_ALL (~0) for the full array to be read.
 * @return true if added, false if not added
 */
bool bacnet_write_property_unsigned_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    unsigned int value,
    uint8_t priority,
    uint32_t array_index)
{
    bool status = false;
    TARGET_DATA target;

    target.write_property = true;
    target.device_id = device_id;
    target.object_type = object_type;
    target.object_instance = object_instance;
    target.object_property = object_property;
    target.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    target.type.Unsigned_Int = value;
    target.priority = priority;
    target.array_index = array_index;
    status = Ringbuf_Put(&Target_Data_Queue, (uint8_t *)&target);

    return status;
}

/**
 * @brief Adds a Write Property request to a remote data point
 * @param device_id - ID of the destination device
 * @param object_type - Type of the object whose property is to be read.
 * @param object_instance - Instance # of the object to be read.
 * @param object_property - Property to be read, but not ALL, REQUIRED, or
 * OPTIONAL.
 * @param value - property value of type SIGNED INT
 * @param priority - BACnet priority for writing 1..16, or 0 if not set
 * @param array_index [in] Optional: if the Property is an array,
 *   - 0 for the array size
 *   - 1 to n for individual array members
 *   - BACNET_ARRAY_ALL (~0) for the full array to be read.
 * @return true if added, false if not added
 */
bool bacnet_write_property_signed_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    signed int value,
    uint8_t priority,
    uint32_t array_index)
{
    bool status = false;
    TARGET_DATA target;

    target.write_property = true;
    target.device_id = device_id;
    target.object_type = object_type;
    target.object_instance = object_instance;
    target.object_property = object_property;
    target.tag = BACNET_APPLICATION_TAG_SIGNED_INT;
    target.type.Signed_Int = value;
    target.priority = priority;
    target.array_index = array_index;
    status = Ringbuf_Put(&Target_Data_Queue, (uint8_t *)&target);

    return status;
}

/**
 * @brief Adds a Write Property request to a remote data point
 * @param device_id - ID of the destination device
 * @param object_type - Type of the object whose property is to be read.
 * @param object_instance - Instance # of the object to be read.
 * @param object_property - Property to be read, but not ALL, REQUIRED, or
 * OPTIONAL.
 * @param value - property value of type SIGNED INT
 * @param priority - BACnet priority for writing 1..16, or 0 if not set
 * @param array_index [in] Optional: if the Property is an array,
 *   - 0 for the array size
 *   - 1 to n for individual array members
 *   - BACNET_ARRAY_ALL (~0) for the full array to be read.
 * @return true if added, false if not added
 */
bool bacnet_write_property_boolean_queue(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    bool value,
    uint8_t priority,
    uint32_t array_index)
{
    bool status = false;
    TARGET_DATA target;

    target.write_property = true;
    target.device_id = device_id;
    target.object_type = object_type;
    target.object_instance = object_instance;
    target.object_property = object_property;
    target.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    target.type.Boolean = value;
    target.priority = priority;
    target.array_index = array_index;
    status = Ringbuf_Put(&Target_Data_Queue, (uint8_t *)&target);

    return status;
}

/**
 * @brief Determines if the BACnet ReadProperty queue is empty
 * @return true if the parameter queue is empty, and thus, idle
 */
bool bacnet_read_write_idle(void)
{
    return Ringbuf_Empty(&Target_Data_Queue);
}

/**
 * @brief Determines if the BACnet ReadProperty queue is full
 * @return true if the parameter queue is full, and thus, busy
 */
bool bacnet_read_write_busy(void)
{
    return Ringbuf_Full(&Target_Data_Queue);
}

/**
 * @brief Sets a Vendor ID filter on I-Am bindings to limit the address
 *  cache usage when we are only reading/writing to a specific vendor ID
 * @param vendor_id - vendor ID to filter, 0=no filter
 */
void bacnet_read_write_vendor_id_filter_set(uint16_t vendor_id)
{
    Target_Vendor_ID = vendor_id;
}

/**
 * @brief Gets a Vendor ID filter on I-Am bindings to limit the address
 *  cache usage when we are only reading/writing to a specific vendor ID
 * @return vendor_id - vendor ID to filter, 0=no filter
 */
uint16_t bacnet_read_write_vendor_id_filter(void)
{
    return Target_Vendor_ID;
}

/**
 * @brief Initializes the ReadProperty module
 */
void bacnet_read_write_init(void)
{
    Ringbuf_Initialize(
        &Target_Data_Queue, (uint8_t *)&Target_Data_Buffer,
        sizeof(Target_Data_Buffer), TARGET_DATA_QUEUE_SIZE,
        TARGET_DATA_QUEUE_COUNT);
    /* handle i-am to support binding to other devices */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, My_I_Am_Bind);
    /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, My_Read_Property_Ack_Handler);
    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE,
        My_Read_Property_Multiple_Ack_Handler);
    /* handle the Simple ACK coming back */
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, MyWritePropertySimpleAckHandler);
    /* handle any errors coming back */
    apdu_set_error_handler(SERVICE_CONFIRMED_READ_PROPERTY, MyErrorHandler);
    apdu_set_error_handler(SERVICE_CONFIRMED_WRITE_PROPERTY, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
    /* configure the address cache */
    address_init();
    /* start the cyclic 1 second timer for Address Cache */
    mstimer_set(&Cache_Timer, CACHE_CYCLE_SECONDS * 1000);
}
