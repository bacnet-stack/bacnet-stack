/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic TimeSynchronization service handler
*
* @section LICENSE
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
*/
#ifndef HANDLER_TIME_SYNCHRONIZATION_H
#define HANDLER_TIME_SYNCHRONIZATION_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/datetime.h"
#include "bacnet/wp.h"

typedef void (*handler_timesync_set_callback_t)(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    bool utc);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* time synchronization handlers */
    BACNET_STACK_EXPORT
    void handler_timesync(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src);
    BACNET_STACK_EXPORT
    void handler_timesync_utc(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src);
    /* time sync master features */
    BACNET_STACK_EXPORT
    int handler_timesync_encode_recipients(
        uint8_t * apdu,
        int max_apdu);
    BACNET_STACK_EXPORT
    bool handler_timesync_align_intervals_set(bool flag);
    BACNET_STACK_EXPORT
    bool handler_timesync_align_intervals(void);
    BACNET_STACK_EXPORT
    bool handler_timesync_interval_set(uint32_t minutes);
    BACNET_STACK_EXPORT
    uint32_t handler_timesync_interval(void);
    BACNET_STACK_EXPORT
    bool handler_timesync_interval_offset_set(uint32_t minutes);
    BACNET_STACK_EXPORT
    uint32_t handler_timesync_interval_offset(void);
    BACNET_STACK_EXPORT
    void handler_timesync_task(BACNET_DATE_TIME *bdatetime);
    BACNET_STACK_EXPORT
    void handler_timesync_init(void);
    BACNET_STACK_EXPORT
    bool handler_timesync_recipient_write(
        BACNET_WRITE_PROPERTY_DATA * wp_data);
    BACNET_STACK_EXPORT
    bool handler_timesync_interval_align(void);
    BACNET_STACK_EXPORT
    bool handler_timesync_interval_align_set(bool flag);
    BACNET_STACK_EXPORT
    bool handler_timesync_recipient_address_set(
        unsigned index,
        BACNET_ADDRESS * address);
    BACNET_STACK_EXPORT
    void handler_timesync_set_callback_set(
        handler_timesync_set_callback_t cb);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
