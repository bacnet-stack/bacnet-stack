/**
 * @file
 * @author Miguel Fernandes <miguelandre.fernandes@gmail.com>
 * @date 6 de Jun de 2013
 * @brief For redirecting stdout, stdin and stderr
 *  see http://www.appelsiini.net/2011/simple-usart-with-avr-libc
 */
#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>
#include <util/setbaud.h>
#include "hardware.h"
#include "uart.h"

void uart_init(void)
{
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;

#if USE_2X
    UCSR0A |= _BV(U2X0);
#else
    UCSR0A &= ~(_BV(U2X0));
#endif

    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8-bit data */
    UCSR0B = _BV(RXEN0) | _BV(TXEN0); /* Enable RX and TX */
}

void uart_putchar(char c, FILE *stream)
{
    if (c == '\n') {
        uart_putchar('\r', stream);
    }
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
}

char uart_getchar(FILE *stream)
{
    loop_until_bit_is_set(UCSR0A, RXC0); /* Wait until data exists. */
    return UDR0;
}
