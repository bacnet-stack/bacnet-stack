/**
 * @file
 * @author Steve Karg
 * @date April 2020
 * @brief Test file for a basic BBMD for BVLC IPv6 handler
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h> /* for standard i/o, like printing */
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <string.h> /* for memcpy */
#include <assert.h>
#include <string.h>
#include "bacnet/bacdcode.h"
#include "bacnet/iam.h"
#include "bacnet/npdu.h"
#include "bacnet/datalink/bip6.h"
#include "bacnet/datalink/bvlc6.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/bbmd6/h_bbmd6.h"
#include "bacnet/basic/bbmd6/vmac.h"

struct device_info_t {
    uint32_t Device_ID;
    BACNET_IP6_ADDRESS BIP6_Addr;
    BACNET_IP6_ADDRESS BIP6_Broadcast_Addr;
    BACNET_ADDRESS BACnet_Address;
};
static struct device_info_t TD;
static struct device_info_t IUT;

#ifndef MAX_MPDU
#define MAX_MPDU 1497
#endif

/* for the reply sent from the handler */
static uint8_t Test_Sent_Message_Type;
static uint8_t Test_Sent_Message_Length;
static uint8_t Test_Sent_Message_Buffer[MAX_MPDU];
static uint16_t Test_Sent_Message_Buffer_Length;
static BACNET_IP6_ADDRESS Test_Sent_Message_Dest;

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
uint16_t bip6_receive(
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
int bip6_send_mpdu(
    const BACNET_IP6_ADDRESS *dest, const uint8_t *mtu, uint16_t mtu_len)
{
    uint8_t message_type = 0;
    uint16_t message_length = 0;
    int header_len = 0;

    header_len =
        bvlc6_decode_header(mtu, mtu_len, &message_type, &message_length);
    Test_Sent_Message_Type = message_type;
    Test_Sent_Message_Length = message_length;
    bvlc6_address_copy(&Test_Sent_Message_Dest, dest);
    if ((header_len == 4) && (mtu_len >= 4)) {
        memcpy(&Test_Sent_Message_Buffer[0], &mtu[4], mtu_len - 4);
        Test_Sent_Message_Buffer_Length = mtu_len - 4;
    } else {
        Test_Sent_Message_Buffer_Length = 0;
    }

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
 * Get the BACnet/IP address
 *
 * @return BACnet/IP address
 */
bool bip6_get_addr(BACNET_IP6_ADDRESS *addr)
{
    return bvlc6_address_copy(addr, &IUT.BIP6_Addr);
}

/**
 * Get the BACnet/IP address
 *
 * @return BACnet/IP address
 */
bool bip6_get_broadcast_addr(BACNET_IP6_ADDRESS *addr)
{
    return bvlc6_address_copy(addr, &IUT.BIP6_Broadcast_Addr);
}

static void test_setup(void)
{
    bvlc6_init();
    /* BACnet_IPv6_Multicast_Address is FF02::BAC0 */
    bvlc6_address_set(
        &TD.BIP6_Broadcast_Addr, BIP6_MULTICAST_LINK_LOCAL, 0, 0, 0, 0, 0, 0,
        BIP6_MULTICAST_GROUP_ID);
    bvlc6_address_set(
        &TD.BIP6_Addr, 0x2001, 0x0DBB, 0xAC10, 0xFE01, 0, 0, 0,
        BIP6_MULTICAST_GROUP_ID);
    TD.Device_ID = 12345;
    bvlc6_vmac_address_set(&TD.BACnet_Address, TD.Device_ID);

    /* BACnet_IPv6_Multicast_Address is FF02::BAC0 */
    bvlc6_address_set(
        &IUT.BIP6_Broadcast_Addr, BIP6_MULTICAST_LINK_LOCAL, 0, 0, 0, 0, 0, 0,
        BIP6_MULTICAST_GROUP_ID);
    bvlc6_address_set(
        &IUT.BIP6_Addr, 0x2001, 0x0DBB, 0xAC10, 0xFE01, 0, 0, 1,
        BIP6_MULTICAST_GROUP_ID);
    IUT.Device_ID = 54321;
    bvlc6_vmac_address_set(&IUT.BACnet_Address, IUT.Device_ID);
}

static void test_cleanup(void)
{
    bvlc6_cleanup();
}

/**
 * @brief Test 15.2.1.1 Initiate Original-Broadcast-NPDU
 */
static void test_Initiate_Original_Broadcast_NPDU(void)
{
    uint8_t pdu[MAX_MPDU] = { 0 };
    int npdu_len = 0;
    int apdu_len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS dest = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint8_t test_pdu[MAX_MPDU] = { 0 };
    uint16_t test_pdu_len = 0;
    uint32_t test_vmac_src = 0;
    int function_len = 0;

    test_setup();
    /* MAKE(the IUT send a broadcast) */
    dest.net = BACNET_BROADCAST_NETWORK;
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    npdu_len = npdu_encode_pdu(&pdu[0], &dest, &IUT.BACnet_Address, &npdu_data);
    apdu_len = iam_encode_apdu(
        &pdu[npdu_len], IUT.Device_ID, MAX_APDU, SEGMENTATION_NONE,
        BACNET_VENDOR_ID);
    pdu_len = npdu_len + apdu_len;
    bvlc6_send_pdu(&dest, &npdu_data, pdu, pdu_len);
    /* DA=Link Local Multicast Address */
    assert(!bvlc6_address_different(
        &TD.BIP6_Broadcast_Addr, &Test_Sent_Message_Dest));
    /* SA = IUT - done in port layer */
    /* Original-Broadcast-NPDU */
    assert(Test_Sent_Message_Type == BVLC6_ORIGINAL_BROADCAST_NPDU);
    if (Test_Sent_Message_Type == BVLC6_ORIGINAL_BROADCAST_NPDU) {
        function_len = bvlc6_decode_original_broadcast(
            Test_Sent_Message_Buffer, Test_Sent_Message_Buffer_Length,
            &test_vmac_src, test_pdu, sizeof(test_pdu), &test_pdu_len);
        assert(function_len > 0);
        /* Source-Virtual-Address = IUT */
        assert(test_vmac_src == IUT.Device_ID);
        /* (any valid BACnet-Unconfirmed-Request-PDU,
            with any valid broadcast network options */
        assert(test_pdu_len == pdu_len);
    }
    test_cleanup();
}

/**
 * @brief Test 15.1.2 Execute Virtual-Address-Resolution
 */
static void test_Execute_Virtual_Address_Resolution(void)
{
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;
    uint32_t test_vmac_src = 0;
    uint32_t test_vmac_dst = 0;
    uint32_t old_device_id = 0;
    int result = 0;
    int function_len = 0;

    test_setup();
    mtu_len = bvlc6_encode_virtual_address_resolution(
        &mtu[0], sizeof(mtu), TD.Device_ID);
    result = bvlc6_bbmd_disabled_handler(
        &TD.BIP6_Addr, &TD.BACnet_Address, &mtu[0], mtu_len);
    assert(result == 0);
    assert(bvlc6_get_function_code() == BVLC6_VIRTUAL_ADDRESS_RESOLUTION);
    assert(Test_Sent_Message_Type == BVLC6_VIRTUAL_ADDRESS_RESOLUTION_ACK);
    assert(VMAC_Find_By_Key(TD.Device_ID) != NULL);
    if (Test_Sent_Message_Type == BVLC6_VIRTUAL_ADDRESS_RESOLUTION_ACK) {
        function_len = bvlc6_decode_virtual_address_resolution_ack(
            Test_Sent_Message_Buffer, Test_Sent_Message_Buffer_Length,
            &test_vmac_src, &test_vmac_dst);
        assert(function_len > 0);
        assert(test_vmac_src == IUT.Device_ID);
        assert(test_vmac_dst == TD.Device_ID);
    }
    /* change Device ID */
    old_device_id = TD.Device_ID;
    TD.Device_ID += 42;
    mtu_len = bvlc6_encode_virtual_address_resolution(
        &mtu[0], sizeof(mtu), TD.Device_ID);
    result = bvlc6_bbmd_disabled_handler(
        &TD.BIP6_Addr, &TD.BACnet_Address, &mtu[0], mtu_len);
    assert(result == 0);
    assert(bvlc6_get_function_code() == BVLC6_VIRTUAL_ADDRESS_RESOLUTION);
    assert(VMAC_Find_By_Key(TD.Device_ID) != NULL);
    assert(VMAC_Find_By_Key(old_device_id) == NULL);
    /* change IPv6 address */
    mtu_len = bvlc6_encode_virtual_address_resolution(
        &mtu[0], sizeof(mtu), TD.Device_ID);
    bvlc6_address_set(
        &TD.BIP6_Addr, 0x2001, 0x0DBB, 0xAC10, 0xFE01, 0, 0, 42,
        BIP6_MULTICAST_GROUP_ID);
    result = bvlc6_bbmd_disabled_handler(
        &TD.BIP6_Addr, &TD.BACnet_Address, &mtu[0], mtu_len);
    assert(result == 0);
    assert(bvlc6_get_function_code() == BVLC6_VIRTUAL_ADDRESS_RESOLUTION);
    assert(VMAC_Find_By_Key(TD.Device_ID) != NULL);
    /* repeat with same device ID and address */
    /* change IPv6 address */
    mtu_len = bvlc6_encode_virtual_address_resolution(
        &mtu[0], sizeof(mtu), TD.Device_ID);
    result = bvlc6_bbmd_disabled_handler(
        &TD.BIP6_Addr, &TD.BACnet_Address, &mtu[0], mtu_len);
    assert(result == 0);
    assert(bvlc6_get_function_code() == BVLC6_VIRTUAL_ADDRESS_RESOLUTION);
    assert(VMAC_Find_By_Key(TD.Device_ID) != NULL);

    test_cleanup();
}

static void test_BBMD_Result(void)
{
    int result = 0;
    uint32_t vmac_src = 0x1234;
    uint16_t result_code[6] = {
        BVLC6_RESULT_SUCCESSFUL_COMPLETION,
        BVLC6_RESULT_ADDRESS_RESOLUTION_NAK,
        BVLC6_RESULT_VIRTUAL_ADDRESS_RESOLUTION_NAK,
        BVLC6_RESULT_REGISTER_FOREIGN_DEVICE_NAK,
        BVLC6_RESULT_DELETE_FOREIGN_DEVICE_NAK,
        BVLC6_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK
    };
    uint16_t test_result_code = 0;
    uint8_t test_function_code = 0;
    BACNET_IP6_ADDRESS addr;
    BACNET_ADDRESS src;
    unsigned int i = 0;
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;

    bvlc6_address_set(
        &addr, BIP6_MULTICAST_LINK_LOCAL, 0, 0, 0, 0, 0, 0,
        BIP6_MULTICAST_GROUP_ID);
    addr.port = 0xBAC0U;
    bvlc6_vmac_address_set(&src, vmac_src);
    for (i = 0; i < 6; i++) {
        mtu_len =
            bvlc6_encode_result(&mtu[0], sizeof(mtu), vmac_src, result_code[i]);
        result = bvlc6_bbmd_disabled_handler(&addr, &src, &mtu[0], mtu_len);
        /* validate that the result is handled (0) */
        assert(result == 0);
        test_result_code = bvlc6_get_last_result();
        assert(test_result_code == result_code[i]);
        test_function_code = bvlc6_get_function_code();
        assert(test_function_code == BVLC6_RESULT);
    }
}

int main(void)
{
    test_BBMD_Result();
    test_Execute_Virtual_Address_Resolution();
    test_Initiate_Original_Broadcast_NPDU();

    return 0;
}
