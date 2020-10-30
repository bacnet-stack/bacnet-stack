### BACnet MS/TP on NUCLEO-F429ZI STM32 Platform

The NUCLEO-F429ZI platform includes the following peripherals:

1) USB ST-Link

2) Ethernet

3) Arduino Uno V3 compatible RS485 shield (DFR0259)

### NUCLEO-F429ZI Arduino Uno V3 Pin Mapping

| NUCLEO   | STM32F4xx   | Arduino     | RS485 DFR0259
|:---------|:------------|:------------|:------------
| CN8-1    | NC          | NC          |
| CN8-3    | IOREF       | IOREF       |
| CN8-5    | RESET       | RESET       | RST BUTTON
| CN8-7    | +3V3        | +3V3        |
| CN8-9    | +5V         | +5V         | +5V
| CN8-11   | GND         | GND         | GND
| CN8-13   | GND         | GND         | GND
| CN8-15   | VIN         | VIN         |
|          |             |             |
| CN9-1    | PA3         | A0          |
| CN9-3    | PC0         | A1          |
| CN9-5    | PC3         | A2          |
| CN9-7    | PF3         | A3          |
| CN9-9    | PF5         | A4          |
| CN9-11   | PF10        | A5          |
| CN9-13   | NC          | D72         |
| CN9-15   | PA7         | D71         |
| CN9-17   | PF2         | D70         |
| CN9-19   | PF1         | D69         |
| CN9-21   | PF0         | D68         |
| CN9-23   | GND         | GND         |
| CN9-25   | PD0         | D67         |
| CN9-27   | PD1         | D66         |
| CN9-29   | PG0         | D65         |
|          |             |             |
| CN7-2    | PB8         | D15         |
| CN7-4    | PB9         | D14         |
| CN7-6    | AVDD        | AVDD        |
| CN7-8    | GND         | GND         |
| CN7-10   | PA5         | D13         | LED-L ANODE (+) (DFR0259)
| CN7-12   | PA6         | D12         |
| CN7-14   | PA7         | D11         |
| CN7-16   | PD14        | D10         |
| CN7-18   | PD15        | D9          | CE (LINKSPRITE V2.1)
| CN7-20   | PF12        | D8          |
|          |             |             |
| CN10-2   | PF13        | D7          |
| CN10-4   | PE9         | D6          |
| CN10-6   | PE11        | D5          |
| CN10-8   | PF14        | D4          |
| CN10-10  | PE13        | D3          |
| CN10-12  | PF15        | D2          | CE (DFR0259)
| CN10-14  | PG14        | D1          | TXD
| CN10-16  | PG9         | D0          | RXD
