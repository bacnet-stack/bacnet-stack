/**
 * @file
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2014
 * @brief API for basic  Command objects, customize for your use
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_COMMAND_H
#define BACNET_BASIC_OBJECT_COMMAND_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifndef MAX_COMMANDS
#define MAX_COMMANDS 4
#endif

#ifndef MAX_COMMAND_ACTIONS
#define MAX_COMMAND_ACTIONS 8
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct bacnet_action_list {
        BACNET_OBJECT_ID Device_Id;     /* Optional */
        BACNET_OBJECT_ID Object_Id;
        BACNET_PROPERTY_ID Property_Identifier;
        uint32_t Property_Array_Index;  /* Conditional */
        BACNET_APPLICATION_DATA_VALUE Value;
        uint8_t Priority;       /* Conditional */
        uint32_t Post_Delay;    /* Optional */
        bool Quit_On_Failure;
        bool Write_Successful;
        struct bacnet_action_list *next;
    } BACNET_ACTION_LIST;

    int cl_encode_apdu(
        uint8_t * apdu,
        BACNET_ACTION_LIST * bcl);

    int cl_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_APPLICATION_TAG tag,
        BACNET_ACTION_LIST * bcl);

    typedef struct command_descr {
        uint32_t Present_Value;
        bool In_Process;
        bool All_Writes_Successful;
        BACNET_ACTION_LIST Action[MAX_COMMAND_ACTIONS];
    } COMMAND_DESCR;

    BACNET_STACK_EXPORT
    void Command_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    BACNET_STACK_EXPORT
    bool Command_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Command_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Command_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Command_Instance_To_Index(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Command_Object_Instance_Add(
        uint32_t instance);

    BACNET_STACK_EXPORT
    bool Command_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Command_Name_Set(
        uint32_t object_instance,
        char *new_name);

    BACNET_STACK_EXPORT
    char *Command_Description(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Command_Description_Set(
        uint32_t instance,
        char *new_name);

    BACNET_STACK_EXPORT
    int Command_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    BACNET_STACK_EXPORT
    bool Command_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_STACK_EXPORT
    uint32_t Command_Present_Value(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Command_Present_Value_Set(
        uint32_t object_instance,
        uint32_t value);

    BACNET_STACK_EXPORT
    bool Command_In_Process(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Command_In_Process_Set(
        uint32_t object_instance,
        bool value);

    BACNET_STACK_EXPORT
    bool Command_All_Writes_Successful(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Command_All_Writes_Successful_Set(
        uint32_t object_instance,
        bool value);

    BACNET_STACK_EXPORT
    bool Command_Change_Of_Value(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Command_Change_Of_Value_Clear(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Command_Encode_Value_List(
        uint32_t object_instance,
        BACNET_PROPERTY_VALUE * value_list);
    float Command_COV_Increment(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Command_COV_Increment_Set(
        uint32_t instance,
        float value);

    /* note: header of Intrinsic_Reporting function is required
       even when INTRINSIC_REPORTING is not defined */
    BACNET_STACK_EXPORT
    void Command_Intrinsic_Reporting(
        uint32_t object_instance);

    BACNET_STACK_EXPORT
    void Command_Init(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
