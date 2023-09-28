# Raspberry Pi connected to a Blinkt! RGB card demo

This demo includes a BACnet server with Color objects and service handlers
for the eight RGB (red-green-blue) LEDs attached to the Blinkt! card.

## Installation

The demo uses pigpiod (Pi GPIO Daemon) and developer library. To install 
and run the daemon at powerup (and immediately):

    $ sudo apt install libpigpio-dev libpigpiod-if-dev pigpiod
    $ sudo systemctl enable pigpiod
    $ sudo systemctl start pigpiod

## Building
    
Build from the root folder:

    $ make blinkt

## Running

Run from the bin/ folder:

    $ ./bin/bacblinkt 9009

