/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2024
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/readrange.h>
#include <bacnet/basic/object/netport.h>
#include <bacnet/basic/object/sc_netport.h>
#include <bacnet/basic/object/bacfile.h>
#include <bacfile-posix.h>
#include <property_test.h>
#ifndef BACDL_BSC
#define BACDL_BSC
#endif
/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(netport_tests, test_network_port)
#else
static void test_network_port(void)
#endif
{
    unsigned port = 0;
    bool status = false;
    unsigned count = 0;
    uint8_t address[16] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
    };
    uint8_t test_address[16] = { 0 };
    uint8_t mac_len;
    uint8_t max_master, max_info_frames;
    uint8_t ip_prefix;
    uint32_t object_instance = 1234, test_object_instance = 0;
    unsigned object_index = 0, test_object_index = 0;
    BACNET_HOST_N_PORT host_n_port, test_host_n_port = { 0 };
    BACNET_OCTET_STRING ip_address;
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY bdt[2] = {
        { .valid = true,
          .dest_address = { .address = { 1, 2, 3, 4 }, .port = 47808 },
          .broadcast_mask = { .address = { 255, 255, 255, 255 } } },
        { .valid = true,
          .dest_address = { .address = { 5, 6, 7, 8 }, .port = 47808 },
          .broadcast_mask = { .address = { 255, 255, 255, 0 } } }
    };
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *test_bdt;
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY fdt[2] = {
        { .valid = true,
          .dest_address = { .address = { 1, 2, 3, 4 }, .port = 47808 },
          .ttl_seconds = 30,
          .ttl_seconds_remaining = 30 },
        { .valid = true,
          .dest_address = { .address = { 5, 6, 7, 8 }, .port = 47808 },
          .ttl_seconds = 60,
          .ttl_seconds_remaining = 60 }
    };
    BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *test_fdt;
    uint8_t port_type[] = { PORT_TYPE_ETHERNET,   PORT_TYPE_ARCNET,
                            PORT_TYPE_MSTP,       PORT_TYPE_PTP,
                            PORT_TYPE_LONTALK,    PORT_TYPE_BIP,
                            PORT_TYPE_ZIGBEE,     PORT_TYPE_VIRTUAL,
                            PORT_TYPE_NON_BACNET, PORT_TYPE_BIP6,
                            PORT_TYPE_BSC,        PORT_TYPE_MAX };
    const int32_t known_fail_property_list[] = { -1 };

    while (port_type[port] != PORT_TYPE_MAX) {
        Network_Port_Init();
        status = Network_Port_Object_Instance_Number_Set(
            object_index, object_instance);
        zassert_true(status, NULL);
        test_object_index = Network_Port_Instance_To_Index(object_instance);
        zassert_equal(object_index, test_object_index, NULL);
        test_object_instance = Network_Port_Index_To_Instance(object_index);
        zassert_equal(object_instance, test_object_instance, NULL);
        test_object_instance = Network_Port_Index_To_Instance(UINT_MAX);
        zassert_equal(test_object_instance, BACNET_MAX_INSTANCE, NULL);
        status = Network_Port_Type_Set(object_instance, port_type[port]);
        zassert_true(status, NULL);
        count = Network_Port_Count();
        zassert_true(count > 0, NULL);
        status = Network_Port_Description_Set(object_instance, "Test Port");
        zassert_true(status, NULL);
        status = Network_Port_Out_Of_Service_Set(object_instance, true);
        zassert_true(status, NULL);
        status = Network_Port_Out_Of_Service(object_instance);
        zassert_true(status, NULL);
        status = Network_Port_Out_Of_Service_Set(object_instance, false);
        zassert_true(status, NULL);
        status = Network_Port_Out_Of_Service(object_instance);
        zassert_false(status, NULL);
        status = Network_Port_Reliability_Set(
            object_instance, RELIABILITY_NO_FAULT_DETECTED);
        zassert_true(status, NULL);
        status = Network_Port_Network_Number_Set(object_instance, 0);
        zassert_true(status, NULL);
        status =
            Network_Port_Quality_Set(object_instance, PORT_QUALITY_UNKNOWN);
        zassert_true(status, NULL);
        status = Network_Port_MAC_Address_Set(object_instance, NULL, 0);
        zassert_false(status, NULL);
        mac_len = Network_Port_MAC_Address_Value(object_instance, NULL, 0);
        if (mac_len > 0) {
            /* test for ports that have a MAC address */
            zassert_not_equal(mac_len, 0, NULL);
            status =
                Network_Port_MAC_Address_Set(object_instance, address, mac_len);
            zassert_true(status, NULL);
            zassert_equal(
                Network_Port_MAC_Address_Value(
                    object_instance, test_address, sizeof(test_address)),
                mac_len, NULL);
            zassert_mem_equal(test_address, address, mac_len, NULL);
        }
        status = Network_Port_APDU_Length_Set(object_instance, MAX_APDU);
        zassert_true(status, NULL);
        status = Network_Port_Link_Speed_Set(object_instance, 0);
        zassert_true(status, NULL);
        status = Network_Port_Changes_Pending_Set(object_instance, false);
        zassert_true(status, NULL);
        Network_Port_Changes_Pending_Activate_Callback_Set(
            object_instance, NULL);
        Network_Port_Changes_Pending_Discard(object_instance);
        Network_Port_Changes_Pending_Discard_Callback_Set(
            object_instance, NULL);
        if (port_type[port] == PORT_TYPE_MSTP) {
            status = Network_Port_MSTP_MAC_Address_Set(object_instance, 127);
            zassert_true(status, NULL);
            address[0] = Network_Port_MSTP_MAC_Address(object_instance);
            zassert_equal(address[0], 127, NULL);
            status = Network_Port_MSTP_Max_Info_Frames_Set(object_instance, 1);
            zassert_true(status, NULL);
            max_info_frames =
                Network_Port_MSTP_Max_Info_Frames(object_instance);
            zassert_equal(max_info_frames, 1, NULL);
            Network_Port_MSTP_Max_Master_Set(object_instance, 127);
            max_master = Network_Port_MSTP_Max_Master(object_instance);
            zassert_equal(max_master, 127, NULL);
            Network_Port_Changes_Pending_Set(object_instance, false);
            status = Network_Port_MSTP_Max_Master_Set(object_instance, 63);
            zassert_true(status, NULL);
            max_master = Network_Port_MSTP_Max_Master(object_instance);
            zassert_equal(max_master, 63, NULL);
            zassert_true(Network_Port_Changes_Pending(object_instance), NULL);
        }
        if (port_type[port] == PORT_TYPE_BIP) {
            status = Network_Port_IP_Address_Set(object_instance, 1, 2, 3, 4);
            zassert_true(status, NULL);
            status = Network_Port_IP_Subnet_Prefix_Set(object_instance, 24);
            zassert_true(status, NULL);
            ip_prefix = Network_Port_IP_Subnet_Prefix(object_instance);
            zassert_equal(ip_prefix, 24, NULL);
            status = Network_Port_IP_Gateway_Set(object_instance, 5, 6, 7, 8);
            zassert_true(status, NULL);
            status = Network_Port_IP_DHCP_Enable_Set(object_instance, true);
            zassert_true(status, NULL);
            status = Network_Port_IP_DHCP_Enable(object_instance);
            zassert_true(status, NULL);
            Network_Port_IP_DHCP_Lease_Time_Set(object_instance, 60);
            zassert_equal(
                Network_Port_IP_DHCP_Lease_Time(object_instance), 60, NULL);
            zassert_equal(
                Network_Port_IP_DHCP_Lease_Time_Remaining(object_instance), 60,
                NULL);
            octetstring_init_ascii_hex(&ip_address, "0x0A0B0C0D");
            status =
                Network_Port_IP_DHCP_Server_Set(object_instance, &ip_address);
            zassert_true(status, NULL);
            status = Network_Port_IP_DNS_Server_Set(
                object_instance, 0, 9, 10, 11, 12);
            zassert_true(status, NULL);
            status = Network_Port_BIP_Port_Set(object_instance, 47808);
            zassert_true(status, NULL);
            status = Network_Port_BIP_Mode_Set(
                object_instance, BACNET_IP_MODE_NORMAL);
            zassert_true(status, NULL);
            zassert_equal(
                Network_Port_BIP_Mode(object_instance), BACNET_IP_MODE_NORMAL,
                NULL);
            status = Network_Port_BBMD_Accept_FD_Registrations_Set(
                object_instance, true);
            zassert_true(status, NULL);
            status = Network_Port_BBMD_BD_Table_Set(object_instance, NULL);
            zassert_true(status, NULL);
            zassert_is_null(Network_Port_BBMD_BD_Table(object_instance), NULL);
            bvlc_broadcast_distribution_table_link_array(bdt, ARRAY_SIZE(bdt));
            status = Network_Port_BBMD_BD_Table_Set(object_instance, bdt);
            zassert_true(status, NULL);
            test_bdt = Network_Port_BBMD_BD_Table(object_instance);
            zassert_not_null(test_bdt, NULL);
            zassert_equal(
                test_bdt->dest_address.address[0],
                bdt[0].dest_address.address[0], NULL);
            status = Network_Port_BBMD_FD_Table_Set(object_instance, NULL);
            zassert_true(status, NULL);
            zassert_is_null(Network_Port_BBMD_FD_Table(object_instance), NULL);
            bvlc_foreign_device_table_link_array(fdt, ARRAY_SIZE(fdt));
            status = Network_Port_BBMD_FD_Table_Set(object_instance, &fdt[0]);
            zassert_true(status, NULL);
            test_fdt = Network_Port_BBMD_FD_Table(object_instance);
            zassert_not_null(test_fdt, NULL);
            zassert_equal(
                test_fdt->dest_address.address[0],
                fdt[0].dest_address.address[0], NULL);
            status = Network_Port_Remote_BBMD_IP_Address(
                object_instance, NULL, NULL, NULL, NULL);
            zassert_false(status, NULL);
            status = Network_Port_Remote_BBMD_IP_Address_Set(
                object_instance, 1, 2, 3, 4);
            zassert_true(status, NULL);
            status = Network_Port_Remote_BBMD_IP_Address(
                object_instance, &test_address[0], &test_address[1],
                &test_address[2], &test_address[3]);
            zassert_true(status, NULL);
            status = Network_Port_Remote_BBMD_IP_Address(
                object_instance, NULL, NULL, NULL, NULL);
            zassert_true(status, NULL);
            zassert_equal(test_address[0], 1, NULL);
            zassert_equal(test_address[1], 2, NULL);
            zassert_equal(test_address[2], 3, NULL);
            zassert_equal(test_address[3], 4, NULL);
            Network_Port_Changes_Pending_Set(object_instance, false);
            status = Network_Port_Remote_BBMD_IP_Address_Set(
                object_instance, 5, 6, 7, 8);
            zassert_true(status, NULL);
            zassert_true(Network_Port_Changes_Pending(object_instance), NULL);
            status =
                Network_Port_Remote_BBMD_BIP_Port_Set(object_instance, 47808);
            zassert_true(status, NULL);
            zassert_equal(
                Network_Port_Remote_BBMD_BIP_Port(object_instance), 47808,
                NULL);
            Network_Port_Remote_BBMD_Address(object_instance, &host_n_port);
            zassert_false(host_n_port.host_name, NULL);
            zassert_true(host_n_port.host_ip_address, NULL);
            zassert_equal(host_n_port.port, 47808, NULL);
            test_host_n_port.host_ip_address = false;
            test_host_n_port.host_name = true;
            characterstring_init_ansi(
                &test_host_n_port.host.name, "bbmd.example.com");
            test_host_n_port.port = 47808;
            Network_Port_Remote_BBMD_Address_Set(
                object_instance, &test_host_n_port);
            Network_Port_Remote_BBMD_Address(object_instance, &host_n_port);
            zassert_false(host_n_port.host_ip_address, NULL);
            zassert_true(host_n_port.host_name, NULL);
            zassert_equal(host_n_port.port, 47808, NULL);
            characterstring_ansi_same(
                &test_host_n_port.host.name, "bbmd.example.com");
            status =
                Network_Port_Remote_BBMD_BIP_Lifetime_Set(object_instance, 60);
            zassert_true(status, NULL);
            zassert_equal(
                Network_Port_Remote_BBMD_BIP_Lifetime(object_instance), 60,
                NULL);
        }
        if (port_type[port] == PORT_TYPE_BIP6) {
            status = Network_Port_IPv6_Address_Set(object_instance, address);
            zassert_true(status, NULL);
            status = Network_Port_BBMD_IP6_Accept_FD_Registrations_Set(
                object_instance, true);
            zassert_true(status, NULL);
            zassert_true(
                Network_Port_BBMD_IP6_Accept_FD_Registrations(object_instance),
                NULL);
            status = Network_Port_BBMD_IP6_BD_Table_Set(object_instance, NULL);
            zassert_true(status, NULL);
            zassert_is_null(
                Network_Port_BBMD_IP6_BD_Table(object_instance), NULL);
            status = Network_Port_BBMD_IP6_FD_Table_Set(object_instance, NULL);
            zassert_true(status, NULL);
            zassert_is_null(
                Network_Port_BBMD_IP6_FD_Table(object_instance), NULL);
            status =
                Network_Port_Remote_BBMD_IP6_Address(object_instance, address);
            zassert_false(status, NULL);
            status = Network_Port_Remote_BBMD_IP6_Address_Set(
                object_instance, address);
            zassert_true(status, NULL);
            status =
                Network_Port_Remote_BBMD_BIP6_Port_Set(object_instance, 47808);
            zassert_true(status, NULL);
            status =
                Network_Port_Remote_BBMD_IP6_Address(object_instance, address);
            zassert_true(status, NULL);
            zassert_equal(
                Network_Port_Remote_BBMD_BIP6_Port(object_instance), 47808,
                NULL);
            Network_Port_Remote_BBMD_Address(object_instance, &host_n_port);
            zassert_true(host_n_port.host_ip_address, NULL);
            zassert_equal(host_n_port.port, 47808, NULL);
            test_host_n_port.host_ip_address = false;
            test_host_n_port.host_name = true;
            characterstring_init_ansi(
                &test_host_n_port.host.name, "bbmd.example.com");
            test_host_n_port.port = 47808;
            Network_Port_Remote_BBMD_Address_Set(
                object_instance, &test_host_n_port);
            Network_Port_Remote_BBMD_Address(object_instance, &host_n_port);
            zassert_false(host_n_port.host_ip_address, NULL);
            zassert_true(host_n_port.host_name, NULL);
            zassert_equal(host_n_port.port, 47808, NULL);
            status =
                Network_Port_Remote_BBMD_BIP6_Lifetime_Set(object_instance, 60);
            zassert_true(status, NULL);
            zassert_equal(
                Network_Port_Remote_BBMD_BIP6_Lifetime(object_instance), 60,
                NULL);
            status = Network_Port_BIP6_Mode_Set(
                object_instance, BACNET_IP_MODE_NORMAL);
            zassert_true(status, NULL);
            zassert_equal(
                Network_Port_BIP6_Mode(object_instance), BACNET_IP_MODE_NORMAL,
                NULL);
            status = Network_Port_IPv6_Address_Set(object_instance, address);
            zassert_true(status, NULL);
            status = Network_Port_IPv6_Subnet_Prefix_Set(object_instance, 24);
            zassert_true(status, NULL);
            status = Network_Port_IPv6_Gateway_Set(object_instance, address);
            zassert_true(status, NULL);
            status =
                Network_Port_IPv6_DNS_Server_Set(object_instance, 0, address);
            zassert_true(status, NULL);
            status = Network_Port_IPv6_Multicast_Address_Set(
                object_instance, address);
            zassert_true(status, NULL);
            status = Network_Port_IP_DHCP_Enable_Set(object_instance, true);
            zassert_true(status, NULL);
            status = Network_Port_IP_DHCP_Enable(object_instance);
            zassert_true(status, NULL);
            Network_Port_IP_DHCP_Lease_Time_Set(object_instance, 60);
            zassert_equal(
                Network_Port_IP_DHCP_Lease_Time(object_instance), 60, NULL);
            zassert_equal(
                Network_Port_IP_DHCP_Lease_Time_Remaining(object_instance), 60,
                NULL);
            status =
                Network_Port_IPv6_DHCP_Server_Set(object_instance, address);
            octetstring_init(&ip_address, address, sizeof(address));
            status =
                Network_Port_IP_DHCP_Server_Set(object_instance, &ip_address);
            zassert_true(status, NULL);
            status = Network_Port_BIP6_Port_Set(object_instance, 47808);
            zassert_true(status, NULL);
            zassert_equal(Network_Port_BIP6_Port(object_instance), 47808, NULL);
            status = Network_Port_IPv6_Gateway_Zone_Index_Set(
                object_instance, "eth0");
            zassert_true(status, NULL);
            zassert_equal(
                strcmp(
                    Network_Port_IPv6_Zone_Index_ASCII(object_instance),
                    "eth0"),
                0, NULL);
            status = Network_Port_IPv6_Auto_Addressing_Enable_Set(
                object_instance, true);
            zassert_true(status, NULL);
            zassert_true(
                Network_Port_IPv6_Auto_Addressing_Enable(object_instance),
                NULL);
        }
        /* generic R/W property test */
        bacnet_object_properties_read_write_test(
            OBJECT_NETWORK_PORT, object_instance, Network_Port_Property_Lists,
            Network_Port_Read_Property, Network_Port_Write_Property,
            known_fail_property_list);
        bacnet_object_name_ascii_test(
            object_instance, Network_Port_Name_Set,
            Network_Port_Object_Name_ASCII);
        port++;
        Network_Port_Changes_Activate();
        zassert_false(Network_Port_Changes_Pending(object_instance), NULL);
        Network_Port_Changes_Pending_Set(object_instance, true);
        Network_Port_Changes_Discard();
        zassert_false(Network_Port_Changes_Pending(object_instance), NULL);
        Network_Port_Cleanup();
    }

    return;
}
/**
 * @}
 */

static void test_network_port_pending_param(void)
{
#ifdef BACDL_BSC
    /* for decode value data */
    unsigned count = 0;
    uint32_t object_instance = 0;
    bool status = false;
#if (BSC_CONF_HUB_FUNCTIONS_NUM != 0) || (BSC_CONF_HUB_CONNECTORS_NUM != 0)
    bool val;
    BACNET_CHARACTER_STRING str;
#endif
#define PRIMARY_HUB_URL1 "SC_Primary_Hub_URI_test"
#define PRIMARY_HUB_URL2 "SC_Primary_Hub_URI_test2"
#define FAILOVER_HUB_URL1 "SC_Failover_Hub_URI_test"
#define FAILOVER_HUB_URL2 "SC_Failover_Hub_URI_test2"
#define HUB_BINDING1 "SC_Hub_Function_Binding_test:12345"
#define HUB_BINDING2 "SC_Hub_Function_Binding_test2:11111"
#define DIRECT_BINDING1 "SC_Direct_Connect_Binding_test:54321"
#define DIRECT_BINDING2 "SC_Direct_Connect_Binding_test2:22222"
#define DIRECT_BINDING1 "SC_Direct_Connect_Binding_test:54321"
#define DIRECT_BINDING2 "SC_Direct_Connect_Binding_test2:22222"
#define URL1 "SC_Direct_Connect_Accept_URI1"
#define URL2 "SC_Direct_Connect_Accept_URI2"

    Network_Port_Init();
    object_instance = 1234;
    status = Network_Port_Object_Instance_Number_Set(0, object_instance);
    zassert_true(status, NULL);
    status = Network_Port_Type_Set(object_instance, PORT_TYPE_BSC);
    zassert_true(status, NULL);
    count = Network_Port_Count();
    zassert_true(count > 0, NULL);

    Network_Port_Max_BVLC_Length_Accepted_Set(object_instance, 1200);
    Network_Port_Max_NPDU_Length_Accepted_Set(object_instance, 1100);
    Network_Port_SC_Minimum_Reconnect_Time_Set(object_instance, 2);
    Network_Port_SC_Maximum_Reconnect_Time_Set(object_instance, 5);
    Network_Port_SC_Connect_Wait_Timeout_Set(object_instance, 40);
    Network_Port_SC_Disconnect_Wait_Timeout_Set(object_instance, 10);
    Network_Port_SC_Heartbeat_Timeout_Set(object_instance, 3);

    // write properties
#if BSC_CONF_HUB_FUNCTIONS_NUM != 0
    Network_Port_SC_Primary_Hub_URI_Set(object_instance, PRIMARY_HUB_URL1);
    Network_Port_SC_Failover_Hub_URI_Set(object_instance, FAILOVER_HUB_URL1);
    Network_Port_SC_Hub_Function_Enable_Set(object_instance, false);
    Network_Port_SC_Hub_Function_Binding_Set(object_instance, HUB_BINDING1);
    Network_Port_SC_Hub_Function_Accept_URI_Set(object_instance, 0, URL1);
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM != 0
    Network_Port_SC_Direct_Connect_Initiate_Enable_Set(object_instance, false);
    Network_Port_SC_Direct_Connect_Accept_Enable_Set(object_instance, false);
    Network_Port_SC_Direct_Connect_Binding_Set(
        object_instance, DIRECT_BINDING1);
    Network_Port_SC_Direct_Connect_Accept_URI_Set(object_instance, 0, URL1);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

    // write dirty properties
    Network_Port_Max_BVLC_Length_Accepted_Dirty_Set(object_instance, 1250);
    Network_Port_Max_NPDU_Length_Accepted_Dirty_Set(object_instance, 1150);
    Network_Port_SC_Minimum_Reconnect_Time_Dirty_Set(object_instance, 3);
    Network_Port_SC_Maximum_Reconnect_Time_Dirty_Set(object_instance, 4);
    Network_Port_SC_Connect_Wait_Timeout_Dirty_Set(object_instance, 50);
    Network_Port_SC_Disconnect_Wait_Timeout_Dirty_Set(object_instance, 15);
    Network_Port_SC_Heartbeat_Timeout_Dirty_Set(object_instance, 6);

#if BSC_CONF_HUB_FUNCTIONS_NUM != 0
    Network_Port_SC_Primary_Hub_URI_Dirty_Set(
        object_instance, PRIMARY_HUB_URL2);
    Network_Port_SC_Failover_Hub_URI_Dirty_Set(
        object_instance, FAILOVER_HUB_URL2);
    Network_Port_SC_Hub_Function_Enable_Dirty_Set(object_instance, true);
    Network_Port_SC_Hub_Function_Binding_Dirty_Set(
        object_instance, HUB_BINDING2);
    Network_Port_SC_Hub_Function_Accept_URI_Dirty_Set(object_instance, 0, URL2);
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM != 0
    Network_Port_SC_Direct_Connect_Initiate_Enable_Dirty_Set(
        object_instance, true);
    Network_Port_SC_Direct_Connect_Accept_Enable_Dirty_Set(
        object_instance, true);
    Network_Port_SC_Direct_Connect_Binding_Dirty_Set(
        object_instance, DIRECT_BINDING2);
    Network_Port_SC_Direct_Connect_Accept_URI_Dirty_Set(
        object_instance, 0, URL2);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

    // check old value
    zassert_true(
        Network_Port_Max_BVLC_Length_Accepted(object_instance) == 1200, NULL);
    zassert_true(
        Network_Port_Max_NPDU_Length_Accepted(object_instance) == 1100, NULL);
    zassert_true(
        Network_Port_SC_Minimum_Reconnect_Time(object_instance) == 2, NULL);
    zassert_true(
        Network_Port_SC_Maximum_Reconnect_Time(object_instance) == 5, NULL);
    zassert_true(
        Network_Port_SC_Connect_Wait_Timeout(object_instance) == 40, NULL);
    zassert_true(
        Network_Port_SC_Disconnect_Wait_Timeout(object_instance) == 10, NULL);
    zassert_true(Network_Port_SC_Heartbeat_Timeout(object_instance) == 3, NULL);

#if BSC_CONF_HUB_FUNCTIONS_NUM != 0
    Network_Port_SC_Primary_Hub_URI(object_instance, &str);
    zassert_true(
        strncmp(
            characterstring_value(&str), PRIMARY_HUB_URL1,
            characterstring_length(&str)) == 0,
        NULL);
    Network_Port_SC_Failover_Hub_URI(object_instance, &str);
    zassert_true(
        strncmp(
            characterstring_value(&str), FAILOVER_HUB_URL1,
            characterstring_length(&str)) == 0,
        NULL);
    val = Network_Port_SC_Hub_Function_Enable(object_instance);
    zassert_true(val == false, NULL);
    Network_Port_SC_Hub_Function_Binding(object_instance, &str);
    zassert_true(
        strncmp(
            characterstring_value(&str), HUB_BINDING1,
            characterstring_length(&str)) == 0,
        NULL);
    Network_Port_SC_Hub_Function_Accept_URI(object_instance, 0, &str);
    zassert_true(
        strncmp(
            characterstring_value(&str), URL1, characterstring_length(&str)) ==
            0,
        NULL);
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM != 0
    val = Network_Port_SC_Direct_Connect_Initiate_Enable(object_instance);
    zassert_true(val == false, NULL);
    val = Network_Port_SC_Direct_Connect_Accept_Enable(object_instance);
    zassert_true(val == false, NULL);
    Network_Port_SC_Direct_Connect_Binding(object_instance, &str);
    zassert_true(
        strncmp(
            characterstring_value(&str), DIRECT_BINDING1,
            characterstring_length(&str)) == 0,
        NULL);
    Network_Port_SC_Direct_Connect_Accept_URI(object_instance, 0, &str);
    zassert_true(
        strncmp(
            characterstring_value(&str), URL1, characterstring_length(&str)) ==
            0,
        NULL);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

    // apply
    Network_Port_Changes_Pending_Activate(object_instance);

    // check new value
    zassert_true(
        Network_Port_Max_BVLC_Length_Accepted(object_instance) == 1250, NULL);
    zassert_true(
        Network_Port_Max_NPDU_Length_Accepted(object_instance) == 1150, NULL);
    zassert_true(
        Network_Port_SC_Minimum_Reconnect_Time(object_instance) == 3, NULL);
    zassert_true(
        Network_Port_SC_Maximum_Reconnect_Time(object_instance) == 4, NULL);
    zassert_true(
        Network_Port_SC_Connect_Wait_Timeout(object_instance) == 50, NULL);
    zassert_true(
        Network_Port_SC_Disconnect_Wait_Timeout(object_instance) == 15, NULL);
    zassert_true(Network_Port_SC_Heartbeat_Timeout(object_instance) == 6, NULL);

#if BSC_CONF_HUB_FUNCTIONS_NUM != 0
    Network_Port_SC_Primary_Hub_URI(object_instance, &str);
    zassert_true(
        strncmp(
            characterstring_value(&str), PRIMARY_HUB_URL2,
            characterstring_length(&str)) == 0,
        NULL);
    Network_Port_SC_Failover_Hub_URI(object_instance, &str);
    zassert_true(
        strncmp(
            characterstring_value(&str), FAILOVER_HUB_URL2,
            characterstring_length(&str)) == 0,
        NULL);
    val = Network_Port_SC_Hub_Function_Enable(object_instance);
    zassert_true(val == true, NULL);
    Network_Port_SC_Hub_Function_Binding(object_instance, &str);
    zassert_true(
        strncmp(
            characterstring_value(&str), HUB_BINDING2,
            characterstring_length(&str)) == 0,
        NULL);
    Network_Port_SC_Hub_Function_Accept_URI(object_instance, 0, &str);
    zassert_true(
        strncmp(
            characterstring_value(&str), URL2, characterstring_length(&str)) ==
            0,
        NULL);
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM != 0
    val = Network_Port_SC_Direct_Connect_Initiate_Enable(object_instance);
    zassert_true(val == true, NULL);
    val = Network_Port_SC_Direct_Connect_Accept_Enable(object_instance);
    zassert_true(val == true, NULL);
    Network_Port_SC_Direct_Connect_Binding(object_instance, &str);
    zassert_true(
        strncmp(
            characterstring_value(&str), DIRECT_BINDING2,
            characterstring_length(&str)) == 0,
        NULL);
    Network_Port_SC_Direct_Connect_Accept_URI(object_instance, 0, &str);
    zassert_true(
        strncmp(
            characterstring_value(&str), URL2, characterstring_length(&str)) ==
            0,
        NULL);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

#endif /* BACDL_BSC */
    return;
}

static void test_network_port_sc_direct_connect_accept_uri(void)
{
#if defined(BACDL_BSC) && (BSC_CONF_HUB_CONNECTORS_NUM != 0)
#define URL1 "SC_Direct_Connect_Accept_URI1"
#define URL2 "SC_Direct_Connect_Accept_URI2"
#define URL3 "SC_Direct_Connect_Accept_URI3"
#define URL_SMALL "bla-bla-bla"
#define URL_BIG "big-bla-big-bla-big-bla-big-bla-big-bla"
    char urls0[] = URL1 " " URL2 " " URL3;
    char urls1[] = URL1 " " URL_SMALL " " URL3;
    char urls2[] = URL1 " " URL_BIG " " URL3;
    char urls3[] = URL1 " " URL_BIG " " URL3 " " URL_SMALL;
    char urls4[] = URL1 " " URL_BIG " " URL3 " " URL_SMALL " " URL_BIG;

    unsigned count = 0;
    uint32_t object_instance = 0;
    bool status = false;
    BACNET_CHARACTER_STRING str = { 0 };

    Network_Port_Init();
    object_instance = 1234;
    status = Network_Port_Object_Instance_Number_Set(0, object_instance);
    zassert_true(status, NULL);
    count = Network_Port_Count();
    zassert_true(count > 0, NULL);

    // init
    Network_Port_SC_Direct_Connect_Accept_URIs_Set(object_instance, urls0);
    zassert_true(
        strcmp(
            Network_Port_SC_Direct_Connect_Accept_URIs_char(object_instance),
            urls0) == 0,
        NULL);

    zassert_true(
        Network_Port_SC_Direct_Connect_Accept_URI(object_instance, 0, &str),
        NULL);
    zassert_true(
        strncmp(
            characterstring_value(&str), URL1, characterstring_length(&str)) ==
            0,
        NULL);

    zassert_true(
        Network_Port_SC_Direct_Connect_Accept_URI(object_instance, 1, &str),
        NULL);
    zassert_true(
        strncmp(
            characterstring_value(&str), URL2, characterstring_length(&str)) ==
            0,
        NULL);

    zassert_true(
        Network_Port_SC_Direct_Connect_Accept_URI(object_instance, 2, &str),
        NULL);
    zassert_true(
        strncmp(
            characterstring_value(&str), URL3, characterstring_length(&str)) ==
            0,
        NULL);

    // change
    zassert_true(
        Network_Port_SC_Direct_Connect_Accept_URI_Set(
            object_instance, 1, URL_SMALL),
        NULL);
    zassert_true(
        strcmp(
            Network_Port_SC_Direct_Connect_Accept_URIs_char(object_instance),
            urls1) == 0,
        NULL);

    zassert_true(
        Network_Port_SC_Direct_Connect_Accept_URI_Set(
            object_instance, 1, URL_BIG),
        NULL);
    zassert_true(
        strcmp(
            Network_Port_SC_Direct_Connect_Accept_URIs_char(object_instance),
            urls2) == 0,
        NULL);

    // append
    zassert_true(
        Network_Port_SC_Direct_Connect_Accept_URI_Set(
            object_instance, 4, URL_SMALL),
        NULL);
    zassert_true(
        strcmp(
            Network_Port_SC_Direct_Connect_Accept_URIs_char(object_instance),
            urls3) == 0,
        NULL);

    zassert_true(
        Network_Port_SC_Direct_Connect_Accept_URI_Set(
            object_instance, 4, URL_BIG),
        NULL);
    zassert_true(
        strcmp(
            Network_Port_SC_Direct_Connect_Accept_URIs_char(object_instance),
            urls4) == 0,
        NULL);

    // check last
    zassert_true(
        Network_Port_SC_Direct_Connect_Accept_URI(object_instance, 0, &str),
        NULL);
    zassert_true(strcmp(characterstring_value(&str), URL1) == 0, NULL);

    zassert_true(
        Network_Port_SC_Direct_Connect_Accept_URI(object_instance, 1, &str),
        NULL);
    zassert_true(strcmp(characterstring_value(&str), URL_BIG) == 0, NULL);

    zassert_true(
        Network_Port_SC_Direct_Connect_Accept_URI(object_instance, 2, &str),
        NULL);
    zassert_true(strcmp(characterstring_value(&str), URL3) == 0, NULL);

    zassert_true(
        Network_Port_SC_Direct_Connect_Accept_URI(object_instance, 3, &str),
        NULL);
    zassert_true(strcmp(characterstring_value(&str), URL_SMALL) == 0, NULL);

    zassert_true(
        Network_Port_SC_Direct_Connect_Accept_URI(object_instance, 4, &str),
        NULL);
    zassert_true(
        strncmp(
            characterstring_value(&str), URL_BIG,
            characterstring_length(&str)) == 0,
        NULL);

#endif /* BACDL_BSC && BSC_CONF_HUB_CONNECTORS_NUM!=0 */

    return;
}

#define BACFILE_START 0

static void test_network_port_sc_certificates(void)
{
#ifdef BACDL_BSC
    unsigned count = 0;
    uint32_t instance = 0;
    uint32_t file_instance = 0;
    bool status = false;
    char *filename_ca_cert = "ca_cert.pem";
    char *filename_cert = "cert.pem";
    char *filename_key = "key.pem";

    Network_Port_Init();
    instance = 1234;
    status = Network_Port_Object_Instance_Number_Set(0, instance);
    zassert_true(status, NULL);
    count = Network_Port_Count();
    zassert_true(count > 0, NULL);

    bacfile_init();
    bacfile_posix_init();
    // CA certificate
    status = bacfile_create(BSC_ISSUER_CERTIFICATE_FILE_1_INSTANCE);
    zassert_true(status, NULL);
    bacfile_pathname_set(
        BSC_ISSUER_CERTIFICATE_FILE_1_INSTANCE, filename_ca_cert);
    status = Network_Port_Issuer_Certificate_File_Set(
        instance, 0, BSC_ISSUER_CERTIFICATE_FILE_1_INSTANCE);
    zassert_true(status, NULL);
    file_instance = Network_Port_Issuer_Certificate_File(instance, 0);
    zassert_equal(file_instance, BSC_ISSUER_CERTIFICATE_FILE_1_INSTANCE, NULL);

    status = bacfile_create(BSC_OPERATIONAL_CERTIFICATE_FILE_INSTANCE);
    zassert_true(status, NULL);
    bacfile_pathname_set(
        BSC_OPERATIONAL_CERTIFICATE_FILE_INSTANCE, filename_cert);
    status = Network_Port_Operational_Certificate_File_Set(
        instance, BSC_OPERATIONAL_CERTIFICATE_FILE_INSTANCE);
    zassert_true(status, NULL);
    file_instance = Network_Port_Operational_Certificate_File(instance);
    zassert_equal(
        file_instance, BSC_OPERATIONAL_CERTIFICATE_FILE_INSTANCE, NULL);

    status = bacfile_create(BSC_CERTIFICATE_SIGNING_REQUEST_FILE_INSTANCE);
    zassert_true(status, NULL);
    bacfile_pathname_set(
        BSC_CERTIFICATE_SIGNING_REQUEST_FILE_INSTANCE, filename_key);
    status = Network_Port_Certificate_Key_File_Set(
        instance, BSC_CERTIFICATE_SIGNING_REQUEST_FILE_INSTANCE);
    zassert_true(status, NULL);
    file_instance = Network_Port_Certificate_Key_File(instance);
    zassert_equal(
        file_instance, BSC_CERTIFICATE_SIGNING_REQUEST_FILE_INSTANCE, NULL);

    // reset

    // check bacfile after reset

#endif /* BACDL_BSC */

    return;
}

static void test_network_port_sc_status_encode_decode(void)
{
#ifdef BACDL_BSC
    unsigned count = 0;
    uint32_t instance = 0;
    bool status = false;
    BACNET_DATE_TIME ts = { { 2202, 5, 7, 3 }, { 12, 34, 22, 10 } };
    BACNET_HOST_N_PORT_DATA peer_address = { 1, "\xef\x00\x00\x10", 50001 };
    uint8_t peer_VMAC[BACNET_PEER_VMAC_LENGTH] = { 1, 2, 3, 4, 5, 6 };
    uint8_t peer_UUID[16] = { 0 };
    BACNET_HOST_N_PORT_DATA peer_address2 = { 1, "\xef\x00\x00\x10", 50002 };

    BACNET_READ_PROPERTY_DATA rpdata;
    BACNET_OBJECT_PROPERTY_VALUE object_value;
    BACNET_APPLICATION_DATA_VALUE value; /* for decode value data */
    uint8_t apdu[MAX_APDU];
    int len;
    int len2;
    char str[512];

    const char REQ_STR[] =
        "{1946/05/07-12:34:22.10, 239.0.0.16:50001, 1.2.3.4.5.6, "
        "00000000-0000-0000-0000-000000000000, 38, \"error details\"}";

    const char HUB_STATUS_STR[] = "{1946/05/07-12:34:22.10, "
                                  "239.0.0.16:50001, 1.2.3.4.5.6, "
                                  "00000000-0000-0000-0000-000000000000, 38, "
                                  "\"error details\"}";

    const char DIRECT_STATUS_STR[] =
        "{connect url, 3, 1946/05/07-12:34:22.10, 1946/05/07-12:34:22.10, "
        "239.0.0.16:50001, 1.2.3.4.5.6, 00000000-0000-0000-0000-000000000000, "
        "38, \"error message of direct status\"}";

    const char PRIMARY_HUB_STATUS[] =
        "{2, 1946/05/07-12:34:22.10, "
        "1946/05/07-12:34:22.10, 38, \"error message\"}";

    const char FAILOVER_HUB_STATUS[] =
        "{3, 1946/05/07-12:34:22.10, "
        "1946/05/07-12:34:22.10, 14, \"again error message\"}";

    Network_Port_Init();
    instance = 1234;
    status = Network_Port_Object_Instance_Number_Set(0, instance);
    zassert_true(status, NULL);
    count = Network_Port_Count();
    zassert_true(count > 0, NULL);

    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_NETWORK_PORT;
    rpdata.object_instance = instance;

    object_value.object_type = rpdata.object_type;
    object_value.object_instance = rpdata.object_instance;
    object_value.value = &value;

    /* SC_Failed_Connection_Requests */
    count = Network_Port_SC_Failed_Connection_Requests_Count(instance);
    zassert_equal(count, 0, NULL);
    status = Network_Port_SC_Failed_Connection_Requests_Add(
        instance, &ts, &peer_address, peer_VMAC, peer_UUID,
        ERROR_CODE_VT_SESSION_ALREADY_CLOSED, "error details");
    zassert_true(status, NULL);
    count = Network_Port_SC_Failed_Connection_Requests_Count(instance);
    zassert_equal(count, 1, NULL);

    object_value.object_property = rpdata.object_property =
        PROP_SC_FAILED_CONNECTION_REQUESTS;

    if (property_list_bacnet_array_member(
            rpdata.object_type, rpdata.object_property)) {
        object_value.array_index = rpdata.array_index = 0;
        len = Network_Port_Read_Property(&rpdata);
        zassert_not_equal(len, -1, NULL);
        object_value.array_index = rpdata.array_index = 1;
        len = Network_Port_Read_Property(&rpdata);
        zassert_not_equal(len, -1, NULL);
    }
    object_value.array_index = rpdata.array_index = BACNET_ARRAY_ALL;
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len > 0, NULL);

    len2 = bacapp_decode_known_property(
        apdu, len, &value, rpdata.object_type, rpdata.object_property);
    zassert_equal(len2, len, NULL);

    len = bacapp_snprintf_value(NULL, 0, &object_value);
    zassert_true((len > 0) && (len < sizeof(str) - 1), NULL);
    len2 = bacapp_snprintf_value(str, len + 1, &object_value);
    zassert_equal(len2, len, NULL);

    zassert_true(strncmp(str, REQ_STR, strlen(REQ_STR)) == 0, NULL);

#if BSC_CONF_HUB_FUNCTIONS_NUM != 0

    /* SC_Hub_Function_Connection_Status */
    count = Network_Port_SC_Hub_Function_Connection_Status_Count(instance);
    zassert_equal(count, 0, NULL);

    status = Network_Port_SC_Hub_Function_Connection_Status_Add(
        instance, BACNET_SC_CONNECTION_STATE_CONNECTED, &ts, &ts, &peer_address,
        peer_VMAC, peer_UUID, ERROR_CODE_VT_SESSION_ALREADY_CLOSED,
        "hided error message");
    zassert_true(status, NULL);

    status = Network_Port_SC_Hub_Function_Connection_Status_Add(
        instance, BACNET_SC_CONNECTION_STATE_FAILED_TO_CONNECT, &ts, &ts,
        &peer_address2, peer_VMAC, peer_UUID,
        ERROR_CODE_VT_SESSION_ALREADY_CLOSED, "error message of hub status");
    zassert_true(status, NULL);

    count = Network_Port_SC_Hub_Function_Connection_Status_Count(instance);
    zassert_equal(count, 2, NULL);

    object_value.object_property = rpdata.object_property =
        PROP_SC_HUB_FUNCTION_CONNECTION_STATUS;

    if (property_list_bacnet_array_member(
            rpdata.object_type, rpdata.object_property)) {
        object_value.array_index = rpdata.array_index = 0;
        len = Network_Port_Read_Property(&rpdata);
        zassert_not_equal(len, -1, NULL);

        // context (error: property is not BacArray)
        object_value.array_index = rpdata.array_index = 1;
        len = Network_Port_Read_Property(&rpdata);
        zassert_not_equal(len, -1, NULL);
    }

    len = bacapp_snprintf_value(NULL, 0, &object_value);
    zassert_true((len > 0) && (len < sizeof(str) - 1), NULL);
    len2 = bacapp_snprintf_value(str, len + 1, &object_value);
    zassert_equal(len2, len, NULL);

    zassert_true(
        strncmp(str, HUB_STATUS_STR, strlen(HUB_STATUS_STR)) == 0, NULL);

    /* SC_Primary_Hub_Connection_Status */
    status = Network_Port_SC_Primary_Hub_Connection_Status_Set(
        instance, BACNET_SC_CONNECTION_STATE_DISCONNECTED_WITH_ERRORS, &ts, &ts,
        ERROR_CODE_VT_SESSION_ALREADY_CLOSED, "error message");
    zassert_true(status, NULL);

    rpdata.array_index = BACNET_ARRAY_ALL;
    object_value.object_property = rpdata.object_property =
        PROP_SC_PRIMARY_HUB_CONNECTION_STATUS;

    // context
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len > 0, NULL);

    len2 = bacapp_decode_known_property(
        apdu, len, &value, rpdata.object_type, rpdata.object_property);
    zassert_equal(len2, len, NULL);

    len = bacapp_snprintf_value(NULL, 0, &object_value);
    zassert_true((len > 0) && (len < sizeof(str) - 1), NULL);
    len2 = bacapp_snprintf_value(str, len + 1, &object_value);
    zassert_equal(len2, len, NULL);
    zassert_true(
        strncmp(str, PRIMARY_HUB_STATUS, strlen(PRIMARY_HUB_STATUS)) == 0,
        NULL);

    /* SC_Failover_Hub_Connection_Status */
    status = Network_Port_SC_Failover_Hub_Connection_Status_Set(
        instance, BACNET_SC_CONNECTION_STATE_FAILED_TO_CONNECT, &ts, &ts,
        ERROR_CODE_INVALID_TIME_STAMP, "again error message");
    zassert_true(status, NULL);

    object_value.object_property = rpdata.object_property =
        PROP_SC_FAILOVER_HUB_CONNECTION_STATUS;

    // context
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len > 0, NULL);

    len2 = bacapp_decode_known_property(
        apdu, len, &value, rpdata.object_type, rpdata.object_property);
    zassert_equal(len2, len, NULL);

    len = bacapp_snprintf_value(NULL, 0, &object_value);
    zassert_true((len > 0) && (len < sizeof(str) - 1), NULL);
    len2 = bacapp_snprintf_value(str, len + 1, &object_value);
    zassert_equal(len2, len, NULL);
    zassert_true(
        strncmp(str, FAILOVER_HUB_STATUS, strlen(FAILOVER_HUB_STATUS)) == 0,
        NULL);

#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */

#if BSC_CONF_HUB_CONNECTORS_NUM != 0

    /* SC_Hub_Function_Connection_Status */
    count = Network_Port_SC_Direct_Connect_Connection_Status_Count(instance);
    zassert_equal(count, 0, NULL);

    status = Network_Port_SC_Direct_Connect_Connection_Status_Add(
        instance, "connect url", BACNET_SC_CONNECTION_STATE_FAILED_TO_CONNECT,
        &ts, &ts, &peer_address, peer_VMAC, peer_UUID,
        ERROR_CODE_VT_SESSION_ALREADY_CLOSED, "error message of direct status");
    zassert_true(status, NULL);

    count = Network_Port_SC_Direct_Connect_Connection_Status_Count(instance);
    zassert_equal(count, 1, NULL);

    object_value.object_property = rpdata.object_property =
        PROP_SC_DIRECT_CONNECT_CONNECTION_STATUS;

    if (property_list_bacnet_array_member(
            rpdata.object_type, rpdata.object_property)) {
        object_value.array_index = rpdata.array_index = 0;
        len = Network_Port_Read_Property(&rpdata);
        zassert_not_equal(len, -1, NULL);
        object_value.array_index = rpdata.array_index = 1;
        len = Network_Port_Read_Property(&rpdata);
        zassert_not_equal(len, -1, NULL);
    }
    object_value.array_index = rpdata.array_index = BACNET_ARRAY_ALL;
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len > 0, NULL);

    len2 = bacapp_decode_known_property(
        apdu, len, &value, rpdata.object_type, rpdata.object_property);
    zassert_equal(len2, len, NULL);

    len = bacapp_snprintf_value(NULL, 0, &object_value);
    zassert_true((len > 0) && (len < sizeof(str) - 1), NULL);
    len2 = bacapp_snprintf_value(str, len + 1, &object_value);
    zassert_equal(len2, len, NULL);

    zassert_true(
        strncmp(str, DIRECT_STATUS_STR, strlen(DIRECT_STATUS_STR)) == 0, NULL);

#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

#endif /* BACDL_BSC */

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(netport_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        netport_tests, ztest_unit_test(test_network_port),
        ztest_unit_test(test_network_port_pending_param),
        ztest_unit_test(test_network_port_sc_direct_connect_accept_uri),
        ztest_unit_test(test_network_port_sc_certificates),
        ztest_unit_test(test_network_port_sc_status_encode_decode));

    ztest_run_test_suite(netport_tests);
}
#endif
