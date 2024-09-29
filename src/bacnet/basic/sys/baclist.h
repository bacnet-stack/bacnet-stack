/**
 * @file
 * @brief A dynamic BACnetLIST or BACnetARRAY implementation
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SYS_BACLIST_H
#define BACNET_SYS_BACLIST_H
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

typedef struct bacnet_list {
    OS_Keylist list;
    /* used for discovering object property list or array */
    uint32_t size;
    int32_t index;
} BACNET_LIST;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void BACnet_List_Sublist_Add(BACNET_LIST *list, KEY key, BACNET_LIST *sub_list);
BACNET_LIST *BACnet_List_Sublist(BACNET_LIST *list, KEY key);
uint32_t BACnet_List_Size(BACNET_LIST *list);
void BACnet_List_Size_Set(BACNET_LIST *list, uint32_t size);
int32_t BACnet_List_Index(BACNET_LIST *list);
void BACnet_List_Index_Set(BACNET_LIST *list, int32_t index);

void BACnet_List_Init(BACNET_LIST *list);
void BACnet_List_Cleanup(BACNET_LIST *list);
BACNET_LIST *BACnet_List_Create(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
