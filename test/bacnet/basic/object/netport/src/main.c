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
#if BACNET_SECURE_CONNECT
    /* for decode value data */
    unsigned count = 0;
    uint32_t object_instance = 0;
    bool status = false;
#if defined(BACNET_SECURE_CONNECT_HUB) || defined(BACNET_SECURE_CONNECT_DIRECT)
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
#ifdef BACNET_SECURE_CONNECT_HUB
    Network_Port_SC_Primary_Hub_URI_Set(object_instance,
        "SC_Primary_Hub_URI_test");
    Network_Port_SC_Failover_Hub_URI_Set(object_instance,
        "SC_Failover_Hub_URI_test");
    Network_Port_SC_Hub_Function_Enable_Set(object_instance, false);
    Network_Port_SC_Hub_Function_Binding_Set(object_instance,
        "SC_Hub_Function_Binding_test");
#endif /* BACNET_SECURE_CONNECT_HUB */
#ifdef BACNET_SECURE_CONNECT_DIRECT
    Network_Port_SC_Direct_Connect_Initiate_Enable_Set(object_instance,
        false);
    Network_Port_SC_Direct_Connect_Accept_Enable_Set(object_instance,
        false);
    Network_Port_SC_Direct_Connect_Binding_Set(object_instance,
        "SC_Direct_Connect_Binding_test");
#endif /* BACNET_SECURE_CONNECT_DIRECT */

    // write dirty properties
#ifdef BACNET_SECURE_CONNECT_HUB
    Network_Port_SC_Primary_Hub_URI_Dirty_Set(object_instance,
        "SC_Primary_Hub_URI_test2");
    Network_Port_SC_Failover_Hub_URI_Dirty_Set(object_instance,
        "SC_Failover_Hub_URI_test2");
    Network_Port_SC_Hub_Function_Enable_Dirty_Set(object_instance, true);
    Network_Port_SC_Hub_Function_Binding_Dirty_Set(object_instance,
        "SC_Hub_Function_Binding_test2");
#endif /* BACNET_SECURE_CONNECT_HUB */
#ifdef BACNET_SECURE_CONNECT_DIRECT
    Network_Port_SC_Direct_Connect_Initiate_Enable_Dirty_Set(object_instance,
        true);
    Network_Port_SC_Direct_Connect_Accept_Enable_Dirty_Set(object_instance,
        true);
    Network_Port_SC_Direct_Connect_Binding_Dirty_Set(object_instance,
        "SC_Direct_Connect_Binding_test2");
#endif /* BACNET_SECURE_CONNECT_DIRECT */

    // check old value
#ifdef BACNET_SECURE_CONNECT_HUB
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
#endif /* BACNET_SECURE_CONNECT_HUB */
#ifdef BACNET_SECURE_CONNECT_DIRECT
    val = Network_Port_SC_Direct_Connect_Initiate_Enable(object_instance);
    zassert_true(val == false, NULL);
    val = Network_Port_SC_Direct_Connect_Accept_Enable(object_instance);
    zassert_true(val == false, NULL);
    Network_Port_SC_Direct_Connect_Binding(object_instance, &str);
    zassert_true(strcmp(characterstring_value(&str),
        "SC_Direct_Connect_Binding_test") == 0, NULL);
#endif /* BACNET_SECURE_CONNECT_DIRECT */

    // apply
    Network_Port_Pending_Params_Apply(object_instance);

    // check new value
#ifdef BACNET_SECURE_CONNECT_HUB
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
#endif /* BACNET_SECURE_CONNECT_HUB */
#ifdef BACNET_SECURE_CONNECT_DIRECT
    val = Network_Port_SC_Direct_Connect_Initiate_Enable(object_instance);
    zassert_true(val == true, NULL);
    val = Network_Port_SC_Direct_Connect_Accept_Enable(object_instance);
    zassert_true(val == true, NULL);
    Network_Port_SC_Direct_Connect_Binding(object_instance, &str);
    zassert_true(strcmp(characterstring_value(&str),
        "SC_Direct_Connect_Binding_test2") == 0, NULL);
#endif /* BACNET_SECURE_CONNECT_DIRECT */

#endif /* BACNET_SECURE_CONNECT */
    return;
}

static void test_network_files(void)
{
#if BACNET_SECURE_CONNECT

    BACNET_CHARACTER_STRING file_name;
    BACNET_UNSIGNED_INTEGER len;
    uint8_t buffer[2000];
    size_t i;
    uint32_t object_instance = 0;
    bool status = false;
    unsigned count = 0;
    FILE *pFile = NULL;

    /* prepare files */
    for (i = 0; i < sizeof(buffer); i++)
        buffer[i] = i % 0xff;
    zassert_true(bacfile_object_name(0, &file_name), NULL);
    pFile = fopen(characterstring_value(&file_name), "wb");
    zassert_true(pFile != NULL, NULL);
    fwrite(buffer, sizeof(buffer), 1, pFile);
    fclose(pFile);

    for (i = 0; i < 200; i++)
        buffer[i] = (i + 10) % 0xff;
    zassert_true(bacfile_object_name(1, &file_name), NULL);
    pFile = fopen(characterstring_value(&file_name), "wb");
    zassert_true(pFile != NULL, NULL);
    fwrite(buffer, 200, 1, pFile);
    fclose(pFile);

    for (i = 0; i < 300; i++)
        buffer[i] = (i + 30) % 0xff;
    zassert_true(bacfile_object_name(2, &file_name), NULL);
    pFile = fopen(characterstring_value(&file_name), "wb");
    zassert_true(pFile != NULL, NULL);
    fwrite(buffer, 300, 1, pFile);
    fclose(pFile);

    /* prepare netport */
    Network_Port_Init();
    object_instance = 1234;
    status = Network_Port_Object_Instance_Number_Set(0, object_instance);
    zassert_true(status, NULL);
    count = Network_Port_Count();
    zassert_true(count > 0, NULL);

    /* test success */
    Network_Port_Operational_Certificate_File_Set(object_instance, 0);
    Network_Port_Issuer_Certificate_File_Set(object_instance, 0, 1);
    Network_Port_Issuer_Certificate_File_Set(object_instance, 1, 2);
    Network_Port_Certificate_Signing_Request_File_Set(object_instance, 1);

    /*  Operational_Certificate_File */
    len = Network_Port_Operational_Certificate_File_Read(object_instance,
        NULL, 0);
    zassert_equal(len, 2000, NULL);

    memset(buffer, 0, sizeof(buffer));
    len = Network_Port_Operational_Certificate_File_Read(object_instance,
        buffer, sizeof(buffer));
    zassert_equal(len, 2000, NULL);
    for (i = 0; i < len; i++)
        zassert_equal(buffer[i], i % 0xff, NULL);

    /*  Issuer_Certificate_File[0] */
    len = Network_Port_Issuer_Certificate_File_Read(object_instance,
        0, NULL, 0);
    zassert_equal(len, 200, NULL);

    memset(buffer, 0, sizeof(buffer));
    len = Network_Port_Issuer_Certificate_File_Read(object_instance,
        0, buffer, sizeof(buffer));
    zassert_equal(len, 200, NULL);
    for (i = 0; i < len; i++)
        zassert_equal(buffer[i], (i + 10) % 0xff, NULL);

    /*  Issuer_Certificate_File[1] */
    len = Network_Port_Issuer_Certificate_File_Read(object_instance,
        1, NULL, 0);
    zassert_equal(len, 300, NULL);

    memset(buffer, 0, sizeof(buffer));
    len = Network_Port_Issuer_Certificate_File_Read(object_instance,
        1, buffer, sizeof(buffer));
    zassert_equal(len, 300, NULL);
    for (i = 0; i < len; i++)
        zassert_equal(buffer[i], (i + 30) % 0xff, NULL);

    /*  Issuer_Certificate_File[2] - error */
    len = Network_Port_Issuer_Certificate_File_Read(object_instance,
        2, NULL, 0);
    zassert_equal(len, 0, NULL);
    len = Network_Port_Issuer_Certificate_File_Read(object_instance,
        2, buffer, sizeof(buffer));
    zassert_equal(len, 0, NULL);

    /*  Issuer_Certificate_File[2] - error */
    len = Network_Port_Issuer_Certificate_File_Read(object_instance,
        2, NULL, 0);
    zassert_equal(len, 0, NULL);
    len = Network_Port_Issuer_Certificate_File_Read(object_instance,
        2, buffer, sizeof(buffer));
    zassert_equal(len, 0, NULL);

    /*  Certificate_Signing_Request_File_Read */
    len = Network_Port_Certificate_Signing_Request_File_Read(object_instance,
        NULL, 0);
    zassert_equal(len, 200, NULL);
    len = Network_Port_Certificate_Signing_Request_File_Read(object_instance,
        buffer, sizeof(buffer));
    zassert_equal(len, 200, NULL);
    for (i = 0; i < len; i++)
        zassert_equal(buffer[i], (i + 10) % 0xff, NULL);

    /* file instance error */
    Network_Port_Operational_Certificate_File_Set(object_instance, 10);

    len = Network_Port_Operational_Certificate_File_Read(object_instance,
        NULL, 0);
    zassert_equal(len, 0, NULL);
    len = Network_Port_Operational_Certificate_File_Read(object_instance,
        buffer, sizeof(buffer));
    zassert_equal(len, 0, NULL);

#endif /* BACNET_SECURE_CONNECT */
    return;
}

static void test_network_SC_Failed_Connection_Requests(void)
{
#if BACNET_SECURE_CONNECT

    uint32_t object_instance = 0;
    bool status = false;
    unsigned count = 0;
    int index;

    BACNET_SC_FAILED_CONNECTION_REQUEST *rec;
    BACNET_DATE_TIME ts;
    BACNET_HOST_N_PORT peer_address;
    uint8_t peer_VMAC[BACNET_PEER_VMAC_LENGHT] =
        {0x01, 0x23, 0x45, 0x67, 0x89, 0xab};
    uint8_t uuid[16] = {0};
    char *host = "bacnet";
    BACNET_ERROR_CODE err;

    /* prepare netport */
    Network_Port_Init();
    object_instance = 1234;
    status = Network_Port_Object_Instance_Number_Set(0, object_instance);
    zassert_true(status, NULL);
    count = Network_Port_Count();
    zassert_true(count > 0, NULL);

    datetime_set_values(&ts, 2000, 7, 15, 13, 30, 30, 50);
    peer_address.host_ip_address = false;
    peer_address.host_name = true;
    characterstring_init_ansi_safe(&peer_address.host.name, host, strlen(host));

    /* test add */
    count = Network_Port_SC_Failed_Connection_Requests_Count(object_instance);
    zassert_equal(count, 0, NULL);

    peer_address.port = 10010;
    index = Network_Port_SC_Failed_Connection_Requests_Add(object_instance,
        &ts, &peer_address, peer_VMAC, uuid, ERROR_CODE_UNEXPECTED_DATA,
        "error");
    zassert_true(index >= 0, NULL);

    peer_address.port = 10005;
    index = Network_Port_SC_Failed_Connection_Requests_Add(object_instance,
        &ts, &peer_address, peer_VMAC, uuid, ERROR_CODE_HTTP_NOT_A_SERVER,
        "error");
    zassert_true(index >= 0, NULL);

    peer_address.port = 10030;
    index = Network_Port_SC_Failed_Connection_Requests_Add(object_instance,
        &ts, &peer_address, peer_VMAC, uuid, ERROR_CODE_TLS_ERROR, "error");
    zassert_true(index >= 0, NULL);

    peer_address.port = 10011;
    index = Network_Port_SC_Failed_Connection_Requests_Add(object_instance,
        &ts, &peer_address, peer_VMAC, uuid, ERROR_CODE_OTHER, NULL);
    zassert_true(index >= 0, NULL);

    count = Network_Port_SC_Failed_Connection_Requests_Count(object_instance);
    zassert_equal(count, 4, NULL);

    /* test get / delete by peer */
    peer_address.port = 10010;
    rec = Network_Port_SC_Failed_Connection_Requests_Get_By_Peer(
        object_instance, &peer_address);
    zassert_true(rec != NULL, NULL);
    zassert_equal(rec->Error, ERROR_CODE_UNEXPECTED_DATA, NULL);

    peer_address.port = 10099;
    rec = Network_Port_SC_Failed_Connection_Requests_Get_By_Peer(
        object_instance, &peer_address);
    zassert_true(rec == NULL, NULL);

    peer_address.port = 10010;
    status = Network_Port_SC_Failed_Connection_Requests_Delete_By_Peer(
        object_instance, &peer_address);
    zassert_true(status, NULL);
    count = Network_Port_SC_Failed_Connection_Requests_Count(object_instance);
    zassert_equal(count, 3, NULL);

    peer_address.port = 10099;
    status = Network_Port_SC_Failed_Connection_Requests_Delete_By_Peer(
        object_instance, &peer_address);
    zassert_false(status, NULL);
    count = Network_Port_SC_Failed_Connection_Requests_Count(object_instance);
    zassert_equal(count, 3, NULL);

    /* test get / delete by index */
    rec = Network_Port_SC_Failed_Connection_Requests_Get(object_instance, 1);
    zassert_true(rec != NULL, NULL);
    err = rec->Error;

    status = Network_Port_SC_Failed_Connection_Requests_Delete_By_Index(
        object_instance, 1);
    zassert_true(status, NULL);
    count = Network_Port_SC_Failed_Connection_Requests_Count(object_instance);
    zassert_equal(count, 2, NULL);

    rec = Network_Port_SC_Failed_Connection_Requests_Get(object_instance, 1);
    zassert_true(rec != NULL, NULL);
    zassert_not_equal(rec->Error, err, NULL);

    status = Network_Port_SC_Failed_Connection_Requests_Delete_By_Index(
        object_instance, 10);
    zassert_false(status, NULL);
    count = Network_Port_SC_Failed_Connection_Requests_Count(object_instance);
    zassert_equal(count, 2, NULL);

    /* test delete all */
    status = Network_Port_SC_Failed_Connection_Requests_Delete_All(
        object_instance);
    zassert_true(status, NULL);
    count = Network_Port_SC_Failed_Connection_Requests_Count(object_instance);
    zassert_equal(count, 0, NULL);

#endif /* BACNET_SECURE_CONNECT */
    return;
}

void test_main(void)
{
    ztest_test_suite(netport_tests,
     ztest_unit_test(test_network_port),
     ztest_unit_test(test_network_port_pending_param),
     ztest_unit_test(test_network_files),
     ztest_unit_test(test_network_SC_Failed_Connection_Requests)
     );

    ztest_run_test_suite(netport_tests);
}
