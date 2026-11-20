/**
 * @file
 * @brief mock BACnet MS/TP datalink functions
 * @date November 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <bacnet/bacdef.h>
#include <bacnet/npdu.h>
#include <bacnet/datalink/dlmstp.h>

static uint8_t Max_Info_Frames = 1;
static uint8_t Max_Master = 127;

bool dlmstp_init(const char *ifname)
{
    (void)ifname;

    return true;
}

void dlmstp_cleanup(void)
{
}

int dlmstp_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    (void)dest;
    (void)npdu_data;
    (void)pdu;
    (void)pdu_len;

    return 0;
}

uint16_t dlmstp_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout)
{
    (void)src;
    (void)pdu;
    (void)max_pdu;
    (void)timeout;

    return 0;
}

void dlmstp_get_my_address(BACNET_ADDRESS *my_address)
{
    if (my_address) {
        my_address->mac_len = 0;
    }
}

void dlmstp_get_broadcast_address(BACNET_ADDRESS *dest)
{
    if (dest) {
        dest->mac_len = 0;
    }
}

/**
 * @brief Store the value without a range check, so that a test can see
 *  every value that the caller passes
 * @param max_info_frames - value to store
 */
void dlmstp_set_max_info_frames(uint8_t max_info_frames)
{
    Max_Info_Frames = max_info_frames;
}

uint8_t dlmstp_max_info_frames(void)
{
    return Max_Info_Frames;
}

/**
 * @brief Store the value without a range check, so that a test can see
 *  every value that the caller passes
 * @param max_master - value to store
 */
void dlmstp_set_max_master(uint8_t max_master)
{
    Max_Master = max_master;
}

uint8_t dlmstp_max_master(void)
{
    return Max_Master;
}
