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
#ifndef PROPERTY_H
#define PROPERTY_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/rp.h"
#include "bacnet/proplist.h"

/** @file property.h  Library of all required and optional object properties */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    const int * property_list_optional(
        BACNET_OBJECT_TYPE object_type);
    const int * property_list_required(
        BACNET_OBJECT_TYPE object_type);
    void property_list_special(
        BACNET_OBJECT_TYPE object_type,
        struct special_property_list_t *pPropertyList);
    BACNET_PROPERTY_ID property_list_special_property(
        BACNET_OBJECT_TYPE object_type,
        BACNET_PROPERTY_ID special_property,
        unsigned index);
    unsigned property_list_special_count(
        BACNET_OBJECT_TYPE object_type,
        BACNET_PROPERTY_ID special_property);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
