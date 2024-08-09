/**
 * @file
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @brief API for a basic BACnet OctetString Value object implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_OCTET_STRING_VALUE_H
#define BACNET_BASIC_OBJECT_OCTET_STRING_VALUE_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/wp.h"
#include "bacnet/rp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct octetstring_value_descr {
        unsigned Event_State:3;
        bool Out_Of_Service;
        BACNET_OCTET_STRING Present_Value;
    } OCTETSTRING_VALUE_DESCR;


    BACNET_STACK_EXPORT
    void OctetString_Value_Property_Lists(const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    BACNET_STACK_EXPORT
    bool OctetString_Value_Valid_Instance(uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned OctetString_Value_Count(void);
    BACNET_STACK_EXPORT
    uint32_t OctetString_Value_Index_To_Instance(unsigned index);
    BACNET_STACK_EXPORT
    unsigned OctetString_Value_Instance_To_Index(uint32_t object_instance);

    BACNET_STACK_EXPORT
    bool OctetString_Value_Object_Name(uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);

    BACNET_STACK_EXPORT
    int OctetString_Value_Read_Property(BACNET_READ_PROPERTY_DATA * rpdata);

    BACNET_STACK_EXPORT
    bool OctetString_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *
        wp_data);

    BACNET_STACK_EXPORT
    bool OctetString_Value_Present_Value_Set(uint32_t object_instance,
        const BACNET_OCTET_STRING * value,
        uint8_t priority);
    BACNET_STACK_EXPORT
    BACNET_OCTET_STRING *OctetString_Value_Present_Value(uint32_t
        object_instance);

    BACNET_STACK_EXPORT
    bool OctetString_Value_Change_Of_Value(uint32_t instance);
    BACNET_STACK_EXPORT
    void OctetString_Value_Change_Of_Value_Clear(uint32_t instance);
    BACNET_STACK_EXPORT
    bool OctetString_Value_Encode_Value_List(uint32_t object_instance,
        BACNET_PROPERTY_VALUE * value_list);

    BACNET_STACK_EXPORT
    char *OctetString_Value_Description(uint32_t instance);
    BACNET_STACK_EXPORT
    bool OctetString_Value_Description_Set(uint32_t instance,
        const char *new_name);

    BACNET_STACK_EXPORT
    bool OctetString_Value_Out_Of_Service(uint32_t instance);
    BACNET_STACK_EXPORT
    void OctetString_Value_Out_Of_Service_Set(uint32_t instance,
        bool oos_flag);

    /* note: header of Intrinsic_Reporting function is required
       even when INTRINSIC_REPORTING is not defined */
    BACNET_STACK_EXPORT
    void OctetString_Value_Intrinsic_Reporting(uint32_t object_instance);

    BACNET_STACK_EXPORT
    void OctetString_Value_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
