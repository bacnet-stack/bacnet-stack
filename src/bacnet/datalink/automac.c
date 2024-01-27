/**
 * @file
 * @brief BACnet MSTP zero-config auto MAC address
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2010
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/config.h"
#include "bacnet/datalink/mstpdef.h"
#include "bacnet/datalink/automac.h"

/* MS/TP Auto MAC address functionality */

/* table to track tokens and poll-for-master frames */
typedef struct {
    /* Poll For Master indicates empty slot */
    bool pfm : 1;
    /* a device that emits a frame indicates a used slot */
    bool emitter : 1;
    /* token - indicates a token was passed from this slot */
    /* important to know who the Next Station is */
    bool token : 1;
    /* reserve some slots for fixed addresses */
    bool reserved : 1;
} AUTO_MAC_DATA;
static AUTO_MAC_DATA Auto_MAC_Data[MSTP_MAC_SLOTS_MAX];
/* my automatic MAC address */
static uint8_t My_MAC_Address;
/* my no-token silence timer time slot in milliseconds */
static uint16_t My_Time_Slot;
/* indication that PFM has happened for a full cycle */
static bool PFM_Cycle_Complete;
/* indicate that we are an auto-mode node */
static bool Auto_Mode_Enabled;

/****************************************************************************
 * DESCRIPTION: Indication that we are an automode node
 * RETURN:      true if automode enabled
 * NOTES:       none
 *****************************************************************************/
bool automac_enabled(void)
{
    return Auto_Mode_Enabled;
}

/****************************************************************************
 * DESCRIPTION: Sets the automode status
 * RETURN:      nothing
 * NOTES:       none
 *****************************************************************************/
void automac_enabled_set(bool status)
{
    Auto_Mode_Enabled = status;
}

/****************************************************************************
 * DESCRIPTION: Indication that PFM has happened for a full cycle
 * RETURN:      true if full
 * NOTES:       none
 *****************************************************************************/
bool automac_pfm_cycle_complete(void)
{
    return PFM_Cycle_Complete;
}

/****************************************************************************
 * DESCRIPTION: Indicates that an address is used or taken
 * RETURN:      true if used
 * NOTES:       none
 *****************************************************************************/
static bool automac_address_used(uint8_t mac)
{
    bool status = false;

    if (mac < MSTP_MAC_SLOTS_MAX) {
        if ((Auto_MAC_Data[mac].emitter) || (Auto_MAC_Data[mac].reserved) ||
            (Auto_MAC_Data[mac].token)) {
            status = true;
        }
    }

    return status;
}

/****************************************************************************
 * DESCRIPTION: Validates an address as available, not taken, and within bounds
 * RETURN:      true if valid
 * NOTES:       none
 *****************************************************************************/
bool automac_free_address_valid(uint8_t mac)
{
    bool status = false;

    if (mac < MSTP_MAC_SLOTS_MAX) {
        if ((Auto_MAC_Data[mac].pfm) && (!automac_address_used(mac))) {
            status = true;
        }
    }

    return status;
}

/****************************************************************************
 * DESCRIPTION: Determines the next station address to send the token
 * RETURN:      Next_Station, or 255 if there are no next stations
 * NOTES:       none
 *****************************************************************************/
uint8_t automac_next_station(uint8_t mac)
{
    uint8_t i = 0; /* loop counter */
    uint8_t next_station = 255; /* return value */
    uint8_t test_station = 0; /* station number to test for token */

    test_station = (mac + 1) % 128;
    for (i = 0; i < MSTP_MAC_SLOTS_MAX; i++) {
        if (Auto_MAC_Data[test_station].token) {
            next_station = test_station;
            break;
        }
        test_station = (test_station + 1) % MSTP_MAC_SLOTS_MAX;
    }

    return next_station;
}

/****************************************************************************
 * DESCRIPTION: Determines the number of free MAC addresses
 * RETURN:      Number of free MAC addresses
 * NOTES:       none
 *****************************************************************************/
uint8_t automac_free_address_count(void)
{
    uint8_t i = 0;
    uint8_t slots = 0;

    for (i = 0; i < MSTP_MAC_SLOTS_MAX; i++) {
        if (automac_free_address_valid(i)) {
            slots++;
        }
    }

    return slots;
}

/****************************************************************************
 * DESCRIPTION: Determines the number of free MAC addresses
 * RETURN:      Number of free MAC addresses
 * NOTES:       none
 *****************************************************************************/
uint8_t automac_free_address_mac(uint8_t count)
{
    uint8_t i = 0;
    uint8_t slots = 0;
    uint8_t mac = 255;

    for (i = 0; i < MSTP_MAC_SLOTS_MAX; i++) {
        if (automac_free_address_valid(i)) {
            if (slots == count) {
                mac = i;
                break;
            }
            slots++;
        }
    }

    return mac;
}

/****************************************************************************
 * DESCRIPTION: Gets a free random address to use
 * RETURN:      free MAC addresses
 * NOTES:       none
 *****************************************************************************/
uint8_t automac_free_address_random(void)
{
    uint8_t count = 0;
    uint8_t random_count = 0;
    uint8_t mac = 255;

    count = automac_free_address_count();
    if (count) {
        random_count = (uint8_t)(rand() % count);
        mac = automac_free_address_mac(random_count);
    }

    return mac;
}

/****************************************************************************
 * DESCRIPTION: Gets the address stored.
 * RETURN:      MAC addresses
 * NOTES:       none
 *****************************************************************************/
uint8_t automac_address(void)
{
    return My_MAC_Address;
}

/****************************************************************************
 * DESCRIPTION: Sets the MAC address
 * RETURN:      MAC addresses
 * NOTES:       none
 *****************************************************************************/
void automac_address_set(uint8_t mac)
{
    My_MAC_Address = mac;
}

/****************************************************************************
 * DESCRIPTION: Gets the address stored.
 * RETURN:      MAC addresses
 * NOTES:       none
 *****************************************************************************/
uint16_t automac_time_slot(void)
{
    return My_Time_Slot;
}

/****************************************************************************
 * DESCRIPTION: Sets the MAC address
 * RETURN:      MAC addresses
 * NOTES:       none
 *****************************************************************************/
void automac_address_init(void)
{
    My_MAC_Address = MSTP_MAC_SLOTS_OFFSET +
        rand() % (MSTP_MAC_SLOTS_MAX - MSTP_MAC_SLOTS_OFFSET);
    /* at least as long as a dropped token - worst case */
    My_Time_Slot = Tno_token + (MSTP_MAC_SLOTS_MAX * Tslot);
    My_Time_Slot += (uint16_t)My_MAC_Address * Tslot;
}

/****************************************************************************
 * DESCRIPTION: Sets an open address slot
 * RETURN:      nothing
 * NOTES:       none
 *****************************************************************************/
void automac_pfm_set(uint8_t mac)
{
    if (mac < MSTP_MAC_SLOTS_MAX) {
        if (Auto_MAC_Data[mac].pfm) {
            /* indicate that we have completed enough PFM to continue */
            if (automac_free_address_count() > 0) {
                PFM_Cycle_Complete = true;
            }
        }
        Auto_MAC_Data[mac].pfm = true;
    }
}

/****************************************************************************
 * DESCRIPTION: Sets a used address slot
 * RETURN:      nothing
 * NOTES:       none
 *****************************************************************************/
void automac_token_set(uint8_t mac)
{
    if (mac < MSTP_MAC_SLOTS_MAX) {
        Auto_MAC_Data[mac].token = true;
    }
}

/****************************************************************************
 * DESCRIPTION: Sets a used address slot
 * RETURN:      nothing
 * NOTES:       none
 *****************************************************************************/
void automac_emitter_set(uint8_t mac)
{
    if (mac < MSTP_MAC_SLOTS_MAX) {
        Auto_MAC_Data[mac].emitter = true;
    }
}

/****************************************************************************
 * DESCRIPTION: Initializes the auto MAC module
 * RETURN:      nothing
 * NOTES:       none
 *****************************************************************************/
void automac_init(void)
{
    uint8_t i = 0;

    for (i = 0; i < MSTP_MAC_SLOTS_MAX; i++) {
        Auto_MAC_Data[i].token = false;
        Auto_MAC_Data[i].emitter = false;
        Auto_MAC_Data[i].pfm = false;
        if (i < MSTP_MAC_SLOTS_OFFSET) {
            Auto_MAC_Data[i].reserved = true;
        } else {
            Auto_MAC_Data[i].reserved = false;
        }
    }
    automac_address_init();
    PFM_Cycle_Complete = false;
}
