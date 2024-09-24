/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef BV_H
#define BV_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacerror.h"
#include "bacnet/wp.h"

#ifndef MAX_BINARY_VALUES
#define MAX_BINARY_VALUES 10
#endif


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void Binary_Value_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    bool Binary_Value_Valid_Instance(
        uint32_t object_instance);
    unsigned Binary_Value_Count(
        void);
    uint32_t Binary_Value_Index_To_Instance(
        unsigned index);
    char *Binary_Value_Name(
        uint32_t object_instance);

    void Binary_Value_Init(
        void);

    BACNET_BINARY_PV Binary_Value_Present_Value(
        uint32_t object_instance);

    int Binary_Value_Encode_Property_APDU(
        uint8_t * apdu,
        uint32_t object_instance,
        BACNET_PROPERTY_ID property,
        uint32_t array_index,
        BACNET_ERROR_CLASS * error_class,
        BACNET_ERROR_CODE * error_code);

    bool Binary_Value_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data,
        BACNET_ERROR_CLASS * error_class,
        BACNET_ERROR_CODE * error_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
