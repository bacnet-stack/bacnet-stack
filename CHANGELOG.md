# BACnet Stack ChangeLog

BACnet open source protocol stack C library for embedded systems,
Linux, MacOS, BSD, and Windows

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

The git repositories are hosted at the following sites:
* https://bacnet.sourceforge.net/
* https://github.com/bacnet-stack/bacnet-stack/

## [Unreleased]

### Security
### Added
### Changed
### Fixed
### Removed

## [1.4.1] - Unreleased

### Security
* Secured SubscribeCOVProperty decoder. Changed datatype of monitoredProperty
  in struct BACnet_Subscribe_COV_Data. (#892)
* Secured the BACnet Who-Request decoder by changing deprecated decode
  functions. (#891)

### Added

* Added linked list of lighting-command notification callbacks. (#893)
* Added bvlc6.sh script to enable foreign-device-registration for
  client tools. (#889)
* Added check for zero length buffer size in primitive decoders
  that returns zero to enable simpler complex value optional
  element decoding. (#876)
* Added bitstring-bits-used-set API to use in audit-log status bits (#879)

### Fixed

* Fixed spelling errors detected by code-spell utility. (#895)
* Fixed cppcheck preprocessorErrorDirective. Suppressed new cppcheck
  warnings until fixed. (#895)
* Fixed all confirmed service handlers, except GetEventInformation, by sending
  a reject when confirmed services are received with zero length and
  required parameters are missing. (#885)
* Fixed the NDPU priority on confirmed messages to use requested NDPU
  priority. (#885)
* Fixed datalink environment for BIP6 foreign device registration for
  the example apps. (#884)
* Fixed basic Multistate Value and Input objects that were missing RELIABILITY
  in the optional list of properties. (#880)
* Fixed write present value in basic Analog Value object when the type
  is not valid, don't overwrite error code. (#881)
* Fixed DeviceCommunicationControl error code when no service request. (#877)
* Fixed CMakeLists routing option. If BAC_ROUTING was turned off,
  the gateway app wasn't deselected. (#874)
* Fixed use of 'class' keyword as a variable in BACnet/SC by removing. (#872)
* Fixed bacdcode.c to allow writing an empty CharacterString (#871)
* Fixed defects found when enabling style and CERT-C addon for CPPCHECK
  with some suppressions. (#869)
* Fixed DeviceCommunicationControl service handler to return Service
  Request Denied when the DISABLE parameter is given in protocol-revision 20
  or higher builds. (#867)
* Fixed dlmstp ringbuffer initialization corruption. (#865)
* Fixed typo in msv.c Object_Name. (#864)
* Fixed missing BitString_Value_Create and BitString_Value_Delete in
  device.c Object_Table. (#863)
* Fixed ability to compile with BACNET_PROTOCOL_REVISION<17. (#862)
* Fixed days.c module epoch functions and added datetime module fed
  by mstimer clock which includes daylight savings time and clock API
  with TimeSynchronization integration.
* Fixed missing Time-Of-Device-Restart property in the basic device
  object. (#860)
* Fixed compiler workarounds by removing non-standand C functions
  strcasecmp and strncasecmp. (#858)
* Fixed libc compiler differences by adding bacnet_strnlen and
  bacnet_stricmp functions. (#857)
* Fixed hardcoded NORMAL event state with the actual object event. (#853)
* Fixed compiler warning in write-group module constant. (#856)
* Fixed lighting command operations by refactoring from the lighting
  output object and adding unit testing. (#855)
* Fixed Analog Input and Value object Event_Detection_Enable. (#854)
* Fixed ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY condition in basic
  Notification Class object WriteProperty handler. (#852)
* Fixed Systick Interrupt in ports that should never be activated using
  NVIC_EnableIRQ because it's an system exception. (#850)
* Fixed missing DLMSTP start, valid, and invalid frame complete callbacks
  accidentally removed in common DLMSTP module. (#848)
* Fixed compiler warnings by removing extraneous structure dereferences.
* Fixed doxygen build for newer doxygen. (#845)

### Changed

* Change the basic CharacterString Value object by adding CreateObject and
  DeleteObject service support. (#888) (#886)
* Changed some debug.c module functions to macros to be able to use them
  in code without having to add #ifdef around them in the code. This
  enables smaller non-printing embedded builds to use the same handlers
  as the example OS based applications. Refactored errno use in service
  using debug_perror. Changed debug_perror usage to debug_fprintf.
  Renamed debug_aprintf to debug_printf_stdout for clarity.
  Converted most debug_fprintf usage to debug_print to reduce text
  bloat in AVR build. (#885)

## [1.4.0] - 2024-11-04

### Security

* Secured BACnetAccessRule encoding and decoding and added unit testing.
  Fixed BACnetAccessRule application integration into Access Rights object.
  Improved unit testing and code coverage. (#790)
* Secured Active-COV-Subscriptions property encoding. (#763)
* Secured COV handling after refactor to using local buffer. (#802)

### Added

* Added BACnet Secure Connect datalink. Added BACnet/SC hub application. (#818)
* Added WriteGroup service to blinkt app demo. (#843)
* Added blinkt and piface build in gcc pipeline to prevent bit rot. (#842)
* Added missing MS/TP zero configuration preferred address API and usage. (#840)
* Added unit testing for FIFO peek feature. (#839)
* Added IPv6 Zone Index function to return ASCII. (#838)
* Added network port callbacks for pending changes activate and discard. (#836)
* Added WriteProperty setter for BACnet Unsigned Integer values. (#834)
* Added linker specific code for Darwin for compiling mstpcrc. (#833)
* Added WriteGroup service and Channel object interfaces. (#829)
* Added common property test for BACnetARRAY. (#828)
* Added code to parse BACnetAccessRule from ASCII command line. (#827)
* Added a common BACnetARRAY write property iterator function. (#826)
* Added cmake build artificats to gitignore.
* Added some optional properties into the object property lists up to
  protocol-revision 22. (#789)
* Added serial-number property to basic device object example. (#789)
* Added optional description property to basic network port object
  example. (#788)
* Added ucix_get_list and ucix_set_list function. (#780)
* Added uci include and lib for cmake. (#779)
* Added BACnet Ethernet support for MacOS X via pcap library. (#775)
* Added local tool aptfile to use with apt based development
  installation. (#772)
* Added MS/TP support for MacOS X. (#764)
* Added pre-commit clang-format 17 format check in pipeline. Formatted
  entire code base except external/ folders. (#755)
* Added RP and RPM error code indications in callbacks in bac-rw.c
  module handling. (#756)
* Added editorconfig, a widely used and supported file in profession IDEs.
  The configuration file indicates how files should be formatted. (#753)
* Added pre-commit configuration which can automatically check some basic
  code formatting rules everytime someone makes commit. This can also be
  used in the pipeline during a pull-request for automated checking.
  pre-commit can also be used in a local workspace or within an IDE.
  pre-commit makes it easy to select clang-format version so everyone is
  using same one.
* Added basic object-name get for ASCII names to enable free if they were
  dynamically created. Added unit testing to validate the basic object
  ASCII object-name API. (#754)

### Changed

* Changed the lint apt install in quality pipeline to be consistent (#837)
* Changed apps/epics by enabling BACnetARRAY checking. (#823)
* Changed  BACnet Ethernet and IPv6 on win32 by switching from
  WinPcap to npcap.  Included npcap SDK in cmake and libs for
  IPv6 in cmake. (#774)
* Changed pipeline gcc.yml to reduce known warnings from legacy
  API by adding LEGACY flag to make bip-apps with GNU99. (#795)
* Changed ATmega168 example for ATmega328 on Arduino Uno R3 with DFR0259
  RS485 shield. Added ADC interface from BDK port and mapped to some AV
  objects. Removed MS/TP MAC address DIP switch GPIO and moved MS/TP
  configuration to AV objects. Added AV units property. Added some
  Uno R3 Digital Inputs and outputs mapped to some BV. Added AVR EEPROM
  from BDK port and mapped some non-volatile data
  including MAC address and max manager and baud rate,
  device ID and names and description and location. (#784)
* Changed the datalink abstraction to enable selecting multiple datalinks
  using BACDL_MULTIPLE and one or more other BACDL defines. (#717)
* Moved west manifest, zephyr folder, and ports/zephyr folders to
  another repository https://github.com/bacnet-stack/bacnet-stack-zephyr
  so that the rapid pace of Zephyr OS development changes will have
  a less impact on the development of the BACnet Stack library. (#757)
* Removed static scope on character array used for object-name since the array
  gets copied into characterstring array and static is not needed. (#754)
* Changed clang-format rules to always align const to left side,
  and insertbraces to avoid having to use clang-tidy. (#753)
* Moved external files, such as driver libraries, to external/ folders.
  It is lot easier to work with automatic formatters if we have external
  files in different folder. For some tools we can just exclude
  external folder all together. (#744)
* Changed most of the functions use of pointer parameters to const.
  Used clang-tidy and sonarlint to help find places where const could be
  easily applied. (#714)
* Changed bacapp_snprintf_value() to be const correct. After that
  we got a warning that the 4th function call argument is an uninitialized
  value in bacapp_snprintf_weeklyschedule(). Fixed the warning by setting
  array_index to zero. (#714)

### Fixed

* Fixed MS/TP zero-config preferred-station setting to not filter getter. (#844)
* Fixed MS/TP module flush stderr compile error from leftover. (#844)
* Fixed device object compile errors and added IPv6 recipes for blinkt
  and piface (#841)
* Fixed BACnet basic file object to have dynamic name, mime-type, path. (#835)
* Fixed MS/TP Slave Node FSM to enable receiving DNER frames. (#832)
* Fixed BACnetLightingCommand decode options by setting them. (#830)
* Fixed jquery.js used for syntax highlighting in PERL documentation. (#817)
* Fixed EXC_BAD_ACCESS in datalink_set() strcasecmp(). (#816)
* Fixed BACNET_APPLICATION_DATA_VALUE declarations to be initialized so
  that the next pointer is NULL by default. (#814)
* Fixed mstpcap utility by setting This-Station to the MS/TP promiscuous
  mode address 255. Fixed MS/TP receive filter for valid data frames which
  was missing. Fixed MS/TP zero-config duplicate node detection. Reset
  silence during MS/TP capture after writing packet to prevent endless
  packets when stream is interrupted. (#812)
* Fixed the Linux MSTP turnaround delay implementation. (#809)
* Fixed app/router compiled with optimize Os when the program exits with
  bit out of range 0 - FD_SETSIZE on fd_set. Solution disables optimize
  for size by static set optimize 2 for GCC. Fixes (#793) (#808)
* Fixed app/router-ipv6 duplicate symbol by using Device_Object_Instance_Number
  from device-client.c module. Fixes (#778). (#806) (#807)
* Fixed bsd/bip6.c:35:16: error: variadic macros are a C99 feature (#805)
* Fixed MS/TP FSM TS (this station) filter that was removed for zero-config
  permiscuous feature. (#803)
* Fixed app router-ipv6 cmake (#800)
* Fixed app router cmake (#799)
* Fixed IP_DHCP_Enable property being present in Network Port object
  by adding compiler define. (#796)
* Fixed app/router in cmake recipe. (#794)
* Fixed mismatched comments in netport.c module. (#792)
* Fixed app/server when compiled with BAC_ROUTING. (#783)
* Fixed a warning emitted from arm-none-eabi-gcc in hostnport.c (#785)
* Fixed duplicated C file in CMakeLists.txt (#781)
* Fixed cmake dependencies to build readbdt, readfdt and router-ipv6
  if BACDL_BIP=OFF. (#777)
* Fixed compiler warning in ports/bsd/rs485.c module. (#771)
* Fixed UTF-8 passwords for DeviceCommunicationControl to hold up to
  20 UTF-8 characters. (#767)
* Fixed "types-limits" compiler warnings. (#766)
* Fixed variable data type for boolean in RPM structure. Fixed RPM error
  handling to use callback. Fixed bacrpm app example when not enough
  command line parameters are used. Fixed empty-list EPICS printing.
  Fixed RPM-Ack processing for end of list-of-results. Added minimal
  handling for segmentation-not-supported during RPM of object
  properties. (#765)
* Fixed the order of operations in SubscribeCOV so the dest_index gets written
  to the correct slot instead of an initial MAX_COV_SUBSCRIPTIONS-1. (#761)
* Fixed some spelling typos in comments. (#762)
* Fixed COV detection in the basic Binary Output object example. (#751)

## [1.3.8] - 2024-08-26

### Security

* Secured and refactored BACnetActionCommand codec into bacaction.c module
  for command object and added to bacapp module encode/decode with define
  for enabling and pseudo application tag for internal use. (#702)
* Secured ReadProperty-REQUEST and -ACK decoding. (#702)

### Added

* Added BACDL_ETHERNET build in the pipeline for MacOS Linux and Windows. (#822)
* Added apps/router to ports/bsd by sharing API with ports/linux. (#821)
* Added missing router-mstp in cmake (#820)
* Added shield option explanation to ports/stm32f4xx/README.md (#749)
* Added API for intrinsic reporting properties in Binary Value and Binary
  Input objects (#742)
* Added load control object into zephyr basic device example (#739)
* Added clauses c) and f) of 13.3.6 (out_of_range) algorithm and enabling
  transitions from high/low limit states to normal when Event_Enable = 0 for
  the basic Analog Value and Analog Input objects (#733)
* Added mstpcap to apps/Makefile BSD build (#730)
* Added prototype for device object property list member to use for
  storing device data storing. (#735)
* Added device WriteProperty callback for non-volatile storing in basic
  device examples. (#728)
* Added CreateObject, DeleteObject, and COV to Integer Value object (#719)
* Added CreateObject and DeleteObject to basic Load Control object. (#713)
* Added Exception_Schedule property to schedule object example. (#709)
* Added CreateObject and DeleteObject to basic device object table for
  calendar object.
* Added alaternate define for BACNET_NPDU_DATA as BACNET_NPCI_DATA.
* Added mstpcap app build for MacOS. (#705)
* Defined INT_MAX when it is not already defined by compiler or libc. (#702)
* Added BACnetScale to bacapp module. Improved complex property value decoding.
  Refactored bacapp_decode_known_property() function. (#702)
* Added ASHRAE 135-2020cn engineering units. (#703)
* Added bacnet_strnlen and bacnet_stricmp to avoid libc compiler problems
* Added Zephyr settings subsys to enable storing of BACnet values according
  to BACnet object property value path. (#697)
* Added BACnet Basic features to enable basic samples.
  Refactored the zephyr BACnet profile B-SS sample to use BACnet basic
  subsys.(#697)
* Added BDT-Entry and FDT-Entry to BACapp for known property
  encoding/decoding. (#700)
* Added reject response to unknown reserved network layer message types. (#690)
* Added a check for apdu_len exceeding MAX_APDU in apdu_handler()
  for confirmed service and ignore the message if the APDU portion of the
  message is too long. (#696)
* Added BACnet/IPv6 properties to basic Network port object (#686)
* Added extra objects to STM32F4xx example to elicit edge cases in
  object-list for testing. (#683)

### Changed

* Stripped tabs and trailing white spaces, and fixed end of files (#748)
* Changed clang-format to ignore javascript files and ignore some tables
  in C/H source files. Replaced unicode characters found in the source code.
  Changed permissions of shell scripts - with shebang - to be executable. (#747)
* Refactored required writable property function from epics app (#743)
* Include more more code in pipeline for clean code builds. (#725)
* Changed CMake in pipeline to force C89/C90 compatible and for test C99 (#722)
* Added some new compiler warnings to help with clean code, and removed
  some compiler warning ingores as they do not produce any warnings.
  Added -Werror flag only in the pipeline build to keep the build clean. (#718)
* Updated B-SS profile sample build for Zephyr OS. Added bacnet-basic
  callback in zephyr subsys to include init and task in same thread. (#711)
* Simplified bacapp_data_len() and moved into bacdcode module
  as bacnet_enclosed_data_len() function. (#702)
* Refactored and improved the bacapp_snprintf() function for printing
  EPICS.(#702)
* Changed many header file include guards to unique namespace.
  Updated many file headers comments with SPDX [issue #55].
  Added all the currently used license files to the license/ folder
  so that the license is included with the source. (#666) (#716)
* Added set time callback for BACnet TimeSynchronization services. (#691)
* Reduced MS/TP MAX_APDU to use 480 by default in examples from 1476 so that
  devices do not use the new MS/TP extended frame types which older routers
  do not understand. (#683)

### Fixed

* Fixed MacOS specific usage during FreeBSD 11.4 build. (#745)
* Fixed compiler warnings with MSVC /Wall in C89/C90 builds (#740)
* Fixed compiler warnings from variadic macros in C89/C90 builds, and
  changed self-assigns to void casts. (#737)
* Fixed the length of the basic Network Port object MAC address property. (#741)
* Fixed SubscribeCOV to report an error: resources, no-space-to-add-list-element
  on reaching MAX_COV_ADDRESSES limit with COV subsriptions. (#734)
* Fixed zephyr BACnet/IP for use in native_posix. Fixed zephyr logging
  level for BACnet. (#738)
* Fixed endless query in bac-rw module when error is returned. (#727)
* Fixed Lighting Output object lighting command decoding and ramp operations
* Fixed network port warning for unused static function. (#712)
* Fixed BACnetPriorityArray decoding in bacapp module. (#712)
* Fixed epics print of BACnetDateTime complex data. (#712)
* Fixed typo in command line argument check of example app for BACnet
  Network-Number-Is. (#707)
* Fixed Lighting Output WriteProperty to handle known property decoding. (#702)
* Fixed BACnetHostNPort known property decoding. (#700)
* Fixed MS/TP that was not working in ports/win32 (#694)
* Fixed the common DLMSTP module destination address to use the destination
  in the request instead of zero (copy/pasta error). (#693)
* Fixed network priority reponses for test 10.1.2. (#687)
* Fixed basic device object and ReadRange handling for test 9.21.2.2 and
  9.21.2.3 array index. (#692)
* Fixed Object type list length for protocol-revision 24. (#684)

### Removed

* Removed deprecated Keylist_Key() functions from usage. (#702)
* Removed pseudo application datatypes from bacapp_data_decode() which only
  uses primitive application tag encoded values. (#702)
* Deprecated bacapp_decode_application_data_len() and
  bacapp_decode_context_data_len() as they are no longer used in any code
  in the library.(#702)


## [1.3.7] - 2024-06-26

### Security

* Secured ReadPropertyMultiple code, and improved unit test coverage. (#650)
* Secured BACnetTimeValue codec, and improved unit test coverage. (#648)
* Secured BACnetAcknowledgeAlarmInfo codec and improved unit testing code
  coverage. (#647)
* Secured APDU handler by avoiding read ahead. (#645)

### Added

* Added context to MS/TP user data to enable additional
  user data. (#676)
* Added activate-changes to the ReinitializeDevice options. (#674)
* Added example basic bitstring value object. (#668)
* Added floating point compares in cases where they don't exist in math
  library. (#665)
* Added memap, avstack, and checkstackusage tools to STM32F4xx and STM32F19x
  ports example Makefile and CMake builds to calculate CSTACK depth and RAM
  usage. Added .nm and .su to .gitignore to skip the analysis file
  residue. (#661)
* Added cmake to STM32F10x port example, using the common datalink
  dlmstp.c module with MS/TP extended frames and zero-config support. (#661)
* Added existing BBMD unit test to coverage by converting to cmake (#657)
* Added BACAPP Kconfig options for Zephyr OS builds. (#655)
* Added simpler API to get/set Network Port MSTP MAC address (#653)
* Added git mail map to consolodate and decode commit names (#652)
* Added secure BACnet primitive datatype encode functions. (#643)
* Added function to determine if an object property is a BACnetARRAY.
  Added property test for BACnetARRAY members. (#642)
* Added basic structured view object and unit test. Added example structured
  view into server example. (#641)
* Added reliability property to basic analog-value. (#639)

### Changed

* Changed MS/TP master node self destination checks to be
  located in receive FSM. Changed MSTP zero configuration: modified
  comments for state transition names; modified next station increment;
  refactored the UUID rand() to not be required by common
  zero config implementation; added more unit tests. (#676)
* Refactored ports/xplained to use common DLMSTP module to enable extended
  frames (#665)
* Refactored snprintf common subsequent shift usage into a function. (#656)
* Changed config.h to default to client-server apps (#651)
* Cleaned up code of BACnetWeeklySchedule (#646)

### Fixed

* Fixed typos in ai.c and ao.c basic object examples (#673)
* Fixed datatype conversion errors found by splint.
  Fixed Binary input/value set. (#672)
* Fixed wildcard check in create object for Binary Input objects. (#663)
* Fixed memory leaks in create object initialization if repeated. (#664)(#662)
* Fixed the Zephyr-OS BIP6 datalink module. (#659)
* Fixed redundant GCC compiler flags in ARM, OS, and test builds, and made them
  more consistent across various builds. (#658)
* Fixed redundant redeclaration of various functions detected by change
  in compiler flags. (#658)
* Fixed string truncation warning in bip-init detected by change
  in compiler flags. (#658)
* Fixed some set-but-not-used variables by creating stub functions
  instead of using macros. (#658)
* Fixed RPM compiler warning. (#654)
* Fixed basic analog-value object intrinsic reporting for ack
  notification. (#640)
* Fixed basic analog-value object write property of present-value to
  priority 6. (#640)
* Fixed basic analog-value alarm-ack functionality. (#639)


### Removed

* Removed local dlmstp.c module from stm32f10x port in favor of using
  the common src/bacnet/datalink for easier maintenance. (#661)
* Removed creation of objects from basic device object into the server
  example. (#641)

## [1.3.6] - 2024-05-12

### Security

* Fixed bacapp snprintf to account for string size zero behavior of snprintf.
* Changed all the sprintf to use snprintf instead. (#628)

### Added

* Added Get/Set functions in the basic notification class object to support
  properties relative permanence requirement. (#629)
* Added help text to explain how to decode complex data in the WriteProperty
  example app. (#627)
* Added host_n_port_context_decode function.
* Added timestamp & datetime snprintf ASCII function.
* Added required linux Ethernet library for ethernet build. (#620)
* Added .obj to gitignore. (#620)
* Added create-object and delete-object recipes in GCC Makefile. (#620)
* Added datalink timer to all example OS apps. (#620)
* Added writefile API to basic file object example. (#620)
* Added API to device-client to make it more robust. (#620)
* Added API in network-port object for getting the ASCII object-name. (#620)
* Added debug print with a timestamp option. (#620)
* Added debug print with hex dump print. (#620)
* Added API to network port object for activate and discard. (#620)
* Added default define for debug with timestamp. (#620)
* Added prototype in header for disabled debug printf. (#620)
* Added fifo peek ahead function to peek at more than one byte. (#620)
* Added get-mac value for network port that uses buffer rather than
  octetstring. (#620)
* Added API for basic multistate objects number-of-states.
* Added reliability, active-text, inactive-text to basic binary-input object.
* Added reliability property to basic binary-value object.
* Added API for setting multi state text with null-terminated name lists
  in basic objects. (#614)
* Added Create/Delete object services to Analog Input, Analog Value,
  Binary Input, Binary Value, Multistate Input, Multistate Value basic
  object examples, and updated their units tests. (#612)

### Changed

* Changed clang-format to include AlignAfterOpenBracket: AlwaysBreak and
  BinPackArguments: true. Used make pretty-test to reformat the test/bacnet
  .c/.h files with the updated format.
* Changed most microcontroller ports to use BACAPP_MINIMAL to specify
  which datatypes can be written. (#620)
* Changed format in CMake to enable cleaner SC merge. (#620)
* Changed the first instance of a basic integer value object from 1 to 0. (#619)
* Changed basic time-value object present-value to be decoupled from
current time, and changed out-of-service property to be writable.

### Fixed

* Fixed nuisance print messages in ports/linux/dlmstp by changing
  to debug print only. (#633)
* Fixed compile warnings in basic objects. (#630)
* Added API for setting multi state text with null-terminated name lists
  in basic objects. (#630)
* Fixed example app router-ipv6 to build under ports/win32. (#630)
* Fixed example app router-mstp to build under ports/win32 with MinGW. (#630)
* Fixed invalid comparison in life-safety-zone basic object.
* Fixed CMake build for BDT and FDT to only apply to BIP and BIP6
* Fixed basic notification class object logic behind valid transitions. (#623)
* Fixed export build that uses rpm_ack_object_property_process(). (#622)
* Fixed zephyr bip_get_addr endian UDP port number
* Fixed BACnet port for APPLE to use BSD in CMake. (#620)
* Fixed zephyr OS for BACnet/IP warning. (#620)
* Fixed zephyr OS log to not require log_strdup. (#620)
* Fixed UDP port endian for zephyr os BACnet/IP
* Fixed basic network port object header dependency on readrange.h file
* Fixed basic binary object active and inactive text setting.
* Fixed unit test checking for unknown property in basic objects.
* Fixed example apps to enable binding to device instance 4194303. (#615)
* Fixed compile warnings in basic objects. (#614)
* Fixed life safety zone default object name. (#613)

## [1.3.5] - 2024-04-01

### Security

* Secured the WPM and RPM client service encoders. (#604)

### Added

* Added test for unsupported property to common property test.(#609)
* Added a few core stack headers as includes into bacdef.h file.(#602)
* Added Kconfig and bacnet-config.h options in ports/zephyr to keep small
  footprint for MCUs having less RAM.(#606)
* Added Keylist_Data_Free function to free all nodes and data in a list.(#595)
* Added basic Life Safety Zone object type in the apps/server example,
  with unit testing.(#595)
* Added extended frame client unit test.(#592)

### Changed

* Changed property lists member function for WriteProperty default case
  by refactoring.(#609)
* Changed time-value object unit testing by refactoring.(#609)
* Changed ports/zephry for BACnet/IP and date-time with latest
  Zephyr OS.(#606)
* Changed Zephyr OS west manifest to target zephyr v3.6.0.(#601)
* Changed ZTEST_NEW_API adjustments as deprecated.(#601)
* Changed position of bacnet/bacdef.h to be the first bacnet header
  to include. BACnet headers need to pull in optional configuration and
  optional ecosystem overrides to allow integrators to control
  builds.  This change places bacnet/bacdef.h to the top of the BACnet
  Stack header files to consistently introduce integrator and ports
  header files.(#598)(#600)
* Added dependent BACnet stack headers into bacdef.h file.(#602)
* Changed bacdef.h and other stack includes in c/h files to have a
  common pattern.(#602)
* Moved bits.h, bytes.h, and bacnet_stack_exports.h under
  bacnet/basic/sys/ folder.(#602)

### Fixed

* Fix double promotion in format specifier %f by casting floats
  to double.(#608)
* Fixed the implementation of object-instance and object index
  differentiation object/basic/ai.c module.(#607)
* Fixed RPM and WPM apps when fail to encode request.(#604)
* Fixed WPM app number of arguments checking.(#604)
* Fixed routing to a remote network in the router-mstp example.(#592)
* Fixed handling of received MS/TP extended frames.(#592)
* Fixed MSTP_Master_Node_FSM and MSTP_Slave_Node_FSM for extended frames.(#592)
* Fixed MSTP COBS frame encoding.(#592)
* Fixed router-ipv6 application for remote networks.(#592)

### Removed

* Removed BACnet objects from ports/zephyr. There should only be datalink
  and OS related interfaces in OS ports.(#606)

## [1.3.4] - 2024-03-02

### Security

* Secured bacapp_decode_application_data_safe(),bacapp_decode_application_data_len(),
  bacapp_decode_context_data(), bacapp_decode_known_property() for timestamp,
  bacapp_decode_context_data_len(), and bacapp_data_len() functions. (#578)

### Added

* Added SHIELD=dfr0259 or SHIELD=linksprite build options to RS485
  driver for stm32f4xx port.
* Added FAQ 18 for firewall info (#587)
* Added a BASH script for parallel EPICS clients registering as foreign devices
  to a BBMD (#586)
* Added an example application - bacdiscover - to discover devices and their
  objects and properties on a specific destination network. The application
  uses a BACnet Discovery FSM module along with the BACnet R/W FSM.
  The BACnet Discovery module stores the binary property data in Keylists
  and includes device object property queries and iterators. (#583)
* Added callback from BACnet R/W FSM module for I-Am messages. (#583)
* Added an example application - bacapdu - to send an arbitrary
  APDU string of hex-ASCII. (#580)
* Added a clean target recipe in apps to remove stale libbacnet.a file.
* Added missing binary input functions to set custom object names. (#574)
* Added Keylist_Index_Key to deprecate Keylist_Key function in
  keylist module. Added unit test for Keylist_Index_Key API. Changed
  modules using Keylist_Key. Changed keylist module to use bool and
  stdint value for key not found. (#572)
* Added missing object functions to analog inputs and values. (#568)

### Changed

* Changed BACnet R/W FSM module to remove dependency on
  rpm_ack_decode_service_request() which uses large calloc/free
  value lists. Created an alternate RPM-ACK to RP-ACK processing
  function. (#583)
* Changed basic RPM handler to skip over unknown property values. (#583)
* Changed the release script to use tag option and remove tag reminder.
  Fixed release tool to have a prefix folder with the tag version name.
  Enabled rebuilding tagged release code and zips in a working tree in temp folder.

### Fixed

* Fixed makefile for building the Linux router application. (#585)
* Fixed Command, Credential Data Input, and Schedule objects unit tests. (#578)
* Fixed apps/Makefile to use apps/lib/libbacnet.a library file instead of
  a library file in system /usr/lib folder.
* Fixed example Analog Output to set proper bounds for
  Analog_Output_Present_Value_Set and Analog_Output_Present_Value_Write. (#575)
* Fixed the Network Port object subnet mask for IP example. (#573)
* Fixed bacnet_address_init() when setting only the dnet value. (#570)
* Fixed MSVC snprintf from C99 in platform.h file. (#570)

### Removed

* Removed the ARM math library binary files in the stm32f4xx port

## [1.3.3] - 2024-02-2

### Security

* Secured the following services by refactoring the size check
  and refactoring the service requests from the service header,
  adding APDU size checking and length features, and adding unit
  tests to check for length when passing NULL buffer:
  ARF/AWF/COV/CO/DO/DCC/Event/GE/ALE/RLE/LSO/RD/RR/RP/WP. (#553)

### Added

* Added bacapp decoding for accumulator SCALE property. (#566)
* Added a MS/TP zero-config (automatically choose an unused MAC address)
  using an algorithm that starts with MAC=64 and waits for a random number
  of PFM (minimum of 8 plus modulo 64) before attempting to choose a MAC
  sequentially from 64..127. The confirmation uses a 128-bit UUID with the
  MSTP Test Request frame. The modifications are in src/bacnet/datalink/mstp.c
  and src/bacnet/datalink/dlmstp.c modules enabling any device to use
  zero-config if enabled. A working demonstration is in the ports/stm32f4xx
  for the NUCLEO board. Complete unit testing is included.  Options include
  lurking forever (wait for a router or another master node before joining)
  or lurking for a minimum time (enables self forming automatic MAC addressing
  device nodes). (#564)
* Added basic Calendar object, unit tests, and integration with
  example device object. (#440)
* Added basic Time Value object, unit tests, and integration with
  example device object. (#440)
* Added the SpecialEvent struct for the Exception_Schedule property
  of Schedule, encode/decode/same functions, unit tests, and integrated
  into bacapp functions. (#474)
* Added the CalendarEntry struct for the Date_List property of Calendar
  and the SpecialEvent struct, encode/decode functions, unit tests, and
  integrated into bacapp functions. (#474)
* Added the DateRange struct for the Effective_Period property of Schedule,
  encode/decode functions, unit tests, and integrated into bacapp
  functions. (#474)
* Added Binary Lighting Output object example. (#522)
* Added more unit testing for device object.  (#522)
* Added CMake to AT91SAM7S port example. (#560)
* Added ports AT91SAM7S and STM32F4xx CMake builds into pipeline. (#560)
* Added openocd debug launcher under vscode in STM32F4xx example. (#559)
* Added generic property list member checking for write property members
  of network port object in STM32F4xx example. (#559)
* Added a new src/bacnet/datalink/dlmstp.c module as a reference.
  Integrated the new dlmstp.c into the STM32F4xx example port. (#559)
* Added CMake build option for stm32f4xx. Added fixes and comments to
  stm32f4xx example device.c from bacnet/basic/object/device.c module. (#556)
* Added apdu size checking on BACnetPropertyStates decode. Added more
  BACnetPropertyStates codec unit test coverage. Added more enumerations
  to BACNET_PROPERTY_STATES aka struct BACnetPropertyStates.
* Added more tool descriptions to bin/readme.txt file.
* Added McCabe complexity generation to Makefile.
* Added bacnet_npdu_encode_pdu API with additional size of PDU argument. (#549)

### Changed

* Changed piface example app to support binary-lighting-output object type
  and blink warn. (#522)
* Changed example device object to not create objects
  when device object-table is overridden  (#522)
* Changed example STM32F4xx DLMSTP module to use core MSTP FSM
* Changed automac module from ports into bacnet/datalink and added a unit
  test. (#557)
* Changed Life-Safety-Point object to use Create/Delete-Object (#555)
* Changed npdu_encode function to return length when given a NULL buffer. (#549)
* Changed NPDU handler use local buffer which reduced TSM dependency. (#549)

### Fixed

* Fixed CMakeLists.txt: to exclude h_routed_npdu.c when BAC_ROUTING=OFF. (#562)
* Fixed BACNET_STACK_EXPORT macro to Analog_Output_Read_Property function. (#561)
* Fixed build on FreeBSD. (#554)
* Fixed spelling in bacucov help message.
* Fixed COBS conversion for large MSTP data-not-expecting-reply frames. (#550)
* Fixed compilation with BACNET_SVC_SERVER=1 for client apps. (#552)

### Removed

* Removed BACnetPropertyStates local enumeration BACNET_PROPERTY_STATE_TYPE.

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

### Removed

* Removed linux/rx_fsm.c example which duplicated MSTPCAP features (#544)

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
