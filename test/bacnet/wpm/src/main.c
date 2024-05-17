/**
 * @file
 * @brief Unit test for service
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2023
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/wpm.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Decode service header for WritePropertyMultiple
 */
static int wpm_decode_apdu(uint8_t *apdu, unsigned apdu_len, uint8_t *invoke_id)
{
    int len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST) {
        return BACNET_STATUS_ERROR;
    }
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    if (invoke_id) {
        *invoke_id = apdu[2];
    }
    if (apdu[3] != SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE) {
        return BACNET_STATUS_ERROR;
    }
    len = 4;

    return len;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(wp_tests, testWritePropertyMultiple)
#else
static void testWritePropertyMultiple(void)
#endif
{
    BACNET_WRITE_ACCESS_DATA write_access_data[3] = { 0 };
    BACNET_WRITE_ACCESS_DATA test_write_access_data[3] = { 0 };
    BACNET_PROPERTY_VALUE property_value[3] = { 0 };
    BACNET_PROPERTY_VALUE test_property_value[3] = { 0 };
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    uint8_t invoke_id = 1;
    uint8_t test_invoke_id = 0;
    int apdu_len = 0;
    int len = 0;
    uint8_t apdu[480] = { 0 };
    int offset = 0;
    uint8_t tag_number = 0;
    bool status = false;

    wpm_write_access_data_link_array(write_access_data, 3);

    write_access_data[0].object_type = OBJECT_ANALOG_VALUE;
    write_access_data[0].object_instance = 1;
    write_access_data[0].listOfProperties = &property_value[0];
    bacapp_property_value_list_init(&property_value[0], 1);
    property_value[0].propertyIdentifier = PROP_PRESENT_VALUE;
    property_value[0].propertyArrayIndex = 0;
    property_value[0].value.tag = BACNET_APPLICATION_TAG_REAL;
    property_value[0].value.type.Real = 3.14159;
    property_value[0].value.next = NULL;
    property_value[0].priority = 0;

    write_access_data[1].object_type = OBJECT_ANALOG_VALUE;
    write_access_data[1].object_instance = 2;
    write_access_data[1].listOfProperties = &property_value[1];
    bacapp_property_value_list_init(&property_value[1], 1);
    property_value[1].propertyIdentifier = PROP_PRESENT_VALUE;
    property_value[1].propertyArrayIndex = 0;
    property_value[1].value.tag = BACNET_APPLICATION_TAG_REAL;
    property_value[1].value.type.Real = 1.41421;
    property_value[1].value.next = NULL;
    property_value[1].priority = 0;

    write_access_data[2].object_type = OBJECT_BINARY_VALUE;
    write_access_data[2].object_instance = 1;
    write_access_data[2].listOfProperties = &property_value[2];
    bacapp_property_value_list_init(&property_value[2], 1);
    property_value[2].propertyIdentifier = PROP_PRESENT_VALUE;
    property_value[2].propertyArrayIndex = 0;
    property_value[2].value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
    property_value[2].value.type.Enumerated = BINARY_ACTIVE;
    property_value[2].value.next = NULL;
    property_value[2].priority = 0;

    apdu_len =
        wpm_encode_apdu(apdu, sizeof(apdu), invoke_id, write_access_data);
    zassert_not_equal(apdu_len, 0, NULL);

    wpm_write_access_data_link_array(test_write_access_data, 3);
    test_write_access_data[0].listOfProperties = &property_value[0];
    bacapp_property_value_list_init(&test_property_value[0], 1);
    test_write_access_data[1].listOfProperties = &property_value[1];
    bacapp_property_value_list_init(&test_property_value[1], 1);
    test_write_access_data[1].listOfProperties = &property_value[2];
    bacapp_property_value_list_init(&test_property_value[2], 1);

    len = wpm_decode_apdu(apdu, apdu_len, &test_invoke_id);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    offset += len;
    /* decode service request */
    do {
        /* decode Object Identifier */
        len = wpm_decode_object_id(&apdu[offset], apdu_len - offset, &wp_data);
        zassert_not_equal(len, 0, NULL);
        offset += len;
        /* Opening tag 1 - List of Properties */
        status = decode_is_opening_tag_number(&apdu[offset++], 1);
        zassert_not_equal(status, false, NULL);
        do {
            /* decode a 'Property Identifier':
                (3) an optional 'Property Array Index'
                (4) a 'Property Value'
                (5) an optional 'Priority' */
            len = wpm_decode_object_property(
                &apdu[offset], apdu_len - offset, &wp_data);
            zassert_not_equal(len, 0, NULL);
            offset += len;
            printf(
                "WPM: type=%lu instance=%lu property=%lu "
                "priority=%lu index=%ld\n",
                (unsigned long)wp_data.object_type,
                (unsigned long)wp_data.object_instance,
                (unsigned long)wp_data.object_property,
                (unsigned long)wp_data.priority, (long)wp_data.array_index);
            /* Closing tag 1 - List of Properties */
            if (decode_is_closing_tag_number(&apdu[offset], 1)) {
                tag_number = 1;
                offset++;
            } else {
                /* it was not tag 1, decode next Property Identifier */
                tag_number = 0;
            }
            /* end decoding List of Properties for "that" object */
        } while (tag_number != 1);
    } while (offset < apdu_len);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(wp_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(wp_tests, ztest_unit_test(testWritePropertyMultiple));

    ztest_run_test_suite(wp_tests);
}
#endif
