BACnet Simple Router Demo
=========================

The Simple Router demo connects one BACnet/IP and one BACnet MS/TP network.
The router demo also includes a BBMD so that Foreign Device Registration can
be used to tunnel local command line demos to BACnet/IP and BACnet MS/TP
networks.

Configuration
=============

It uses environment variables to configure
the MS/TP COM port and BACnet/IP address on Windows:

set BACNET_IFACE=192.168.0.1
set BACNET_MSTP_IFACE=COM1
set BACNET_MSTP_BAUD=38400
set BACNET_MSTP_MAC=99

It uses environment variables to configure
the MS/TP COM port and BACnet/IP address on Linux:

export BACNET_IFACE=eth0
export BACNET_MSTP_IFACE=/dev/ttyUSB0
export BACNET_MSTP_BAUD=38400
export BACNET_MSTP_MAC=99

Also uses these configurations, but defaults to these values if not set:
set BACNET_MAX_INFO_FRAMES=128
set BACNET_MAX_MASTER=127
set BACNET_IP_PORT=47808
set BACNET_IP_NET=1
set BACNET_MSTP_NET=2

Note: NET number must be unique and 1..65534 (never 0 or 65535)

Example Usage
=============
Build the demo applications for BACnet/IP:
$ make
Build the simple router demo:
$ make router-mstp
Configure the router demo:
$ ./bin/router-mstp.sh enp0s3 /dev/ttyUSB0
Run the router demo:
$ ./bin/router-mstp

In another terminal, configure client apps for Foreign Device Registration:
$ ./bin/bvlc.sh 192.168.0.1
$ ./bin/bacwi -1
$ ./bin/bacepics 123
