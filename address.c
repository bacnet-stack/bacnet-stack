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
 Boston, MA  02111-1307, USA.

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
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "config.h"
#include "bacaddr.h"
#include "address.h"
#include "bacdef.h"
#include "bacdcode.h"

/* This module is used to handle the address binding that */
/* occurs in BACnet.  A device id is bound to a MAC address. */
/* The normal method is using Who-Is, and using the data from I-Am */

static struct Address_Cache_Entry {
    bool valid;
    bool bind_request;
    uint32_t device_id;
    unsigned max_apdu;
    BACNET_ADDRESS address;
} Address_Cache[MAX_ADDRESS_CACHE];

void address_remove_device(uint32_t device_id)
{
    unsigned i;

    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        if ((Address_Cache[i].valid ||
                Address_Cache[i].bind_request) &&
            (Address_Cache[i].device_id == device_id)) {
            Address_Cache[i].valid = false;
            break;
        }
    }

    return;
}

void address_init(void)
{
    unsigned i;

    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        Address_Cache[i].valid = false;
        Address_Cache[i].bind_request = false;
    }

    return;
}

bool address_get_by_device(uint32_t device_id,
    unsigned *max_apdu, BACNET_ADDRESS * src)
{
    unsigned i;
    bool found = false;         /* return value */

    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        if (Address_Cache[i].valid &&
            (Address_Cache[i].device_id == device_id)) {
            bacnet_address_copy(src, &Address_Cache[i].address);
            *max_apdu = Address_Cache[i].max_apdu;
            found = true;
            break;
        }
    }

    return found;
}

void address_add(uint32_t device_id,
    unsigned max_apdu, BACNET_ADDRESS * src)
{
    unsigned i;
    bool found = false;         /* return value */

    /* existing device - update address */
    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        if (Address_Cache[i].valid &&
            (Address_Cache[i].device_id == device_id)) {
            bacnet_address_copy(&Address_Cache[i].address, src);
            Address_Cache[i].max_apdu = max_apdu;
            found = true;
            break;
        }
    }
    /* new device */
    if (!found) {
        for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
            if (!Address_Cache[i].valid) {
                Address_Cache[i].valid = true;
                Address_Cache[i].device_id = device_id;
                Address_Cache[i].max_apdu = max_apdu;
                bacnet_address_copy(&Address_Cache[i].address, src);
                break;
            }
        }
    }

    return;
}

/* returns true if device is already bound */
/* also returns the address and max apdu if already bound */
bool address_bind_request(uint32_t device_id,
    unsigned *max_apdu, BACNET_ADDRESS * src)
{
    unsigned i;
    bool found = false;         /* return value */

    /* existing device - update address */
    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        if (Address_Cache[i].valid &&
            (Address_Cache[i].device_id == device_id)) {
            found = true;
            bacnet_address_copy(src, &Address_Cache[i].address);
            *max_apdu = Address_Cache[i].max_apdu;
            break;
        }
        /* already have a bind request active for this puppy */
        else if (Address_Cache[i].bind_request &&
            (Address_Cache[i].device_id == device_id)) {
            return found;
        }
    }

    if (!found) {
        for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
            if (!(Address_Cache[i].bind_request || Address_Cache[i].valid)) {
                Address_Cache[i].bind_request = true;
                Address_Cache[i].device_id = device_id;
                /* now would be a good time to do a Who-Is request */
                break;
            }
        }
    }

    return found;
}

void address_add_binding(uint32_t device_id,
    unsigned max_apdu, BACNET_ADDRESS * src)
{
    unsigned i;
    bool found = false;         /* return value */

    /* existing device - update address */
    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        if (Address_Cache[i].valid &&
            (Address_Cache[i].device_id == device_id)) {
            bacnet_address_copy(&Address_Cache[i].address, src);
            Address_Cache[i].max_apdu = max_apdu;
            found = true;
            break;
        }
    }
    /* add new device - but only if bind requested */
    if (!found) {
        for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
            if (!Address_Cache[i].valid && Address_Cache[i].bind_request) {
                Address_Cache[i].valid = true;
                Address_Cache[i].bind_request = false;
                Address_Cache[i].device_id = device_id;
                Address_Cache[i].max_apdu = max_apdu;
                bacnet_address_copy(&Address_Cache[i].address, src);
                break;
            }
        }
    }

    return;
}

bool address_get_by_index(unsigned index,
    uint32_t * device_id, unsigned *max_apdu, BACNET_ADDRESS * src)
{
    bool found = false;         /* return value */

    if (index < MAX_ADDRESS_CACHE) {
        if (Address_Cache[index].valid) {
            bacnet_address_copy(src, &Address_Cache[index].address);
            *device_id = Address_Cache[index].device_id;
            *max_apdu = Address_Cache[index].max_apdu;
            found = true;
        }
    }

    return found;
}

unsigned address_count(void)
{
    unsigned i;
    unsigned count = 0;         /* return value */

    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        if (Address_Cache[i].valid)
            count++;
    }

    return count;
}

bool address_match(BACNET_ADDRESS * dest, BACNET_ADDRESS * src)
{
    unsigned i;
    unsigned max_len;
    bool match = true;          /* return value */

    if (dest->mac_len != src->mac_len)
        match = false;
    max_len = dest->mac_len;
    if (max_len > MAX_MAC_LEN)
        max_len = MAX_MAC_LEN;
    for (i = 0; i < max_len; i++) {
        if (dest->mac[i] != src->mac[i])
            match = false;
    }
    if (dest->net != src->net)
        match = false;
    if (dest->len != src->len)
        match = false;
    max_len = dest->len;
    if (max_len > MAX_MAC_LEN)
        max_len = MAX_MAC_LEN;
    for (i = 0; i < max_len; i++) {
        if (dest->adr[i] != src->adr[i])
            match = false;
    }

    return match;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

static void set_address(unsigned index, BACNET_ADDRESS * dest)
{
    unsigned i;

    for (i = 0; i < MAX_MAC_LEN; i++) {
        dest->mac[i] = index;
    }
    dest->mac_len = MAX_MAC_LEN;
    dest->net = 7;
    dest->len = MAX_MAC_LEN;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        dest->adr[i] = index;
    }
}

void testAddress(Test * pTest)
{
    unsigned i, count;
    BACNET_ADDRESS src;
    uint32_t device_id = 0;
    unsigned max_apdu = 480;
    BACNET_ADDRESS test_address;
    uint32_t test_device_id = 0;
    unsigned test_max_apdu = 0;

    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        set_address(i, &src);
        device_id = i * 255;
        address_add(device_id, max_apdu, &src);
        count = address_count();
        ct_test(pTest, count == (i + 1));
    }

    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        device_id = i * 255;
        ct_test(pTest, address_get_by_device(device_id, &test_max_apdu,
                &test_address));
        set_address(i, &src);
        ct_test(pTest, test_max_apdu == max_apdu);
        ct_test(pTest, address_match(&test_address, &src));
        ct_test(pTest, address_get_by_index(i, &test_device_id,
                &test_max_apdu, &test_address));
        ct_test(pTest, test_device_id == device_id);
        ct_test(pTest, test_max_apdu == max_apdu);
        ct_test(pTest, address_match(&test_address, &src));
        ct_test(pTest, address_count() == MAX_ADDRESS_CACHE);
    }

    for (i = 0; i < MAX_ADDRESS_CACHE; i++) {
        device_id = i * 255;
        address_remove_device(device_id);
        ct_test(pTest, !address_get_by_device(device_id, &test_max_apdu,
                &test_address));
        count = address_count();
        ct_test(pTest, count == (MAX_ADDRESS_CACHE - i - 1));
    }
}

#ifdef TEST_ADDRESS
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Address", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testAddress);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_ADDRESS */
#endif                          /* TEST */
