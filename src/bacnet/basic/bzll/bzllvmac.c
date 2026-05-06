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
#include "bacnet/bacint.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/keylist.h"
/* me! */
#include "bacnet/basic/bzll/bzllvmac.h"

/**
 * @brief Data structure to be stored in the list.
 * Keep the vmac data and the node status together
 */
struct bzll_node_table_entry {
    struct bzll_vmac_data vmac_data;
    struct bzll_node_status_table_entry node_status;
};

/* enable debugging */
static bool VMAC_Debug = false;

/**
 * @brief Enable debugging if print is enabled
 */
void BZLL_VMAC_Debug_Enable(void)
{
    VMAC_Debug = true;
}

/**
 * @brief Determine if debugging is enabled
 */
bool BZLL_VMAC_Debug_Enabled(void)
{
    return VMAC_Debug;
}

/** @file
    Handle VMAC address binding */

/* This module is used to handle the virtual MAC address binding that */
/* occurs in BACnet for ZigBee or IPv6. */

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist VMAC_List;

/**
 * @brief Returns the number of VMAC in the list
 *
 * @return Number of VMAC in the list
 */
unsigned int BZLL_VMAC_Count(void)
{
    return (unsigned int)Keylist_Count(VMAC_List);
}

/**
 * @brief Generate a random VMAC address
 *
 * H.7.2 Using Device Instance as a VMAC Address
 *
 * The random portion of a random instance VMAC address
 * is a number in the range 0 to 4194303.
 * The resulting random instance VMAC address
 * is in the range 4194304 to 8388607 (X'100000' to X'7FFFFF ').
 * The generation of a random instance VMAC shall yield
 * any number in the entire range with equal probability.
 * A random instance VMAC is formatted as follows:
 *
 * Bit Number:
 *   7   6   5   4   3   2   1   0
 * |---|---|---|---|---|---|---|---|
 * | 0 | 1 |   High 6 Bits         |
 * |---|---|---|---|---|---|---|---|
 * |           Middle Octet        |
 * |---|---|---|---|---|---|---|---|
 * |           Low Octet           |
 * |---|---|---|---|---|---|---|---|
 *
 * @return A random generated instance.
 */
static uint32_t bzll_random_instance_vmac(void)
{
    uint32_t random_instance;

    /* The random portion of a random instance VMAC address
       is a number in the range 0 to 4194303. */
    random_instance = (uint32_t)(rand() % 4194304UL) + 4194304UL;

    return random_instance;
}

/**
 * @brief Adds a VMAC to the list
 *
 * @param device_id [in] - BACnet device object instance number.
 * If the device_id is equal to #BACNET_MAX_INSTANCE or NULL, a random instance
 * will be generated. #bzll_random_instance_vmac.
 * @param vmac [in] - BACnet ZigBee Link Layer address
 *
 * @return true if the device ID and MAC are added
 */
bool BZLL_VMAC_Add(uint32_t *device_id, const struct bzll_vmac_data *vmac)
{
    bool status = false;
    struct bzll_vmac_data *list_vmac = NULL;
    uint32_t list_device_id = 0;
    int index = 0;
    size_t i = 0;
    bool found = false;
    uint32_t dev_id = BACNET_MAX_INSTANCE;

    if (device_id) {
        dev_id = *device_id;
    }

    if (dev_id == BACNET_MAX_INSTANCE) {
        do {
            /* generate a new random instance VMAC address */
            dev_id = bzll_random_instance_vmac();
            /* check if the generated device ID is already in use */
        } while (BZLL_VMAC_Entry_By_Device_ID(dev_id, NULL));
        if (device_id) {
            *device_id = dev_id;
        }
    }

    if (BZLL_VMAC_Entry_To_Device_ID(vmac, &list_device_id)) {
        if (list_device_id == dev_id) {
            /* valid VMAC entry exists. */
            found = true;
            status = true;
        } else {
            /* VMAC exists, but device ID changed */
            BZLL_VMAC_Delete(list_device_id);
        }
    }
    if (!found) {
        list_vmac = Keylist_Data(VMAC_List, dev_id);
        if (list_vmac) {
            /* device ID already exists. Update MAC. */
            memmove(list_vmac, vmac, sizeof(struct bzll_node_table_entry));
            found = true;
            status = true;
        }
    }
    if (!found) {
        /* new entry - add it! */
        list_vmac = calloc(1, sizeof(struct bzll_node_table_entry));
        if (list_vmac) {
            /* copy the MAC into the data store */
            for (i = 0; i < sizeof(list_vmac->mac); i++) {
                list_vmac->mac[i] = vmac->mac[i];
            }
            list_vmac->endpoint = vmac->endpoint;
            index = Keylist_Data_Add(VMAC_List, dev_id, list_vmac);
            if (index >= 0) {
                status = true;
                if (VMAC_Debug) {
                    debug_fprintf(
                        stderr, "BZLL VMAC %u added.\n", (unsigned int)dev_id);
                }
            }
        }
    }

    return status;
}

/**
 * @brief Finds a VMAC in the list by seeking the Device ID, and deletes it.
 *
 * @param device_id [in] - BACnet device object instance number
 *
 * @return true if it is deleted successfully
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
 * @brief Finds a VMAC in the list by seeking the Device ID.
 *
 * @param device_id [in] - BACnet device object instance number
 * @param vmac [out] - the VMAC data from the list
 *
 * @return true if the VMAC data is found
 */
bool BZLL_VMAC_Entry_By_Device_ID(
    uint32_t device_id, struct bzll_vmac_data *vmac)
{
    struct bzll_vmac_data *data = Keylist_Data(VMAC_List, device_id);
    if (data) {
        if (vmac) {
            BZLL_VMAC_Copy(vmac, data);
        }
        return true;
    }
    return false;
}

/**
 * @brief Finds a VMAC in the list by seeking the list index
 *
 * @param index [in] - Index that shall be returned
 * @param device_id [out] - BACnet device object instance number
 * @param vmac [out] - the VMAC data from the list
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
                BZLL_VMAC_Copy(vmac, data);
            }
        }
    }

    return status;
}

/**
 * @brief Compare the VMAC address
 *
 * @param vmac1 [in] - VMAC address that will be compared to vmac2
 * @param vmac2 [in] - VMAC address that will be compared to vmac1
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
 * @brief Copy the VMAC address
 *
 * @param vmac_dest [out] - VMAC address that will be copied to
 * @param vmac_src [in] - VMAC address that will be copied from
 *
 * @return true if the addresses are the same
 */
bool BZLL_VMAC_Copy(
    struct bzll_vmac_data *vmac_dest, const struct bzll_vmac_data *vmac_src)
{
    bool status = false;

    if (vmac_dest && vmac_src) {
        memcpy(vmac_dest->mac, vmac_src->mac, BZLL_VMAC_EUI64);
        vmac_dest->endpoint = vmac_src->endpoint;
        status = true;
    }

    return status;
}

/**
 * @brief Finds a VMAC in the list by seeking a matching VMAC address
 *
 * @param vmac [in] - VMAC address that will be sought
 * @param device_id [out] - BACnet device object instance number
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
 * @brief Copies the VMAC address into the provided MAC and endpoint
 *
 * @param vmac [in] - VMAC address that will be copied
 * @param mac [out] - pointer to the MAC array to copy into
 * @param endpoint [out] - pointer to the endpoint to copy into
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
        status = true;
    }

    return status;
}

/**
 * @brief Convert a BACnet VMAC Address from a Device ID
 *
 * @param addr [out] - BACnet address that be set
 * @param device_id [in] - 22-bit device ID
 *
 * @return true if the address is converted from the device ID
 */
bool BZLL_VMAC_Device_ID_To_Address(BACNET_ADDRESS *addr, uint32_t device_id)
{
    bool status = false;

    if (addr) {
        encode_unsigned24(&addr->mac[0], device_id);
        addr->mac_len = 3;
        addr->net = 0;
        addr->len = 0;
        status = true;
    }

    return status;
}

/**
 * @brief Convert a BACnet VMAC Address to a Device ID
 *
 * @param addr [in] - BACnet address that be set
 * @param device_id [out] - 22-bit device ID
 *
 * @return true if the device is converted from the address
 */
bool BZLL_VMAC_Device_ID_From_Address(
    const BACNET_ADDRESS *addr, uint32_t *device_id)
{
    bool status = false;

    if (addr && device_id) {
        if (addr->mac_len == 3) {
            decode_unsigned24(&addr->mac[0], device_id);
            status = true;
        }
    }

    return status;
}

/**
 * @brief Cleans up the memory used by the VMAC list data
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
 * @brief Initializes the VMAC list data
 */
void BZLL_VMAC_Init(void)
{
    VMAC_List = Keylist_Create();
    if (VMAC_List) {
        atexit(BZLL_VMAC_Cleanup);
        debug_fprintf(stderr, "BZLL VMAC List initialized.\n");
    }
}

/**
 * @brief Retrieves the pointer to the status entry from vmac data.
 *
 * @param vmac [in] - VMAC Address to be found
 * @param status_entry [out] - Pointer to the status entry to be updated
 *
 * @return true if the VMAC is found
 */
bool BZLL_VMAC_Node_Status_Entry(
    const struct bzll_vmac_data *vmac,
    struct bzll_node_status_table_entry **status_entry)
{
    struct bzll_node_table_entry *list_vmac;
    int count = 0;
    int index = 0;

    if ((!vmac) || (!status_entry)) {
        return false; /* invalid parameter */
    }
    *status_entry = NULL;
    count = Keylist_Count(VMAC_List);
    while (count) {
        index = count - 1;
        list_vmac = Keylist_Data_Index(VMAC_List, index);
        if (list_vmac) {
            if (BZLL_VMAC_Same(vmac, &list_vmac->vmac_data)) {
                *status_entry = &list_vmac->node_status;
                return true;
            }
        }
        count--;
    }
    return false;
}

/**
 * @brief Retrieves the pointer to the status entry from index.
 *
 * @param index [in] - index
 * @param status_entry [out] - Pointer to the status entry to be updated
 *
 * @return true if index is found
 */
bool BZLL_VMAC_Node_Status_By_Index_Entry(
    const int index, struct bzll_node_status_table_entry **status_entry)
{
    struct bzll_node_table_entry *list_vmac;

    if (!status_entry) {
        return false; /* invalid parameter */
    }

    list_vmac = Keylist_Data_Index(VMAC_List, index);
    if (list_vmac) {
        *status_entry = &list_vmac->node_status;
        return true;
    }
    return false;
}
