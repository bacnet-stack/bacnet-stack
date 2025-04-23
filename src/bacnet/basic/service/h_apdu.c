/**
 * @file
 * @brief Handles Application Protocol Data Units (APDU) for BACnet
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/dcc.h"
#include "bacnet/iam.h"
/* basic objects, services, TSM */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"

/* APDU Timeout in Milliseconds */
static uint16_t Timeout_Milliseconds = 3000;
/* Number of APDU Retries */
static uint8_t Number_Of_Retries = 3;
static uint8_t Local_Network_Priority; /* Fixing test 10.1.2 Network priority */
#if BACNET_SEGMENTATION_ENABLED
/* APDU Segment Timeout in Milliseconds */
static uint16_t Segment_Timeout_Milliseconds = 5000;
static uint8_t max_segments;
#endif

/* a simple table for crossing the services supported */
static BACNET_SERVICES_SUPPORTED
    confirmed_service_supported[MAX_BACNET_CONFIRMED_SERVICE] = {
        SERVICE_SUPPORTED_ACKNOWLEDGE_ALARM,
        SERVICE_SUPPORTED_CONFIRMED_COV_NOTIFICATION,
        SERVICE_SUPPORTED_CONFIRMED_EVENT_NOTIFICATION,
        SERVICE_SUPPORTED_GET_ALARM_SUMMARY,
        SERVICE_SUPPORTED_GET_ENROLLMENT_SUMMARY,
        SERVICE_SUPPORTED_SUBSCRIBE_COV,
        SERVICE_SUPPORTED_ATOMIC_READ_FILE,
        SERVICE_SUPPORTED_ATOMIC_WRITE_FILE,
        SERVICE_SUPPORTED_ADD_LIST_ELEMENT,
        SERVICE_SUPPORTED_REMOVE_LIST_ELEMENT,
        SERVICE_SUPPORTED_CREATE_OBJECT,
        SERVICE_SUPPORTED_DELETE_OBJECT,
        SERVICE_SUPPORTED_READ_PROPERTY,
        SERVICE_SUPPORTED_READ_PROP_CONDITIONAL,
        SERVICE_SUPPORTED_READ_PROP_MULTIPLE,
        SERVICE_SUPPORTED_WRITE_PROPERTY,
        SERVICE_SUPPORTED_WRITE_PROP_MULTIPLE,
        SERVICE_SUPPORTED_DEVICE_COMMUNICATION_CONTROL,
        SERVICE_SUPPORTED_PRIVATE_TRANSFER,
        SERVICE_SUPPORTED_TEXT_MESSAGE,
        SERVICE_SUPPORTED_REINITIALIZE_DEVICE,
        SERVICE_SUPPORTED_VT_OPEN,
        SERVICE_SUPPORTED_VT_CLOSE,
        SERVICE_SUPPORTED_VT_DATA,
        SERVICE_SUPPORTED_AUTHENTICATE,
        SERVICE_SUPPORTED_REQUEST_KEY,
        SERVICE_SUPPORTED_READ_RANGE,
        SERVICE_SUPPORTED_LIFE_SAFETY_OPERATION,
        SERVICE_SUPPORTED_SUBSCRIBE_COV_PROPERTY,
        SERVICE_SUPPORTED_GET_EVENT_INFORMATION,
        SERVICE_SUPPORTED_SUBSCRIBE_COV_PROPERTY_MULTIPLE,
        SERVICE_SUPPORTED_CONFIRMED_COV_NOTIFICATION_MULTIPLE,
        SERVICE_SUPPORTED_CONFIRMED_AUDIT_NOTIFICATION,
        SERVICE_SUPPORTED_AUDIT_LOG_QUERY
    };

/**
 * @brief get the local network priority
 * @return local network priority
 */
uint8_t apdu_network_priority(void)
{
    return Local_Network_Priority;
}

/**
 * @brief set the local network priority
 * @param net - local network priority
 */
void apdu_network_priority_set(uint8_t pri)
{
    Local_Network_Priority = pri & 0x03;
}

/* a simple table for crossing the services supported */
static BACNET_SERVICES_SUPPORTED
    unconfirmed_service_supported[MAX_BACNET_UNCONFIRMED_SERVICE] = {
        SERVICE_SUPPORTED_I_AM,
        SERVICE_SUPPORTED_I_HAVE,
        SERVICE_SUPPORTED_UNCONFIRMED_COV_NOTIFICATION,
        SERVICE_SUPPORTED_UNCONFIRMED_EVENT_NOTIFICATION,
        SERVICE_SUPPORTED_UNCONFIRMED_PRIVATE_TRANSFER,
        SERVICE_SUPPORTED_UNCONFIRMED_TEXT_MESSAGE,
        SERVICE_SUPPORTED_TIME_SYNCHRONIZATION,
        SERVICE_SUPPORTED_WHO_HAS,
        SERVICE_SUPPORTED_WHO_IS,
        SERVICE_SUPPORTED_UTC_TIME_SYNCHRONIZATION,
        SERVICE_SUPPORTED_WRITE_GROUP,
        SERVICE_SUPPORTED_UNCONFIRMED_COV_NOTIFICATION_MULTIPLE,
        SERVICE_SUPPORTED_UNCONFIRMED_AUDIT_NOTIFICATION,
        SERVICE_SUPPORTED_WHO_AM_I,
        SERVICE_SUPPORTED_YOU_ARE
    };

/* Confirmed Function Handlers */
/* If they are not set, they are handled by a reject message */
static confirmed_function Confirmed_Function[MAX_BACNET_CONFIRMED_SERVICE];

/**
 * @brief Set a handler function for the given confirmed service.
 *
 * @param service_choice Service, see SERVICE_CONFIRMED_X enumeration.
 * @param pFunction  Pointer to the function, being in charge of the service.
 */
void apdu_set_confirmed_handler(
    BACNET_CONFIRMED_SERVICE service_choice, confirmed_function pFunction)
{
    if (service_choice < MAX_BACNET_CONFIRMED_SERVICE) {
        Confirmed_Function[service_choice] = pFunction;
    }
}

/* Allow the APDU handler to automatically reject */
static confirmed_function Unrecognized_Service_Handler;

/**
 * @brief Set a handler function called for an unsupported service.
 *
 * @param pFunction  Pointer to the function, being in charge,
 *                   if a unsupported service has been requested.
 */
void apdu_set_unrecognized_service_handler_handler(confirmed_function pFunction)
{
    Unrecognized_Service_Handler = pFunction;
}

/* Unconfirmed Function Handlers */
/* If they are not set, they are not handled */
static unconfirmed_function
    Unconfirmed_Function[MAX_BACNET_UNCONFIRMED_SERVICE];

/**
 * @brief Set a handler function for the given unconfirmed service.
 *
 * @param service_choice Service, see SERVICE_UNCONFIRMED_X enumeration.
 * @param pFunction  Pointer to the function, being in charge of the service.
 */
void apdu_set_unconfirmed_handler(
    BACNET_UNCONFIRMED_SERVICE service_choice, unconfirmed_function pFunction)
{
    if (service_choice < MAX_BACNET_UNCONFIRMED_SERVICE) {
        Unconfirmed_Function[service_choice] = pFunction;
    }
}

/**
 * @brief Checks if the given service is supported or not.
 *
 * @param service_supported  Service in question
 *
 * @return true/false
 */
bool apdu_service_supported(BACNET_SERVICES_SUPPORTED service_supported)
{
    int i = 0;
    bool status = false;
    bool found = false;

    if (service_supported < MAX_BACNET_SERVICES_SUPPORTED) {
        /* is it a confirmed service? */
        for (i = 0; i < MAX_BACNET_CONFIRMED_SERVICE; i++) {
            if (confirmed_service_supported[i] == service_supported) {
                found = true;
                if (Confirmed_Function[i] != NULL) {
#ifdef BAC_ROUTING
                    /* Check to see if the current Device supports this service.
                     */
                    int len = Routed_Device_Service_Approval(
                        confirmed_service_supported[i], 0, NULL, 0);
                    if (len > 0) {
                        break; /* Not supported - return false */
                    }
#endif

                    status = true;
                }
                break;
            }
        }

        if (!found) {
            /* is it an unconfirmed service? */
            for (i = 0; i < MAX_BACNET_UNCONFIRMED_SERVICE; i++) {
                if (unconfirmed_service_supported[i] == service_supported) {
                    if (Unconfirmed_Function[i] != NULL) {
                        status = true;
                    }
                    break;
                }
            }
        }
    }
    return status;
}

/** Function to translate a SERVICE_SUPPORTED_ enum to its SERVICE_CONFIRMED_
 *  or SERVICE_UNCONFIRMED_ index.
 *  Useful with the bactext_confirmed_service_name() functions.
 *
 * @param service_supported [in] The SERVICE_SUPPORTED_ enum value to convert.
 * @param index [out] The SERVICE_CONFIRMED_ or SERVICE_UNCONFIRMED_ index,
 *                    if found.
 * @param bIsConfirmed [out] True if index is a SERVICE_CONFIRMED_ type.
 * @return True if a match was found and index and bIsConfirmed are valid.
 */
bool apdu_service_supported_to_index(
    BACNET_SERVICES_SUPPORTED service_supported,
    size_t *index,
    bool *bIsConfirmed)
{
    int i = 0;
    bool found = false;

    *bIsConfirmed = false;
    if (service_supported < MAX_BACNET_SERVICES_SUPPORTED) {
        /* is it a confirmed service? */
        for (i = 0; i < MAX_BACNET_CONFIRMED_SERVICE; i++) {
            if (confirmed_service_supported[i] == service_supported) {
                found = true;
                *index = (size_t)i;
                *bIsConfirmed = true;
                break;
            }
        }

        if (!found) {
            /* is it an unconfirmed service? */
            for (i = 0; i < MAX_BACNET_UNCONFIRMED_SERVICE; i++) {
                if (unconfirmed_service_supported[i] == service_supported) {
                    found = true;
                    *index = (size_t)i;
                    break;
                }
            }
        }
    }
    return found;
}

/* Confirmed ACK Function Handlers */
static union {
    confirmed_simple_ack_function simple;
    confirmed_ack_function complex;
} Confirmed_ACK_Function[MAX_BACNET_CONFIRMED_SERVICE];

/**
 * @brief Determine if the BACnet service is a Simple Ack Service
 * @param service_choice [in] BACnet confirmed service choice
 */
bool apdu_confirmed_simple_ack_service(BACNET_CONFIRMED_SERVICE service_choice)
{
    bool status = false;

    switch (service_choice) {
        case SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM:
        case SERVICE_CONFIRMED_AUDIT_NOTIFICATION:
        case SERVICE_CONFIRMED_COV_NOTIFICATION:
        case SERVICE_CONFIRMED_COV_NOTIFICATION_MULTIPLE:
        case SERVICE_CONFIRMED_EVENT_NOTIFICATION:
        case SERVICE_CONFIRMED_SUBSCRIBE_COV:
        case SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY:
        case SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY_MULTIPLE:
        case SERVICE_CONFIRMED_LIFE_SAFETY_OPERATION:
            /* Object Access Services */
        case SERVICE_CONFIRMED_ADD_LIST_ELEMENT:
        case SERVICE_CONFIRMED_REMOVE_LIST_ELEMENT:
        case SERVICE_CONFIRMED_DELETE_OBJECT:
        case SERVICE_CONFIRMED_WRITE_PROPERTY:
        case SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE:
            /* Remote Device Management Services */
        case SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL:
        case SERVICE_CONFIRMED_TEXT_MESSAGE:
        case SERVICE_CONFIRMED_REINITIALIZE_DEVICE:
            /* Virtual Terminal Services */
        case SERVICE_CONFIRMED_VT_CLOSE:
            /* Security Services */
        case SERVICE_CONFIRMED_REQUEST_KEY:
            status = true;
            break;
        default:
            break;
    }

    return status;
}

/**
 * @brief Set the the BACnet Simple Ack Service handler
 * @param service_choice [in] BACnet confirmed service choice
 * @param pFunction [in] handler for the service
 */
void apdu_set_confirmed_simple_ack_handler(
    BACNET_CONFIRMED_SERVICE service_choice,
    confirmed_simple_ack_function pFunction)
{
    if (apdu_confirmed_simple_ack_service(service_choice)) {
        Confirmed_ACK_Function[service_choice].simple = pFunction;
    }
}

/**
 * @brief Set the the BACnet Confirmed Ack Service handler
 * @param service_choice [in] BACnet confirmed service choice
 * @param pFunction [in] handler for the service
 */
void apdu_set_confirmed_ack_handler(
    BACNET_CONFIRMED_SERVICE service_choice, confirmed_ack_function pFunction)
{
    if (!apdu_confirmed_simple_ack_service(service_choice)) {
        if (service_choice < MAX_BACNET_CONFIRMED_SERVICE) {
            Confirmed_ACK_Function[service_choice].complex = pFunction;
        }
    }
}

static union {
    error_function error;
    complex_error_function complex;
} Error_Function[MAX_BACNET_CONFIRMED_SERVICE];

/**
 * @brief Determine if the service uses Complex Error function
 * @param service_choice  Service, see SERVICE_CONFIRMED_X enumeration.
 * @return true if the service uses a Complex Error function
 */
bool apdu_complex_error(uint8_t service_choice)
{
    bool status = false;

    switch (service_choice) {
        case SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY_MULTIPLE:
        case SERVICE_CONFIRMED_ADD_LIST_ELEMENT:
        case SERVICE_CONFIRMED_REMOVE_LIST_ELEMENT:
        case SERVICE_CONFIRMED_CREATE_OBJECT:
        case SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE:
        case SERVICE_CONFIRMED_PRIVATE_TRANSFER:
        case SERVICE_CONFIRMED_VT_CLOSE:
            status = true;
            break;
        default:
            break;
    }

    return status;
}

/**
 * @brief Set a error handler function for the given confirmed service.
 *
 * @param service_choice  Service, see SERVICE_CONFIRMED_X enumeration.
 * @param pFunction  Pointer to the function, being in charge of the error
 * handling.
 */
void apdu_set_error_handler(
    BACNET_CONFIRMED_SERVICE service_choice, error_function pFunction)
{
    if ((service_choice < MAX_BACNET_CONFIRMED_SERVICE) &&
        (!apdu_complex_error(service_choice))) {
        Error_Function[service_choice].error = pFunction;
    }
}

/**
 * @brief Set a complex error handler function for the given confirmed service.
 *
 * @param service_choice  Service, see SERVICE_CONFIRMED_X enumeration.
 * @param pFunction  Pointer to the function, being in charge of the error
 * handling.
 */
void apdu_set_complex_error_handler(
    BACNET_CONFIRMED_SERVICE service_choice, complex_error_function pFunction)
{
    if (apdu_complex_error(service_choice)) {
        Error_Function[service_choice].complex = pFunction;
    }
}

static abort_function Abort_Function;

void apdu_set_abort_handler(abort_function pFunction)
{
    Abort_Function = pFunction;
}

static reject_function Reject_Function;

/**
 * @brief Set a handler function called for a rejected service.
 *
 * @param pFunction  Pointer to the function, being in charge,
 *                   if a service request will be rejected.
 */
void apdu_set_reject_handler(reject_function pFunction)
{
    Reject_Function = pFunction;
}

/**
 * @brief Decode the given confirmed service request from the received data.
 *
 * @param apdu  Received data buffer.
 * @param apdu_len  Count of valid bytes in the receive buffer.
 * @param service_data  Pointer of the structure being filled with the service
 *                      data.
 * @param service_choice  Pointer to a variable taking the service requested,
 *                        see SERVICE_CONFIRMED_X
 * @param service_request  Pointer to a pointer that will take the pointer
 *                         were the service request data can be found in
 *                         the receive buffer.
 * @param service_request_len  Pointer to a variable that takes the length
 *                             of the service request data.
 *
 * @return The length of the service data  and reflects the offset, where
 *         we are in PDU after the service had been decoded.
 */
uint16_t apdu_decode_confirmed_service_request(
    uint8_t *apdu, /* APDU data */
    uint16_t apdu_len,
    BACNET_CONFIRMED_SERVICE_DATA *service_data,
    uint8_t *service_choice,
    uint8_t **service_request,
    uint16_t *service_request_len)
{
    uint16_t len = 0; /* counts where we are in PDU */

    if (apdu_len >= 3) {
        service_data->segmented_message = (apdu[0] & BIT(3)) ? true : false;
        service_data->more_follows = (apdu[0] & BIT(2)) ? true : false;
        service_data->segmented_response_accepted =
            (apdu[0] & BIT(1)) ? true : false;
        service_data->max_segs = decode_max_segs(apdu[1]);
        service_data->max_resp = decode_max_apdu(apdu[1]);
        service_data->invoke_id = apdu[2];
        service_data->priority = apdu_network_priority();
        len = 3;
        if (service_data->segmented_message) {
            if (apdu_len >= (len + 2)) {
                service_data->sequence_number = apdu[len++];
                service_data->proposed_window_number = apdu[len++];
            } else {
                return 0;
            }
        }
        if (apdu_len > MAX_APDU) {
            return 0;
        } else if (apdu_len == (len + 1)) {
            /* no request data as seen with Inneasoft BACnet Explorer */
            *service_choice = apdu[len++];
            *service_request = NULL;
            *service_request_len = 0;
        } else if (apdu_len >= (len + 2)) {
            *service_choice = apdu[len++];
            *service_request = &apdu[len];
            *service_request_len = apdu_len - len;
        } else {
            return 0;
        }
    }

    return len;
}

uint16_t apdu_timeout(void)
{
    return Timeout_Milliseconds;
}

void apdu_timeout_set(uint16_t milliseconds)
{
    Timeout_Milliseconds = milliseconds;
}

uint8_t apdu_retries(void)
{
    return Number_Of_Retries;
}

void apdu_retries_set(uint8_t value)
{
    Number_Of_Retries = value;
}

static bool apdu_confirmed_dcc_disabled(uint8_t service_choice)
{
    bool status = false;

    if (dcc_communication_disabled()) {
        switch (service_choice) {
            case SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL:
            case SERVICE_CONFIRMED_REINITIALIZE_DEVICE:
                break;
            default:
                status = true;
                break;
        }
    }
    else if (dcc_communication_initiation_disabled()) {
        switch (service_choice) {
            case SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL:
            case SERVICE_CONFIRMED_REINITIALIZE_DEVICE:
            /* WhoIs will be processed and I-Am initiated as response. */
            case SERVICE_UNCONFIRMED_WHO_IS:
            case SERVICE_UNCONFIRMED_WHO_HAS:
            case SERVICE_CONFIRMED_AUDIT_NOTIFICATION:
            case SERVICE_UNCONFIRMED_AUDIT_NOTIFICATION:
                break;
            default:
                status = true;
                break;
        }
    }

    return status;
}

/**
 * When network communications are completely disabled,
 * only DeviceCommunicationControl and ReinitializeDevice APDUs
 * shall be processed and no messages shall be initiated.
 * If the request is valid and the 'Enable/Disable' parameter is
 * DISABLE_INITIATION, the responding BACnet-user shall
 * discontinue the initiation of messages except for I-Am
 * requests issued in accordance with the Who-Is service procedure.
 *
 * @param service_choice  Service, like SERVICE_UNCONFIRMED_WHO_IS
 *
 * @return true, if being disabled.
 */
static bool apdu_unconfirmed_dcc_disabled(uint8_t service_choice)
{
    bool status = false;

    if (dcc_communication_disabled()) {
        /* there are no Unconfirmed messages that
           can be processed in this state */
        status = true;
    } else if (dcc_communication_initiation_disabled()) {
        /* WhoIs & WhoHas will be processed */
        switch (service_choice) {
            case SERVICE_UNCONFIRMED_WHO_IS:
            case SERVICE_UNCONFIRMED_WHO_HAS:
                break;
            default:
                status = true;
                break;
        }
    }

    return status;
}

/* Invoke special handler for confirmed service */
void invoke_confirmed_service_service_request(
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data,
    uint8_t service_choice,
    uint8_t *service_request,
    uint32_t service_request_len)
{
    if (apdu_confirmed_dcc_disabled(service_choice)) {
        /* When network communications are completely disabled,
            only DeviceCommunicationControl and ReinitializeDevice
            APDUs shall be processed and no messages shall be
            initiated. */
        return;
    }
    if ((service_choice < MAX_BACNET_CONFIRMED_SERVICE) &&
        (Confirmed_Function[service_choice])) {
        Confirmed_Function[service_choice](
            service_request, service_request_len, src, service_data);
    } else if (Unrecognized_Service_Handler) {
        Unrecognized_Service_Handler(
            service_request, service_request_len, src, service_data);
    }
}

#if BACNET_SEGMENTATION_ENABLED
/** Handler for messages with segmentation :
   - store new packet if sequence number is ok
   - NACK packet if sequence number is not ok
   - call the final functions with reassembled data when last packet ok is
   received
*/
void apdu_handler_confirmed_service_segment(
    BACNET_ADDRESS *src,
    uint8_t *apdu, /* APDU data */
    uint32_t apdu_len)
{
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    uint8_t internal_service_id = 0;
    uint8_t service_choice = 0;
    uint8_t *service_request = NULL;
    uint16_t service_request_len = 0;
    uint32_t len = 0; /* counts where we are in PDU */
    bool segment_ok;

    len = apdu_decode_confirmed_service_request(
        &apdu[0], /* APDU data */
        apdu_len, &service_data, &service_choice, &service_request,
        &service_request_len);

    if (len == 0) {
        /* service data unable to be decoded - simply drop */
        return;
    }
    /* new segment : memorize it */
    segment_ok = tsm_set_segmented_confirmed_service_received(
        src, &service_data, &internal_service_id, &service_request,
        &service_request_len);
    /* last segment  */
    if (segment_ok && !service_data.more_follows) {
        /* Clear peer informations */
        tsm_clear_peer_id(internal_service_id);
        /* Invoke service handler */
        invoke_confirmed_service_service_request(
            src, &service_data, service_choice, service_request,
            service_request_len);
        /* We must free invoke_id, and associated data */
        tsm_free_invoke_id_check(internal_service_id, NULL, true);
    }
}
#endif

/* Handler for normal message without segmentation, or segmented complete
 * message reassembled all-in-one */
void apdu_handler_confirmed_service(
    BACNET_ADDRESS *src,
    uint8_t *apdu, /* APDU data */
    uint32_t apdu_len)
{
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    uint8_t service_choice = 0;
    uint8_t *service_request = NULL;
    uint16_t service_request_len = 0;
    uint32_t len = 0; /* counts where we are in PDU */

    len = apdu_decode_confirmed_service_request(
        &apdu[0], /* APDU data */
        apdu_len, &service_data, &service_choice, &service_request,
        &service_request_len);

#if BACNET_SEGMENTATION_ENABLED
    /* Check for unexpected request is received in active TSM state */
    if (check_unexpected_pdu_received(src,&service_data))
    {
        return;
    }
#endif
    invoke_confirmed_service_service_request(
        src, &service_data, service_choice, service_request,
        service_request_len);
}

/** Process the APDU header and invoke the appropriate service handler
 * to manage the received request.
 * Almost all requests and ACKs invoke this function.
 * @ingroup MISCHNDLR
 *
 * @param src [in] The BACNET_ADDRESS of the message's source.
 * @param apdu [in] The apdu portion of the request, to be processed.
 * @param apdu_len [in] The total (remaining) length of the apdu.
 */
void apdu_handler(
    BACNET_ADDRESS *src,
    uint8_t *apdu, /* APDU data */
    uint16_t apdu_len)
{
    BACNET_PDU_TYPE pdu_type;
    uint8_t service_choice = 0;
    uint8_t *service_request = NULL;
    uint16_t service_request_len = 0;
    int len = 0; /* counts where we are in PDU */
#if BACNET_SEGMENTATION_ENABLED
    uint8_t sequence_number = 0;
    uint8_t actual_window_size = 0;
    bool nak = false;
#endif
#if !BACNET_SVC_SERVER
    uint8_t invoke_id = 0;
    BACNET_CONFIRMED_SERVICE_ACK_DATA service_ack_data = { 0 };
    BACNET_ERROR_CODE error_code = ERROR_CODE_SUCCESS;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_SERVICES;
    uint8_t reason = 0;
    bool server = false;
#endif

    if (!apdu) {
        return;
    }
    if (apdu_len == 0) {
        return;
    }
    pdu_type = apdu[0] & 0xF0;
    switch (pdu_type) {
        case PDU_TYPE_CONFIRMED_SERVICE_REQUEST:
            /* segmented_message_reception ? */
#if BACNET_SEGMENTATION_ENABLED
            if (apdu[0] & BIT(3)) {
                apdu_handler_confirmed_service_segment(src, apdu, apdu_len);
            } else 
#endif
            {
                apdu_handler_confirmed_service(src, apdu, apdu_len);
            }
            break;
        case PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST:
            if (apdu_len < 2) {
                break;
            }
            service_choice = apdu[1];
            /* prepare the service request buffer and length */
            service_request_len = apdu_len - 2;
            service_request = &apdu[2];
            if (apdu_unconfirmed_dcc_disabled(service_choice)) {
                /* When network communications are disabled,
                    only DeviceCommunicationControl and
                    ReinitializeDevice APDUs shall be processed and no
                    messages shall be initiated. If communications have
                    been initiation disabled, then WhoIs may be
                    processed. */
                break;
            }
            if (service_choice < MAX_BACNET_UNCONFIRMED_SERVICE) {
                if (Unconfirmed_Function[service_choice]) {
                    Unconfirmed_Function[service_choice](
                        service_request, service_request_len, src);
                }
            }
            break;
#if !BACNET_SVC_SERVER
        case PDU_TYPE_SIMPLE_ACK:
            if (apdu_len < 3) {
                break;
            }
            invoke_id = apdu[1];
            service_choice = apdu[2];
            if (apdu_confirmed_simple_ack_service(service_choice)) {
                if (Confirmed_ACK_Function[service_choice].simple != NULL) {
                    Confirmed_ACK_Function[service_choice].simple(
                        src, invoke_id);
                }
                tsm_free_invoke_id(invoke_id);
            }
            break;
        case PDU_TYPE_COMPLEX_ACK:
            if (apdu_len < 3) {
                break;
            }
            service_ack_data.segmented_message =
                (apdu[0] & BIT(3)) ? true : false;
            service_ack_data.more_follows = (apdu[0] & BIT(2)) ? true : false;
            invoke_id = service_ack_data.invoke_id = apdu[1];
            len = 2;
            if (service_ack_data.segmented_message) {
                if (apdu_len < 5) {
                    break;
                }
                service_ack_data.sequence_number = apdu[len++];
                service_ack_data.proposed_window_number = apdu[len++];
            }
            service_choice = apdu[len++];
            /* prepare the service request buffer and length */
            service_request_len = apdu_len - (uint16_t)len;
            service_request = &apdu[len];
            if (!apdu_confirmed_simple_ack_service(service_choice)) {
                if (service_choice < MAX_BACNET_CONFIRMED_SERVICE) {
                    if (Confirmed_ACK_Function[service_choice].complex !=
                        NULL) {
                        Confirmed_ACK_Function[service_choice].complex(
                            service_request, service_request_len, src,
                            &service_ack_data);
                    }
                }
                tsm_free_invoke_id(invoke_id);
            }
            break;
        case PDU_TYPE_SEGMENT_ACK:
#if! BACNET_SEGMENTATION_ENABLED
            /* FIXME: what about a denial of service attack here?
                we could check src to see if that matched the tsm */
            tsm_free_invoke_id(invoke_id);
#else
            if (apdu_len < 4) {
                break;
            }
            server = apdu[0] & 0x01;
            nak = apdu[0] & 0x02;
            invoke_id = apdu[1];
            sequence_number = apdu[2];
            actual_window_size = apdu[3];
            /* we care because we support segmented message sending */
            tsm_segmentack_received(
                invoke_id, sequence_number, actual_window_size, nak, server,
                src);
#endif
            break;
        case PDU_TYPE_ERROR:
            if (apdu_len < 3) {
                break;
            }
            invoke_id = apdu[1];
            service_choice = apdu[2];
            /* prepare the service request buffer and length */
            service_request_len = apdu_len - 3;
            service_request = &apdu[3];
            if (apdu_complex_error(service_choice)) {
                if (Error_Function[service_choice].complex) {
                    Error_Function[service_choice].complex(
                        src, invoke_id, service_choice, service_request,
                        service_request_len);
                }
            } else if (service_choice < MAX_BACNET_CONFIRMED_SERVICE) {
                len = bacerror_decode_error_class_and_code(
                    service_request, service_request_len, &error_class,
                    &error_code);
                if ((len != 0) && (Error_Function[service_choice].error)) {
                    Error_Function[service_choice].error(
                        src, invoke_id, (BACNET_ERROR_CLASS)error_class,
                        (BACNET_ERROR_CODE)error_code);
                }
            }
#if BACNET_SEGMENTATION_ENABLED
            /*Release the data*/
            tsm_free_invoke_id_segmentation(src, invoke_id);
#else
            tsm_free_invoke_id(invoke_id);
#endif
            break;
        case PDU_TYPE_REJECT:
            if (apdu_len < 3) {
                break;
            }
            invoke_id = apdu[1];
            reason = apdu[2];
            if (Reject_Function) {
                Reject_Function(src, invoke_id, reason);
            }
#if BACNET_SEGMENTATION_ENABLED
            /*Release the data*/
            tsm_free_invoke_id_segmentation(src, invoke_id);
#else
            tsm_free_invoke_id(invoke_id);
#endif
            break;
        case PDU_TYPE_ABORT:
            if (apdu_len < 3) {
                break;
            }
            server = apdu[0] & 0x01;
            invoke_id = apdu[1];
            reason = apdu[2];
            if (!server) {
                /*AbortPDU_Received*/
                if (Abort_Function) {
                    Abort_Function(src, invoke_id, reason, server);
                }
            }
#if BACNET_SEGMENTATION_ENABLED
            else {
                /*SendAbort*/
                abort_pdu_send(invoke_id, src, reason, server);
            }
            /*Release the data*/
            tsm_free_invoke_id_segmentation(src, invoke_id);
#else
            tsm_free_invoke_id(invoke_id);
#endif
            break;
#endif
        default:
            break;
    }
}

#if BACNET_SEGMENTATION_ENABLED
/*Return the APDU segment timeout*/
uint16_t apdu_segment_timeout(void)
{
    return Segment_Timeout_Milliseconds;
}

/*Set the APDU segment timeout*/
void apdu_segment_timeout_set(uint16_t milliseconds)
{
    Segment_Timeout_Milliseconds = milliseconds;
}

/** Process the APDU header and invoke the appropriate service handler
 * to manage the received request.
 * Almost all requests and ACKs invoke this function.
 * @ingroup MISCHNDLR
 *
 * @param apdu [in] The apdu portion of the response, to be sent
 * @param fixed_pdu_header [in] The apdu header for the response
 * @return apdu_length[out] The length of the apdu header
 */
int apdu_encode_fixed_header(
    uint8_t *apdu, BACNET_APDU_FIXED_HEADER *fixed_pdu_header)
{
    int apdu_len = 0;
    switch (fixed_pdu_header->pdu_type) {
        case PDU_TYPE_CONFIRMED_SERVICE_REQUEST:
            apdu[apdu_len++] = fixed_pdu_header->pdu_type
                /* flag 'SA' if we accept many segments */
                | (fixed_pdu_header->service_data.request_data
                           .segmented_response_accepted
                       ? 0x02
                       : 0x00)
                /* flag 'MOR' if we more segments are coming */
                | (fixed_pdu_header->service_data.request_data.more_follows
                       ? 0x04
                       : 0x00)
                /* flag 'SEG' if we more segments are coming */
                | (fixed_pdu_header->service_data.request_data.segmented_message
                       ? 0x08
                       : 0x00);
            apdu[apdu_len++] = encode_max_segs_max_apdu(
                fixed_pdu_header->service_data.request_data.max_segs,
                fixed_pdu_header->service_data.request_data.max_resp);
            apdu[apdu_len++] =
                fixed_pdu_header->service_data.request_data.invoke_id;
            /* extra data for segmented messages sending */
            if (fixed_pdu_header->service_data.request_data.segmented_message) {
                /* packet sequence number */
                apdu[apdu_len++] =
                    fixed_pdu_header->service_data.request_data.sequence_number;
                /* window size proposal */
                apdu[apdu_len++] = fixed_pdu_header->service_data.request_data
                                       .proposed_window_number;
            }
            /* service choice */
            apdu[apdu_len++] = fixed_pdu_header->service_choice;
            break;
        case PDU_TYPE_COMPLEX_ACK:
            apdu[apdu_len++] = fixed_pdu_header->pdu_type
                /* flag 'MOR' if we more segments are coming */
                | (fixed_pdu_header->service_data.ack_data.more_follows ? 0x04
                                                                        : 0x00)
                /* flag 'SEG' if we more segments are coming */
                | (fixed_pdu_header->service_data.ack_data.segmented_message
                       ? 0x08
                       : 0x00);
            apdu[apdu_len++] =
                fixed_pdu_header->service_data.ack_data.invoke_id;
            /* extra data for segmented messages sending */
            if (fixed_pdu_header->service_data.ack_data.segmented_message) {
                /* packet sequence number */
                apdu[apdu_len++] =
                    fixed_pdu_header->service_data.ack_data.sequence_number;
                /* window size proposal */
                apdu[apdu_len++] = fixed_pdu_header->service_data.ack_data
                                       .proposed_window_number;
            }
            /* service choice */
            apdu[apdu_len++] = fixed_pdu_header->service_choice;
            break;
        default:
            break;
    }
    return apdu_len;
}

/** Handler to assign the header fields to the response
 * The response can be segmented/unsegmented
 *
 * @param fixed_pdu_header [in] The apdu header of the response, to be sent.
 * @param pdu_type [in] The pdu_type of the response.
 * @param invoke_id [in] The invoike_id of the response
 * @param service[in] The service choice for which the response has to be
 * processed
 * @param max_apdu[in]  The maximum apdu length
 */
void apdu_init_fixed_header(
    BACNET_APDU_FIXED_HEADER *fixed_pdu_header,
    uint8_t pdu_type,
    uint8_t invoke_id,
    uint8_t service,
    int max_apdu)
{
    fixed_pdu_header->pdu_type = pdu_type;

    fixed_pdu_header->service_data.common_data.invoke_id = invoke_id;
    fixed_pdu_header->service_data.common_data.more_follows = false;
    fixed_pdu_header->service_data.common_data.proposed_window_number = 0;
    fixed_pdu_header->service_data.common_data.sequence_number = 0;
    fixed_pdu_header->service_data.common_data.segmented_message = false;
    switch (pdu_type) {
        case PDU_TYPE_CONFIRMED_SERVICE_REQUEST:
            fixed_pdu_header->service_data.request_data.max_segs =
                MAX_SEGMENTS_ACCEPTED;
            /* allow to specify a lower APDU size : support arbitrary reduction
             * of APDU packets between peers */
            fixed_pdu_header->service_data.request_data.max_resp =
                max_apdu < MAX_APDU ? max_apdu : MAX_APDU;
            fixed_pdu_header->service_data.request_data
                .segmented_response_accepted = MAX_SEGMENTS_ACCEPTED > 1;
            break;
        case PDU_TYPE_COMPLEX_ACK:
        default:
            break;
    }
    fixed_pdu_header->service_choice = service;
}

void max_segments_accepted_set(uint8_t maxSegments)
{ 
    max_segments = maxSegments;
}

uint8_t max_segments_accepted_get(void)
{
   return max_segments;
}
#endif
