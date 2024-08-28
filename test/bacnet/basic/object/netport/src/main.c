/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2024
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/readrange.h>
#include <bacnet/basic/object/netport.h>
#include <property_test.h>

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
    uint32_t object_instance = 0;
    uint8_t port_type[] = { PORT_TYPE_ETHERNET,   PORT_TYPE_ARCNET,
                            PORT_TYPE_MSTP,       PORT_TYPE_PTP,
                            PORT_TYPE_LONTALK,    PORT_TYPE_BIP,
                            PORT_TYPE_ZIGBEE,     PORT_TYPE_VIRTUAL,
                            PORT_TYPE_NON_BACNET, PORT_TYPE_BIP6,
                            PORT_TYPE_MAX };
    const int known_fail_property_list[] = {
        PROP_IP_DNS_SERVER,
        PROP_BBMD_BROADCAST_DISTRIBUTION_TABLE,
        PROP_BBMD_FOREIGN_DEVICE_TABLE,
        PROP_FD_BBMD_ADDRESS,
        PROP_IPV6_DNS_SERVER,
        -1
    };

    while (port_type[port] != PORT_TYPE_MAX) {
        object_instance = 1234;
        status = Network_Port_Object_Instance_Number_Set(0, object_instance);
        zassert_true(status, NULL);
        status = Network_Port_Type_Set(object_instance, port_type[port]);
        zassert_true(status, NULL);
        Network_Port_Init();
        count = Network_Port_Count();
        zassert_true(count > 0, NULL);
        bacnet_object_properties_read_write_test(
            OBJECT_NETWORK_PORT, object_instance, Network_Port_Property_Lists,
            Network_Port_Read_Property, Network_Port_Write_Property,
            known_fail_property_list);
        bacnet_object_name_ascii_test(
            object_instance,
            Network_Port_Name_Set,
            Network_Port_Object_Name_ASCII);
        port++;
    }

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(netport_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(netport_tests, ztest_unit_test(test_network_port));

    ztest_run_test_suite(netport_tests);
}
#endif
