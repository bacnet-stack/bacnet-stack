/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2020
 * @brief Test file for a basic BBMD for BVLC IPv4 handler
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h> /* for standard i/o, like printing */
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <string.h> /* for memcpy */
#include <assert.h>
#include <string.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/iam.h"
#include "bacnet/npdu.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/datalink/bvlc.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/bbmd/h_bbmd.h"

struct device_info_t {
    uint32_t Device_ID;
    BACNET_IP_ADDRESS BIP_Addr;
    BACNET_IP_ADDRESS BIP_Broadcast_Addr;
    BACNET_ADDRESS BACnet_Address;
};
static struct device_info_t TD;
static struct device_info_t IUT;

/* for the reply sent from the handler */
static uint8_t Test_Sent_Message_Type;
static uint8_t Test_Sent_Message_Length;
static uint8_t Test_Sent_Message_Buffer[MAX_APDU];
static uint16_t Test_Sent_Message_Buffer_Length;
static BACNET_IP_ADDRESS Test_Sent_Message_Dest;
/* tracks every destination sent to, for tests that forward to many peers */
#define TEST_SENT_MESSAGE_DEST_MAX 16
static BACNET_IP_ADDRESS
    Test_Sent_Message_Dest_List[TEST_SENT_MESSAGE_DEST_MAX];
static unsigned Test_Sent_Message_Count;

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
uint16_t bip_receive(
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
 * @param dest - Points to a BACNET_IP_ADDRESS structure containing the
 *  destination address.
 * @param mtu - the bytes of data to send
 * @param mtu_len - the number of bytes of data to send
 *
 * @return Upon successful completion, returns the number of bytes sent.
 *  Otherwise, -1 shall be returned to indicate the error.
 */
int bip_send_mpdu(
    const BACNET_IP_ADDRESS *dest, const uint8_t *mtu, uint16_t mtu_len)
{
    uint8_t message_type = 0;
    uint16_t message_length = 0;
    int header_len = 0;

    header_len =
        bvlc_decode_header(mtu, mtu_len, &message_type, &message_length);
    Test_Sent_Message_Type = message_type;
    Test_Sent_Message_Length = message_length;
    bvlc_address_copy(&Test_Sent_Message_Dest, dest);
    if (Test_Sent_Message_Count < TEST_SENT_MESSAGE_DEST_MAX) {
        bvlc_address_copy(
            &Test_Sent_Message_Dest_List[Test_Sent_Message_Count], dest);
    }
    Test_Sent_Message_Count++;
    if ((header_len == 4) && (mtu_len >= 4)) {
        memcpy(&Test_Sent_Message_Buffer[0], &mtu[4], mtu_len - 4);
        Test_Sent_Message_Buffer_Length = mtu_len - 4;
    } else {
        Test_Sent_Message_Buffer_Length = 0;
    }

    return 0;
}

/**
 * @brief Check whether a destination address was sent to since the
 *  message counters were last reset.
 *
 * @param addr - destination address to look for
 * @return true if a message was sent to addr
 */
static bool test_sent_message_dest_contains(const BACNET_IP_ADDRESS *addr)
{
    unsigned i = 0;
    unsigned count = Test_Sent_Message_Count;

    if (count > TEST_SENT_MESSAGE_DEST_MAX) {
        count = TEST_SENT_MESSAGE_DEST_MAX;
    }
    for (i = 0; i < count; i++) {
        if (!bvlc_address_different(&Test_Sent_Message_Dest_List[i], addr)) {
            return true;
        }
    }

    return false;
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
bool bip_get_addr(BACNET_IP_ADDRESS *addr)
{
    return bvlc_address_copy(addr, &IUT.BIP_Addr);
}

/**
 * Get the BACnet/IP address
 *
 * @return BACnet/IP address
 */
bool bip_get_broadcast_addr(BACNET_IP_ADDRESS *addr)
{
    return bvlc_address_copy(addr, &IUT.BIP_Broadcast_Addr);
}

static void test_setup(void)
{
    bvlc_init();
    bvlc_address_set(&TD.BIP_Broadcast_Addr, 255, 255, 255, 255);
    bvlc_address_set(&TD.BIP_Addr, 192, 168, 1, 100);
    TD.Device_ID = 12345;

    bvlc_address_set(&IUT.BIP_Broadcast_Addr, 255, 255, 255, 255);
    bvlc_address_set(&IUT.BIP_Addr, 192, 168, 1, 10);
    IUT.Device_ID = 54321;

    Test_Sent_Message_Count = 0;
}

static void test_cleanup(void)
{
}

/**
 * @brief Test 15.2.1.1 Initiate Original-Broadcast-NPDU
 */
static void test_Initiate_Original_Broadcast_NPDU(void)
{
    uint8_t pdu[MAX_APDU] = { 0 };
    int npdu_len = 0;
    int apdu_len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS dest = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint8_t test_pdu[MAX_APDU] = { 0 };
    uint16_t test_pdu_len = 0;
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
    bvlc_send_pdu(&dest, &npdu_data, pdu, pdu_len);
    /* DA=Link Local Multicast Address */
    assert(!bvlc_address_different(
        &TD.BIP_Broadcast_Addr, &Test_Sent_Message_Dest));
    /* SA = IUT - done in port layer */
    /* Original-Broadcast-NPDU */
    assert(Test_Sent_Message_Type == BVLC_ORIGINAL_BROADCAST_NPDU);
    if (Test_Sent_Message_Type == BVLC_ORIGINAL_BROADCAST_NPDU) {
        function_len = bvlc_decode_original_broadcast(
            Test_Sent_Message_Buffer, Test_Sent_Message_Buffer_Length, test_pdu,
            sizeof(test_pdu), &test_pdu_len);
        printf(
            "len=%u pdu[%u] test_pdu[%u] %s\n", (unsigned)function_len,
            (unsigned)Test_Sent_Message_Buffer_Length,
            (unsigned)sizeof(test_pdu), (function_len > 0) ? "PASS" : "FAIL");
        assert(function_len > 0);
        /* (any valid BACnet-Unconfirmed-Request-PDU,
            with any valid broadcast network options */
        assert(test_pdu_len == pdu_len);
    }
    test_cleanup();
}

static void test_Initiate_Original_Broadcast_NPDU_Uses_Broadcast_Port(void)
{
    uint8_t pdu[MAX_APDU] = { 0 };
    int npdu_len = 0;
    int apdu_len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS dest = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };

    test_setup();
    IUT.BIP_Addr.port = 0xBAC1U;
    IUT.BIP_Broadcast_Addr.port = 0xBAC2U;

    dest.net = BACNET_BROADCAST_NETWORK;
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    npdu_len = npdu_encode_pdu(&pdu[0], &dest, &IUT.BACnet_Address, &npdu_data);
    apdu_len = iam_encode_apdu(
        &pdu[npdu_len], IUT.Device_ID, MAX_APDU, SEGMENTATION_NONE,
        BACNET_VENDOR_ID);
    pdu_len = npdu_len + apdu_len;
    bvlc_send_pdu(&dest, &npdu_data, pdu, pdu_len);

    assert(Test_Sent_Message_Type == BVLC_ORIGINAL_BROADCAST_NPDU);
    assert(Test_Sent_Message_Dest.port == IUT.BIP_Broadcast_Addr.port);
    assert(Test_Sent_Message_Dest.port != IUT.BIP_Addr.port);
    test_cleanup();
}

/**
 * @brief Test that the BBMD NAT anti-loop check in bbmd_bdt_forward_npdu()
 *  only skips the BDT peer whose forward address equals the NAT global
 *  address, and still forwards to every other BDT peer. (bug B2)
 */
static void test_BBMD_NAT_Anti_Loop_Forward(void)
{
    uint8_t pdu[MAX_APDU] = { 0 };
    int npdu_len = 0;
    int apdu_len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS dest = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint8_t mtu[MAX_APDU] = { 0 };
    int mtu_len = 0;
    BACNET_ADDRESS src = { 0 };
    BACNET_IP_ADDRESS peer_A = { 0 };
    BACNET_IP_ADDRESS peer_B_is_global = { 0 };
    BACNET_IP_ADDRESS peer_C = { 0 };
    BACNET_IP_ADDRESS global_address = { 0 };
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt = NULL;
    bool status = false;

    test_setup();
    /* build an Original-Broadcast-NPDU as received from a 3rd party device */
    dest.net = BACNET_BROADCAST_NETWORK;
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    npdu_len = npdu_encode_pdu(&pdu[0], &dest, &TD.BACnet_Address, &npdu_data);
    apdu_len = iam_encode_apdu(
        &pdu[npdu_len], TD.Device_ID, MAX_APDU, SEGMENTATION_NONE,
        BACNET_VENDOR_ID);
    pdu_len = npdu_len + apdu_len;
    mtu_len =
        bvlc_encode_original_broadcast(&mtu[0], sizeof(mtu), &pdu[0], pdu_len);
    assert(mtu_len > 0);

    /* configure the BDT: peer B's forward address equals the NAT global
       address; peers A and C are ordinary remote BBMDs */
    bvlc_address_set(&peer_A, 192, 168, 1, 20);
    peer_A.port = 0xBAC0U;
    bvlc_address_set(&peer_B_is_global, 203, 0, 113, 5);
    peer_B_is_global.port = 0xBAC0U;
    bvlc_address_set(&peer_C, 192, 168, 1, 30);
    peer_C.port = 0xBAC0U;
    bvlc_address_copy(&global_address, &peer_B_is_global);

    bvlc_bdt_list_clear();
    bdt = bvlc_bdt_list();
    bdt[0].valid = true;
    bvlc_address_copy(&bdt[0].dest_address, &peer_A);
    bvlc_broadcast_distribution_mask_set(
        &bdt[0].broadcast_mask, 255, 255, 255, 255);
    bdt[1].valid = true;
    bvlc_address_copy(&bdt[1].dest_address, &peer_B_is_global);
    bvlc_broadcast_distribution_mask_set(
        &bdt[1].broadcast_mask, 255, 255, 255, 255);
    bdt[2].valid = true;
    bvlc_address_copy(&bdt[2].dest_address, &peer_C);
    bvlc_broadcast_distribution_mask_set(
        &bdt[2].broadcast_mask, 255, 255, 255, 255);

    /* Case 1: NAT handling disabled - every valid BDT peer is forwarded */
    bvlc_disable_nat();
    Test_Sent_Message_Count = 0;
    (void)bvlc_bbmd_enabled_handler(&TD.BIP_Addr, &src, &mtu[0], mtu_len);
    assert(Test_Sent_Message_Count == 3);
    status = test_sent_message_dest_contains(&peer_A);
    assert(status);
    status = test_sent_message_dest_contains(&peer_B_is_global);
    assert(status);
    status = test_sent_message_dest_contains(&peer_C);
    assert(status);

    /* Case 2: NAT handling enabled - only the peer whose forward address
       equals the NAT global address is skipped, to avoid a forwarding
       loop through the NAT router; all other peers are still forwarded */
    bvlc_set_global_address_for_nat(&global_address);
    Test_Sent_Message_Count = 0;
    (void)bvlc_bbmd_enabled_handler(&TD.BIP_Addr, &src, &mtu[0], mtu_len);
    assert(Test_Sent_Message_Count == 2);
    status = test_sent_message_dest_contains(&peer_A);
    assert(status);
    status = test_sent_message_dest_contains(&peer_B_is_global);
    assert(!status);
    status = test_sent_message_dest_contains(&peer_C);
    assert(status);

    bvlc_disable_nat();
    bvlc_bdt_list_clear();
    test_cleanup();
}

/**
 * @brief Test that the BBMD NAT anti-loop check in bbmd_fdt_forward_npdu()
 *  only skips the FDT peer whose forward address equals the NAT global
 *  address, and still forwards to every other live FDT peer. (bug B2)
 */
static void test_BBMD_NAT_Anti_Loop_Forward_FDT(void)
{
    uint8_t pdu[MAX_APDU] = { 0 };
    int npdu_len = 0;
    int apdu_len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS dest = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint8_t mtu[MAX_APDU] = { 0 };
    int mtu_len = 0;
    BACNET_ADDRESS src = { 0 };
    BACNET_IP_ADDRESS fd_A = { 0 };
    BACNET_IP_ADDRESS fd_B_is_global = { 0 };
    BACNET_IP_ADDRESS fd_C = { 0 };
    BACNET_IP_ADDRESS global_address = { 0 };
    bool status = false;

    test_setup();
    /* build an Original-Broadcast-NPDU as received from a 3rd party device */
    dest.net = BACNET_BROADCAST_NETWORK;
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    npdu_len = npdu_encode_pdu(&pdu[0], &dest, &TD.BACnet_Address, &npdu_data);
    apdu_len = iam_encode_apdu(
        &pdu[npdu_len], TD.Device_ID, MAX_APDU, SEGMENTATION_NONE,
        BACNET_VENDOR_ID);
    pdu_len = npdu_len + apdu_len;
    mtu_len =
        bvlc_encode_original_broadcast(&mtu[0], sizeof(mtu), &pdu[0], pdu_len);
    assert(mtu_len > 0);

    /* no BDT peers - isolate this test to the FDT forwarding path */
    bvlc_bdt_list_clear();

    /* configure the FDT: foreign device B's forward address equals the
       NAT global address; foreign devices A and C are ordinary peers */
    bvlc_address_set(&fd_A, 192, 168, 1, 40);
    fd_A.port = 0xBAC0U;
    bvlc_address_set(&fd_B_is_global, 203, 0, 113, 6);
    fd_B_is_global.port = 0xBAC0U;
    bvlc_address_set(&fd_C, 192, 168, 1, 50);
    fd_C.port = 0xBAC0U;
    bvlc_address_copy(&global_address, &fd_B_is_global);

    bvlc_foreign_device_table_valid_clear(bvlc_fdt_list());
    status = bvlc_foreign_device_table_entry_add(bvlc_fdt_list(), &fd_A, 60);
    assert(status);
    status = bvlc_foreign_device_table_entry_add(
        bvlc_fdt_list(), &fd_B_is_global, 60);
    assert(status);
    status = bvlc_foreign_device_table_entry_add(bvlc_fdt_list(), &fd_C, 60);
    assert(status);

    /* Case 1: NAT handling disabled - every live FDT peer is forwarded */
    bvlc_disable_nat();
    Test_Sent_Message_Count = 0;
    (void)bvlc_bbmd_enabled_handler(&TD.BIP_Addr, &src, &mtu[0], mtu_len);
    assert(Test_Sent_Message_Count == 3);
    status = test_sent_message_dest_contains(&fd_A);
    assert(status);
    status = test_sent_message_dest_contains(&fd_B_is_global);
    assert(status);
    status = test_sent_message_dest_contains(&fd_C);
    assert(status);

    /* Case 2: NAT handling enabled - only the peer whose forward address
       equals the NAT global address is skipped, to avoid a forwarding
       loop through the NAT router; all other peers are still forwarded */
    bvlc_set_global_address_for_nat(&global_address);
    Test_Sent_Message_Count = 0;
    (void)bvlc_bbmd_enabled_handler(&TD.BIP_Addr, &src, &mtu[0], mtu_len);
    assert(Test_Sent_Message_Count == 2);
    status = test_sent_message_dest_contains(&fd_A);
    assert(status);
    status = test_sent_message_dest_contains(&fd_B_is_global);
    assert(!status);
    status = test_sent_message_dest_contains(&fd_C);
    assert(status);

    bvlc_disable_nat();
    bvlc_foreign_device_table_valid_clear(bvlc_fdt_list());
    test_cleanup();
}

static void test_BBMD_Result(void)
{
    int result = 0;
    uint16_t result_code[] = {
        BVLC_RESULT_SUCCESSFUL_COMPLETION,
        BVLC_RESULT_WRITE_BROADCAST_DISTRIBUTION_TABLE_NAK,
        BVLC_RESULT_READ_BROADCAST_DISTRIBUTION_TABLE_NAK,
        BVLC_RESULT_REGISTER_FOREIGN_DEVICE_NAK,
        BVLC_RESULT_READ_FOREIGN_DEVICE_TABLE_NAK,
        BVLC_RESULT_DELETE_FOREIGN_DEVICE_TABLE_ENTRY_NAK,
        BVLC_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK
    };
    size_t result_code_max = sizeof(result_code) / sizeof(result_code[0]);
    uint16_t test_result_code = 0;
    uint8_t test_function_code = 0;
    BACNET_IP_ADDRESS addr;
    BACNET_ADDRESS src;
    unsigned int i = 0;
    uint8_t mtu[MAX_APDU] = { 0 };
    uint16_t mtu_len = 0;

    bvlc_address_port_from_ascii(&addr, "192.168.0.1", "0xBAC0");
    for (i = 0; i < result_code_max; i++) {
        mtu_len = bvlc_encode_result(&mtu[0], sizeof(mtu), result_code[i]);
        result = bvlc_bbmd_disabled_handler(&addr, &src, &mtu[0], mtu_len);
        /* validate that the result is handled (0) */
        assert(result == 0);
        test_result_code = bvlc_get_last_result();
        assert(test_result_code == result_code[i]);
        test_function_code = bvlc_get_function_code();
        assert(test_function_code == BVLC_RESULT);
        result = bvlc_bbmd_enabled_handler(&addr, &src, &mtu[0], mtu_len);
        /* validate that the result is handled (0) */
        assert(result == 0);
        test_result_code = bvlc_get_last_result();
        assert(test_result_code == result_code[i]);
        test_function_code = bvlc_get_function_code();
        assert(test_function_code == BVLC_RESULT);
    }
}

int main(void)
{
    /* individual tests */
    test_BBMD_Result();
    test_Initiate_Original_Broadcast_NPDU();
    test_Initiate_Original_Broadcast_NPDU_Uses_Broadcast_Port();
    test_BBMD_NAT_Anti_Loop_Forward();
    test_BBMD_NAT_Anti_Loop_Forward_FDT();

    return 0;
}
