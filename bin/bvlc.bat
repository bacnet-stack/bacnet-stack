@echo off
echo Example of parameters for Foreign Device Registration
echo This CMD window will use port 47809 to communicate
@echo on
set BACNET_IP_PORT=47809
@echo off
echo The BBMD is located at the standard port 47808 and at
echo the dotted IP address passed in on the command line.
echo When the demo client applications see the BBMD address,
echo they register as a Foreign Device to it.
@echo on
set BACNET_BBMD_PORT=47808
set BACNET_BBMD_ADDRESS=%1
