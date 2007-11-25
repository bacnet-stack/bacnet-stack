This port was done with the Atmel ATmega168 using two tools:
1. The WinAVR compiler avr-gcc (GCC) 4.1.2 (WinAVR 20070525)
and tools from <http://winavr.sourceforge.net/>, hints and 
sample code from <http://www.avrfreaks.net/> and 
<http://savannah.gnu.org/projects/avr-libc/>
"avr-binutils, avr-gcc, and avr-libc form the heart of the 
Free Software toolchain for the Atmel AVR microcontrollers."
2. AVR Studio from Atmel <http://atmel.com/>

The hardware is expected to utilize the signals as defined
in the spreadsheet hardware.ods (OpenOffice.org calc).
Attach a DS75176 RS-485 transceiver (or similar) to the USART.
DS75176 ATmega168
------  ---------
 RO       RXD
 /RE      --choice of I/O
 DE       --choice of I/O
 DI       TXD
 GND      GND
 DO       --to RS-485 wire
 DO       --to RS-485 wire
 +5V      From 5V Regulator

The makefile allows you to build just the dlmstp or a simple
server. dlmstp is the datalink layer for MS/TP over RS-485.

I used the makefile from the command line on Windows:
C:\code\bacnet-stack\ports\atmega168> make clean all

I also used the bacnet.aps project file in AVR Studio to 
make the project and simulate it.

Note that the bacnet stack is currently layed out as encapsulating 
modules that include both client and server functionality for each service.
The nice thing about the all in one modules that it permits easy unit 
testing. The bad thing is that it puts all the unused code into the build.  
Therefore, until the code is split into separate modules, 
the unused sections must be commented out (use #if 0, #endif).  

Hopefully you find it useful!

Steve Karg
