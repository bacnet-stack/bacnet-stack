### BACnet MS/TP on Arduino Uno R3 Platform - Atmega328p

The Arduino Uno R3 platform for this example uses the following peripherals:

1) USB bootloader using AVR-119 protocol

2) 7-position DIP switch for MS/TP MAC address

3) Arduino Uno V3 compatible RS485 shield (DFR0259)

### Arduino Uno V3 Pin Mapping

| ATmega328p  | Arduino     | RS485 DFR0259  | BACnet Object   |
|:------------|:------------|:---------------|:----------------|
| NC          | NC          |                |                 |
| +5V         | IOREF       |                |                 |
| RESET       | RESET       | RST BUTTON     |                 |
| +3V3        | +3V3        |                |                 |
| +5V         | +5V         | +5V            |                 |
| GND         | GND         | GND            |                 |
| GND         | GND         | GND            |                 |
| VIN         | VIN         |                |                 |
|             |             |                |                 |
| ADC0/PC0    | A0          |                | AV 0 Millivolts |
| ADC1/PC1    | A1          |                | AV 1 Millivolts |
| ADC2/PC2    | A2          |                | AV 2 Millivolts |
| ADC3/PC3    | A3          |                | AV 3 Millivolts |
| ADC4/PC4    | A4*         |                |                 |
| ADC5/PC5    | A5*         |                |                 |
|             |             |                |                 |
| ADC5/PC5    | SCL*        | I2C            |                 |
| ADC4/PC4    | SDA*        | I2C            |                 |
| AVDD        | AREF        |                |                 |
| GND         | GND         |                |                 |
| SCK/PB5     | D13         | LED-L ANODE (+)| BV 99 (output)  |
| MISO/PB4    | D12         |                | BV 9 (output)   |
| MOSI/PB3    | D11         |                | BV 8 (output)   |
| SS/PB2      | D10         |                | BV 7 (output)   |
| OC1/PB1     | D9          |                | BV 6 (output)   |
| ICP/PB0     | D8          |                | BV 5 (output)   |
|             |             |                |                 |
| AIN1/PD7    | D7          |                | BV 4 (input)    |
| AIN0/PD6    | D6          |                | BV 3 (input)    |
| T1/PD5      | D5          |                | BV 2 (input)    |
| T0/PD4      | D4          |                | BV 1 (input)    |
| INT1/PD3    | D3          |                | BV 0 (input)    |
| INT0/PD2    | D2          | CE DE /RE RTS  | RS485           |
| TXD/PD1     | D1/Tx**     | TXD            | RS485           |
| RXD/PD0     | D0/Rx**     | RXD            | RS485           |

\* ADC4/PC4: A4 and SDA are shared I/O

\* ADC5/PC5: A5 and SCL are shared I/O

\** shared with Uno R3 USB Tx/Rx. DFR0259 switch ON to disable.

### Building this Project

#### GNU Makefile

There is a GNU Makefile that uses avr-gcc and avr-libc to build the project.
avrdude can be used to program the Arduino Uno R3 board via USB.

The GNU Makefile is used in the continuous integration pipeline to validate
the build is not broken.  The Makefile is called from an Ubuntu image
container after installing the necessary tools:

    sudo apt-get update -qq
    sudo apt-get install -qq build-essential
    sudo apt-get install -qq gcc-avr avr-libc binutils-avr avrdude

To add the build and debug tools to MinGW64 environment:

    pacman --noconfirm -S mingw-w64-x86_64-gcc-avr
    pacman --noconfirm -S mingw-w64-x86_64-avr-libc
    pacman --noconfirm -S mingw-w64-x86_64-binutils-avr
    pacman --noconfirm -S mingw-w64-x86_64-avrdude

The build sequence is usually:

    make clean all

The Makefile includes a recipe to use avrdude to program the Uno R3 via USB

    make install

#### GCC and CStack Usage

CStack check for GCC is included in the device object as property 512.
The compile shows 352 (.bss) + 136 (.data) = 488 bytes of RAM used, and
the ATmega328 has 2048 bytes of RAM, leaving 1560 bytes for the CStack.
BACnet Device object proprietary property 512 is mapped to the CStack Size
and returns 1376 using ReadProperty request.  After some ReadProperty and
WriteProperty requests, the CStack shows 1159 CStack bytes free.
Note that the value 0xC5 (197) was used to paint the CStack.
Keep this in mind when developing.

##### Shield option

The DFR0259 shield for RS485 was used, but any RS485 circuit could be
attached to the Arduino Uno R3 using the same pins for Tx, Rx, CE RE/DE RTS.

A 7-position DIP switch was connected to GPIO for the MS/TP MAC address,
but the value could also be stored in EEPROM.

### BACnet Capabilities

The BACnet Capabilities include WhoIs, I-Am, ReadProperty, and
WriteProperty support.  The BACnet objects include a Device object,
10 Binary Value objects, and 10 Analog Value objects.  An GPIO output
is controlled by Binary Value object instance 0.  All required object
properties can be retrieved using ReadProperty.  The Present_Value
property of the Analog Value and Binary Value objects can be
written using WriteProperty.  The Object_Identifier, Object_Name,
Max_Info_Frames, Max_Master, and baud rate (property 9600) of the
Device object can be written using WriteProperty.
