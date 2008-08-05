/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2006 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#include <stdint.h>
#include "bacenum.h"
#include "bacdcode.h"
#include "bacdef.h"
#include "bacapp.h"
#include "timesync.h"

/* encode service */
int timesync_encode_apdu_service(
    uint8_t * apdu,
    BACNET_UNCONFIRMED_SERVICE service,
    BACNET_DATE * my_date,
    BACNET_TIME * my_time)
{
    int len = 0;        /* length of each encoding */
    int apdu_len = 0;   /* total length of the apdu, return value */

    if (apdu && my_date && my_time) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = service;
        apdu_len = 2;
        len = encode_application_date(&apdu[apdu_len], my_date);
        apdu_len += len;
        len = encode_application_time(&apdu[apdu_len], my_time);
        apdu_len += len;
    }

    return apdu_len;
}

int timesync_utc_encode_apdu(
    uint8_t * apdu,
    BACNET_DATE * my_date,
    BACNET_TIME * my_time)
{
    return timesync_encode_apdu_service(apdu,
        SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, my_date, my_time);
}

int timesync_encode_apdu(
    uint8_t * apdu,
    BACNET_DATE * my_date,
    BACNET_TIME * my_time)
{
    return timesync_encode_apdu_service(apdu,
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, my_date, my_time);
}

/* decode the service request only */
int timesync_decode_service_request(
    uint8_t * apdu,
    unsigned apdu_len,
    BACNET_DATE * my_date,
    BACNET_TIME * my_time)
{
    int len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value = 0;

    if (apdu_len && my_date && my_time) {
        /* date */
        len +=
            decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        if (tag_number == BACNET_APPLICATION_TAG_DATE) {
            len += decode_date(&apdu[len], my_date);
        } else
            return -1;
        /* time */
        len +=
            decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        if (tag_number == BACNET_APPLICATION_TAG_TIME) {
            len += decode_bacnet_time(&apdu[len], my_time);
        } else
            return -1;
    }

    return len;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

int timesync_decode_apdu_service(
    uint8_t * apdu,
    BACNET_UNCONFIRMED_SERVICE service,
    unsigned apdu_len,
    BACNET_DATE * my_date,
    BACNET_TIME * my_time)
{
    int len = 0;

    if (!apdu)
        return -1;
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST)
        return -1;
    if (apdu[1] != service)
        return -1;
    /* optional limits - must be used as a pair */
    if (apdu_len > 2) {
        len =
            timesync_decode_service_request(&apdu[2], apdu_len - 2, my_date,
            my_time);
    }

    return len;
}

int timesync_utc_decode_apdu(
    uint8_t * apdu,
    unsigned apdu_len,
    BACNET_DATE * my_date,
    BACNET_TIME * my_time)
{
    return timesync_decode_apdu_service(apdu,
        SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, apdu_len, my_date,
        my_time);
}

int timesync_decode_apdu(
    uint8_t * apdu,
    unsigned apdu_len,
    BACNET_DATE * my_date,
    BACNET_TIME * my_time)
{
    return timesync_decode_apdu_service(apdu,
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, apdu_len, my_date, my_time);
}

void testTimeSyncData(
    Test * pTest,
    BACNET_DATE * my_date,
    BACNET_TIME * my_time)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    BACNET_DATE test_date;
    BACNET_TIME test_time;

    len = timesync_encode_apdu(&apdu[0], my_date, my_time);
    ct_test(pTest, len != 0);
    apdu_len = len;
    len = timesync_decode_apdu(&apdu[0], apdu_len, &test_date, &test_time);
    ct_test(pTest, len != -1);
    ct_test(pTest, datetime_compare_time(my_time, &test_time) == 0);
    ct_test(pTest, datetime_compare_date(my_date, &test_date) == 0);

    len = timesync_utc_encode_apdu(&apdu[0], my_date, my_time);
    ct_test(pTest, len != 0);
    apdu_len = len;
    len = timesync_utc_decode_apdu(&apdu[0], apdu_len, &test_date, &test_time);
    ct_test(pTest, len != -1);
    ct_test(pTest, datetime_compare_time(my_time, &test_time) == 0);
    ct_test(pTest, datetime_compare_date(my_date, &test_date) == 0);
}

void testTimeSync(
    Test * pTest)
{
    BACNET_DATE bdate;
    BACNET_TIME btime;

    bdate.year = 2006;  /* AD */
    bdate.month = 4;    /* 1=Jan */
    bdate.day = 11;     /* 1..31 */
    bdate.wday = 1;     /* 1=Monday */

    btime.hour = 7;
    btime.min = 0;
    btime.sec = 3;
    btime.hundredths = 1;

    testTimeSyncData(pTest, &bdate, &btime);
}

#ifdef TEST_TIMESYNC
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Time-Sync", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testTimeSync);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_WHOIS */
#endif /* TEST */
