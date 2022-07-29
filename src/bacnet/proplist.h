/**************************************************************************
*
* Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
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
*********************************************************************/
#ifndef PROPLIST_H
#define PROPLIST_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/rp.h"

/** @file Property_List property encode decode helper */
struct property_list_t {
    const int *pList;
    unsigned count;
};

struct special_property_list_t {
    struct property_list_t Required;
    struct property_list_t Optional;
    struct property_list_t Proprietary;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    unsigned property_list_count(
        const int *pList);
    BACNET_STACK_EXPORT
    bool property_list_member(
        const int *pList,
        int object_property);
    BACNET_STACK_EXPORT
    int property_list_encode(
        BACNET_READ_PROPERTY_DATA * rpdata,
        const int *pListRequired,
        const int *pListOptional,
        const int *pListProprietary);
    BACNET_STACK_EXPORT
    int property_list_common_encode(
        BACNET_READ_PROPERTY_DATA *rpdata,
        uint32_t device_instance_number);
    BACNET_STACK_EXPORT
    bool property_list_common(
        BACNET_PROPERTY_ID property);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
