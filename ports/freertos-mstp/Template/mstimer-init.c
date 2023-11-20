/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2023
 * @brief main application
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "bacnet/basic/sys/mstimer.h"

/* counter for the timer */
static volatile uint32_t Millisecond_Counter;

static TaskHandle_t mstimer_task_handle;
static void mstimer_task(void *pvParameters)
{
    const TickType_t xFrequency = 1 / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime;

    xLastWakeTime = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        Millisecond_Counter++;
    }
    /* Should never go there */
    vTaskDelete(NULL);
}

/**
 * @brief Function retrieves the system time, in milliseconds.
 * The system time is the time elapsed since OS was started.
 * @return milliseconds since OS was started
 */
unsigned long mstimer_now(void)
{
    return Millisecond_Counter;
}

/**
 * @brief Initialization for timer
 */
void mstimer_init(void)
{
    xTaskCreate(mstimer_task, (const signed char *const) "Millisecond Timer",
        configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1,
        &mstimer_task_handle);
}
