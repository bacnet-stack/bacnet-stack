/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/
#ifndef CLIENT_H
#define CLIENT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "apdu.h"
#include "bacapp.h"
#include "bacenum.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* unconfirmed requests */
    void Send_WhoIs(
        int32_t low_limit,
        int32_t high_limit);

    void Send_WhoHas_Object(
        int32_t low_limit,
        int32_t high_limit,
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance);

    void Send_WhoHas_Name(
        int32_t low_limit,
        int32_t high_limit,
        char *object_name);

    void Send_I_Have(
        uint32_t device_id,
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance,
        char *object_name);

/* returns the invoke ID for confirmed request, or 0 if failed */
    uint8_t Send_Read_Property_Request(
        uint32_t device_id,     /* destination device */
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance,
        BACNET_PROPERTY_ID object_property,
        int32_t array_index);

/* returns the invoke ID for confirmed request, or 0 if failed */
    uint8_t Send_Write_Property_Request(
        uint32_t device_id,     /* destination device */
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance,
        BACNET_PROPERTY_ID object_property,
        BACNET_APPLICATION_DATA_VALUE * object_value,
        uint8_t priority,
        int32_t array_index);

/* returns the invoke ID for confirmed request, or 0 if failed */
    uint8_t Send_Reinitialize_Device_Request(
        uint32_t device_id,
        BACNET_REINITIALIZED_STATE state,
        char *password);

/* returns the invoke ID for confirmed request, or 0 if failed */
    uint8_t Send_Device_Communication_Control_Request(
        uint32_t device_id,
        uint16_t timeDuration,  /* 0=optional */
        BACNET_COMMUNICATION_ENABLE_DISABLE state,
        char *password);        /* NULL=optional */

    void Send_TimeSync(
        BACNET_DATE * bdate,
        BACNET_TIME * btime);
    void Send_TimeSyncUTC(
        BACNET_DATE * bdate,
        BACNET_TIME * btime);

    uint8_t Send_Atomic_Read_File_Stream(
        uint32_t device_id,
        uint32_t file_instance,
        int fileStartPosition,
        unsigned requestedOctetCount);
    uint8_t Send_Atomic_Write_File_Stream(
        uint32_t device_id,
        uint32_t file_instance,
        int fileStartPosition,
        BACNET_OCTET_STRING * fileData);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
