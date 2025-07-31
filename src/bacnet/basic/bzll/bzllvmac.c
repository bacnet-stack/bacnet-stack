/**
 * @file
 * @brief Virtual MAC (VMAC) for BACnet ZigBee Link Layer
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2025
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "bacnet/bacdef.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "bacnet/basic/bzll/bzllvmac.h"

/* enable debugging */
static bool VMAC_Debug = false;

/**
 * @brief Enable debugging if print is enabled
 */
void BZLL_VMAC_Debug_Enable(void)
{
    VMAC_Debug = true;
}

/** @file
    Handle VMAC address binding */

/* This module is used to handle the virtual MAC address binding that */
/* occurs in BACnet for ZigBee or IPv6. */

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist VMAC_List;

/**
 * Returns the number of VMAC in the list
 */
unsigned int BZLL_VMAC_Count(void)
{
    return (unsigned int)Keylist_Count(VMAC_List);
}

/**
 * Adds a VMAC to the list
 *
 * @param device_id - BACnet device object instance number
 * @param vmac - BACnet ZigBee Link Layer address
 *
 * @return true if the device ID and MAC are added
 */
bool BZLL_VMAC_Add(uint32_t device_id, const struct bzll_vmac_data *vmac)
{
    bool status = false;
    struct bzll_vmac_data *list_vmac = NULL;
    uint32_t list_device_id = 0;
    int index = 0;
    size_t i = 0;
    bool found = false;

    if (BZLL_VMAC_Entry_To_Device_ID(vmac, &list_device_id)) {
        if (list_device_id == device_id) {
            /* valid VMAC entry exists. */
            found = true;
            status = true;
        } else {
            /* VMAC exists, but device ID changed */
            BZLL_VMAC_Delete(list_device_id);
        }
    }
    if (!found) {
        list_vmac = Keylist_Data(VMAC_List, device_id);
        if (list_vmac) {
            /* device ID already exists. Update MAC. */
            memmove(list_vmac, &vmac, sizeof(struct bzll_vmac_data));
            found = true;
            status = true;
        }
    }
    if (!found) {
        /* new entry - add it! */
        list_vmac = calloc(1, sizeof(struct bzll_vmac_data));
        if (list_vmac) {
            /* copy the MAC into the data store */
            for (i = 0; i < sizeof(list_vmac->mac); i++) {
                list_vmac->mac[i] = vmac->mac[i];
            }
            list_vmac->endpoint = vmac->endpoint;
            index = Keylist_Data_Add(VMAC_List, device_id, list_vmac);
            if (index >= 0) {
                status = true;
                if (VMAC_Debug) {
                    debug_fprintf(
                        stderr, "BZLL VMAC %u added.\n",
                        (unsigned int)device_id);
                }
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
bool BZLL_VMAC_Delete(uint32_t device_id)
{
    bool status = false;
    struct bzll_vmac_data *pVMAC;

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
bool BZLL_VMAC_Entry_By_Device_ID(
    uint32_t device_id, struct bzll_vmac_data *vmac)
{
    struct bzll_vmac_data *data = Keylist_Data(VMAC_List, device_id);
    if (data && vmac) {
        memcpy(vmac, data, sizeof(struct bzll_vmac_data));
        return true;
    }
    return false;
}

/**
 * Finds a VMAC in the list by seeking the list index
 *
 * @param index - Index that shall be returned
 * @param device_id - BACnet device object instance number
 *
 * @return true if the device_id and vmac are found
 */
bool BZLL_VMAC_Entry_By_Index(
    int index, uint32_t *device_id, struct bzll_vmac_data *vmac)
{
    bool status = false;
    struct bzll_vmac_data *data;
    KEY key = 0;

    data = Keylist_Data_Index(VMAC_List, index);
    if (data) {
        /* virtual MAC is the Device ID */
        status = Keylist_Index_Key(VMAC_List, index, &key);
        if (status) {
            if (device_id) {
                *device_id = key;
            }
            if (vmac) {
                memcpy(vmac, data, sizeof(struct bzll_vmac_data));
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
bool BZLL_VMAC_Same(
    const struct bzll_vmac_data *vmac1, const struct bzll_vmac_data *vmac2)
{
    bool status = false;

    if (vmac1 && vmac2) {
        if (memcmp(vmac1->mac, vmac2->mac, BZLL_VMAC_EUI64) == 0 &&
            vmac1->endpoint == vmac2->endpoint) {
            status = true;
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
bool BZLL_VMAC_Entry_To_Device_ID(
    const struct bzll_vmac_data *vmac, uint32_t *device_id)
{
    bool status = false;
    struct bzll_vmac_data *list_vmac;
    int count = 0;
    int index = 0;

    if (!vmac) {
        return false; /* invalid parameter */
    }
    count = Keylist_Count(VMAC_List);
    while (count) {
        index = count - 1;
        list_vmac = Keylist_Data_Index(VMAC_List, index);
        if (list_vmac) {
            if (BZLL_VMAC_Same(vmac, list_vmac)) {
                status = Keylist_Index_Key(VMAC_List, index, device_id);
                break;
            }
        }
        count--;
    }

    return status;
}

/**
 * Copies the VMAC address into the provided MAC and endpoint
 *
 * @param vmac - VMAC address that will be copied
 * @param mac - pointer to the MAC array to copy into
 * @param endpoint - pointer to the endpoint to copy into
 *
 * @return true if the VMAC address was copied
 */
bool BZLL_VMAC_Entry_Set(
    struct bzll_vmac_data *vmac, const uint8_t *mac, uint8_t endpoint)
{
    bool status = false;
    unsigned int i;

    if (vmac && mac) {
        for (i = 0; i < BZLL_VMAC_EUI64; i++) {
            vmac->mac[i] = mac[i]; /* copy the MAC */
        }
        vmac->endpoint = endpoint;
    }

    return status;
}

/**
 * Cleans up the memory used by the VMAC list data
 */
void BZLL_VMAC_Cleanup(void)
{
    struct bzll_vmac_data *pVMAC;
    const int index = 0;
    unsigned i = 0;

    if (VMAC_List) {
        do {
            uint32_t device_id;
            if (VMAC_Debug) {
                Keylist_Index_Key(VMAC_List, index, &device_id);
            }
            pVMAC = Keylist_Data_Delete_By_Index(VMAC_List, index);
            if (pVMAC) {
                if (VMAC_Debug) {
                    debug_fprintf(
                        stderr, "BZLL VMAC List: %lu [",
                        (unsigned long)device_id);
                    /* print the MAC */
                    for (i = 0; i < BZLL_VMAC_EUI64; i++) {
                        debug_fprintf(stderr, "%02X", pVMAC->mac[i]);
                    }
                    debug_fprintf(stderr, "]\n");
                }
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
void BZLL_VMAC_Init(void)
{
    VMAC_List = Keylist_Create();
    if (VMAC_List) {
        atexit(BZLL_VMAC_Cleanup);
        debug_fprintf(stderr, "BZLL VMAC List initialized.\n");
    }
}
