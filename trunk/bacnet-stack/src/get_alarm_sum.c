/**************************************************************************
*
* Copyright (C) 2011 Krzysztof Malorny <malornykrzysztof@gmail.com>
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
#include <assert.h>

#include "bacdcode.h"
#include "get_alarm_sum.h"
#include "npdu.h"


int get_alarm_summary_ack_encode_apdu_init(
    uint8_t * apdu,
    uint8_t invoke_id)
{
    int apdu_len = 0;   /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK; /* complex ACK service */
        apdu[1] = invoke_id;            /* original invoke id from request */
        apdu[2] = SERVICE_CONFIRMED_GET_ALARM_SUMMARY;
        apdu_len = 3;
    }

    return apdu_len;
}

int get_alarm_sumary_ack_encode_apdu_data(
    uint8_t * apdu,
    size_t max_apdu,
    BACNET_GET_ALARM_SUMMARY_DATA * get_alarm_data)
{
    int apdu_len = 0;   /* total length of the apdu, return value */

    if (!apdu) {
        apdu_len = BACNET_STATUS_ERROR;
    }
    else if(max_apdu >= 10) {
        /* tag 0 - Object Identifier */
        apdu_len += encode_application_object_id(&apdu[apdu_len],
                        (int)get_alarm_data->objectIdentifier.type,
                        get_alarm_data->objectIdentifier.instance);
        /* tag 1 - Alarm State */
        apdu_len += encode_application_enumerated(&apdu[apdu_len],
                        get_alarm_data->alarmState);
        /* tag 2 - Acknowledged Transitions */
        apdu_len += encode_application_bitstring(&apdu[apdu_len],
                        &get_alarm_data->acknowledgedTransitions);
    }
    else {
        apdu_len = BACNET_STATUS_ABORT;
    }

    return apdu_len;
}
