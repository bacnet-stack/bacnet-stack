/**
 * @file
 * @brief Header file for a basic APDU Handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_SERVICE_APDU_HANDLER_H
#define BACNET_BASIC_SERVICE_APDU_HANDLER_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* generic unconfirmed function handler */
/* Suitable to handle the following services: */
/* I_Am, Who_Is, Unconfirmed_COV_Notification, I_Have, */
/* Unconfirmed_Event_Notification, Unconfirmed_Private_Transfer, */
/* Unconfirmed_Text_Message, Time_Synchronization, Who_Has, */
/* UTC_Time_Synchronization */
typedef void (*unconfirmed_function)(
    uint8_t *service_request, uint16_t len, BACNET_ADDRESS *src);

/* generic confirmed function handler */
/* Suitable to handle the following services: */
/* Acknowledge_Alarm, Confirmed_COV_Notification, */
/* Confirmed_Event_Notification, Get_Alarm_Summary, */
/* Get_Enrollment_Summary_Handler, Get_Event_Information, */
/* Subscribe_COV_Handler, Subscribe_COV_Property, */
/* Life_Safety_Operation, Atomic_Read_File, */
/* Confirmed_Atomic_Write_File, Add_List_Element, */
/* Remove_List_Element, Create_Object_Handler, */
/* Delete_Object_Handler, Read_Property, */
/* Read_Property_Conditional, Read_Property_Multiple, Read_Range, */
/* Write_Property, Write_Property_Multiple, */
/* Device_Communication_Control, Confirmed_Private_Transfer, */
/* Confirmed_Text_Message, Reinitialize_Device, */
/* VT_Open, VT_Close, VT_Data_Handler, */
/* Authenticate, Request_Key */
typedef void (*confirmed_function)(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data);

/* generic confirmed simple ack function handler */
typedef void (*confirmed_simple_ack_function)(
    BACNET_ADDRESS *src, uint8_t invoke_id);

/* generic confirmed ack function handler */
typedef void (*confirmed_ack_function)(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data);

/* generic error reply function */
typedef void (*error_function)(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code);

/* complex error reply function */
typedef void (*complex_error_function)(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    uint8_t service_choice,
    uint8_t *service_request,
    uint16_t service_len);

/* generic abort reply function */
typedef void (*abort_function)(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server);

/* generic reject reply function */
typedef void (*reject_function)(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason);

BACNET_STACK_EXPORT
void apdu_set_confirmed_ack_handler(
    BACNET_CONFIRMED_SERVICE service_choice, confirmed_ack_function pFunction);

BACNET_STACK_EXPORT
bool apdu_confirmed_simple_ack_service(BACNET_CONFIRMED_SERVICE service_choice);

BACNET_STACK_EXPORT
void apdu_set_confirmed_simple_ack_handler(
    BACNET_CONFIRMED_SERVICE service_choice,
    confirmed_simple_ack_function pFunction);

/* configure reject for confirmed services that are not supported */
BACNET_STACK_EXPORT
void apdu_set_unrecognized_service_handler_handler(
    confirmed_function pFunction);

BACNET_STACK_EXPORT
void apdu_set_confirmed_handler(
    BACNET_CONFIRMED_SERVICE service_choice, confirmed_function pFunction);

BACNET_STACK_EXPORT
void apdu_set_unconfirmed_handler(
    BACNET_UNCONFIRMED_SERVICE service_choice, unconfirmed_function pFunction);

/* returns true if the service is supported by a handler */
BACNET_STACK_EXPORT
bool apdu_service_supported(BACNET_SERVICES_SUPPORTED service_supported);

/* Function to translate a SERVICE_SUPPORTED_ enum to its SERVICE_CONFIRMED_
 *  or SERVICE_UNCONFIRMED_ index.
 */
BACNET_STACK_EXPORT
bool apdu_service_supported_to_index(
    BACNET_SERVICES_SUPPORTED service_supported,
    size_t *index,
    bool *bIsConfirmed);

BACNET_STACK_EXPORT
bool apdu_complex_error(uint8_t service_choice);

BACNET_STACK_EXPORT
void apdu_set_error_handler(
    BACNET_CONFIRMED_SERVICE service_choice, error_function pFunction);

BACNET_STACK_EXPORT
void apdu_set_complex_error_handler(
    BACNET_CONFIRMED_SERVICE service_choice, complex_error_function pFunction);

BACNET_STACK_EXPORT
void apdu_set_abort_handler(abort_function pFunction);

BACNET_STACK_EXPORT
void apdu_set_reject_handler(reject_function pFunction);

BACNET_STACK_EXPORT
uint16_t apdu_decode_confirmed_service_request(
    uint8_t *apdu, /* APDU data */
    uint16_t apdu_len,
    BACNET_CONFIRMED_SERVICE_DATA *service_data,
    uint8_t *service_choice,
    uint8_t **service_request,
    uint16_t *service_request_len);

BACNET_STACK_EXPORT
uint16_t apdu_timeout(void);
BACNET_STACK_EXPORT
void apdu_timeout_set(uint16_t value);
BACNET_STACK_EXPORT
uint8_t apdu_retries(void);
BACNET_STACK_EXPORT
void apdu_retries_set(uint8_t value);

BACNET_STACK_EXPORT
void apdu_handler(
    BACNET_ADDRESS *src, /* source address */
    uint8_t *apdu, /* APDU data */
    uint16_t pdu_len); /* for confirmed messages */

#if BACNET_SEGMENTATION_ENABLED
BACNET_STACK_EXPORT
uint16_t apdu_segment_timeout(void);

BACNET_STACK_EXPORT
void apdu_segment_timeout_set(uint16_t milliseconds);

BACNET_STACK_EXPORT
int apdu_encode_fixed_header(
    uint8_t *apdu, BACNET_APDU_FIXED_HEADER *fixed_pdu_header);

BACNET_STACK_EXPORT
void apdu_init_fixed_header(
    BACNET_APDU_FIXED_HEADER *fixed_pdu_header,
    uint8_t pdu_type,
    uint8_t invoke_id,
    uint8_t service,
    int max_apdu);

BACNET_STACK_EXPORT
void apdu_max_segments_accepted_set(uint8_t maxSegments);

BACNET_STACK_EXPORT
uint8_t apdu_max_segments_accepted_get(void);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
