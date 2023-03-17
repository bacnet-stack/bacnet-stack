/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/readrange.h>
#include <bacnet/basic/object/netport.h>
#include <bacnet/basic/object/sc_netport.h>
#include <bacnet/basic/object/bacfile.h>

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
        PORT_TYPE_BIP6, PORT_TYPE_SERIAL, PORT_TYPE_BSC, PORT_TYPE_MAX };

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
#ifdef BACDL_BSC
    /* for decode value data */
    unsigned count = 0;
    uint32_t object_instance = 0;
    bool status = false;
#if (BSC_CONF_HUB_FUNCTIONS_NUM!=0) || (BSC_CONF_HUB_CONNECTORS_NUM!=0)
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
#if BSC_CONF_HUB_FUNCTIONS_NUM!=0
    Network_Port_SC_Primary_Hub_URI_Set(object_instance, PRIMARY_HUB_URL1);
    Network_Port_SC_Failover_Hub_URI_Set(object_instance, FAILOVER_HUB_URL1);
    Network_Port_SC_Hub_Function_Enable_Set(object_instance, false);
    Network_Port_SC_Hub_Function_Binding_Set(object_instance, HUB_BINDING1);
    Network_Port_SC_Hub_Function_Accept_URI_Set(object_instance, 0, URL1);
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM!=0
    Network_Port_SC_Direct_Connect_Initiate_Enable_Set(object_instance, false);
    Network_Port_SC_Direct_Connect_Accept_Enable_Set(object_instance, false);
    Network_Port_SC_Direct_Connect_Binding_Set(object_instance,
        DIRECT_BINDING1);
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

#if BSC_CONF_HUB_FUNCTIONS_NUM!=0
    Network_Port_SC_Primary_Hub_URI_Dirty_Set(object_instance,
        PRIMARY_HUB_URL2);
    Network_Port_SC_Failover_Hub_URI_Dirty_Set(object_instance,
        FAILOVER_HUB_URL2);
    Network_Port_SC_Hub_Function_Enable_Dirty_Set(object_instance, true);
    Network_Port_SC_Hub_Function_Binding_Dirty_Set(object_instance,
        HUB_BINDING2);
    Network_Port_SC_Hub_Function_Accept_URI_Dirty_Set(object_instance, 0, URL2);
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM!=0
    Network_Port_SC_Direct_Connect_Initiate_Enable_Dirty_Set(object_instance,
        true);
    Network_Port_SC_Direct_Connect_Accept_Enable_Dirty_Set(object_instance,
        true);
    Network_Port_SC_Direct_Connect_Binding_Dirty_Set(object_instance,
        DIRECT_BINDING2);
    Network_Port_SC_Direct_Connect_Accept_URI_Dirty_Set(object_instance, 0,
        URL2);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

    // check old value
    zassert_true(Network_Port_Max_BVLC_Length_Accepted(object_instance) == 1200,
        NULL);
    zassert_true(Network_Port_Max_NPDU_Length_Accepted(object_instance) == 1100,
        NULL);
    zassert_true(Network_Port_SC_Minimum_Reconnect_Time(object_instance) == 2,
        NULL);
    zassert_true(Network_Port_SC_Maximum_Reconnect_Time(object_instance) == 5,
        NULL);
    zassert_true(Network_Port_SC_Connect_Wait_Timeout(object_instance) == 40,
        NULL);
    zassert_true(Network_Port_SC_Disconnect_Wait_Timeout(object_instance) == 10,
        NULL);
    zassert_true(Network_Port_SC_Heartbeat_Timeout(object_instance) == 3, NULL);

#if BSC_CONF_HUB_FUNCTIONS_NUM!=0
    Network_Port_SC_Primary_Hub_URI(object_instance, &str);
    zassert_true(strncmp(characterstring_value(&str),
        PRIMARY_HUB_URL1, characterstring_length(&str)) == 0, NULL);
    Network_Port_SC_Failover_Hub_URI(object_instance, &str);
    zassert_true(strncmp(characterstring_value(&str),
        FAILOVER_HUB_URL1, characterstring_length(&str)) == 0, NULL);
    val = Network_Port_SC_Hub_Function_Enable(object_instance);
    zassert_true(val == false, NULL);
    Network_Port_SC_Hub_Function_Binding(object_instance, &str);
    zassert_true(strncmp(characterstring_value(&str),
        HUB_BINDING1, characterstring_length(&str)) == 0,
        NULL);
    Network_Port_SC_Hub_Function_Accept_URI(object_instance, 0, &str);
    zassert_true(strncmp(characterstring_value(&str), URL1,
        characterstring_length(&str)) == 0, NULL);
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM!=0
    val = Network_Port_SC_Direct_Connect_Initiate_Enable(object_instance);
    zassert_true(val == false, NULL);
    val = Network_Port_SC_Direct_Connect_Accept_Enable(object_instance);
    zassert_true(val == false, NULL);
    Network_Port_SC_Direct_Connect_Binding(object_instance, &str);
    zassert_true(strncmp(characterstring_value(&str),
        DIRECT_BINDING1, characterstring_length(&str)) == 0,
        NULL);
    Network_Port_SC_Direct_Connect_Accept_URI(object_instance, 0, &str);
    zassert_true(strncmp(characterstring_value(&str), URL1,
        characterstring_length(&str)) == 0, NULL);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

    // apply
    Network_Port_Pending_Params_Apply(object_instance);

    // check new value
    zassert_true(Network_Port_Max_BVLC_Length_Accepted(object_instance) == 1250,
        NULL);
    zassert_true(Network_Port_Max_NPDU_Length_Accepted(object_instance) == 1150,
        NULL);
    zassert_true(Network_Port_SC_Minimum_Reconnect_Time(object_instance) == 3,
        NULL);
    zassert_true(Network_Port_SC_Maximum_Reconnect_Time(object_instance) == 4,
        NULL);
    zassert_true(Network_Port_SC_Connect_Wait_Timeout(object_instance) == 50,
        NULL);
    zassert_true(Network_Port_SC_Disconnect_Wait_Timeout(object_instance) == 15,
        NULL);
    zassert_true(Network_Port_SC_Heartbeat_Timeout(object_instance) == 6, NULL);

#if BSC_CONF_HUB_FUNCTIONS_NUM!=0
    Network_Port_SC_Primary_Hub_URI(object_instance, &str);
    zassert_true(strncmp(characterstring_value(&str),
        PRIMARY_HUB_URL2, characterstring_length(&str)) == 0, NULL);
    Network_Port_SC_Failover_Hub_URI(object_instance, &str);
    zassert_true(strncmp(characterstring_value(&str),
        FAILOVER_HUB_URL2, characterstring_length(&str)) == 0, NULL);
    val = Network_Port_SC_Hub_Function_Enable(object_instance);
    zassert_true(val == true, NULL);
    Network_Port_SC_Hub_Function_Binding(object_instance, &str);
    zassert_true(strncmp(characterstring_value(&str),
        HUB_BINDING2, characterstring_length(&str)) == 0, NULL);
    Network_Port_SC_Hub_Function_Accept_URI(object_instance, 0, &str);
    zassert_true(strncmp(characterstring_value(&str), URL2,
        characterstring_length(&str)) == 0, NULL);
#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */
#if BSC_CONF_HUB_CONNECTORS_NUM!=0
    val = Network_Port_SC_Direct_Connect_Initiate_Enable(object_instance);
    zassert_true(val == true, NULL);
    val = Network_Port_SC_Direct_Connect_Accept_Enable(object_instance);
    zassert_true(val == true, NULL);
    Network_Port_SC_Direct_Connect_Binding(object_instance, &str);
    zassert_true(strncmp(characterstring_value(&str),
        DIRECT_BINDING2, characterstring_length(&str)) == 0, NULL);
    Network_Port_SC_Direct_Connect_Accept_URI(object_instance, 0, &str);
    zassert_true(strncmp(characterstring_value(&str), URL2,
        characterstring_length(&str)) == 0, NULL);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

#endif /* BACDL_BSC */
    return;
}

static void test_network_port_sc_direct_connect_accept_uri(void)
{
#if defined(BACDL_BSC) && (BSC_CONF_HUB_CONNECTORS_NUM!=0)
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
    BACNET_CHARACTER_STRING str = {0};

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
    zassert_true(strncmp(characterstring_value(&str), URL1,
        characterstring_length(&str)) == 0, NULL);

    zassert_true(Network_Port_SC_Direct_Connect_Accept_URI(object_instance,
        1, &str), NULL);
    zassert_true(strncmp(characterstring_value(&str), URL2,
        characterstring_length(&str)) == 0, NULL);

    zassert_true(Network_Port_SC_Direct_Connect_Accept_URI(object_instance,
        2, &str), NULL);
    zassert_true(strncmp(characterstring_value(&str), URL3,
        characterstring_length(&str)) == 0, NULL);

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
    zassert_true(strncmp(characterstring_value(&str), URL_BIG,
        characterstring_length(&str)) == 0, NULL);

#endif /* BACDL_BSC && BSC_CONF_HUB_CONNECTORS_NUM!=0 */

    return;
}

#define BACFILE_START          0

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
    // CA certificate
    status = bacfile_create(BSC_ISSUER_CERTIFICATE_FILE_1_INSTANCE);
    zassert_true(status, NULL);
    bacfile_pathname_set(BSC_ISSUER_CERTIFICATE_FILE_1_INSTANCE,
        filename_ca_cert);
    status = Network_Port_Issuer_Certificate_File_Set(instance, 0,
        BSC_ISSUER_CERTIFICATE_FILE_1_INSTANCE);
    zassert_true(status, NULL);
    file_instance = Network_Port_Issuer_Certificate_File(instance, 0);
    zassert_equal(file_instance, BSC_ISSUER_CERTIFICATE_FILE_1_INSTANCE, NULL);

    status = bacfile_create(BSC_OPERATIONAL_CERTIFICATE_FILE_INSTANCE);
    zassert_true(status, NULL);
    bacfile_pathname_set(BSC_OPERATIONAL_CERTIFICATE_FILE_INSTANCE,
        filename_cert);
    status = Network_Port_Operational_Certificate_File_Set(instance,
        BSC_OPERATIONAL_CERTIFICATE_FILE_INSTANCE);
    zassert_true(status, NULL);
    file_instance = Network_Port_Operational_Certificate_File(instance);
    zassert_equal(file_instance, BSC_OPERATIONAL_CERTIFICATE_FILE_INSTANCE,
        NULL);

    status = bacfile_create(BSC_CERTIFICATE_SIGNING_REQUEST_FILE_INSTANCE);
    zassert_true(status, NULL);
    bacfile_pathname_set(BSC_CERTIFICATE_SIGNING_REQUEST_FILE_INSTANCE,
        filename_key);
    status = Network_Port_Certificate_Key_File_Set(instance,
        BSC_CERTIFICATE_SIGNING_REQUEST_FILE_INSTANCE);
    zassert_true(status, NULL);
    file_instance = Network_Port_Certificate_Key_File(instance);
    zassert_equal(file_instance, BSC_CERTIFICATE_SIGNING_REQUEST_FILE_INSTANCE,
        NULL);

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
    BACNET_DATE_TIME ts = {{2202, 5, 7, 3}, {12,34,22,10}};
    BACNET_HOST_N_PORT_DATA peer_address = {1, "\xef\x00\x00\x10", 50001};
    uint8_t peer_VMAC[BACNET_PEER_VMAC_LENGTH] = {1,2,3,4,5,6};
    uint8_t peer_UUID[16] = {0};
    BACNET_HOST_N_PORT_DATA peer_address2 = {1, "\xef\x00\x00\x10", 50002};

    BACNET_READ_PROPERTY_DATA rpdata;
    BACNET_OBJECT_PROPERTY_VALUE object_value;
    BACNET_APPLICATION_DATA_VALUE value; /* for decode value data */
    uint8_t apdu[MAX_APDU];
    uint16_t apdu_len_max;
    int len, len2, apdu_len, str_len;
    char str[512];

    const char REQ_STR[] =
        "{Wednesday, May 7, 1946 12:34:22.10, 239.0.0.16:50001, 1.2.3.4.5.6, "
        "00000000-0000-0000-0000-000000000000, 38, \"error details\"}";

    const char HUB_STATUS_STR[] = "{1, Wednesday, May 7, 1946 12:34:22.10, "
        "Wednesday, May 7, 1946 12:34:22.10, 239.0.0.16:50001, 1.2.3.4.5.6, "
        "00000000-0000-0000-0000-000000000000, 0}{3, Wednesday, May 7, 1946 "
        "12:34:22.10, Wednesday, May 7, 1946 12:34:22.10, 239.0.0.16:50002, "
        "1.2.3.4.5.6, 00000000-0000-0000-0000-000000000000, 38, "
        "\"error message of hub status\"}";

    const char DIRECT_STATUS_STR[] =
        "{connect url, 3, Wednesday, May 7, 1946 12:34:22.10, Wednesday, May "
        "7, 1946 12:34:22.10, 239.0.0.16:50001, 1.2.3.4.5.6, "
        "00000000-0000-0000-0000-000000000000, 38, "
        "\"error message of direct status\"}";

    const char PRIMARY_HUB_STATUS[] = "{2, Wednesday, May 7, 1946 12:34:22.10, "
        "Wednesday, May 7, 1946 12:34:22.10, 38, \"error message\"}";

    const char FAILOVER_HUB_STATUS[] = "{3, Wednesday, May 7, 1946 12:34:22.10,"
        " Wednesday, May 7, 1946 12:34:22.10, 14, \"again error message\"}";

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
    status = Network_Port_SC_Failed_Connection_Requests_Add(instance, &ts,
        &peer_address, peer_VMAC, peer_UUID,
        ERROR_CODE_VT_SESSION_ALREADY_CLOSED, "error details");
    zassert_true(status, NULL);
    count = Network_Port_SC_Failed_Connection_Requests_Count(instance);
    zassert_equal(count, 1, NULL);

    object_value.object_property = rpdata.object_property =
        PROP_SC_FAILED_CONNECTION_REQUESTS;

    // count (error: property is not BacArray)
    object_value.array_index = rpdata.array_index = 0;
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len == -1, NULL);

    // context (error: property is not BacArray)
    object_value.array_index = rpdata.array_index = 1;
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len == -1, NULL);

    // all context
    object_value.array_index = rpdata.array_index = BACNET_ARRAY_ALL;
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len > 0, NULL);

    str_len = 0;
    apdu_len = 0;
    while ((apdu_len < len)) {
        len2 = bacapp_decode_known_property(apdu + apdu_len, len - apdu_len,
            &value, rpdata.object_type, rpdata.object_property);
        zassert_true(len2 > 0, NULL);
        apdu_len += len2;

        len2 = bacapp_snprintf_value(NULL, 0, &object_value);
        zassert_true((len2 > 0) && (len2 < sizeof(str) - 1 - str_len), NULL);
        str_len +=
            bacapp_snprintf_value(str + str_len, len2 + 1, &object_value);
    }

    zassert_true(strncmp(str, REQ_STR, strlen(REQ_STR)) == 0, NULL);

#if BSC_CONF_HUB_FUNCTIONS_NUM!=0

    /* SC_Hub_Function_Connection_Status */
    count = Network_Port_SC_Hub_Function_Connection_Status_Count(instance);
    zassert_equal(count, 0, NULL);

    status = Network_Port_SC_Hub_Function_Connection_Status_Add(instance, 
        BACNET_CONNECTED, &ts, &ts,&peer_address, peer_VMAC, peer_UUID,
        ERROR_CODE_VT_SESSION_ALREADY_CLOSED, "hided error message");
    zassert_true(status, NULL);

    status = Network_Port_SC_Hub_Function_Connection_Status_Add(instance,
        BACNET_FAILED_TO_CONNECT, &ts, &ts, &peer_address2, peer_VMAC,peer_UUID,
        ERROR_CODE_VT_SESSION_ALREADY_CLOSED, "error message of hub status");
    zassert_true(status, NULL);

    count = Network_Port_SC_Hub_Function_Connection_Status_Count(instance);
    zassert_equal(count, 2, NULL);

    object_value.object_property = rpdata.object_property =
        PROP_SC_HUB_FUNCTION_CONNECTION_STATUS;

    // count (error: property is not BacArray)
    object_value.array_index = rpdata.array_index = 0;
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len == -1, NULL);

    // context (error: property is not BacArray)
    object_value.array_index = rpdata.array_index = 1;
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len == -1, NULL);

    // all context
    object_value.array_index = rpdata.array_index = BACNET_ARRAY_ALL;
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len > 0, NULL);

    str_len = 0;
    apdu_len = 0;
    while ((apdu_len < len)) {
        len2 = bacapp_decode_known_property(apdu + apdu_len, len - apdu_len,
            &value, rpdata.object_type, rpdata.object_property);
        zassert_true(len2 > 0, NULL);
        apdu_len += len2;

        len2 = bacapp_snprintf_value(NULL, 0, &object_value);
        zassert_true((len2 > 0) && (len2 < sizeof(str) - 1 - str_len), NULL);
        str_len +=
            bacapp_snprintf_value(str + str_len, len2 + 1, &object_value);
    }

    zassert_true(
        strncmp(str, HUB_STATUS_STR, strlen(HUB_STATUS_STR)) == 0, NULL);

    /* SC_Primary_Hub_Connection_Status */
    status = Network_Port_SC_Primary_Hub_Connection_Status_Set(instance,
        BACNET_DISCONNECTED_WITH_ERRORS, &ts, &ts,
        ERROR_CODE_VT_SESSION_ALREADY_CLOSED, "error message");
    zassert_true(status, NULL);

    object_value.object_property = rpdata.object_property =
        PROP_SC_PRIMARY_HUB_CONNECTION_STATUS;

    // context
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len > 0, NULL);

    len2 = bacapp_decode_known_property(apdu, len, &value, rpdata.object_type,
        rpdata.object_property);
    zassert_equal(len2, len, NULL);

    len = bacapp_snprintf_value(NULL, 0, &object_value);
    zassert_true((len > 0) && (len < sizeof(str) - 1), NULL);
    len2 = bacapp_snprintf_value(str, len + 1, &object_value);
    zassert_equal(len2, len, NULL);
    zassert_true(
        strncmp(str, PRIMARY_HUB_STATUS, strlen(PRIMARY_HUB_STATUS))== 0, NULL);

    /* SC_Failover_Hub_Connection_Status */
    status = Network_Port_SC_Failover_Hub_Connection_Status_Set(instance,
        BACNET_FAILED_TO_CONNECT, &ts, &ts, ERROR_CODE_INVALID_TIME_STAMP,
        "again error message");
    zassert_true(status, NULL);

    object_value.object_property = rpdata.object_property =
        PROP_SC_FAILOVER_HUB_CONNECTION_STATUS;

    // context
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len > 0, NULL);

    len2 = bacapp_decode_known_property(apdu, len, &value, rpdata.object_type,
        rpdata.object_property);
    zassert_equal(len2, len, NULL);

    len = bacapp_snprintf_value(NULL, 0, &object_value);
    zassert_true((len > 0) && (len < sizeof(str) - 1), NULL);
    len2 = bacapp_snprintf_value(str, len + 1, &object_value);
    zassert_equal(len2, len, NULL);
    zassert_true(
        strncmp(str, FAILOVER_HUB_STATUS, strlen(FAILOVER_HUB_STATUS))== 0,
        NULL);

#endif /* BSC_CONF_HUB_FUNCTIONS_NUM!=0 */

#if BSC_CONF_HUB_CONNECTORS_NUM!=0

    /* SC_Hub_Function_Connection_Status */
    count = Network_Port_SC_Direct_Connect_Connection_Status_Count(instance);
    zassert_equal(count, 0, NULL);

    status = Network_Port_SC_Direct_Connect_Connection_Status_Add(instance,
        "connect url", BACNET_FAILED_TO_CONNECT, &ts, &ts, &peer_address,
        peer_VMAC, peer_UUID, ERROR_CODE_VT_SESSION_ALREADY_CLOSED,
        "error message of direct status");
    zassert_true(status, NULL);

    count = Network_Port_SC_Direct_Connect_Connection_Status_Count(instance);
    zassert_equal(count, 1, NULL);

    object_value.object_property = rpdata.object_property =
        PROP_SC_DIRECT_CONNECT_CONNECTION_STATUS;

    // count (error: property is not BacArray)
    object_value.array_index = rpdata.array_index = 0;
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len == -1, NULL);

    // context (error: property is not BacArray)
    object_value.array_index = rpdata.array_index = 1;
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len == -1, NULL);

    // all context
    object_value.array_index = rpdata.array_index = BACNET_ARRAY_ALL;
    len = Network_Port_Read_Property(&rpdata);
    zassert_true(len > 0, NULL);

    str_len = 0;
    apdu_len = 0;
    while ((apdu_len < len)) {
        len2 = bacapp_decode_known_property(apdu + apdu_len, len - apdu_len,
            &value, rpdata.object_type, rpdata.object_property);
        zassert_true(len2 > 0, NULL);
        apdu_len += len2;

        len2 = bacapp_snprintf_value(NULL, 0, &object_value);
        zassert_true((len2 > 0) && (len2 < sizeof(str) - 1 - str_len), NULL);
        str_len +=
            bacapp_snprintf_value(str + str_len, len2 + 1, &object_value);
    }

    zassert_true(
        strncmp(str, DIRECT_STATUS_STR, strlen(DIRECT_STATUS_STR)) == 0, NULL);
#endif /* BSC_CONF_HUB_CONNECTORS_NUM!=0 */

#endif /* BACDL_BSC */

    return;
}

void test_main(void)
{
    ztest_test_suite(netport_tests,
     ztest_unit_test(test_network_port),
     ztest_unit_test(test_network_port_pending_param),
     ztest_unit_test(test_network_port_sc_direct_connect_accept_uri),
     ztest_unit_test(test_network_port_sc_certificates),
     ztest_unit_test(test_network_port_sc_status_encode_decode)
     );

    ztest_run_test_suite(netport_tests);
}
