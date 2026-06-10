/**
 * @file
 * @brief Virtual MAC (VMAC) for BACnet ZigBee Link Layer
 * @author Luiz Santana <luiz.santana@dsr-corporation.com>
 * @date April 2026
 * @copyright SPDX-License-Identifier: MIT
 */

#include <stdio.h> /* for standard i/o, like printing */
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <string.h> /* for memcpy */
#include <assert.h>
#include <string.h>
#include "bacnet/bacaddr.h"
#include "bacnet/basic/bzll/bzllvmac.h"
/* support file*/
#include "support.h"
/* me! */
#include "test_bzllvmac.h"

static struct device_info_t TD;
static struct device_info_t IUT;

static void test_Execute_Virtual_Address_Resolution(void);
static void test_Execute_Generate_Random_Address(void);
static void test_Debug_Enable(void);
static void test_Node_Status(void);

/**
 * @brief Array with all the tests.
 */
static test_suite_t test_suite[] = {
    TEST_ENTRY(test_Debug_Enable),
    TEST_ENTRY(test_Execute_Virtual_Address_Resolution),
    TEST_ENTRY(test_Execute_Generate_Random_Address),
    TEST_ENTRY(test_Node_Status),
};

/**
 * @brief Test setup function
 */
static void test_setup(void)
{
    uint8_t td_mac[BZLL_VMAC_EUI64] = { 0x00, 0x12, 0x34, 0x56,
                                        0x78, 0x9A, 0xBC, 0xDE };
    uint8_t td_endpoint = 0x01;
    uint8_t iut_mac[BZLL_VMAC_EUI64] = { 0x00, 0x12, 0x34, 0x56,
                                         0x78, 0x9A, 0xBC, 0xDF };
    uint8_t iut_endpoint = 0x02;

    support_set_device_info(&IUT);

    BZLL_VMAC_Init();
    TD.Device_ID = 12345;
    bacnet_vmac_address_set(&TD.BACnet_Address, TD.Device_ID);
    BZLL_VMAC_Entry_Set(&TD.VMAC_Data, td_mac, td_endpoint);
    IUT.Device_ID = 67890;
    bacnet_vmac_address_set(&IUT.BACnet_Address, IUT.Device_ID);
    BZLL_VMAC_Entry_Set(&IUT.VMAC_Data, iut_mac, iut_endpoint);
}

/**
 * @brief Cleanup function to free resources
 */
static void test_cleanup(void)
{
    BZLL_VMAC_Cleanup();
    support_set_device_info(NULL);
}

/**
 * @brief Test function to execute the virtual address resolution
 * and verify the functionality of the BZLL VMAC handling.
 *
 * This function will test adding, retrieving, and comparing VMAC entries.
 * It will also check the behavior when changing device IDs.
 */
static void test_Execute_Virtual_Address_Resolution(void)
{
    uint32_t test_vmac_src = 0;
    uint32_t test_device_id = 0;
    uint32_t old_device_id = 0;
    struct bzll_vmac_data test_vmac_data = { 0 };
    BACNET_ADDRESS test_addr = { 0 };
    unsigned int count = 0;
    int index = 0;
    bool status = false;

    /* Add Vmac*/
    status = BZLL_VMAC_Add(&TD.Device_ID, &TD.VMAC_Data);
    assert(status == true);
    status = BZLL_VMAC_Entry_By_Device_ID(TD.Device_ID, &test_vmac_data);
    assert(status == true);
    status = BZLL_VMAC_Same(&TD.VMAC_Data, &test_vmac_data);
    assert(status == true);

    /* change Device ID */
    old_device_id = TD.Device_ID;
    TD.Device_ID += 42;
    status = BZLL_VMAC_Add(&TD.Device_ID, &TD.VMAC_Data);
    assert(status == true);
    count = BZLL_VMAC_Count();
    assert(count == 1);
    status = BZLL_VMAC_Entry_By_Device_ID(TD.Device_ID, &test_vmac_data);
    assert(status == true);
    status = BZLL_VMAC_Entry_By_Device_ID(old_device_id, &test_vmac_data);
    assert(status == false);
    status = BZLL_VMAC_Entry_By_Index(0, &test_device_id, &test_vmac_data);
    assert(status == true);
    assert(test_device_id == TD.Device_ID);
    status = BZLL_VMAC_Same(&TD.VMAC_Data, &test_vmac_data);
    assert(status == true);
    status = BZLL_VMAC_Add(&IUT.Device_ID, &IUT.VMAC_Data);
    assert(status == true);
    count = BZLL_VMAC_Count();
    assert(count == 2);
    for (index = 0; index < count; index++) {
        status = BZLL_VMAC_Entry_By_Index(index, &test_vmac_src, NULL);
        assert(status == true);
        status = BZLL_VMAC_Entry_By_Device_ID(test_vmac_src, &test_vmac_data);
        assert(status == true);
    }

    /* Delete Vmac */
    status = BZLL_VMAC_Delete(IUT.Device_ID);
    assert(status == true);
    count = BZLL_VMAC_Count();
    assert(count == 1);
    memset(&test_vmac_data, 0, sizeof(test_vmac_data));
    status = BZLL_VMAC_Copy(&TD.VMAC_Data, &test_vmac_data);
    assert(status == true);
    status = BZLL_VMAC_Same(&TD.VMAC_Data, &test_vmac_data);
    assert(status == true);

    /* BACnet Address Handling */
    bacnet_vmac_address_set(&TD.BACnet_Address, TD.Device_ID);
    status = BZLL_VMAC_Device_ID_To_Address(&test_addr, TD.Device_ID);
    assert(status == true);
    status = bacnet_address_same(&test_addr, &TD.BACnet_Address);
    assert(status == true);
    test_device_id = 0;
    status = BZLL_VMAC_Device_ID_From_Address(&test_addr, &test_device_id);
    assert(status == true);
    assert(test_device_id == TD.Device_ID);
}

/**
 * @brief Test random address generation
 */
static void test_Execute_Generate_Random_Address(void)
{
    struct bzll_vmac_data test_vmac_data = { 0 };
    uint32_t test_device_id = BACNET_MAX_INSTANCE;
    bool status = false;
    unsigned int count = 0;

    TD.Device_ID = BACNET_MAX_INSTANCE;
    status = BZLL_VMAC_Add(&TD.Device_ID, &TD.VMAC_Data);
    assert(status == true);
    assert(TD.Device_ID != BACNET_MAX_INSTANCE);
    assert(TD.Device_ID >= 0x100000);
    assert(TD.Device_ID <= 0x7FFFFF);
    count = BZLL_VMAC_Count();
    assert(count == 1);
    status = BZLL_VMAC_Entry_By_Device_ID(TD.Device_ID, &test_vmac_data);
    assert(status == true);
    status = BZLL_VMAC_Same(&TD.VMAC_Data, &test_vmac_data);
    assert(status == true);
    status = BZLL_VMAC_Entry_To_Device_ID(&TD.VMAC_Data, &test_device_id);
    assert(status == true);
    assert(TD.Device_ID == test_device_id);

    IUT.Device_ID = BACNET_MAX_INSTANCE;
    status = BZLL_VMAC_Add(&IUT.Device_ID, &IUT.VMAC_Data);
    assert(status == true);
    assert(IUT.Device_ID != BACNET_MAX_INSTANCE);
    assert(IUT.Device_ID >= 0x100000);
    assert(IUT.Device_ID <= 0x7FFFFF);
    count = BZLL_VMAC_Count();
    assert(count == 2);
    status = BZLL_VMAC_Entry_By_Device_ID(IUT.Device_ID, &test_vmac_data);
    assert(status == true);
    status = BZLL_VMAC_Same(&IUT.VMAC_Data, &test_vmac_data);
    assert(status == true);
    assert(TD.Device_ID != IUT.Device_ID);
    status = BZLL_VMAC_Entry_To_Device_ID(&IUT.VMAC_Data, &test_device_id);
    assert(status == true);
    assert(IUT.Device_ID == test_device_id);
}

/**
 * @brief Test debug enable
 */
static void test_Debug_Enable(void)
{
    bool status = false;
    BZLL_VMAC_Debug_Enable();
    status = BZLL_VMAC_Debug_Enabled();
    assert(status == true);
}

/**
 * @brief Test the Node Status handling
 */
static void test_Node_Status(void)
{
    bool status = false;
    struct bzll_node_status_table_entry *status_entry = NULL;

    /* No VMAC */
    status = BZLL_VMAC_Node_Status_Entry(&TD.VMAC_Data, &status_entry);
    assert(status == false);
    assert(status_entry == NULL);
    status = BZLL_VMAC_Node_Status_By_Index_Entry(0, &status_entry);
    assert(status == false);
    assert(status_entry == NULL);

    /* VMAC Added */
    BZLL_VMAC_Add(&TD.Device_ID, &TD.VMAC_Data);
    status = BZLL_VMAC_Node_Status_Entry(&TD.VMAC_Data, &status_entry);
    assert(status == true);
    assert(status_entry != NULL);
    status = BZLL_VMAC_Node_Status_By_Index_Entry(0, &status_entry);
    assert(status == true);
    assert(status_entry != NULL);

    /* Change Node Status */
    BZLL_VMAC_Add(&IUT.Device_ID, &IUT.VMAC_Data);
    status = BZLL_VMAC_Node_Status_By_Index_Entry(0, &status_entry);
    status_entry->valid = true;
    status_entry->ttl_seconds_remaining = 50;
    status = BZLL_VMAC_Node_Status_By_Index_Entry(1, &status_entry);
    status_entry->valid = false;
    status_entry->ttl_seconds_remaining = 70;

    /* Retrieve Node Status */
    status = BZLL_VMAC_Node_Status_Entry(&TD.VMAC_Data, &status_entry);
    assert(status == true);
    assert(status_entry->valid == true);
    assert(status_entry->ttl_seconds_remaining == 50);
    status = BZLL_VMAC_Node_Status_Entry(&IUT.VMAC_Data, &status_entry);
    assert(status == true);
    assert(status_entry->valid == false);
    assert(status_entry->ttl_seconds_remaining == 70);
}

/**
 * @brief Test vmac bzll wrapper
 */
void test_bzllvmac(void)
{
    RUN_TESTS(test_suite, test_setup, test_cleanup);
}
