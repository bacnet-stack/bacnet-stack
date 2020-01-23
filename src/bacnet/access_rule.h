/**************************************************************************
*
* Copyright (C) 2015 Nikola Jelic <nikola.jelic@euroicc.com>
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

#ifndef ACCESS_RULE_H
#define ACCESS_RULE_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacdevobjpropref.h"

typedef enum {
    TIME_RANGE_SPECIFIER_SPECIFIED = 0,
    TIME_RANGE_SPECIFIER_ALWAYS = 1,
    TIME_RANGE_SPECIFIER_MAX = 2
} BACNET_ACCESS_RULE_TIME_RANGE_SPECIFIER;

typedef enum {
    LOCATION_SPECIFIER_SPECIFIED = 0,
    LOCATION_SPECIFIER_ALL = 1,
    LOCATION_SPECIFIER_MAX = 2
} BACNET_ACCESS_RULE_LOCATION_SPECIFIER;

typedef struct {
    BACNET_ACCESS_RULE_TIME_RANGE_SPECIFIER time_range_specifier;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE time_range;
    BACNET_ACCESS_RULE_LOCATION_SPECIFIER location_specifier;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE location;
    bool enable;
} BACNET_ACCESS_RULE;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    int bacapp_encode_access_rule(
        uint8_t * apdu,
        BACNET_ACCESS_RULE * rule);
    int bacapp_encode_context_access_rule(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_ACCESS_RULE * rule);
    int bacapp_decode_access_rule(
        uint8_t * apdu,
        BACNET_ACCESS_RULE * rule);
    int bacapp_decode_context_access_rule(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_ACCESS_RULE * rule);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
