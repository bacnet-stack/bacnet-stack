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
static void testTimeSyncRecipientData(
    const BACNET_RECIPIENT_LIST *recipient1,
    const BACNET_RECIPIENT_LIST *recipient2)
{
    unsigned i = 0;

    if (recipient1 && recipient2) {
        zassert_equal(recipient1->tag, recipient2->tag, NULL);
        if (recipient1->tag == 0) {
            zassert_equal(
                recipient1->type.device.type, recipient2->type.device.type,
                NULL);
            zassert_equal(
                recipient1->type.device.instance,
                recipient2->type.device.instance, NULL);
        } else if (recipient1->tag == 1) {
            zassert_equal(
                recipient1->type.address.net, recipient2->type.address.net,
                NULL);
            if (recipient1->type.address.net == BACNET_BROADCAST_NETWORK) {
                zassert_equal(
                    recipient1->type.address.mac_len,
                    recipient2->type.address.mac_len, NULL);
            } else if (recipient1->type.address.net) {
                zassert_equal(
                    recipient1->type.address.len, recipient2->type.address.len,
                    NULL);
                for (i = 0; i < recipient1->type.address.len; i++) {
                    zassert_equal(
                        recipient1->type.address.adr[i],
                        recipient2->type.address.adr[i], NULL);
                }
            } else {
                zassert_equal(
                    recipient1->type.address.mac_len,
                    recipient2->type.address.mac_len, NULL);
                for (i = 0; i < recipient1->type.address.mac_len; i++) {
                    zassert_equal(
                        recipient1->type.address.mac[i],
                        recipient2->type.address.mac[i], NULL);
                }
            }
        } else {
            zassert_true(recipient1->tag <= 1, NULL);
        }
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(timesync_tests, testTimeSyncRecipient)
#else
static void testTimeSyncRecipient(void)
#endif
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    BACNET_RECIPIENT_LIST recipient[4] = { 0 };
    BACNET_RECIPIENT_LIST test_recipient[4] = { 0 };

    /* link the recipient list */
    recipient[0].next = &recipient[1];
    recipient[1].next = &recipient[2];
    recipient[2].next = &recipient[3];
    recipient[3].next = NULL;
    /* link the test recipient list */
    test_recipient[0].next = &test_recipient[1];
    test_recipient[1].next = &test_recipient[2];
    test_recipient[2].next = &test_recipient[3];
    test_recipient[3].next = NULL;
    /* load the test data - device */
    recipient[0].tag = 0;
    recipient[0].type.device.type = OBJECT_DEVICE;
    recipient[0].type.device.instance = 1234;
    /* load the test data - address */
    /* network = broadcast */
    recipient[1].tag = 1;
    recipient[1].type.address.net = BACNET_BROADCAST_NETWORK;
    recipient[1].type.address.mac_len = 0;
    /* network = non-zero */
    recipient[2].tag = 1;
    recipient[2].type.address.net = 4201;
    recipient[2].type.address.adr[0] = 127;
    recipient[2].type.address.len = 1;
    /* network = zero */
    recipient[3].type.address.net = 0;
    recipient[3].type.address.mac[0] = 10;
    recipient[3].type.address.mac[1] = 1;
    recipient[3].type.address.mac[2] = 0;
    recipient[3].type.address.mac[3] = 86;
    recipient[3].type.address.mac[4] = 0xBA;
    recipient[3].type.address.mac[5] = 0xC1;
    recipient[3].type.address.mac_len = 6;
    /* perform positive test */
    len = timesync_encode_timesync_recipients(
        &apdu[0], sizeof(apdu), &recipient[0]);
    zassert_not_equal(len, BACNET_STATUS_ABORT, NULL);
    zassert_true(len > 0, NULL);
    len = timesync_decode_timesync_recipients(
        &apdu[0], sizeof(apdu), &test_recipient[0]);
    zassert_not_equal(len, BACNET_STATUS_ABORT, NULL);
    zassert_true(len > 0, NULL);
    testTimeSyncRecipientData(&recipient[0], &test_recipient[0]);
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
