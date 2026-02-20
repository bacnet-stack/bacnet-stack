/**
 * @file
 * @authors Miguel Fernandes <miguelandre.fernandes@gmail.com> Testimony Adams <adamstestimony@gmail.com>
 * @date 6 de Jun de 2013
 * @brief BACnet Virtual Link Control for Pico
 */

#ifndef BVLC_PICO_H
#define BVLC_PICO_H

#include <stdint.h>
#include "bacnet/datalink/bvlc.h"

#define BACNET_BVLC_RESULT uint8_t
#define BACNET_BVLC_FUNCTION uint8_t

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum packet size for BACnet/IP */
#define BIP_MPDU_MAX 1506

uint16_t bvlc_for_non_bbmd(
    uint8_t *addr,
    uint16_t *port,
    uint8_t *npdu,
    uint16_t received_bytes);

BACNET_BVLC_FUNCTION pico_bvlc_get_function_code(void);

#ifdef __cplusplus
}
#endif

#endif /* BVLC_PICO_H */
