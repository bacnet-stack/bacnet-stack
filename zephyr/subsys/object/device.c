/*
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/** @file device.c Zephyr specific part of the Base "class". */

#ifdef CONFIG_BACNET_USE_SECTION_ITERABLE_OBJECT_TABLE
#include <zephyr.h>
#endif
#include "bacnet/basic/object/device.h"
#include "object.h"

#ifdef CONFIG_BACNET_USE_SECTION_ITERABLE_OBJECT_TABLE
  extern struct object_functions _object_functions_list_end[];
#endif

#if BAC_ROUTING
static object_functions Routing_object = {
    .Object_Type = OBJECT_DEVICE,
    .Object_Init = NULL,
    .Object_Count = Device_Count,
    .Object_Index_To_Instance = Routed_Device_Index_To_Instance,
    .Object_Valid_Instance = Routed_Device_Valid_Object_Instance_Number,
    .Object_Name = Routed_Device_Name,
    .Object_Read_Property = Routed_Device_Read_Property_Local,
    .Object_Write_Property = Routed_Device_Write_Property_Local,
    .Object_RPM_List = Device_Property_Lists,
    .Object_RR_Info = DeviceGetRRInfo
    .Object_Iterator =  NULL,
    .Object_Value_List =  NULL,
    .Object_COV =  NULL,
    .Object_COV_Clear =  NULL,
    .Object_Intrinsic_Reporting =  NULL,
};
static bool routing_Device = false;

/* In Zephyr port the object_functions table is saved in ROM and 
   can't change fields value.
   Instead this Device_Objects_Get_First(Next)_Object() returns the "Routing"
   object when asked "Device" object, see static filter functions. */
void Routing_Device_Init(uint32_t first_object_instance)
{
    /* Initialize with our preset strings */
    Add_Routed_Device(first_object_instance, &My_Object_Name, Description);

    routing_Device = true;
}

#endif /* BAC_ROUTING */

#ifdef CONFIG_BACNET_USE_SECTION_ITERABLE_OBJECT_TABLE
static struct object_functions *Device_Object_Filter_Out(
    struct object_functions *pObject)
{
#if BAC_ROUTING
    if (routing_Device && pObject == &Device_object)
        return &Routing_object;
    else
#endif
    return pObject;
}

static struct object_functions *Device_Object_Filter_In(
    struct object_functions *pObject)
{
#if BAC_ROUTING
    if (routing_Device && pObject == &Routing_object)
        return &Device_object;
    else
#endif
    return pObject;
}

struct object_functions *Device_Objects_Get_First_Object(void)
{
    STRUCT_SECTION_FOREACH(object_functions, pObject) {
        return Device_Object_Filter_Out(pObject);
    }
    return NULL;
}

struct object_functions *Device_Objects_Get_Next_Object(
    struct object_functions *object)
{
    if (object == NULL)
        return NULL;
    
    object = Device_Object_Filter_In(object);
    ++object;

    if (object < _object_functions_list_end) {
        return Device_Object_Filter_Out(object);
    }
    return NULL;
}
#endif /* CONFIG_BACNET_USE_SECTION_ITERABLE_OBJECT_TABLE */

/**
 * Allocate a Bacnet object
 */
void* Bacnet_Object_Allocate(size_t size)
{
    return k_malloc(size);
}

/**
 * Free a Bacnet object
 */
void Bacnet_Object_Free(void *descr)
{
    k_free(descr);
}
