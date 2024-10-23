/**
 * @file
 * @brief API for Virtual MAC (VMAC) of BACnet/IPv6 neighbors
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2016
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_BBMD6_VMAC_H
#define BACNET_BASIC_BBMD6_VMAC_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/* define the max MAC as big as IPv6 + port number */
#define VMAC_MAC_MAX 18
/**
 * VMAC data structure
 *
 * @{
 */
struct vmac_data {
    uint8_t mac[VMAC_MAC_MAX];
    uint8_t mac_len;
};
/** @} */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
unsigned int VMAC_Count(void);
BACNET_STACK_EXPORT
struct vmac_data *VMAC_Find_By_Key(uint32_t device_id);
BACNET_STACK_EXPORT
bool VMAC_Find_By_Data(const struct vmac_data *vmac, uint32_t *device_id);
BACNET_STACK_EXPORT
bool VMAC_Add(uint32_t device_id, const struct vmac_data *pVMAC);
BACNET_STACK_EXPORT
bool VMAC_Delete(uint32_t device_id);
BACNET_STACK_EXPORT
bool VMAC_Different(
    const struct vmac_data *vmac1, const struct vmac_data *vmac2);
BACNET_STACK_EXPORT
bool VMAC_Match(const struct vmac_data *vmac1, const struct vmac_data *vmac2);
BACNET_STACK_EXPORT
void VMAC_Cleanup(void);
BACNET_STACK_EXPORT
void VMAC_Init(void);
BACNET_STACK_EXPORT
void VMAC_Debug_Enable(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
