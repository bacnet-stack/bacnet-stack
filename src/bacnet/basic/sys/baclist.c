/**
 * @file
 * @brief A dynamic BACnetLIST or BACnetARRAY implementation
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2024
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "bacnet/basic/sys/baclist.h"

/**
 * @brief Add a key and data to a BACnetLIST
 * @param list Pointer to the list
 * @param key Key to be added
 * @param data Pointer to the data to be added
 */
void BACnet_List_Sublist_Add(BACNET_LIST *list, KEY key, BACNET_LIST *sub_list)
{
    if (list) {
        BACnet_List_Init(list);
        Keylist_Data_Add(list->list, key, sub_list);
    }
}

/**
 * @brief Add a key and data to a BACnetLIST
 * @param list Pointer to the list
 * @param key Key to be added
 * @param data Pointer to the data to be added
 */
BACNET_LIST *BACnet_List_Sublist(BACNET_LIST *list, KEY key)
{
    BACNET_LIST *sub_list = NULL;

    if (list) {
        BACnet_List_Init(list);
        sub_list = Keylist_Data(list->list, key);
    }

    return sub_list;
}

/**
 * @brief Get the size of the list previously set
 * @param list Pointer to the list
 * @return The size of the list
 */
uint32_t BACnet_List_Size(BACNET_LIST *list)
{
    uint32_t size = 0;

    if (list) {
        size = list->size
    }

    return size;
}

/**
 * @brief Set the size of the list
 * @param list Pointer to the list
 * @param size The size of the list
 */
void BACnet_List_Size_Set(BACNET_LIST *list, uint32_t size)
{
    if (list) {
        list->size = size;
    }
}

/**
 * @brief Get the current index of the list previously set
 * @param list Pointer to the list
 * @return The current index of the list
 */
int32_t BACnet_List_Index(BACNET_LIST *list)
{
    int32_t index = 0;

    if (list) {
        index = list->index;
    }

    return index;
}

/**
 * @brief Set the index of the list
 * @param list Pointer to the list
 * @param size The index of the list
 */
void BACnet_List_Index_Set(BACNET_LIST *list, int32_t index)
{
    if (list) {
        list->index = index;
    }
}

/**
 * @brief free all the key list objects and properties
 * @param list Pointer to the list, may include sub-lists
 * @note recursive function!
 */
void BACnet_List_Cleanup(BACNET_LIST *list)
{
    BACNET_LIST *sub_list;

    if (list) {
        do {
            sub_list = Keylist_Data_Pop(list->list);
            BACnet_List_Cleanup(sub_list);
        } while (sub_list);
        Keylist_Delete(list->list);
        list->list = NULL;
    }
}

/**
 * @brief Initialize the BACnetLIST
 * @param list Pointer to the list
 */
void BACnet_List_Init(BACNET_LIST *list)
{
    if (list) {
        if (list->list == NULL) {
            list->list = Keylist_Create();
            list->size = 0;
            list->index = 0;
        }
    }
}

BACNET_LIST *BACnet_List_Create(void)
{
    return (BACNET_LIST *)calloc(1, sizeof(BACNET_LIST));
}
