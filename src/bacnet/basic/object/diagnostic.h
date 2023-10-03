/**
 * @file
 * @author Edward Hague
 * @date 2023
 * @brief Diagnostic Object
 *
 * @section DESCRIPTION
 *
 * yada
 *
 * @section LICENSE
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
 */

#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/apdu.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void Diagnostic_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    BACNET_STACK_EXPORT
    void Diagnostic_Property_List(
        uint32_t object_instance,
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    BACNET_STACK_EXPORT
    bool Diagnostic_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Diagnostic_Name_Set(
        uint32_t object_instance,
        char *new_name);

    BACNET_STACK_EXPORT
    char *Diagnostic_Description(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Diagnostic_Description_Set(
        uint32_t instance,
        char *new_name);

    BACNET_STACK_EXPORT
    BACNET_RELIABILITY Diagnostic_Reliability(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Diagnostic_Reliability_Set(
        uint32_t object_instance,
        BACNET_RELIABILITY value);

    BACNET_STACK_EXPORT
    bool Diagnostic_Out_Of_Service(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Diagnostic_Out_Of_Service_Set(
        uint32_t instance,
        bool oos_flag);

    BACNET_STACK_EXPORT
    bool Diagnostic_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Diagnostic_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Diagnostic_Index_To_Instance(
        unsigned find_index);
    BACNET_STACK_EXPORT
    unsigned Diagnostic_Instance_To_Index(
        uint32_t object_instance);

    BACNET_STACK_EXPORT
    bool Diagnostic_Read_Range(
        BACNET_READ_RANGE_DATA * pRequest,
        RR_PROP_INFO * pInfo);

    BACNET_STACK_EXPORT
    void Diagnostic_Init(
        void);

    /* handling for read property service */
    BACNET_STACK_EXPORT
    int Diagnostic_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);

    /* handling for write property service */
    BACNET_STACK_EXPORT
    bool Diagnostic_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
