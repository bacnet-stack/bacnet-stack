/**************************************************************************
*
* Copyright (C) 2015 Nikola Jelic <nikola.jelic@euroicc.com>
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
#ifndef ACCESS_USER_H
#define ACCESS_USER_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacerror.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"


#ifndef MAX_ACCESS_USERS
#define MAX_ACCESS_USERS 4
#endif

#ifndef MAX_ACCESS_USER_CREDENTIALS_COUNT
#define MAX_ACCESS_USER_CREDENTIALS_COUNT 4
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct {
        uint32_t global_identifier;
        BACNET_RELIABILITY reliability;
        BACNET_ACCESS_USER_TYPE user_type;
        uint32_t credentials_count;
        BACNET_DEVICE_OBJECT_REFERENCE
            credentials[MAX_ACCESS_USER_CREDENTIALS_COUNT];
    } ACCESS_USER_DESCR;

    BACNET_STACK_EXPORT
    void Access_User_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    BACNET_STACK_EXPORT
    bool Access_User_Valid_Instance(
        uint32_t object_instance);
    unsigned Access_User_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Access_User_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Access_User_Instance_To_Index(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Access_User_Object_Instance_Add(
        uint32_t instance);

    BACNET_STACK_EXPORT
    bool Access_User_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Access_User_Name_Set(
        uint32_t object_instance,
        char *new_name);

    BACNET_STACK_EXPORT
    int Access_User_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    BACNET_STACK_EXPORT
    bool Access_User_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_STACK_EXPORT
    bool Access_User_Create(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Access_User_Delete(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Access_User_Cleanup(
        void);
    BACNET_STACK_EXPORT
    void Access_User_Init(
        void);

#ifdef BAC_TEST
#include "ctest.h"
    BACNET_STACK_EXPORT
    void testAccessUser(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
