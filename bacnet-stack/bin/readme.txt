BACnet Tools are binary demo application command line
utilities that use BACnet/IP to perform a variety of
BACnet services. Some tools use BACnet WhoIs to bind to
devices, but can also use a static binding file address_cache.

Most of the tools have help (--help option), and use
environment variables to configure the datalink.

The Client Tools use WhoIs to bind to target devices.
The WhoIs can be eliminated by using the address_cache
file, which is read by each client tool from the current
working directory.  Having the device address from the
address_cache file will greatly improve the throughput
and speed of the client tools. The address_cache file
can be generated using the standard output of the bacwi tool.

EXAMPLE:
bacwi -1 > address_cache

Client Tools
------------
bacrp - BACnet ReadProperty service
bacwp - BACnet ReadProperty service
bacarf - BACnet AtomicReadFile service
bacawf - BACnet AtomicWriteFile service
bacdcc - BACnet DeviceCommunicationControl service
bacrd - BACnet ReinitializeDevice service
bacwh - BACnet WhoHas service
bacwi - BACnet WhoIs service
bacepics - BACnet EPICS for Device object.
bacts - BACnet TimeSynchronization service
bacucov - BACnet UnconfirmedChangeOfValue service
bacrpm - BACnet ReadPropertyMultiple service

Server Tools
------------
bacserv - BACnet Device Simulator

Router Tools
------------
baciamr - BACnet I-Am-Router to Network message
bacinitr - BACnet Initialize Router message
bacwir - BACnet Who-Is Router to Network message

MS/TP Capture Tool
------------------
The mstpcap tool is used for capturing MS/TP traffic
from an RS-485 serial adapter and saving the packets
in a file for viewing by Wireshark.

Environment Variables
---------------------
BACNET_APDU_TIMEOUT - set this value in milliseconds to change
    the APDU timeout.  APDU Timeout is how much time a client
    waits for a response from a BACnet device. Default is 3000ms.

BACNET_IFACE - set this value to dotted IP address (Windows) of
    the interface (see ipconfig command on Windows) for which you
    want to bind.  On Linux, set this to the /dev interface
    (i.e. eth0, arc0).  Default is eth0 on Linux, and the default
    interface on Windows.  Hence, if there is only a single network
    interface on Windows, the applications will choose it, and this
    setting will not be needed.

BACNET_IP_PORT - UDP/IP port number (0..65534) used for BACnet/IP
    communications.  Default is 47808 (0xBAC0).

BACNET_BBMD_PORT - UDP/IP port number (0..65534) used for Foreign
    Device Registration.  Defaults to 47808 (0xBAC0).

BACNET_BBMD_TIMETOLIVE - number of seconds used in Foreign Device
    Registration (0..65535). Defaults to 60000 seconds.

BACNET_BBMD_ADDRESS - dotted IPv4 address of the BBMD or Foreign Device
    Registrar.

Source Code
-----------
The source code for the BACnet-Tools can be found at:
http://bacnet.sourceforge.net/
