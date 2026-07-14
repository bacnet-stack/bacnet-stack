/**
 * @file
 * @brief Unit test for BACnetActionCommand codec.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/ztest.h>
#include "bacnet/bacaction.h"
#include "bacnet/bacdcode.h"

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetAction_Tests, test_BACnetActionPropertyValue)
#else
static void test_BACnetActionPropertyValue(void)
#endif
{
    BACNET_ACTION_PROPERTY_VALUE in = { 0 };
    BACNET_ACTION_PROPERTY_VALUE out = { 0 };
    uint8_t octets[] = { 0x01, 0x02, 0x03 };
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;

    in.tag = BACNET_APPLICATION_TAG_NULL;
    len = bacnet_action_property_value_encode(apdu, &in);
    zassert_true(len > 0, NULL);
    zassert_equal(
        bacnet_action_property_value_decode(apdu, (uint32_t)len, &out), len,
        NULL);
    zassert_true(bacnet_action_property_value_same(&in, &out), NULL);

    in.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    in.type.Boolean = true;
    len = bacnet_action_property_value_encode(apdu, &in);
    zassert_true(len > 0, NULL);
    zassert_equal(
        bacnet_action_property_value_decode(apdu, (uint32_t)len, &out), len,
        NULL);
    zassert_true(bacnet_action_property_value_same(&in, &out), NULL);

    in.tag = BACNET_APPLICATION_TAG_OCTET_STRING;
    octetstring_init(&in.type.Octet_String, octets, sizeof(octets));
    len = bacnet_action_property_value_encode(apdu, &in);
    zassert_true(len > 0, NULL);
    zassert_equal(
        bacnet_action_property_value_decode(apdu, (uint32_t)len, &out), len,
        NULL);
    zassert_true(bacnet_action_property_value_same(&in, &out), NULL);

    in.tag = BACNET_APPLICATION_TAG_CHARACTER_STRING;
    characterstring_init_ansi(&in.type.Character_String, "action-value");
    len = bacnet_action_property_value_encode(apdu, &in);
    zassert_true(len > 0, NULL);
    zassert_equal(
        bacnet_action_property_value_decode(apdu, (uint32_t)len, &out), len,
        NULL);
    zassert_true(bacnet_action_property_value_same(&in, &out), NULL);

    in.tag = BACNET_APPLICATION_TAG_BIT_STRING;
    bitstring_init(&in.type.Bit_String);
    bitstring_set_bit(&in.type.Bit_String, 2, true);
    bitstring_set_bit(&in.type.Bit_String, 7, true);
    len = bacnet_action_property_value_encode(apdu, &in);
    zassert_true(len > 0, NULL);
    zassert_equal(
        bacnet_action_property_value_decode(apdu, (uint32_t)len, &out), len,
        NULL);
    zassert_true(bacnet_action_property_value_same(&in, &out), NULL);

    in.tag = BACNET_APPLICATION_TAG_DATE;
    in.type.Date.year = 2026;
    in.type.Date.month = 7;
    in.type.Date.day = 14;
    in.type.Date.wday = 2;
    len = bacnet_action_property_value_encode(apdu, &in);
    zassert_true(len > 0, NULL);
    zassert_equal(
        bacnet_action_property_value_decode(apdu, (uint32_t)len, &out), len,
        NULL);
    zassert_true(bacnet_action_property_value_same(&in, &out), NULL);

    in.tag = BACNET_APPLICATION_TAG_TIME;
    in.type.Time.hour = 12;
    in.type.Time.min = 34;
    in.type.Time.sec = 56;
    in.type.Time.hundredths = 78;
    len = bacnet_action_property_value_encode(apdu, &in);
    zassert_true(len > 0, NULL);
    zassert_equal(
        bacnet_action_property_value_decode(apdu, (uint32_t)len, &out), len,
        NULL);
    zassert_true(bacnet_action_property_value_same(&in, &out), NULL);

    in.tag = BACNET_APPLICATION_TAG_OBJECT_ID;
    in.type.Object_Id.type = OBJECT_ANALOG_VALUE;
    in.type.Object_Id.instance = 1234;
    len = bacnet_action_property_value_encode(apdu, &in);
    zassert_true(len > 0, NULL);
    zassert_equal(
        bacnet_action_property_value_decode(apdu, (uint32_t)len, &out), len,
        NULL);
    zassert_true(bacnet_action_property_value_same(&in, &out), NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(BACnetAction_Tests, test_BACnetActionCommandConstructedValue)
#else
static void test_BACnetActionCommandConstructedValue(void)
#endif
{
    BACNET_ACTION_LIST in = { 0 };
    BACNET_ACTION_LIST out = { 0 };
    BACNET_CHARACTER_STRING cs = { 0 };
    uint8_t payload[MAX_APDU] = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    int payload_len = 0;
    int apdu_len = 0;
    int len = 0;

    characterstring_init_ansi(&cs, "constructed-action-payload");
    payload_len = encode_application_character_string(payload, &cs);
    zassert_true(payload_len > 0, NULL);

    in.Device_Id.type = OBJECT_DEVICE;
    in.Device_Id.instance = 100;
    in.Object_Id.type = OBJECT_ANALOG_VALUE;
    in.Object_Id.instance = 200;
    in.Property_Identifier = PROP_PRESENT_VALUE;
    in.Property_Array_Index = BACNET_ARRAY_ALL;
    in.Priority = BACNET_MIN_PRIORITY;
    in.Post_Delay = 10;
    in.Quit_On_Failure = true;
    in.Write_Successful = false;
    in.Property_Value.data_len = (uint16_t)payload_len;
    memcpy(in.Property_Value.data, payload, (size_t)payload_len);

    apdu_len = bacnet_action_command_encode(apdu, &in);
    zassert_true(apdu_len > 0, NULL);

    len = bacnet_action_command_decode(apdu, (size_t)apdu_len, &out);
    zassert_equal(len, apdu_len, NULL);
    zassert_true(bacnet_action_command_same(&in, &out), NULL);

    zassert_equal(
        out.Property_Value.data_len, in.Property_Value.data_len, NULL);
    zassert_equal(
        memcmp(
            out.Property_Value.data, in.Property_Value.data,
            in.Property_Value.data_len),
        0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(BACnetAction_Tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        BACnetAction_Tests, ztest_unit_test(test_BACnetActionPropertyValue),
        ztest_unit_test(test_BACnetActionCommandConstructedValue));

    ztest_run_test_suite(BACnetAction_Tests);
}
#endif
