.. _b-ss_sample:

BACnet Profile B-SS Sample
##########################

Overview
********

This is a simple application demonstrating configuration of a
BACnet B-SS (simple sensor) device profile.

Requirements
************

* A board with Ethernet support, for instance: mimxrt1064_evk

Building and Running
********************

This sample can be found under :bacnet_file:`samples/profiles/b-ss` in
the BACnet tree.

The sample can be built for several platforms.


QEMU testing
************

The main logic of work can be found at the link
https://docs.zephyrproject.org/3.0.0/guides/networking/qemu_setup.html

Steps to testing
1 Load and make net-tools:
    git clone https://github.com/zephyrproject-rtos/net-tools
    cd net-tools
    make
2 Run net-tools loops in two termitals:
    first: cd ~/net-tools && ./loop-socat.sh
    second: cd ~/net-tools && sudo ./loop-slip-tap.sh
3 Configure prj.conf
    The net-tools creates and uses network 192.0.2.0/24 as `tap0` interface.
    Need change prj.conf:
    CONFIG_NET_CONFIG_MY_IPV4_ADDR="192.0.2.1"
    CONFIG_NET_CONFIG_PEER_IPV4_ADDR="192.0.2.2"
4 Set BACNET_IFACE to tap0:
    export BACNET_IFACE=tap0
5 Compile and run b-ss profile:
    west build -b qemu_x86 -p always -t run bacnet-stack/zephyr/samples/profiles/b-ss/
6 Run bacnet-stack app tools as a test tool, like readprop:
    bacnet-stack/apps/readprop/bacrp --mac 192.0.2.1:47808 55 17 1 77
    here:
      17 - OBJECT_SCHEDULE
      1 - schedule index
      77 - PROP_OBJECT_NAME
    expected result - string:
     "SCHEDULE 1"
