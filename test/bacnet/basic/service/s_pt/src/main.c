/**
 * @file
 * @brief Unit tests for Send_Private_Transfer_Request
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/ztest.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/bacdcode.h"
#include "bacnet/dcc.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/service/s_pt.h"
#include "bacnet/basic/tsm/tsm.h"

#define TEST_DEVICE_ID 1234
#define GUARD_SIZE 16
#define GUARD_BYTE 0xA5

/* stub to avoid linking the entire application data library */
int bacapp_encode_data_list(
    uint8_t *apdu, const BACNET_APPLICATION_DATA_VALUE *value)
{
    if (!value) {
        return 0;
    }
    return encode_application_unsigned(apdu, value->type.Unsigned_Int);
}

/**
 * @brief Send with a destination that needs a routed NPDU and a buffer
 *  that is too short to hold it. Nothing shall be written past the
 *  caller-provided size, and the invoke ID shall be released.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(s_pt_tests, test_Send_Private_Transfer_Request_ShortBuffer)
#else
static void test_Send_Private_Transfer_Request_ShortBuffer(void)
#endif
{
    uint8_t buffer[GUARD_SIZE + 64];
    BACNET_ADDRESS dest = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    uint8_t invoke_id;
    size_t max_pdu;
    size_t i;
    uint8_t idle_count;

    dcc_set_status_duration(COMMUNICATION_ENABLE, 0);
    dest.net = 5;
    dest.len = 1;
    dest.mac_len = 6;
    dest.adr[0] = 1;
    address_init();
    address_add(TEST_DEVICE_ID, MAX_APDU, &dest);
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = 1;
    value.next = NULL;

    idle_count = tsm_transaction_idle_count();
    for (max_pdu = 1; max_pdu <= 4; max_pdu++) {
        memset(buffer, GUARD_BYTE, sizeof(buffer));
        invoke_id = Send_Private_Transfer_Request(
            buffer, max_pdu, TEST_DEVICE_ID, 1, 1, &value);
        zassert_equal(invoke_id, 0, "max_pdu=%zu", max_pdu);
        for (i = max_pdu; i < sizeof(buffer); i++) {
            zassert_equal(
                buffer[i], GUARD_BYTE, "max_pdu=%zu i=%zu", max_pdu, i);
        }
        zassert_equal(tsm_transaction_idle_count(), idle_count, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(s_pt_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        s_pt_tests,
        ztest_unit_test(test_Send_Private_Transfer_Request_ShortBuffer));

    ztest_run_test_suite(s_pt_tests);
}
#endif
