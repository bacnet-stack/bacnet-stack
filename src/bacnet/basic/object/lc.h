/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @brief The Load Control Objects from 135-2004-Addendum e 
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_LOAD_CONTROL_H
#define BACNET_BASIC_OBJECT_LOAD_CONTROL_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

typedef enum bacnet_load_control_state {
    SHED_INACTIVE,
    SHED_REQUEST_PENDING,
    SHED_NON_COMPLIANT,
    SHED_COMPLIANT,
    MAX_LOAD_CONTROL_STATE
} BACNET_LOAD_CONTROL_STATE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void Load_Control_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    BACNET_STACK_EXPORT
    void Load_Control_State_Machine_Handler(
        void);

    BACNET_STACK_EXPORT
    bool Load_Control_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Load_Control_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Load_Control_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Load_Control_Instance_To_Index(
        uint32_t object_instance);

    BACNET_STACK_EXPORT
    bool Load_Control_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);

    BACNET_STACK_EXPORT
    void Load_Control_Init(
        void);

    BACNET_STACK_EXPORT
    unsigned Load_Control_Priority_For_Writing(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Load_Control_Priority_For_Writing_Set(
        uint32_t object_instance, unsigned priority);
    BACNET_STACK_EXPORT 
    bool Load_Control_Manipulated_Variable_Reference(
        uint32_t object_instance,
        BACNET_OBJECT_PROPERTY_REFERENCE * object_property_reference);
    BACNET_STACK_EXPORT 
    bool Load_Control_Manipulated_Variable_Reference_Set(
        uint32_t object_instance,
        BACNET_OBJECT_PROPERTY_REFERENCE * object_property_reference);

    BACNET_STACK_EXPORT
    int Load_Control_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);

    BACNET_STACK_EXPORT
    bool Load_Control_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    /* functions used for unit testing */
    BACNET_STACK_EXPORT
    void Load_Control_State_Machine(
        int object_index, 
        BACNET_DATE_TIME *bdatetime);
    BACNET_STACK_EXPORT
    BACNET_LOAD_CONTROL_STATE Load_Control_State(
        int object_index);
    BACNET_STACK_EXPORT
    BACNET_OBJECT_ID Load_Control_Object_ID(
        int object_index);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
