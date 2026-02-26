/**
 * @file
 * @brief test BACnet TimeSynchronization services encoding and decoding API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/datetime.h>
#include <bacnet/timesync.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testTimeSyncRecipientListData(
    const BACNET_RECIPIENT_LIST *list_head_a,
    const BACNET_RECIPIENT_LIST *list_head_b)
{
    bool status = false;
    const BACNET_RECIPIENT_LIST *list_a;
    const BACNET_RECIPIENT_LIST *list_b;

    list_a = list_head_a;
    list_b = list_head_b;
    while (list_a && list_b) {
        status = bacnet_recipient_same(&list_a->recipient, &list_b->recipient);
        zassert_true(status, NULL);
        list_a = list_a->next;
        list_b = list_b->next;
    }
    zassert_true(list_a == NULL && list_b == NULL, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(timesync_tests, testTimeSyncRecipient)
#else
static void testTimeSyncRecipient(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int apdu_len = 0, test_len = 0;
    BACNET_RECIPIENT_LIST recipient[4] = { 0 };
    BACNET_RECIPIENT_LIST test_recipient[4] = { 0 };

    /* link the recipient list */
    bacnet_recipient_list_link_array(recipient, ARRAY_SIZE(recipient));
    bacnet_recipient_list_link_array(
        test_recipient, ARRAY_SIZE(test_recipient));
    /* load the test data - device */
    recipient[0].recipient.tag = BACNET_RECIPIENT_TAG_DEVICE;
    recipient[0].recipient.type.device.type = OBJECT_DEVICE;
    recipient[0].recipient.type.device.instance = 1234;
    /* load the test data - address */
    /* network = broadcast */
    recipient[1].recipient.tag = BACNET_RECIPIENT_TAG_ADDRESS;
    recipient[1].recipient.type.address.net = BACNET_BROADCAST_NETWORK;
    recipient[1].recipient.type.address.mac_len = 0;
    /* network = non-zero */
    recipient[2].recipient.tag = BACNET_RECIPIENT_TAG_ADDRESS;
    recipient[2].recipient.type.address.net = 4201;
    recipient[2].recipient.type.address.adr[0] = 127;
    recipient[2].recipient.type.address.len = 1;
    /* network = zero */
    recipient[3].recipient.tag = BACNET_RECIPIENT_TAG_ADDRESS;
    recipient[3].recipient.type.address.net = 0;
    recipient[3].recipient.type.address.mac[0] = 10;
    recipient[3].recipient.type.address.mac[1] = 1;
    recipient[3].recipient.type.address.mac[2] = 0;
    recipient[3].recipient.type.address.mac[3] = 86;
    recipient[3].recipient.type.address.mac[4] = 0xBA;
    recipient[3].recipient.type.address.mac[5] = 0xC1;
    recipient[3].recipient.type.address.mac_len = 6;
    /* perform positive test */
    apdu_len = timesync_encode_timesync_recipients(
        &apdu[0], sizeof(apdu), &recipient[0]);
    zassert_not_equal(apdu_len, BACNET_STATUS_ABORT, NULL);
    zassert_true(apdu_len > 0, NULL);
    test_len = timesync_decode_timesync_recipients(
        &apdu[0], apdu_len, &test_recipient[0]);
    zassert_equal(apdu_len, test_len, NULL);
    zassert_true(test_len > 0, NULL);
    testTimeSyncRecipientListData(&recipient[0], &test_recipient[0]);
    while (apdu_len) {
        apdu_len--;
        test_len = timesync_decode_timesync_recipients(
            &apdu[0], apdu_len, &test_recipient[0]);
        if ((apdu_len == 18) || (apdu_len == 11) || (apdu_len == 5)) {
            /* some valid shorter lists */
            zassert_equal(test_len, apdu_len, NULL);
        } else {
            zassert_equal(
                test_len, BACNET_STATUS_ABORT, "apdu_len=%d test_len=%d",
                apdu_len, test_len);
        }
    }
    /* negative test */
    apdu_len = timesync_encode_timesync_recipients(
        &apdu[0], sizeof(apdu), &recipient[0]);
    test_len = timesync_decode_timesync_recipients(&apdu[0], apdu_len, NULL);
    zassert_equal(test_len, apdu_len, NULL);
}

static int timesync_decode_apdu_service(
    const uint8_t *apdu,
    BACNET_UNCONFIRMED_SERVICE service,
    unsigned apdu_len,
    BACNET_DATE *my_date,
    BACNET_TIME *my_time)
{
    int len = 0;

    if (!apdu) {
        return -1;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST) {
        return -1;
    }
    if (apdu[1] != service) {
        return -1;
    }
    /* optional limits - must be used as a pair */
    if (apdu_len > 2) {
        len = timesync_decode_service_request(
            &apdu[2], apdu_len - 2, my_date, my_time);
    }

    return len;
}

int timesync_utc_decode_apdu(
    const uint8_t *apdu,
    unsigned apdu_len,
    BACNET_DATE *my_date,
    BACNET_TIME *my_time)
{
    return timesync_decode_apdu_service(
        apdu, SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, apdu_len, my_date,
        my_time);
}

int timesync_decode_apdu(
    const uint8_t *apdu,
    unsigned apdu_len,
    BACNET_DATE *my_date,
    BACNET_TIME *my_time)
{
    return timesync_decode_apdu_service(
        apdu, SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, apdu_len, my_date,
        my_time);
}

static void
testTimeSyncData(const BACNET_DATE *my_date, const BACNET_TIME *my_time)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    BACNET_DATE test_date;
    BACNET_TIME test_time;

    len = timesync_encode_apdu(&apdu[0], my_date, my_time);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;
    len = timesync_decode_apdu(&apdu[0], apdu_len, &test_date, &test_time);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(datetime_compare_time(my_time, &test_time), 0, NULL);
    zassert_equal(datetime_compare_date(my_date, &test_date), 0, NULL);

    len = timesync_utc_encode_apdu(&apdu[0], my_date, my_time);
    zassert_not_equal(len, 0, NULL);
    apdu_len = len;
    len = timesync_utc_decode_apdu(&apdu[0], apdu_len, &test_date, &test_time);
    zassert_not_equal(len, -1, NULL);
    zassert_equal(datetime_compare_time(my_time, &test_time), 0, NULL);
    zassert_equal(datetime_compare_date(my_date, &test_date), 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(timesync_tests, testTimeSync)
#else
static void testTimeSync(void)
#endif
{
    BACNET_DATE bdate;
    BACNET_TIME btime;

    bdate.year = 2006; /* AD */
    bdate.month = 4; /* 1=Jan */
    bdate.day = 11; /* 1..31 */
    bdate.wday = 1; /* 1=Monday */

    btime.hour = 7;
    btime.min = 0;
    btime.sec = 3;
    btime.hundredths = 1;

    testTimeSyncData(&bdate, &btime);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(timesync_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        timesync_tests, ztest_unit_test(testTimeSync),
        ztest_unit_test(testTimeSyncRecipient));

    ztest_run_test_suite(timesync_tests);
}
#endif
