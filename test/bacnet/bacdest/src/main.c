/**
 * @file
 * @brief Unit test for BACnetDestination encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <bacnet/bacaddr.h>
#include <bacnet/bacdest.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_destination_tests, testBACnetDestination)
#else
static void testBACnetDestination(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_DESTINATION destination = { 0 }, test_destination = { 0 };
    int apdu_len = 0, null_len = 0, test_len = 0;
    bool status = false;

    destination.Recipient.tag = BACNET_RECIPIENT_TAG_DEVICE;
    destination.Recipient.type.device.type = OBJECT_DEVICE;
    destination.Recipient.type.device.instance = 1234;
    null_len = bacnet_destination_encode(NULL, &destination);
    apdu_len = bacnet_destination_encode(apdu, &destination);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_destination_decode(apdu, apdu_len, &test_destination);
    zassert_equal(apdu_len, test_len, NULL);
    status = bacnet_destination_same(&destination, &test_destination);
    zassert_true(status, NULL);

    destination.Recipient.tag = BACNET_RECIPIENT_TAG_ADDRESS;
    destination.Recipient.type.address.net = 1234;
    destination.Recipient.type.address.len = 6;
    destination.Recipient.type.address.adr[0] = 1;
    destination.Recipient.type.address.adr[1] = 2;
    destination.Recipient.type.address.adr[2] = 3;
    destination.Recipient.type.address.adr[3] = 4;
    destination.Recipient.type.address.adr[4] = 5;
    destination.Recipient.type.address.adr[5] = 6;
    null_len = bacnet_destination_encode(NULL, &destination);
    apdu_len = bacnet_destination_encode(apdu, &destination);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_destination_decode(apdu, apdu_len, &test_destination);
    zassert_equal(test_len, apdu_len, NULL);
    status = bacnet_destination_same(&destination, &test_destination);
    zassert_true(status, NULL);

    null_len = bacnet_destination_encode(NULL, &destination);
    apdu_len = bacnet_destination_encode(apdu, &destination);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_destination_decode(apdu, apdu_len, &test_destination);
    zassert_equal(test_len, apdu_len, NULL);
    test_len = bacnet_destination_decode(apdu, apdu_len, NULL);
    zassert_equal(test_len, apdu_len, NULL);
    status = bacnet_destination_same(&destination, &test_destination);
    zassert_true(status, NULL);
    bacnet_destination_copy(&test_destination, &destination);
    status = bacnet_destination_same(&destination, &test_destination);
    zassert_true(status, NULL);

    /* decoding, some negative tests */
    test_len = bacnet_destination_decode(NULL, apdu_len, &test_destination);
    zassert_equal(test_len, BACNET_STATUS_REJECT, NULL);
    test_len = bacnet_destination_decode(apdu, 0, &test_destination);
    zassert_equal(test_len, BACNET_STATUS_REJECT, NULL);
    /* context */
    null_len = bacnet_destination_context_encode(NULL, 4, &destination);
    apdu_len = bacnet_destination_context_encode(apdu, 4, &destination);
    zassert_equal(apdu_len, null_len, NULL);
    test_len =
        bacnet_destination_context_decode(apdu, apdu_len, 4, &test_destination);
    zassert_equal(apdu_len, test_len, NULL);
    /* defaults */
    status = bacnet_destination_default(&destination);
    zassert_false(status, NULL);
}
/**
 * @}
 */

/**
 * Unit Test for ASCII conversion
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_destination_tests, test_BACnetDestination_ASCII)
#else
static void test_BACnetDestination_ASCII(void)
#endif
{
    BACNET_DESTINATION destination = { 0 }, test_destination = { 0 };
    int len = 0, test_len = 0, null_len = 0;
    const char *ascii = "("
                        "ValidDays=[1,2,3,4,5,6,7];"
                        "FromTime=0:00:00.0;ToTime=23:59:59.99;"
                        "Recipient=Device(type=8,instance=4194303);"
                        "ProcessIdentifier=0;"
                        "ConfirmedNotify=false;"
                        "Transitions=[]"
                        ")";
    bool status = false;
    char *test_ascii = NULL;

    bacnet_destination_default_init(&destination);
    len = bacnet_destination_from_ascii(&test_destination, ascii);
    zassert_true(len > 0, NULL);
    status = bacnet_destination_same(&destination, &test_destination);
    zassert_true(status, NULL);
    /* get the length without NULL termination */
    null_len = bacnet_destination_to_ascii(&test_destination, NULL, 0);
    if (null_len > 0) {
        test_ascii = calloc(null_len, sizeof(char));
        if (test_ascii) {
            test_len = bacnet_destination_to_ascii(
                &test_destination, test_ascii, null_len);
            zassert_equal(null_len, test_len, NULL);
            while (--test_len) {
                len = bacnet_destination_to_ascii(
                    &test_destination, test_ascii, test_len);
                if (test_len > 0) {
                    zassert_equal(
                        len, test_len, "%s len=%d test_len=%d", test_ascii, len,
                        test_len);
                } else {
                    zassert_equal(
                        len, null_len, "%s len=%d null_len=%d", test_ascii, len,
                        null_len);
                }
            }
            free(test_ascii);
        }
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_destination_tests, testBACnetRecipient)
#else
static void testBACnetRecipient(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_RECIPIENT value = { 0 }, test_value = { 0 };
    BACNET_MAC_ADDRESS mac = { .len = 1, .adr[0] = 0x01 };
    BACNET_MAC_ADDRESS adr = { .len = 1, .adr[0] = 0x02 };
    uint16_t snet = 1234;
    BACNET_ADDRESS address = { 0 };
    int apdu_len = 0, null_len = 0, test_len = 0;
    uint8_t tag_number = 4;
    bool status = false;

    /* device */
    bacnet_recipient_device_set(&value, OBJECT_DEVICE, 123);
    status = bacnet_recipient_device_valid(&value);
    zassert_true(status, NULL);
    bacnet_recipient_copy(&test_value, &value);
    status = bacnet_recipient_same(&value, &test_value);
    zassert_true(status, NULL);
    null_len = bacnet_recipient_encode(NULL, &value);
    apdu_len = bacnet_recipient_encode(apdu, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_recipient_decode(apdu, apdu_len, &test_value);
    zassert_equal(apdu_len, test_len, NULL);
    status = bacnet_recipient_same(&value, &test_value);
    zassert_true(status, NULL);
    /* address */
    status = bacnet_address_init(&address, &mac, snet, &adr);
    zassert_true(status, NULL);
    bacnet_recipient_address_set(&value, &address);
    bacnet_recipient_copy(&test_value, &value);
    status = bacnet_recipient_same(&value, &test_value);
    zassert_true(status, NULL);
    null_len = bacnet_recipient_encode(NULL, &value);
    apdu_len = bacnet_recipient_encode(apdu, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_recipient_decode(apdu, apdu_len, &test_value);
    zassert_equal(apdu_len, test_len, NULL);
    status = bacnet_recipient_same(&value, &test_value);
    zassert_true(status, NULL);
    /* context */
    null_len = bacnet_recipient_context_encode(NULL, tag_number, &value);
    apdu_len = bacnet_recipient_context_encode(apdu, tag_number, &value);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_recipient_context_decode(
        apdu, apdu_len, tag_number, &test_value);
    zassert_equal(apdu_len, test_len, NULL);
    status = bacnet_recipient_same(&value, &test_value);
    zassert_true(status, NULL);

    bacnet_recipient_device_wildcard_set(&value);
    status = bacnet_recipient_device_wildcard(&value);
    zassert_true(status, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_destination_tests, test_BACnetRecipient_ASCII)
#else
static void test_BACnetRecipient_ASCII(void)
#endif
{
    bool status = false;
    BACNET_RECIPIENT value = { 0 }, test_value = { 0 };
    BACNET_ADDRESS address = { 0 };
    BACNET_MAC_ADDRESS mac = { .len = 1, .adr[0] = 0x01 };
    BACNET_MAC_ADDRESS adr = { .len = 1, .adr[0] = 0x02 };
    uint16_t snet = 1234;
    int len = 0;
    char ascii[80] = "";

    bacnet_recipient_device_set(&value, OBJECT_DEVICE, 4194303);
    len = bacnet_recipient_to_ascii(&value, ascii, sizeof(ascii));
    zassert_true(len > 0, NULL);
    status = bacnet_recipient_from_ascii(&test_value, ascii);
    zassert_true(status, "ascii=%s", ascii);
    status = bacnet_recipient_same(&value, &test_value);
    zassert_true(status, NULL);

    status = bacnet_address_init(&address, &mac, snet, &adr);
    zassert_true(status, NULL);
    bacnet_recipient_address_set(&value, &address);
    len = bacnet_recipient_to_ascii(&value, ascii, sizeof(ascii));
    zassert_true(len > 0, NULL);
    status = bacnet_recipient_from_ascii(&test_value, ascii);
    zassert_true(status, "ascii=%s", ascii);
    status = bacnet_recipient_same(&value, &test_value);
    zassert_true(status, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacnet_destination_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bacnet_destination_tests, ztest_unit_test(testBACnetDestination),
        ztest_unit_test(test_BACnetDestination_ASCII),
        ztest_unit_test(testBACnetRecipient),
        ztest_unit_test(test_BACnetRecipient_ASCII));

    ztest_run_test_suite(bacnet_destination_tests);
}
#endif
