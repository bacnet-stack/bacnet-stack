/**
 * @file
 * @brief test BACnetEventNotification encoding and decoding API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/event.h>
#include <bacnet/bactext.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

static BACNET_EVENT_NOTIFICATION_DATA Data;
static BACNET_CHARACTER_STRING Message_Text;
static BACNET_EVENT_NOTIFICATION_DATA Test_Data;
static BACNET_CHARACTER_STRING Test_Message_Text;

/**
 * @brief Helper test to BACnet event handlers
 */
static void verifyBaseEventState(void)
{
    zassert_equal(Data.processIdentifier, Test_Data.processIdentifier, NULL);
    zassert_equal(
        Data.initiatingObjectIdentifier.instance,
        Test_Data.initiatingObjectIdentifier.instance, NULL);
    zassert_equal(
        Data.initiatingObjectIdentifier.type,
        Test_Data.initiatingObjectIdentifier.type, NULL);
    zassert_equal(
        Data.eventObjectIdentifier.instance,
        Test_Data.eventObjectIdentifier.instance, NULL);
    zassert_equal(
        Data.eventObjectIdentifier.type, Test_Data.eventObjectIdentifier.type,
        NULL);
    zassert_equal(Data.notificationClass, Test_Data.notificationClass, NULL);
    zassert_equal(Data.priority, Test_Data.priority, NULL);
    zassert_equal(Data.notifyType, Test_Data.notifyType, NULL);
    zassert_equal(Data.fromState, Test_Data.fromState, NULL);
    zassert_equal(Data.toState, Test_Data.toState, NULL);
    zassert_equal(Data.toState, Test_Data.toState, NULL);

    if (Data.messageText != NULL && Test_Data.messageText != NULL) {
        zassert_equal(
            Data.messageText->encoding, Test_Data.messageText->encoding, NULL);
        zassert_equal(
            Data.messageText->length, Test_Data.messageText->length, NULL);
        zassert_equal(
            strcmp(Data.messageText->value, Test_Data.messageText->value), 0,
            NULL);
    }

    zassert_equal(Data.timeStamp.tag, Test_Data.timeStamp.tag, NULL);

    switch (Data.timeStamp.tag) {
        case TIME_STAMP_SEQUENCE:
            zassert_equal(
                Data.timeStamp.value.sequenceNum,
                Test_Data.timeStamp.value.sequenceNum, NULL);
            break;

        case TIME_STAMP_DATETIME:
            zassert_equal(
                Data.timeStamp.value.dateTime.time.hour,
                Test_Data.timeStamp.value.dateTime.time.hour, NULL);
            zassert_equal(
                Data.timeStamp.value.dateTime.time.min,
                Test_Data.timeStamp.value.dateTime.time.min, NULL);
            zassert_equal(
                Data.timeStamp.value.dateTime.time.sec,
                Test_Data.timeStamp.value.dateTime.time.sec, NULL);
            zassert_equal(
                Data.timeStamp.value.dateTime.time.hundredths,
                Test_Data.timeStamp.value.dateTime.time.hundredths, NULL);

            zassert_equal(
                Data.timeStamp.value.dateTime.date.day,
                Test_Data.timeStamp.value.dateTime.date.day, NULL);
            zassert_equal(
                Data.timeStamp.value.dateTime.date.month,
                Test_Data.timeStamp.value.dateTime.date.month, NULL);
            zassert_equal(
                Data.timeStamp.value.dateTime.date.wday,
                Test_Data.timeStamp.value.dateTime.date.wday, NULL);
            zassert_equal(
                Data.timeStamp.value.dateTime.date.year,
                Test_Data.timeStamp.value.dateTime.date.year, NULL);
            break;

        case TIME_STAMP_TIME:
            zassert_equal(
                Data.timeStamp.value.time.hour,
                Test_Data.timeStamp.value.time.hour, NULL);
            zassert_equal(
                Data.timeStamp.value.time.min,
                Test_Data.timeStamp.value.time.min, NULL);
            zassert_equal(
                Data.timeStamp.value.time.sec,
                Test_Data.timeStamp.value.time.sec, NULL);
            zassert_equal(
                Data.timeStamp.value.time.hundredths,
                Test_Data.timeStamp.value.time.hundredths, NULL);
            break;

        default:
            zassert_true(false, "Unknown type");
            break;
    }
}

static void testEventBufferReady(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    Data.eventType = EVENT_BUFFER_READY;
    Data.notificationParams.bufferReady.previousNotification = 1234;
    Data.notificationParams.bufferReady.currentNotification = 2345;
    Data.notificationParams.bufferReady.bufferProperty.deviceIdentifier.type =
        OBJECT_DEVICE;
    Data.notificationParams.bufferReady.bufferProperty.deviceIdentifier
        .instance = 500;
    Data.notificationParams.bufferReady.bufferProperty.objectIdentifier.type =
        OBJECT_ANALOG_INPUT;
    Data.notificationParams.bufferReady.bufferProperty.objectIdentifier
        .instance = 100;
    Data.notificationParams.bufferReady.bufferProperty.propertyIdentifier =
        PROP_PRESENT_VALUE;
    Data.notificationParams.bufferReady.bufferProperty.arrayIndex = 0;

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_equal(
        Data.notificationParams.bufferReady.previousNotification,
        Test_Data.notificationParams.bufferReady.previousNotification, NULL);

    zassert_equal(
        Data.notificationParams.bufferReady.currentNotification,
        Test_Data.notificationParams.bufferReady.currentNotification, NULL);

    zassert_equal(
        Data.notificationParams.bufferReady.bufferProperty.deviceIdentifier
            .type,
        Test_Data.notificationParams.bufferReady.bufferProperty.deviceIdentifier
            .type,
        NULL);

    zassert_equal(
        Data.notificationParams.bufferReady.bufferProperty.deviceIdentifier
            .instance,
        Test_Data.notificationParams.bufferReady.bufferProperty.deviceIdentifier
            .instance,
        NULL);

    zassert_equal(
        Data.notificationParams.bufferReady.bufferProperty.objectIdentifier
            .instance,
        Test_Data.notificationParams.bufferReady.bufferProperty.objectIdentifier
            .instance,
        NULL);

    zassert_equal(
        Data.notificationParams.bufferReady.bufferProperty.objectIdentifier
            .type,
        Test_Data.notificationParams.bufferReady.bufferProperty.objectIdentifier
            .type,
        NULL);

    zassert_equal(
        Data.notificationParams.bufferReady.bufferProperty.propertyIdentifier,
        Test_Data.notificationParams.bufferReady.bufferProperty
            .propertyIdentifier,
        NULL);

    zassert_equal(
        Data.notificationParams.bufferReady.bufferProperty.arrayIndex,
        Test_Data.notificationParams.bufferReady.bufferProperty.arrayIndex,
        NULL);
}

static void testEventAccessEvent(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    // OPTIONAL authenticationFactor omitted
    Data.eventType = EVENT_ACCESS_EVENT;
    Data.notificationParams.accessEvent.accessEvent =
        ACCESS_EVENT_LOCKED_BY_HIGHER_AUTHORITY;
    Data.notificationParams.accessEvent.accessEventTag = 7;
    Data.notificationParams.accessEvent.accessEventTime.tag =
        TIME_STAMP_SEQUENCE;
    Data.notificationParams.accessEvent.accessEventTime.value.sequenceNum = 17;
    Data.notificationParams.accessEvent.accessCredential.deviceIdentifier
        .instance = 1234;
    Data.notificationParams.accessEvent.accessCredential.deviceIdentifier.type =
        OBJECT_DEVICE;
    Data.notificationParams.accessEvent.accessCredential.objectIdentifier
        .instance = 17;
    Data.notificationParams.accessEvent.accessCredential.objectIdentifier.type =
        OBJECT_ACCESS_POINT;
    Data.notificationParams.accessEvent.authenticationFactor.format_type =
        AUTHENTICATION_FACTOR_MAX; // omit authenticationFactor

    bitstring_init(&Data.notificationParams.accessEvent.statusFlags);
    bitstring_set_bit(
        &Data.notificationParams.accessEvent.statusFlags, STATUS_FLAG_IN_ALARM,
        true);
    bitstring_set_bit(
        &Data.notificationParams.accessEvent.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &Data.notificationParams.accessEvent.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.accessEvent.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_equal(
        Data.notificationParams.accessEvent.accessEvent,
        Test_Data.notificationParams.accessEvent.accessEvent, NULL);

    zassert_true(
        bitstring_same(
            &Data.notificationParams.accessEvent.statusFlags,
            &Test_Data.notificationParams.accessEvent.statusFlags),
        NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.accessEventTag,
        Test_Data.notificationParams.accessEvent.accessEventTag, NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.accessEventTime.tag,
        Test_Data.notificationParams.accessEvent.accessEventTime.tag, NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.accessEventTime.value.sequenceNum,
        Test_Data.notificationParams.accessEvent.accessEventTime.value
            .sequenceNum,
        NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.accessCredential.deviceIdentifier
            .instance,
        Test_Data.notificationParams.accessEvent.accessCredential
            .deviceIdentifier.instance,
        NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.accessCredential.deviceIdentifier
            .type,
        Test_Data.notificationParams.accessEvent.accessCredential
            .deviceIdentifier.type,
        NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.accessCredential.objectIdentifier
            .instance,
        Test_Data.notificationParams.accessEvent.accessCredential
            .objectIdentifier.instance,
        NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.accessCredential.objectIdentifier
            .type,
        Test_Data.notificationParams.accessEvent.accessCredential
            .objectIdentifier.type,
        NULL);

    // OPTIONAL authenticationFactor included
    Data.eventType = EVENT_ACCESS_EVENT;
    Data.notificationParams.accessEvent.accessEvent =
        ACCESS_EVENT_LOCKED_BY_HIGHER_AUTHORITY;
    Data.notificationParams.accessEvent.accessEventTag = 7;
    Data.notificationParams.accessEvent.accessEventTime.tag =
        TIME_STAMP_SEQUENCE;
    Data.notificationParams.accessEvent.accessEventTime.value.sequenceNum = 17;
    Data.notificationParams.accessEvent.accessCredential.deviceIdentifier
        .instance = 1234;
    Data.notificationParams.accessEvent.accessCredential.deviceIdentifier.type =
        OBJECT_DEVICE;
    Data.notificationParams.accessEvent.accessCredential.objectIdentifier
        .instance = 17;
    Data.notificationParams.accessEvent.accessCredential.objectIdentifier.type =
        OBJECT_ACCESS_POINT;
    Data.notificationParams.accessEvent.authenticationFactor.format_type =
        AUTHENTICATION_FACTOR_SIMPLE_NUMBER16;
    Data.notificationParams.accessEvent.authenticationFactor.format_class = 215;
    uint8_t octetstringValue[2] = { 0x00, 0x10 };

    octetstring_init(
        &Data.notificationParams.accessEvent.authenticationFactor.value,
        octetstringValue, 2);

    bitstring_init(&Data.notificationParams.accessEvent.statusFlags);
    bitstring_set_bit(
        &Data.notificationParams.accessEvent.statusFlags, STATUS_FLAG_IN_ALARM,
        true);
    bitstring_set_bit(
        &Data.notificationParams.accessEvent.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &Data.notificationParams.accessEvent.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.accessEvent.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_equal(
        Data.notificationParams.accessEvent.accessEvent,
        Test_Data.notificationParams.accessEvent.accessEvent, NULL);

    zassert_true(
        bitstring_same(
            &Data.notificationParams.accessEvent.statusFlags,
            &Test_Data.notificationParams.accessEvent.statusFlags),
        NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.accessEventTag,
        Test_Data.notificationParams.accessEvent.accessEventTag, NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.accessEventTime.tag,
        Test_Data.notificationParams.accessEvent.accessEventTime.tag, NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.accessEventTime.value.sequenceNum,
        Test_Data.notificationParams.accessEvent.accessEventTime.value
            .sequenceNum,
        NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.accessCredential.deviceIdentifier
            .instance,
        Test_Data.notificationParams.accessEvent.accessCredential
            .deviceIdentifier.instance,
        NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.accessCredential.deviceIdentifier
            .type,
        Test_Data.notificationParams.accessEvent.accessCredential
            .deviceIdentifier.type,
        NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.accessCredential.objectIdentifier
            .instance,
        Test_Data.notificationParams.accessEvent.accessCredential
            .objectIdentifier.instance,
        NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.accessCredential.objectIdentifier
            .type,
        Test_Data.notificationParams.accessEvent.accessCredential
            .objectIdentifier.type,
        NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.authenticationFactor.format_type,
        Test_Data.notificationParams.accessEvent.authenticationFactor
            .format_type,
        NULL);

    zassert_equal(
        Data.notificationParams.accessEvent.authenticationFactor.format_class,
        Test_Data.notificationParams.accessEvent.authenticationFactor
            .format_class,
        NULL);

    zassert_true(
        octetstring_value_same(
            &Data.notificationParams.accessEvent.authenticationFactor.value,
            &Test_Data.notificationParams.accessEvent.authenticationFactor
                 .value),
        NULL);
}

static void testEventDoubleOutOfRange(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    Data.eventType = EVENT_DOUBLE_OUT_OF_RANGE;
    Data.notificationParams.doubleOutOfRange.exceedingValue = 3.45;
    Data.notificationParams.doubleOutOfRange.deadband = 2.34;
    Data.notificationParams.doubleOutOfRange.exceededLimit = 1.23;
    bitstring_init(&Data.notificationParams.doubleOutOfRange.statusFlags);
    bitstring_set_bit(
        &Data.notificationParams.doubleOutOfRange.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.doubleOutOfRange.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(
        &Data.notificationParams.doubleOutOfRange.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.doubleOutOfRange.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);
    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();
    zassert_false(
        islessgreater(
            Data.notificationParams.doubleOutOfRange.deadband,
            Test_Data.notificationParams.doubleOutOfRange.deadband),
        NULL);
    zassert_false(
        islessgreater(
            Data.notificationParams.doubleOutOfRange.exceededLimit,
            Test_Data.notificationParams.doubleOutOfRange.exceededLimit),
        NULL);
    zassert_false(
        islessgreater(
            Data.notificationParams.doubleOutOfRange.exceedingValue,
            Test_Data.notificationParams.doubleOutOfRange.exceedingValue),
        NULL);
    zassert_true(
        bitstring_same(
            &Data.notificationParams.doubleOutOfRange.statusFlags,
            &Test_Data.notificationParams.doubleOutOfRange.statusFlags),
        NULL);
}

static void testEventSignedOutOfRange(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    Data.eventType = EVENT_SIGNED_OUT_OF_RANGE;
    Data.notificationParams.signedOutOfRange.exceedingValue = -345;
    Data.notificationParams.signedOutOfRange.deadband = 234;
    Data.notificationParams.signedOutOfRange.exceededLimit = -123;
    bitstring_init(&Data.notificationParams.signedOutOfRange.statusFlags);
    bitstring_set_bit(
        &Data.notificationParams.signedOutOfRange.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.signedOutOfRange.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(
        &Data.notificationParams.signedOutOfRange.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.signedOutOfRange.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);
    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();
    zassert_equal(
        Data.notificationParams.signedOutOfRange.deadband,
        Test_Data.notificationParams.signedOutOfRange.deadband, NULL);
    zassert_equal(
        Data.notificationParams.signedOutOfRange.exceededLimit,
        Test_Data.notificationParams.signedOutOfRange.exceededLimit, NULL);
    zassert_equal(
        Data.notificationParams.signedOutOfRange.exceedingValue,
        Test_Data.notificationParams.signedOutOfRange.exceedingValue, NULL);
    zassert_true(
        bitstring_same(
            &Data.notificationParams.signedOutOfRange.statusFlags,
            &Test_Data.notificationParams.signedOutOfRange.statusFlags),
        NULL);
}

static void testEventProprietary(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    Data.eventType = EVENT_PROPRIETARY_MIN;
    Data.notificationParams.complexEventType.values[0].propertyIdentifier =
        PROP_PRESENT_VALUE;
    Data.notificationParams.complexEventType.values[0].priority = 1;
    Data.notificationParams.complexEventType.values[0].propertyArrayIndex =
        BACNET_ARRAY_ALL;
    Data.notificationParams.complexEventType.values[0].value.tag =
        BACNET_APPLICATION_TAG_REAL;
    Data.notificationParams.complexEventType.values[0].value.type.Real = 1.0f;
    Data.notificationParams.complexEventType.values[0].value.context_specific =
        false;
    Data.notificationParams.complexEventType.values[0].value.context_tag = 0;
    Data.notificationParams.complexEventType.values[0].value.next = NULL;
    Data.notificationParams.complexEventType.values[0].next = NULL;
    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();
    zassert_true(
        bacapp_same_value(
            &Data.notificationParams.complexEventType.values[0].value,
            &Test_Data.notificationParams.complexEventType.values[0].value),
        NULL);
}

static void testEventUnsignedOutOfRange(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    Data.eventType = EVENT_UNSIGNED_OUT_OF_RANGE;
    Data.notificationParams.unsignedOutOfRange.exceedingValue = 345;
    Data.notificationParams.unsignedOutOfRange.deadband = 234;
    Data.notificationParams.unsignedOutOfRange.exceededLimit = 123;
    bitstring_init(&Data.notificationParams.unsignedOutOfRange.statusFlags);
    bitstring_set_bit(
        &Data.notificationParams.unsignedOutOfRange.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.unsignedOutOfRange.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(
        &Data.notificationParams.unsignedOutOfRange.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.unsignedOutOfRange.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);
    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();
    zassert_equal(
        Data.notificationParams.unsignedOutOfRange.deadband,
        Test_Data.notificationParams.unsignedOutOfRange.deadband, NULL);
    zassert_equal(
        Data.notificationParams.unsignedOutOfRange.exceededLimit,
        Test_Data.notificationParams.unsignedOutOfRange.exceededLimit, NULL);
    zassert_equal(
        Data.notificationParams.unsignedOutOfRange.exceedingValue,
        Test_Data.notificationParams.unsignedOutOfRange.exceedingValue, NULL);
    zassert_true(
        bitstring_same(
            &Data.notificationParams.unsignedOutOfRange.statusFlags,
            &Test_Data.notificationParams.unsignedOutOfRange.statusFlags),
        NULL);
}

static void testEventChangeOfCharacterstring(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;
    BACNET_CHARACTER_STRING changed_value = { 0 };
    BACNET_CHARACTER_STRING alarm_value = { 0 };

    Data.eventType = EVENT_CHANGE_OF_CHARACTERSTRING;
    Data.notificationParams.changeOfCharacterstring.changedValue =
        &changed_value;
    Data.notificationParams.changeOfCharacterstring.alarmValue = &alarm_value;
    bitstring_init(
        &Data.notificationParams.changeOfCharacterstring.statusFlags);
    bitstring_set_bit(
        &Data.notificationParams.changeOfCharacterstring.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.changeOfCharacterstring.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfCharacterstring.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfCharacterstring.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);
    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();
    zassert_true(
        characterstring_same(
            Data.notificationParams.changeOfCharacterstring.changedValue,
            Test_Data.notificationParams.changeOfCharacterstring.changedValue),
        NULL);
    zassert_true(
        bitstring_same(
            &Data.notificationParams.changeOfCharacterstring.statusFlags,
            &Test_Data.notificationParams.changeOfCharacterstring.statusFlags),
        NULL);
    zassert_true(
        characterstring_same(
            Data.notificationParams.changeOfCharacterstring.alarmValue,
            Test_Data.notificationParams.changeOfCharacterstring.alarmValue),
        NULL);
}

static void testEventChangeOfStatusFlags(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;
    BACNET_OCTET_STRING extended_ostring = { 0 };
    BACNET_CHARACTER_STRING extended_cstring = { 0 };
    BACNET_BIT_STRING extended_bstring = { 0 };
    BACNET_PROPERTY_VALUE extended_pvalue = {
        .next = NULL,
        .priority = 1,
        .propertyArrayIndex = BACNET_ARRAY_ALL,
        .propertyIdentifier = PROP_PRESENT_VALUE,
        .value = { .context_specific = false,
                   .context_tag = 0,
                   .next = NULL,
                   .tag = BACNET_APPLICATION_TAG_REAL,
                   .type.Real = 1.0f }
    };
    unsigned i;
    BACNET_EVENT_EXTENDED_PARAMETER present_value[] = {
        { .tag = BACNET_APPLICATION_TAG_EMPTYLIST, .type.Unsigned_Int = 0 },
        { .tag = BACNET_APPLICATION_TAG_NULL, .type.Unsigned_Int = 0 },
        { .tag = BACNET_APPLICATION_TAG_BOOLEAN, .type.Boolean = true },
        { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
          .type.Unsigned_Int = 1234 },
        { .tag = BACNET_APPLICATION_TAG_SIGNED_INT, .type.Signed_Int = -1234 },
        { .tag = BACNET_APPLICATION_TAG_REAL, .type.Real = 1.0f },
        { .tag = BACNET_APPLICATION_TAG_DOUBLE, .type.Double = 1.0 },
        { .tag = BACNET_APPLICATION_TAG_OCTET_STRING,
          .type.Octet_String = &extended_ostring },
        { .tag = BACNET_APPLICATION_TAG_CHARACTER_STRING,
          .type.Character_String = &extended_cstring },
        { .tag = BACNET_APPLICATION_TAG_BIT_STRING,
          .type.Bit_String = &extended_bstring },
        { .tag = BACNET_APPLICATION_TAG_ENUMERATED, .type.Enumerated = 1 },
        { .tag = BACNET_APPLICATION_TAG_DATE,
          .type.Date = { .day = 1, .month = 1, .year = 1945 } },
        { .tag = BACNET_APPLICATION_TAG_TIME,
          .type.Time = { .hour = 1, .min = 2, .sec = 3, .hundredths = 4 } },
        { .tag = BACNET_APPLICATION_TAG_OBJECT_ID,
          .type.Object_Id = { .instance = 100, .type = OBJECT_ANALOG_INPUT } },
        { .tag = BACNET_APPLICATION_TAG_PROPERTY_VALUE,
          .type.Property_Value = &extended_pvalue },
        { .tag = MAX_BACNET_APPLICATION_TAG, .type.Unsigned_Int = 0 },
    };

    Data.eventType = EVENT_CHANGE_OF_STATUS_FLAGS;
    bitstring_init(
        &Data.notificationParams.changeOfStatusFlags.referencedFlags);
    bitstring_set_bit(
        &Data.notificationParams.changeOfStatusFlags.referencedFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.changeOfStatusFlags.referencedFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfStatusFlags.referencedFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfStatusFlags.referencedFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);
    i = 0;
    while (present_value[i].tag != MAX_BACNET_APPLICATION_TAG) {
        memcpy(
            &Data.notificationParams.changeOfStatusFlags.presentValue,
            &present_value[i], sizeof(present_value[i]));
        memset(apdu, 0, MAX_APDU);
        null_len = event_notify_encode_service_request(NULL, &Data);
        apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
        zassert_equal(
            apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
        memset(&Test_Data, 0, sizeof(Test_Data));
        test_len =
            event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
        zassert_equal(
            apdu_len, test_len, "tag=%s apdu_len=%d test_len=%d",
            bactext_application_tag_name(
                Data.notificationParams.changeOfStatusFlags.presentValue.tag),
            apdu_len, test_len);
        verifyBaseEventState();
        zassert_true(
            bitstring_same(
                &Data.notificationParams.changeOfStatusFlags.referencedFlags,
                &Test_Data.notificationParams.changeOfStatusFlags
                     .referencedFlags),
            NULL);
        i++;
    }
}

static void testEventchangeOfReliability(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int apdu_len, test_len, null_len;
    BACNET_PROPERTY_VALUE property_values = {
        .priority = 1,
        .propertyArrayIndex = BACNET_ARRAY_ALL,
        .propertyIdentifier = PROP_PRESENT_VALUE,
        .value = { .context_specific = false,
                   .context_tag = 0,
                   .next = NULL,
                   .tag = BACNET_APPLICATION_TAG_REAL,
                   .type.Real = 1.0f },
        .next = NULL
    };
    BACNET_PROPERTY_VALUE test_property_values = { 0 };

    Data.eventType = EVENT_CHANGE_OF_RELIABILITY;
    Data.notificationParams.changeOfReliability.reliability =
        RELIABILITY_NO_FAULT_DETECTED;
    Data.notificationParams.changeOfReliability.propertyValues =
        &property_values;

    bitstring_init(&Data.notificationParams.changeOfReliability.statusFlags);
    bitstring_set_bit(
        &Data.notificationParams.changeOfReliability.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.changeOfReliability.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfReliability.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfReliability.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    Test_Data.notificationParams.changeOfReliability.propertyValues =
        &test_property_values;
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();
    zassert_equal(
        Data.notificationParams.changeOfReliability.reliability,
        Test_Data.notificationParams.changeOfReliability.reliability, NULL);
    zassert_true(
        bitstring_same(
            &Data.notificationParams.changeOfReliability.statusFlags,
            &Test_Data.notificationParams.changeOfReliability.statusFlags),
        NULL);
    /* property_values vs test_property_values*/
    zassert_equal(
        property_values.propertyIdentifier,
        test_property_values.propertyIdentifier, "property=%u test_property=%u",
        (unsigned)property_values.propertyIdentifier,
        (unsigned)test_property_values.propertyIdentifier);
    zassert_equal(
        property_values.propertyArrayIndex,
        test_property_values.propertyArrayIndex, NULL);
    zassert_equal(
        property_values.priority, test_property_values.priority,
        "priority=%u test_priority=%u", (unsigned)property_values.priority,
        (unsigned)test_property_values.priority);
    zassert_true(
        bacapp_same_value(&property_values.value, &test_property_values.value),
        NULL);
    zassert_equal(property_values.next, test_property_values.next, NULL);
}

static void testEventNone(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    Data.eventType = EVENT_NONE;
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();
}

static void testEventChangeOfState(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    Data.eventType = EVENT_CHANGE_OF_STATE;
    Data.notificationParams.changeOfState.newState.tag = PROP_STATE_UNITS;
    Data.notificationParams.changeOfState.newState.state.units =
        UNITS_SQUARE_METERS;

    bitstring_init(&Data.notificationParams.changeOfState.statusFlags);
    bitstring_set_bit(
        &Data.notificationParams.changeOfState.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.changeOfState.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfState.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfState.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();

    zassert_equal(
        Data.notificationParams.changeOfState.newState.tag,
        Test_Data.notificationParams.changeOfState.newState.tag, NULL);
    zassert_equal(
        Data.notificationParams.changeOfState.newState.state.units,
        Test_Data.notificationParams.changeOfState.newState.state.units, NULL);

    zassert_true(
        bitstring_same(
            &Data.notificationParams.changeOfState.statusFlags,
            &Test_Data.notificationParams.changeOfState.statusFlags),
        NULL);

    /*
     ** Same, but timestamp of
     */
    Data.timeStamp.tag = TIME_STAMP_DATETIME;
    Data.timeStamp.value.dateTime.time.hour = 1;
    Data.timeStamp.value.dateTime.time.min = 2;
    Data.timeStamp.value.dateTime.time.sec = 3;
    Data.timeStamp.value.dateTime.time.hundredths = 4;

    Data.timeStamp.value.dateTime.date.day = 1;
    Data.timeStamp.value.dateTime.date.month = 1;
    Data.timeStamp.value.dateTime.date.wday = 1;
    Data.timeStamp.value.dateTime.date.year = 1945;

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();
    zassert_equal(
        Data.notificationParams.changeOfState.newState.tag,
        Test_Data.notificationParams.changeOfState.newState.tag, NULL);
    zassert_equal(
        Data.notificationParams.changeOfState.newState.state.units,
        Test_Data.notificationParams.changeOfState.newState.state.units, NULL);
}

static void testEventChangeOfBitstring(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    Data.timeStamp.value.sequenceNum = 1234;
    Data.timeStamp.tag = TIME_STAMP_SEQUENCE;

    Data.eventType = EVENT_CHANGE_OF_BITSTRING;

    bitstring_init(
        &Data.notificationParams.changeOfBitstring.referencedBitString);
    bitstring_set_bit(
        &Data.notificationParams.changeOfBitstring.referencedBitString, 0,
        true);
    bitstring_set_bit(
        &Data.notificationParams.changeOfBitstring.referencedBitString, 1,
        false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfBitstring.referencedBitString, 2,
        true);
    bitstring_set_bit(
        &Data.notificationParams.changeOfBitstring.referencedBitString, 2,
        false);

    bitstring_init(&Data.notificationParams.changeOfBitstring.statusFlags);

    bitstring_set_bit(
        &Data.notificationParams.changeOfBitstring.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.changeOfBitstring.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfBitstring.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfBitstring.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_true(
        bitstring_same(
            &Data.notificationParams.changeOfBitstring.referencedBitString,
            &Test_Data.notificationParams.changeOfBitstring
                 .referencedBitString),
        NULL);

    zassert_true(
        bitstring_same(
            &Data.notificationParams.changeOfBitstring.statusFlags,
            &Test_Data.notificationParams.changeOfBitstring.statusFlags),
        NULL);
}

static void testEventChangeOfValue(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    Data.eventType = EVENT_CHANGE_OF_VALUE;
    Data.notificationParams.changeOfValue.tag = CHANGE_OF_VALUE_REAL;
    Data.notificationParams.changeOfValue.newValue.changeValue = 1.23f;

    bitstring_init(&Data.notificationParams.changeOfValue.statusFlags);

    bitstring_set_bit(
        &Data.notificationParams.changeOfValue.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.changeOfValue.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfValue.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfValue.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_true(
        bitstring_same(
            &Data.notificationParams.changeOfValue.statusFlags,
            &Test_Data.notificationParams.changeOfValue.statusFlags),
        NULL);

    zassert_equal(
        Data.notificationParams.changeOfValue.tag,
        Test_Data.notificationParams.changeOfValue.tag, NULL);

    zassert_false(
        islessgreater(
            Data.notificationParams.changeOfValue.newValue.changeValue,
            Test_Data.notificationParams.changeOfValue.newValue.changeValue),
        NULL);

    /*
     ** Event Type = EVENT_CHANGE_OF_VALUE - bitstring value
     */

    Data.notificationParams.changeOfValue.tag = CHANGE_OF_VALUE_BITS;
    bitstring_init(&Data.notificationParams.changeOfValue.newValue.changedBits);
    bitstring_set_bit(
        &Data.notificationParams.changeOfValue.newValue.changedBits, 0, true);
    bitstring_set_bit(
        &Data.notificationParams.changeOfValue.newValue.changedBits, 1, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfValue.newValue.changedBits, 2, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfValue.newValue.changedBits, 3, false);
    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
    zassert_equal(apdu_len, test_len, NULL);

    verifyBaseEventState();
    zassert_true(
        bitstring_same(
            &Data.notificationParams.changeOfValue.statusFlags,
            &Test_Data.notificationParams.changeOfValue.statusFlags),
        NULL);
    zassert_equal(
        Data.notificationParams.changeOfValue.tag,
        Test_Data.notificationParams.changeOfValue.tag, NULL);
    zassert_true(
        bitstring_same(
            &Data.notificationParams.changeOfValue.newValue.changedBits,
            &Test_Data.notificationParams.changeOfValue.newValue.changedBits),
        NULL);
}

static void testEventCommandFailure(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    /*
     ** commandValue = enumerated
     */
    Data.eventType = EVENT_COMMAND_FAILURE;
    Data.notificationParams.commandFailure.tag = COMMAND_FAILURE_BINARY_PV;
    Data.notificationParams.commandFailure.commandValue.binaryValue =
        BINARY_INACTIVE;
    Data.notificationParams.commandFailure.feedbackValue.binaryValue =
        BINARY_ACTIVE;
    bitstring_init(&Data.notificationParams.commandFailure.statusFlags);
    bitstring_set_bit(
        &Data.notificationParams.commandFailure.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.commandFailure.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &Data.notificationParams.commandFailure.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.commandFailure.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);
    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);

    verifyBaseEventState();

    zassert_equal(
        Data.notificationParams.commandFailure.commandValue.binaryValue,
        Test_Data.notificationParams.commandFailure.commandValue.binaryValue,
        NULL);

    zassert_equal(
        Data.notificationParams.commandFailure.feedbackValue.binaryValue,
        Test_Data.notificationParams.commandFailure.feedbackValue.binaryValue,
        NULL);

    zassert_true(
        bitstring_same(
            &Data.notificationParams.commandFailure.statusFlags,
            &Test_Data.notificationParams.commandFailure.statusFlags),
        NULL);

    /*
     ** commandValue = unsigned
     */
    Data.eventType = EVENT_COMMAND_FAILURE;
    Data.notificationParams.commandFailure.tag = COMMAND_FAILURE_UNSIGNED;
    Data.notificationParams.commandFailure.commandValue.unsignedValue = 10;
    Data.notificationParams.commandFailure.feedbackValue.unsignedValue = 2;

    bitstring_init(&Data.notificationParams.commandFailure.statusFlags);

    bitstring_set_bit(
        &Data.notificationParams.commandFailure.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.commandFailure.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &Data.notificationParams.commandFailure.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.commandFailure.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_equal(
        Data.notificationParams.commandFailure.commandValue.unsignedValue,
        Test_Data.notificationParams.commandFailure.commandValue.unsignedValue,
        NULL);

    zassert_equal(
        Data.notificationParams.commandFailure.feedbackValue.unsignedValue,
        Test_Data.notificationParams.commandFailure.feedbackValue.unsignedValue,
        NULL);

    zassert_true(
        bitstring_same(
            &Data.notificationParams.commandFailure.statusFlags,
            &Test_Data.notificationParams.commandFailure.statusFlags),
        NULL);
}

static void testEventFloatingLimit(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    Data.eventType = EVENT_FLOATING_LIMIT;
    Data.notificationParams.floatingLimit.referenceValue = 1.23f;
    Data.notificationParams.floatingLimit.setPointValue = 2.34f;
    Data.notificationParams.floatingLimit.errorLimit = 3.45f;

    bitstring_init(&Data.notificationParams.floatingLimit.statusFlags);

    bitstring_set_bit(
        &Data.notificationParams.floatingLimit.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.floatingLimit.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &Data.notificationParams.floatingLimit.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.floatingLimit.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();

    zassert_false(
        islessgreater(
            Data.notificationParams.floatingLimit.referenceValue,
            Test_Data.notificationParams.floatingLimit.referenceValue),
        NULL);
    zassert_false(
        islessgreater(
            Data.notificationParams.floatingLimit.setPointValue,
            Test_Data.notificationParams.floatingLimit.setPointValue),
        NULL);
    zassert_false(
        islessgreater(
            Data.notificationParams.floatingLimit.errorLimit,
            Test_Data.notificationParams.floatingLimit.errorLimit),
        NULL);
    zassert_true(
        bitstring_same(
            &Data.notificationParams.floatingLimit.statusFlags,
            &Test_Data.notificationParams.floatingLimit.statusFlags),
        NULL);
}

static void testEventOutOfRange(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    Data.eventType = EVENT_OUT_OF_RANGE;
    Data.notificationParams.outOfRange.exceedingValue = 3.45f;
    Data.notificationParams.outOfRange.deadband = 2.34f;
    Data.notificationParams.outOfRange.exceededLimit = 1.23f;

    bitstring_init(&Data.notificationParams.outOfRange.statusFlags);

    bitstring_set_bit(
        &Data.notificationParams.outOfRange.statusFlags, STATUS_FLAG_IN_ALARM,
        true);
    bitstring_set_bit(
        &Data.notificationParams.outOfRange.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &Data.notificationParams.outOfRange.statusFlags, STATUS_FLAG_OVERRIDDEN,
        false);
    bitstring_set_bit(
        &Data.notificationParams.outOfRange.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_false(
        islessgreater(
            Data.notificationParams.outOfRange.deadband,
            Test_Data.notificationParams.outOfRange.deadband),
        NULL);

    zassert_false(
        islessgreater(
            Data.notificationParams.outOfRange.exceededLimit,
            Test_Data.notificationParams.outOfRange.exceededLimit),
        NULL);

    zassert_false(
        islessgreater(
            Data.notificationParams.outOfRange.exceedingValue,
            Test_Data.notificationParams.outOfRange.exceedingValue),
        NULL);
    zassert_true(
        bitstring_same(
            &Data.notificationParams.outOfRange.statusFlags,
            &Test_Data.notificationParams.outOfRange.statusFlags),
        NULL);
}

static void testEventChangeOfLifeSafety(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    Data.eventType = EVENT_CHANGE_OF_LIFE_SAFETY;
    Data.notificationParams.changeOfLifeSafety.newState =
        LIFE_SAFETY_STATE_ALARM;
    Data.notificationParams.changeOfLifeSafety.newMode = LIFE_SAFETY_MODE_ARMED;
    Data.notificationParams.changeOfLifeSafety.operationExpected =
        LIFE_SAFETY_OP_RESET;

    bitstring_init(&Data.notificationParams.changeOfLifeSafety.statusFlags);

    bitstring_set_bit(
        &Data.notificationParams.changeOfLifeSafety.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.changeOfLifeSafety.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfLifeSafety.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfLifeSafety.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_equal(
        Data.notificationParams.changeOfLifeSafety.newMode,
        Test_Data.notificationParams.changeOfLifeSafety.newMode, NULL);

    zassert_equal(
        Data.notificationParams.changeOfLifeSafety.newState,
        Test_Data.notificationParams.changeOfLifeSafety.newState, NULL);

    zassert_equal(
        Data.notificationParams.changeOfLifeSafety.operationExpected,
        Test_Data.notificationParams.changeOfLifeSafety.operationExpected,
        NULL);

    zassert_true(
        bitstring_same(
            &Data.notificationParams.changeOfLifeSafety.statusFlags,
            &Test_Data.notificationParams.changeOfLifeSafety.statusFlags),
        NULL);
}

static void testEventUnsignedRange(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    Data.eventType = EVENT_UNSIGNED_RANGE;
    Data.notificationParams.unsignedRange.exceedingValue = 1234;
    Data.notificationParams.unsignedRange.exceededLimit = 2345;

    bitstring_init(&Data.notificationParams.unsignedRange.statusFlags);

    bitstring_set_bit(
        &Data.notificationParams.unsignedRange.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.unsignedRange.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &Data.notificationParams.unsignedRange.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.unsignedRange.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_equal(
        Data.notificationParams.unsignedRange.exceedingValue,
        Test_Data.notificationParams.unsignedRange.exceedingValue, NULL);

    zassert_equal(
        Data.notificationParams.unsignedRange.exceededLimit,
        Test_Data.notificationParams.unsignedRange.exceededLimit, NULL);

    zassert_true(
        bitstring_same(
            &Data.notificationParams.unsignedRange.statusFlags,
            &Test_Data.notificationParams.unsignedRange.statusFlags),
        NULL);
}

static void testEventExtended(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;
    BACNET_OCTET_STRING extended_ostring = { 0 };
    BACNET_CHARACTER_STRING extended_cstring = { 0 };
    BACNET_BIT_STRING extended_bstring = { 0 };
    BACNET_PROPERTY_VALUE extended_pvalue = {
        .next = NULL,
        .priority = 1,
        .propertyArrayIndex = BACNET_ARRAY_ALL,
        .propertyIdentifier = PROP_PRESENT_VALUE,
        .value = { .context_specific = false,
                   .context_tag = 0,
                   .next = NULL,
                   .tag = BACNET_APPLICATION_TAG_REAL,
                   .type.Real = 1.0f }
    };
    unsigned i;
    BACNET_EVENT_EXTENDED_PARAMETER extended_parameters[] = {
        { .tag = BACNET_APPLICATION_TAG_NULL, .type.Unsigned_Int = 0 },
        { .tag = BACNET_APPLICATION_TAG_BOOLEAN, .type.Boolean = true },
        { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
          .type.Unsigned_Int = 1234 },
        { .tag = BACNET_APPLICATION_TAG_SIGNED_INT, .type.Signed_Int = -1234 },
        { .tag = BACNET_APPLICATION_TAG_REAL, .type.Real = 1.0f },
        { .tag = BACNET_APPLICATION_TAG_DOUBLE, .type.Double = 1.0 },
        { .tag = BACNET_APPLICATION_TAG_OCTET_STRING,
          .type.Octet_String = &extended_ostring },
        { .tag = BACNET_APPLICATION_TAG_CHARACTER_STRING,
          .type.Character_String = &extended_cstring },
        { .tag = BACNET_APPLICATION_TAG_BIT_STRING,
          .type.Bit_String = &extended_bstring },
        { .tag = BACNET_APPLICATION_TAG_ENUMERATED, .type.Enumerated = 1 },
        { .tag = BACNET_APPLICATION_TAG_DATE,
          .type.Date = { .day = 1, .month = 1, .year = 1945 } },
        { .tag = BACNET_APPLICATION_TAG_TIME,
          .type.Time = { .hour = 1, .min = 2, .sec = 3, .hundredths = 4 } },
        { .tag = BACNET_APPLICATION_TAG_OBJECT_ID,
          .type.Object_Id = { .instance = 100, .type = OBJECT_ANALOG_INPUT } },
        { .tag = BACNET_APPLICATION_TAG_PROPERTY_VALUE,
          .type.Property_Value = &extended_pvalue },
        { .tag = MAX_BACNET_APPLICATION_TAG, .type.Unsigned_Int = 0 },
    };

    Data.eventType = EVENT_EXTENDED;
    Data.notificationParams.extended.vendorID = 1234;
    Data.notificationParams.extended.extendedEventType = 4321;
    i = 0;
    while (extended_parameters[i].tag != MAX_BACNET_APPLICATION_TAG) {
        memcpy(
            &Data.notificationParams.extended.parameters,
            &extended_parameters[i], sizeof(extended_parameters[i]));
        memset(apdu, 0, MAX_APDU);
        null_len = event_notify_encode_service_request(NULL, &Data);
        apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
        zassert_equal(
            apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
        memset(&Test_Data, 0, sizeof(Test_Data));
        test_len =
            event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
        zassert_equal(
            apdu_len, test_len, "tag=%s apdu_len=%d test_len=%d",
            bactext_application_tag_name(
                Data.notificationParams.extended.parameters.tag),
            apdu_len, test_len);
        verifyBaseEventState();
        zassert_equal(
            Data.notificationParams.extended.vendorID,
            Test_Data.notificationParams.extended.vendorID, NULL);
        zassert_equal(
            Data.notificationParams.extended.extendedEventType,
            Test_Data.notificationParams.extended.extendedEventType, NULL);
        zassert_equal(
            Data.notificationParams.extended.parameters.tag,
            Test_Data.notificationParams.extended.parameters.tag, NULL);
        i++;
    }
}

static void testEventChangeOfDiscreteValue(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;
    BACNET_OCTET_STRING extended_ostring = { 0 };
    BACNET_CHARACTER_STRING extended_cstring = { 0 };
    unsigned i;
    BACNET_EVENT_DISCRETE_VALUE discrete_values[] = {
        { .tag = BACNET_APPLICATION_TAG_BOOLEAN, .type.Boolean = true },
        { .tag = BACNET_APPLICATION_TAG_UNSIGNED_INT,
          .type.Unsigned_Int = 1234 },
        { .tag = BACNET_APPLICATION_TAG_SIGNED_INT, .type.Signed_Int = -1234 },
        { .tag = BACNET_APPLICATION_TAG_OCTET_STRING,
          .type.Octet_String = &extended_ostring },
        { .tag = BACNET_APPLICATION_TAG_CHARACTER_STRING,
          .type.Character_String = &extended_cstring },
        { .tag = BACNET_APPLICATION_TAG_ENUMERATED, .type.Enumerated = 1 },
        { .tag = BACNET_APPLICATION_TAG_DATE,
          .type.Date = { .day = 1, .month = 1, .year = 1945 } },
        { .tag = BACNET_APPLICATION_TAG_TIME,
          .type.Time = { .hour = 1, .min = 2, .sec = 3, .hundredths = 4 } },
        { .tag = BACNET_APPLICATION_TAG_OBJECT_ID,
          .type.Object_Id = { .instance = 100, .type = OBJECT_ANALOG_INPUT } },
        { .tag = BACNET_APPLICATION_TAG_DATETIME,
          .type.Date_Time = { { .day = 1, .month = 1, .year = 2025 },
                              { .hour = 1,
                                .min = 1,
                                .sec = 1,
                                .hundredths = 1 } } },
        { .tag = MAX_BACNET_APPLICATION_TAG, .type.Unsigned_Int = 0 },
    };

    Data.eventType = EVENT_CHANGE_OF_DISCRETE_VALUE;
    bitstring_init(&Data.notificationParams.changeOfDiscreteValue.statusFlags);
    bitstring_set_bit(
        &Data.notificationParams.changeOfDiscreteValue.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.changeOfDiscreteValue.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfDiscreteValue.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfDiscreteValue.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);
    i = 0;
    while (discrete_values[i].tag != MAX_BACNET_APPLICATION_TAG) {
        memcpy(
            &Data.notificationParams.changeOfDiscreteValue.newValue,
            &discrete_values[i], sizeof(discrete_values[i]));
        memset(apdu, 0, MAX_APDU);
        null_len = event_notify_encode_service_request(NULL, &Data);
        apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
        zassert_equal(
            apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
        memset(&Test_Data, 0, sizeof(Test_Data));
        test_len =
            event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
        zassert_equal(
            apdu_len, test_len, "tag[%u]=%s apdu_len=%d test_len=%d",
            (unsigned)
                Data.notificationParams.changeOfDiscreteValue.newValue.tag,
            bactext_application_tag_name(
                Data.notificationParams.changeOfDiscreteValue.newValue.tag),
            apdu_len, test_len);
        verifyBaseEventState();
        zassert_equal(
            Data.notificationParams.changeOfDiscreteValue.newValue.tag,
            Test_Data.notificationParams.changeOfDiscreteValue.newValue.tag,
            NULL);
        i++;
    }
    zassert_true(
        bitstring_same(
            &Data.notificationParams.changeOfDiscreteValue.statusFlags,
            &Test_Data.notificationParams.changeOfDiscreteValue.statusFlags),
        NULL);
}

static void testEventChangeOfTimer(void)
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;

    Data.eventType = EVENT_CHANGE_OF_TIMER;
    Data.notificationParams.changeOfTimer.newState = TIMER_STATE_IDLE;
    Data.notificationParams.changeOfTimer.initialTimeout = 1500;
    datetime_init_ascii(
        &Data.notificationParams.changeOfTimer.expirationTime,
        "2025/12/31-23:59:59.99");
    datetime_init_ascii(
        &Data.notificationParams.changeOfTimer.updateTime,
        "2025/11/04-16:42:01.01");
    Data.notificationParams.changeOfTimer.lastStateChange =
        TIMER_TRANSITION_NONE;
    bitstring_init(&Data.notificationParams.changeOfTimer.statusFlags);
    bitstring_set_bit(
        &Data.notificationParams.changeOfTimer.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &Data.notificationParams.changeOfTimer.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfTimer.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &Data.notificationParams.changeOfTimer.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);
    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &Data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &Data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&Test_Data, 0, sizeof(Test_Data));
    test_len =
        event_notify_decode_service_request(&apdu[0], apdu_len, &Test_Data);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();
    zassert_equal(
        Data.notificationParams.changeOfTimer.newState,
        Test_Data.notificationParams.changeOfTimer.newState, NULL);
    zassert_equal(
        Data.notificationParams.changeOfTimer.initialTimeout,
        Test_Data.notificationParams.changeOfTimer.initialTimeout, NULL);
    zassert_equal(
        Data.notificationParams.changeOfTimer.lastStateChange,
        Test_Data.notificationParams.changeOfTimer.lastStateChange, NULL);
    zassert_equal(
        datetime_compare(
            &Data.notificationParams.changeOfTimer.expirationTime,
            &Test_Data.notificationParams.changeOfTimer.expirationTime),
        0, NULL);
    zassert_equal(
        datetime_compare(
            &Data.notificationParams.changeOfTimer.updateTime,
            &Test_Data.notificationParams.changeOfTimer.updateTime),
        0, NULL);
    zassert_true(
        bitstring_same(
            &Data.notificationParams.changeOfTimer.statusFlags,
            &Test_Data.notificationParams.changeOfTimer.statusFlags),
        NULL);
}

/**
 * @brief Test BACnet event handlers
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(event_tests, testEventNotification)
#else
static void testEventNotification(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;
    uint8_t invoke_id = 2;

    /* common to all the notification types */
    characterstring_init_ansi(
        &Message_Text, "This is a test of the message text\n");
    Data.messageText = &Message_Text;
    Test_Data.messageText = &Test_Message_Text;
    Data.processIdentifier = 1234;
    Data.initiatingObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    Data.initiatingObjectIdentifier.instance = 100;
    Data.eventObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    Data.eventObjectIdentifier.instance = 200;
    Data.timeStamp.value.sequenceNum = 1234;
    Data.timeStamp.tag = TIME_STAMP_SEQUENCE;
    Data.notificationClass = 50;
    Data.priority = 50;
    Data.notifyType = NOTIFY_ALARM;
    Data.fromState = EVENT_STATE_NORMAL;
    Data.toState = EVENT_STATE_OFFNORMAL;
    /*
     ** Event Type = EVENT_CHANGE_OF_BITSTRING
     */
    testEventChangeOfBitstring();
    /*
     ** Event Type = EVENT_CHANGE_OF_STATE
     */
    testEventChangeOfState();
    /*
     ** Event Type = EVENT_CHANGE_OF_VALUE
     */
    testEventChangeOfValue();
    /*
     ** Event Type = EVENT_COMMAND_FAILURE
     */
    testEventCommandFailure();
    /*
     ** Event Type = EVENT_FLOATING_LIMIT
     */
    testEventFloatingLimit();
    /*
     ** Event Type = EVENT_OUT_OF_RANGE
     */
    testEventOutOfRange();
    /*
     ** Event Type = EVENT_CHANGE_OF_LIFE_SAFETY
     */
    testEventChangeOfLifeSafety();
    /*
     ** Event Type = EVENT_EXTENDED
     */
    testEventExtended();
    /*
     ** Event Type = EVENT_BUFFER_READY
     */
    testEventBufferReady();
    /*
     ** Event Type = EVENT_UNSIGNED_RANGE
     */
    testEventUnsignedRange();
    /*
     ** Event Type = EVENT_ACCESS_EVENT
     */
    testEventAccessEvent();
    /*
     ** Event Type = EVENT_DOUBLE_OUT_OF_RANGE
     */
    testEventDoubleOutOfRange();
    /*
     ** Event Type = EVENT_SIGNED_OUT_OF_RANGE
     */
    testEventSignedOutOfRange();
    /*
     ** Event Type = EVENT_UNSIGNED_OUT_OF_RANGE
     */
    testEventUnsignedOutOfRange();
    /*
    ** EVENT_CHANGE_OF_CHARACTERSTRING
    */
    testEventChangeOfCharacterstring();
    /*
    ** EVENT_CHANGE_OF_STATUS_FLAGS
    */
    testEventChangeOfStatusFlags();
    /*
    ** EVENT_CHANGE_OF_RELIABILITY
    */
    testEventchangeOfReliability();
    /*
    ** EVENT_NONE
    */
    testEventNone();
    /*
     ** Event Type = EVENT_CHANGE_OF_DISCRETE_VALUE
     */
    testEventChangeOfDiscreteValue();
    /*
     ** Event Type = EVENT_CHANGE_OF_TIMER
     */
    testEventChangeOfTimer();
    /*
     ** Event Type = EVENT_PROPRIETARY_MIN
     */
    testEventProprietary();
    /* function coverage: Confirmed and Unconfirmed Event Notifications */
    null_len = cevent_notify_encode_apdu(NULL, invoke_id, &Data);
    apdu_len = cevent_notify_encode_apdu(apdu, invoke_id, &Data);
    zassert_true(apdu_len > 0, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_equal(apdu[0], PDU_TYPE_CONFIRMED_SERVICE_REQUEST, NULL);
    zassert_equal(apdu[2], invoke_id, NULL);
    zassert_equal(apdu[3], SERVICE_CONFIRMED_EVENT_NOTIFICATION, NULL);
    null_len = uevent_notify_encode_apdu(NULL, &Data);
    apdu_len = uevent_notify_encode_apdu(apdu, &Data);
    zassert_true(apdu_len > 0, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_equal(apdu[0], PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST, NULL);
    zassert_equal(apdu[1], SERVICE_UNCONFIRMED_EVENT_NOTIFICATION, NULL);

    null_len =
        event_notification_service_request_encode(NULL, sizeof(apdu), &Data);
    apdu_len =
        event_notification_service_request_encode(apdu, sizeof(apdu), &Data);
    zassert_true(apdu_len > 0, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    while (apdu_len > 0) {
        apdu_len--;
        test_len =
            event_notification_service_request_encode(apdu, apdu_len, &Data);
        zassert_equal(test_len, 0, NULL);
    }
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(event_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(event_tests, ztest_unit_test(testEventNotification));

    ztest_run_test_suite(event_tests);
}
#endif
