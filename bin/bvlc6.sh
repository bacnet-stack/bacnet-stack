#!/bin/bash
echo Example of parameters for Foreign Device Registration for IPv6
BACNET_BIP6_PORT=47809
export BACNET_BIP6_PORT
echo This console will use port ${BACNET_BIP6_PORT} to communicate.
echo
BACNET_BBMD6_PORT=47808
export BACNET_BBMD6_PORT
echo The BBMD is located at the port ${BACNET_BBMD6_PORT} and
echo is at the dotted IP address passed on the command line.
BACNET_BBMD6_ADDRESS=${1}
export BACNET_BBMD6_ADDRESS
echo The BBMD IP address is ${BACNET_BBMD6_ADDRESS}
echo When the demo client applications see the BBMD address,
echo they register as a Foreign Device to it.
echo
echo Launching new shell using the BBMD environment...
/bin/bash
