/************************************************************************
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
*
*************************************************************************/
#include "hardware.h"

#if defined(__GNUC__) && (__GNUC__ > 4)
/* AVR fuse settings for ATmega644P */
FUSES = {
    /* External Ceramic Resonator - configuration */
    /* Full Swing Crystal Oscillator Clock Selection */
    /* Ceramic resonator, slowly rising power 1K CK 14CK + 65 ms */
    /* note: fuses are enabled by clearing the bit, so
       any fuses listed below are cleared fuses. */
    .low = (FUSE_CKSEL3 & FUSE_SUT0 & FUSE_SUT1),
        /* BOOTSZ configuration:
           BOOTSZ1 BOOTSZ0 Boot Size
           ------- ------- ---------
           1       1      512
           1       0     1024
           0       1     2048
           0       0     4096
         */
        /* note: fuses are enabled by clearing the bit, so
           any fuses listed below are cleared fuses. */
        .high =
        (FUSE_BOOTSZ1 & FUSE_BOOTRST & FUSE_EESAVE & FUSE_SPIEN & FUSE_JTAGEN),
        /* Brown-out detection VCC=2.7V */
        /* BODLEVEL configuration 
           BODLEVEL2 BODLEVEL1 BODLEVEL0 Voltage
           --------- --------- --------- --------
           1         1         1     disabled
           1         1         0       1.8V
           1         0         1       2.7V
           1         0         0       4.3V
         */
        /* note: fuses are enabled by clearing the bit, so
           any fuses listed below are cleared fuses. */
        .extended = (FUSE_BODLEVEL1 & FUSE_BODLEVEL0)
};

/* AVR lock bits - unlocked */
LOCKBITS = LOCKBITS_DEFAULT;
#endif
