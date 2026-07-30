/**
 * @file
 * @brief Stub implementation of the APDU layer needed to link h_npdu.c
 * @copyright SPDX-License-Identifier: MIT
 *
 * h_npdu.c only reaches into the APDU layer when an NPDU carries an APDU
 * (i.e. it is not a network layer message). None of the tests in this
 * suite exercise that path, so these stubs just need to exist for the
 * linker.
 */
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/apdu.h"

void apdu_handler(BACNET_ADDRESS *src, uint8_t *apdu, uint16_t apdu_len)
{
    (void)src;
    (void)apdu;
    (void)apdu_len;
}

void apdu_network_priority_set(uint8_t pri)
{
    (void)pri;
}
