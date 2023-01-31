.. _SC-node-hub:

Secure Connect Hub
##################

Overview
********

The sc-hub sample application for Zephyr implements a SC server with supporting
the Webscoket protocol.

The source code for this sample application can be found at:
:bacnet_zephyr_file:`samples/sc-hub`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

There are multiple ways to use this application. One of the most common
usage scenario is to run echo-server application inside QEMU. This is
described in :ref:`networking_with_qemu`.

Build echo-server sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/sc-hub
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Example building for the qemu_x86 support:

.. zephyr-app-commands::
   :zephyr-app: samples/sc-hub
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Running with a SC Hub in Linux Host
===================================

There is one useful testing scenario that can be used with Linux host.
Here echo-server is run in QEMU and echo-client is run in Linux host.

To use QEMU for testing, follow the :ref:`networking_with_qemu` guide.

1 Create network interface:
  for QEMU:
    - run `./loop-socat.sh` from `~/net-tools` in first concole
    - run `sudo ./loop-slip-tap.sh` from `~/net-tools` in second concole

  for native_posix board:
    - run `sudo ./net-setup.sh` from `~/net-tools` in a concole

2 Check path to certificates in CMakeLists.txt

3 Run sc-hub application in QEMU or native_posix:

.. zephyr-app-commands::
   :zephyr-app: samples/sc-hub
   :host-os: unix
   :board: qemu_x86 or native_posix
   :goals: run
   :compact:

4 Run a bacnet application as SC node, ex :bacnet:`bin/server`
  HUB URL parameters must be IP of sc-hub (usualy "192.0.2.1"):

  - BACNET_SC_PRIMARY_HUB_URI="wss://192.0.2.1:50000"
  - BACNET_SC_FAILOVER_HUB_URI="wss://192.0.2.1:50000"

  See :bacnet:`bin/bsc-server.sh` for decription SC Hub parameters.

5 Check results:
  The :bacnet:`bin/readprop` can read a bacnet propery SC Hub using connect to
  the SC Hub.

  HUB URL parameters must be IP of sc-hub (usualy "192.0.2.1"):

  - BACNET_SC_PRIMARY_HUB_URI="wss://192.0.2.1:50000"
  - BACNET_SC_FAILOVER_HUB_URI="wss://192.0.2.1:50000"
  
  Ex. run
  $ :bacnet:`bin/bsc-client.sh`
  $ :bacnet:`bin/bacrp` 123 56 1 77

  returns string  "BACnet/BSC Port"

  See :bacnet:`bin/bsc-client.sh` for decription SC parameters for the readprop.
