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
#ifndef ACCESS_CREDENTIAL_H
#define ACCESS_CREDENTIAL_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacerror.h"
#include "bacnet/datetime.h"
#include "bacnet/timestamp.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/assigned_access_rights.h"
#include "bacnet/credential_authentication_factor.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"


#ifndef MAX_ACCESS_CREDENTIALS
#define MAX_ACCESS_CREDENTIALS 4
#endif

#ifndef MAX_REASONS_FOR_DISABLE
#define MAX_REASONS_FOR_DISABLE 4
#endif

#ifndef MAX_AUTHENTICATION_FACTORS
#define MAX_AUTHENTICATION_FACTORS 4
#endif

#ifndef MAX_ASSIGNED_ACCESS_RIGHTS
#define MAX_ASSIGNED_ACCESS_RIGHTS 4
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct {
        uint32_t global_identifier;
        BACNET_RELIABILITY reliability;
        bool credential_status;
        uint32_t reasons_count;
        BACNET_ACCESS_CREDENTIAL_DISABLE_REASON
            reason_for_disable[MAX_REASONS_FOR_DISABLE];
        uint32_t auth_factors_count;
        BACNET_CREDENTIAL_AUTHENTICATION_FACTOR
            auth_factors[MAX_AUTHENTICATION_FACTORS];
        BACNET_DATE_TIME activation_time, expiration_time;
        BACNET_ACCESS_CREDENTIAL_DISABLE credential_disable;
        uint32_t assigned_access_rights_count;
        BACNET_ASSIGNED_ACCESS_RIGHTS
            assigned_access_rights[MAX_ASSIGNED_ACCESS_RIGHTS];
    } ACCESS_CREDENTIAL_DESCR;

    BACNET_STACK_EXPORT
    void Access_Credential_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    BACNET_STACK_EXPORT
    bool Access_Credential_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Access_Credential_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Access_Credential_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Access_Credential_Instance_To_Index(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Access_Credential_Object_Instance_Add(
        uint32_t instance);


    BACNET_STACK_EXPORT
    bool Access_Credential_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Access_Credential_Name_Set(
        uint32_t object_instance,
        char *new_name);

    BACNET_STACK_EXPORT
    int Access_Credential_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    BACNET_STACK_EXPORT
    bool Access_Credential_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_STACK_EXPORT
    bool Access_Credential_Create(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Access_Credential_Delete(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Access_Credential_Cleanup(
        void);
    BACNET_STACK_EXPORT
    void Access_Credential_Init(
        void);

#ifdef TEST
#include "ctest.h"
    BACNET_STACK_EXPORT
    void testAccessCredential(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
