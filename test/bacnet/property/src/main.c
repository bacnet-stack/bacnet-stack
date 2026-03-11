/**
 * @file
 * @brief Unit test for BACnet property special lists
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/property.h>
#include <bacnet/bactext.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(property_tests, testPropList)
#else
static void testPropList(void)
#endif
{
    unsigned i = 0, j = 0;
    unsigned count = 0;
    BACNET_PROPERTY_ID property = MAX_BACNET_PROPERTY_ID;
    unsigned object_id = 0, object_name = 0, object_type = 0;
    struct special_property_list_t property_list = { 0 };
    bool status = false;

    for (i = 0; i < OBJECT_PROPRIETARY_MIN; i++) {
        count = property_list_special_count((BACNET_OBJECT_TYPE)i, PROP_ALL);
        zassert_true(count >= 3, NULL);
        object_id = 0;
        object_name = 0;
        object_type = 0;
        for (j = 0; j < count; j++) {
            property = property_list_special_property(
                (BACNET_OBJECT_TYPE)i, PROP_ALL, j);
            if (property == PROP_OBJECT_TYPE) {
                object_type++;
            }
            if (property == PROP_OBJECT_IDENTIFIER) {
                object_id++;
            }
            if (property == PROP_OBJECT_NAME) {
                object_name++;
            }
        }
        zassert_equal(
            object_type, 1, "%s: duplicate object type property",
            bactext_object_type_name((BACNET_OBJECT_TYPE)i));
        zassert_equal(object_id, 1, NULL);
        zassert_equal(object_name, 1, NULL);
        /* test member function */
        property_list_special((BACNET_OBJECT_TYPE)i, &property_list);
        zassert_true(
            property_list_member(
                property_list.Required.pList, PROP_OBJECT_TYPE),
            NULL);
        zassert_true(
            property_list_member(
                property_list.Required.pList, PROP_OBJECT_IDENTIFIER),
            NULL);
        zassert_true(
            property_list_member(
                property_list.Required.pList, PROP_OBJECT_NAME),
            NULL);
        /* validate commandable object property list */
        if ((i == OBJECT_CHANNEL) ||
            (property_list_member(
                 property_list.Required.pList, PROP_PRESENT_VALUE) &&
             (property_list_member(
                  property_list.Required.pList, PROP_PRIORITY_ARRAY) ||
              property_list_member(
                  property_list.Optional.pList, PROP_PRIORITY_ARRAY)))) {
            zassert_true(
                property_list_commandable_member(
                    (BACNET_OBJECT_TYPE)i, PROP_PRESENT_VALUE),
                "Object %s present-value is not listed as commandable",
                bactext_object_type_name((BACNET_OBJECT_TYPE)i));
        } else {
            zassert_false(
                property_list_commandable_member(
                    (BACNET_OBJECT_TYPE)i, PROP_PRESENT_VALUE),
                "Object %s present-value is listed as commandable",
                bactext_object_type_name((BACNET_OBJECT_TYPE)i));
        }
    }
    /* property is a BACnetARRAY */
    for (i = 0; i < OBJECT_PROPRIETARY_MIN; i++) {
        object_type = i;
        status =
            property_list_bacnet_array_member(object_type, PROP_PRESENT_VALUE);
        if (object_type == OBJECT_GLOBAL_GROUP) {
            zassert_true(status, NULL);
        } else {
            zassert_false(status, NULL);
        }
        status =
            property_list_bacnet_array_member(object_type, PROP_PRIORITY_ARRAY);
        zassert_true(status, NULL);
    }
    count = property_list_count(property_list_bacnet_array());
    zassert_true(count > 0, NULL);

    /* property is read-only */
    zassert_true(
        property_list_read_only_member(OBJECT_AVERAGING, PROP_MINIMUM_VALUE),
        "Averaging minimum-value should be read-only");
    zassert_true(
        property_list_read_only_member(OBJECT_CALENDAR, PROP_PRESENT_VALUE),
        "Calendar present-value should be read-only");
    zassert_true(
        property_list_read_only_member(OBJECT_COMMAND, PROP_IN_PROCESS),
        "Command in-process should be read-only");
    zassert_true(
        property_list_read_only_member(OBJECT_ANALOG_INPUT, PROP_PRESENT_VALUE),
        "Analog Input present-value should be read-only");
    zassert_false(
        property_list_read_only_member(
            OBJECT_ANALOG_OUTPUT, PROP_PRESENT_VALUE),
        "Analog Output present-value should not be read-only");
    zassert_true(
        property_list_read_only_member(OBJECT_DEVICE, PROP_VENDOR_NAME),
        "Device vendor-name should be read-only");
    zassert_true(
        property_list_read_only_member(OBJECT_PROGRAM, PROP_REASON_FOR_HALT),
        "Program reason-for-halt should be read-only");
    zassert_true(
        property_list_read_only_member(OBJECT_FILE, PROP_MODIFICATION_DATE),
        "File modification-date should be read-only");
    zassert_true(
        property_list_read_only_member(
            OBJECT_PULSE_CONVERTER, PROP_COUNT_BEFORE_CHANGE),
        "Pulse Converter count-before-change should be read-only");
    zassert_true(
        property_list_read_only_member(
            OBJECT_TRENDLOG, PROP_TOTAL_RECORD_COUNT),
        "Trend Log total-record-count should be read-only");
    zassert_true(
        property_list_read_only_member(
            OBJECT_GLOBAL_GROUP, PROP_MEMBER_STATUS_FLAGS),
        "Global Group member-status-flags should be read-only");
    zassert_true(
        property_list_read_only_member(OBJECT_CHANNEL, PROP_WRITE_STATUS),
        "Channel write-status should be read-only");
    zassert_true(
        property_list_read_only_member(
            OBJECT_LIGHTING_OUTPUT, PROP_IN_PROGRESS),
        "Lighting Output in-progress should be read-only");
    zassert_true(
        property_list_read_only_member(
            OBJECT_BINARY_LIGHTING_OUTPUT, PROP_EGRESS_ACTIVE),
        "Binary Lighting Output egress-active should be read-only");
    zassert_true(
        property_list_read_only_member(
            OBJECT_NETWORK_PORT, PROP_CHANGES_PENDING),
        "Network Port changes-pending should be read-only");
    zassert_true(
        property_list_read_only_member(OBJECT_LOAD_CONTROL, PROP_PRESENT_VALUE),
        "Load Control present-value should be read-only");
    zassert_true(
        property_list_read_only_member(OBJECT_DEVICE, PROP_RELIABILITY),
        "Reliability should be read-only");
    zassert_true(
        property_list_read_only_member(
            OBJECT_DEVICE, PROP_PROPRIETARY_RANGE_MIN),
        "Proprietary properties should be read-only");
    for (i = 0; i < OBJECT_PROPRIETARY_MIN; i++) {
        object_type = i;
        status = property_list_read_only_member(object_type, PROP_OBJECT_NAME);
        zassert_false(
            status, "Object name should not be read-only for object type %d",
            object_type);
    }
    /* test property_lists_member() - checks all three lists at once */
    property_list_special(OBJECT_ANALOG_INPUT, &property_list);
    /* required property must be found */
    zassert_true(
        property_lists_member(
            property_list.Required.pList, property_list.Optional.pList,
            property_list.Proprietary.pList, PROP_PRESENT_VALUE),
        "Analog Input present-value should be in property lists");
    /* optional property must be found */
    zassert_true(
        property_lists_member(
            property_list.Required.pList, property_list.Optional.pList,
            property_list.Proprietary.pList, PROP_COV_INCREMENT),
        "Analog Input cov-increment should be in property lists");
    /* property not in any list must not be found */
    zassert_false(
        property_lists_member(
            property_list.Required.pList, property_list.Optional.pList,
            property_list.Proprietary.pList, PROP_PRIORITY_ARRAY),
        "Analog Input priority-array should not be in property lists");
    /* NULL lists must not find any property */
    zassert_false(
        property_lists_member(NULL, NULL, NULL, PROP_PRESENT_VALUE),
        "NULL lists should not find any property");
}

/**
 * @brief Test property_list_common() - common properties for all objects
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(property_tests, testPropListCommon)
#else
static void testPropListCommon(void)
#endif
{
    /* object-identifier and object-type are common (handled by this module) */
    zassert_true(
        property_list_common(PROP_OBJECT_IDENTIFIER),
        "object-identifier should be a common property");
    zassert_true(
        property_list_common(PROP_OBJECT_TYPE),
        "object-type should be a common property");
    /* other properties are not common */
    zassert_false(
        property_list_common(PROP_OBJECT_NAME),
        "object-name should not be a common property");
    zassert_false(
        property_list_common(PROP_PRESENT_VALUE),
        "present-value should not be a common property");
    zassert_false(
        property_list_common(PROP_DESCRIPTION),
        "description should not be a common property");
}

/**
 * @brief Test property_list_encode() and property_list_common_encode()
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(property_tests, testPropListEncode)
#else
static void testPropListEncode(void)
#endif
{
    /* known property lists with 8 required, 2 optional, 0 proprietary */
    static const int32_t pRequired[] = {
        PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME,  PROP_OBJECT_TYPE,
        PROP_PRESENT_VALUE,     PROP_STATUS_FLAGS, PROP_EVENT_STATE,
        PROP_OUT_OF_SERVICE,    PROP_UNITS,        -1
    };
    static const int32_t pOptional[] = { PROP_DESCRIPTION, PROP_COV_INCREMENT,
                                         -1 };
    static const int32_t pProprietary[] = { -1 };
    /* count: 8 required - 3 standard + 2 optional = 7 entries in
     * PROP_PROPERTY_LIST */
    const uint32_t expected_count = 7;
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    int len = 0;
    BACNET_UNSIGNED_INTEGER decoded_count = 0;
    BACNET_OBJECT_TYPE decoded_type = 0;
    uint32_t decoded_instance = 0;

    /* --- property_list_encode --- */
    /* NULL rpdata returns 0 */
    len = property_list_encode(NULL, pRequired, pOptional, pProprietary);
    zassert_equal(len, 0, "NULL rpdata should return 0");

    /* no application data buffer returns 0 */
    rpdata.object_property = PROP_PROPERTY_LIST;
    rpdata.array_index = BACNET_ARRAY_ALL;
    rpdata.application_data = NULL;
    rpdata.application_data_len = 0;
    len = property_list_encode(&rpdata, pRequired, pOptional, pProprietary);
    zassert_equal(len, 0, "no buffer should return 0");

    /* array_index == 0: encode count of entries */
    rpdata.application_data = apdu;
    rpdata.application_data_len = sizeof(apdu);
    rpdata.array_index = 0;
    len = property_list_encode(&rpdata, pRequired, pOptional, pProprietary);
    zassert_true(len > 0, "array_index=0 should encode the count");
    zassert_equal(
        bacnet_unsigned_application_decode(apdu, len, &decoded_count), len,
        "decoded length should match encoded length");
    zassert_equal(
        decoded_count, expected_count, "decoded count should match expected");

    /* array_index == BACNET_ARRAY_ALL: encode all non-standard entries */
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = property_list_encode(&rpdata, pRequired, pOptional, pProprietary);
    zassert_true(len > 0, "array_index=ALL should encode all entries");

    /* array_index == 1: encode first non-standard required property */
    rpdata.array_index = 1;
    len = property_list_encode(&rpdata, pRequired, pOptional, pProprietary);
    zassert_true(len > 0, "array_index=1 should encode first entry");

    /* array_index == last valid index */
    rpdata.array_index = expected_count;
    len = property_list_encode(&rpdata, pRequired, pOptional, pProprietary);
    zassert_true(len > 0, "array_index=last should encode the last entry");

    /* array_index out of range: error */
    rpdata.array_index = expected_count + 1;
    len = property_list_encode(&rpdata, pRequired, pOptional, pProprietary);
    zassert_equal(
        len, BACNET_STATUS_ERROR,
        "out-of-range array_index should return BACNET_STATUS_ERROR");

    /* non-PROP_PROPERTY_LIST property: error */
    rpdata.array_index = BACNET_ARRAY_ALL;
    rpdata.object_property = PROP_OBJECT_NAME;
    len = property_list_encode(&rpdata, pRequired, pOptional, pProprietary);
    zassert_equal(
        len, BACNET_STATUS_ERROR,
        "unknown property should return BACNET_STATUS_ERROR");

    /* --- property_list_common_encode --- */
    /* NULL rpdata returns 0 */
    len = property_list_common_encode(NULL, 0);
    zassert_equal(len, 0, "NULL rpdata should return 0");

    /* no buffer returns 0 */
    rpdata.application_data = NULL;
    rpdata.application_data_len = 0;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    len = property_list_common_encode(&rpdata, 1);
    zassert_equal(len, 0, "no buffer should return 0");

    /* PROP_OBJECT_IDENTIFIER encodes object type + instance */
    rpdata.object_type = OBJECT_ANALOG_INPUT;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.application_data = apdu;
    rpdata.application_data_len = sizeof(apdu);
    len = property_list_common_encode(&rpdata, 0);
    zassert_true(len > 0, "PROP_OBJECT_IDENTIFIER should encode successfully");
    zassert_equal(
        bacnet_object_id_application_decode(
            apdu, len, &decoded_type, &decoded_instance),
        len, "decoded length should match encoded length");
    zassert_equal(
        decoded_type, OBJECT_ANALOG_INPUT, "decoded object type should match");
    zassert_equal(
        decoded_instance, (uint32_t)1, "decoded object instance should match");

    /* PROP_OBJECT_IDENTIFIER on Device uses device_instance_number */
    rpdata.object_type = OBJECT_DEVICE;
    rpdata.object_instance = BACNET_MAX_INSTANCE;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    len = property_list_common_encode(&rpdata, 1234);
    zassert_true(len > 0, "Device PROP_OBJECT_IDENTIFIER should encode");
    zassert_equal(
        bacnet_object_id_application_decode(
            apdu, len, &decoded_type, &decoded_instance),
        len, "decoded length should match encoded length");
    zassert_equal(
        decoded_type, OBJECT_DEVICE, "decoded type should be OBJECT_DEVICE");
    zassert_equal(
        decoded_instance, (uint32_t)1234,
        "Device instance should use device_instance_number parameter");

    /* PROP_OBJECT_TYPE encodes the object type enumeration */
    rpdata.object_type = OBJECT_ANALOG_OUTPUT;
    rpdata.object_property = PROP_OBJECT_TYPE;
    len = property_list_common_encode(&rpdata, 0);
    zassert_true(len > 0, "PROP_OBJECT_TYPE should encode successfully");

    /* unknown property returns BACNET_STATUS_ERROR */
    rpdata.object_property = PROP_OBJECT_NAME;
    len = property_list_common_encode(&rpdata, 0);
    zassert_equal(
        len, BACNET_STATUS_ERROR,
        "unknown property should return BACNET_STATUS_ERROR");
}

/**
 * @brief Test property_list_bacnet_list() and
 * property_list_bacnet_list_member()
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(property_tests, testPropListBACnetList)
#else
static void testPropListBACnetList(void)
#endif
{
    const int32_t *pList = NULL;
    uint32_t count = 0;

    /* list pointer is non-NULL and has at least one entry */
    pList = property_list_bacnet_list();
    zassert_not_null(pList, "BACnetLIST should not be NULL");
    count = property_list_count(pList);
    zassert_true(count > 0, "BACnetLIST should have at least one entry");

    /* known BACnetLIST members - present in the global list */
    zassert_true(
        property_list_bacnet_list_member(OBJECT_DEVICE, PROP_DATE_LIST),
        "date-list should be a BACnetLIST member");
    zassert_true(
        property_list_bacnet_list_member(
            OBJECT_DEVICE, PROP_ACTIVE_COV_SUBSCRIPTIONS),
        "active-cov-subscriptions should be a BACnetLIST member");
    zassert_true(
        property_list_bacnet_list_member(
            OBJECT_DEVICE, PROP_DEVICE_ADDRESS_BINDING),
        "device-address-binding should be a BACnetLIST member");
    zassert_true(
        property_list_bacnet_list_member(
            OBJECT_NOTIFICATION_CLASS, PROP_RECIPIENT_LIST),
        "recipient-list should be a BACnetLIST member");
    zassert_true(
        property_list_bacnet_list_member(OBJECT_TRENDLOG, PROP_LOG_BUFFER),
        "log-buffer should be a BACnetLIST member");

    /* BACnetARRAY properties are not BACnetLIST */
    zassert_false(
        property_list_bacnet_list_member(
            OBJECT_ANALOG_OUTPUT, PROP_PRIORITY_ARRAY),
        "priority-array should not be a BACnetLIST member");
    zassert_false(
        property_list_bacnet_list_member(OBJECT_DEVICE, PROP_OBJECT_LIST),
        "object-list should not be a BACnetLIST member");

    /* standard scalar properties are not BACnetLIST */
    zassert_false(
        property_list_bacnet_list_member(
            OBJECT_ANALOG_INPUT, PROP_PRESENT_VALUE),
        "Analog Input present-value should not be a BACnetLIST member");
    zassert_false(
        property_list_bacnet_list_member(OBJECT_DEVICE, PROP_VENDOR_NAME),
        "vendor-name should not be a BACnetLIST member");

    /* object-type exceptions: Group present-value is a BACnetLIST */
    zassert_true(
        property_list_bacnet_list_member(OBJECT_GROUP, PROP_PRESENT_VALUE),
        "Group present-value should be a BACnetLIST member");

    /* object-type exceptions: Channel list-of-object-property-references
       is NOT a BACnetLIST (it is a BACnetARRAY) */
    zassert_false(
        property_list_bacnet_list_member(
            OBJECT_CHANNEL, PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES),
        "Channel list-of-object-property-references should not be a "
        "BACnetLIST member");

    /* proprietary range properties are always considered BACnetLIST */
    zassert_true(
        property_list_bacnet_list_member(
            OBJECT_ANALOG_INPUT, PROP_PROPRIETARY_RANGE_MIN),
        "proprietary properties should be considered BACnetLIST members");
    zassert_true(
        property_list_bacnet_list_member(
            OBJECT_DEVICE, PROP_PROPRIETARY_RANGE_MAX),
        "proprietary properties should be considered BACnetLIST members");
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(property_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        property_tests, ztest_unit_test(testPropList),
        ztest_unit_test(testPropListCommon),
        ztest_unit_test(testPropListEncode),
        ztest_unit_test(testPropListBACnetList));

    ztest_run_test_suite(property_tests);
}
#endif
