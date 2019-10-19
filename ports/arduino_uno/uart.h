/**
 * @file
 * @author Miguel Fernandes <miguelandre.fernandes@gmail.com>
 * @date 6 de Jun de 2013
 * @brief BACnet Virtual Link Control for Wiznet on Arduino-Uno
 */
#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void uart_init(void);
void uart_putchar(char c,
    FILE * stream);
char uart_getchar(FILE * stream);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
