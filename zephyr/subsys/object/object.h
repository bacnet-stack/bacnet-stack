/*
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ZTEST_UNITTEST
  #include <zephyr/kernel.h>
#else
  #include "stdlib.h"
  #define k_malloc(a) malloc(a)
  #define k_free(a) free(a)

  #define _DO_CONCAT(x, y) x ## y
  #define _CONCAT(x, y) _DO_CONCAT(x, y)

#endif
#include "bacnet/basic/sys/keylist.h"


#define OBJECT_ENUM_FUNCTIONS(object_name, descr_type)                      \
    descr_type *_CONCAT(object_name, _Find_Description)(uint32_t instance)  \
        { return Keylist_Data(Object_List, instance); }                     \
    unsigned _CONCAT(object_name, _Count)(void)                             \
        { return Keylist_Count(Object_List); }                              \
    uint32_t _CONCAT(object_name, _Index_To_Instance)(unsigned index)       \
        { return Keylist_Key(Object_List, index); }                         \
    unsigned _CONCAT(object_name, _Instance_To_Index)(uint32_t instance)    \
        { return (unsigned)Keylist_Index(Object_List, instance); }


#define OBJECT_MEMORY_FUNCTIONS(object_name, descr_type)                    \
    bool _CONCAT(object_name, _Create)(uint32_t instance)                   \
    {                                                                       \
        descr_type *descr = Keylist_Data(Object_List, instance);            \
        if (!descr) {                                                       \
            descr = k_malloc(sizeof(descr_type));                           \
            if (descr) {                                                    \
                _CONCAT(object_name, _Init_Description)(descr, instance);   \
                return Keylist_Data_Add(Object_List, instance, descr) >= 0; \
            }                                                               \
        }                                                                   \
        return false;                                                       \
    }                                                                       \
    void _CONCAT(object_name, _Cleanup)(void)                               \
    {                                                                       \
        descr_type *descr;                                                  \
        if (Object_List) {                                                  \
            do {                                                            \
                descr = Keylist_Data_Pop(Object_List);                      \
                if (descr) {                                                \
                    k_free(descr);                                          \
                }                                                           \
            } while (descr);                                                \
            Keylist_Delete(Object_List);                                    \
            Object_List = NULL;                                             \
        }                                                                   \
    }                                                                       \
    bool _CONCAT(object_name, _Delete)(uint32_t instance)                   \
    {                                                                       \
        bool status = false;                                                \
        descr_type *descr = Keylist_Data_Delete(Object_List, instance);     \
        if (descr) {                                                        \
            k_free(descr);                                                  \
            status = true;                                                  \
        }                                                                   \
        return status;                                                      \
    }

#define OBJECT_FUNCTIONS_WITHOUT_INIT(object_name, descr_type)  \
    static OS_Keylist Object_List = NULL;                       \
    OBJECT_ENUM_FUNCTIONS(object_name, descr_type)              \
    OBJECT_MEMORY_FUNCTIONS(object_name, descr_type)

#define OBJECT_FUNCTIONS(object_name, descr_type)               \
    OBJECT_FUNCTIONS_WITHOUT_INIT(object_name, descr_type)      \
    void _CONCAT(object_name, _Init)(void){                     \
        if (!Object_List) {                                     \
            Object_List = Keylist_Create();                     \
        }                                                       \
    }
