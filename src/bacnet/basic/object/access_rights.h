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
#ifndef ACCESS_RIGHTS_H
#define ACCESS_RIGHTS_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacerror.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/access_rule.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"


#ifndef MAX_ACCESS_RIGHTSS
#define MAX_ACCESS_RIGHTSS 4
#endif

#ifndef MAX_NEGATIVE_ACCESS_RIGHTS_RULES
#define MAX_NEGATIVE_ACCESS_RIGHTS_RULES 4
#endif

#ifndef MAX_POSITIVE_ACCESS_RIGHTS_RULES
#define MAX_POSITIVE_ACCESS_RIGHTS_RULES 4
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct {
        uint32_t global_identifier;
        BACNET_RELIABILITY reliability;
        bool enable;
        uint32_t negative_access_rules_count, positive_access_rules_count;
                 BACNET_ACCESS_RULE
            negative_access_rules[MAX_NEGATIVE_ACCESS_RIGHTS_RULES];
                 BACNET_ACCESS_RULE
            positive_access_rules[MAX_POSITIVE_ACCESS_RIGHTS_RULES];
    } ACCESS_RIGHTS_DESCR;

    BACNET_STACK_EXPORT
    void Access_Rights_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    BACNET_STACK_EXPORT
    bool Access_Rights_Valid_Instance(
        uint32_t object_instance);
    unsigned Access_Rights_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Access_Rights_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Access_Rights_Instance_To_Index(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Access_Rights_Object_Instance_Add(
        uint32_t instance);

    BACNET_STACK_EXPORT
    bool Access_Rights_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Access_Rights_Name_Set(
        uint32_t object_instance,
        char *new_name);

    BACNET_STACK_EXPORT
    int Access_Rights_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    BACNET_STACK_EXPORT
    bool Access_Rights_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_STACK_EXPORT
    bool Access_Rights_Create(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Access_Rights_Delete(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Access_Rights_Cleanup(
        void);
    BACNET_STACK_EXPORT
    void Access_Rights_Init(
        void);

#ifdef BAC_TEST
#include "ctest.h"
    BACNET_STACK_EXPORT
    void testAccessRights(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
