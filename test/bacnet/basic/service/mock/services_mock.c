/**
 * @file
 * @brief mock BACnet service handler functions
 * @author Steve Karg
 * @date January 2023
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/service/h_apdu.h>
#include <bacnet/basic/tsm/tsm.h>

bool tsm_get_transaction_pdu(
    uint8_t invokeID,
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *ndpu_data,
    uint8_t *apdu,
    uint16_t *apdu_len)
{
    (void)invokeID;
    (void)dest;
    (void)ndpu_data;
    (void)apdu;
    (void)apdu_len;

    return false;
}

uint16_t apdu_decode_confirmed_service_request(
    uint8_t *apdu,
    uint16_t apdu_len,
    BACNET_CONFIRMED_SERVICE_DATA *service_data,
    uint8_t *service_choice,
    uint8_t **service_request,
    uint16_t *service_request_len)
{
    (void)apdu;
    (void)apdu_len;
    (void)service_data;
    (void)service_choice;
    (void)service_request;
    (void)service_request_len;

    return 0;
}
