/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void led_ld3_on(
        void);
    void led_ld4_on(
        void);
    void led_ld3_off(
        void);
    void led_ld4_off(
        void);
    bool led_ld3_state(
        void);
    void led_ld3_toggle(
        void);

    void led_tx_on(
        void);
    void led_rx_on(
        void);

    void led_tx_on_interval(
        uint16_t interval_ms);
    void led_rx_on_interval(
        uint16_t interval_ms);

    void led_tx_off(
        void);
    void led_rx_off(
        void);

    void led_tx_off_delay(
        uint32_t delay_ms);
    void led_rx_off_delay(
        uint32_t delay_ms);

    void led_tx_toggle(
        void);
    void led_rx_toggle(
        void);

    bool led_tx_state(
        void);
    bool led_rx_state(
        void);

    void led_task(
        void);
    void led_init(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
