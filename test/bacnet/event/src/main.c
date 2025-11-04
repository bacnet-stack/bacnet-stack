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
        data.eventObjectIdentifier.type, data2.eventObjectIdentifier.type,
        NULL);
    zassert_equal(data.notificationClass, data2.notificationClass, NULL);
    zassert_equal(data.priority, data2.priority, NULL);
    zassert_equal(data.notifyType, data2.notifyType, NULL);
    zassert_equal(data.fromState, data2.fromState, NULL);
    zassert_equal(data.toState, data2.toState, NULL);
    zassert_equal(data.toState, data2.toState, NULL);

    if (data.messageText != NULL && data2.messageText != NULL) {
        zassert_equal(
            data.messageText->encoding, data2.messageText->encoding, NULL);
        zassert_equal(
            data.messageText->length, data2.messageText->length, NULL);
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
                data.timeStamp.value.time.hour, data2.timeStamp.value.time.hour,
                NULL);
            zassert_equal(
                data.timeStamp.value.time.min, data2.timeStamp.value.time.min,
                NULL);
            zassert_equal(
                data.timeStamp.value.time.sec, data2.timeStamp.value.time.sec,
                NULL);
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
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(event_tests, testEventEventState)
#else
static void testEventEventState(void)
#endif
{
    uint8_t apdu[MAX_APDU];
    int apdu_len, test_len, null_len;
    BACNET_CHARACTER_STRING messageText = { 0 };
    BACNET_CHARACTER_STRING messageText2 = { 0 };
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
    uint8_t invoke_id = 2;

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
    data.notificationParams.changeOfState.newState.tag = PROP_STATE_UNITS;
    data.notificationParams.changeOfState.newState.state.units =
        UNITS_SQUARE_METERS;

    bitstring_init(&data.notificationParams.changeOfState.statusFlags);
    bitstring_set_bit(
        &data.notificationParams.changeOfState.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &data.notificationParams.changeOfState.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &data.notificationParams.changeOfState.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &data.notificationParams.changeOfState.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();

    zassert_equal(
        data.notificationParams.changeOfState.newState.tag,
        data2.notificationParams.changeOfState.newState.tag, NULL);
    zassert_equal(
        data.notificationParams.changeOfState.newState.state.units,
        data2.notificationParams.changeOfState.newState.state.units, NULL);

    zassert_true(
        bitstring_same(
            &data.notificationParams.changeOfState.statusFlags,
            &data2.notificationParams.changeOfState.statusFlags),
        NULL);

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

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();
    zassert_equal(
        data.notificationParams.changeOfState.newState.tag,
        data2.notificationParams.changeOfState.newState.tag, NULL);
    zassert_equal(
        data.notificationParams.changeOfState.newState.state.units,
        data2.notificationParams.changeOfState.newState.state.units, NULL);
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

    bitstring_set_bit(
        &data.notificationParams.changeOfBitstring.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &data.notificationParams.changeOfBitstring.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(
        &data.notificationParams.changeOfBitstring.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &data.notificationParams.changeOfBitstring.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_true(
        bitstring_same(
            &data.notificationParams.changeOfBitstring.referencedBitString,
            &data2.notificationParams.changeOfBitstring.referencedBitString),
        NULL);

    zassert_true(
        bitstring_same(
            &data.notificationParams.changeOfBitstring.statusFlags,
            &data2.notificationParams.changeOfBitstring.statusFlags),
        NULL);

    /*
     ** Event Type = EVENT_CHANGE_OF_VALUE - float value
     */
    data.eventType = EVENT_CHANGE_OF_VALUE;
    data.notificationParams.changeOfValue.tag = CHANGE_OF_VALUE_REAL;
    data.notificationParams.changeOfValue.newValue.changeValue = 1.23f;

    bitstring_init(&data.notificationParams.changeOfValue.statusFlags);

    bitstring_set_bit(
        &data.notificationParams.changeOfValue.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &data.notificationParams.changeOfValue.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &data.notificationParams.changeOfValue.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &data.notificationParams.changeOfValue.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_true(
        bitstring_same(
            &data.notificationParams.changeOfValue.statusFlags,
            &data2.notificationParams.changeOfValue.statusFlags),
        NULL);

    zassert_equal(
        data.notificationParams.changeOfValue.tag,
        data2.notificationParams.changeOfValue.tag, NULL);

    zassert_false(
        islessgreater(
            data.notificationParams.changeOfValue.newValue.changeValue,
            data2.notificationParams.changeOfValue.newValue.changeValue),
        NULL);

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
    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);
    zassert_equal(apdu_len, test_len, NULL);

    verifyBaseEventState();
    zassert_true(
        bitstring_same(
            &data.notificationParams.changeOfValue.statusFlags,
            &data2.notificationParams.changeOfValue.statusFlags),
        NULL);
    zassert_equal(
        data.notificationParams.changeOfValue.tag,
        data2.notificationParams.changeOfValue.tag, NULL);
    zassert_true(
        bitstring_same(
            &data.notificationParams.changeOfValue.newValue.changedBits,
            &data2.notificationParams.changeOfValue.newValue.changedBits),
        NULL);

    /*
     ** Event Type = EVENT_COMMAND_FAILURE
     */

    /*
     ** commandValue = enumerated
     */
    data.eventType = EVENT_COMMAND_FAILURE;
    data.notificationParams.commandFailure.tag = COMMAND_FAILURE_BINARY_PV;
    data.notificationParams.commandFailure.commandValue.binaryValue =
        BINARY_INACTIVE;
    data.notificationParams.commandFailure.feedbackValue.binaryValue =
        BINARY_ACTIVE;
    bitstring_init(&data.notificationParams.commandFailure.statusFlags);
    bitstring_set_bit(
        &data.notificationParams.commandFailure.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &data.notificationParams.commandFailure.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &data.notificationParams.commandFailure.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &data.notificationParams.commandFailure.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);
    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);

    verifyBaseEventState();

    zassert_equal(
        data.notificationParams.commandFailure.commandValue.binaryValue,
        data2.notificationParams.commandFailure.commandValue.binaryValue, NULL);

    zassert_equal(
        data.notificationParams.commandFailure.feedbackValue.binaryValue,
        data2.notificationParams.commandFailure.feedbackValue.binaryValue,
        NULL);

    zassert_true(
        bitstring_same(
            &data.notificationParams.commandFailure.statusFlags,
            &data2.notificationParams.commandFailure.statusFlags),
        NULL);

    /*
     ** commandValue = unsigned
     */
    data.eventType = EVENT_COMMAND_FAILURE;
    data.notificationParams.commandFailure.tag = COMMAND_FAILURE_UNSIGNED;
    data.notificationParams.commandFailure.commandValue.unsignedValue = 10;
    data.notificationParams.commandFailure.feedbackValue.unsignedValue = 2;

    bitstring_init(&data.notificationParams.commandFailure.statusFlags);

    bitstring_set_bit(
        &data.notificationParams.commandFailure.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &data.notificationParams.commandFailure.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &data.notificationParams.commandFailure.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &data.notificationParams.commandFailure.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_equal(
        data.notificationParams.commandFailure.commandValue.unsignedValue,
        data2.notificationParams.commandFailure.commandValue.unsignedValue,
        NULL);

    zassert_equal(
        data.notificationParams.commandFailure.feedbackValue.unsignedValue,
        data2.notificationParams.commandFailure.feedbackValue.unsignedValue,
        NULL);

    zassert_true(
        bitstring_same(
            &data.notificationParams.commandFailure.statusFlags,
            &data2.notificationParams.commandFailure.statusFlags),
        NULL);

    /*
     ** Event Type = EVENT_FLOATING_LIMIT
     */
    data.eventType = EVENT_FLOATING_LIMIT;
    data.notificationParams.floatingLimit.referenceValue = 1.23f;
    data.notificationParams.floatingLimit.setPointValue = 2.34f;
    data.notificationParams.floatingLimit.errorLimit = 3.45f;

    bitstring_init(&data.notificationParams.floatingLimit.statusFlags);

    bitstring_set_bit(
        &data.notificationParams.floatingLimit.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &data.notificationParams.floatingLimit.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &data.notificationParams.floatingLimit.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &data.notificationParams.floatingLimit.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();

    zassert_false(
        islessgreater(
            data.notificationParams.floatingLimit.referenceValue,
            data2.notificationParams.floatingLimit.referenceValue),
        NULL);
    zassert_false(
        islessgreater(
            data.notificationParams.floatingLimit.setPointValue,
            data2.notificationParams.floatingLimit.setPointValue),
        NULL);
    zassert_false(
        islessgreater(
            data.notificationParams.floatingLimit.errorLimit,
            data2.notificationParams.floatingLimit.errorLimit),
        NULL);
    zassert_true(
        bitstring_same(
            &data.notificationParams.floatingLimit.statusFlags,
            &data2.notificationParams.floatingLimit.statusFlags),
        NULL);

    /*
     ** Event Type = EVENT_OUT_OF_RANGE
     */
    data.eventType = EVENT_OUT_OF_RANGE;
    data.notificationParams.outOfRange.exceedingValue = 3.45f;
    data.notificationParams.outOfRange.deadband = 2.34f;
    data.notificationParams.outOfRange.exceededLimit = 1.23f;

    bitstring_init(&data.notificationParams.outOfRange.statusFlags);

    bitstring_set_bit(
        &data.notificationParams.outOfRange.statusFlags, STATUS_FLAG_IN_ALARM,
        true);
    bitstring_set_bit(
        &data.notificationParams.outOfRange.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &data.notificationParams.outOfRange.statusFlags, STATUS_FLAG_OVERRIDDEN,
        false);
    bitstring_set_bit(
        &data.notificationParams.outOfRange.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_false(
        islessgreater(
            data.notificationParams.outOfRange.deadband,
            data2.notificationParams.outOfRange.deadband),
        NULL);

    zassert_false(
        islessgreater(
            data.notificationParams.outOfRange.exceededLimit,
            data2.notificationParams.outOfRange.exceededLimit),
        NULL);

    zassert_false(
        islessgreater(
            data.notificationParams.outOfRange.exceedingValue,
            data2.notificationParams.outOfRange.exceedingValue),
        NULL);
    zassert_true(
        bitstring_same(
            &data.notificationParams.outOfRange.statusFlags,
            &data2.notificationParams.outOfRange.statusFlags),
        NULL);

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

    bitstring_set_bit(
        &data.notificationParams.changeOfLifeSafety.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &data.notificationParams.changeOfLifeSafety.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(
        &data.notificationParams.changeOfLifeSafety.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &data.notificationParams.changeOfLifeSafety.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);

    zassert_equal(apdu_len, test_len, NULL);
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
        bitstring_same(
            &data.notificationParams.changeOfLifeSafety.statusFlags,
            &data2.notificationParams.changeOfLifeSafety.statusFlags),
        NULL);

    /*
     ** Event Type = EVENT_UNSIGNED_RANGE
     */
    data.eventType = EVENT_UNSIGNED_RANGE;
    data.notificationParams.unsignedRange.exceedingValue = 1234;
    data.notificationParams.unsignedRange.exceededLimit = 2345;

    bitstring_init(&data.notificationParams.unsignedRange.statusFlags);

    bitstring_set_bit(
        &data.notificationParams.unsignedRange.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &data.notificationParams.unsignedRange.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &data.notificationParams.unsignedRange.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &data.notificationParams.unsignedRange.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_equal(
        data.notificationParams.unsignedRange.exceedingValue,
        data2.notificationParams.unsignedRange.exceedingValue, NULL);

    zassert_equal(
        data.notificationParams.unsignedRange.exceededLimit,
        data2.notificationParams.unsignedRange.exceededLimit, NULL);

    zassert_true(
        bitstring_same(
            &data.notificationParams.unsignedRange.statusFlags,
            &data2.notificationParams.unsignedRange.statusFlags),
        NULL);
    /*
     ** Event Type = EVENT_EXTENDED
     */
    data.eventType = EVENT_EXTENDED;
    data.notificationParams.extended.vendorID = 1234;
    data.notificationParams.extended.extendedEventType = 4321;
    i = 0;
    while (extended_parameters[i].tag != MAX_BACNET_APPLICATION_TAG) {
        memcpy(
            &data.notificationParams.extended.parameters,
            &extended_parameters[i], sizeof(extended_parameters[i]));
        memset(apdu, 0, MAX_APDU);
        null_len = event_notify_encode_service_request(NULL, &data);
        apdu_len = event_notify_encode_service_request(&apdu[0], &data);
        zassert_equal(
            apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
        memset(&data2, 0, sizeof(data2));
        data2.messageText = &messageText2;
        test_len =
            event_notify_decode_service_request(&apdu[0], apdu_len, &data2);
        zassert_equal(
            apdu_len, test_len, "tag=%s apdu_len=%d test_len=%d",
            bactext_application_tag_name(
                data.notificationParams.extended.parameters.tag),
            apdu_len, test_len);
        verifyBaseEventState();
        zassert_equal(
            data.notificationParams.extended.vendorID,
            data2.notificationParams.extended.vendorID, NULL);
        zassert_equal(
            data.notificationParams.extended.extendedEventType,
            data2.notificationParams.extended.extendedEventType, NULL);
        zassert_equal(
            data.notificationParams.extended.parameters.tag,
            data2.notificationParams.extended.parameters.tag, NULL);
        i++;
    }
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

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);

    zassert_equal(apdu_len, test_len, NULL);
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
            .type,
        NULL);

    zassert_equal(
        data.notificationParams.bufferReady.bufferProperty.deviceIdentifier
            .instance,
        data2.notificationParams.bufferReady.bufferProperty.deviceIdentifier
            .instance,
        NULL);

    zassert_equal(
        data.notificationParams.bufferReady.bufferProperty.objectIdentifier
            .instance,
        data2.notificationParams.bufferReady.bufferProperty.objectIdentifier
            .instance,
        NULL);

    zassert_equal(
        data.notificationParams.bufferReady.bufferProperty.objectIdentifier
            .type,
        data2.notificationParams.bufferReady.bufferProperty.objectIdentifier
            .type,
        NULL);

    zassert_equal(
        data.notificationParams.bufferReady.bufferProperty.propertyIdentifier,
        data2.notificationParams.bufferReady.bufferProperty.propertyIdentifier,
        NULL);

    zassert_equal(
        data.notificationParams.bufferReady.bufferProperty.arrayIndex,
        data2.notificationParams.bufferReady.bufferProperty.arrayIndex, NULL);

    /*
     ** Event Type = EVENT_ACCESS_EVENT
     */

    // OPTIONAL authenticationFactor omitted
    data.eventType = EVENT_ACCESS_EVENT;
    data.notificationParams.accessEvent.accessEvent =
        ACCESS_EVENT_LOCKED_BY_HIGHER_AUTHORITY;
    data.notificationParams.accessEvent.accessEventTag = 7;
    data.notificationParams.accessEvent.accessEventTime.tag =
        TIME_STAMP_SEQUENCE;
    data.notificationParams.accessEvent.accessEventTime.value.sequenceNum = 17;
    data.notificationParams.accessEvent.accessCredential.deviceIdentifier
        .instance = 1234;
    data.notificationParams.accessEvent.accessCredential.deviceIdentifier.type =
        OBJECT_DEVICE;
    data.notificationParams.accessEvent.accessCredential.objectIdentifier
        .instance = 17;
    data.notificationParams.accessEvent.accessCredential.objectIdentifier.type =
        OBJECT_ACCESS_POINT;
    data.notificationParams.accessEvent.authenticationFactor.format_type =
        AUTHENTICATION_FACTOR_MAX; // omit authenticationFactor

    bitstring_init(&data.notificationParams.accessEvent.statusFlags);
    bitstring_set_bit(
        &data.notificationParams.accessEvent.statusFlags, STATUS_FLAG_IN_ALARM,
        true);
    bitstring_set_bit(
        &data.notificationParams.accessEvent.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &data.notificationParams.accessEvent.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &data.notificationParams.accessEvent.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_equal(
        data.notificationParams.accessEvent.accessEvent,
        data2.notificationParams.accessEvent.accessEvent, NULL);

    zassert_true(
        bitstring_same(
            &data.notificationParams.accessEvent.statusFlags,
            &data2.notificationParams.accessEvent.statusFlags),
        NULL);

    zassert_equal(
        data.notificationParams.accessEvent.accessEventTag,
        data2.notificationParams.accessEvent.accessEventTag, NULL);

    zassert_equal(
        data.notificationParams.accessEvent.accessEventTime.tag,
        data2.notificationParams.accessEvent.accessEventTime.tag, NULL);

    zassert_equal(
        data.notificationParams.accessEvent.accessEventTime.value.sequenceNum,
        data2.notificationParams.accessEvent.accessEventTime.value.sequenceNum,
        NULL);

    zassert_equal(
        data.notificationParams.accessEvent.accessCredential.deviceIdentifier
            .instance,
        data2.notificationParams.accessEvent.accessCredential.deviceIdentifier
            .instance,
        NULL);

    zassert_equal(
        data.notificationParams.accessEvent.accessCredential.deviceIdentifier
            .type,
        data2.notificationParams.accessEvent.accessCredential.deviceIdentifier
            .type,
        NULL);

    zassert_equal(
        data.notificationParams.accessEvent.accessCredential.objectIdentifier
            .instance,
        data2.notificationParams.accessEvent.accessCredential.objectIdentifier
            .instance,
        NULL);

    zassert_equal(
        data.notificationParams.accessEvent.accessCredential.objectIdentifier
            .type,
        data2.notificationParams.accessEvent.accessCredential.objectIdentifier
            .type,
        NULL);

    // OPTIONAL authenticationFactor included
    data.eventType = EVENT_ACCESS_EVENT;
    data.notificationParams.accessEvent.accessEvent =
        ACCESS_EVENT_LOCKED_BY_HIGHER_AUTHORITY;
    data.notificationParams.accessEvent.accessEventTag = 7;
    data.notificationParams.accessEvent.accessEventTime.tag =
        TIME_STAMP_SEQUENCE;
    data.notificationParams.accessEvent.accessEventTime.value.sequenceNum = 17;
    data.notificationParams.accessEvent.accessCredential.deviceIdentifier
        .instance = 1234;
    data.notificationParams.accessEvent.accessCredential.deviceIdentifier.type =
        OBJECT_DEVICE;
    data.notificationParams.accessEvent.accessCredential.objectIdentifier
        .instance = 17;
    data.notificationParams.accessEvent.accessCredential.objectIdentifier.type =
        OBJECT_ACCESS_POINT;
    data.notificationParams.accessEvent.authenticationFactor.format_type =
        AUTHENTICATION_FACTOR_SIMPLE_NUMBER16;
    data.notificationParams.accessEvent.authenticationFactor.format_class = 215;
    uint8_t octetstringValue[2] = { 0x00, 0x10 };

    octetstring_init(
        &data.notificationParams.accessEvent.authenticationFactor.value,
        octetstringValue, 2);

    bitstring_init(&data.notificationParams.accessEvent.statusFlags);
    bitstring_set_bit(
        &data.notificationParams.accessEvent.statusFlags, STATUS_FLAG_IN_ALARM,
        true);
    bitstring_set_bit(
        &data.notificationParams.accessEvent.statusFlags, STATUS_FLAG_FAULT,
        false);
    bitstring_set_bit(
        &data.notificationParams.accessEvent.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &data.notificationParams.accessEvent.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);

    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);

    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);

    zassert_equal(apdu_len, test_len, NULL);
    verifyBaseEventState();

    zassert_equal(
        data.notificationParams.accessEvent.accessEvent,
        data2.notificationParams.accessEvent.accessEvent, NULL);

    zassert_true(
        bitstring_same(
            &data.notificationParams.accessEvent.statusFlags,
            &data2.notificationParams.accessEvent.statusFlags),
        NULL);

    zassert_equal(
        data.notificationParams.accessEvent.accessEventTag,
        data2.notificationParams.accessEvent.accessEventTag, NULL);

    zassert_equal(
        data.notificationParams.accessEvent.accessEventTime.tag,
        data2.notificationParams.accessEvent.accessEventTime.tag, NULL);

    zassert_equal(
        data.notificationParams.accessEvent.accessEventTime.value.sequenceNum,
        data2.notificationParams.accessEvent.accessEventTime.value.sequenceNum,
        NULL);

    zassert_equal(
        data.notificationParams.accessEvent.accessCredential.deviceIdentifier
            .instance,
        data2.notificationParams.accessEvent.accessCredential.deviceIdentifier
            .instance,
        NULL);

    zassert_equal(
        data.notificationParams.accessEvent.accessCredential.deviceIdentifier
            .type,
        data2.notificationParams.accessEvent.accessCredential.deviceIdentifier
            .type,
        NULL);

    zassert_equal(
        data.notificationParams.accessEvent.accessCredential.objectIdentifier
            .instance,
        data2.notificationParams.accessEvent.accessCredential.objectIdentifier
            .instance,
        NULL);

    zassert_equal(
        data.notificationParams.accessEvent.accessCredential.objectIdentifier
            .type,
        data2.notificationParams.accessEvent.accessCredential.objectIdentifier
            .type,
        NULL);

    zassert_equal(
        data.notificationParams.accessEvent.authenticationFactor.format_type,
        data2.notificationParams.accessEvent.authenticationFactor.format_type,
        NULL);

    zassert_equal(
        data.notificationParams.accessEvent.authenticationFactor.format_class,
        data2.notificationParams.accessEvent.authenticationFactor.format_class,
        NULL);

    zassert_true(
        octetstring_value_same(
            &data.notificationParams.accessEvent.authenticationFactor.value,
            &data2.notificationParams.accessEvent.authenticationFactor.value),
        NULL);

    /*
     ** Event Type = EVENT_DOUBLE_OUT_OF_RANGE
     */
    data.eventType = EVENT_DOUBLE_OUT_OF_RANGE;
    data.notificationParams.doubleOutOfRange.exceedingValue = 3.45;
    data.notificationParams.doubleOutOfRange.deadband = 2.34;
    data.notificationParams.doubleOutOfRange.exceededLimit = 1.23;
    bitstring_init(&data.notificationParams.doubleOutOfRange.statusFlags);
    bitstring_set_bit(
        &data.notificationParams.doubleOutOfRange.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &data.notificationParams.doubleOutOfRange.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(
        &data.notificationParams.doubleOutOfRange.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &data.notificationParams.doubleOutOfRange.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);
    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();
    zassert_false(
        islessgreater(
            data.notificationParams.doubleOutOfRange.deadband,
            data2.notificationParams.doubleOutOfRange.deadband),
        NULL);
    zassert_false(
        islessgreater(
            data.notificationParams.doubleOutOfRange.exceededLimit,
            data2.notificationParams.doubleOutOfRange.exceededLimit),
        NULL);
    zassert_false(
        islessgreater(
            data.notificationParams.doubleOutOfRange.exceedingValue,
            data2.notificationParams.doubleOutOfRange.exceedingValue),
        NULL);
    zassert_true(
        bitstring_same(
            &data.notificationParams.doubleOutOfRange.statusFlags,
            &data2.notificationParams.doubleOutOfRange.statusFlags),
        NULL);
    /*
     ** Event Type = EVENT_SIGNED_OUT_OF_RANGE
     */
    data.eventType = EVENT_SIGNED_OUT_OF_RANGE;
    data.notificationParams.signedOutOfRange.exceedingValue = -345;
    data.notificationParams.signedOutOfRange.deadband = 234;
    data.notificationParams.signedOutOfRange.exceededLimit = -123;
    bitstring_init(&data.notificationParams.signedOutOfRange.statusFlags);
    bitstring_set_bit(
        &data.notificationParams.signedOutOfRange.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &data.notificationParams.signedOutOfRange.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(
        &data.notificationParams.signedOutOfRange.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &data.notificationParams.signedOutOfRange.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);
    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();
    zassert_equal(
        data.notificationParams.signedOutOfRange.deadband,
        data2.notificationParams.signedOutOfRange.deadband, NULL);
    zassert_equal(
        data.notificationParams.signedOutOfRange.exceededLimit,
        data2.notificationParams.signedOutOfRange.exceededLimit, NULL);
    zassert_equal(
        data.notificationParams.signedOutOfRange.exceedingValue,
        data2.notificationParams.signedOutOfRange.exceedingValue, NULL);
    zassert_true(
        bitstring_same(
            &data.notificationParams.signedOutOfRange.statusFlags,
            &data2.notificationParams.signedOutOfRange.statusFlags),
        NULL);
    /*
    ** EVENT_CHANGE_OF_CHARACTERSTRING
    */

    /*
     ** Event Type = EVENT_UNSIGNED_OUT_OF_RANGE
     */
    data.eventType = EVENT_UNSIGNED_OUT_OF_RANGE;
    data.notificationParams.unsignedOutOfRange.exceedingValue = 345;
    data.notificationParams.unsignedOutOfRange.deadband = 234;
    data.notificationParams.unsignedOutOfRange.exceededLimit = 123;
    bitstring_init(&data.notificationParams.unsignedOutOfRange.statusFlags);
    bitstring_set_bit(
        &data.notificationParams.unsignedOutOfRange.statusFlags,
        STATUS_FLAG_IN_ALARM, true);
    bitstring_set_bit(
        &data.notificationParams.unsignedOutOfRange.statusFlags,
        STATUS_FLAG_FAULT, false);
    bitstring_set_bit(
        &data.notificationParams.unsignedOutOfRange.statusFlags,
        STATUS_FLAG_OVERRIDDEN, false);
    bitstring_set_bit(
        &data.notificationParams.unsignedOutOfRange.statusFlags,
        STATUS_FLAG_OUT_OF_SERVICE, false);
    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();
    zassert_equal(
        data.notificationParams.unsignedOutOfRange.deadband,
        data2.notificationParams.unsignedOutOfRange.deadband, NULL);
    zassert_equal(
        data.notificationParams.unsignedOutOfRange.exceededLimit,
        data2.notificationParams.unsignedOutOfRange.exceededLimit, NULL);
    zassert_equal(
        data.notificationParams.unsignedOutOfRange.exceedingValue,
        data2.notificationParams.unsignedOutOfRange.exceedingValue, NULL);
    zassert_true(
        bitstring_same(
            &data.notificationParams.unsignedOutOfRange.statusFlags,
            &data2.notificationParams.unsignedOutOfRange.statusFlags),
        NULL);
    /*
     ** Event Type = EVENT_PROPRIETARY_MIN
     */
    data.eventType = EVENT_PROPRIETARY_MIN;
    data.notificationParams.complexEventType.values[0].propertyIdentifier =
        PROP_PRESENT_VALUE;
    data.notificationParams.complexEventType.values[0].priority = 1;
    data.notificationParams.complexEventType.values[0].propertyArrayIndex =
        BACNET_ARRAY_ALL;
    data.notificationParams.complexEventType.values[0].value.tag =
        BACNET_APPLICATION_TAG_REAL;
    data.notificationParams.complexEventType.values[0].value.type.Real = 1.0f;
    data.notificationParams.complexEventType.values[0].value.context_specific =
        false;
    data.notificationParams.complexEventType.values[0].value.context_tag = 0;
    data.notificationParams.complexEventType.values[0].value.next = NULL;
    data.notificationParams.complexEventType.values[0].next = NULL;
    memset(apdu, 0, MAX_APDU);
    null_len = event_notify_encode_service_request(NULL, &data);
    apdu_len = event_notify_encode_service_request(&apdu[0], &data);
    zassert_equal(
        apdu_len, null_len, "apdu_len=%d null_len=%d", apdu_len, null_len);
    memset(&data2, 0, sizeof(data2));
    data2.messageText = &messageText2;
    test_len = event_notify_decode_service_request(&apdu[0], apdu_len, &data2);
    zassert_equal(
        apdu_len, test_len, "apdu_len=%d test_len=%d", apdu_len, test_len);
    verifyBaseEventState();
    zassert_true(
        bacapp_same_value(
            &data.notificationParams.complexEventType.values[0].value,
            &data2.notificationParams.complexEventType.values[0].value),
        NULL);
    /* function coverage: Confirmed and Unconfirmed Event Notifications */
    null_len = cevent_notify_encode_apdu(NULL, invoke_id, &data);
    apdu_len = cevent_notify_encode_apdu(apdu, invoke_id, &data);
    zassert_true(apdu_len > 0, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_equal(apdu[0], PDU_TYPE_CONFIRMED_SERVICE_REQUEST, NULL);
    zassert_equal(apdu[2], invoke_id, NULL);
    zassert_equal(apdu[3], SERVICE_CONFIRMED_EVENT_NOTIFICATION, NULL);
    null_len = uevent_notify_encode_apdu(NULL, &data);
    apdu_len = uevent_notify_encode_apdu(apdu, &data);
    zassert_true(apdu_len > 0, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    zassert_equal(apdu[0], PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST, NULL);
    zassert_equal(apdu[1], SERVICE_UNCONFIRMED_EVENT_NOTIFICATION, NULL);

    null_len =
        event_notification_service_request_encode(NULL, sizeof(apdu), &data);
    apdu_len =
        event_notification_service_request_encode(apdu, sizeof(apdu), &data);
    zassert_true(apdu_len > 0, NULL);
    zassert_equal(apdu_len, null_len, NULL);
    while (apdu_len > 0) {
        apdu_len--;
        test_len =
            event_notification_service_request_encode(apdu, apdu_len, &data);
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
    ztest_test_suite(event_tests, ztest_unit_test(testEventEventState));

    ztest_run_test_suite(event_tests);
}
#endif
