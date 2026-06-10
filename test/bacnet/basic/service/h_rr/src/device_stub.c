/**
 * @file
 * @brief Stub implementation for device-related functions needed by h_rr tests
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/readrange.h"
#include "bacnet/basic/object/device.h"

/**
 * Control variables for test stubs
 */
bool Device_RR_Info_Should_Return_Handler = false;

/**
 * Stub RR handler that returns a simple list of values
 * This handler demonstrates the basic payload encoding for a ReadRange response
 */
int Stub_RR_Handler(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest)
{
    int apdu_len = 0;

    if (pRequest == NULL || apdu == NULL) {
        return -1;
    }

    /* For this stub, we'll encode a simple integer list as response
     * Tag context 0: opening tag
     */
    if (pRequest->RequestType == RR_READ_ALL) {
        /* Encode a simple response with a few dummy integer values
         * This is a minimal demonstration of encoding */
        apdu_len = 0;
        /* Set result flags: FIRST_ITEM and LAST_ITEM */
        bitstring_init(&pRequest->ResultFlags);
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM, 1);
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM, 1);
    }

    return apdu_len;
}

/**
 * Stub RR info function that returns handler for RR requests
 * This is the rr_info_function type that fills in RR_PROP_INFO
 */
bool Stub_RR_Info_Function(
    BACNET_READ_RANGE_DATA *pRequest, RR_PROP_INFO *pInfo)
{
    if (pRequest == NULL || pInfo == NULL) {
        return false;
    }

    /* Indicate we support all request types */
    pInfo->RequestTypes = RR_READ_ALL | RR_BY_POSITION;
    pInfo->Handler = Stub_RR_Handler;

    return true;
}

/**
 * Stub implementation of Device_Objects_RR_Info
 * Returns a pointer to the RR info function for the given object type,
 * or NULL if not supported or not found.
 */
rr_info_function Device_Objects_RR_Info(BACNET_OBJECT_TYPE object_type)
{
    (void)object_type;
    /* Return the configured handler or NULL */
    if (Device_RR_Info_Should_Return_Handler) {
        return Stub_RR_Info_Function;
    }
    return NULL;
}

/**
 * Mock Device_Valid_Object_Instance_Number to control test behavior
 */
bool Device_Valid_Object_Instance_Number(uint32_t object_id)
{
    (void)object_id;
    return true;
}
