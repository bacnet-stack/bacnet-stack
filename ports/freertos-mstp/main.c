/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2023
 * @brief main application
 *
 * SPDX-License-Identifier: MIT
 */
#include "FreeRTOS.h"
#include "task.h"
#include "bacnet/basic/sys/mstimer.h"
#include "rs485.h"
#include "bacnet.h"

#if configCHECK_FOR_STACK_OVERFLOW
static volatile unsigned Overflow_Count;
/* FreeRTOS - signal when task stack is overflowed */
void vApplicationStackOverflowHook(
    TaskHandle_t xTask,
    char *pcTaskName)
{
    Overflow_Count++;
    for (;;) {
        /* fixme! */
    }
}
#endif

int main(void)
{
    mstimer_init();
    bacnet_init();

    /*Start Scheduler*/
    vTaskStartScheduler();
    while (1) {
        /* running the RTOS */
    }
}
