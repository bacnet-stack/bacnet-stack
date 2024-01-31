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

* Added more tool descriptions to bin/readme.txt file
* Added bacnet_npdu_encode_pdu API with additional size of PDU argument. (#549)

### Changed

* Changed npdu_encode function to return length when given a NULL buffer. (#549)
* Changed NPDU handler use local buffer which reduced TSM dependency. (#549)

### Fixed

* Fixed spelling in bacucov help message.
* Fixed COBS conversion for large MSTP data-not-expecting-reply frames. (#550)
* Fixed compilation with BACNET_SVC_SERVER=1 for client apps. (#552)

## [1.3.2] - 2023-12-21

### Security

* Secured bacdevobjpropref module decode buffer overflow reads (#541)
* Secured BACnet app decode function APDU over-read (#546) [bugs:#85]

### Added

* Added st-link install recipe for stm32f4xx port example
* Added defines for each supported BACAPP_TYPES_EXTRA (#543)
  to enable selective footprint reduction of BACNET_APPLICATION_DATA_VALUE
  struct (#411)
* Added MSTP extended frames transmit to src/datalink/mstp.c module
  and ports/stm32f4xx/dlmstp.c module (#531)
* Added MSTP extended frames receiving to src/datalink/mstp.c module
  used by mstpcap (#529)
* Added menu to release script (#506)

### Changed

* Changed SubscribeCOV Cancellations to always reply with Result+ (#526)
* Changed Who-Has to process when DCC initiation is disabled
* Changed mstp.c external API to remove rs485.h dependency
  for send frame. (#531)
* Changed ReadRange handling to remove RefIndx == 0 condition as BACnet
  norm allows this case (#539)
* Removed rx_fsm.c examples which duplicated MSTPCAP features (#544)
* Changed BACnet app decode function APDU size datatype to 32-bit (#546)
* Add WPM workaround for BTL Specified Test 9.23.2.X5 (#548)

### Fixed

* Fixed MSTP slave FSM for Data-Expecting-Reply frames (#538) 
* Removed duplicate cobs.c file in MSVC project (#535)
* Fixed CMake for code::blocks build (#533)
* Fixed BACnet/IP builds for BBMD clients without BBMD tables. (#523)
* Fixed decoding empty array of complex type in RPM
* Fixed device object ReinitializeDevice service handling examples of
  no-password in the device. (#518)
* Fixed DeviceCommunicationControl service handling example of
  no-password in the device. (#518)
* Fixed incorrect apdu_len calculation when encoding Recipient_List
  which had resulted in malformed APDU. (#517)
* Fixed reinitializing a bacnet stack on windows by checking
  for valid socket before cleaning up WSA (#514)
* Fixed a warning that 'device_id' is not used (#510)
* Fixed Microsoft Visual Studio project by adding new linear.c (#507)

## [1.3.1] - 2023-09-29

### Added

* Added example Channel object WriteProperty callback into example Device objects. (#504)
* Added Microsoft Visual Studio 2022 Community Edition solution to ports/win32 (#502)
* Added details in apps/blinkt example about starting app with systemd (#505)

### Fixed

* Refactored WriteProperty of object-name property rules into example device object (#504)

### Changed

* Changed WriteProperty string property checker to ignore length
  check with zero option.(#504)

## [1.3.0] - 2023-09-28

### Added

* Added [feature#14] EventTimeStamp decoding from ReadPropertyMultiple app. (#503)
* Added Channel, Color, Color Temperature, & Lighting Output demo
  app with Blinkt! (#503)
* Added pipeline build of piface and blinkt apps with Raspberry Pi
  OS image. (#503)
* Added linear interpolation library functions used in fading and ramping. (#503)

### Changed

* Added Device timer API to feed elapsed milliseconds to children
  objects. (#503)
* Changed gitignore to ease the maintainenance of source files in
  app folder. (#503)
* Changed example server app device simulator to use mstimer instead
  of OS time. (#503)
* Changed example channel object to be dynamically created or deleted. (#503)
* Changed example channel object to handle color & color temperature
  objects. (#503)

### Fixed

* Fixed datetime decode of invalid application tag. (#495)
* Fixed extraneous SO_BINDTODEVICE error message in Linux BIP. (#498)
* Fixed example Color, Color Temperature, and Lighting object fade,
  ramp, and step. (#503)
* Fixed and secured BACnetXYcolor and ColorCommand codecs. (#503)

## [1.2.0] - 2023-09-11

### Security

* secured decoders for BACnetTimeValue, BACnetHostNPort, BACnetTimeStamp,
BACnetAddress, and Weekly_Schedule and improved unit test code coverage. (#481)
* secured AtomicReadFile and AtomicWriteFile service decoders and
improved unit test code coverage. (#481)
* secured BACnet Error service decoder and improved unit test code coverage. (#481)
* secured ReinitializeDevice handler by clearing password before decoding (#485) (#487)

### Added

* Added or updated the BACnet primitive value decoders named bacnet_x_decode(),
bacnet_x_application_decode() and bacnet_x_context_decode where x is one of
the 13 BACnet primitive value names. The decoders can accept a NULL data
value pointer when only the length is needed. (#481)
* Added bacnet_tag_decode() and BACNET_TAG data structure. (#481)
* Added improved unit test code for the primitive value decoders (#481)
* Added improved test code coverage for BACnet objects and properties. (#481)

### Changed

* Changed the insecure decoding API decoration to 'deprecated' which is defined
in src/bacnet/basic/sys/platform.h and can be disabled during a build. (#481)
* Changed some of the BACnet primitive value encoders so that all of them can
accept a NULL APDU buffer pointer when only the length is needed. (#481)

### Fixed

* Fixed missing Link_Speeds property in network port objects when
Link_Speed property is writable (#488)
* Fixed Microchip xmega xplained example project to build under GCC in pipeline.
* Fixed BACnet/IP on OS to bind broadcast to specific port (#489)
* Fixed (bug#83) mstpcap.exe permission denied in Wireshark (#492)
* Fixed wrong calculation of frame length in bacapp_data_len(). In cases
when 2 sets of opening & closing tags (first opening tag context tag is 3,
second is 0) were sent, one of the frames wasn't added, and the resulted
sum was wrong. Added unit test for case with two sets of opening & closing
tags. (#491)

## [1.1.2] - 2023-08-18

### Security

* Fixed bacapp_data_len() + bacnet_tag_number_and_value_decode() (#453)
* Fixed router-mstp and router-ipv6 apps action for unknown dnet (#454)
* Fixed router-mstp app to p revent npdu_len from wrapping around at npdu_len=1 (#452)
* Fixed router app where port might be null (#451)
* Fixed [bug#80] npdu_decode via deprecation (#447)
* Fixed [bug#79] out of bounds jump in h_apdu.c:apdu_handler (#446)

### Added

* Added github.com to sf.net github workflow for releases (#471)
* Added MSTP Slave Node option to stm32f10x port (#467)
* Added AFL + Libfuzzer harnesses  (#455)

### Changed

* Improve router-mstp app usage (#470)
* Updated zephyr to v3 4 0 in ci (#461) (#463)

### Fixed

* Fixed encode_context_bacnet_address (#464)
* Fixed west.yml imported repository set (#460) (#462)
* Fixed spurious interrupts from STM32 ORE (#456)
* Fixed writeproperty app known property option and priority (#450)
* Fixed segfault on mstp cleanup on linux port (#445)
* Fixed minimal config by adding bitstring (#443)
* Fixed WhoIs app APDU timeout (#444)

## [1.1.1] - 2023-06-30

### Fixed

* Fixed BACnetARRAY encoder for index=ALL (#442)

## [1.1.0] - 2023-06-25

### Added

* Added minimim support for BACnet protocol-revision 0 through 24. See 
BACNET_PROTOCOL_REVISION define in bacdef.h
* Added example objects for color and color temperature, new for protocol-revision 24.
* Added current-command-priority to output objects revision 17 or later.
* Added demo apps ack-alarm, add-list-element, and event (#418)
* Added UTC option to BACnet TimeSync app (#396)
* Added --time and --date options to command line time sync for override (#215)
* Added --retry option for repeating Who-Is or I-Am C number of times (#199)
* Added write BDT application (#170)
* Added repeat option to Who-Is and I-Am app (#117)
* Added UTF8 multibyte to wide char printing in ReadProperty and ReadPropertyMultiple
apps if supported by the compiler and C library (#106).
- Added IPv6 Foreign Device Registration and IPv6 support for BSD and macOS for apps
* Added example of spawning 100 servers using a BBMD connection.
* Added COBS encode and decode from BACnet standard (#183)
* Added Schedule encoding/decoding (#319)* Add What-Is-Network-Number and Network-Number-Is services (#304)
* Added BACDL_CUSTOM datalink define which allows for externally linked datalink_xxx functions (#292)
* Added Zephyr OS integration (see zephyr folder)

### Changed

* Modified example objects to support CreateObject and DeleteObject API for
these objects: file, analog output, binary output, multi-state output, color,
color temperature).
* Unit tests and functional tests have been moved from the source C files
into their own test files under the test folder in a src/ mirrored folder
structure under test/. 
* The testing framework was moved from ctest to ztest, using CMake, CTest, and LCOV. 
It's now possible to visually view code coverage from the testing of each file
in the library. The tests are run in continuous integration.  Adding new tests
can be done by as copying an existing folder under test/ as a starting point, and
adding the new test folder name into the CMakeLists.txt under test/ folder, or
editing the existing test C file and extending or fixing the existing test. 
* Most (all?) of the primitive value encoders are now using a snprintf()
style API pattern, where passing NULL in the buffer parameter will
return the length that the buffer would use (i.e. returns the length that the
buffer needs to be).  This makes simple to write the parent encoders to check
the length versus their buffer before attempting the encoding at the expense of
an extra function call to get the length.
* BACnetARRAY property types now have a helper function to use for encoding (see bacnet_array_encode) 

## Deprecated

* Added BACNET_STACK_DEPRECATED for functions which will be removed in a future
release, with a suggested replacement function to use instead.

## Fixed

* Fixed WritePropertyMultiple decoding (#438)
* Fixed WhoIs app APDU timeout
* Fixed BACnetARRAY buffer access (#414) (#430) (#437)
* Fixed complex event type decoding (#420)
* Fixed VMAC address update for IPv6 (#429)
* Fixed compiler warnings for C89 builds (#433)
* Fixed win32 build warnings (#431)
* Fixed MSTP stat lost_token_counter that was missing a drop case (#421)
* Fixed and extend BACnetReliability per 135-2020 (#399)
* Fixed BACnet property operational-certificate-file enum value (#395)
* Fixed duplicate network port enums. Add BACnet/SC port types. (#388)
* Fixed the length for bad data crc in mstpcap (#393)
* Fixed decode_signed32() length for -8388608 and NULL apdu handling.
* Fixed gateway example Makefile build warnings (#380)
* Fixed gcc pedantic warnings and GNU89 GNU99 GNU11 and GNU17 warnings (#371)
* Fixed use of memmove instead of memcpy with overlapping data (#361)
* Fixed linkage conflict when use stack in C++ project (#360)
* Fixed ipv6 bind interface for link local comunication (#359)
* Fixed static timeGetTime. Make it similar to linux port (#358)
* Fixed export ucix_for_each_section_type function (#357)
* Fixed min data len for UNCONFIRMED_SERVICE_REQUEST from 3 to 2 (#356)
* Fixed uninitialized and unused variable compiler Warnings (#353)
* Fixed the Time stamp to print ISO 8601 format for event-time-stamps (#354)
* Fixed apps/router/Makefile add CFLAGS (#352)
* Fixed the install dll in bindir (#351)
* Fixed APDU missing services
* Fixed spelling errors
* Fixed missing getter for broadcast BIP socket (#350)
* Fixed RPM to return UNKNOWN_OBJECT for non-existent objects (#337)
* Fixed BACnet object text off-by-one.
* Fixed Notifications to initially be disabled.
* Fixed BACnet/IP Forwarded NPDU to FDT entries
* Fixed cppcheck warnings. Fix warnings found by splint (#324) (#250) (#244)
* Fixed cross compile by convert struct tm and time.h to datetime.h usage (#330)
* Fixed confirmed service with service supported enumeration
* Fixed the RP handler (#323)
* Fixed print property name and units for lighting objects (#313)
* Fixed BACnet/IP on OS to listen to broadcast on a separate socket (#293)
* Fixed segmentation error when file has capacity size (#300)
* Fixed the bacapp snprintf return size (#294)
* Fixed writepropertymultiple error handler (#289)
* Fixed ports stm32f4xx IAR project warnings (#268)
* Fixed BACnet application defines for STM32 IAR project (#267)
* Fixed missing bigend.c file to IAR project (#266)
* Fixed unreachable code in octetstring init ascii hex (#265)
* Fixed confirmed request service decode error (#259)
* Fixed BDT-1 port override and BIP NAT port number for apps (#252)
* Fixed MSTP to discard confirmed local broadcast (#248)
* Fixed file object TSM lookup PDU type comparison
* Fixed "Analog" cruft in Integer value object example (#243) (#242)
* Fixed double free rpm_data apps/readpropm (#241)
* Fixed MSTP transmitted frame and pdu counters. Add more stats (#236) (#235)
* Fixed bitstring capacity 8-bit size bug (#227)
* Fixed valgrind warnings in trendlog functions (#226)
* Fixed UCI compile errors (#220)
* Fixed UCI device_init from src to apps to be more maintainable (#219)
* Fixed network port object where BDT is required (#184)
* Fixed keylist signed integer wraparound (#179)
* Fixed FreeBSD 11 compile via gmake gcc (#180)
* Fixed data race issues on ring buffer for MSTP (#174)
* Fixed IPv6 MAC length to be 3 after public review (#169)
* Fixed Send_WhoIs_Local destination address (#167)
* Fixed DCC password out of range to reject (#166)
* Fixed MSTP skip data state to result in not-for-us (#165)
* Fixed the gateway example routing and lookups (#163)
* Fixed Issue #157 (Router segfaults) (#159)
* Fixed WPM for BTL * 9.23.2.X1 (#155)
* Fixed the encoding of network port number for BACnet IP (#147)
* Fixed confirmed APDU from original broadcast in BIP and BIP6 datalink to discard (#149)
* Fixed Bug#74 Printing of the Read property acknowledgement when APDU len is > 255 bytes (#148)
* Fixed RP and RPM Network Port for indefinite object instance (#146)
* Fixed Linux BIP with no default gateway (#144)
* Fixed Issue #129 compile of ports/linux/dlmstp_linux.h on Alpine Linux (#139)
* Fixed atomic readfile app when file reports zero length (#133)
* Fixed Bug#72 reading zero bytes in linux BIP (#132)
* Fixed #125 mstp.c source include (#126)
* Fixed mstpcap to ignore FF padding (#120)
* Fixed file object file size type (#116)
* Fixed confirmed ACK and simple ack callback without casting (#112)
* Fixed AVR ports IDE project builds (#111)
* Fixed BACDL_ETHERNET compilation (#108)
* Fixed comments, add sanity checks, and implicit type conversion warning (#102)
