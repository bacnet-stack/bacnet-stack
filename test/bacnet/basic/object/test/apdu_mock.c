/**
 * @file
 * @brief mock APDU handler functions
 * @author Steve Karg
 * @date January 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/service/h_apdu.h>

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
