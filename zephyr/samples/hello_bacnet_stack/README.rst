.. _hello_bacnet_stack:

Hello BACnet-Stack
##################

Overview
********

A simple ssample that can be used with any :ref:`supported board <boards>` and
prints "Hello BACnet-Stack" to the console.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_bacnet_stack
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

Sample Output
=============

.. code-block:: console

    Hello BACnet-Stack! x86

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
