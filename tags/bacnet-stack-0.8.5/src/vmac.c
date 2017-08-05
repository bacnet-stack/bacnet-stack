/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2015 Steve Karg

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
#include <stdlib.h>
#include "config.h"
#include "bacdef.h"
#include "keylist.h"
/* me! */
#include "vmac.h"

/** @file
    Handle VMAC address binding */

/* This module is used to handle the virtual MAC address binding that */
/* occurs in BACnet for ZigBee or IPv6. */

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist VMAC_List;

/**
 * Returns the number of VMAC in the list
 */
unsigned int VMAC_Count(void)
{
    return (unsigned int)Keylist_Count(VMAC_List);
}

/**
 * Adds a VMAC to the list
 *
 * @param device_id - BACnet device object instance number
 * @param src - BACnet/IPv6 address
 *
 * @return true if the device ID and MAC are added
 */
bool VMAC_Add(uint32_t device_id, struct vmac_data *src)
{
    bool status = false;
    struct vmac_data *pVMAC = NULL;
    int index = 0;
    size_t i = 0;

    pVMAC = Keylist_Data(VMAC_List, device_id);
    if (!pVMAC) {
        pVMAC = calloc(1, sizeof(struct vmac_data));
        if (pVMAC) {
            /* copy the MAC into the data store */
            for (i = 0; i < sizeof(pVMAC->mac); i++) {
                if (i < src->mac_len) {
                    pVMAC->mac[i] = src->mac[i];
                } else {
                    break;
                }
            }
            pVMAC->mac_len = src->mac_len;
            index = Keylist_Data_Add(VMAC_List, device_id, pVMAC);
            if (index >= 0) {
                status = true;
                printf("VMAC %u added.\n", (unsigned int)device_id);
            }
        }
    }

    return status;
}

/**
 * Finds a VMAC in the list by seeking the Device ID, and deletes it.
 *
 * @param device_id - BACnet device object instance number
 *
 * @return pointer to the VMAC data from the list - be sure to free() it!
 */
bool VMAC_Delete(uint32_t device_id)
{
    bool status = false;
    struct vmac_data *pVMAC;

    pVMAC = Keylist_Data_Delete(VMAC_List, device_id);
    if (pVMAC) {
        free(pVMAC);
        status = true;
    }

    return status;
}

/**
 * Finds a VMAC in the list by seeking the Device ID.
 *
 * @param device_id - BACnet device object instance number
 *
 * @return pointer to the VMAC data from the list
 */
struct vmac_data *VMAC_Find_By_Key(uint32_t device_id)
{
    return Keylist_Data(VMAC_List, device_id);
}

/** Compare the VMAC address
 *
 * @param vmac1 - VMAC address that will be compared to vmac2
 * @param vmac2 - VMAC address that will be compared to vmac1
 *
 * @return true if the addresses are different
 */
bool VMAC_Different(
    struct vmac_data *vmac1,
    struct vmac_data *vmac2)
{
    bool status = false;
    unsigned int i = 0;
    unsigned int mac_len = VMAC_MAC_MAX;

    if (vmac1 && vmac2) {
        if (vmac1->mac_len != vmac2->mac_len) {
            status = true;
        } else {
            if (vmac1->mac_len < mac_len) {
                mac_len = (unsigned int)vmac1->mac_len;
            }
            for (i = 0; i < mac_len; i++) {
                if (vmac1->mac[i] != vmac2->mac[i]) {
                    status = true;
                }
            }
        }
    }

    return status;
}

/** Compare the VMAC address
 *
 * @param vmac1 - VMAC address that will be compared to vmac2
 * @param vmac2 - VMAC address that will be compared to vmac1
 *
 * @return true if the addresses are the same
 */
bool VMAC_Match(
    struct vmac_data *vmac1,
    struct vmac_data *vmac2)
{
    bool status = false;
    unsigned int i = 0;
    unsigned int mac_len = VMAC_MAC_MAX;

    if (vmac1 && vmac2 && vmac1->mac_len) {
        status = true;
        if (vmac1->mac_len != vmac2->mac_len) {
            status = false;
        } else {
            if (vmac1->mac_len < mac_len) {
                mac_len = (unsigned int)vmac1->mac_len;
            }
            for (i = 0; i < mac_len; i++) {
                if (vmac1->mac[i] != vmac2->mac[i]) {
                    status = false;
                }
            }
        }
    }

    return status;
}

/**
 * Finds a VMAC in the list by seeking a matching VMAC address
 *
 * @param vmac - VMAC address that will be sought
 * @param device_id - BACnet device object instance number
 *
 * @return true if the VMAC address was found
 */
bool VMAC_Find_By_Data(struct vmac_data *vmac, uint32_t *device_id)
{
    bool status = false;
    struct vmac_data *list_vmac;
    int count = 0;
    int index = 0;

    count = Keylist_Count(VMAC_List);
    while (count) {
        index = count - 1;
        list_vmac = Keylist_Data_Index(VMAC_List, index);
        if (list_vmac) {
            if (VMAC_Match(vmac, list_vmac)) {
                if (device_id) {
                    *device_id = Keylist_Key(VMAC_List, index);
                }
                status = true;
                break;
            }
        }
        count--;
    }

    return status;
}

/**
 * Cleans up the memory used by the VMAC list data
 */
void VMAC_Cleanup(void)
{
    struct vmac_data *pVMAC;

    if (VMAC_List) {
        do {
            pVMAC = Keylist_Data_Pop(VMAC_List);
            if (pVMAC) {
                free(pVMAC);
            }
        } while (pVMAC);
        Keylist_Delete(VMAC_List);
        VMAC_List = NULL;
    }
}

/**
 * Initializes the VMAC list data
 */
void VMAC_Init(void)
{
    VMAC_List = Keylist_Create();
    if (VMAC_List) {
        atexit(VMAC_Cleanup);
        printf("VMAC List initialized.\n");
    }
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testVMAC(
    Test * pTest)
{
    uint32_t device_id = 123;
    uint32_t test_device_id = 0;
    struct vmac_data test_vmac_data;
    struct vmac_data *pVMAC;
    unsigned int i = 0;
    bool status = false;

    VMAC_Init();
    for (i = 0; i < VMAC_MAC_MAX; i++) {
        test_vmac_data.mac[i] = 1 + i;
    }
    test_vmac_data.mac_len = VMAC_MAC_MAX;
    status = VMAC_Add(device_id, &test_vmac_data);
    ct_test(pTest, status);
    pVMAC = VMAC_Find_By_Key(0);
    ct_test(pTest, pVMAC == NULL);
    pVMAC = VMAC_Find_By_Key(device_id);
    ct_test(pTest, pVMAC);
    status = VMAC_Different(pVMAC, &test_vmac_data);
    ct_test(pTest, !status);
    status = VMAC_Match(pVMAC, &test_vmac_data);
    ct_test(pTest, status);
    status = VMAC_Find_By_Data(&test_vmac_data, &test_device_id);
    ct_test(pTest, status);
    ct_test(pTest, test_device_id == device_id);
    status = VMAC_Delete(device_id);
    ct_test(pTest, status);
    pVMAC = VMAC_Find_By_Key(device_id);
    ct_test(pTest, pVMAC == NULL);
    VMAC_Cleanup();
}

#ifdef TEST_VMAC
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet VMAC", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testVMAC);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif
#endif
