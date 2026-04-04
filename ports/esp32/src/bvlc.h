/**
 * @file
 * @brief BACnet Virtual Link Control interfaces for the PlatformIO ESP32 port
 * @author Kato Gangstad
 */

#ifndef M5STAMPLC_BVLC_H
#define M5STAMPLC_BVLC_H

#include <stdint.h>

#include "bacnet/datalink/bvlc.h"

#define BACNET_BVLC_RESULT uint8_t
#define BACNET_BVLC_FUNCTION uint8_t

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BIP_MPDU_MAX
#define BIP_MPDU_MAX 1506
#endif

/**
 * @brief Handle non-BBMD BVLC functions for a received frame
 * @param addr source IPv4 address
 * @param port source UDP port
 * @param npdu received frame buffer
 * @param received_bytes frame length in bytes
 * @return non-zero when the frame was consumed by BVLC handling
 */
uint16_t bvlc_for_non_bbmd(
    uint8_t *addr, uint16_t *port, uint8_t *npdu, uint16_t received_bytes);

/**
 * @brief Get the most recently decoded BVLC function code
 * @return BVLC function code
 */
BACNET_BVLC_FUNCTION pico_bvlc_get_function_code(void);

#ifdef __cplusplus
}
#endif

#endif
