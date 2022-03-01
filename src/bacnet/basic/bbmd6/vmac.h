/**
* @file
* @author Steve Karg
* @date 2016
*/
#ifndef VMAC_H
#define VMAC_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacnet_stack_exports.h"

/* define the max MAC as big as IPv6 + port number */
#define VMAC_MAC_MAX 18
/**
* VMAC data structure
*
* @{
*/
struct vmac_data {
    uint8_t mac[18];
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
    bool VMAC_Find_By_Data(struct vmac_data *vmac, uint32_t *device_id);
    BACNET_STACK_EXPORT
    bool VMAC_Add(uint32_t device_id, struct vmac_data *pVMAC);
    BACNET_STACK_EXPORT
    bool VMAC_Delete(uint32_t device_id);
    BACNET_STACK_EXPORT
    bool VMAC_Different(
        struct vmac_data *vmac1,
        struct vmac_data *vmac2);
    BACNET_STACK_EXPORT
    bool VMAC_Match(
        struct vmac_data *vmac1,
        struct vmac_data *vmac2);
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
