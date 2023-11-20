/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2023
 * @brief main application
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datalink/dlmstp.h"
#include "rs485.h"
#include "bacnet.h"

#if configCHECK_FOR_STACK_OVERFLOW
static volatile unsigned Overflow_Count;
/* FreeRTOS - signal when task stack is overflowed */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    Overflow_Count++;
    for (;;) {
        /* fixme! */
    }
}
#endif

static void print_mstp_statistics(void)
{
    struct dlmstp_statistics statistics = { 0 };
    dlmstp_fill_statistics(&statistics);
    printf("========== MSTP Statistics ==========\n");
    printf("Lost Tokens . . . . . . . : %u\n", statistics.lost_token_counter);
    printf("Frame: Received Invalid . : %u\n",
        statistics.receive_invalid_frame_counter);
    printf("Frame: Received Valid . . : %u\n",
        statistics.receive_valid_frame_counter);
    printf(
        "Frame: Transmit . . . . . : %u\n", statistics.transmit_frame_counter);
    printf("PDU: Received . . . . . . : %u\n", statistics.receive_pdu_counter);
    printf("PDU: Transmitted  . . . . : %u\n", statistics.transmit_pdu_counter);
}

int main(void)
{
    printf("FreeRTOS Template Example. Initializing...\n");
    print_mstp_statistics();
    mstimer_init();
    bacnet_init();

    /*Start Scheduler*/
    vTaskStartScheduler();
    while (1) {
        /* running the RTOS */
    }
}
