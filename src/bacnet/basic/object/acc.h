#ifndef ACC_H
#define ACC_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacint.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    void Accumulator_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    bool Accumulator_Valid_Instance(
        uint32_t object_instance);
    unsigned Accumulator_Count(
        void);
    uint32_t Accumulator_Index_To_Instance(
        unsigned index);
    unsigned Accumulator_Instance_To_Index(
        uint32_t instance);
    bool Accumulator_Object_Instance_Add(
        uint32_t instance);

    char *Accumulator_Name(
        uint32_t object_instance);
    bool Accumulator_Name_Set(
        uint32_t object_instance,
        char *new_name);

    char *Accumulator_Description(
        uint32_t instance);
    bool Accumulator_Description_Set(
        uint32_t instance,
        char *new_name);

    bool Accumulator_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);


    bool Accumulator_Units_Set(
        uint32_t instance,
        uint16_t units);
    uint16_t Accumulator_Units(
        uint32_t instance);

    int Accumulator_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    bool Accumulator_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

	BACNET_UNSIGNED_INTEGER Accumulator_Present_Value(uint32_t object_instance);
    bool Accumulator_Present_Value_Set(
        uint32_t object_instance,
        BACNET_UNSIGNED_INTEGER value);

    BACNET_UNSIGNED_INTEGER Accumulator_Max_Pres_Value(
        uint32_t object_instance);
    bool Accumulator_Max_Pres_Value_Set(
        uint32_t object_instance,
        BACNET_UNSIGNED_INTEGER value);

    int32_t Accumulator_Scale_Integer(uint32_t object_instance);
    bool Accumulator_Scale_Integer_Set(uint32_t object_instance, int32_t);

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



