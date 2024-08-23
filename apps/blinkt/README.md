# Raspberry Pi connected to a Blinkt! RGB card demo

This demo includes a BACnet server with Color objects and service handlers
for the eight RGB (red-green-blue) LEDs attached to the Blinkt! card.

## Installation

The demo uses pigpiod (Pi GPIO Daemon) and developer library. To install
and run the daemon at powerup (and immediately):

    $ sudo apt install libpigpio-dev libpigpiod-if-dev pigpiod
    $ sudo systemctl enable pigpiod
    $ sudo systemctl start pigpiod

## WiFi Power

If you are using a Raspberry Pi with WiFi, you will likely want
to disable WiFi Power saving.  Under Raspberry Pi OS, so the following.

### Startup service in systemd

To manage to start programs you should use systemd Unit files.

File name: /etc/systemd/system/wlan0pwr.service

You can edit the file using the systemd edit:

    rpi ~$ sudo systemctl --full --force edit wlan0pwr.service

In the empty editor insert these statements, save them and quit the editor:

    [Unit]
    Description=Disable wlan0 powersave
    After=network-online.target
    Wants=network-online.target

    [Service]
    Type=oneshot
    ExecStart=/sbin/iw wlan0 set power_save off

    [Install]
    WantedBy=multi-user.target

Enable the new service with:

    rpi ~$ sudo systemctl enable wlan0pwr.service

## Building the Blinkt! BACnet Application

Build from the root folder:

    $ make blinkt

## Running the Blinkt! BACnet Application

Run from the bin/ folder:

    $ ./bin/bacblinkt 9009

## Blinkt! as a Startup service with systemd

To manage to start programs you should use systemd Unit files.
Here is a very simple template you can use to start to solve your problem.
Create a new service with:

    rpi ~$ sudo systemctl --full --force edit bacnet.service

In the empty editor insert these statements, save them and quit the editor:

    [Unit]
    Description=BACnet Service
    After=network-online.target

    [Service]
    ExecStart=/home/pi/bacnet.sh

    [Install]
    WantedBy=network-online.target

Enable the new service after the next reboot with:

    rpi ~$ sudo systemctl enable bacnet.service

Create your bacnet.sh shell script in your home folder (change from pi if
that is not your home folder).  Use the shell script to set any environment
variables that you want, or configuration settings for the bin/bacblinkt
application such as a specific device ID.

A simplistic bacnet.sh script will look like this (with stdout/stderr to /dev/null):

    #!/bin/bash
    /home/pi/bacnet-stack/bin/bacblinkt 9009 > /dev/null 2>&1

