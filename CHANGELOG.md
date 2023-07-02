# BACnet Stack ChangeLog

BACnet open source protocol stack C library for embedded systems, 
Linux, MacOS, BSD, and Windows

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

The git repositories are hosted at the following sites:
* https://bacnet.sourceforge.net/
* https://github.com/bacnet*stack

## [Unreleased]

## [1.1.1] - 2023-06-30

### Fixed

- Fix BACnetARRAY encoder for index=ALL (#442)

## [1.1.0] - 2023-06-25

### Added

- Added minimim support for BACnet protocol-revision 0 through 24. See 
BACNET_PROTOCOL_REVISION define in bacdef.h
- Added example objects for color and color temperature, new for protocol-revision 24. 
- Added current-command-priority to output objects revision 17 or later.
- Added demo apps ack-alarm, add-list-element, and event (#418)
- Added UTC option to BACnet TimeSync app (#396)
- Added --time and --date options to command line time sync for override (#215)
- Added --retry option for repeating Who-Is or I-Am C number of times (#199)
- Added write BDT application (#170)
- Added repeat option to Who-Is and I-Am app (#117)
- Added UTF8 multibyte to wide char printing in ReadProperty and ReadPropertyMultiple 
apps if supported by the compiler and C library (#106).
- Added IPv6 Foreign Device Registration and IPv6 support for BSD and macOS for apps
- Added example of spawning 100 servers using a BBMD connection.
- Added COBS encode and decode from BACnet standard (#183)
- Added Schedule encoding/decoding (#319)* Add What-Is-Network-Number and Network-Number-Is services (#304)
- Added BACDL_CUSTOM datalink define which allows for externally linked datalink_xxx functions (#292)
- Added Zephyr OS integration (see zephyr folder)

### Changed

- Modified example objects to support CreateObject and DeleteObject API for 
these objects: file, analog output, binary output, multi-state output, color, 
color temperature). 
- Unit tests and functional tests have been moved from the source C files 
into their own test files under the test folder in a src/ mirrored folder 
structure under test/.  
- The testing framework was moved from ctest to ztest, using CMake, CTest, and LCOV.  
It's now possible to visually view code coverage from the testing of each file 
in the library. The tests are run in continuous integration.  Adding new tests 
can be done by as copying an existing folder under test/ as a starting point, and 
adding the new test folder name into the CMakeLists.txt under test/ folder, or 
editing the existing test C file and extending or fixing the existing test. 
- Most (all?) of the primitive value encoders are now using a snprintf() 
style API pattern, where passing NULL in the buffer parameter will 
return the length that the buffer would use (i.e. returns the length that the 
buffer needs to be).  This makes simple to write the parent encoders to check 
the length versus their buffer before attempting the encoding at the expense of 
an extra function call to get the length.
- BACnetARRAY property types now have a helper function to use for encoding (see bacnet_array_encode) 

## Deprecated

- Added BACNET_STACK_DEPRECATED for functions which will be removed in a future
release, with a suggested replacement function to use instead.

## Fixed

* Fix WritePropertyMultiple decoding (#438)
* Fix WhoIs app APDU timeout
* Fix BACnetARRAY buffer access (#414) (#430) (#437)
* Fix complex event type decoding (#420)
* Fix VMAC address update for IPv6 (#429)
* Fix compiler warnings for C89 builds (#433)
* Fix win32 build warnings (#431)
* Fix MSTP stat lost_token_counter that was missing a drop case (#421)
* Fix and extend BACnetReliability per 135-2020 (#399)
* Fix BACnet property operational-certificate-file enum value (#395)
* Fix duplicate network port enums. Add BACnet/SC port types. (#388)
* Fix the length for bad data crc in mstpcap (#393)
* Fix decode_signed32() length for -8388608 and NULL apdu handling.
* Fix gateway example Makefile build warnings (#380)
* Fix gcc pedantic warnings and GNU89 GNU99 GNU11 and GNU17 warnings (#371)
* Fix use of memmove instead of memcpy with overlapping data (#361)
* Fix linkage conflict when use stack in C++ project (#360)
* Fix ipv6 bind interface for link local comunication (#359)
* Fix static timeGetTime. Make it similar to linux port (#358)
* Fix export ucix_for_each_section_type function (#357)
* Fix min data len for UNCONFIRMED_SERVICE_REQUEST from 3 to 2 (#356)
* Fix uninitialized and unused variable compiler Warnings (#353)
* Fix the Time stamp to print ISO 8601 format for event-time-stamps (#354)
* Fix apps/router/Makefile add CFLAGS (#352)
* Fix the install dll in bindir (#351)
* Fix APDU missing services
* Fix spelling errors
* Fix missing getter for broadcast BIP socket (#350)
* Fix RPM to return UNKNOWN_OBJECT for non-existent objects (#337)
* Fix BACnet object text off-by-one.
* Fix Notifications to initially be disabled.
* Fix BACnet/IP Forwarded NPDU to FDT entries
* Fix cppcheck warnings. Fix warnings found by splint (#324) (#250) (#244)
* Fix cross compile by convert struct tm and time.h to datetime.h usage (#330)
* Fix confirmed service with service supported enumeration
* Fix the RP handler (#323)
* Fix print property name and units for lighting objects (#313)
* Fix BACnet/IP on OS to listen to broadcast on a separate socket (#293)
* Fix segmentation error when file has capacity size (#300)
* Fix the bacapp snprintf return size (#294)
* Fix writepropertymultiple error handler (#289)
* Fix ports stm32f4xx IAR project warnings (#268)
* Fix BACnet application defines for STM32 IAR project (#267)
* Fix missing bigend.c file to IAR project (#266)
* Fix unreachable code in octetstring init ascii hex (#265)
* Fix confirmed request service decode error (#259)
* Fix BDT-1 port override and BIP NAT port number for apps (#252)
* Fix MSTP to discard confirmed local broadcast (#248)
* Fix file object TSM lookup PDU type comparison
* Fix "Analog" cruft in Integer value object example (#243) (#242)
* Fix double free rpm_data apps/readpropm (#241)
* Fix MSTP transmitted frame and pdu counters. Add more stats (#236) (#235)
* Fix bitstring capacity 8-bit size bug (#227)
* Fix valgrind warnings in trendlog functions (#226)
* Fix UCI compile errors (#220)
* Fix UCI device_init from src to apps to be more maintainable (#219)
* Fix network port object where BDT is required (#184)
* Fix keylist signed integer wraparound (#179)
* Fix FreeBSD 11 compile via gmake gcc (#180)
* Fix data race issues on ring buffer for MSTP (#174)
* Fix IPv6 MAC length to be 3 after public review (#169)
* Fix Send_WhoIs_Local destination address (#167)
* Fix DCC password out of range to reject (#166)
* Fix MSTP skip data state to result in not-for-us (#165)
* Fix the gateway example routing and lookups (#163)
* Fix Issue #157 (Router segfaults) (#159)
* Fix WPM for BTL - 9.23.2.X1 (#155)
* Fix the encoding of network port number for BACnet IP (#147)
* Fix confirmed APDU from original broadcast in BIP and BIP6 datalink to discard (#149)
* Fix Bug#74 Printing of the Read property acknowledgement when APDU len is > 255 bytes (#148)
* Fix RP and RPM Network Port for indefinite object instance (#146)
* Fix Linux BIP with no default gateway (#144)
* Fix Issue #129 compile of ports/linux/dlmstp_linux.h on Alpine Linux (#139)
* Fix atomic readfile app when file reports zero length (#133)
* Fix Bug#72 reading zero bytes in linux BIP (#132)
* Fix #125 mstp.c source include (#126)
* Fix mstpcap to ignore FF padding (#120)
* Fix file object file size type (#116)
* Fix confirmed ACK and simple ack callback without casting (#112)
* Fix AVR ports IDE project builds (#111)
* Fix BACDL_ETHERNET compilation (#108)
* Fix comments, add sanity checks, and implicit type conversion warning (#102)