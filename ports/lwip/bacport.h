/**************************************************************************
*
* Copyright (C) 2020 Steve Karg <skarg@users.sourceforge.net>
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

#ifndef BACPORT_H
#define BACPORT_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "lwip/def.h"
#include "lwip/udp.h"
#include "lwip/memp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/inet.h"

#define BACNET_OBJECT_TABLE(table_name, _type, _init, _count,               \
                            _index_to_instance, _valid_instance, _object_name, \
                            _read_property, _write_property, _RPM_list,     \
                            _RR_info, _iterator, _value_list, _COV,         \
                            _COV_clear, _intrinsic_reporting)               \
    static_assert(false, "Unsupported BACNET_OBJECT_TABLE for this platform")

#endif
