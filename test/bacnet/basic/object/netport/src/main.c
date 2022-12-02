/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/readrange.h>
#include <bacnet/basic/object/netport.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void test_network_port(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    /* for decode value data */
    BACNET_APPLICATION_DATA_VALUE value;
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    unsigned port = 0;
    unsigned count = 0;
    uint32_t object_instance = 0;
    bool status = false;
    uint8_t port_type[] = { PORT_TYPE_ETHERNET, PORT_TYPE_ARCNET,
        PORT_TYPE_MSTP, PORT_TYPE_PTP, PORT_TYPE_LONTALK, PORT_TYPE_BIP,
        PORT_TYPE_ZIGBEE, PORT_TYPE_VIRTUAL, PORT_TYPE_NON_BACNET,
        PORT_TYPE_BIP6, PORT_TYPE_MAX };

    while (port_type[port] != PORT_TYPE_MAX) {
        Network_Port_Init();
        object_instance = 1234;
        status = Network_Port_Object_Instance_Number_Set(0, object_instance);
        zassert_true(status, NULL);
        status = Network_Port_Type_Set(object_instance, port_type[port]);
        zassert_true(status, NULL);
        count = Network_Port_Count();
        zassert_true(count > 0, NULL);
        rpdata.application_data = &apdu[0];
        rpdata.application_data_len = sizeof(apdu);
        rpdata.object_type = OBJECT_NETWORK_PORT;
        rpdata.object_instance = object_instance;
        Network_Port_Property_Lists(&pRequired, &pOptional, &pProprietary);
        while ((*pRequired) != -1) {
            rpdata.object_property = *pRequired;
            rpdata.array_index = BACNET_ARRAY_ALL;
            len = Network_Port_Read_Property(&rpdata);
            zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
            if (len > 0) {
                test_len = bacapp_decode_application_data(
                    rpdata.application_data,
                    (uint8_t)rpdata.application_data_len, &value);
                zassert_true(test_len >= 0, NULL);
                if (test_len < 0) {
                    printf("<decode failed!>\n");
                }
            }
            pRequired++;
        }
        while ((*pOptional) != -1) {
            rpdata.object_property = *pOptional;
            rpdata.array_index = BACNET_ARRAY_ALL;
            len = Network_Port_Read_Property(&rpdata);
            zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
            if (len > 0) {
                test_len = bacapp_decode_application_data(
                    rpdata.application_data,
                    (uint8_t)rpdata.application_data_len, &value);
                zassert_true(test_len >= 0, NULL);
                if (test_len < 0) {
                    printf("<decode failed!>\n");
                }
            }
            pOptional++;
        }
        port++;
    }

    return;
}
/**
 * @}
 */

static void test_network_port_pending_param(void)
{
#if BACDL_BSC
    /* for decode value data */
    unsigned count = 0;
    uint32_t object_instance = 0;
    bool status = false;
#if (BSC_CONF_HUB_FUNCTIONS_NUM!=0) || (BSC_CONF_HUB_CONNECTORS_NUM!=0)
    bool val;
    BACNET_CHARACTER_STRING str;
#endif

    Network_Port_Init();
    object_instance = 1234;
    status = Network_Port_Object_Instance_Number_Set(0, object_instance);
    zassert_true(status, NULL);
    status = Network_Port_Type_Set(object_instance, PORT_TYPE_BIP);
    zassert_true(status, NULL);
    count = Network_Port_Count();
    zassert_true(count > 0, NULL);

    // write properties
#if BSC_CONF_HUB_FUNCTIONS_NUM!=0
    Network_Port_SC_Primary_Hub_URI_Set(object_instance,
        "SC_Primary_Hub_URI_test");
    Network_Port_SC_Failover_Hub_URI_Set(object_instance,
        "SC_Failover_Hub_URI_test");
    Network_Port_SC_Hub_Function_Enable_Set(object_instance, false);
    Network_Port_SC_Hub_Function_Binding_Set(object_instance,
        "SC_Hub_Function_Binding_test");
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM!=0
    Network_Port_SC_Direct_Connect_Initiate_Enable_Set(object_instance,
        false);
    Network_Port_SC_Direct_Connect_Accept_Enable_Set(object_instance,
        false);
    Network_Port_SC_Direct_Connect_Binding_Set(object_instance,
        "SC_Direct_Connect_Binding_test");
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

    // write dirty properties
#if BSC_CONF_HUB_FUNCTIONS_NUM!=0
    Network_Port_SC_Primary_Hub_URI_Dirty_Set(object_instance,
        "SC_Primary_Hub_URI_test2");
    Network_Port_SC_Failover_Hub_URI_Dirty_Set(object_instance,
        "SC_Failover_Hub_URI_test2");
    Network_Port_SC_Hub_Function_Enable_Dirty_Set(object_instance, true);
    Network_Port_SC_Hub_Function_Binding_Dirty_Set(object_instance,
        "SC_Hub_Function_Binding_test2");
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM!=0
    Network_Port_SC_Direct_Connect_Initiate_Enable_Dirty_Set(object_instance,
        true);
    Network_Port_SC_Direct_Connect_Accept_Enable_Dirty_Set(object_instance,
        true);
    Network_Port_SC_Direct_Connect_Binding_Dirty_Set(object_instance,
        "SC_Direct_Connect_Binding_test2");
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

    // check old value
#if BSC_CONF_HUB_FUNCTIONS_NUM!=0
    Network_Port_SC_Primary_Hub_URI(object_instance, &str);
    zassert_true(strcmp(characterstring_value(&str),
        "SC_Primary_Hub_URI_test") == 0, NULL);
    Network_Port_SC_Failover_Hub_URI(object_instance, &str);
    zassert_true(strcmp(characterstring_value(&str),
        "SC_Failover_Hub_URI_test") == 0, NULL);
    val = Network_Port_SC_Hub_Function_Enable(object_instance);
    zassert_true(val == false, NULL);
    Network_Port_SC_Hub_Function_Binding(object_instance, &str);
    zassert_true(strcmp(characterstring_value(&str),
        "SC_Hub_Function_Binding_test") == 0, NULL);
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM!=0
    val = Network_Port_SC_Direct_Connect_Initiate_Enable(object_instance);
    zassert_true(val == false, NULL);
    val = Network_Port_SC_Direct_Connect_Accept_Enable(object_instance);
    zassert_true(val == false, NULL);
    Network_Port_SC_Direct_Connect_Binding(object_instance, &str);
    zassert_true(strcmp(characterstring_value(&str),
        "SC_Direct_Connect_Binding_test") == 0, NULL);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

    // apply
    Network_Port_Pending_Params_Apply(object_instance);

    // check new value
#if BSC_CONF_HUB_FUNCTIONS_NUM!=0
    Network_Port_SC_Primary_Hub_URI(object_instance, &str);
    zassert_true(strcmp(characterstring_value(&str),
        "SC_Primary_Hub_URI_test2") == 0, NULL);
    Network_Port_SC_Failover_Hub_URI(object_instance, &str);
    zassert_true(strcmp(characterstring_value(&str),
        "SC_Failover_Hub_URI_test2") == 0, NULL);
    val = Network_Port_SC_Hub_Function_Enable(object_instance);
    zassert_true(val == true, NULL);
    Network_Port_SC_Hub_Function_Binding(object_instance, &str);
    zassert_true(strcmp(characterstring_value(&str),
        "SC_Hub_Function_Binding_test2") == 0, NULL);
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM!=0
    val = Network_Port_SC_Direct_Connect_Initiate_Enable(object_instance);
    zassert_true(val == true, NULL);
    val = Network_Port_SC_Direct_Connect_Accept_Enable(object_instance);
    zassert_true(val == true, NULL);
    Network_Port_SC_Direct_Connect_Binding(object_instance, &str);
    zassert_true(strcmp(characterstring_value(&str),
        "SC_Direct_Connect_Binding_test2") == 0, NULL);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

#endif /* BACDL_BSC */
    return;
}

static void test_network_port_sc_direct_connect_accept_uri(void)
{
#if (BACDL_BSC) && (BSC_CONF_HUB_CONNECTORS_NUM!=0)
    #define URL1        "SC_Direct_Connect_Accept_URI1"
    #define URL2        "SC_Direct_Connect_Accept_URI2"
    #define URL3        "SC_Direct_Connect_Accept_URI3"
    #define URL_SMALL   "bla-bla-bla"
    #define URL_BIG     "big-bla-big-bla-big-bla-big-bla-big-bla"
    char urls0[] = URL1 " " URL2 " " URL3;
    char urls1[] = URL1 " " URL_SMALL " " URL3;
    char urls2[] = URL1 " " URL_BIG " " URL3;
    char urls3[] = URL1 " " URL_BIG " " URL3 " " URL_SMALL;
    char urls4[] = URL1 " " URL_BIG " " URL3 " " URL_SMALL " " URL_BIG;

    unsigned count = 0;
    uint32_t object_instance = 0;
    bool status = false;
    BACNET_CHARACTER_STRING str;

    Network_Port_Init();
    object_instance = 1234;
    status = Network_Port_Object_Instance_Number_Set(0, object_instance);
    zassert_true(status, NULL);
    count = Network_Port_Count();
    zassert_true(count > 0, NULL);

    // init 
    Network_Port_SC_Direct_Connect_Accept_URIs_Set(object_instance, urls0);
    zassert_true(
        strcmp(Network_Port_SC_Direct_Connect_Accept_URIs_char(object_instance),
        urls0) == 0, NULL);

    zassert_true(Network_Port_SC_Direct_Connect_Accept_URI(object_instance,
        0, &str), NULL);
    zassert_true(strcmp(characterstring_value(&str), URL1) == 0, NULL);

    zassert_true(Network_Port_SC_Direct_Connect_Accept_URI(object_instance,
        1, &str), NULL);
    zassert_true(strcmp(characterstring_value(&str), URL2) == 0, NULL);

    zassert_true(Network_Port_SC_Direct_Connect_Accept_URI(object_instance,
        2, &str), NULL);
    zassert_true(strcmp(characterstring_value(&str), URL3) == 0, NULL);

    // change
    zassert_true(Network_Port_SC_Direct_Connect_Accept_URI_Set(object_instance,
        1, URL_SMALL), NULL);
    zassert_true(
        strcmp(Network_Port_SC_Direct_Connect_Accept_URIs_char(object_instance),
        urls1) == 0, NULL);

    zassert_true(Network_Port_SC_Direct_Connect_Accept_URI_Set(object_instance,
        1, URL_BIG), NULL);
    zassert_true(
        strcmp(Network_Port_SC_Direct_Connect_Accept_URIs_char(object_instance),
        urls2) == 0, NULL);

    // append
    zassert_true(Network_Port_SC_Direct_Connect_Accept_URI_Set(object_instance,
        4, URL_SMALL), NULL);
    zassert_true(
        strcmp(Network_Port_SC_Direct_Connect_Accept_URIs_char(object_instance),
        urls3) == 0, NULL);

    zassert_true(Network_Port_SC_Direct_Connect_Accept_URI_Set(object_instance,
        4, URL_BIG), NULL);
    zassert_true(
        strcmp(Network_Port_SC_Direct_Connect_Accept_URIs_char(object_instance),
        urls4) == 0, NULL);

    // check last
    zassert_true(Network_Port_SC_Direct_Connect_Accept_URI(object_instance,
        0, &str), NULL);
    zassert_true(strcmp(characterstring_value(&str), URL1) == 0, NULL);

    zassert_true(Network_Port_SC_Direct_Connect_Accept_URI(object_instance,
        1, &str), NULL);
    zassert_true(strcmp(characterstring_value(&str), URL_BIG) == 0, NULL);

    zassert_true(Network_Port_SC_Direct_Connect_Accept_URI(object_instance,
        2, &str), NULL);
    zassert_true(strcmp(characterstring_value(&str), URL3) == 0, NULL);

    zassert_true(Network_Port_SC_Direct_Connect_Accept_URI(object_instance,
        3, &str), NULL);
    zassert_true(strcmp(characterstring_value(&str), URL_SMALL) == 0, NULL);

    zassert_true(Network_Port_SC_Direct_Connect_Accept_URI(object_instance,
        4, &str), NULL);
    zassert_true(strcmp(characterstring_value(&str), URL_BIG) == 0, NULL);

#endif /* BACDL_BSC && BSC_CONF_HUB_CONNECTORS_NUM!=0 */

    return;
}

void test_main(void)
{
    ztest_test_suite(netport_tests,
     ztest_unit_test(test_network_port),
     ztest_unit_test(test_network_port_pending_param),
     ztest_unit_test(test_network_port_sc_direct_connect_accept_uri)
     );

    ztest_run_test_suite(netport_tests);
}
