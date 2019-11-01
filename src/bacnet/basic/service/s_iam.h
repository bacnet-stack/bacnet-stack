/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic I-Am service send
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
#ifndef SEND_I_AM_H
#define SEND_I_AM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/apdu.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/npdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void Send_I_Am_To_Network(BACNET_ADDRESS* target_address, uint32_t device_id,
                          unsigned int max_apdu, int segmentation,
                          uint16_t vendor_id);
int iam_encode_pdu(uint8_t* buffer, BACNET_ADDRESS* dest,
                   BACNET_NPDU_DATA* npdu_data);
void Send_I_Am(uint8_t* buffer);
int iam_unicast_encode_pdu(uint8_t* buffer, BACNET_ADDRESS* src,
                           BACNET_ADDRESS* dest, BACNET_NPDU_DATA* npdu_data);
void Send_I_Am_Unicast(uint8_t* buffer, BACNET_ADDRESS* src);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
