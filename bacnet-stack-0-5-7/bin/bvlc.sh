#!/bin/bash
echo Example of parameters for Foreign Device Registration
BACNET_IP_PORT=47809
export BACNET_IP_PORT
echo This console will use port ${BACNET_IP_PORT} to communicate.
echo
BACNET_BBMD_PORT=47808
export BACNET_BBMD_PORT
echo The BBMD is located at the port ${BACNET_BBMD_PORT} and
echo is at the dotted IP address passed on the command line.
BACNET_BBMD_ADDRESS=${1}
export BACNET_BBMD_ADDRESS
echo The BBMD IP address is ${BACNET_BBMD_ADDRESS}
echo When the demo client applications see the BBMD address,
echo they register as a Foreign Device to it.
echo
echo Launching new shell using the BBMD environment...
/bin/bash
