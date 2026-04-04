/**
 * @file
 * @brief BACnet MS/TP datalink environment declarations for the PlatformIO
 * ESP32 port
 * @author Kato Gangstad
 */

#ifndef M5STAMPLC_DLENV_H
#define M5STAMPLC_DLENV_H

#include <stdbool.h>
#include <stdint.h>

#include "bacnet/bacdef.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BACNET_MSTP_MAX_MASTER 127
#define BACNET_MSTP_MAX_INFO_FRAMES 1

/**
 * @brief Initialize the BACnet MS/TP datalink environment
 * @param mac_address local MS/TP MAC address
 * @return true if initialization succeeded
 */
bool m5_dlenv_init(uint8_t mac_address);

/**
 * @brief Run periodic MS/TP maintenance work
 * @param seconds elapsed seconds since the last call
 */
void dlenv_maintenance_timer(uint16_t seconds);

/**
 * @brief Shut down the MS/TP datalink environment
 */
void dlenv_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif
