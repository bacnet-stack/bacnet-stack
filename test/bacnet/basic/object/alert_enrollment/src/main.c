/**
 * @file
 * @brief Unit test for Alert Enrollment object
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/alert_enrollment.h>
#include <bacnet/bactext.h>
#include <bacnet/proplist.h>
#include <property_test.h>
#include "stubs.h"

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test the Alert Enrollment object ReadProperty/WriteProperty
 * and object-name APIs against the generic property test harness
 */
static void testAlert_Enrollment(void)
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int32_t skip_fail_property_list[] = { -1 };

    Alert_Enrollment_Init();
    object_instance = Alert_Enrollment_Create(object_instance);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);
    count = Alert_Enrollment_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Alert_Enrollment_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    zassert_equal(Alert_Enrollment_Instance_To_Index(object_instance), 0, NULL);
    zassert_true(Alert_Enrollment_Valid_Instance(object_instance), NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_ALERT_ENROLLMENT, object_instance,
        Alert_Enrollment_Property_Lists, Alert_Enrollment_Read_Property,
        Alert_Enrollment_Write_Property, skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Alert_Enrollment_Name_Set,
        Alert_Enrollment_Name_ASCII);
    /* cleanup */
    status = Alert_Enrollment_Delete(object_instance);
    zassert_true(status, NULL);
    zassert_false(Alert_Enrollment_Valid_Instance(object_instance), NULL);
    Alert_Enrollment_Cleanup();
}

/**
 * @brief Test direct Alert Enrollment property and alert-queue APIs
 */
static void testAlert_Enrollment_APIs(void)
{
    const uint32_t instance = 123;
    const uint32_t invalid_instance = instance + 1;
    const char *sample_description = "Test Alert Enrollment";
    char sample_context[] = "context";
    bool status = false;
    uint32_t test_instance = BACNET_MAX_INSTANCE;
    BACNET_OBJECT_ID value = { 0 };
    BACNET_ALERT alert = { 0 };
    unsigned i;

    Alert_Enrollment_Init();
    test_instance = Alert_Enrollment_Create(instance);
    zassert_equal(test_instance, instance, NULL);
    zassert_true(Alert_Enrollment_Valid_Instance(instance), NULL);
    zassert_false(Alert_Enrollment_Valid_Instance(invalid_instance), NULL);

    /* default present-value is Device,MAX_INSTANCE */
    value = Alert_Enrollment_Present_Value(instance);
    zassert_equal(value.type, OBJECT_DEVICE, NULL);
    zassert_equal(value.instance, BACNET_MAX_INSTANCE, NULL);

    /* present-value get/set */
    value.type = OBJECT_ANALOG_INPUT;
    value.instance = 42;
    Alert_Enrollment_Present_Value_Set(instance, value);
    value = Alert_Enrollment_Present_Value(instance);
    zassert_equal(value.type, OBJECT_ANALOG_INPUT, NULL);
    zassert_equal(value.instance, 42, NULL);

    /* description get/set */
    zassert_is_null(Alert_Enrollment_Description(instance), NULL);
    status = Alert_Enrollment_Description_Set(instance, sample_description);
    zassert_true(status, NULL);
    zassert_equal(
        Alert_Enrollment_Description(instance), sample_description, NULL);
    status =
        Alert_Enrollment_Description_Set(invalid_instance, sample_description);
    zassert_false(status, NULL);
    zassert_is_null(Alert_Enrollment_Description(invalid_instance), NULL);

    /* notification-class get/set */
    zassert_equal(
        Alert_Enrollment_Notification_Class(instance), BACNET_MAX_INSTANCE,
        NULL);
    status = Alert_Enrollment_Notification_Class_Set(instance, 7);
    zassert_true(status, NULL);
    zassert_equal(Alert_Enrollment_Notification_Class(instance), 7, NULL);
    status = Alert_Enrollment_Notification_Class_Set(invalid_instance, 3);
    zassert_false(status, NULL);
    zassert_equal(
        Alert_Enrollment_Notification_Class(invalid_instance),
        BACNET_MAX_INSTANCE, NULL);

    /* context get/set */
    zassert_is_null(Alert_Enrollment_Context_Get(instance), NULL);
    Alert_Enrollment_Context_Set(instance, (void *)sample_context);
    zassert_equal(Alert_Enrollment_Context_Get(instance), sample_context, NULL);
    zassert_is_null(Alert_Enrollment_Context_Get(invalid_instance), NULL);

    /* alert queue: NULL and invalid-instance arguments are rejected */
    zassert_false(Alert_Enrollment_Queue_Alert(instance, NULL), NULL);
    zassert_false(Alert_Enrollment_Queue_Alert(invalid_instance, &alert), NULL);

    /* queuing an alert updates present-value to the alert source */
    alert.source.type = OBJECT_BINARY_INPUT;
    alert.source.instance = 99;
    alert.vendorID = 260;
    alert.extendedEventType = 42;
    characterstring_buffer_ansi_init(&alert.messageText, "sample alert");
    status = Alert_Enrollment_Queue_Alert(instance, &alert);
    zassert_true(status, NULL);
    value = Alert_Enrollment_Present_Value(instance);
    zassert_equal(value.type, OBJECT_BINARY_INPUT, NULL);
    zassert_equal(value.instance, 99, NULL);

    /* fill the ring buffer to capacity, then verify it rejects further
       alerts until the intrinsic-reporting hook drains it */
    for (i = 0; i < (ALERT_ENROLLMENT_ALERT_COUNT - 1); i++) {
        characterstring_buffer_ansi_init(&alert.messageText, "queued");
        status = Alert_Enrollment_Queue_Alert(instance, &alert);
        zassert_true(status, NULL);
    }
    characterstring_buffer_ansi_init(&alert.messageText, "overflow");
    status = Alert_Enrollment_Queue_Alert(instance, &alert);
    zassert_false(status, NULL);

    /* draining via the intrinsic-reporting hook must not fault, and
       must make room in the queue again */
    Alert_Enrollment_Intrinsic_Reporting(instance);
    characterstring_buffer_ansi_init(&alert.messageText, "post-drain");
    status = Alert_Enrollment_Queue_Alert(instance, &alert);
    zassert_true(status, NULL);
    Alert_Enrollment_Intrinsic_Reporting(instance);

    /* invalid instance is a no-op, not a fault */
    Alert_Enrollment_Intrinsic_Reporting(invalid_instance);

    status = Alert_Enrollment_Delete(instance);
    zassert_true(status, NULL);
    zassert_false(Alert_Enrollment_Valid_Instance(instance), NULL);
    Alert_Enrollment_Cleanup();
}

/**
 * @brief Test that Alert_Enrollment_Intrinsic_Reporting reports each queued
 * alert's data through Notification_Class_common_reporting_function
 * unmodified, in FIFO order, and drains the queue
 */
static void testAlert_Enrollment_Notification_Reporting(void)
{
    const uint32_t instance = 789;
    bool status = false;
    BACNET_ALERT alert1 = { 0 };
    BACNET_ALERT alert2 = { 0 };
    BACNET_CHARACTER_STRING expected_text;
    const BACNET_EVENT_NOTIFICATION_DATA *event_data;
    const BACNET_CHARACTER_STRING *message_text;

    Alert_Enrollment_Init();
    zassert_not_equal(
        Alert_Enrollment_Create(instance), BACNET_MAX_INSTANCE, NULL);
    status = Alert_Enrollment_Notification_Class_Set(instance, 11);
    zassert_true(status, NULL);

    alert1.source.type = OBJECT_BINARY_INPUT;
    alert1.source.instance = 7;
    alert1.vendorID = 260;
    alert1.extendedEventType = 55;
    alert1.timeStamp.tag = TIME_STAMP_DATETIME;
    alert1.timeStamp.value.dateTime.date.year = 2024;
    alert1.timeStamp.value.dateTime.date.month = 6;
    alert1.timeStamp.value.dateTime.date.day = 15;
    alert1.timeStamp.value.dateTime.time.hour = 10;
    alert1.timeStamp.value.dateTime.time.min = 30;
    alert1.timeStamp.value.dateTime.time.sec = 0;
    alert1.timeStamp.value.dateTime.time.hundredths = 0;
    characterstring_buffer_ansi_init(&alert1.messageText, "first alert");

    /* second alert uses distinct values so the test can catch alerts
       being mixed up, reused, or reported out of order */
    alert2.source.type = OBJECT_ANALOG_INPUT;
    alert2.source.instance = 21;
    alert2.vendorID = 999;
    alert2.extendedEventType = 77;
    alert2.timeStamp.tag = TIME_STAMP_DATETIME;
    alert2.timeStamp.value.dateTime.date.year = 2024;
    alert2.timeStamp.value.dateTime.date.month = 7;
    alert2.timeStamp.value.dateTime.date.day = 4;
    alert2.timeStamp.value.dateTime.time.hour = 18;
    alert2.timeStamp.value.dateTime.time.min = 45;
    alert2.timeStamp.value.dateTime.time.sec = 12;
    alert2.timeStamp.value.dateTime.time.hundredths = 50;
    characterstring_buffer_ansi_init(&alert2.messageText, "second alert");

    Reporting_Stub_Reset();
    status = Alert_Enrollment_Queue_Alert(instance, &alert1);
    zassert_true(status, NULL);
    status = Alert_Enrollment_Queue_Alert(instance, &alert2);
    zassert_true(status, NULL);

    Alert_Enrollment_Intrinsic_Reporting(instance);
    zassert_equal(Reporting_Stub_Call_Count(), 2, NULL);

    /* first notification must reflect alert1's data exactly */
    event_data = Reporting_Stub_Event_Data(0);
    zassert_not_null(event_data, NULL);
    zassert_equal(event_data->eventType, EVENT_EXTENDED, NULL);
    zassert_equal(event_data->fromState, EVENT_STATE_NORMAL, NULL);
    zassert_equal(event_data->toState, EVENT_STATE_NORMAL, NULL);
    zassert_equal(event_data->notifyType, NOTIFY_EVENT, NULL);
    zassert_equal(
        event_data->eventObjectIdentifier.type, OBJECT_ALERT_ENROLLMENT, NULL);
    zassert_equal(event_data->eventObjectIdentifier.instance, instance, NULL);
    zassert_equal(event_data->notificationClass, 11, NULL);
    zassert_equal(event_data->timeStamp.tag, TIME_STAMP_DATETIME, NULL);
    zassert_equal(event_data->timeStamp.value.dateTime.date.year, 2024, NULL);
    zassert_equal(event_data->timeStamp.value.dateTime.date.month, 6, NULL);
    zassert_equal(event_data->timeStamp.value.dateTime.date.day, 15, NULL);
    zassert_equal(event_data->timeStamp.value.dateTime.time.hour, 10, NULL);
    zassert_equal(event_data->timeStamp.value.dateTime.time.min, 30, NULL);
    zassert_equal(event_data->notificationParams.extended.vendorID, 260, NULL);
    zassert_equal(
        event_data->notificationParams.extended.extendedEventType, 55, NULL);
    zassert_equal(
        event_data->notificationParams.extended.parameters.tag,
        BACNET_APPLICATION_TAG_OBJECT_ID, NULL);
    zassert_equal(
        event_data->notificationParams.extended.parameters.type.Object_Id.type,
        OBJECT_BINARY_INPUT, NULL);
    zassert_equal(
        event_data->notificationParams.extended.parameters.type.Object_Id
            .instance,
        7, NULL);
    message_text = Reporting_Stub_Message_Text(0);
    zassert_not_null(message_text, NULL);
    characterstring_init_ansi(&expected_text, "first alert");
    zassert_true(characterstring_same(message_text, &expected_text), NULL);

    /* second notification must reflect alert2's data, not alert1's */
    event_data = Reporting_Stub_Event_Data(1);
    zassert_not_null(event_data, NULL);
    zassert_equal(event_data->eventObjectIdentifier.instance, instance, NULL);
    zassert_equal(event_data->notificationClass, 11, NULL);
    zassert_equal(event_data->timeStamp.value.dateTime.date.month, 7, NULL);
    zassert_equal(event_data->timeStamp.value.dateTime.date.day, 4, NULL);
    zassert_equal(event_data->timeStamp.value.dateTime.time.hour, 18, NULL);
    zassert_equal(event_data->timeStamp.value.dateTime.time.min, 45, NULL);
    zassert_equal(event_data->timeStamp.value.dateTime.time.sec, 12, NULL);
    zassert_equal(
        event_data->timeStamp.value.dateTime.time.hundredths, 50, NULL);
    zassert_equal(event_data->notificationParams.extended.vendorID, 999, NULL);
    zassert_equal(
        event_data->notificationParams.extended.extendedEventType, 77, NULL);
    zassert_equal(
        event_data->notificationParams.extended.parameters.type.Object_Id.type,
        OBJECT_ANALOG_INPUT, NULL);
    zassert_equal(
        event_data->notificationParams.extended.parameters.type.Object_Id
            .instance,
        21, NULL);
    message_text = Reporting_Stub_Message_Text(1);
    zassert_not_null(message_text, NULL);
    characterstring_init_ansi(&expected_text, "second alert");
    zassert_true(characterstring_same(message_text, &expected_text), NULL);

    /* the queue is now drained: a further call must not report again */
    Reporting_Stub_Reset();
    Alert_Enrollment_Intrinsic_Reporting(instance);
    zassert_equal(Reporting_Stub_Call_Count(), 0, NULL);

    Alert_Enrollment_Delete(instance);
    Alert_Enrollment_Cleanup();
}

/**
 * @brief Test the Alert Enrollment Writable_Property_List API
 */
static void testAlert_Enrollment_Writable_Properties(void)
{
    const uint32_t instance = 456;
    const uint32_t invalid_instance = instance + 1;
    const int32_t *properties = NULL;

    Alert_Enrollment_Init();
    zassert_not_equal(
        Alert_Enrollment_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* Alert Enrollment has no always-writable properties */
    Alert_Enrollment_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    zassert_equal(properties[0], -1, NULL);

    /* unknown instance: must still return a valid (empty) list */
    properties = NULL;
    Alert_Enrollment_Writable_Property_List(invalid_instance, &properties);
    zassert_not_null(properties, NULL);
    zassert_equal(properties[0], -1, NULL);

    /* NULL properties pointer: must not crash */
    Alert_Enrollment_Writable_Property_List(instance, NULL);

    Alert_Enrollment_Delete(instance);
    Alert_Enrollment_Cleanup();
}
/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(
        alert_enrollment_tests, ztest_unit_test(testAlert_Enrollment),
        ztest_unit_test(testAlert_Enrollment_APIs),
        ztest_unit_test(testAlert_Enrollment_Notification_Reporting),
        ztest_unit_test(testAlert_Enrollment_Writable_Properties));

    ztest_run_test_suite(alert_enrollment_tests);
}
