/**
 * @file
 * @brief test BACnet Life Safety Zone object APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/bacdevobjpropref.h>
#include <bacnet/basic/object/lsz.h>
#include <property_test.h>

static int lsz_zone_members_count(
    uint32_t object_instance,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *first,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *second)
{
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    int apdu_len = 0;
    int len = 0;
    int count = 0;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE member = { 0 };

    rpdata.object_type = OBJECT_LIFE_SAFETY_ZONE;
    rpdata.object_instance = object_instance;
    rpdata.object_property = PROP_ZONE_MEMBERS;
    rpdata.array_index = BACNET_ARRAY_ALL;
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);

    apdu_len = Life_Safety_Zone_Read_Property(&rpdata);
    zassert_true(apdu_len >= 0, NULL);
    while (len < apdu_len) {
        int decoded_len = bacnet_device_object_property_reference_decode(
            &apdu[len], apdu_len - len, &member);
        zassert_true(decoded_len > 0, NULL);
        len += decoded_len;
        if ((count == 0) && first) {
            *first = member;
        } else if ((count == 1) && second) {
            *second = member;
        }
        count++;
    }
    zassert_equal(len, apdu_len, NULL);

    return count;
}

static bool lsz_zone_members_write(
    uint32_t object_instance,
    const uint8_t *payload,
    int payload_len,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    bool status = false;

    wp_data.object_type = OBJECT_LIFE_SAFETY_ZONE;
    wp_data.object_instance = object_instance;
    wp_data.object_property = PROP_ZONE_MEMBERS;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;
    wp_data.application_data_len = payload_len;
    if (payload && (payload_len > 0) &&
        (payload_len <= sizeof(wp_data.application_data))) {
        memcpy(wp_data.application_data, payload, payload_len);
    } else {
        return false;
    }
    status = Life_Safety_Zone_Write_Property(&wp_data);
    if (error_class) {
        *error_class = wp_data.error_class;
    }
    if (error_code) {
        *error_code = wp_data.error_code;
    }

    return status;
}

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(testsLifeSafetyZone, testLifeSafetyZone)
#else
static void testLifeSafetyZone(void)
#endif
{
    bool status;
    unsigned count = 0;
    uint32_t object_instance = 0, test_object_instance = 0;
    const int skip_fail_property_list[] = { -1 };
    const char *test_name = NULL;
    char *sample_name = "sample";

    Life_Safety_Zone_Init();
    object_instance = Life_Safety_Zone_Create(BACNET_MAX_INSTANCE);
    count = Life_Safety_Zone_Count();
    zassert_true(count > 0, NULL);
    test_object_instance = Life_Safety_Zone_Index_To_Instance(0);
    zassert_equal(test_object_instance, object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_LIFE_SAFETY_ZONE, object_instance,
        Life_Safety_Zone_Property_Lists, Life_Safety_Zone_Read_Property,
        Life_Safety_Zone_Write_Property, skip_fail_property_list);
    /* test the ASCII name get/set */
    status = Life_Safety_Zone_Name_Set(object_instance, sample_name);
    zassert_true(status, NULL);
    test_name = Life_Safety_Zone_Name_ASCII(object_instance);
    zassert_equal(test_name, sample_name, NULL);
    status = Life_Safety_Zone_Name_Set(object_instance, NULL);
    zassert_true(status, NULL);
    test_name = Life_Safety_Zone_Name_ASCII(object_instance);
    zassert_equal(test_name, NULL, NULL);
    /* cleanup */
    status = Life_Safety_Zone_Delete(object_instance);
}

/**
 * @brief Regression tests for zone-members WriteProperty decode handling
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(testsLifeSafetyZone, testLifeSafetyZoneZoneMembersWriteProperty)
#else
static void testLifeSafetyZoneZoneMembersWriteProperty(void)
#endif
{
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE member = { 0 };
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE member2 = { 0 };
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE test_member = { 0 };
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE test_member2 = { 0 };
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_DEVICE;
    BACNET_ERROR_CODE error_code = ERROR_CODE_OTHER;
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int len2 = 0;
    uint32_t object_instance;
    bool status;

    Life_Safety_Zone_Init();
    object_instance = Life_Safety_Zone_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    member.objectIdentifier.type = OBJECT_ANALOG_INPUT;
    member.objectIdentifier.instance = 1;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    member.deviceIdentifier.type = BACNET_NO_DEV_TYPE;
    member.deviceIdentifier.instance = BACNET_NO_DEV_ID;

    member2.objectIdentifier.type = OBJECT_BINARY_INPUT;
    member2.objectIdentifier.instance = 2;
    member2.propertyIdentifier = PROP_PRESENT_VALUE;
    member2.arrayIndex = BACNET_ARRAY_ALL;
    member2.deviceIdentifier.type = BACNET_NO_DEV_TYPE;
    member2.deviceIdentifier.instance = BACNET_NO_DEV_ID;

    len = bacapp_encode_device_obj_property_ref(&apdu[0], &member);
    zassert_true(len > 0, NULL);
    status = lsz_zone_members_write(
        object_instance, &apdu[0], len, &error_class, &error_code);
    zassert_true(status, NULL);
    zassert_equal(
        lsz_zone_members_count(object_instance, &test_member, NULL), 1, NULL);
    zassert_true(
        bacnet_device_object_property_reference_same(&test_member, &member),
        NULL);

    /* zero-length decode (first tag mismatch): opening[3], null, closing[3] */
    apdu[0] = 0x3E;
    apdu[1] = 0x00;
    apdu[2] = 0x3F;
    status = lsz_zone_members_write(
        object_instance, &apdu[0], 3, &error_class, &error_code);
    zassert_false(status, NULL);
    zassert_equal(error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(error_code, ERROR_CODE_INVALID_DATA_TYPE, NULL);
    zassert_equal(
        lsz_zone_members_count(object_instance, &test_member, NULL), 1, NULL);
    zassert_true(
        bacnet_device_object_property_reference_same(&test_member, &member),
        NULL);

    /* zero-length decode with non-empty payload and wrong first element tag */
    apdu[0] = 0x00;
    status = lsz_zone_members_write(
        object_instance, &apdu[0], 1, &error_class, &error_code);
    zassert_false(status, NULL);
    zassert_equal(error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(error_code, ERROR_CODE_INVALID_DATA_TYPE, NULL);
    zassert_equal(
        lsz_zone_members_count(object_instance, &test_member, NULL), 1, NULL);
    zassert_true(
        bacnet_device_object_property_reference_same(&test_member, &member),
        NULL);

    /* negative decode: payload starts like ref, but truncated */
    len = bacapp_encode_device_obj_property_ref(&apdu[0], &member);
    zassert_true(len > 1, NULL);
    status = lsz_zone_members_write(
        object_instance, &apdu[0], 1, &error_class, &error_code);
    zassert_false(status, NULL);
    zassert_equal(error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(error_code, ERROR_CODE_INVALID_DATA_TYPE, NULL);
    zassert_equal(
        lsz_zone_members_count(object_instance, &test_member, NULL), 1, NULL);
    zassert_true(
        bacnet_device_object_property_reference_same(&test_member, &member),
        NULL);

    /* malformed tail after valid first element must not partially commit */
    len = bacapp_encode_device_obj_property_ref(&apdu[0], &member2);
    zassert_true(len > 0, NULL);
    apdu[len] = 0x00;
    status = lsz_zone_members_write(
        object_instance, &apdu[0], len + 1, &error_class, &error_code);
    zassert_false(status, NULL);
    zassert_equal(error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(error_code, ERROR_CODE_INVALID_DATA_TYPE, NULL);
    zassert_equal(
        lsz_zone_members_count(object_instance, &test_member, NULL), 1, NULL);
    zassert_true(
        bacnet_device_object_property_reference_same(&test_member, &member),
        NULL);

    /* positive decode + multi-element list + cursor advancement */
    len = bacapp_encode_device_obj_property_ref(&apdu[0], &member);
    zassert_true(len > 0, NULL);
    len2 = bacapp_encode_device_obj_property_ref(&apdu[len], &member2);
    zassert_true(len2 > 0, NULL);
    status = lsz_zone_members_write(
        object_instance, &apdu[0], len + len2, &error_class, &error_code);
    zassert_true(status, NULL);
    zassert_equal(
        lsz_zone_members_count(object_instance, &test_member, &test_member2), 2,
        NULL);
    zassert_true(
        bacnet_device_object_property_reference_same(&test_member, &member),
        NULL);
    zassert_true(
        bacnet_device_object_property_reference_same(&test_member2, &member2),
        NULL);

    status = Life_Safety_Zone_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(testsLifeSafetyZone, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        testsLifeSafetyZone, ztest_unit_test(testLifeSafetyZone),
        ztest_unit_test(testLifeSafetyZoneZoneMembersWriteProperty));

    ztest_run_test_suite(testsLifeSafetyZone);
}
#endif
