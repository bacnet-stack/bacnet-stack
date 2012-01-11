This port was done with the STM32 ARM Cortex-M3 STM32F103RGT6 on 
a STM32 Discovery Kit using the STM32 CMSIS library and drivers
and IAR EWARM 6.10 compiler.

The hardware interface only uses the USART and a peripheral pin
(RTS) for the MS/TP RS-485 interface, and the System Clock for 
the millisecond timer.

It was created for the STM32 Design Challenge on March 20, 2011,
by Steve Karg.