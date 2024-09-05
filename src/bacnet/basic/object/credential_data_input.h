/**
 * @file
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @brief Credential Data Input Objects - customize for your use
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_CREDENTIAL_DATA_INPUT_H
#define BACNET_BASIC_OBJECT_CREDENTIAL_DATA_INPUT_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/timestamp.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/authentication_factor.h"
#include "bacnet/authentication_factor_format.h"
#include "bacnet/timestamp.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

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

BACNET_STACK_EXPORT
void Credential_Data_Input_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);
BACNET_STACK_EXPORT
bool Credential_Data_Input_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Credential_Data_Input_Count(void);
BACNET_STACK_EXPORT
uint32_t Credential_Data_Input_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Credential_Data_Input_Instance_To_Index(uint32_t instance);
BACNET_STACK_EXPORT
bool Credential_Data_Input_Object_Instance_Add(uint32_t instance);

BACNET_STACK_EXPORT
bool Credential_Data_Input_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Credential_Data_Input_Name_Set(uint32_t object_instance, char *new_name);

BACNET_STACK_EXPORT
bool Credential_Data_Input_Out_Of_Service(uint32_t instance);
BACNET_STACK_EXPORT
void Credential_Data_Input_Out_Of_Service_Set(uint32_t instance, bool oos_flag);

BACNET_STACK_EXPORT
int Credential_Data_Input_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Credential_Data_Input_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
uint32_t Credential_Data_Input_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Credential_Data_Input_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Credential_Data_Input_Cleanup(void);
BACNET_STACK_EXPORT
void Credential_Data_Input_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
