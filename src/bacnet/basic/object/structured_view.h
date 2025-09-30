/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2024
 * @brief API for a Structured View object used by a BACnet device object
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_STRUCTURED_VIEW_OBJECT_H
#define BACNET_BASIC_STRUCTURED_VIEW_OBJECT_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/rp.h"

struct BACnetSubordinateData;
typedef struct BACnetSubordinateData {
    uint32_t Device_Instance;
    BACNET_OBJECT_TYPE Object_Type;
    uint32_t Object_Instance;
    const char *Annotations;
    BACNET_NODE_TYPE Node_Type;
    BACNET_RELATIONSHIP Relationship;
    /* simple linked list */
    struct BACnetSubordinateData *next;
} BACNET_SUBORDINATE_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Structured_View_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);
BACNET_STACK_EXPORT
bool Structured_View_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Structured_View_Count(void);
BACNET_STACK_EXPORT
uint32_t Structured_View_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Structured_View_Instance_To_Index(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Structured_View_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Structured_View_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Structured_View_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
int Structured_View_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
const char *Structured_View_Description(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Structured_View_Description_Set(
    uint32_t object_instance, const char *new_name);

BACNET_STACK_EXPORT
BACNET_NODE_TYPE Structured_View_Node_Type(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Structured_View_Node_Type_Set(
    uint32_t object_instance, BACNET_NODE_TYPE node_type);

BACNET_STACK_EXPORT
const char *Structured_View_Node_Subtype(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Structured_View_Node_Subtype_Set(
    uint32_t object_instance, const char *new_name);

BACNET_STACK_EXPORT
BACNET_SUBORDINATE_DATA *
Structured_View_Subordinate_List(uint32_t object_instance);
BACNET_STACK_EXPORT
void Structured_View_Subordinate_List_Set(
    uint32_t object_instance, BACNET_SUBORDINATE_DATA *subordinate_list);
BACNET_STACK_EXPORT
BACNET_SUBORDINATE_DATA *Structured_View_Subordinate_List_Member(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index);
BACNET_STACK_EXPORT
unsigned int Structured_View_Subordinate_List_Count(uint32_t object_instance);

BACNET_STACK_EXPORT
BACNET_RELATIONSHIP
Structured_View_Default_Subordinate_Relationship(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Structured_View_Default_Subordinate_Relationship_Set(
    uint32_t object_instance, BACNET_RELATIONSHIP relationship);

BACNET_STACK_EXPORT
int Structured_View_Subordinate_List_Element_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu);
BACNET_STACK_EXPORT
int Structured_View_Subordinate_Annotations_Element_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu);
BACNET_STACK_EXPORT
int Structured_View_Subordinate_Node_Types_Element_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu);
BACNET_STACK_EXPORT
int Structured_View_Subordinate_Relationships_Element_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu);

BACNET_STACK_EXPORT
BACNET_DEVICE_OBJECT_REFERENCE *
Structured_View_Represents(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Structured_View_Represents_Set(
    uint32_t object_instance, const BACNET_DEVICE_OBJECT_REFERENCE *represents);

BACNET_STACK_EXPORT
void *Structured_View_Context_Get(uint32_t object_instance);
BACNET_STACK_EXPORT
void Structured_View_Context_Set(uint32_t object_instance, void *context);

BACNET_STACK_EXPORT
uint32_t Structured_View_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Structured_View_Delete(uint32_t object_instance);

BACNET_STACK_EXPORT
void Structured_View_Cleanup(void);
BACNET_STACK_EXPORT
void Structured_View_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
