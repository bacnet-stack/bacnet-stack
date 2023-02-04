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
#ifndef BACADDR_H
#define BACADDR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void bacnet_address_copy(BACNET_ADDRESS *dest, BACNET_ADDRESS *src);
BACNET_STACK_EXPORT
bool bacnet_address_same(BACNET_ADDRESS *dest, BACNET_ADDRESS *src);
BACNET_STACK_EXPORT
bool bacnet_address_init(BACNET_ADDRESS *dest,
    BACNET_MAC_ADDRESS *mac,
    uint16_t dnet,
    BACNET_MAC_ADDRESS *adr);

BACNET_STACK_EXPORT
bool bacnet_address_mac_same(
    BACNET_MAC_ADDRESS *dest, BACNET_MAC_ADDRESS *src);
BACNET_STACK_EXPORT
void bacnet_address_mac_init(
    BACNET_MAC_ADDRESS *mac, uint8_t *adr, uint8_t len);
BACNET_STACK_EXPORT
bool bacnet_address_mac_from_ascii(BACNET_MAC_ADDRESS *mac, const char *arg);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
