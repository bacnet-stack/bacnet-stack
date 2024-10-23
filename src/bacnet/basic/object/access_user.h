/**
 * @file
 * @brief API for basic BACnet Access User Object implementation.
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_ACCESS_USER_H
#define BACNET_BASIC_OBJECT_ACCESS_USER_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
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
    const int **pRequired, const int **pOptional, const int **pProprietary);
BACNET_STACK_EXPORT
bool Access_User_Valid_Instance(uint32_t object_instance);
unsigned Access_User_Count(void);
BACNET_STACK_EXPORT
uint32_t Access_User_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Access_User_Instance_To_Index(uint32_t instance);
BACNET_STACK_EXPORT
bool Access_User_Object_Instance_Add(uint32_t instance);

BACNET_STACK_EXPORT
bool Access_User_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Access_User_Name_Set(uint32_t object_instance, char *new_name);

BACNET_STACK_EXPORT
int Access_User_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Access_User_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
uint32_t Access_User_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Access_User_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Access_User_Cleanup(void);
BACNET_STACK_EXPORT
void Access_User_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
