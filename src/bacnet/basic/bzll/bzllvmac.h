/**
 * @file
 * @brief API for Virtual MAC (VMAC) of BACnet Zigbee Link Layer (BZLL)
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_BZLL_VMAC_H
#define BACNET_BASIC_BZLL_VMAC_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/* define the max MAC as big as EUI64 */
#define BZLL_VMAC_EUI64 8
/**
 * VMAC data structure
 *
 * @{
 */
struct bzll_vmac_data {
    uint8_t mac[BZLL_VMAC_EUI64];
    uint8_t endpoint;
};
/** @} */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
bool BZLL_VMAC_Entry_By_Device_ID(
    uint32_t device_id, struct bzll_vmac_data *vmac);
BACNET_STACK_EXPORT
bool BZLL_VMAC_Entry_By_Index(
    int index, uint32_t *device_id, struct bzll_vmac_data *vmac);
BACNET_STACK_EXPORT
bool BZLL_VMAC_Entry_To_Device_ID(
    const struct bzll_vmac_data *vmac, uint32_t *device_id);
BACNET_STACK_EXPORT
bool BZLL_VMAC_Entry_Set(
    struct bzll_vmac_data *vmac, const uint8_t *mac, uint8_t endpoint);
BACNET_STACK_EXPORT
bool BZLL_VMAC_Add(uint32_t device_id, const struct bzll_vmac_data *vmac);
BACNET_STACK_EXPORT
bool BZLL_VMAC_Delete(uint32_t device_id);
BACNET_STACK_EXPORT
bool BZLL_VMAC_Same(
    const struct bzll_vmac_data *vmac1, const struct bzll_vmac_data *vmac2);

BACNET_STACK_EXPORT
unsigned int BZLL_VMAC_Count(void);
BACNET_STACK_EXPORT
void BZLL_VMAC_Cleanup(void);
BACNET_STACK_EXPORT
void BZLL_VMAC_Init(void);

BACNET_STACK_EXPORT
void BZLL_VMAC_Debug_Enable(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
