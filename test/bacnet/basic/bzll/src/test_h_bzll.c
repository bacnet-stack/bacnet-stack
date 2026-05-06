/**
 * @file
 * @brief BACnet ZigBee Link Layer Handler
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
#include "bacnet/basic/bzll/h_bzll.h"
/* support file*/
#include "support.h"
/* me! */
#include "test_h_bzll.h"

#define BZLL_NODE_TIME_TO_LIVE_S (600)
#define BZLL_SCAN_NODES_INTERVAL_S (3600)

/**
 * @brief Struct to be used during the test. Expands #device_info_t
 */
struct bzll_device_info_t {
    struct device_info_t device_info;
    uint8_t *mtu;
    int mtu_len;
    int status;
    struct bzll_vmac_data *expected_vmac_data;
};

static struct bzll_device_info_t TD;
static struct bzll_device_info_t IUT;

/* Auxiliary functions */
static void aux_test_vmac_data_broadcast(struct bzll_vmac_data *vmac_data);
static int aux_test_bzll_send_mpdu_cb(
    const struct bzll_vmac_data *dest, const uint8_t *mtu, int mtu_len);
static int aux_test_bzll_get_my_addr_cb(struct bzll_vmac_data *addr);
static int aux_bzll_advertise_address_cb(
    const uint8_t *protocol_addr, const uint16_t address_size);

/* Test functions */
static void test_bzll_Debug_Enable(void);
static void test_bzll_send_pdu(void);
static void test_bzll_receive(void);
static void test_bzll_address_handler(void);
static void test_bzll_timer_handling(void);

/**
 * @brief Array with all the tests.
 */
static test_suite_t test_suite[] = { TEST_ENTRY(test_bzll_Debug_Enable),
                                     TEST_ENTRY(test_bzll_send_pdu),
                                     TEST_ENTRY(test_bzll_receive),
                                     TEST_ENTRY(test_bzll_address_handler),
                                     TEST_ENTRY(test_bzll_timer_handling) };

/**
 * @brief Auxiliary function to fill the broadcast address
 * @param vmac_data [out] - The broadcast vmac address
 */
static void aux_test_vmac_data_broadcast(struct bzll_vmac_data *vmac_data)
{
    memset(vmac_data->mac, 0xFF, BZLL_VMAC_EUI64);
    vmac_data->endpoint = 0xFF;
}

/**
 * @brief Auxiliary function to test the send mpdu callback
 * @param dest [in] - destination address
 * @param mtu [in] - Payload
 * @param mtu_len [in] - Payload size
 * @return mtu_len if success, negative values if error
 */
static int aux_test_bzll_send_mpdu_cb(
    const struct bzll_vmac_data *dest, const uint8_t *mtu, int mtu_len)
{
    if (TD.status) {
        return TD.status;
    }

    assert(BZLL_VMAC_Same(dest, TD.expected_vmac_data));
    assert(mtu_len == TD.mtu_len);
    for (size_t i = 0; i < TD.mtu_len; i++) {
        assert(mtu[i] == TD.mtu[i]);
    }

    return mtu_len;
}

/**
 * @brief Auxiliary function to test the get my address callback
 * @param addr [out] - my address
 * @return 0 if success, negative values for error
 */
static int aux_test_bzll_get_my_addr_cb(struct bzll_vmac_data *addr)
{
    if (TD.expected_vmac_data) {
        BZLL_VMAC_Copy(addr, TD.expected_vmac_data);
        return 0;
    }
    return -1;
}

/**
 * @brief Auxiliary function to test the advertise address callback
 * @param protocol_addr [in] - address to be advertise
 * @param address_size [in] - address size
 * @return 0 if success, negative values for error
 */
static int aux_bzll_advertise_address_cb(
    const uint8_t *protocol_addr, const uint16_t address_size)
{
    assert(address_size == TD.device_info.BACnet_Address.mac_len);
    for (size_t i = 0; i < address_size; i++) {
        assert(protocol_addr[i] == TD.device_info.BACnet_Address.mac[i]);
    }
    return TD.status;
}

/**
 * @brief Test setup function
 */
static void test_setup(void)
{
    TD.device_info.Device_ID = 12345;
    uint8_t iut_mac[BZLL_VMAC_EUI64] = { 0x00, 0x12, 0x34, 0x56,
                                         0x78, 0x9A, 0xBC, 0xDF };
    uint8_t iut_endpoint = 0x02;

    bzll_init("TestInterface");

    support_set_device_info(&IUT.device_info);

    TD.device_info.Device_ID = 12345;
    bacnet_vmac_address_set(
        &TD.device_info.BACnet_Address, TD.device_info.Device_ID);
    IUT.device_info.Device_ID = 67890;
    bacnet_vmac_address_set(
        &IUT.device_info.BACnet_Address, IUT.device_info.Device_ID);
    BZLL_VMAC_Entry_Set(&IUT.device_info.VMAC_Data, iut_mac, iut_endpoint);

    bzll_send_mpdu_callback_set(aux_test_bzll_send_mpdu_cb);
    bzll_get_my_address_callback_set(aux_test_bzll_get_my_addr_cb);
    bzll_advertise_address_callback_set(aux_bzll_advertise_address_cb);
}

/**
 * @brief Cleanup function to free resources
 */
static void test_cleanup(void)
{
    bzll_cleanup();
    bzll_send_mpdu_callback_set(NULL);
    bzll_get_my_address_callback_set(NULL);
    bzll_advertise_address_callback_set(NULL);

    support_set_device_info(NULL);
}

/**
 * @brief Test debug enable
 */
static void test_bzll_Debug_Enable(void)
{
    bool status = false;

    bzll_debug_enable();
    status = bzll_debug_enabled();
    assert(status == true);
}

/**
 * @brief Test send pdu
 */
static void test_bzll_send_pdu(void)
{
    int status = -1;
    BACNET_NPDU_DATA test_npdu_data;
    uint8_t test_pdu[BZLL_MPDU_MAX];
    unsigned test_pdu_len = 10;
    struct bzll_vmac_data test_vmac_data;

    TD.mtu = test_pdu;
    TD.expected_vmac_data = &test_vmac_data;
    TD.status = 0;

    test_pdu_len = 100;
    memset(test_pdu, 0xCC, test_pdu_len);
    TD.mtu_len = test_pdu_len;
    TD.device_info.BACnet_Address.len = 3;

    /* Broadcast net broadcast */
    TD.device_info.BACnet_Address.net = BACNET_BROADCAST_NETWORK;
    aux_test_vmac_data_broadcast(&test_vmac_data);
    status = bzll_send_pdu(
        &TD.device_info.BACnet_Address, &test_npdu_data, test_pdu,
        test_pdu_len);
    assert(status == test_pdu_len);

    /* Broadcast net with no destination */
    TD.device_info.BACnet_Address.net = 1;
    TD.device_info.BACnet_Address.mac_len = 0;
    status = bzll_send_pdu(
        &TD.device_info.BACnet_Address, &test_npdu_data, test_pdu,
        test_pdu_len);
    assert(status == test_pdu_len);

    /* Unicast No VMAC */
    bacnet_vmac_address_set(
        &TD.device_info.BACnet_Address, TD.device_info.Device_ID);
    TD.device_info.BACnet_Address.len = 3;
    status = bzll_send_pdu(
        &TD.device_info.BACnet_Address, &test_npdu_data, test_pdu,
        test_pdu_len);
    assert(status == -1);

    BZLL_VMAC_Add(&TD.device_info.Device_ID, &TD.device_info.VMAC_Data);
    TD.expected_vmac_data = &TD.device_info.VMAC_Data;

    /* Unicast Fail to transmit*/
    TD.status = -1;
    status = bzll_send_pdu(
        &TD.device_info.BACnet_Address, &test_npdu_data, test_pdu,
        test_pdu_len);
    assert(status == -1);

    /* Unicast */
    TD.device_info.BACnet_Address.net = 1;
    TD.status = 0;
    status = bzll_send_pdu(
        &TD.device_info.BACnet_Address, &test_npdu_data, test_pdu,
        test_pdu_len);
    assert(status == test_pdu_len);

    /* Invalid DADDR */
    TD.device_info.BACnet_Address.net = 1;
    TD.device_info.BACnet_Address.mac_len = 4;
    status = bzll_send_pdu(
        &TD.device_info.BACnet_Address, &test_npdu_data, test_pdu,
        test_pdu_len);
    assert(status == -1);
}

/**
 * @brief Test receive
 */
static void test_bzll_receive(void)
{
    uint16_t status = 0;
    BACNET_ADDRESS src = { 0 };
    uint8_t npdu[BZLL_MPDU_MAX];
    uint16_t max_npdu = BZLL_MPDU_MAX;
    uint8_t recv_npdu[100];
    uint16_t npdu_len = 100;
    unsigned timeout = 0;
    struct bzll_node_status_table_entry *status_entry;

    TD.expected_vmac_data = &TD.device_info.VMAC_Data;
    memset(recv_npdu, 0xDD, npdu_len);

    /* No data received */
    status = bzll_receive(&src, npdu, max_npdu, timeout);
    assert(status == 0);

    /* Received from my address */
    bzll_receive_pdu(&TD.device_info.VMAC_Data, recv_npdu, npdu_len);
    status = bzll_receive(&src, npdu, max_npdu, timeout);
    assert(status == 0);

    /* Dropped Received, not enough space */
    bzll_receive_pdu(&IUT.device_info.VMAC_Data, recv_npdu, npdu_len);
    status = bzll_receive(&src, npdu, max_npdu - 1, timeout);
    assert(status == 0);

    /* Received from a known address*/
    memset(npdu, 0, max_npdu);
    BZLL_VMAC_Add(&IUT.device_info.Device_ID, &IUT.device_info.VMAC_Data);
    bzll_receive_pdu(&IUT.device_info.VMAC_Data, recv_npdu, npdu_len);
    status = bzll_receive(&src, npdu, max_npdu, timeout);
    assert(status == 100);
    for (size_t i = 0; i < status; i++) {
        assert(npdu[i] == recv_npdu[i]);
    }
    assert(bacnet_address_same(&src, &IUT.device_info.BACnet_Address));
    BZLL_VMAC_Node_Status_Entry(&IUT.device_info.VMAC_Data, &status_entry);
    assert(status_entry->valid == true);
    assert(status_entry->ttl_seconds_remaining == BZLL_NODE_TIME_TO_LIVE_S);

    /* Received from a unknown address */
    memset(npdu, 0, max_npdu);
    BZLL_VMAC_Delete(IUT.device_info.Device_ID);
    bzll_receive_pdu(&IUT.device_info.VMAC_Data, recv_npdu, npdu_len);
    status = bzll_receive(&src, npdu, max_npdu, timeout);
    assert(status == 100);
    for (size_t i = 0; i < status; i++) {
        assert(npdu[i] == recv_npdu[i]);
    }
    assert(!bacnet_address_same(&src, &IUT.device_info.BACnet_Address));
    BZLL_VMAC_Node_Status_Entry(&IUT.device_info.VMAC_Data, &status_entry);
    assert(status_entry->valid == true);
    assert(status_entry->ttl_seconds_remaining == BZLL_NODE_TIME_TO_LIVE_S);
}

/**
 * @brief Test the address handling
 */
static void test_bzll_address_handler(void)
{
    struct bzll_node_status_table_entry *status_entry;
    BACNET_ADDRESS test_dest = { 0 };
    uint32_t test_transfer_size = 0;
    uint32_t test_device_id = 0;
    bool ret = false;
    int status = 0;

    /* Get Broadcast Address */
    bzll_get_broadcast_address(&test_dest);
    assert(test_dest.net == BACNET_BROADCAST_NETWORK);
    assert(test_dest.mac_len == 3);
    for (size_t i = 0; i < test_dest.mac_len; i++) {
        assert(test_dest.mac[i] == 0xFF);
    }

    /* Get my address */
    bzll_get_my_address(&test_dest);
    assert(bacnet_address_same(&test_dest, &IUT.device_info.BACnet_Address));

    /* Set my address */
    bzll_set_my_address(&TD.device_info.BACnet_Address);

    /* Get maximum incoming transfer size */
    bzll_get_maximum_incoming_transfer_size(&test_transfer_size);
    assert(test_transfer_size == BZLL_MPDU_MAX);

    /* Get maximum outgoing transfer size */
    test_transfer_size = 0;
    bzll_get_maximum_outgoing_transfer_size(&test_transfer_size);
    assert(test_transfer_size == BZLL_MPDU_MAX);

    /* Match protocol address */
    ret = bzll_match_protocol_address(
        IUT.device_info.BACnet_Address.mac,
        IUT.device_info.BACnet_Address.mac_len);
    assert(ret == true);

    ret = bzll_match_protocol_address(
        TD.device_info.BACnet_Address.mac,
        TD.device_info.BACnet_Address.mac_len);
    assert(ret == false);

    /* Update node protocol address */
    BZLL_VMAC_Add(&TD.device_info.Device_ID, &TD.device_info.VMAC_Data);
    TD.device_info.Device_ID = 54321;
    bacnet_vmac_address_set(
        &TD.device_info.BACnet_Address, TD.device_info.Device_ID);
    ret = bzll_update_node_protocol_address(
        &TD.device_info.VMAC_Data, TD.device_info.BACnet_Address.mac,
        TD.device_info.BACnet_Address.mac_len);
    assert(ret == true);
    status = BZLL_VMAC_Count();
    assert(status == 1);
    BZLL_VMAC_Entry_To_Device_ID(&TD.device_info.VMAC_Data, &test_device_id);
    assert(test_device_id == TD.device_info.Device_ID);
    BZLL_VMAC_Node_Status_Entry(&TD.device_info.VMAC_Data, &status_entry);
    assert(status_entry->valid == true);
    assert(status_entry->ttl_seconds_remaining == BZLL_NODE_TIME_TO_LIVE_S);

    /* Update my address */
    support_set_device_info(&TD.device_info);
    TD.device_info.Device_ID = 0x123456;
    bacnet_vmac_address_set(
        &TD.device_info.BACnet_Address, TD.device_info.Device_ID);
    ret = bzll_update_object_protocol_address(
        TD.device_info.BACnet_Address.mac,
        TD.device_info.BACnet_Address.mac_len);
    assert(ret == true);
}

/**
 * @brief Test the timer related functions
 */
static void test_bzll_timer_handling(void)
{
    struct bzll_node_status_table_entry *status_entry;
    struct bzll_vmac_data vmac_data;
    int count = 0;

    /* Test if empty */
    bzll_maintenance_timer(1);

    /* Set up other tests */
    BZLL_VMAC_Add(&TD.device_info.Device_ID, &TD.device_info.VMAC_Data);
    BZLL_VMAC_Add(&IUT.device_info.Device_ID, &IUT.device_info.VMAC_Data);
    BZLL_VMAC_Node_Status_Entry(&TD.device_info.VMAC_Data, &status_entry);
    status_entry->valid = true;
    status_entry->ttl_seconds_remaining = BZLL_NODE_TIME_TO_LIVE_S;
    BZLL_VMAC_Node_Status_Entry(&IUT.device_info.VMAC_Data, &status_entry);
    status_entry->valid = true;
    status_entry->ttl_seconds_remaining = BZLL_NODE_TIME_TO_LIVE_S - 10;

    /* Test nothing to do */
    bzll_maintenance_timer(1);
    BZLL_VMAC_Node_Status_Entry(&TD.device_info.VMAC_Data, &status_entry);
    assert(
        status_entry->ttl_seconds_remaining == (BZLL_NODE_TIME_TO_LIVE_S - 1));
    BZLL_VMAC_Node_Status_Entry(&IUT.device_info.VMAC_Data, &status_entry);
    assert(
        status_entry->ttl_seconds_remaining == (BZLL_NODE_TIME_TO_LIVE_S - 11));

    /* Remove invalid, request valid*/
    BZLL_VMAC_Node_Status_Entry(&IUT.device_info.VMAC_Data, &status_entry);
    status_entry->valid = false;
    TD.expected_vmac_data = &TD.device_info.VMAC_Data;
    bzll_maintenance_timer(BZLL_NODE_TIME_TO_LIVE_S);
    count = BZLL_VMAC_Count();
    assert(count == 1);
    BZLL_VMAC_Node_Status_Entry(&TD.device_info.VMAC_Data, &status_entry);
    assert(status_entry->ttl_seconds_remaining == (BZLL_NODE_TIME_TO_LIVE_S));
    assert(status_entry->valid == false);

    /* Request Scan */
    BZLL_VMAC_Node_Status_Entry(&TD.device_info.VMAC_Data, &status_entry);
    status_entry->valid = true;
    status_entry->ttl_seconds_remaining = BZLL_SCAN_NODES_INTERVAL_S + 1;
    aux_test_vmac_data_broadcast(&vmac_data);
    TD.expected_vmac_data = &vmac_data;
    bzll_maintenance_timer(BZLL_SCAN_NODES_INTERVAL_S);
    BZLL_VMAC_Node_Status_Entry(&TD.device_info.VMAC_Data, &status_entry);
    assert(status_entry->valid == false);
    assert(status_entry->ttl_seconds_remaining == BZLL_NODE_TIME_TO_LIVE_S);
}

/**
 * @brief Test bzll wrapper
 */
void test_h_bzll(void)
{
    RUN_TESTS(test_suite, test_setup, test_cleanup);
}
