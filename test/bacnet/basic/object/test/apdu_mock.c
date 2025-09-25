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

bool apdu_service_supported(BACNET_SERVICES_SUPPORTED service_supported)
{
    (void)service_supported;
    return true;
}

static uint16_t Timeout_Milliseconds = 1000;
uint16_t apdu_timeout(void)
{
    return Timeout_Milliseconds;
}

void apdu_timeout_set(uint16_t milliseconds)
{
    Timeout_Milliseconds = milliseconds;
}

static uint8_t Number_Of_Retries = 3;
uint8_t apdu_retries(void)
{
    return Number_Of_Retries;
}

void apdu_retries_set(uint8_t value)
{
    Number_Of_Retries = value;
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
