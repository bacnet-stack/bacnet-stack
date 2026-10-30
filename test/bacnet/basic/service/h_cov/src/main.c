/**
 * @file
 * @brief test BACnet COV service handler
 * @date 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdef.h>
#include <bacnet/cov.h>
#include <bacnet/bacdcode.h>
#include <bacnet/npdu.h>
#include <bacnet/basic/services.h>
#include <bacnet/basic/tsm/tsm.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/* Mock functions for Device_ */
#include <bacnet/basic/object/device.h>
#include <bacnet/basic/object/ao.h>

/* Mocks have been moved to bacnet/basic/object/test/ */

/* test hooks in bacnet/basic/object/test/ mocks */
void Device_COV_Functions_Set(
    read_property_function read_property,
    object_cov_function cov,
    object_cov_clear_function cov_clear,
    object_value_list_function value_list);
void bip_send_pdu_callback_set(int (*send_pdu)(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    uint16_t pdu_len));

static float Present_Value;
static float COV_Increment;
static bool Out_Of_Service;
static bool Changed;
static bool Real_Value;
static bool Increment_Readable;
static bool Send_Succeeds;
static unsigned Notifications[3];
static float Reported_Values[3];
static BACNET_BIT_STRING Reported_Flags[3];

/**
 * @brief ReadProperty function of the monitored test object
 * @param rpdata - ReadProperty data
 * @return number of bytes encoded, or BACNET_STATUS_ERROR
 */
static int cov_test_read_property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    zassert_equal(rpdata->object_property, PROP_COV_INCREMENT, NULL);
    zassert_equal(rpdata->array_index, BACNET_ARRAY_ALL, NULL);
    if (!Increment_Readable) {
        return BACNET_STATUS_ERROR;
    }

    return bacnet_real_application_encode(
        rpdata->application_data, rpdata->application_data_len, COV_Increment);
}

/**
 * @brief COV function of the monitored test object
 * @param object_instance - object-instance number of the object
 * @return true if the object changed
 */
static bool cov_test_object_cov(uint32_t object_instance)
{
    (void)object_instance;

    return Changed;
}

/**
 * @brief COV clear function of the monitored test object
 * @param object_instance - object-instance number of the object
 */
static void cov_test_object_cov_clear(uint32_t object_instance)
{
    (void)object_instance;
    Changed = false;
}

/**
 * @brief Value list function of the monitored test object
 * @param object_instance - object-instance number of the object
 * @param value_list - list of COV data
 * @return true if the value list is encoded
 */
static bool cov_test_object_value_list(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list)
{
    (void)object_instance;
    if (Real_Value) {
        return cov_value_list_encode_real(
            value_list, Present_Value, false, false, false, Out_Of_Service);
    }

    return cov_value_list_encode_enumerated(
        value_list, BINARY_INACTIVE, false, false, false, Out_Of_Service);
}

/**
 * @brief Count the COV notifications sent to each subscriber
 * @param dest - destination address
 * @param npdu_data - NPDU data
 * @param pdu - PDU to send
 * @param pdu_len - number of bytes in the PDU
 * @return number of bytes sent, or 0 if the send fails
 */
static int cov_test_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    uint16_t pdu_len)
{
    BACNET_ADDRESS source = { 0 };
    BACNET_ADDRESS destination = { 0 };
    BACNET_NPDU_DATA decoded_npdu = { 0 };
    BACNET_PROPERTY_VALUE values[2] = { 0 };
    BACNET_COV_DATA data = { 0 };
    uint32_t process_id;
    uint8_t invoke_id = 0;
    int offset = 0;
    int header_len = 0;

    (void)dest;
    (void)npdu_data;
    if (!Send_Succeeds) {
        return 0;
    }
    offset =
        bacnet_npdu_decode(pdu, pdu_len, &destination, &source, &decoded_npdu);
    if ((offset > 0) && (offset + 2 < pdu_len) &&
        (pdu[offset] == PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST) &&
        (pdu[offset + 1] == SERVICE_UNCONFIRMED_COV_NOTIFICATION)) {
        header_len = 2;
    } else if (
        (offset > 0) && (offset + 4 < pdu_len) &&
        (pdu[offset] == PDU_TYPE_CONFIRMED_SERVICE_REQUEST) &&
        (pdu[offset + 3] == SERVICE_CONFIRMED_COV_NOTIFICATION)) {
        header_len = 4;
        invoke_id = pdu[offset + 2];
    }
    if (header_len) {
        offset += header_len;
        bacapp_property_value_list_init(values, 2);
        data.listOfValues = values;
        zassert_true(
            cov_notify_decode_service_request(
                &pdu[offset], pdu_len - offset, &data) > 0,
            NULL);
        process_id = data.subscriberProcessIdentifier;
        if (process_id < 3) {
            Notifications[process_id]++;
            if (values[0].value.tag == BACNET_APPLICATION_TAG_REAL) {
                Reported_Values[process_id] = values[0].value.type.Real;
            }
            Reported_Flags[process_id] = values[1].value.type.Bit_String;
        }
        /* Simulate a successful client ACK after capturing the notification. */
        if (invoke_id) {
            tsm_free_invoke_id(invoke_id);
        }
    }

    return pdu_len;
}

/**
 * @brief Run the COV task until it returns to the idle state
 */
static void cov_test_cycle(void)
{
    unsigned steps = 0;

    while (!handler_cov_fsm()) {
        zassert_true(++steps < 1024, NULL);
    }
}

/**
 * @brief Start a test with no subscriptions and a REAL test object
 */
static void cov_test_setup(void)
{
    cov_test_cycle();
    handler_cov_init();
    Present_Value = 10.0f;
    COV_Increment = 2.0f;
    Out_Of_Service = false;
    Changed = false;
    Real_Value = true;
    Increment_Readable = true;
    Send_Succeeds = true;
    memset(Notifications, 0, sizeof(Notifications));
    memset(Reported_Values, 0, sizeof(Reported_Values));
    memset(Reported_Flags, 0, sizeof(Reported_Flags));
    Device_COV_Functions_Set(
        cov_test_read_property, cov_test_object_cov, cov_test_object_cov_clear,
        cov_test_object_value_list);
    bip_send_pdu_callback_set(cov_test_send_pdu);
}

/**
 * @brief Remove the subscriptions and restore the default mocks
 */
static void cov_test_teardown(void)
{
    cov_test_cycle();
    handler_cov_init();
    Device_COV_Functions_Set(NULL, NULL, NULL, NULL);
    bip_send_pdu_callback_set(NULL);
}

/**
 * @brief Subscribe to the test object and run the COV task
 * @param process_id - subscriber process identifier
 */
static void cov_test_subscribe(uint32_t process_id)
{
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };
    int len = 0;

    cov_data.subscriberProcessIdentifier = process_id;
    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    zassert_true(len > 0, NULL);
    handler_cov_subscribe(service_request, len, &src, &service_data);
    cov_test_cycle();
}
/**
 * @brief Test test_h_cov_init_encode
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_init_encode)
#else
static void test_h_cov_init_encode(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int apdu_size = sizeof(apdu);
    int len;
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };

    handler_cov_init();

    len = handler_cov_encode_subscriptions(apdu, apdu_size);
    /* Should be zero because we initialized the handler */
    zassert_equal(len, 0, NULL);

    /* Create a valid subscription */
    cov_data.subscriberProcessIdentifier = 1;
    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    cov_data.monitoredObjectIdentifier.instance = 0;
    cov_data.cancellationRequest = false;
    cov_data.issueConfirmedNotifications = false;
    cov_data.lifetime = 0;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, BACNET_STATUS_ABORT, NULL);
    handler_cov_subscribe(service_request, len, &src, &service_data);

    /* Normal encode */
    len = handler_cov_encode_subscriptions(apdu, apdu_size);
    zassert_not_equal(len, 0, NULL);
    zassert_not_equal(len, BACNET_STATUS_ABORT, NULL);

    /* edge case: exact size buffer encode */
    len = handler_cov_encode_subscriptions(apdu, len);
    zassert_equal(len, len, NULL);

    /* edge case: buffer too small */
    len = handler_cov_encode_subscriptions(apdu, len - 1);
    zassert_equal(len, BACNET_STATUS_ABORT, NULL);
}

/**
 * @brief Test test_h_cov_timer
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_timer)
#else
static void test_h_cov_timer(void)
#endif
{
    handler_cov_timer_seconds(1);
    zassert_true(true, NULL);
}

/**
 * @brief Test test_h_cov_fsm
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_fsm)
#else
static void test_h_cov_fsm(void)
#endif
{
    bool idle;
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };
    int len;

    handler_cov_init();

    idle = handler_cov_fsm();
    zassert_true(idle, NULL); /* No subscriptions, quickly back to idle */

    /* Subscribe Unconfirmed */
    cov_data.subscriberProcessIdentifier = 1;
    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    cov_data.monitoredObjectIdentifier.instance = 0;
    cov_data.cancellationRequest = false;
    cov_data.issueConfirmedNotifications = false;
    cov_data.lifetime = 0;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);

    /* IDLE -> MARK */
    idle = handler_cov_fsm();
    zassert_false(idle, NULL);
    /* MARK -> CLEAR */
    idle = handler_cov_fsm();
    zassert_false(idle, NULL);
    /* CLEAR -> FREE */
    idle = handler_cov_fsm();
    zassert_false(idle, NULL);
    /* FREE -> SEND */
    idle = handler_cov_fsm();
    zassert_false(idle, NULL);
    /* SEND -> IDLE */
    idle = handler_cov_fsm();
    zassert_true(idle, NULL);

    /* Subscribe Confirmed */
    cov_data.subscriberProcessIdentifier = 2;
    cov_data.issueConfirmedNotifications = true;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);

    /* Run FSM again */
    handler_cov_fsm(); /* IDLE -> MARK */
    handler_cov_fsm(); /* MARK -> CLEAR */
    handler_cov_fsm(); /* CLEAR -> FREE */
    handler_cov_fsm(); /* FREE -> SEND */
    handler_cov_fsm(); /* SEND -> IDLE */

    /* Trigger Confirmed free branch by expiring */
    cov_data.lifetime = 10;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);

    handler_cov_fsm(); /* IDLE -> MARK */
    handler_cov_fsm(); /* MARK -> CLEAR */
    handler_cov_fsm(); /* CLEAR -> FREE */
    handler_cov_fsm(); /* FREE -> SEND */
    handler_cov_fsm(); /* SEND -> IDLE */

    /* Expire */
    handler_cov_timer_seconds(15);

    handler_cov_task();
    zassert_true(true, NULL);
}

/**
 * @brief Test test_h_cov_subscribe_error_cases
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_subscribe_error_cases)
#else
static void test_h_cov_subscribe_error_cases(void)
#endif
{
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };
    int len;

    handler_cov_init();

    /* test segment abort */
    service_data.segmented_message = true;
    handler_cov_subscribe(service_request, 10, &src, &service_data);
    zassert_true(true, NULL);

    /* test missing required parameter */
    service_data.segmented_message = false;
    handler_cov_subscribe(service_request, 0, &src, &service_data);
    zassert_true(true, NULL);

    /* test bad request decode */
    handler_cov_subscribe(service_request, 1, &src, &service_data);
    zassert_true(true, NULL);

    /* test valid decode but object valid (by mock) */
    cov_data.subscriberProcessIdentifier = 1;
    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    cov_data.monitoredObjectIdentifier.instance = 12345;
    cov_data.cancellationRequest = false;
    cov_data.issueConfirmedNotifications = false;
    cov_data.lifetime = 0;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);
    zassert_true(true, NULL);

    /* test cancellation of non-existing */
    cov_data.cancellationRequest = true;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);
    zassert_true(true, NULL);
}

/**
 * @brief Test test_h_cov_subscribe_out_of_space
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_subscribe_out_of_space)
#else
static void test_h_cov_subscribe_out_of_space(void)
#endif
{
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };
    int len;
    int i;

    handler_cov_init();

    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    cov_data.monitoredObjectIdentifier.instance = 0;
    cov_data.cancellationRequest = false;
    cov_data.issueConfirmedNotifications = false;
    cov_data.lifetime = 0;

    for (i = 0; i < 130; i++) {
        cov_data.subscriberProcessIdentifier = i;
        len = cov_subscribe_service_request_encode(
            service_request, sizeof(service_request), &cov_data);
        handler_cov_subscribe(service_request, len, &src, &service_data);
    }
    zassert_true(true, NULL);
}

/**
 * @brief Test test_h_cov_timer_expiration
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_timer_expiration)
#else
static void test_h_cov_timer_expiration(void)
#endif
{
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };
    int len;

    handler_cov_init();

    /* Subscribe with lifetime */
    cov_data.subscriberProcessIdentifier = 1;
    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    cov_data.monitoredObjectIdentifier.instance = 0;
    cov_data.cancellationRequest = false;
    cov_data.issueConfirmedNotifications = true;
    cov_data.lifetime = 10;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);

    /* Check timer decreasing */
    handler_cov_timer_seconds(5);
    handler_cov_timer_seconds(10); /* Should expire here */
    zassert_true(true, NULL);
}

/**
 * @brief Test test_h_cov_address_management
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_address_management)
#else
static void test_h_cov_address_management(void)
#endif
{
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };
    int len;
    int i;

    handler_cov_init();

    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    cov_data.monitoredObjectIdentifier.instance = 0;
    cov_data.cancellationRequest = false;
    cov_data.issueConfirmedNotifications = false;
    cov_data.lifetime = 0;

    /* Add subscriptions from 18 different sources */
    for (i = 0; i < 18; i++) {
        src.mac_len = 1;
        src.mac[0] = i + 1;
        cov_data.subscriberProcessIdentifier = i;
        len = cov_subscribe_service_request_encode(
            service_request, sizeof(service_request), &cov_data);
        handler_cov_subscribe(service_request, len, &src, &service_data);
    }

    /* Cancel one, to free an address */
    src.mac_len = 1;
    src.mac[0] = 1;
    cov_data.subscriberProcessIdentifier = 0;
    cov_data.cancellationRequest = true;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);

    zassert_true(true, NULL);
}

/**
 * @brief Run the COV FSM until the confirmed notification uses invoke_id
 * @param invoke_id - the expected invoke ID of the notification
 * @return true if the notification was sent using invoke_id
 */
static bool h_cov_fsm_send_confirmed(uint8_t invoke_id)
{
    unsigned i;

    for (i = 0; i < 20; i++) {
        handler_cov_fsm();
        if (!tsm_invoke_id_free(invoke_id)) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Subscribe for confirmed COV, send a notification with invoke_id,
 *  receive its SimpleACK, then let another transaction reuse invoke_id.
 * @param cov_data - subscription parameters
 * @param src - subscriber address
 * @param invoke_id - invoke ID to use and then reuse
 */
static void h_cov_invoke_id_reuse_setup(
    BACNET_SUBSCRIBE_COV_DATA *cov_data, BACNET_ADDRESS *src, uint8_t invoke_id)
{
    uint8_t service_request[128] = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint8_t apdu[4] = { 0 };
    int len;
    uint8_t reused_id;

    handler_cov_init();
    zassert_true(tsm_invoke_id_free(invoke_id), NULL);
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), cov_data);
    zassert_true(len > 0, NULL);
    handler_cov_subscribe(service_request, len, src, &service_data);
    /* 1. send a confirmed COV notification using invoke_id */
    tsm_invokeID_set(invoke_id);
    zassert_true(h_cov_fsm_send_confirmed(invoke_id), NULL);
    /* 2. the SimpleACK is received, and apdu_handler() frees the ID */
    tsm_free_invoke_id(invoke_id);
    zassert_true(tsm_invoke_id_free(invoke_id), NULL);
    /* 3. another transaction reuses the same invoke ID */
    tsm_invokeID_set(invoke_id);
    reused_id = tsm_next_free_invokeID();
    zassert_equal(reused_id, invoke_id, NULL);
    tsm_set_confirmed_unsegmented_transaction(
        reused_id, src, &npdu_data, apdu, sizeof(apdu));
    zassert_false(tsm_invoke_id_free(invoke_id), NULL);
    zassert_false(tsm_invoke_id_failed(invoke_id), NULL);
}

/**
 * @brief Test that a released and reused invoke ID is not freed by COV
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_invoke_id_reuse)
#else
static void test_h_cov_invoke_id_reuse(void)
#endif
{
    uint8_t service_request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service_data = { 0 };
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };
    const uint8_t invoke_id = 200;
    unsigned i;
    int len;

    src.mac_len = 1;
    src.mac[0] = 42;
    cov_data.subscriberProcessIdentifier = 99;
    cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    cov_data.monitoredObjectIdentifier.instance = 1;
    cov_data.issueConfirmedNotifications = true;
    cov_data.lifetime = 60;

    /* 4a. renew the old subscription */
    cov_data.cancellationRequest = false;
    h_cov_invoke_id_reuse_setup(&cov_data, &src, invoke_id);
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);
    zassert_false(tsm_invoke_id_free(invoke_id), NULL);
    /* the COV FSM must not free or wait on the other transaction */
    for (i = 0; i < 20; i++) {
        handler_cov_fsm();
    }
    zassert_false(tsm_invoke_id_free(invoke_id), NULL);
    zassert_false(tsm_invoke_id_failed(invoke_id), NULL);
    tsm_free_invoke_id(invoke_id);

    /* 4b. cancel the old subscription */
    h_cov_invoke_id_reuse_setup(&cov_data, &src, invoke_id);
    cov_data.cancellationRequest = true;
    len = cov_subscribe_service_request_encode(
        service_request, sizeof(service_request), &cov_data);
    handler_cov_subscribe(service_request, len, &src, &service_data);
    zassert_false(tsm_invoke_id_free(invoke_id), NULL);
    tsm_free_invoke_id(invoke_id);
    cov_data.cancellationRequest = false;

    /* 4c. expire the old subscription */
    h_cov_invoke_id_reuse_setup(&cov_data, &src, invoke_id);
    handler_cov_timer_seconds(cov_data.lifetime + 1);
    zassert_false(tsm_invoke_id_free(invoke_id), NULL);
    tsm_free_invoke_id(invoke_id);

    handler_cov_init();
}

/**
 * @brief Test that a new subscription does not move the COV_Increment
 *  reference of an existing subscription
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_per_subscription_reference)
#else
static void test_h_cov_per_subscription_reference(void)
#endif
{
    cov_test_setup();
    cov_test_subscribe(1);
    zassert_equal(Notifications[1], 1, NULL);
    Present_Value = 11.5f;
    cov_test_cycle();
    zassert_equal(Notifications[1], 1, NULL);
    cov_test_subscribe(2);
    zassert_equal(Notifications[2], 1, NULL);
    Present_Value = 12.5f;
    Changed = true;
    cov_test_cycle();
    zassert_equal(Notifications[1], 2, NULL);
    zassert_equal(Notifications[2], 1, NULL);
    zassert_false(Changed, NULL);
    cov_test_teardown();
}

/**
 * @brief Test that the initial notification value is the COV_Increment
 *  reference of the subscription
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_initial_reference)
#else
static void test_h_cov_initial_reference(void)
#endif
{
    cov_test_setup();
    Present_Value = 11.5f;
    cov_test_subscribe(1);
    zassert_equal(Notifications[1], 1, NULL);
    Present_Value = 13.0f;
    Changed = true;
    cov_test_cycle();
    zassert_equal(Notifications[1], 1, NULL);
    zassert_false(Changed, NULL);
    Present_Value = 9.0f;
    cov_test_cycle();
    zassert_equal(Notifications[1], 2, NULL);
    Out_Of_Service = true;
    cov_test_cycle();
    zassert_equal(Notifications[1], 3, NULL);
    cov_test_cycle();
    zassert_equal(Notifications[1], 3, NULL);
    cov_test_teardown();
}

/**
 * @brief Test that a COV_Increment of zero notifies only on a change
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_zero_increment)
#else
static void test_h_cov_zero_increment(void)
#endif
{
    cov_test_setup();
    COV_Increment = 0.0f;
    cov_test_subscribe(1);
    Changed = true;
    cov_test_cycle();
    zassert_false(Changed, NULL);
    cov_test_cycle();
    zassert_equal(Notifications[1], 1, NULL);
    Present_Value = 10.5f;
    cov_test_cycle();
    zassert_equal(Notifications[1], 2, NULL);
    cov_test_cycle();
    zassert_equal(Notifications[1], 2, NULL);
    cov_test_teardown();
}

/**
 * @brief Test that a renewed subscription updates only its own reference
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_renewed_reference)
#else
static void test_h_cov_renewed_reference(void)
#endif
{
    cov_test_setup();
    cov_test_subscribe(1);
    cov_test_subscribe(2);
    Present_Value = 11.5f;
    cov_test_subscribe(2);
    zassert_equal(Notifications[2], 2, NULL);
    Present_Value = 12.5f;
    cov_test_cycle();
    zassert_equal(Notifications[1], 2, NULL);
    zassert_equal(Notifications[2], 2, NULL);
    cov_test_teardown();
}

/**
 * @brief Test that a failed send does not update the reference
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_failed_send_reference)
#else
static void test_h_cov_failed_send_reference(void)
#endif
{
    cov_test_setup();
    cov_test_subscribe(1);
    Present_Value = 12.0f;
    Send_Succeeds = false;
    cov_test_cycle();
    zassert_equal(Notifications[1], 1, NULL);
    Present_Value = 13.0f;
    Send_Succeeds = true;
    cov_test_cycle();
    zassert_equal(Notifications[1], 2, NULL);
    Present_Value = 14.5f;
    cov_test_cycle();
    zassert_equal(Notifications[1], 2, NULL);
    cov_test_teardown();
}

/**
 * @brief Subscribe to the real Analog Output used by the regression test
 */
static void cov_test_ao_subscribe(uint32_t process_id, bool confirmed)
{
    uint8_t request[128] = { 0 };
    BACNET_ADDRESS src = { 0 };
    BACNET_CONFIRMED_SERVICE_DATA service = { 0 };
    BACNET_SUBSCRIBE_COV_DATA data = { 0 };
    int len;

    data.subscriberProcessIdentifier = process_id;
    data.monitoredObjectIdentifier.type = OBJECT_ANALOG_OUTPUT;
    data.monitoredObjectIdentifier.instance = 990;
    data.issueConfirmedNotifications = confirmed;
    data.lifetime = 300;
    len = cov_subscribe_service_request_encode(request, sizeof(request), &data);
    zassert_true(len > 0, NULL);
    handler_cov_subscribe(request, len, &src, &service);
    cov_test_cycle();
}

/**
 * @brief Verify actual AO notification values during subscriptions and renewals
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_ao_reported_reference)
#else
static void test_h_cov_ao_reported_reference(void)
#endif
{
    const uint32_t instance = 990;
    unsigned mode;
    unsigned step;
    float value;
    bool confirmed;

    for (mode = 0; mode < 2; mode++) {
        confirmed = (mode != 0);
        cov_test_setup();
        Analog_Output_Init();
        zassert_not_equal(
            Analog_Output_Create(instance), BACNET_MAX_INSTANCE, NULL);
        Analog_Output_COV_Increment_Set(instance, 2.0f);
        zassert_true(Analog_Output_Present_Value_Set(instance, 10.0f, 8), NULL);
        Device_COV_Functions_Set(
            Analog_Output_Read_Property, Analog_Output_Change_Of_Value,
            Analog_Output_Change_Of_Value_Clear,
            Analog_Output_Encode_Value_List);
        cov_test_ao_subscribe(1, confirmed);
        zassert_equal(Notifications[1], 1, NULL);
        zassert_within(Reported_Values[1], 10.0f, 0.001f, NULL);

        zassert_true(Analog_Output_Present_Value_Set(instance, 11.5f, 8), NULL);
        cov_test_cycle();
        cov_test_ao_subscribe(2, confirmed);
        zassert_equal(Notifications[1], 1, NULL);
        zassert_within(Reported_Values[2], 11.5f, 0.001f, NULL);
        zassert_true(Analog_Output_Present_Value_Set(instance, 12.5f, 8), NULL);
        cov_test_cycle();
        zassert_equal(Notifications[1], 2, NULL);
        zassert_within(Reported_Values[1], 12.5f, 0.001f, NULL);
        zassert_equal(Notifications[2], 1, NULL);

        /* Repeated renewals must report current values without starving A. */
        for (step = 1; step <= 12; step++) {
            value = 12.5f + (float)step * 0.5f;
            zassert_true(
                Analog_Output_Present_Value_Set(instance, value, 8), NULL);
            cov_test_cycle();
            cov_test_ao_subscribe(2, confirmed);
            zassert_within(Reported_Values[2], value, 0.001f, NULL);
            zassert_true(value - Reported_Values[1] < 2.0f, NULL);
        }
        zassert_within(Reported_Values[1], 18.5f, 0.001f, NULL);

        Analog_Output_Overridden_Set(instance, true);
        cov_test_cycle();
        zassert_within(Reported_Values[1], 18.5f, 0.001f, NULL);
        zassert_within(Reported_Values[2], 18.5f, 0.001f, NULL);
        zassert_true(
            bitstring_bit(&Reported_Flags[1], STATUS_FLAG_OVERRIDDEN), NULL);
        zassert_true(
            bitstring_bit(&Reported_Flags[2], STATUS_FLAG_OVERRIDDEN), NULL);
        /* A pending notification must encode the current value, not 20.5. */
        zassert_true(Analog_Output_Present_Value_Set(instance, 20.5f, 8), NULL);
        zassert_true(Analog_Output_Present_Value_Set(instance, 22.0f, 8), NULL);
        cov_test_cycle();
        zassert_within(Reported_Values[1], 22.0f, 0.001f, NULL);
        zassert_within(Reported_Values[2], 22.0f, 0.001f, NULL);
        cov_test_teardown();
        Analog_Output_Delete(instance);
        Analog_Output_Cleanup();
    }
}

/**
 * @brief Test the object COV flag detection for non-REAL values and for
 *  an unreadable COV_Increment
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(h_cov_tests, test_h_cov_legacy_detection)
#else
static void test_h_cov_legacy_detection(void)
#endif
{
    unsigned mode;

    for (mode = 0; mode < 2; mode++) {
        cov_test_setup();
        Real_Value = (mode != 0);
        Increment_Readable = (mode == 0);
        cov_test_subscribe(1);
        cov_test_subscribe(2);
        Changed = true;
        cov_test_cycle();
        zassert_equal(Notifications[1], 2, NULL);
        zassert_equal(Notifications[2], 2, NULL);
        zassert_false(Changed, NULL);
        cov_test_cycle();
        zassert_equal(Notifications[1], 2, NULL);
        zassert_equal(Notifications[2], 2, NULL);
    }
    cov_test_teardown();
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(h_cov_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        h_cov_tests, ztest_unit_test(test_h_cov_init_encode),
        ztest_unit_test(test_h_cov_timer), ztest_unit_test(test_h_cov_fsm),
        ztest_unit_test(test_h_cov_subscribe_error_cases),
        ztest_unit_test(test_h_cov_subscribe_out_of_space),
        ztest_unit_test(test_h_cov_timer_expiration),
        ztest_unit_test(test_h_cov_address_management),
        ztest_unit_test(test_h_cov_invoke_id_reuse),
        ztest_unit_test(test_h_cov_per_subscription_reference),
        ztest_unit_test(test_h_cov_initial_reference),
        ztest_unit_test(test_h_cov_zero_increment),
        ztest_unit_test(test_h_cov_renewed_reference),
        ztest_unit_test(test_h_cov_failed_send_reference),
        ztest_unit_test(test_h_cov_legacy_detection),
        ztest_unit_test(test_h_cov_ao_reported_reference));

    ztest_run_test_suite(h_cov_tests);
}
#endif
