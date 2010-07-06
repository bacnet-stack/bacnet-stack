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

Client Tools:
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

Server Tools:
bacserv - BACnet Device Simulator

Router Tools:
baciamr - BACnet I-Am-Router to Network message
bacinitr - BACnet Initialize Router message
bacwir - BACnet Who-Is Router to Network message

Capture Tool:
The mstpcap tool is used for capturing MS/TP traffic 
from an RS-485 serial adapter and saving the packets 
in a file for viewing by Wireshark.

The source code for the BACnet-Tools can be found at:
http://bacnet.sourceforge.net/
