/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic WriteProperty service handler
*
* @section LICENSE
*
* SPDX-License-Identifier: MIT
*/
#ifndef HANDLER_WRITE_PROPERTY_H
#define HANDLER_WRITE_PROPERTY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacenum.h"
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void handler_write_property(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data);

    BACNET_STACK_EXPORT
    bool WPValidateString(
        BACNET_APPLICATION_DATA_VALUE * pValue,
        int iMaxLen,
        bool bEmptyAllowed,
        BACNET_ERROR_CLASS * pErrorClass,
        BACNET_ERROR_CODE * pErrorCode);

    BACNET_STACK_EXPORT
    bool WPValidateArgType(
        BACNET_APPLICATION_DATA_VALUE * pValue,
        uint8_t ucExpectedType,
        BACNET_ERROR_CLASS * pErrorClass,
        BACNET_ERROR_CODE * pErrorCode);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
