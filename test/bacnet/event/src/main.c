/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/event.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

static BACNET_EVENT_NOTIFICATION_DATA data;
static BACNET_EVENT_NOTIFICATION_DATA data2;

/**
 * @brief Helper test to BACnet event handlers
 */
static void verifyBaseEventState(void)
{
    zassert_equal(data.processIdentifier, data2.processIdentifier, NULL);
    zassert_equal(
        data.initiatingObjectIdentifier.instance,
            data2.initiatingObjectIdentifier.instance, NULL);
    zassert_equal(
        data.initiatingObjectIdentifier.type,
            data2.initiatingObjectIdentifier.type, NULL);
    zassert_equal(
        data.eventObjectIdentifier.instance,
            data2.eventObjectIdentifier.instance, NULL);
    zassert_equal(
        data.eventObjectIdentifier.type, data2.eventObjectIdentifier.type, NULL);
    zassert_equal(data.notificationClass, data2.notificationClass, NULL);
    zassert_equal(data.priority, data2.priority, NULL);
    zassert_equal(data.notifyType, data2.notifyType, NULL);
    zassert_equal(data.fromState, data2.fromState, NULL);
    zassert_equal(data.toState, data2.toState, NULL);
    zassert_equal(data.toState, data2.toState, NULL);

    if (data.messageText != NULL && data2.messageText != NULL) {
        zassert_equal(data.messageText->encoding, data2.messageText->encoding, NULL);
        zassert_equal(data.messageText->length, data2.messageText->length, NULL);
        zassert_equal(
            strcmp(data.messageText->value, data2.messageText->value), 0, NULL);
    }

    zassert_equal(data.timeStamp.tag, data2.timeStamp.tag, NULL);

    switch (data.timeStamp.tag) {
        case TIME_STAMP_SEQUENCE:
            zassert_equal(
                data.timeStamp.value.sequenceNum,
                    data2.timeStamp.value.sequenceNum, NULL);
            break;

        case TIME_STAMP_DATETIME:
            zassert_equal(
                data.timeStamp.value.dateTime.time.hour,
                    data2.timeStamp.value.dateTime.time.hour, NULL);
            zassert_equal(
                data.timeStamp.value.dateTime.time.min,
                    data2.timeStamp.value.dateTime.time.min, NULL);
            zassert_equal(
                data.timeStamp.value.dateTime.time.sec,
                    data2.timeStamp.value.dateTime.time.sec, NULL);
            zassert_equal(
                data.timeStamp.value.dateTime.time.hundredths,
                    data2.timeStamp.value.dateTime.time.hundredths, NULL);

            zassert_equal(
                data.timeStamp.value.dateTime.date.day,
                    data2.timeStamp.value.dateTime.date.day, NULL);
            zassert_equal(
                data.timeStamp.value.dateTime.date.month,
                    data2.timeStamp.value.dateTime.date.month, NULL);
            zassert_equal(
                data.timeStamp.value.dateTime.date.wday,
                    data2.timeStamp.value.dateTime.date.wday, NULL);
            zassert_equal(
                data.timeStamp.value.dateTime.date.year,
                    data2.timeStamp.value.dateTime.date.year, NULL);
            break;

        case TIME_STAMP_TIME:
            zassert_equal(
                data.timeStamp.value.time.hour,
                    data2.timeStamp.value.time.hour, NULL);
            zassert_equal(
                data.timeStamp.value.time.min,
                    data2.timeStamp.value.time.min, NULL);
            zassert_equal(
                data.timeStamp.value.time.sec,
                    data2.timeStamp.value.time.sec, NULL);
            zassert_equal(
                data.timeStamp.value.time.hundredths,
                    data2.timeStamp.value.time.hundredths, NULL);
            break;

        default:
            zassert_true(false, "Unknown type");
            break;
    }
}

/**
 * @brief Test BACnet event handlers
 */
static void testEventEventState(void)
{
    uint8_t buffer[MAX_APDU];
    int inLen;
    int outLen;
    BACNET_CHARACTER_STRING messageText;
    BACNET_CHARACTER_STRING messageText2;
    characterstring_init_ansi(
        &messageText, "This is a test of the message text\n");

    data.messageText = &messageText;
    data2.messageText = &messageText2;

    data.processIdentifier = 1234;
    data.initiatingObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    data.initiatingObjectIdentifier.instance = 100;
    data.eventObjectIdentifier.type = OBJECT_ANALOG_INPUT;
    data.eventObjectIdentifier.instance = 200;
    data.timeStamp.value.sequenceNum = 1234;
    data.timeStamp.tag = TIME_STAMP_SEQUENCE;
    data.notificationClass = 50;
    data.priority = 50;
    data.notifyType = NOTIFY_ALARM;
    data.fromState = EVENT_STATE_NORMAL;
    data.toState = EVENT_STATE_OFFNORMAL;

    data.eventType = EVENT_CHANGE_OF_STATE;
    data.notificationParams.changeOfState.newState.tag = UNITS;
    data.notificationParams.changeOfState.newState.state.units =
        UNITS_SQUARE_METERS;

    bitstring_init(&data.notificationParams.changeOfState.statusFlags);
    bitstring_set_bit(&data.notificationParams.changeOfState.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(&data.notificationParams.changeOfState.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(&data.notificationParams.changeOfState.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(&data.notificationParams.changeOfState.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    inLen = event_notify_encode_service_request(&buffer[0], &data);

    outLen = event_notify_decode_service_request(&buffer[0], inLen, &data2);

    zassert_equal(inLen, outLen, NULL);
    verifyBaseEventState();

    zassert_equal(
        data.notificationParams.changeOfState.newState.tag,
            data2.notificationParams.changeOfState.newState.tag, NULL);
    zassert_equal(
        data.notificationParams.changeOfState.newState.state.units,
            data2.notificationParams.changeOfState.newState.state.units, NULL);

    zassert_true(
        bitstring_same(&data.notificationParams.changeOfState.statusFlags,
            &data2.notificationParams.changeOfState.statusFlags), NULL);

    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/

    /*
     ** Same, but timestamp of
     */
    data.timeStamp.tag = TIME_STAMP_DATETIME;
    data.timeStamp.value.dateTime.time.hour = 1;
    data.timeStamp.value.dateTime.time.min = 2;
    data.timeStamp.value.dateTime.time.sec = 3;
    data.timeStamp.value.dateTime.time.hundredths = 4;

    data.timeStamp.value.dateTime.date.day = 1;
    data.timeStamp.value.dateTime.date.month = 1;
    data.timeStamp.value.dateTime.date.wday = 1;
    data.timeStamp.value.dateTime.date.year = 1945;

    memset(buffer, 0, MAX_APDU);
    inLen = event_notify_encode_service_request(&buffer[0], &data);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    outLen = event_notify_decode_service_request(&buffer[0], inLen, &data2);

    zassert_equal(inLen, outLen, NULL);
    verifyBaseEventState();
    zassert_equal(
        data.notificationParams.changeOfState.newState.tag,
            data2.notificationParams.changeOfState.newState.tag, NULL);
    zassert_equal(
        data.notificationParams.changeOfState.newState.state.units,
            data2.notificationParams.changeOfState.newState.state.units, NULL);

    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/

    /*
     ** Event Type = EVENT_CHANGE_OF_BITSTRING
     */
    data.timeStamp.value.sequenceNum = 1234;
    data.timeStamp.tag = TIME_STAMP_SEQUENCE;

    data.eventType = EVENT_CHANGE_OF_BITSTRING;

    bitstring_init(
        &data.notificationParams.changeOfBitstring.referencedBitString);
    bitstring_set_bit(
        &data.notificationParams.changeOfBitstring.referencedBitString, 0,
        true);
    bitstring_set_bit(
        &data.notificationParams.changeOfBitstring.referencedBitString, 1,
        false);
    bitstring_set_bit(
        &data.notificationParams.changeOfBitstring.referencedBitString, 2,
        true);
    bitstring_set_bit(
        &data.notificationParams.changeOfBitstring.referencedBitString, 2,
        false);

    bitstring_init(&data.notificationParams.changeOfBitstring.statusFlags);

    bitstring_set_bit(&data.notificationParams.changeOfBitstring.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(&data.notificationParams.changeOfBitstring.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(&data.notificationParams.changeOfBitstring.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(&data.notificationParams.changeOfBitstring.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(buffer, 0, MAX_APDU);
    inLen = event_notify_encode_service_request(&buffer[0], &data);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    outLen = event_notify_decode_service_request(&buffer[0], inLen, &data2);

    zassert_equal(inLen, outLen, NULL);
    verifyBaseEventState();

    zassert_true(
        bitstring_same(
            &data.notificationParams.changeOfBitstring.referencedBitString,
            &data2.notificationParams.changeOfBitstring.referencedBitString), NULL);

    zassert_true(
        bitstring_same(&data.notificationParams.changeOfBitstring.statusFlags,
            &data2.notificationParams.changeOfBitstring.statusFlags), NULL);

    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /*
     ** Event Type = EVENT_CHANGE_OF_VALUE - float value
     */

    data.eventType = EVENT_CHANGE_OF_VALUE;
    data.notificationParams.changeOfValue.tag = CHANGE_OF_VALUE_REAL;
    data.notificationParams.changeOfValue.newValue.changeValue = 1.23f;

    bitstring_init(&data.notificationParams.changeOfValue.statusFlags);

    bitstring_set_bit(&data.notificationParams.changeOfValue.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(&data.notificationParams.changeOfValue.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(&data.notificationParams.changeOfValue.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(&data.notificationParams.changeOfValue.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(buffer, 0, MAX_APDU);
    inLen = event_notify_encode_service_request(&buffer[0], &data);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    outLen = event_notify_decode_service_request(&buffer[0], inLen, &data2);

    zassert_equal(inLen, outLen, NULL);
    verifyBaseEventState();

    zassert_true(
        bitstring_same(&data.notificationParams.changeOfValue.statusFlags,
            &data2.notificationParams.changeOfValue.statusFlags), NULL);

    zassert_equal(
        data.notificationParams.changeOfValue.tag,
            data2.notificationParams.changeOfValue.tag, NULL);

    zassert_equal(
        data.notificationParams.changeOfValue.newValue.changeValue,
            data2.notificationParams.changeOfValue.newValue.changeValue, NULL);

    /*
     ** Event Type = EVENT_CHANGE_OF_VALUE - bitstring value
     */

    data.notificationParams.changeOfValue.tag = CHANGE_OF_VALUE_BITS;

    bitstring_init(&data.notificationParams.changeOfValue.newValue.changedBits);
    bitstring_set_bit(
        &data.notificationParams.changeOfValue.newValue.changedBits, 0, true);
    bitstring_set_bit(
        &data.notificationParams.changeOfValue.newValue.changedBits, 1, false);
    bitstring_set_bit(
        &data.notificationParams.changeOfValue.newValue.changedBits, 2, false);
    bitstring_set_bit(
        &data.notificationParams.changeOfValue.newValue.changedBits, 3, false);

    memset(buffer, 0, MAX_APDU);
    inLen = event_notify_encode_service_request(&buffer[0], &data);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    outLen = event_notify_decode_service_request(&buffer[0], inLen, &data2);

    zassert_equal(inLen, outLen, NULL);
    verifyBaseEventState();

    zassert_true(
        bitstring_same(&data.notificationParams.changeOfValue.statusFlags,
            &data2.notificationParams.changeOfValue.statusFlags), NULL);

    zassert_equal(
        data.notificationParams.changeOfValue.tag,
            data2.notificationParams.changeOfValue.tag, NULL);

    zassert_true(
        bitstring_same(
            &data.notificationParams.changeOfValue.newValue.changedBits,
            &data2.notificationParams.changeOfValue.newValue.changedBits), NULL);

    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /*
     ** Event Type = EVENT_FLOATING_LIMIT
     */
    data.eventType = EVENT_FLOATING_LIMIT;
    data.notificationParams.floatingLimit.referenceValue = 1.23f;
    data.notificationParams.floatingLimit.setPointValue = 2.34f;
    data.notificationParams.floatingLimit.errorLimit = 3.45f;

    bitstring_init(&data.notificationParams.floatingLimit.statusFlags);

    bitstring_set_bit(&data.notificationParams.floatingLimit.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(&data.notificationParams.floatingLimit.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(&data.notificationParams.floatingLimit.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(&data.notificationParams.floatingLimit.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(buffer, 0, MAX_APDU);
    inLen = event_notify_encode_service_request(&buffer[0], &data);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    outLen = event_notify_decode_service_request(&buffer[0], inLen, &data2);

    zassert_equal(inLen, outLen, NULL);
    verifyBaseEventState();

    zassert_equal(
        data.notificationParams.floatingLimit.referenceValue,
            data2.notificationParams.floatingLimit.referenceValue, NULL);

    zassert_equal(
        data.notificationParams.floatingLimit.setPointValue,
            data2.notificationParams.floatingLimit.setPointValue, NULL);

    zassert_equal(
        data.notificationParams.floatingLimit.errorLimit,
            data2.notificationParams.floatingLimit.errorLimit, NULL);
    zassert_true(
        bitstring_same(&data.notificationParams.floatingLimit.statusFlags,
            &data2.notificationParams.floatingLimit.statusFlags), NULL);

    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /*
     ** Event Type = EVENT_OUT_OF_RANGE
     */
    data.eventType = EVENT_OUT_OF_RANGE;
    data.notificationParams.outOfRange.exceedingValue = 3.45f;
    data.notificationParams.outOfRange.deadband = 2.34f;
    data.notificationParams.outOfRange.exceededLimit = 1.23f;

    bitstring_init(&data.notificationParams.outOfRange.statusFlags);

    bitstring_set_bit(&data.notificationParams.outOfRange.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(&data.notificationParams.outOfRange.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(&data.notificationParams.outOfRange.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(&data.notificationParams.outOfRange.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(buffer, 0, MAX_APDU);
    inLen = event_notify_encode_service_request(&buffer[0], &data);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    outLen = event_notify_decode_service_request(&buffer[0], inLen, &data2);

    zassert_equal(inLen, outLen, NULL);
    verifyBaseEventState();

    zassert_equal(
        data.notificationParams.outOfRange.deadband,
            data2.notificationParams.outOfRange.deadband, NULL);

    zassert_equal(
        data.notificationParams.outOfRange.exceededLimit,
            data2.notificationParams.outOfRange.exceededLimit, NULL);

    zassert_equal(
        data.notificationParams.outOfRange.exceedingValue,
            data2.notificationParams.outOfRange.exceedingValue, NULL);
    zassert_true(
        bitstring_same(&data.notificationParams.outOfRange.statusFlags,
            &data2.notificationParams.outOfRange.statusFlags), NULL);

    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /*
     ** Event Type = EVENT_CHANGE_OF_LIFE_SAFETY
     */
    data.eventType = EVENT_CHANGE_OF_LIFE_SAFETY;
    data.notificationParams.changeOfLifeSafety.newState =
        LIFE_SAFETY_STATE_ALARM;
    data.notificationParams.changeOfLifeSafety.newMode = LIFE_SAFETY_MODE_ARMED;
    data.notificationParams.changeOfLifeSafety.operationExpected =
        LIFE_SAFETY_OP_RESET;

    bitstring_init(&data.notificationParams.changeOfLifeSafety.statusFlags);

    bitstring_set_bit(&data.notificationParams.changeOfLifeSafety.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(&data.notificationParams.changeOfLifeSafety.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(&data.notificationParams.changeOfLifeSafety.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(&data.notificationParams.changeOfLifeSafety.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(buffer, 0, MAX_APDU);
    inLen = event_notify_encode_service_request(&buffer[0], &data);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    outLen = event_notify_decode_service_request(&buffer[0], inLen, &data2);

    zassert_equal(inLen, outLen, NULL);
    verifyBaseEventState();

    zassert_equal(
        data.notificationParams.changeOfLifeSafety.newMode,
            data2.notificationParams.changeOfLifeSafety.newMode, NULL);

    zassert_equal(
        data.notificationParams.changeOfLifeSafety.newState,
            data2.notificationParams.changeOfLifeSafety.newState, NULL);

    zassert_equal(
        data.notificationParams.changeOfLifeSafety.operationExpected,
            data2.notificationParams.changeOfLifeSafety.operationExpected, NULL);

    zassert_true(
        bitstring_same(&data.notificationParams.changeOfLifeSafety.statusFlags,
            &data2.notificationParams.changeOfLifeSafety.statusFlags), NULL);

    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /*
     ** Event Type = EVENT_UNSIGNED_RANGE
     */
    data.eventType = EVENT_UNSIGNED_RANGE;
    data.notificationParams.unsignedRange.exceedingValue = 1234;
    data.notificationParams.unsignedRange.exceededLimit = 2345;

    bitstring_init(&data.notificationParams.unsignedRange.statusFlags);

    bitstring_set_bit(&data.notificationParams.unsignedRange.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(&data.notificationParams.unsignedRange.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(&data.notificationParams.unsignedRange.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(&data.notificationParams.unsignedRange.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(buffer, 0, MAX_APDU);
    inLen = event_notify_encode_service_request(&buffer[0], &data);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    outLen = event_notify_decode_service_request(&buffer[0], inLen, &data2);

    zassert_equal(inLen, outLen, NULL);
    verifyBaseEventState();

    zassert_equal(
        data.notificationParams.unsignedRange.exceedingValue,
            data2.notificationParams.unsignedRange.exceedingValue, NULL);

    zassert_equal(
        data.notificationParams.unsignedRange.exceededLimit,
            data2.notificationParams.unsignedRange.exceededLimit, NULL);

    zassert_true(
        bitstring_same(&data.notificationParams.unsignedRange.statusFlags,
            &data2.notificationParams.unsignedRange.statusFlags), NULL);

    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /**********************************************************************************/
    /*
     ** Event Type = EVENT_BUFFER_READY
     */
    data.eventType = EVENT_BUFFER_READY;
    data.notificationParams.bufferReady.previousNotification = 1234;
    data.notificationParams.bufferReady.currentNotification = 2345;
    data.notificationParams.bufferReady.bufferProperty.deviceIdentifier.type =
        OBJECT_DEVICE;
    data.notificationParams.bufferReady.bufferProperty.deviceIdentifier
        .instance = 500;
    data.notificationParams.bufferReady.bufferProperty.objectIdentifier.type =
        OBJECT_ANALOG_INPUT;
    data.notificationParams.bufferReady.bufferProperty.objectIdentifier
        .instance = 100;
    data.notificationParams.bufferReady.bufferProperty.propertyIdentifier =
        PROP_PRESENT_VALUE;
    data.notificationParams.bufferReady.bufferProperty.arrayIndex = 0;

    memset(buffer, 0, MAX_APDU);
    inLen = event_notify_encode_service_request(&buffer[0], &data);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    outLen = event_notify_decode_service_request(&buffer[0], inLen, &data2);

    zassert_equal(inLen, outLen, NULL);
    verifyBaseEventState();

    zassert_equal(
        data.notificationParams.bufferReady.previousNotification,
            data2.notificationParams.bufferReady.previousNotification, NULL);

    zassert_equal(
        data.notificationParams.bufferReady.currentNotification,
            data2.notificationParams.bufferReady.currentNotification, NULL);

    zassert_equal(
        data.notificationParams.bufferReady.bufferProperty.deviceIdentifier
                .type,
            data2.notificationParams.bufferReady.bufferProperty.deviceIdentifier
                .type, NULL);

    zassert_equal(
        data.notificationParams.bufferReady.bufferProperty.deviceIdentifier
                .instance,
            data2.notificationParams.bufferReady.bufferProperty.deviceIdentifier
                .instance, NULL);

    zassert_equal(
        data.notificationParams.bufferReady.bufferProperty.objectIdentifier
                .instance,
            data2.notificationParams.bufferReady.bufferProperty.objectIdentifier
                .instance, NULL);

    zassert_equal(
        data.notificationParams.bufferReady.bufferProperty.objectIdentifier
                .type,
            data2.notificationParams.bufferReady.bufferProperty.objectIdentifier
                .type, NULL);

    zassert_equal(
        data.notificationParams.bufferReady.bufferProperty.propertyIdentifier,
            data2.notificationParams.bufferReady.bufferProperty
                .propertyIdentifier, NULL);

    zassert_equal(
        data.notificationParams.bufferReady.bufferProperty.arrayIndex,
            data2.notificationParams.bufferReady.bufferProperty.arrayIndex, NULL);
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(event_tests,
     ztest_unit_test(testEventEventState)
     );

    ztest_run_test_suite(event_tests);
}
