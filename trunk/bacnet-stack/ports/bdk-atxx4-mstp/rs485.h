/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2004 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307
 USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

#ifndef RS485_H
#define RS485_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void rs485_init(
        void);
    void rs485_rts_enable(
        bool enable);
    bool rs485_byte_available(
        uint8_t * data_register);
    bool rs485_receive_error(
        void);
    void rs485_bytes_send(
        uint8_t * buffer,       /* data to send */
        uint16_t nbytes);       /* number of bytes of data */
    uint32_t rs485_baud_rate(
        void);
    bool rs485_baud_rate_set(
        uint32_t baud);

    void rs485_turnaround_delay(
        void);
    void rs485_silence_time_reset(
        void);
    bool rs485_silence_time_elapsed(
        uint16_t milliseconds);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
