/**
 * @file
 * @author Krzysztof Malorny <malornykrzysztof@gmail.com>
 * @date 2011
 * @brief API for a basic BACnet Notification Class object
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_NOTIFICATION_CLASS_H
#define BACNET_BASIC_OBJECT_NOTIFICATION_CLASS_H
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdest.h"
#include "bacnet/event.h"
#include "bacnet/list_element.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define NC_RESCAN_RECIPIENTS_SECS 60

/* max "length" of recipient_list */
#define NC_MAX_RECIPIENTS 10

#if defined(INTRINSIC_REPORTING)

/* Structure containing configuration for a Notification Class */
typedef struct Notification_Class_info {
    uint8_t
        Priority[MAX_BACNET_EVENT_TRANSITION]; /* BACnetARRAY[3] of Unsigned */
    uint8_t Ack_Required; /* BACnetEventTransitionBits */
    BACNET_DESTINATION
    Recipient_List[NC_MAX_RECIPIENTS]; /* List of BACnetDestination */
} NOTIFICATION_CLASS_INFO;

/* Indicates whether the transaction has been confirmed */
typedef struct Acked_info {
    bool bIsAcked; /* true when transitions is acked */
    BACNET_DATE_TIME Time_Stamp; /* time stamp of when a alarm was generated */
} ACKED_INFO;

/* Information needed to send AckNotification */
typedef struct Ack_Notification {
    bool bSendAckNotify; /* true if need to send AckNotification */
    uint8_t EventState;
} ACK_NOTIFICATION;

BACNET_STACK_EXPORT
void Notification_Class_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);

BACNET_STACK_EXPORT
void Notification_Class_Init(void);

bool Notification_Class_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Notification_Class_Count(void);
BACNET_STACK_EXPORT
uint32_t Notification_Class_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Notification_Class_Instance_To_Index(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Notification_Class_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);

BACNET_STACK_EXPORT
int Notification_Class_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool Notification_Class_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
int Notification_Class_Add_List_Element(BACNET_LIST_ELEMENT_DATA *list_element);

BACNET_STACK_EXPORT
int Notification_Class_Remove_List_Element(
    BACNET_LIST_ELEMENT_DATA *list_element);

BACNET_STACK_EXPORT
void Notification_Class_Get_Priorities(
    uint32_t Object_Instance, uint32_t *pPriorityArray);

BACNET_STACK_EXPORT
void Notification_Class_Set_Priorities(
    uint32_t Object_Instance, uint32_t *pPriorityArray);

BACNET_STACK_EXPORT
void Notification_Class_Get_Ack_Required(
    uint32_t Object_Instance, uint8_t *pAckRequired);

BACNET_STACK_EXPORT
void Notification_Class_Set_Ack_Required(
    uint32_t Object_Instance, uint8_t Ack_Required);

BACNET_STACK_EXPORT
bool Notification_Class_Get_Recipient_List(
    uint32_t Object_Instance, BACNET_DESTINATION *pRecipientList);

BACNET_STACK_EXPORT
bool Notification_Class_Set_Recipient_List(
    uint32_t Object_Instance, BACNET_DESTINATION *pRecipientList);

BACNET_STACK_EXPORT
void Notification_Class_common_reporting_function(
    BACNET_EVENT_NOTIFICATION_DATA *event_data);

BACNET_STACK_EXPORT
void Notification_Class_find_recipient(void);
#endif /* defined(INTRINSIC_REPORTING) */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* NC_H */
