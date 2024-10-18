/**************************************************************************
 *
 * Copyright (C) 2011 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#include <stdint.h>
#include "stm32f4xx.h"
#include "bacnet/basic/sys/mstimer.h"
#include "led.h"

static struct mstimer Off_Delay_Timer[LED_MAX];
static bool LED_State[LED_MAX];

/*************************************************************************
 * Description: Activate the LED
 * Returns: nothing
 * Notes: none
 **************************************************************************/
void led_on(unsigned index)
{
    switch (index) {
        case LED_LD1:
            GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_SET);
            mstimer_set(&Off_Delay_Timer[index], 0);
            LED_State[index] = true;
            break;
        case LED_LD2:
            GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_SET);
            mstimer_set(&Off_Delay_Timer[index], 0);
            LED_State[index] = true;
            break;
        case LED_LD3:
            GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
            mstimer_set(&Off_Delay_Timer[index], 0);
            LED_State[index] = true;
            break;
        case LED_RS485:
            GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_SET);
            mstimer_set(&Off_Delay_Timer[index], 0);
            LED_State[index] = true;
            break;
        default:
            break;
    }
}

/*************************************************************************
 * Description: Deactivate the LED
 * Returns: nothing
 * Notes: none
 **************************************************************************/
void led_off(unsigned index)
{
    switch (index) {
        case LED_LD1:
            GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_RESET);
            mstimer_set(&Off_Delay_Timer[index], 0);
            LED_State[index] = false;
            break;
        case LED_LD2:
            GPIO_WriteBit(GPIOB, GPIO_Pin_7, Bit_RESET);
            mstimer_set(&Off_Delay_Timer[index], 0);
            LED_State[index] = false;
            break;
        case LED_LD3:
            GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET);
            mstimer_set(&Off_Delay_Timer[index], 0);
            LED_State[index] = false;
            break;
        case LED_RS485:
            GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);
            mstimer_set(&Off_Delay_Timer[index], 0);
            LED_State[index] = false;
            break;
        default:
            break;
    }
}

/*************************************************************************
 * Description: Get the state of the LED
 * Returns: true if on, false if off.
 * Notes: none
 *************************************************************************/
bool led_state(unsigned index)
{
    bool state = false;

    if (index < LED_MAX) {
        if (LED_State[index]) {
            state = true;
        }
    }

    return state;
}

/*************************************************************************
 * Description: Toggle the state of the LED
 * Returns: none
 * Notes: none
 *************************************************************************/
void led_toggle(unsigned index)
{
    if (led_state(index)) {
        led_off(index);
    } else {
        led_on(index);
    }
}

/*************************************************************************
 * Description: Delay before going off to give minimum brightness.
 * Returns: none
 * Notes: none
 *************************************************************************/
void led_off_delay(unsigned index, uint32_t delay_ms)
{
    if (index < LED_MAX) {
        mstimer_set(&Off_Delay_Timer[index], delay_ms);
    }
}

/*************************************************************************
 * Description: Turn on, and delay before going off.
 * Returns: none
 * Notes: none
 *************************************************************************/
void led_on_interval(unsigned index, uint16_t interval_ms)
{
    if (index < LED_MAX) {
        led_on(index);
        mstimer_set(&Off_Delay_Timer[index], interval_ms);
    }
}

/*************************************************************************
 * Description: Task for blinking LED
 * Returns: none
 * Notes: none
 *************************************************************************/
void led_task(void)
{
    unsigned index = 0;

    for (index = 0; index < LED_MAX; index++) {
        if (mstimer_expired(&Off_Delay_Timer[index])) {
            led_off(index);
        }
    }
}

/*************************************************************************
 * Description: Initialize the LED hardware
 * Returns: none
 * Notes: none
 *************************************************************************/
void led_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* NUCLEO board user LEDs */
    /* Enable GPIOx clock */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    /* start with fresh structure */
    GPIO_StructInit(&GPIO_InitStructure);
    /* Configure the LED on NUCLEO board */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_7 | GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* RS485 shield user LED */
    /* Enable GPIOx clock */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    /* start with fresh structure */
    GPIO_StructInit(&GPIO_InitStructure);
    /* Configure the LED on NUCLEO board */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    led_off(LED_LD1);
    led_off(LED_LD2);
    led_off(LED_LD3);
    led_off(LED_RS485);
}
