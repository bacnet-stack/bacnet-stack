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
#ifndef CREDENTIAL_DATA_INPUT_H
#define CREDENTIAL_DATA_INPUT_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacerror.h"
#include "timestamp.h"
#include "bacdevobjpropref.h"
#include "authentication_factor.h"
#include "authentication_factor_format.h"
#include "timestamp.h"
#include "rp.h"
#include "wp.h"


#ifndef MAX_CREDENTIAL_DATA_INPUTS
#define MAX_CREDENTIAL_DATA_INPUTS 4
#endif

#ifndef MAX_AUTHENTICATION_FACTOR_FORMAT_COUNT
#define MAX_AUTHENTICATION_FACTOR_FORMAT_COUNT 4
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct {
        BACNET_AUTHENTICATION_FACTOR present_value;
        BACNET_RELIABILITY reliability;
        bool out_of_service;
        uint32_t supported_formats_count;
        BACNET_AUTHENTICATION_FACTOR_FORMAT
            supported_formats[MAX_AUTHENTICATION_FACTOR_FORMAT_COUNT];
        BACNET_TIMESTAMP timestamp;
    } CREDENTIAL_DATA_INPUT_DESCR;

    void Credential_Data_Input_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    bool Credential_Data_Input_Valid_Instance(
        uint32_t object_instance);
    unsigned Credential_Data_Input_Count(
        void);
    uint32_t Credential_Data_Input_Index_To_Instance(
        unsigned index);
    unsigned Credential_Data_Input_Instance_To_Index(
        uint32_t instance);
    bool Credential_Data_Input_Object_Instance_Add(
        uint32_t instance);


    bool Credential_Data_Input_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    bool Credential_Data_Input_Name_Set(
        uint32_t object_instance,
        char *new_name);


    bool Credential_Data_Input_Out_Of_Service(
        uint32_t instance);
    void Credential_Data_Input_Out_Of_Service_Set(
        uint32_t instance,
        bool oos_flag);

    int Credential_Data_Input_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    bool Credential_Data_Input_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    bool Credential_Data_Input_Create(
        uint32_t object_instance);
    bool Credential_Data_Input_Delete(
        uint32_t object_instance);
    void Credential_Data_Input_Cleanup(
        void);
    void Credential_Data_Input_Init(
        void);

#ifdef TEST
#include "ctest.h"
    void testCredentialDataInput(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
