/**
 * @file
 * @brief uBASIC-Plus program object for BACnet
 * @author Steve Karg
 * @date 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef PROGRAM_UBASIC_H
#define PROGRAM_UBASIC_H
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void Program_UBASIC_Task(void);
void Program_UBASIC_Init(uint32_t instance);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
