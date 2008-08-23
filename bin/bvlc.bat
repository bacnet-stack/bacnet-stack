REM Example of parameters for Foreign Device Registration
REM This CMD window will use port 47809 to communicate
set BACNET_IP_PORT=47809
REM The BBMD is located at the standard port 47808 and at
REM the dotted IP address passed in on the command line.
REM When the demo client applications see the BBMD address,
REM they register as a Foreign Device to it.
set BACNET_BBMD_PORT=47808
set BACNET_BBMD_ADDRESS=%1
