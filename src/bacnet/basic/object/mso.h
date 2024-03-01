/**
 * @file
 * @author Steve Karg
 * @date 2005
 * @brief Multi-State Output objects, customize for your use
 *
 * @section DESCRIPTION
 *
 * The Multi-State object is an object with a present-value that
 * uses an integer data type with a sequence of 1 to N values.
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
#ifndef BACNET_MULTISTATE_OUTPUT_H
#define BACNET_MULTISTATE_OUTPUT_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/config.h"
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacerror.h"
#include "bacnet/cov.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

/**
 * @brief Callback for gateway write present value request
 * @param  object_instance - object-instance number of the object
 * @param  old_value - multistate preset-value prior to write
 * @param  value - multistate preset-value of the write
 */
typedef void (*multistate_output_write_present_value_callback)(
    uint32_t object_instance, uint32_t old_value,
    uint32_t value);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void Multistate_Output_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    BACNET_STACK_EXPORT
    bool Multistate_Output_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Multistate_Output_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Multistate_Output_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Multistate_Output_Instance_To_Index(
        uint32_t object_instance);

    BACNET_STACK_EXPORT
    int Multistate_Output_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);

    BACNET_STACK_EXPORT
    bool Multistate_Output_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_STACK_EXPORT
    bool Multistate_Output_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Multistate_Output_Name_Set(
        uint32_t object_instance,
        char *new_name);

    BACNET_STACK_EXPORT
    uint32_t Multistate_Output_Present_Value(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Multistate_Output_Present_Value_Set(
        uint32_t object_instance,
        uint32_t value,
        unsigned priority);
    BACNET_STACK_EXPORT
    bool Multistate_Output_Present_Value_Relinquish(
        uint32_t instance,
        unsigned priority);
    BACNET_STACK_EXPORT
    unsigned Multistate_Output_Present_Value_Priority(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Multistate_Output_Write_Present_Value_Callback_Set(
        multistate_output_write_present_value_callback cb);

    BACNET_STACK_EXPORT
    bool Multistate_Output_Change_Of_Value(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Multistate_Output_Change_Of_Value_Clear(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Multistate_Output_Encode_Value_List(
        uint32_t object_instance,
        BACNET_PROPERTY_VALUE * value_list);

    BACNET_STACK_EXPORT
    bool Multistate_Output_Out_Of_Service(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Multistate_Output_Out_Of_Service_Set(
        uint32_t object_instance,
        bool value);

    BACNET_STACK_EXPORT
    char *Multistate_Output_Description(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Multistate_Output_Description_Set(
        uint32_t object_instance,
        char *text_string);

    BACNET_STACK_EXPORT
    bool Multistate_Output_State_Text_Set(
        uint32_t object_instance,
        uint32_t state_index,
        char *new_name);
    BACNET_STACK_EXPORT
    bool Multistate_Output_Max_States_Set(
        uint32_t instance,
        uint32_t max_states_requested);
    BACNET_STACK_EXPORT
    char *Multistate_Output_State_Text(
        uint32_t object_instance,
        uint32_t state_index);
    BACNET_STACK_EXPORT
    bool Multistate_Output_State_Text_List_Set(
        uint32_t object_instance,
        const char *state_text_list);

    BACNET_STACK_EXPORT
    BACNET_RELIABILITY Multistate_Output_Reliability(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Multistate_Output_Reliability_Set(
        uint32_t object_instance,
        BACNET_RELIABILITY value);

    BACNET_STACK_EXPORT
    uint32_t Multistate_Output_Relinquish_Default(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Multistate_Output_Relinquish_Default_Set(
        uint32_t object_instance,
        uint32_t value);

    BACNET_STACK_EXPORT
    uint32_t Multistate_Output_Create(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Multistate_Output_Delete(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Multistate_Output_Cleanup(
        void);

    BACNET_STACK_EXPORT
    void Multistate_Output_Init(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
