/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2023
 * @brief main application
 *
 * SPDX-License-Identifier: MIT
 */
#include <FreeRTOS.h>
#include "mstimer.h"
#include "rs485.h"
#include "bacnet.h"

#if configCHECK_FOR_STACK_OVERFLOW
static volatile unsigned Overflow_Count;
/* FreeRTOS - signal when task stack is overflowed */
extern void vApplicationStackOverflowHook(
    xTaskHandle xTask, signed char *pcTaskName);
void vApplicationStackOverflowHook(
    xTaskHandle xTask,
    signed char *pcTaskName)
{
    Overflow_Count++;
    for (;;) {
        /* fixme! */
    }
}
#endif

int main(void)
{
    system_init();
    mstimer_init();
    bacnet_init();

    /*Start Scheduler*/
    vTaskStartScheduler();
    while (1) {
        /* running the RTOS */
    }
}
