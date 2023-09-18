/**
 * @file
 * @brief Unit test for BACnetDestination encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2022
 *
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <bacnet/bacdest.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testBACnetRecipientData(
    BACNET_RECIPIENT *data1,
    BACNET_RECIPIENT *data2)
{
    unsigned i = 0;

    if (data1 && data2) {
        zassert_equal(data1->tag, data2->tag, NULL);
        if (data1->tag == BACNET_RECIPIENT_TAG_DEVICE) {
            zassert_equal(
                data1->type.device.type, data2->type.device.type, NULL);
            zassert_equal(
                data1->type.device.instance,
                    data2->type.device.instance, NULL);
        } else if (data1->tag == BACNET_RECIPIENT_TAG_ADDRESS) {
            zassert_equal(
                data1->type.address.net, data2->type.address.net, NULL);
            if (data1->type.address.net == BACNET_BROADCAST_NETWORK) {
                zassert_equal(
                    data1->type.address.mac_len,
                        data2->type.address.mac_len, NULL);
            } else if (data1->type.address.net) {
                zassert_equal(
                    data1->type.address.len,
                        data2->type.address.len, NULL);
                for (i = 0; i < data1->type.address.len; i++) {
                    zassert_equal(
                        data1->type.address.adr[i],
                            data2->type.address.adr[i], NULL);
                }
            } else {
                zassert_equal(
                    data1->type.address.mac_len,
                        data2->type.address.mac_len, NULL);
                for (i = 0; i < data1->type.address.mac_len; i++) {
                    zassert_equal(
                        data1->type.address.mac[i],
                            data2->type.address.mac[i], NULL);
                }
            }
        } else {
            zassert_true(data1->tag < BACNET_RECIPIENT_TAG_MAX, NULL);
        }
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacnet_destination_tests, testBACnetDestination)
#else
static void testBACnetDestination(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_DESTINATION destination = { 0 }, test_destination = { 0 };
    int len = 0, apdu_len = 0, null_len = 0, test_len = 0;

    destination.Recipient.tag = BACNET_RECIPIENT_TAG_DEVICE;
    null_len = bacnet_destination_encode(NULL, &destination);
    apdu_len = bacnet_destination_encode(apdu, &destination);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_destination_decode(apdu, apdu_len,
        &test_destination);
    zassert_equal(apdu_len, test_len, NULL);

    destination.Recipient.tag = BACNET_RECIPIENT_TAG_ADDRESS;
    null_len = bacnet_destination_encode(NULL, &destination);
    apdu_len = bacnet_destination_encode(apdu, &destination);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_destination_decode(apdu, apdu_len,
        &test_destination);
    zassert_equal(test_len, apdu_len, NULL);

    null_len = bacnet_destination_encode(NULL, &destination);
    apdu_len = bacnet_destination_encode(apdu, &destination);
    zassert_equal(apdu_len, null_len, NULL);
    test_len = bacnet_destination_decode(apdu, apdu_len,
        &test_destination);
    zassert_equal(test_len, apdu_len, NULL);

    /* decoding, some negative tests */
    test_len = bacnet_destination_decode(NULL, apdu_len,
        &test_destination);
    zassert_equal(test_len, BACNET_STATUS_REJECT, NULL);
    test_len = bacnet_destination_decode(apdu, 0,
        &test_destination);
    zassert_equal(test_len, BACNET_STATUS_REJECT, NULL);
    test_len = bacnet_destination_decode(apdu, apdu_len,
        NULL);
    zassert_equal(test_len, BACNET_STATUS_REJECT, NULL);
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
    int len = 0, test_len = 0;
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
    /* get the length */
    len = bacnet_destination_to_ascii(&test_destination, NULL, 0);
    if (len > 0) {
        test_ascii = calloc(len, 1);
        if (test_ascii) {
            test_len = bacnet_destination_to_ascii(
                &test_destination, test_ascii, len);
            zassert_equal(len, test_len, NULL);
            free(test_ascii);
        }
    }

}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacnet_destination_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(bacnet_destination_tests,
     ztest_unit_test(testBACnetDestination),
     ztest_unit_test(test_BACnetDestination_ASCII)
     );

    ztest_run_test_suite(bacnet_destination_tests);
}
#endif
