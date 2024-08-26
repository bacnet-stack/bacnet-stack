/**
 * @file
 * @brief Accumulator object header file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2017
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_ACCUMULATOR_H
#define BACNET_BASIC_OBJECT_ACCUMULATOR_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacint.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    BACNET_STACK_EXPORT
    void Accumulator_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    BACNET_STACK_EXPORT
    bool Accumulator_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Accumulator_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Accumulator_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Accumulator_Instance_To_Index(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Accumulator_Object_Instance_Add(
        uint32_t instance);

    BACNET_STACK_EXPORT
    char *Accumulator_Name(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Accumulator_Name_Set(
        uint32_t object_instance,
        char *new_name);

    BACNET_STACK_EXPORT
    char *Accumulator_Description(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Accumulator_Description_Set(
        uint32_t instance,
        char *new_name);

    BACNET_STACK_EXPORT
    bool Accumulator_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);


    BACNET_STACK_EXPORT
    bool Accumulator_Units_Set(
        uint32_t instance,
        uint16_t units);
    BACNET_STACK_EXPORT
    uint16_t Accumulator_Units(
        uint32_t instance);

    BACNET_STACK_EXPORT
    int Accumulator_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    BACNET_STACK_EXPORT
    bool Accumulator_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_STACK_EXPORT
    BACNET_UNSIGNED_INTEGER Accumulator_Present_Value(uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Accumulator_Present_Value_Set(
        uint32_t object_instance,
        BACNET_UNSIGNED_INTEGER value);

    BACNET_STACK_EXPORT
    BACNET_UNSIGNED_INTEGER Accumulator_Max_Pres_Value(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Accumulator_Max_Pres_Value_Set(
        uint32_t object_instance,
        BACNET_UNSIGNED_INTEGER value);

    BACNET_STACK_EXPORT
    int32_t Accumulator_Scale_Integer(uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Accumulator_Scale_Integer_Set(uint32_t object_instance, int32_t);

    BACNET_STACK_EXPORT
    void Accumulator_Init(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#define ACCUMULATOR_OBJ_FUNCTIONS \
    OBJECT_ACCUMULATOR, Accumulator_Init, Accumulator_Count, \
    Accumulator_Index_To_Instance, Accumulator_Valid_Instance, \
    Accumulator_Name, Accumulator_Read_Property, Accumulator_Write_Property, \
    Accumulator_Property_Lists, NULL, NULL
#endif
