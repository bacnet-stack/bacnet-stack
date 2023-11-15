# BACnet Stack ChangeLog

BACnet open source protocol stack C library for embedded systems,
Linux, MacOS, BSD, and Windows

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

The git repositories are hosted at the following sites:
* https://bacnet.sourceforge.net/
* https://github.com/bacnet-stack/bacnet-stack/

## [Unreleased]

### Security

### Added

- Added MSTP extended frames transmit to src/datalink/mstp.c
  and ports/stm32f4xx/dlmstp.c modules (#531)
- Added MSTP extended frames to src/datalink/mstp.c module
  used by mstpcap (#529)
- Added menu to release script (#506)

### Changed

- Changed SubscribeCOV Cancellations to always reply with Result+ (#526)
- Changed Who-Has to process when DCC initiation is disabled
- Changed mstp.c external API to remove rs485.h dependency
  for send frame. (#531)

### Fixed

- Fix BACnet/IP builds for BBMD clients without BBMD tables. (#523)
- Fix decoding empty array of complex type in RPM
- Fix device object ReinitializeDevice service handling examples of
  no-password in the device. (#518)
- Fix DeviceCommunicationControl service handling example of
  no-password in the device. (#518)
- Fix incorrect apdu_len calculation when encoding Recipient_List
  which had resulted in malformed APDU. (#517)
- Fix reinitializing a bacnet stack on windows by checking
  for valid socket before cleaning up WSA (#514)
- Fix a warning that 'device_id' is not used (#510)
- Added new linear.c to Microsoft Visual Studio project (#507)

## [1.3.1] - 2023-09-29

### Added

- Added example Channel object WriteProperty callback into example Device objects. (#504)
- Added Microsoft Visual Studio 2022 Community Edition solution to ports/win32 (#502)
- Added details in apps/blinkt example about starting app with systemd (#505)

### Fixed

- Refactored WriteProperty of object-name property rules into example device object (#504)

### Changed

- Changed WriteProperty string property checker to ignore length
  check with zero option.(#504)

## [1.3.0] - 2023-09-28

### Added

- Added [feature#14] EventTimeStamp decoding from ReadPropertyMultiple app. (#503)
- Added Channel, Color, Color Temperature, & Lighting Output demo
  app with Blinkt! (#503)
- Added pipeline build of piface and blinkt apps with Raspberry Pi
  OS image. (#503)
- Added linear interpolation library functions used in fading and ramping. (#503)

### Changed

- Added Device timer API to feed elapsed milliseconds to children
  objects. (#503)
- Changed gitignore to ease the maintainenance of source files in
  app folder. (#503)
- Changed example server app device simulator to use mstimer instead
  of OS time. (#503)
- Changed example channel object to be dynamically created or deleted. (#503)
- Changed example channel object to handle color & color temperature
  objects. (#503)

### Fixed

- Fixed datetime decode of invalid application tag. (#495)
- Fixed extraneous SO_BINDTODEVICE error message in Linux BIP. (#498)
- Fixed example Color, Color Temperature, and Lighting object fade,
  ramp, and step. (#503)
- Fixed and secured BACnetXYcolor and ColorCommand codecs. (#503)

## [1.2.0] - 2023-09-11

### Security

- secured decoders for BACnetTimeValue, BACnetHostNPort, BACnetTimeStamp,
BACnetAddress, and Weekly_Schedule and improved unit test code coverage. (#481)
- secured AtomicReadFile and AtomicWriteFile service decoders and
improved unit test code coverage. (#481)
- secured BACnet Error service decoder and improved unit test code coverage. (#481)
- fix ReinitializeDevice handler to clear password before decoding (#485) (#487)

### Added

- Added or updated the BACnet primitive value decoders named bacnet_x_decode(),
bacnet_x_application_decode() and bacnet_x_context_decode where x is one of
the 13 BACnet primitive value names. The decoders can accept a NULL data
value pointer when only the length is needed. (#481)
- Added bacnet_tag_decode() and BACNET_TAG data structure. (#481)
- Added improved unit test code for the primitive value decoders (#481)
- Added improved test code coverage for BACnet objects and properties. (#481)

### Changed

- Changed the insecure decoding API decoration to 'deprecated' which is defined
in src/bacnet/basic/sys/platform.h and can be disabled during a build. (#481)
- Changed some of the BACnet primitive value encoders so that all of them can
accept a NULL APDU buffer pointer when only the length is needed. (#481)

### Fixed

- Fixed missing Link_Speeds property in network port objects when
Link_Speed property is writable (#488)
- Fixed Microchip xmega xplained example project to build under GCC in pipeline.
- Fixed BACnet/IP on OS to bind broadcast to specific port (#489)
- Fixed (bug#83) mstpcap.exe permission denied in Wireshark (#492)
- Fixed wrong calculation of frame length in bacapp_data_len(). In cases
when 2 sets of opening & closing tags (first opening tag context tag is 3,
second is 0) were sent, one of the frames wasn't added, and the resulted
sum was wrong. Added unit test for case with two sets of opening & closing
tags. (#491)

## [1.1.2] - 2023-08-18

### Security

- Fix bacapp_data_len() + bacnet_tag_number_and_value_decode() (#453)
- Fix router-mstp and router-ipv6 apps action for unknown dnet (#454)
- Fix router-mstp app to p revent npdu_len from wrapping around at npdu_len=1 (#452)
- Fix router app where port might be null (#451)
- Fix [bug#80] npdu_decode via deprecation (#447)
- Fix [bug#79] out of bounds jump in h_apdu.c:apdu_handler (#446)

### Added

- Added github.com to sf.net github workflow for releases (#471)
- Added MSTP Slave Node option to stm32f10x port (#467)
- Added AFL + Libfuzzer harnesses  (#455)

### Changed

- Improve router-mstp app usage (#470)
- Updated zephyr to v3 4 0 in ci (#461) (#463)

### Fixed

- Fix encode_context_bacnet_address (#464)
- Fix west.yml imported repository set (#460) (#462)
- Fix spurious interrupts from STM32 ORE (#456)
- Fix writeproperty app known property option and priority (#450)
- Fix segfault on mstp cleanup on linux port (#445)
- Fix minimal config by adding bitstring (#443)
- Fix WhoIs app APDU timeout (#444)

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
