/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2013
 * @brief The Channel object is a command object without a priority array,
 * and the present-value property proxies an ANY data type (sort of)
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_CHANNEL_H
#define BACNET_BASIC_OBJECT_CHANNEL_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/object/lo.h"

/* BACNET_CHANNEL_VALUE decodes WriteProperty service requests
   Choose the datatypes that your application supports */
#if !(defined(CHANNEL_NUMERIC) || defined(CHANNEL_NULL) ||              \
    defined(CHANNEL_BOOLEAN) || defined(CHANNEL_UNSIGNED) ||            \
    defined(CHANNEL_SIGNED) || defined(CHANNEL_REAL) ||                 \
    defined(CHANNEL_DOUBLE) || defined(CHANNEL_OCTET_STRING) ||         \
    defined(CHANNEL_CHARACTER_STRING) || defined(CHANNEL_BIT_STRING) || \
    defined(CHANNEL_ENUMERATED) || defined(CHANNEL_DATE) ||             \
    defined(CHANNEL_TIME) || defined(CHANNEL_OBJECT_ID) ||              \
    defined(CHANNEL_LIGHTING_COMMAND) || defined(CHANNEL_XY_COLOR) ||   \
    defined(CHANNEL_COLOR_COMMAND))
#define CHANNEL_NUMERIC
#endif

#if defined(CHANNEL_NUMERIC)
#define CHANNEL_NULL
#define CHANNEL_BOOLEAN
#define CHANNEL_UNSIGNED
#define CHANNEL_SIGNED
#define CHANNEL_REAL
#define CHANNEL_DOUBLE
#define CHANNEL_ENUMERATED
#define CHANNEL_LIGHTING_COMMAND
#define CHANNEL_COLOR_COMMAND
#define CHANNEL_XY_COLOR
#endif

typedef struct BACnet_Channel_Value_t {
    uint8_t tag;
    union {
        /* NULL - not needed as it is encoded in the tag alone */
#if defined(CHANNEL_BOOLEAN)
        bool Boolean;
#endif
#if defined(CHANNEL_UNSIGNED)
        uint32_t Unsigned_Int;
#endif
#if defined(CHANNEL_SIGNED)
        int32_t Signed_Int;
#endif
#if defined(CHANNEL_REAL)
        float Real;
#endif
#if defined(CHANNEL_DOUBLE)
        double Double;
#endif
#if defined(CHANNEL_OCTET_STRING)
        BACNET_OCTET_STRING Octet_String;
#endif
#if defined(CHANNEL_CHARACTER_STRING)
        BACNET_CHARACTER_STRING Character_String;
#endif
#if defined(CHANNEL_BIT_STRING)
        BACNET_BIT_STRING Bit_String;
#endif
#if defined(CHANNEL_ENUMERATED)
        uint32_t Enumerated;
#endif
#if defined(CHANNEL_DATE)
        BACNET_DATE Date;
#endif
#if defined(CHANNEL_TIME)
        BACNET_TIME Time;
#endif
#if defined(CHANNEL_OBJECT_ID)
        BACNET_OBJECT_ID Object_Id;
#endif
#if defined(CHANNEL_LIGHTING_COMMAND)
        BACNET_LIGHTING_COMMAND Lighting_Command;
#endif
#if defined(CHANNEL_COLOR_COMMAND)
        BACNET_COLOR_COMMAND Color_Command;
#endif
#if defined(CHANNEL_XY_COLOR)
        BACNET_XY_COLOR XY_Color;
#endif
    } type;
    /* simple linked list if needed */
    struct BACnet_Channel_Value_t *next;
} BACNET_CHANNEL_VALUE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
BACNET_STACK_EXPORT
void Channel_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);
BACNET_STACK_EXPORT
bool Channel_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Channel_Count(void);
BACNET_STACK_EXPORT
uint32_t Channel_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Channel_Instance_To_Index(uint32_t instance);
BACNET_STACK_EXPORT
bool Channel_Object_Instance_Add(uint32_t instance);

BACNET_STACK_EXPORT
bool Channel_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Channel_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Channel_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
int Channel_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Channel_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
BACNET_CHANNEL_VALUE *Channel_Present_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Channel_Present_Value_Set(
    BACNET_WRITE_PROPERTY_DATA *wp_data, const BACNET_APPLICATION_DATA_VALUE *value);

BACNET_STACK_EXPORT
bool Channel_Out_Of_Service(uint32_t object_instance);
BACNET_STACK_EXPORT
void Channel_Out_Of_Service_Set(uint32_t object_instance, bool oos_flag);

BACNET_STACK_EXPORT
unsigned Channel_Last_Priority(uint32_t object_instance);
BACNET_STACK_EXPORT
BACNET_WRITE_STATUS Channel_Write_Status(uint32_t object_instance);
uint16_t Channel_Number(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Channel_Number_Set(uint32_t object_instance, uint16_t value);

BACNET_STACK_EXPORT
unsigned Channel_Reference_List_Member_Count(uint32_t object_instance);
BACNET_STACK_EXPORT
BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *Channel_Reference_List_Member_Element(
    uint32_t object_instance, unsigned element);
BACNET_STACK_EXPORT
bool Channel_Reference_List_Member_Element_Set(uint32_t object_instance,
    unsigned array_index,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMemberSrc);
BACNET_STACK_EXPORT
unsigned Channel_Reference_List_Member_Element_Add(uint32_t object_instance,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMemberSrc);
BACNET_STACK_EXPORT
uint16_t Channel_Control_Groups_Element(
    uint32_t object_instance, int32_t array_index);
BACNET_STACK_EXPORT
bool Channel_Control_Groups_Element_Set(
    uint32_t object_instance, int32_t array_index, uint16_t value);
BACNET_STACK_EXPORT
bool Channel_Value_Copy(
    BACNET_CHANNEL_VALUE *cvalue, const BACNET_APPLICATION_DATA_VALUE *value);
BACNET_STACK_EXPORT
int Channel_Value_Encode(
    uint8_t *apdu, int apdu_max, const BACNET_CHANNEL_VALUE *value);
BACNET_STACK_EXPORT
int Channel_Coerce_Data_Encode(uint8_t *apdu,
    size_t apdu_size,
    const BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_APPLICATION_TAG tag);
BACNET_STACK_EXPORT
bool Channel_Write_Member_Value(
    BACNET_WRITE_PROPERTY_DATA *wp_data, const BACNET_APPLICATION_DATA_VALUE *value);

BACNET_STACK_EXPORT
void Channel_Write_Property_Internal_Callback_Set(
    write_property_function cb);

BACNET_STACK_EXPORT
uint32_t Channel_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Channel_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Channel_Cleanup(void);
BACNET_STACK_EXPORT
void Channel_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
