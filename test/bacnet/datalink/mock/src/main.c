/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet datalink return codes
 */

#include <stdlib.h>  /* For calloc() */
#include <ztest.h>
#include <bacnet/datalink/datalink.h>
#include <../ports/rx62n/bacnet.h>
#include "bacnet/apdu.h"

extern bool check_arcnet_receive_src;

void bvlc_maintenance_timer(uint16_t seconds)
{
    ztest_check_expected_value(seconds);
}

void bvlc6_maintenance_timer(uint16_t seconds)
{
    ztest_check_expected_value(seconds);
}

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test datalink
 */

static void test_datalink_arcnet(void)
{
    char *iface = "bla-bla-bla";
    char *iface2 = "bla-bla-bla2";
    uint8_t expected_data[] = { 0x5A, 0xA5, 0xDE, 0xAD };
    uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    BACNET_ADDRESS addr = {
        .mac_len = 6, 
        .mac = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE },
        .net = 54,
        .len = 7,
        .adr = { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32 }
    };
    BACNET_ADDRESS addr2 = {0};
    BACNET_NPDU_DATA npdu = {0};

    check_arcnet_receive_src = true;
    zassert_equal(z_cleanup_mock(), 0, NULL);
    datalink_set("arcnet");

    // init
    ztest_expect_value(arcnet_init, interface_name, iface);
    ztest_returns_value(arcnet_init, true);
    zassert_equal(datalink_init(iface), true, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    ztest_expect_value(arcnet_init, interface_name, iface2);
    ztest_returns_value(arcnet_init, false);
    zassert_equal(datalink_init(iface2), false, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // send_pdu
    ztest_expect_value(arcnet_send_pdu, dest, &addr);
    ztest_expect_value(arcnet_send_pdu, npdu_data, &npdu);
    ztest_expect_data(arcnet_send_pdu, pdu, expected_data);
    ztest_returns_value(arcnet_send_pdu, 4);
    zassert_equal(datalink_send_pdu(&addr, &npdu, expected_data,
        sizeof(expected_data)), 4, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // receive
    ztest_expect_value(arcnet_receive, src, &addr);
    ztest_expect_value(arcnet_receive, timeout, 10);
    ztest_expect_data(arcnet_receive, pdu, expected_data);
    ztest_returns_value(arcnet_receive, 4);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 10), 4, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    memset(expected_data, 0xff, sizeof(expected_data));
    memset(data, 0x00, sizeof(data));
    ztest_expect_value(arcnet_receive, src, &addr);
    ztest_expect_value(arcnet_receive, timeout, 15);
    ztest_expect_data(arcnet_receive, pdu, expected_data);
    ztest_returns_value(arcnet_receive, 0);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 15), 0, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_broadcast_address
    ztest_expect_value(arcnet_get_broadcast_address, dest, &addr);
    datalink_get_broadcast_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_my_address
    ztest_expect_value(arcnet_get_my_address, my_address, &addr);
    datalink_get_my_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // set_interface - do nothing
    datalink_set_interface(iface);

    // maintenance_timer - do nothing for arcnet
    datalink_maintenance_timer(42);
}

static void test_datalink_bip(void)
{
    char *iface = "bla-bla-bla";
    char *iface2 = "bla-bla-bla2";
    uint8_t expected_data[] = { 0x5A, 0xA5, 0xDE, 0xAD };
    uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    BACNET_ADDRESS addr = {
        .mac_len = 6, 
        .mac = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE },
        .net = 54,
        .len = 7,
        .adr = { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32 }
    };
    BACNET_ADDRESS addr2 = {0};
    BACNET_NPDU_DATA npdu = {0};

    zassert_equal(z_cleanup_mock(), 0, NULL);
    datalink_set("bip");

    // init
    ztest_expect_value(bip_init, ifname, iface);
    ztest_returns_value(bip_init, true);
    zassert_equal(datalink_init(iface), true, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    ztest_expect_value(bip_init, ifname, iface2);
    ztest_returns_value(bip_init, false);
    zassert_equal(datalink_init(iface2), false, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // send_pdu
    ztest_expect_value(bip_send_pdu, dest, &addr);
    ztest_expect_value(bip_send_pdu, npdu_data, &npdu);
    ztest_expect_data(bip_send_pdu, pdu, expected_data);
    ztest_returns_value(bip_send_pdu, 4);
    zassert_equal(datalink_send_pdu(&addr, &npdu, expected_data,
        sizeof(expected_data)), 4, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // receive
    ztest_expect_value(bip_receive, src, &addr);
    ztest_expect_value(bip_receive, timeout, 10);
    ztest_expect_data(bip_receive, pdu, expected_data);
    ztest_returns_value(bip_receive, 4);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 10), 4, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    memset(expected_data, 0xff, sizeof(expected_data));
    memset(data, 0x00, sizeof(data));
    ztest_expect_value(bip_receive, src, &addr);
    ztest_expect_value(bip_receive, timeout, 15);
    ztest_expect_data(bip_receive, pdu, expected_data);
    ztest_returns_value(bip_receive, 0);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 15), 0, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_broadcast_address
    ztest_expect_value(bip_get_broadcast_address, dest, &addr);
    datalink_get_broadcast_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_my_address
    ztest_expect_value(bip_get_my_address, my_address, &addr);
    datalink_get_my_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // set_interface - do nothing
    datalink_set_interface(iface);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // maintenance_timer
    ztest_expect_value(bvlc_maintenance_timer, seconds, 42);
    datalink_maintenance_timer(42);
    zassert_equal(z_cleanup_mock(), 0, NULL);
}

static void test_datalink_bip6(void)
{
    char *iface = "bla-bla-bla";
    char *iface2 = "bla-bla-bla2";
    uint8_t expected_data[] = { 0x5A, 0xA5, 0xDE, 0xAD };
    uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    BACNET_ADDRESS addr = {
        .mac_len = 6, 
        .mac = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE },
        .net = 54,
        .len = 7,
        .adr = { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32 }
    };
    BACNET_ADDRESS addr2 = {0};
    BACNET_NPDU_DATA npdu = {0};

    zassert_equal(z_cleanup_mock(), 0, NULL);
    datalink_set("bip6");

    // init
    ztest_expect_value(bip6_init, ifname, iface);
    ztest_returns_value(bip6_init, true);
    zassert_equal(datalink_init(iface), true, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    ztest_expect_value(bip6_init, ifname, iface2);
    ztest_returns_value(bip6_init, false);
    zassert_equal(datalink_init(iface2), false, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // send_pdu
    ztest_expect_value(bip6_send_pdu, dest, &addr);
    ztest_expect_value(bip6_send_pdu, npdu_data, &npdu);
    ztest_expect_data(bip6_send_pdu, pdu, expected_data);
    ztest_returns_value(bip6_send_pdu, 4);
    zassert_equal(datalink_send_pdu(&addr, &npdu, expected_data,
        sizeof(expected_data)), 4, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // receive
    ztest_expect_value(bip6_receive, src, &addr);
    ztest_expect_value(bip6_receive, timeout, 10);
    ztest_expect_data(bip6_receive, pdu, expected_data);
    ztest_returns_value(bip6_receive, 4);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 10), 4, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    memset(expected_data, 0xff, sizeof(expected_data));
    memset(data, 0x00, sizeof(data));
    ztest_expect_value(bip6_receive, src, &addr);
    ztest_expect_value(bip6_receive, timeout, 15);
    ztest_expect_data(bip6_receive, pdu, expected_data);
    ztest_returns_value(bip6_receive, 0);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 15), 0, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_broadcast_address
    ztest_expect_value(bip6_get_broadcast_address, my_address, &addr);
    datalink_get_broadcast_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_my_address
    ztest_expect_value(bip6_get_my_address, my_address, &addr);
    datalink_get_my_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // set_interface - do nothing
    datalink_set_interface(iface);

    // maintenance_timer
    ztest_expect_value(bvlc6_maintenance_timer, seconds, 42);
    datalink_maintenance_timer(42);
    zassert_equal(z_cleanup_mock(), 0, NULL);
}

static void test_datalink_dlmstp(void)
{
    char *iface = "bla-bla-bla";
    char *iface2 = "bla-bla-bla2";
    uint8_t expected_data[] = { 0x5A, 0xA5, 0xDE, 0xAD };
    uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    BACNET_ADDRESS addr = {
        .mac_len = 6, 
        .mac = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE },
        .net = 54,
        .len = 7,
        .adr = { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32 }
    };
    BACNET_ADDRESS addr2 = {0};
    BACNET_NPDU_DATA npdu = {0};

    zassert_equal(z_cleanup_mock(), 0, NULL);
    datalink_set("mstp");

    // init
    ztest_expect_value(dlmstp_init, ifname, iface);
    ztest_returns_value(dlmstp_init, true);
    zassert_equal(datalink_init(iface), true, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    ztest_expect_value(dlmstp_init, ifname, iface2);
    ztest_returns_value(dlmstp_init, false);
    zassert_equal(datalink_init(iface2), false, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // send_pdu
    ztest_expect_value(dlmstp_send_pdu, dest, &addr);
    ztest_expect_value(dlmstp_send_pdu, npdu_data, &npdu);
    ztest_expect_data(dlmstp_send_pdu, pdu, expected_data);
    ztest_returns_value(dlmstp_send_pdu, 4);
    zassert_equal(datalink_send_pdu(&addr, &npdu, expected_data,
        sizeof(expected_data)), 4, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // receive
    ztest_expect_value(dlmstp_receive, src, &addr);
    ztest_expect_value(dlmstp_receive, timeout, 10);
    ztest_expect_data(dlmstp_receive, pdu, expected_data);
    ztest_returns_value(dlmstp_receive, 4);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 10), 4, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    memset(expected_data, 0xff, sizeof(expected_data));
    memset(data, 0x00, sizeof(data));
    ztest_expect_value(dlmstp_receive, src, &addr);
    ztest_expect_value(dlmstp_receive, timeout, 15);
    ztest_expect_data(dlmstp_receive, pdu, expected_data);
    ztest_returns_value(dlmstp_receive, 0);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 15), 0, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_broadcast_address
    ztest_expect_value(dlmstp_get_broadcast_address, dest, &addr);
    datalink_get_broadcast_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_my_address
    ztest_expect_value(dlmstp_get_my_address, my_address, &addr);
    datalink_get_my_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // set_interface - do nothing
    datalink_set_interface(iface);

    // maintenance_timer - do nothing for dlmstp
    datalink_maintenance_timer(42);
}

static void test_datalink_ethernet(void)
{
    char *iface = "bla-bla-bla";
    char *iface2 = "bla-bla-bla2";
    uint8_t expected_data[] = { 0x5A, 0xA5, 0xDE, 0xAD };
    uint8_t data[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    BACNET_ADDRESS addr = {
        .mac_len = 6, 
        .mac = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE },
        .net = 54,
        .len = 7,
        .adr = { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32 }
    };
    BACNET_ADDRESS addr2 = {0};
    BACNET_NPDU_DATA npdu = {0};

    zassert_equal(z_cleanup_mock(), 0, NULL);
    datalink_set("ethernet");

    // init
    ztest_expect_value(ethernet_init, interface_name, iface);
    ztest_returns_value(ethernet_init, true);
    zassert_equal(datalink_init(iface), true, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    ztest_expect_value(ethernet_init, interface_name, iface2);
    ztest_returns_value(ethernet_init, false);
    zassert_equal(datalink_init(iface2), false, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // send_pdu
    ztest_expect_value(ethernet_send_pdu, dest, &addr);
    ztest_expect_value(ethernet_send_pdu, npdu_data, &npdu);
    ztest_expect_data(ethernet_send_pdu, pdu, expected_data);
    ztest_returns_value(ethernet_send_pdu, 4);
    zassert_equal(datalink_send_pdu(&addr, &npdu, expected_data,
        sizeof(expected_data)), 4, NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // receive
    ztest_expect_value(ethernet_receive, src, &addr);
    ztest_expect_value(ethernet_receive, timeout, 10);
    ztest_expect_data(ethernet_receive, pdu, expected_data);
    ztest_returns_value(ethernet_receive, 4);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 10), 4, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    memset(expected_data, 0xff, sizeof(expected_data));
    memset(data, 0x00, sizeof(data));
    ztest_expect_value(ethernet_receive, src, &addr);
    ztest_expect_value(ethernet_receive, timeout, 15);
    ztest_expect_data(ethernet_receive, pdu, expected_data);
    ztest_returns_value(ethernet_receive, 0);
    zassert_equal(datalink_receive(&addr, data, sizeof(data), 15), 0, NULL);
    zassert_mem_equal(expected_data, data, sizeof(data), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_broadcast_address
    ztest_expect_value(ethernet_get_broadcast_address, dest, &addr);
    datalink_get_broadcast_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // get_my_address
    ztest_expect_value(ethernet_get_my_address, my_address, &addr);
    datalink_get_my_address(&addr2);
    zassert_mem_equal(&addr, &addr2, sizeof(addr), NULL);
    zassert_equal(z_cleanup_mock(), 0, NULL);

    // set_interface - do nothing
    datalink_set_interface(iface);

    // maintenance_timer - do nothing for ethernet
    datalink_maintenance_timer(42);
}

/**
 * @brief Test bacnet.c
 */


uint8_t Handler_Transmit_Buffer[MAX_PDU] = { 0 };

// Mock functions

bool Binary_Output_Out_Of_Service(uint32_t instance)
{
    (void)instance;
    return true;
}

BACNET_BINARY_PV Binary_Output_Present_Value(uint32_t instance)
{
    (void)instance;
    return BINARY_INACTIVE;
}

BACNET_POLARITY Binary_Output_Polarity(uint32_t instance)
{
    (void)instance;
    return POLARITY_NORMAL;
}

void Device_Init(void)
{}

void apdu_set_unrecognized_service_handler_handler(void* pFunction)
{
    (void)pFunction;
}

void apdu_set_unconfirmed_handler(BACNET_UNCONFIRMED_SERVICE service_choice,
    void* pFunction)
{
    (void)service_choice;
    (void)pFunction;
}

void apdu_set_confirmed_handler(BACNET_CONFIRMED_SERVICE service_choice,
    void* pFunction)
{
    (void)service_choice;
    (void)pFunction;
}

void handler_unrecognized_service(uint8_t * service_request,
        uint16_t service_len, BACNET_ADDRESS * dest,
        BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    (void)service_request;
    (void)service_len;
    (void)dest;
    (void)service_data;
}
void handler_who_is(uint8_t * service_request, uint16_t service_len,
        BACNET_ADDRESS * src)
{
    (void)service_request;
    (void)service_len;
    (void)src;
}
void handler_who_has(uint8_t * service_request, uint16_t service_len,
        BACNET_ADDRESS * src)
{
    (void)service_request;
    (void)service_len;
    (void)src;
}
void handler_read_property(uint8_t *service_request,
    uint16_t service_len, BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    (void)service_request;
    (void)service_len;
    (void)src;
    (void)service_data;
}
void handler_read_property_multiple(uint8_t * service_request,
        uint16_t service_len, BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    (void)service_request;
    (void)service_len;
    (void)src;
    (void)service_data;
}
void handler_reinitialize_device( uint8_t * service_request,
        uint16_t service_len, BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    (void)service_request;
    (void)service_len;
    (void)src;
    (void)service_data;
}
void handler_write_property(uint8_t * service_request,
        uint16_t service_len, BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    (void)service_request;
    (void)service_len;
    (void)src;
    (void)service_data;
}
void handler_device_communication_control(uint8_t * service_request,
        uint16_t service_len, BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    (void)service_request;
    (void)service_len;
    (void)src;
    (void)service_data;
}

void Send_I_Am(uint8_t *buffer)
{
    (void)buffer;
}

unsigned long mstimer_now(void)
{
    static unsigned long l = 1;
    return l++;
}

void dcc_timer_seconds(uint32_t seconds)
{
    ztest_check_expected_value(seconds);
}

void npdu_handler(BACNET_ADDRESS *src, uint8_t *pdu, uint16_t pdu_len)
{
    ztest_check_expected_value(pdu_len);
    ztest_check_expected_data(pdu, pdu_len);
}

//  test

static void test_bacnet_task(void)
{
    uint8_t expected_data[MAX_MPDU] = { 0x5A, 0xA5, 0xDE, 0xAD };
    uint8_t expected_data2[MAX_MPDU] = { 0xAA, 0xBB, 0xCC, 0xDD };

    check_arcnet_receive_src = false;
    zassert_equal(z_cleanup_mock(), 0, NULL);
    datalink_set("arcnet");

    ztest_expect_value(arcnet_receive, timeout, 0);
    ztest_expect_data(arcnet_receive, pdu, expected_data);
    ztest_returns_value(arcnet_receive, 4);
    ztest_expect_value(npdu_handler, pdu_len, 4);
    ztest_expect_data(npdu_handler, pdu, expected_data);
    bacnet_task();
    zassert_equal(z_cleanup_mock(), 0, NULL);

    ztest_expect_value(arcnet_receive, timeout, 0);
    ztest_expect_data(arcnet_receive, pdu, expected_data2);
    ztest_returns_value(arcnet_receive, 0);
    bacnet_task();
    zassert_equal(z_cleanup_mock(), 0, NULL);
}

/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(datalink_tests,
     ztest_unit_test(test_datalink_arcnet),
     ztest_unit_test(test_datalink_bip),
     ztest_unit_test(test_datalink_bip6),
     ztest_unit_test(test_datalink_dlmstp),
     ztest_unit_test(test_datalink_ethernet),
     ztest_unit_test(test_bacnet_task)
     );

    ztest_run_test_suite(datalink_tests);
}
