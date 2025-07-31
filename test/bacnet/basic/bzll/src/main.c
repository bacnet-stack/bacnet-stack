/**
 * @file
 * @brief Test file for a basic BACnet Zigbee Link Layer (BZLL)
 * @author Steve Karg
 * @date July 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h> /* for standard i/o, like printing */
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <string.h> /* for memcpy */
#include <assert.h>
#include <string.h>
#include "bacnet/bacaddr.h"
#include "bacnet/bacdcode.h"
#include "bacnet/iam.h"
#include "bacnet/npdu.h"
#include "bacnet/datalink/bzll.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/bzll/bzllvmac.h"

struct device_info_t {
    uint32_t Device_ID;
    /* MAC Address shall be a ZigBee EUI64 and BACnet endpoint */
    struct bzll_vmac_data VMAC_Data;
    BACNET_ADDRESS BACnet_Address;
};
static struct device_info_t TD;
static struct device_info_t IUT;

/* network stub functions */
/**
 * BACnet/IP Datalink Receive handler.
 *
 * @param src - returns the source address
 * @param npdu - returns the NPDU buffer
 * @param max_npdu -maximum size of the NPDU buffer
 * @param timeout - number of milliseconds to wait for a packet
 *
 * @return Number of bytes received, or 0 if none or timeout.
 */
uint16_t bzll_receive(
    BACNET_ADDRESS *src, uint8_t *npdu, uint16_t max_npdu, unsigned timeout)
{
    (void)src;
    (void)npdu;
    (void)max_npdu;
    (void)timeout;

    return 0;
}

/**
 * The send function for BACnet/IPv6 driver layer
 *
 * @param dest - Points to a BACNET_IP6_ADDRESS structure containing the
 *  destination address.
 * @param mtu - the bytes of data to send
 * @param mtu_len - the number of bytes of data to send
 *
 * @return Upon successful completion, returns the number of bytes sent.
 *  Otherwise, -1 shall be returned to indicate the error.
 */
int bzll_send_mpdu(
    const BACNET_IP6_ADDRESS *dest, const uint8_t *mtu, uint16_t mtu_len)
{
    (void)dest;
    (void)mtu;
    (void)mtu_len;
    return 0;
}

/** Return the Object Instance number for our (single) Device Object.
 * This is a key function, widely invoked by the handler code, since
 * it provides "our" (ie, local) address.
 *
 * @return The Instance number used in the BACNET_OBJECT_ID for the Device.
 */
uint32_t Device_Object_Instance_Number(void)
{
    return IUT.Device_ID;
}

/**
 * Test setup function
 */
static void test_setup(void)
{
    uint8_t td_mac[BZLL_VMAC_EUI64] = { 0x00, 0x12, 0x34, 0x56,
                                        0x78, 0x9A, 0xBC, 0xDE };
    uint8_t td_endpoint = 0x01;
    uint8_t iut_mac[BZLL_VMAC_EUI64] = { 0x00, 0x12, 0x34, 0x56,
                                         0x78, 0x9A, 0xBC, 0xDF };
    uint8_t iut_endpoint = 0x02;

    BZLL_VMAC_Init();
    TD.Device_ID = 12345;
    bacnet_vmac_address_set(&TD.BACnet_Address, TD.Device_ID);
    BZLL_VMAC_Entry_Set(&TD.VMAC_Data, td_mac, td_endpoint);
    IUT.Device_ID = 67890;
    bacnet_vmac_address_set(&IUT.BACnet_Address, IUT.Device_ID);
    BZLL_VMAC_Entry_Set(&IUT.VMAC_Data, iut_mac, iut_endpoint);
}

/**
 * Cleanup function to free resources
 */
static void test_cleanup(void)
{
    BZLL_VMAC_Cleanup();
}

/**
 * Test function to execute the virtual address resolution
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
    unsigned int count = 0;
    int index = 0;
    bool status = false;

    test_setup();
    status = BZLL_VMAC_Add(TD.Device_ID, &TD.VMAC_Data);
    assert(status == true);
    status = BZLL_VMAC_Entry_By_Device_ID(TD.Device_ID, &test_vmac_data);
    assert(status == true);
    status = BZLL_VMAC_Same(&TD.VMAC_Data, &test_vmac_data);
    assert(status == true);
    /* change Device ID */
    old_device_id = TD.Device_ID;
    TD.Device_ID += 42;
    status = BZLL_VMAC_Add(TD.Device_ID, &TD.VMAC_Data);
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
    status = BZLL_VMAC_Add(IUT.Device_ID, &IUT.VMAC_Data);
    assert(status == true);
    count = BZLL_VMAC_Count();
    for (index = 0; index < count; index++) {
        status = BZLL_VMAC_Entry_By_Index(index, &test_vmac_src, NULL);
        assert(status == true);
        status = BZLL_VMAC_Entry_By_Device_ID(test_vmac_src, &test_vmac_data);
        assert(status == true);
    }
    test_cleanup();
}

/**
 * Main function to execute the test
 *
 * @return 0 on success, non-zero on failure
 */
int main(void)
{
    test_Execute_Virtual_Address_Resolution();

    return 0;
}
