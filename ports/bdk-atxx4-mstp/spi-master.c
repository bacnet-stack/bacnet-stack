/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "hardware.h"
#include "spi-master.h"

void spi_master_init(void)
{
    /* SS, MOSI, MISO, SCK */
    DDRB=(1<<PINB4)|(1<<PINB5)|(0<<PINB6)|(1<<PINB7);
    /* Enable SPI, Master, Clk/128 */
    SPCR=(1<<SPR0)|(1<<SPR1)|(1<<MSTR)|(0<<DORD)|(1<<SPE)|(0<<SPIE);
    power_spi_enable();
}

uint8_t spi_master_transfer(
    uint8_t txdata)
{
    /* Send SPI data */
    SPDR = txdata;
    /* Wait for transmission complete */
    while(!(SPSR & (1<<SPIF))) {
        /* do nothing */
    }

    return SPDR;
}

#ifdef TEST_SPI_MASTER
uint8_t Data_Register;
int main(void)
{
    spi_master_init();

    while(1)
    {
        /* loopback test */
        Data_Register = spi_master_transfer(0xBA);
    }
}
#endif
