/**
 * @file
 * @brief LED configuration and operation
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2014
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>

#define LED_RS485_RX 0
#define LED_RS485_TX 1
#define LED_APDU 2
#define LED_DEBUG 3

#define LEDS_MAX 4


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef CONF_BOARD_ENABLE_RS485_XPLAINED
    void led_on(
        uint8_t index);
    void led_on_interval(
        uint8_t index,
        uint16_t interval_ms);
    void led_off(
        uint8_t index);
    void led_off_delay(
        uint8_t index,
        uint32_t delay_ms);
    void led_toggle(
        uint8_t index);
    bool led_state(
        uint8_t index);
    void led_task(
        void);
    void led_init(
        void);
#else
    /* dummy stubs */
	#define led_on(x)
    #define led_on_interval(x, ms)
    #define led_off(x)
    #define led_off_delay(x, ms)
    #define led_toggle(x)
    #define led_state(x)
    #define led_task()
    #define led_init()
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
