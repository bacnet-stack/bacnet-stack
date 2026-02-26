# BACnet Stack ChangeLog

BACnet open source protocol stack C library for embedded systems,
microcontrollers, Linux, macOS, BSD, and Windows

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

The git repositories are hosted at the following sites:
* https://bacnet.sourceforge.net/
* https://github.com/bacnet-stack/bacnet-stack/

## [Unreleased] - 2026-02-23

### Security

* Secured various BACnet type and service request decoders by replacing
  deprecated decoders. Secured ReadProperty-Ack decoder, Trend Log object
  TL_fetch_property() function, WritePropertyMultiple service decoders,
  TimeSynchronization-Request decoder, ReadPropertyMultiple-Request and
  ReadPropertyMultiple-Ack decoders, ConfirmedPrivateTransfer-Request
  and UnconfirmedPrivateTransfer-Request decoders, Add/Remove ListElement
  service request decoder, I-Have-Request service decoder, BACnetEventState
  change-of-state decoder, BACnetCredentialAuthenticationFactor decoder,
  BACnetPropertyState decoder, BACnetAssignedAccessRights decoder,
  LifeSafetyOperation-Request service decoder, BACnetAuthenticationFactor
  decoder in the Crediential Data Input object. (#1244)
* Secured BVLC decoder by replacing deprecated primitive decoder usage. (#1241)
* Secured decoding length underflow in wp_decode_service_request() and
  bacnet_action_command_decode() which had similar issue. (#1231)
* Secured Schedule_Weekly_Schedule_Set() the example schedule object
  by fixing stack buffer overflow. The memcpy was using
  sizeof(BACNET_WEEKLY_SCHEDULE) instead of sizeof(BACNET_DAILY_SCHEDULE),
  causing it to read 6784 bytes from a 968-byte source buffer, leading
  to stack buffer overflow and segmentation fault in the test_schedule
  unit test. (#1222)
* Secured npdu_is_expected_reply() function where the MS/TP reply matcher
  could have an out-of-bounds read. (#1178)
* Secured ubasic interpreter tokenizer_string() and tokenizer_label()
  off-by-one buffer overflow when processing string literals longer
  than the buffer limit.
  Fixed ubasic potential string buffer overflows by using snprintf.
  Fixed ubasic label strings to use UBASIC_LABEL_LEN_MAX as buffer limit.
  Fixed ubasic string variables to initialize with zeros.
  Fixed compile errors when UBASIC_DEBUG_STRINGVARIABLES is defined.
  Added ubasic string variables user accessor API and unit testing. (#1196)
* Secured BACnet file object pathname received from BACnet AtomicWriteFile
  or ReadFile service used without validation which was vulnerable to
  directory traversal attacks. (#1197)

### Added

* Added BACNET_STACK_DEPRECATED_DISABLE guards around all of the deprecated
  decoding functions to ensure they are not used except intentionally for
  legacy code bases. (#1244)
* Added Who-Is-Router-To-Network process in basic Notification Class when
  recipient address router MAC address is unknown. (#1243)
* Add initialization and unpacking functions for BACnet Character
  String buffer structure. (#1242)
* Added Host-N-Port minimal encode and decode which utilizes the octet
  string and character string buffer encode and decode. (#1239)
* Added API for extending the basic Device object and children with
  proprietary properties for ReadProperty and WriteProperty services. (#1238)
* Added octet and character string buffer codecs to used with fixed
  size buffers that are not declared as BACNET_OCTET_STRING
  or BACNET_CHARACTER_STRING. (#1237)
* Added CreateObject and DeleteObject for basic Accumulator objects and
  WriteProperty handling for object-name, scale, out-of-service, units,
  and max-pres-value. (#1234)
* Added text and parser for BACnet ReinitializeDevice service states. (#1228)
* Added Device Management-Backup and Restore-B to example object/device.c
  and basic/server/bacnet-device.c so that when ReinitializeDevice STARTBACKUP
  is requested a backup file is stored in CreateObject format for all the
  writable properties in the device. When ReinitializeDevice STARTRESTORE
  is requested a backup file is restored from CreateObject format for all the
  writable properties in the device. (#1223)
* Added apps/dmbrcap for Device Management-Backup and Restore to convert
  a backup file encoded with CreateObject to Wireshark PCAP format. (#1223)
* Added segmentation support functions and example changes, but
  no support for segmentation in the TSM or APDU handlers. (#1218)
* Added channel and timer object write-property observers in blinkt app
  to monitor internal writes. Added vacancy timer command line argument
  for testing initial timer object vacancy time for lights channel. (#1212)
* Added debug prints for lighting output properties to assist in identifying
  out-of-range values. (#1211)
* Added API to get the RGB pixel and brightness values from the blinkt
  interface. Added API to the color-RGB library to convert from ASCII
  CSS color name to X,Y and brightness. Added a default color name command
  line option. Set the color and brightness at startup. Changed the Blinkt
  example app to use the basic-server framework. (#1210)
* Added enumeration text lookup for BACnetAuthenticationStatus,
  BACnetAuthorizationMode, BACnetAccessCredentialDisable,
  BACnetAccessCredentialDisableReason, BACnetAccessUserType,
  BACnetAccessZoneOccupancyState, BACnetWriteStatus, BACnetIPMode,
  BACnetDoorValue, BACnetMaintenance, BACnetEscalatorFault,
  BACnetEscalatorMode, BACnetEscalatorOperationDirection,
  BACnetBackupState, BACnetSecurityLevel, BACnetLiftCarDirection,
  BACnetLiftCarDoorCommand, BACnetLiftCarDriveStatus, BACnetLiftCarMode,
  BACnetLiftFault, BACnetLiftGroupMode, BACnetAuditLevel, BACnetAuditOperation,
  BACnetSCHubConnectorState, BACnetSCConnectionState, BACnetNodeRelationship,
  BACnetAction, BACnetFileAccessMethod, BACnetLockStatus,
  BACnetDoorAlarmState, BACnetDoorStatus, BACnetDoorSecuredStatus,
  and BACnetAccessEvent. (#1209)
* Added a new API for writable property lists across all the basic example
  object types, preparing for the introduction of a Writable_Property_List
  property in every object in a future BACnet standard revision.
  The lists can be used by backup and restore feature to automatically
  choose the object property values in the backup that can be restored
  via internal WriteProperty directly from BACnet CreateObject services with
  List of Initial Values. Updated existing device objects to include
  these writable property lists. (#1206)
* Added post-write notifications for channel, timer, and loop objects. (#1204)
* Added device WriteProperty callbacks for Timer object in example device
  objects implementations. (#1203)
* Added file path name checking for AtomicReadFile and AtomicWriteFile
  example applications. Prohibits use of relative and absolute file paths
  when BACNET_FILE_PATH_RESTRICTED is defined non-zero. (#1197)
* Added API and optional properties to basic load control object example
  Refactored BACnetShedLevel encoding, decoding, and printing into separate
  file. Added BACnetShedLevel validation testing. (#1187)
* Added API for Analog_Input_Notification_Class, Analog_Input_Event_Enable,
  and Analog_Input_Notify_Type. (#1184)
* Added API and optional properties to basic lighting output object example
  for power and feedback value. (#1185)
* Added properties to the Channel object write member value coercion
  minimal properties supported. (#1176)
* Added Send_x_Address() API to ReadPropertyMultiple, WritePropertyMultiple,
  and SubscribeSOV services primarily for interacting with MS/TP slaves (#1174)
* Added npdu_set_i_am_router_to_network_handler() API. Fixed sending to
  broadcast address in npdu_send_what_is_network_number() API.  (#1169)
* Added BACnetRecipient and BACnetAddressBinding codecs for EPICS application.
  The implementation includes full encode/decode, ASCII conversion,
  comparison, and copy functions for both data types, along with
  comprehensive test coverage and supporting string utility functions.
  Integrated the new types into the application data framework with
  application tag assignments. (#1163)
* Added library specific string manipulation utilities including strcmp,
  strncmp, and snprintf with offset functions. (#1163)(#1164)
* Added default option to bactext name functions so that NULL can be
  returned when a name does not exist. (#1160)
* Added library specific ltrim, rtrim, and trim string functions. (#1159)
* Added library specific itoa, ltoa, ultoa, dtoa, utoa, and general
  print format with ASCII return functions. (#1157)
* Added library specific string-to functions similar to stdlib.
  Added library specific string-to functions for BACnet primitives. (#1151)

### Changed

* Changed BACFILE define dependencies to reflect bacfile-posix.c dependence
  since bacfile.c is now independent of any back end file system. (#1227)
* Changed the default BACnet protocol revision to 28 to enable usage of
  special lighting output values. (#1211)
* Changed bacnet_strtof and bacnet_strtold functions to use strtod to
  improve compatibility with C89 standards while ensuring proper type
  casting and range checking. (#1207)
* Changed RGB color clamp function to avoid Zephyr RTOS name collisions. (#1201)
* Changed the load control object AbleToMeetShed to only check for immediate
  shed ability and added CanNowComplyWithShed function to attempt to meet the
  shed request while in the non-compliant state. (#1191)
* Changed the size of MAX_HEADER in BACDL_MULTIPLE because 8 is not
  big enough for some datalinks (e.g. mstp). (#1170)
* Changed printf() in many apps to use debug_printf() and friends. (#1168)
* Changed apps to use common BACnet value string parsing functions. (#1152)
* Changed basic object API for units properties to use BACNET_ENGINEERING_UNITS
  datatype. (#1104)
* Changed all the property list values into int32_t to support the larger
  BACnet property enumerations when an int is 8-bit or 16-bit in size. (#1145)

### Fixed

* Fixed the basic Schedule object to set the correct present-value
  based on the Device object date and time. (#1236)
* Fixed dlenv_init() for BACnet/SC. bsc_register_as_node() was blocking
  when the hub was not reachable. Added API so that BACnet/SC
  node can register via thread that is reponsible for connecting
  BACnet/SC hub and the dlenv_init() can continue without waiting. (#1230)
* Fixed bacfile-posix file write to return the number of bytes written. (#1223)
* Fixed Event parsing and help text for the example uevent and event apps.
  Fixed initialization of event data by adding static CharacterString for
  message text. Fixed the event parsing to start at argument zero. (#1221)
* Fixed You-Are-Request encoding and decoding to use object-id instead
  of unsigned. (#1220)
* Fixed handling for abort and reject errors in Write Property service. (#1216)
* Fixed lighting output object lighting-commands for warn-off and
  warn-relinquish when an update at the specified priority slot
  shall occur after an egress time delay. (#1214)
* Fixed lighting output object lighting-commands for warn-off and
  warn-relinquish when blink-warn notification shall not occur. (#1212)
* Fixed timer object task to initiate a write-request at expiration. (#1212)
* Fixed the server name in the blinkt app and removed the unnecessary
  device.c module. (#1211)
* Fixed Channel object for Color object present-value which does not
  use coercion. (#1210)
* Fixed lighting output object lighting-command last-on-value to only
  be updated with the last value of the Present_Value property that
  was greater than or equal to 1.0%, keeping in mind that the Present_Value
  shall indicate the target level of the operation and not the current
  value. (#1205)
* Fixed CreateObject service list-of-initial-values encoding and decoding.
  Changed the data structure to be similar to WriteProperty. (#1199)
* Fixed lighting-output object blink warn to honor blink-warn-enable.
  Fixed the blink warn logic for a non-zero percent value blink inhibit.
  Fixed the warn relinquish to actually relinquish. (#1192)
* Fixed channel-value encoding in the channel object when no-coercian
  is required for lighting-command, color-command, and xy-color. (#1190)
* Fixed NULL handling in CharacterString sprintf which caused an endless
  loop. (#1189)
* Fixed a regression in the rpm_ack_object_property_process() function
  that prevented proper parsing of multi-object ReadPropertyMultiple ACK
  responses. The bug was introduced in PR [#765] and caused the function
  to incorrectly return ERROR_CODE_INVALID_TAG after processing the
  first object, even when additional valid objects were present in
  the response. Added tests that use rpm_ack_object_property_process()
  with a multi-object RPM ACK to verify the fix and prevent regression. (#1183)
* Fix warnings during unit testing of BACnet secure connect node. (#1182)
* Fixed the BACnetChannelValue coercion function to include all the coercions,
  no coercions, and invalid datatypes described in Table 12-63 of the Channel
  object.  Expanded unit testing code coverage for BACnetChannelValue. (#1181)
* Fixed the Channel object to handle all data types that do not need
  coercion when written. Fixed present-value when no value is able to
  be encoded. (#1176)
* Fixed the Loop object read/write references and manipulated variables
  update during timer loop by adding callbacks to device read/write property
  in basic example device object. (#1175)
* Fixed library specific strcmp/stricmp functions match standard strcmp. (#1173)
* Fixed compiler macro redefined warning when optional datatypes are defined
  globally. (#1172)
* Fixed copy and compare API of BACnetObjectPropertyReference structure. (#1171)
* Fixed array-bounds on BACnetObjectPropertyReference parsing. (#1167)
* Fixed the missing BACnetObjectPropertyReference,
  BACnetSCFailedConnectionRequest, BACnetSCHubFunctionConnection,
  BACnetSCDirectConnection,BACnetSCHubConnection, BACnetTimerStateChangeValue,
  and BACnetAddressBinding text used in debugging.(#1166)
* Fixed the units to/from ASCII to function for other units properties. (#1165)
* Fixed datetime integer overflow on 8-bit AVR compiler (#1162)
* Fixed the ports/linux BACnet/IP cache netmask for accurate subnet
  prefix calculation implementation which had always returned 0. (#1155)
* Fixed the loop object empty reference property by initializing to self.
  When configured for self, the manipulated property value will update
  the controlled variable value for simulation.(#1158)
* Fixed unit test stack corruption from using stack based message text
  characterstring pointer in multiple functions and setting the value
  in the global event and test event data structures. (#1154)
* Fix timesync recipient encoding to check for OBJECT_DEVICE type before
  encoding. (#1153)
* Fixed segmented ComplexACK in MS/TP by adding postpone reply because
  transmission of the segmented ComplexACK cannot begin until the node
  holds the token. (#1116)

### Removed

## [1.4.2] - 2025-11-15

### Security

* Secured BACnetAuthenticationFactorFormat decoding by refactoring deprecated
  functions and validating with unit testing. (#1127)
* Secured UnconfirmedEventNotification-Request and
  ConfirmedEventNotification-Request, BACnetNotificationParameters,
  and BACnetAuthenticationFactor decoding by refactoring deprecated
  functions and validating with unit testing. (#1126)
* Secured I-Am request encoding and decoding, and updated the example apps
  and handlers to use secure version of I-Am decoder. (#1080)
* Secured GetEventInformation-Request and -ACK decoder and encoder. (#1026)

### Added

* Added a basic creatable Loop object with PID control. Integrated into
  the basic device object and server examples. (#1141)
* Added defines for lighting output object present-value special values. (#1137)
* Added get copy API to timer object for state-change-value (#1134)
* Added Audit Log and Time Value objects to basic device and builds. (#1128)
* Added ListElement service callback for storing data. (#1128)
* Added a basic timer object type example. (#1123)
* Added BACnetLIST utility for handling WriteProperty to a list. (#1123)
* Added Device_Object_Functions() API to return basic object API table
  of functions for all objects. Added Device_Object_Functions_Find() API
  to enable override of basic object API function. (#1115)
* Added new enumerations, text, BACnetARRAY and BACnetList from
  protocol-revision 30 (#1114)
* Added a context variable in basic object data structure and API
  to get or set the context pointer. (#1111)
* Added uBASIC+BACnet README document to describe the programming language,
  porting, and integration. (#1108)
* Added API to output objects for priority-array property value
  inspection. (#1096)
* Added lighting command refresh from tracking value API. (#1094)
* Added MS/TP statistics counters for BadCRC and Poll-For-Master. (#1081)
* Added Lighting Output API to implement override for HOA control.
  Integrated lighting command overridden behavior into the lighting
  output object and added Overridden status flags API.
  Added Lighting Output API to implement a momentary override
  to the output that is cleared at the next lighting command. (#1086)
* Added Trim_Fade_Time, High_End_Trim, Low_End_Trim, Last_On_Value
  and Default_On_Value properties to lighting output object.
  Added TRIM_ACTIVE flag to lighting command.
  Added Last_On_Value and Default_On_Value
  to lighting command for restore and toggle. (#1086)
* Added bacnet_recipient_device_set() and bacnet_recipient_address_set()
  API. (#1083)
* Added MS/TP datalink option to BACnet basic server example. (#1077)
* Added fixups to Microsoft Visual Studio build: added server-mini, etc. (#1061)
* Added WriteProperty to GTK Discover app.  For enumerated properties,
  the property text is converted to an integer.  For commandable object
  properties (present-value), priority is encoded after the value
  or null using @ symbol.
* Added dynamic and static RAM file systems to use with file objects. (#1058)
* Added check for read-only during AtomicWriteFile service API for
  BACnet File object. (#1058)
* Added simple project for GTK BACnet Discovery Application. (#1064) (#1065)  (#1070)
* Added BACnet Zigbee VMAC table and unit test. (#1054)
* Added BACnet Zigbee Link Layer (BZLL) with stubs. (#1052)
* Added a debug print when tsm retries. (#1040)
* Added bvlc_delete_from_bbmd() API to unregister as a foreign device
  and become normal (not foreign). (#1041)
* Added host_n_port_to_minimal() API to support ipv4 address. (#1039)
* Added BACnetErrorCode text for new enumerations.
* Added known property decoding in UnconfirmedCOVNotification handler. (#1030)
* Added the ability for apps/ucov to send property specific application
  tagged data using -1 argument for tag. (#1030)
* Added the ability to write to the mstp mac address and link speed
  property values in the basic network port object. (#1025)
* Added new error-code enumerations from 135-2024 BACnet.
* Added new BACnetPropertyState enumerations from 135-2024 BACnet.
* Added a basic example Auditlog object. (#458)
* Added You-Are and Who-Am-I encoding, decoding, unit tests, and command
  line applications bacyouare and bacwhoami. Added You-Are handler and
  integrated with apps/server example with JSON printout. (#1024)
* Added object type and services supported BACnetBitString sizes for
  protocol revision 25-30. (#1020)
* Added new BACnet text for BACnetLifeSafetyMode, BACnetLifeSafetyOperation,
  BACnetRestartReason, BACnetNetworkType, BACnetNetworkNumberQuality,
  BACnetNetworkPortCommand, BACnetAuthenticationDecision,
  BACnetAuthorizationPosture, BACnetFaultType, BACnetPriorityFilter,
  BACnetSuccessFilter, and BACnetResultFlags. (#1020)
* Added new BACnetPropertyIdentifier, BACnetEngineeringUnits, BACnetEventState,
  BACnetRestartReason, BACnetLifeSafetyMode, BACnetLifeSafetyOperation,
  BACnetLifeSafetyState, BACnet_Services_Supported, BACnetLightingOperation,
  BACnetBinaryLightingPV, BACnetNetworkPortCommand,
  BACnetAuthenticationDecision, BACnetAuthorizationPosture, BACnetFaultType,
  BACnetPriorityFilter, BACnetResultFlags, and BACnetSuccessFilter
  enumerations. (#1020)
* Added more writable properties to Analog, Binary, and Multistate Output
  objects. (#1012)
* Added missing API defined in header into ports/win32/dlmstp.c module,
  added a PDU queue and refactored receive thread, and refactored MS/TP
  timing parameters. (#1003)
* Added missing API defined in header into ports/linux/dlmstp.c module,
  and refactored MS/TP timing parameters. (#1003)
* Added missing API defined in header into ports/bsd/dlmstp.c module,
  and refactored MS/TP timing parameters. (#1003)
* Added Egress_Time integration during the blink-warn and warn-relinquish
  lighting operations. (#1008)
* Added writable properties lighting-command-default-priority, egress-time,
  blink-warn-enable, and relinquish-default. (#1008)
* Added schedule object WriteProperty handling for effective-period,
  list-of-object-property-references and exception-schedule properties. (#1000)
* Added multiple uBASIC program objects to stm32f4xx example port. (#995)
* Added more API for BACnet basic server device object. (#994)
* Added the weekly-schedule property write in basic schedule object. (#990)
* Added uBASIC-Plus program object example to STM32F4xx. (#967)
* Added guards in create object initialization to prevent memory leaks. (#965)

### Changed

* Changed enumerations for BACnetNetworkPortCommand where there was a conflict
  with final assigned values in multiple addenda. (#1124)
* Changed CharacterString_Value_Out_Of_Service_Set() function to remove
  confusion about an assignment inside an if-statement. (#1113)
* Changed device object unit test to use common read-write property test.
  Extended the basic BACnet device object example API.
  Added BACnet/IP and COV test mocks to enable device object testing
  with less dependencies. (#1106)
* Changed Who-Am-I and You-Are JSON handlers to eliminate dynamic
  memory allocation for model and serial number strings,
  improving memory management and simplifying code. (#1089)
* Change stm32f4xx example to use static RAM file system. (#1058)
* Changed the bacnet file object to be storage agnostic by refactoring
  and using callbacks. (#1056)
* Changed ReadRange by-position and by-sequence encoding by refactoring
  into a common module. (#1028)
* Changed default MS/TP APDU to 480 to avoid extended frames by default. (#1003)
* Changed mirror script to improve debugging. (#968)
* Changed dlenv to support multiple datalinks via environment variable. (#966)

### Fixed

* Fixed the apps/blinkt example project to control 8 lighting outputs.(#1143)
* Fixed the sequence of BACnet/SC datalink initialization that was
  broken during datalink environment changes and POSIX file refactoring.
  Refactored the UUID and VMAC random functions into port specific
  since stdlib rand() is not random and caused duplicate UUID and VMAC
  preventing BACnet/SC from forming any stable connections.
  Enabled debug in BACnet/SC datalink when BUILD=debug used.(#1142)
* Fixed WPM workaround for BTL Specified Test 9.23.2.X5 by reverting.(#1140)
* Fixed the API integration for the additional datatypes now supported
  in the Channel object by adding CHANNEL_VALUE_ALL to enable and test. (#1135)
* Fixed the error class returned for AlarmAcknowledgment (#1131)
* Fixed object creation failure when create is called before init. (#1122)
* Fixed octetstring_copy_value() and added unit tests. (#1121)
* Fixed the MS/TP compare DER function which can now include the NPDU network
  priority in the comparison. (#1119)
* Fixed basic program object internal datatype for reason-for-fault
  and change properties. (#1110)
* Fixed compile errors in basic/server/device when BACAPP_TIMESTAMP
  or Channel object are not enabled. (#1109)
* Fixed units property declaration in basic Analog Input header file to be
  uint16_t instead of uint8_t. Added range checking of units property
  in example objects WriteProperty handler. (#1107)
* Fixed Lighting Output object STOP lighting command so that it sets
  the present-value. (#1101)
* Fixed the lighting command RAMP TO ramp rate to always clamp within
  0.1 and 100.0 to avoid endless rate of 0.0. (#1100)
* Fixed Lighting Output step operations mixup. (#1099)
* Fixed Lighting Output step operations to set the Priority_Array slot. (#1098)
* Fixed the lighting output objects current priority comparison during
  lighting commands by using priority 17 for relinquish default
  instead of 0. (#1097)
* Fixed CMake Error in libwebsocket: Compatibility with CMake < 3.5 has
  been removed from CMake (#1095)
* Fixed Lighting Output Relinquish values. (#1094)
* Fixed copied code that no longer needs static function scope variables
  for text names. (#1092)
* Fixed compiler warning format '%u' expects argument of type 'unsigned int',
  but argument 4 has type 'uint32_t' {aka 'long unsigned int'}
  [-Werror=format=] by casting or increasing format specifier size
  and casting. (#1092)
* Fixed Lighting_Command to ignore write priority and use its own. (#1086)
* Fixed BACnetLightingOperation reserved range. (#1086)
* Fixed missing prototype warning in lighting.c module.
* Fixed AddListElement and RemoveListElement which were checking
  the wrong return value from Device object. Fixed decoding of
  ListElement Tag 0: Object ID instance. (#1083)
* Fixed Notification_Class_Add_List_Element() and
  Notification_Class_Remove_List_Element() element counter index
  and empty slot detection. (#1083)
* Fixed win32 builds where UNICODE is defined. The code now uses CreateFileA
  instead of CreateFile due to ANSI-C filenames. (#1076)
* Fixed the usage of index vs instance in the basic trend log object
  example. (#1074)
* Fixed bacapp_encode_context_data_value() to enable all non-primative
  value context encoding by removing the filter. (#1075)
* Fixed point-to-point VPN tunnel sockets for BACnet/IP by using
  IFF_POINTTOPOINT flag when getting the broadcast address. (#1066)
* Fixed missing text sprintf for Network Port object BACnetNetworkType,
  BACnetNetworkNumberQuality, and BACnetProtocolLevel enumerations. (#1069)
* Fixed missing enumeration text for Program object: BACnetProgramError,
  BACnetProgramState, and BACnetProgramRequest (#1068)
* Fixed missing enumeration text for BACnetNodeType, BACnetSilencedState,
  BACnetLoggingType (#1067)
* Fixed bacfile_count() function return type (#1058)
* Fixed the signed value for start position and start record in BACnet
  file object abstraction. (#1057)
* Fixed the Linux DLMSTP standalone test application Makefile. (#1055)
* Fixed ISO C90 forbids mixed declarations and code warning.(#1053)
* Fixed the MS/TP invalid frame counter that was incremented for valid
  frames not for us.(#1053)
* Fixed the ports/linux MS/TP by adding receive buffers, maximizing
  the thread priority, locking the mutex, iterating over queued replies
  to find a match, and sleeping before replying to enable the application
  to provide a reply. (#1051)
* Fixed the Microsoft Visual Studio compile environment. (#1050)
* Fixed the use of uninitialized local variables in COV handlers. (#1049)
* Fixed MS/TP zero-config FSM getting stuck when duplicate address is
  detected. (#1048)
* Fixed issues on FreeBSD with CMake build for BSC and IPv6 datalinks. (#1046)
* Fixed Network_Port_Changes_Discard() and added missing bip_get_interface()
  and Device_Time_Of_Restart() API. (#1038)
* Fixed bbmd_register_as_foreign_device() when only BBMD_CLIENT_ENABLED
  and not  BBMD_ENABLED. (#1032)
* Fixed GetEvent usage of linked list by initializing next in all
  the examples and unit test. (#1026)
* Fixed usage of Keylist_Data_Add() return value in Calendar,
  CharacterString Value, Load Control, and BACnet/SC Network Port
  objects. (#1016)
* Fixed BACnet/IP initialization on a network interface where the system
  reports the interface's unicast IP address as being the same as its
  broadcast IP address (e.g., utunX interfaces for VPNs on macOS). (#1011)
* Fixed ports/xplained conversion of double warning. (#1004)
* Fixed BACNET_USE_DOUBLE usage in AVR ports. (#1004)
* Fixed integer out-of-range in AVR port. (#1004)
* Fixed missing function prototype for whois_request_encode(). (#1004)
* Fixed Linux MS/TP 76800 bitrate for Linux 2.6.20+ circa 2007 and added
  get/set API for config. (#1007)
* Fixed network port object to accept host name option of host-n-port
  writes. (#997) (#1001)
* Fixed missing exports in bacnet/basic header files. (#996)
* Fixed NDPU comparison functions that were missing segment-ack PDU. (#991)
* Fixed WriteProperty NULL bypass which is only for present-value property
  of commandable objects. (#984)
* Fixed the ghost Device ID 0 in the I-Am response when the actual
  routed devices are less than the MAX_NUM_DEVICES for gateway device. (#981)
* Fixed BACnetGroupChannelValue encoding and decoding of BACnetChannelValue
  which was deemed errata by BACnet standard committee. (#980)
* Fixed some INTRINSIC_REPORTING #ifs in AV and BV basic objects. (#977)
* Fixed network specific original broadcast for IP in apps/router. (#976)(#989)

## [1.4.1] - 2025-04-11

### Security
* Secured ReadRange service codecs. Added ReadRange unit testing.
  Secured ReadRange-ACK handler to enable APDU size checking. (#957)
* Secured BACnet/SC URL handling by changing all the sprintf
  to snprintf which ensures null string endings. (#936)
* Secured win32 port of localtime by using secure OS API functions
  when compiled with MSVC. (#936)
* Secured SubscribeCOVProperty decoder. Changed datatype of monitoredProperty
  in struct BACnet_Subscribe_COV_Data. (#892)
* Secured the BACnet Who-Request decoder by changing deprecated decode
  functions. (#891)

### Added

* Added the option to specify a target destination for
  apps/server-discover. (#958)
* Added Program object task state transitions from request and callbacks. (#960)
* Added write present value callbacks for Analog Value and Integer Value
  basic object examples. (#956)
* Added BACNET_IP_BROADCAST_USE_INADDR_ANY ifdef option for apps/router
  network interface port binding. (#953)
* Added null parsing to weekly-schedule writes. (#954)
* Added BACnet BACnet Testing Laboratories Implementation Guidelines
* Added bacmini example app with minimal analog and binary objects. (#934)
* Added bacnet-basic from zephyr project to create basis for apps/server-basic
  bacbasic example. (#933)
* Added Send_I_Am_Broadcast() function to Who-Is handler so that
  other Send_I_Am() will honor DCC Disable-Initiatiation. (#918)
* Added simple script to aid in mirror the Github repository with Sourceforge.
* Added unit test while reading all object properties to flag properties
  not in the property-list. (#910)
* Added enumeration for last-property used in unit testing. (#910)
* Added missing reliability property in the basic multistate output
  object example. (#910)
* Added an optional MS/TP automatic baudrate detection option into
  the core MS/TP state machine. (#900)
* Added linked list of lighting-command notification callbacks. (#893)
* Added bvlc6.sh script to enable foreign-device-registration for
  client tools. (#889)
* Added check for zero length buffer size in primitive decoders
  that returns zero to enable simpler complex value optional
  element decoding. (#876)
* Added bitstring-bits-used-set API to use in audit-log status bits (#879)

### Fixed

* Fixed debug printf warnings in BACnet/SC modules. (#963)
* Fixed MS/TP lost-token-counter that was lost during refactoring. (#962)
* Fixed duplicate code by removal in apps/readprop. (#959)
* Fixed ReadRange app to read and pretty-print a Trend Log log-buffer (#947)
* Fixed bip cleanup to enable initializing interfaces after cleanup. (#949)
* Fixed configure script and removed aptfile duplicity. (#946)
* Fixed the COV for Analog Input and Analog Value objects when
  fault is detected in Reliability property. (#943)
* Fixed WriteProperty error code for PROP_FD_BBMD_ADDRESS and
  PROP_FD_SUBSCRIPTION_LIFETIME properties. (#925)
* Fixed dead-code warning after enabling all datalinks for basic
  network port object using the property list as the R/W checking
  for the port type. (#925)
* Fixed the basic multi-state output priority-array datatype encoding. (#932)
* Fixed windows build of bacpoll and bacdiscover by removing DLL export
  in basic client headers (#930)
* Fixed Device_Write_Property_Object_Name() to return WRITE_ACCESS_DENIED
  in case where an object name is not writable using BACnet protocol. (#927)
* Fixed WriteProperty writing to object properties when the array-index
  is valid (#931)
* Fixed Who-Has object instance by checking for valid instance. (#922)
* Fixed out-of-service writability to be consistent with present-value
  in objects using Write_Enabled flag (#921)
* Fixed the NDPU encoding for confirmed COV notifications (#917)
* Fixed the ReinitializeDevice and DeviceCommunicationControl
  password length checking for non-UTF8 passwords. (#914)
* Changed link-speed, network-number, network-number-quality,
  and apdu-length properties of the network port object to be
  optional when protocol-revision is 24 or greater. (#913)
* Fixed error-code returned when an object does not support WriteProperty
  but has properties that are known. (#912)
* Fixed structured view object subtype get and set. (#909)
* Fixed bacrp and bacrpm apps when reading the array index zero of
  arrays by adding a BACnet application decoder that understands that
  array index zero is supposed to be an unsigned integer tagged value. (#908)
* Fixed index zero reading for basic objects BACnetARRAY property values by
  removing BACnetARRAY index validation in all the basic and example objects
  and moving the BACnetARRAY index validation into the common ReadProperty,
  ReadPropertyMultiple, WriteProperty, and WritePropertyMultiple
  handlers. (#908)
* Fixed multi-state-input and multi-state-value basic objects usage of the
  Write_Enabled flag by adding an API to get/set the flag. (#903)
 * Fixed usage of 8-bit modulo operator off-by-one maximums. (#901)
* Fixed legacy make build recipe for library. 'make library' now builds.
* Fixed IPv6 to leave multicast when registering as foreign device. (#899)
* Fixed IPv6 handler to ignore original-broadcast when registered as
  a foreign-device (#898)
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

* Changed the folder for bacmini application to server-mini, and fixed
  for Makefile and CMake builds. (#941)
* Changed date encoding when year is out of range to use wildcard.
  Updated APDU encoding pattern for date and time. (#897)
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

### Removed

* Removed polarity property in binary value object as it is not standard. (#910)

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
