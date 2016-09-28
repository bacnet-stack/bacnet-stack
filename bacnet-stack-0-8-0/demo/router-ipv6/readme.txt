BACnet Simple Router Demo
=========================

The Simple Router demo connects one BACnet/IP and one BACnet/IPv6 network.
It also includes a BBMD so that Foreign Device Registration can be used
to tunnel local command line demos to BACnet/IP and BACnet IPv6 networks.

Configuration
=============

It uses environment variables to configure
the BACnet/IP port and BACnet/IPv6 address on Linux:

export BACNET_IFACE=eth0
export BACNET_BIP6_IFACE=eth0

Also uses these configurations, but defaults to these values if not set:
export BACNET_IP_PORT=47808
export BACNET_BIP6_PORT=47808
export BACNET_BIP6_BROADCAST=FF05
export BACNET_IP_NET=1
export BACNET_IP6_NET=2
