/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

/** @file objects.h Defines helper for create object_function table enuminator.
  */

#ifndef OBJECTS_H
#define OBJECTS_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/ai.h"

#define BACNET_OBJECT_TABLE(table_name, _type, _init, _count, \
                            _index_to_instance, _valid_instance, _object_name, \
                            _read_property, _write_property, _RPM_list, \
                            _RR_info, _iterator, _value_list, _COV, \
                            _COV_clear, _intrinsic_reporting) \
STRUCT_SECTION_ITERABLE(object_functions, table_name) = {   \
    .Object_Type = _type,                                   \
    .Object_Init = _init,                                   \
    .Object_Count = _count,                                 \
    .Object_Index_To_Instance = _index_to_instance,         \
    .Object_Valid_Instance = _valid_instance,               \
    .Object_Name = _object_name,                            \
    .Object_Read_Property = _read_property,                 \
    .Object_Write_Property = _write_property,               \
    .Object_RPM_List = _RPM_list,                           \
    .Object_RR_Info = _RR_info,                             \
    .Object_Iterator = _iterator,                           \
    .Object_Value_List = _value_list,                       \
    .Object_COV = _COV,                                     \
    .Object_COV_Clear = _COV_clear,                         \
    .Object_Intrinsic_Reporting = _intrinsic_reporting      \
}

#define AI_DESCR_STRUCT(table_name, _instance)              \
STRUCT_SECTION_ITERABLE(analog_input_descr, table_name) = { \
    .instance = _instance                                   \
}



#endif /* OBJECTS_H */
