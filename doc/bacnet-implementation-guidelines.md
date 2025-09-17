---
title: BACnet Testing Laboratories Implementation Guidelines
subtitle: recommended guidelines for the options and capabilities of all BACnet devices
description: The BACnet standard offers many options in the implementation of BACnet devices. Members of the BACnet Testing Labs (BTL) have agreed on specific options of capabilities that should be implemented in BACnet devices to maximize their interoperability with devices that are tested and listed by the BTL.
copyright: BACnet is a registered trademark of ASHRAE. ASHRAE does not endorse, approve or test products for compliance with ASHRAE standards. Compliance of listed products to the requirements of ASHRAE Standard 135 is the responsibility of BACnet International. BTL is a registered trademark of BACnet International.
---
# BACnet Testing Laboratories Implementation Guidelines

# 1 :Introduction

The BACnet standard offers many options in the implementation of BACnet
devices. Members of the BACnet Testing Labs (BTL) have agreed on
specific options of capabilities that should be implemented in BACnet
devices to maximize their interoperability with devices that are tested
and listed by the BTL.

This document presents those options and capabilities as recommended
guidelines for all BACnet implementers. It also presents guidelines to
avoid mistakes made in the past by new BACnet implementers.

Note: as of 2024, this document is no longer published or maintained
by the BTL. The BTL and the BACnet committee have been using these
guidelines to improve the BACnet protocol and testing standard such that
recent protcol-revisions no longer require as much implementation guidance.

# 2: General

The following guidelines apply to all devices.

## 2.1: Beware of assumptions

Don't make assumptions about the behaviour of a communicating peer
beyond what the BACnet standard specifies. The peer device might not
implement some optional functionality, property or service that is designated by the standard. Common examples of such are the basis
of specific recommendations in this guide.

In particular, be prepared to communicate with devices implemented to an
earlier protocol revision. Additions are listed in the “History of
Revisions” at the end of the BACnet standard. The referenced Addenda
detailing the changes are available online from ASHRAE.

## 2.2: Be prepared to fall back when utilizing optional features

Do not rely on other devices supporting an optional capability of
BACnet; it might not be there. Whenever possible provide a fallback
position by using features that are required. For example, if one wants
reasonably timely data updates from a peer device and seeks to acquire
them by subscribing for change of value reports (COV), fall back to
polling for the data if the peer device does not support COV
subscription, or if it rejects the subscription request for some other
reason.

## 2.3: All BACnet devices shall execute ReadProperty-Request

All BACnet devices shall be able to execute the ReadProperty service
request, even BACnet client devices. The BACnet standard requires this
for all devices with an application layer.

BACnet routers may or may not have an application layer.  Therefore,
BACnet Routers may or may not be able execute ReadProperty service request.

## 2.4: A device shall execute all forms of a service

When a device is able to execute a service request, it shall execute all
forms of the service. This allows a client device to select the form of
the service request it chooses and expect successful interoperation.
“All forms of the service” include all forms of the service parameters
(see 7.11 in this guide, for example), and its application to
appropriate objects and services (see 7.12 in this guide).

Except as specifically noted in the standard.

It should be noted that some forms of some services will, when executed
correctly by a device, always return a Result(-). For example, a device
that contains no writable array properties will always return a
Result(-) to WriteProperty requests that contain an Array Index
parameter.

Example Exceptions:

|                                                |                 |                                                                                                                                                                |
| ---------------------------------------------- | --------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Service                                        | Parameter       | Reason                                                                                                                                                         |
| DeviceCommunicationControl, ReinitializeDevice | 'Password'      | Devices may require the 'password' property to be present and therefore respond with a 'Result(-)' if this parameter is missing.                               |
| DeviceCommunicationControl                     | 'Time Duration' | Devices which do not support an internal clock are not required to support this parameter and may return 'Result(-)' if a finite duration is specified.        |
| SubscribeCOV                                   | 'Lifetime'      | If 'IssueConfirmedNotifications' is present and 'Lifetime' is absent, a device may return a 'Result(-)' if they do not support the indefinite lifetime option. |

## 2.5: The maximum conveyable APDUvalue of Max\_APDU\_Length\_Accepted might be different on different ports

The actual maximum conveyable APDU may be limited due to routers in the
path to the remote network. Standard 135, Clause 19.4 describes a method
to determine the maximum conveyable APDU.

## 2.6: Polling intervals should be configurable

For devices that are capable of polling, the poll interval should be
configurable, and the default should be reasonable, not “as fast as
possible.” This reduces the risk of overloading slower LANs and swamping
routers; in addition, experience shows that not all Ethernet devices
might be able to handle BACnet requests sent “as fast as possible,”
perhaps due to internal processing delays.

## 2.7: Do not flood the network on startup

Upon device startup, do not flood the network with messages. This avoids
overloading networks and routers, and possibly recipient devices. When
power is restored to a building, 100s or 1000s of devices may be trying
to re-establish connections at about the same time. One possible method
to help with flooding is to limit the number of outstanding requests.

## 2.8: For proprietary extensions, use higher numbers

When creating vendor-proprietary extensions see Standard 135, Clause 23,
whether it is for proprietary object types, proprietary properties or
extending enumerations, use higher-numbered values rather than assign
sequentially from the lowest value allowed for vendor-proprietary
extensions. This applies particularly to the BACnetEngineeringUnits
enumeration, for which the reserved range may need to be extended.

## 2.9: Be prepared for future changes

In the future the available CHOICEs in any particular ASN.1 production
(Standard 135, Clause 21) might be expanded beyond what is currently
defined, as might enumerations (such as BACnetPropertyIdentifier). In
addition, reserved fields and values might later be defined and used.

Devices (and parsers) should be implemented to handle these situations
when encountered, without crashing the device.

## 2.10: Don't give up forever

If a confirmed service request fails (all APDU timeouts and retries pass
without a response), don't give up forever on communicating with the
remote device. If dynamic binding (Who-Is or Who-Has) is used to locate
the remote device or object, fall back to occasionally initiating the
Who-Is or Who-Has to locate the remote device or object.

Similarly, if a Who-Is or Who-Has intended to locate a remote device or
object is sent without receiving the corresponding I-Am or I-Have,
consider re-sending the Who-Is or Who-Has occasionally until the I-Am or
I-Have is seen.

## 2.11: DeviceCommunicationControl(disable-initiation) does not require Protocol Revision 4

The BTL allows a device to support the 'disable-initiation' option for
the 'enable-disable' parameter (introduced by Protocol\_Revision 4)
regardless of the Protocol\_Revision claimed by the device.

## 2.12: Units properties may support values from higher Protocol Revision levels

The BTL allows a device to support values for Units property
enumerations defined by Protocol\_Revision levels higher than that
claimed for the device.

## 2.13: The Accumulator and Pulse Converter objects require Protocol Revision 4

The BTL requires that devices that support the Accumulator and Pulse
Converter objects must claim Protocol\_Revision 4 or higher.

## 2.14: Be prepared to handle MAC addresses of any size less than or equal to 6 bytes

MAC addresses are not limited to 1, 2, or 6 bytes as might be thought
from Standard 135, Table 6-2. Any size MAC address should be allowed
which is less than or equal to 6 bytes. If the device is a router, then
any size MAC address with a DLEN of 7 bytes or less shall be supported.

As an example, gateways to proprietary systems are allowed to provide
MAC addresses that are not included in the standard table.

## 2.15: Passwords shall be 20 characters or less

The standard requires that the passwords used for the
DeviceCommunicationService and ReinitializeService are restricted to a
maximum of 20 characters.

## 2.16: Addresses are expected to be no more than 6 bytes

Before Addendum 135-2008q-2 which defined the VMAC layer, the SSPC was
considering the use of MAC addresses up to 18 bytes. With the adoption
of 135-2008q-2 and the VMAC layer approach, addresses are expected to be
no more than 6 bytes. So there is no longer a need for implementations
to expect longer addresses. The support for LON addresses of length 7 is
so restrictive--only usable as a DLEN and never as an SLEN--that only
routers and devices that themselves will utilize the form of LON
addresses of length 7, are required to support that.

## 2.17: Don’t restrict to only one data link type of address

There should not be any restriction in the implementation to only one
data link type of address. Be prepared to communicate with devices from
any data link layer, using BACnet standard addresses.

# 3: The Device object

The following guidelines cover devices that contain a Device object, or
that access another device’s Device object.

## 3.1: All BACnet devices shall contain a Device object

All BACnet devices with an application layer shall contain a Device
object, even BACnet clients. This is required by the BACnet standard. See
Standard 135, Clause 12.10.

## 3.2: All Device objects shall contain a non-empty Object\_List property

All Device objects shall contain an Object\_List property, and this
property shall include the Device object. This is required by the BACnet
standard.

## 3.3: Be prepared to read the Object\_List array element by element

Some small devices that do not support segmentation have Object\_List
properties that are too large to transmit unsegmented. If a device needs
to read another’s Object\_List property, be prepared to read it array
element by array element.

## 3.4: The Device instance shall be configurable

Device instances are required to be configurable to allow for an
internetwork-wide identity across the entire valid range of 0 to
4194302.

## 3.5: Client devices should expect to see Device instances across the entire range of 0 to 4194302

Client devices should expect to see Device instances across the entire
range of 0 to 4194302.

## 3.6: Device instance number 4194303

Device instance number 4194303 can be used as a “wildcard” value for
reading a Device object’s Object\_Identifier property (to determine its
Device instance). If a ReadProperty or ReadPropertyMultiple request is
received for the Object\_Identifier property of Device 4194303, the
response shall convey the responding device’s correct Device object
instance.

## 3.7: The length of Bit Strings might be different than expected

The length of the Device object's Bit String properties, in particular
the Protocol\_Services\_Supported and Protocol\_Object\_Types\_Supported
properties will vary depending upon the protocol revision to which the
device was implemented. Client devices should be prepared to accept Bit
String values from servers with lengths longer or shorter than those
defined for the Protocol\_Revision value to which the client device was
implemented.

The length of Protocol\_Services\_Supported property value

|          |      |                                                                                                                |
| -------- | ---- | -------------------------------------------------------------------------------------------------------------- |
| Revision | Size | Notes                                                                                                          |
| 0        | 35   |                                                                                                                |
| 1        | 37   | Added ReadRange(35), UTCTimeSynchronization(36),                                                               |
| 2 – 13   | 40   | LifeSafetyOperation(37), SubscribeCOVProperty(38), GetEventInformation(39)                                     |
| 14 – 17  | 41   | WriteGroup(40)                                                                                                 |
| 18 – 19  | 44   | SubscribeCOVPropertyMultiple(41), ConfirmedCOVNotificationMultiple(42), UnconfirmedCOVNotificationMultiple(43) |
| 20 - 21  | 47   | ConfirmedAuditNotification(44), AuditLogQuery(45), UnconfirmedAuditNotification(46)                            |
| 22-29    | 49   | WhoAmI(47), YouAre(48)                                                                                         |
| 30       | 50   | AuthRequest(49)                                                                                                |

The length of Protocol\_Object\_Types\_Supported property value

|          |      |                                                                                                                                                                                                                                                                                                                                                |
| -------- | ---- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Revision | Size | Notes                                                                                                                                                                                                                                                                                                                                          |
| 0        | 18   |                                                                                                                                                                                                                                                                                                                                                |
| 1        | 21   | Added Averaging(18), Multi-state Value(19), Trend Log(20)                                                                                                                                                                                                                                                                                      |
| 2 - 3    | 23   | Added Life Safety Point(21) and Life Safety Zone(22)                                                                                                                                                                                                                                                                                           |
| 4        | 25   | Added Accumulator(23) and Pulse Converter(24)                                                                                                                                                                                                                                                                                                  |
| 5        | 30   | Added Structured View(29), and reserved bits for several other objects in review.                                                                                                                                                                                                                                                              |
| 6        | 31   | Added Load Control(28), Access Door(30)                                                                                                                                                                                                                                                                                                        |
| 7 - 8    | 31   | Added Event Log(25) and Trend Log Multiple(27), and reserved bit for Global Group(26) object in review                                                                                                                                                                                                                                         |
| 9        | 38   | Added Access Credential(32), Access Point(33), Access Rights(34), Access User(35), Access Zone(36), Credential Data Input(37), and reserved bit for a future object type                                                                                                                                                                       |
| 10       | 51   | Added BitString Value(39), CharacterString Value(40), Date Pattern Value(41), Date Value(42), DateTime Pattern Value(43), DateTime Value(44), Integer Value(45), Large Analog Value(46), OctetString Value(47), Positive Integer Value(48), Time Pattern Value(49), Time Value(50), and reserved bit for object Network Security(38) in review |
| 11 - 12  | 51   | Added Global Group(26) and Network Security(38)                                                                                                                                                                                                                                                                                                |
| 13       | 53   | Added Notification Forwarder(51) and Alert Enrollment(52)                                                                                                                                                                                                                                                                                      |
| 14 - 15  | 55   | Added Channel(53) and Lighting Output(54)                                                                                                                                                                                                                                                                                                      |
| 16       | 56   | Added Binary Lighting Output(55)                                                                                                                                                                                                                                                                                                               |
| 17       | 57   | Added Timer(31) and Network Port(56)                                                                                                                                                                                                                                                                                                           |
| 18 - 19  | 60   | Added Elevator Group(57), Escalator(58), and Lift(59)                                                                                                                                                                                                                                                                                          |
| 20       | 63   | Added Staging (60), Audit Log (61), Audit Reporter (62)                                                                                                                                                                                                                                                                                        |
| 24-30    | 65   | Added Color (63), Color Temperature (64)                                                                                                                                                                                                                                                                                                       |

## 3.8: The length of the BACnetLogStatus will vary depending upon the protocol revision

The length of the BACnetLogStatus in ReadRange-ACK responses, when
reading the Log\_Buffer property of logging objects, will vary depending
upon the protocol revision to which the device was implemented.

The length of BACnetLogStatus

|               |      |                                     |
| ------------- | ---- | ----------------------------------- |
| Revision      | Size | Notes                               |
| 0 – 6         | 2    | log-disabled (0), buffer-purged (1) |
| 7 and greater | 3    | Added log-interrupted (2)           |

## 3.9: The Protocol\_Services\_Supported property identifies only executable services

Only bits representing services that are executable by the device shall
be set to '1' in the Protocol\_Services\_Supported property. Bits
representing services that are initiated by the device but not executed
shall be set to '0'.

This property is a means for the device to advertise to other devices
which services may be sent to the device with an expectation that they
will be accepted and executed.

# 4: The Network Port object

The following guidelines cover devices that contain Network Port
objects.

## 4.1: Network Port instance number 4194303

Network Port instance number 4194303 is used to read properties from the
Network Port object representing the network port through which the
request was received.

## 4.2: Writability

Specific properties in Network Port objects are required to be writable.
For MS/TP, the Max\_Manager and Max\_Info\_Frames are required to be
writable if the device supports DS-WP-B. For BACnet/IP and BACnet/IPv6,
the FD\_BBMD\_Address and FD\_Subscription\_Lifetime properties in
FOREIGN mode and BBMD\_Broadcast\_Distribution\_Table and
BBMD\_Accept\_FD\_Registrations properties in BBMD mode.

# 5: MS/TP

The following guidelines cover devices that support the MS/TP data link.
See Standard 135, Clause 9.

## 5.1: Support Baud Rates Beyond 38.4k

Support for 9.6k and 38.4k bps is required by the BACnet standard. If
possible, also support other BACnet standard baud-rates for better
throughput for all devices sharing the MS/TP.

## 5.2: Support Auto-Bauding

All MS/TP devices must support a configurable baud-rate that is
maintained over a power-cycle. Non-routing MS/TP devices might consider
implementing auto-bauding, to simplify initial device deployment.
Auto-baud detection should be enabled on start-up to detect baud-rate
changes. Devices doing auto-bauding should maintain knowledge of the
last baud rate over a power loss to allow for more efficient
reconnection to the network. After a power-cycle, devices that require
information from the network to operate (client capabilities) should
wait a finite period of time for network traffic before initiating token
passing as they may be the only client on the network.

## 5.3: MS/TP MAC address must be configurable

The MAC address of a MS/TP device must be configurable by some means,
whether by DIP switches or a configuration tool. Support for the entire
address range (0..127 for managers and 0..254 for subordinates) is
required by the BACnet standard.

# 6: Segmentation

The following guidelines cover devices that support segmentation.  See
Standard 135, Clause 5.3 for details.

It should be noted that there exists an implicit assumption: If a device
supports segmentation for both transmit and receive, the maximum number
of segments it can send and receive are identical.

## 6.1: Support all services with all segments full

If a device supports segmentation, it shall be able to execute any
service request with all segments filled.

This rule applies in general to service initiation as well, though there
may be exceptions such as restricting the number of property requests
(and the datatypes requested) contained in a ReadPropertyMultiple
request so that the response, including worst-case error returns, will
not overrun the number of supported segments.

## 6.2: The maximum number of segments that can be accepted is advertised in two places

The maximum number of segments that can be accepted by a device is
"advertised" in two places: the Max\_Segments\_Accepted property of the
Device object (Standard 135, Clause 12.10.19) and in the
'max-segments-accepted' parameter of the BACnet-Confirmed-Request-APDU
(Standard 135, Clause 20.1.2.4). This value must be at least 2 if the
device supports segmentation.

The value of the Max\_Segments\_Accepted property, if present, of a peer
device should be considered when initiating a service request to that
peer. Devices responding to a service request shall use the
'max-segments-accepted' parameter of a confirmed service request to
determine if the response is transmittable (See Standard 135, Clause
5.4.5.3, transition CannotSendSegmentedComplex-ACK.).

## 6.3: Indicate the current max-segments-accepted limitation in the request APDU

If the number of the segments that can be received in the response for a
particular request is less than that displayed in the
Max\_Segments\_Accepted property of the Device object for some reason,
such as other concurrent requests using up available buffers, a smaller
value shall be indicated in the 'max-segments-accepted' parameter of the
BACnet-Confirmed-Request-APDU.

The value of zero for this parameter, indicating "unspecified number of
segments accepted," should not be used.

## 6.4: If ‘max-segmented-accepted’ is ‘000’, the maximum number of segments might be two

If the 'max-segments-accepted' parameter of a confirmed service request
is ‘000’, indicating “unspecified number of segments accepted”) and the
‘segmented-response-accepted’ parameter is TRUE, the maximum number of
segments of a message that the peer device will accept might be as low
as two. The use of B'000' signifying that the initiating client itself
does not know the maximum number of segments which it can accept, should
be avoided since that makes it impossible for the server to determine if
the response is transmittable.

## 6.5: Devices supporting segmentation shall support non-segmented methods to retrieve large amounts of data

Client devices that support segmentation and retrieve large amounts of
data shall be prepared to use non-segmented services when server devices
indicate they do not support segmentation.

For example, the Object\_List for a device may not fit into a single
segment. In this case, the individual array items shall be read. One
method is to read the size of the Object\_List property and then read
each item individually using the ReadProperty service or a group of
items could be read using ReadPropertyMultiple service provided that
each request and answer response can be placed in a single segment.

## 6.6: Devices shall be prepared to interoperate with implementations that claim segmentation in one direction only

Devices shall be able to interoperate with implementations that support
segmentation on transmit only. Devices shall be able to interoperate
with implementations that support segmentation on receive only.

# 7: Device Binding

The following guidelines cover the implementation of the Who-Is, I-Am,
Who-Has and I-Have service requests, which are used to locate ("bind
to") specific devices.

## 7.1: Implement I-Am and I-Have

The Who-Is service is a convenient mechanism used by most devices to
bind to peer devices. A few devices use the Who-Has request instead. For
this reason all devices should execute the Who-Is and Who-Has requests
and initiate, in response, the I-Am or I-Have request. MS/TP subordinate
devices are exempt because they cannot initiate requests, however there
are devices that cannot communicate with the subordinate devices. See
Clause 6.8.

It is preferable that I-Am and I-Have requests generated as a result of
a Who-Is or Who-Has request be unicast to the requester. This reduces
the amount of unnecessary network traffic elsewhere in the system.

## 7.2: Broadcast an I-Am on start-up

It is a common practice to broadcast an I-Am globally (to the entire
internetwork) on start-up. Though this practice can slow startup of
large systems after a power failure, it allows client devices that have
already started to bind to this device sooner, and may restore normal
operation more quickly when a device has been moved or replaced, perhaps
with a device with a different MAC address.

## 7.3: Use Who-Is and I-Am for device binding

Use Who-Is and I-Am for locating devices on the internetwork (“device
binding”).

When a device with a worldwide unique MAC address (such as an Ethernet
MAC address) is replaced, static binding (entering a device’s network
number and MAC address) can create a lot of error-prone work, because
every single static reference to the device that was replaced,
throughout the entire system, needs to be reconfigured.

## 7.4: Do not periodically broadcast I-Am and I-Have

Although the BACnet standard allows I-Am and I-Have requests to be
initiated at any time by a BACnet device, these requests should not be
periodically broadcast. This creates unnecessary traffic that can impede
PTP connections and slower LANs. In addition, some devices undertake
additional actions (in the form of confirmed service requests to the
device that sent the I-Am) in response to an I-Am because the device may
have been replaced, the device may have been replaced with a different
type of device, or its location may have changed.

## 7.5: Restrict the Who-Is device instance range

Do not issue globally broadcast Who-Is messages (i.e., without 'Device
Instance Range Low/High Limit' parameters), except as the first step of
mapping a system such as by a workstation. In large systems these cause
massive numbers of I-Am broadcasts, causing I-Am responses and other
messages to be lost. It is better to issue a single Who-Is request for
each specific peer device that is to be located.

## 7.6: Space out Who-Is broadcasts

Do not issue a number of Who-Is requests, one immediately after the
next. The resulting flood of Who-Is and I-am messages can overwhelm
routers and slower connections and LANs. It is better to space the
requests out, say, one per second or one per 100 mS, rather than issue
them all at once.

## 7.7: Reduce the rate of repeated Who-Is broadcasts

If it is not absolutely essential that a device’s function requires the
earliest possible establishment of communications with a peer (for
example, if it is not involved in life- or equipment-safety operations),
the rate at which an unanswered Who-Is for that peer is repeated should
be reduced. If part of a system becomes unreachable, say by a router
going offline, the resulting flood of rapidly-repeated Who-Is requests
for devices on the other side of that router can impede PTP
communication and slower LANs.

Where the peer-to-peer communications are less than critical, it is
better to initially repeat the unanswered Who-Is every 10 seconds, or
the time specified by the Device object’s APDU\_Timeout property,
increasing over time to some maximum interval, say, 5 minutes or the
time calculated by multiplying the value of the Device object’s
APDU\_Timeout property by (1 plus the value of APDU\_Retries).

## 7.8: Client devices should support static binding

Client devices should be able to be configured with the network number
and MAC address of a device with which they will be communicating, as an
alternative to the device instance. This will allow them to communicate
with MS/TP subordinates or other devices that do not support Who-Is and
Who-Has execution.

## 7.9: Support for proxy I-Am responses for MS/TP devices does not require Protocol Revision 4

The BTL allows a device to support proxy I-Am responses for MS/TP
devices regardless of the Protocol\_Revision claimed by the device.
Support for the feature is indicated by the presence of the
Slave\_Proxy\_Enable property.

# 8: Data Sharing

The following guidelines apply to all devices with an application layer,
i.e., they acquire data from or provide data to other devices in BACnet
APDUs.

## 8.1: Support ReadPropertyMultiple

Support ReadPropertyMultiple execution (and, if a client device,
ReadPropertyMultiple initiation) even in small devices. The improvement
in network bandwidth when data is being polled is significant.

## 8.2: Clients should be able to fall back to smaller ReadPropertyMultiple requests

Devices that initiate ReadPropertyMultiple requests should be able to
retry with, or be reconfigured to use, ReadPropertyMultiple requests for
fewer property values if there is some indication that the data cannot
be conveyed to the client. This will help ensure interoperation between
the two devices.

The indicators received from the peer device typically are an Abort
(BUFFER\_OVERFLOW) APDU or an Abort (SEGMENTATION\_NOT\_SUPPORTED) APDU
(see Standard 135, Clause 5.4.5.3, transition
'CannotSendSegmented-ComplexACK'). Another indicator could be that a
reply is never received, if there is an intervening connection (LonTalk
or MS/TP LAN, or PTP connection) unable to convey the reply. (See item
2.5.)

## 8.3: Clients should be able to fall back to ReadProperty requests

Devices that initiate ReadPropertyMultiple requests should be able to
revert to ReadProperty requests if the server device does not indicate
(on the server's Device object's Protocol\_Services\_Supported property)
support ReadPropertyMultiple execution , or (as in 7.2) if there is some
indication that the data cannot be conveyed to the client, or if an
Error PDU conveying the code SERVICE\_NOT\_SUPPORTED or a Reject PDU
conveying the code UNRECOGNIZED\_SERVICE is received. This will help
ensure interoperation between the two devices.

## 8.4: Do not poll as fast as the network allows, and polling intervals should be configurable

A device that polls other devices for data should not poll as fast as
the network will allow; this can impede PTP communication and slower
LANs. The interval between polls should also be configurable.

## 8.5: If multiple devices are being polled, alternate the polls

A device that is polling multiple peer devices for data at the same
rates should, if possible, rotate the requests among the peer devices
“round-robin” rather than send a number of ReadProperty (or
ReadPropertyMultiple) requests to one device, then a number of requests
to the next device, and so on.

## 8.6: Do not subscribe for COV notifications with 'Lifetime' set to zero (indefinite lifetime)

Devices should not issue SubscribeCOV or SubscribeCOVProperty requests
with the 'Lifetime' parameter set to zero. (The value zero is prohibited
for SubscribeCOVProperty requests.) The COV subscription list is not
guaranteed to remain through a device reset or power loss, or the device
holding the subscription could fail and be replaced. Worse, if the
subscription list is permanently held and the subscribing device is
removed, the storage for the removed device's subscription remains
allocated forever.

SubscribeCOV requests should be issued with a non-zero 'Lifetime'
parameter for some period (such as the amount of time it is acceptable
for the subscribing device to operate with out-of-date data), and the
subscription should be refreshed periodically.

## 8.7: If a COV subscription fails, poll for data and/or inform the operator

COV subscriptions might not always succeed, even with devices that
execute SubscribeCOV or Subscribe-COVProperty requests. Devices are
permitted to support a limited number of subscriptions. See Standard 135
Clause K.1.12.

## 8.8: COV notifications from proprietary objects should include Present\_Value and Status\_Flags

COV notifications from proprietary objects should include the values of
the Present\_Value and Status\_Flags properties in addition to any other
property values included in the notifications. This is for consistency
with most other COV notifications.

## 8.9: List properties shall be modifiable using WriteProperty

List properties that may be modified using BACnet services shall also be
writable using the WriteProperty service request (and
WritePropertyMultiple, if supported). Modifications of such properties
shall not be restricted (by the server device) to the AddListElement and
RemoveListElement service requests, for better interoperability.

## 8.10: List properties should be modified using AddListElement and RemoveListElement

In order to reduce the potential for conflict when two or more clients
are modifying a list property concurrently, the list should be modified
using AddListElement and RemoveListElement service requests. (However,
if an element of a list is removed by one RemoveListElement request and
a subsequent RemoveListElement request specifies that element, the
latter request will fail in its entirety.)

## 8.11: ReadPropertyMultiple execution shall support ALL, REQUIRED, and OPTIONAL

The BACnet standard __requires__ that
ReadPropertyMultiple service execution supports the special property
identifiers ALL, REQUIRED, and OPTIONAL. See Standard 135, Clause
15.7.2. Some workstation devices rely on these special properties for
accessing other devices’ objects. If no optional properties are
supported by the referenced object when reading with the OPTIONAL
property, then the result shall be an empty 'List of Results' (in the
presence of no other errors). If a property which is not readable using
the ReadPropertyMultiple service is in the specified object and would
match the special property identifier, then the entry for that property
shall contain ‘Error Class’: PROPERTY and ‘Error Code’:
READ\_ACCESS\_DENIED. The Property\_List property, if present in the
object, shall not be returned to ReadPropertyMultiple service requests
specifying the special property identifiers ALL and REQUIRED.

## 8.12: ReadRange execution shall support all list properties

The BACnet standard requires that ReadRange service execution support
any property that is a list. ReadRange service execution is not allowed
to be restricted to logging objects’ Log\_Buffer properties, if there
are other List properties in the device.

## 8.13: Inter-related properties should be read and written together for consistency in their values

Inter-related properties, such as High\_Limit and Low\_Limit should be
read and written together using ReadPropertyMultiple and
WritePropertyMultiple. Although this does not guarantee consistency
between the values, because the operations are not required to be
atomic, it may improve the likelihood of consistency over access using
ReadProperty and WriteProperty services.

## 8.14: Writable Character String properties should support strings of useful length

Writable Character String properties should support strings of useful
length. Although “useful” is a vague term, an Object\_Name property that
can contain at most a four octet Character String is not likely to be
useful to a user, whereas an Object\_Name of length 512 is likely to be
far more than adequate.

## 8.15: Client devices should be able to handle Signed and INTEGER values up to 32 bits

Client devices should be able to handle Signed and INTEGER values at
least up to 32 bits in length, unless the particular value is
specifically restricted by the BACnet standard (such as Unsigned8). This
is the minimum recommended by the BTL for interoperability with other
devices.

## 8.16: Client devices should be able to handle Enumerated values up to 16 bits

Client devices should be able to handle Enumerated values at least up to
16 bits in length, unless the particular value is specifically
restricted by the BACnet standard. This is the minimum recommended by
the BTL for interoperability with other devices.

## 8.17: Time and Date wildcard value rules

Time and Date wildcard values (X’FF’) are specified in Standard 135,
Clauses 20.1.12 and 20.1.13 These values should only be used:

  - in service requests and property values for unspecified time or
    unspecified date,

  - when all elements of the Time or Date value are unknown, or

  - when the standard specifies the use of time pattern, date pattern or
    datetime pattern.

Time and Date values should not use wildcard values when conveying
actual time information, even for the Day of Week or Hundredths of
Seconds. Use of Date wildcard values (X’FF’) as patterns in any
dateRange, including in dateRange calendarEntry elements, is
specifically forbidden. See Standard 135, Clause 12.1.7.

## 8.18: Workstation devices should be able to read all basic datatypes

Operator workstations should be able to read all basic (“application,”
or “primitive”) datatypes (as enumerated in Standard 135, Clause
20.2.1.4 of the BACnet standard), and be able to read basic datatypes
from proprietary properties. This is recommended for improved
interoperability with other devices.

## 8.19: Support as-yet-undefined and proprietary object properties with primitive datatypes

Devices that can read and write properties in other devices should be
configurable to read and write properties in as-yet-undefined standard
objects and proprietary objects, as well as as-yet-undefined standard
properties and proprietary properties in any objects, at least where
those properties have primitive datatypes.

This improves the ability of such devices to access new objects as they
are defined, as well as access to other manufacturers' proprietary
objects.

## 8.20: Use detailed error reporting when all ReadPropertyMultiple accesses fail

The BTL recommends that if all of a ReadPropertyMultiple request's
accesses fail, the response should be a 'Result(+)' primitive returning
an error class and code for each access rather than a 'Result(-)'
primitive conveying a single error code and class, particularly where
the error classes and codes differ for different accesses.

## 8.21: ReadPropertyMultiple data values are returned in the order requested

The data values conveyed in a ReadPropertyMultiple-ACK shall be returned
in the order in which they are requested in the
ReadPropertyMultiple-Request. See Standard 135, Clause 15.7.2.

## 8.22: Support NaN and +/−INF values for REAL and Double datatypes

IEEE REAL and Double numbers have encodings for NaN (Not a Number) as
well as plus and minus infinity. If such a value is written to the
device, an error reply may be appropriate. Be prepared to process such a
value "appropriately" if it is returned in a response to a request such
as ReadProperty.

## 8.23: The absence of a Priority\_Array property does not indicate the Present\_Value is read only

The Present\_Value property may be writable even if the value object
does not support the Priority\_Array property.

## 8.24: Ignore Priority and NULL for Non-Commandable object writes

Clause 19.2.1.1 Commandable Properties specifies the list of
commandable object properties (only present-value for some objects),
and this table defines the list of commandable objects.
If the priority-array property for the object is optional
and not implemented for an object instance, then the object
is not commandable. The Channel object is commandable and does
not include a priority-array.

If any property is writable but the object is not commandable
(does not contain a priority-array or is a Channel object),
the device shall process a WriteProperty, WriteGroup or
WritePropertyMultiple request and ignore the priority parameter
if it is included in the request.

If an attempt is made to relinquish a present-value property but
the object is not commandable, the property shall not be changed,
and the write shall be considered successful [PR 21+].

## 8.25: Server devices are required to support all COV lifetime values up to 8 hours (28800 seconds)

Devices that execute SubscribeCOV or SubscribeCOVProperty (COV servers)
accept all lifetime values in the range 1 through 28800 seconds (8
hours).

## 8.26: Client devices are required to support a COV lifetime value within 1 to 28800 seconds

COV clients must be able to subscribe with a lifetime in the range 1
through 28800 seconds. Client devices are not required to support the
entire range and are they are allowed to support values out of that
range.

##

## 8.27: INTEGER datatype encoding differs from Unsigned encoding

INTEGER datatype encoding differs in its data from Unsigned encoding, by
the existence of a leading 0 octet on positive values whose high bit in
the second octet is set. A common programming mistake of omitting that
leading 0 octet, for example the value 180, would make the value instead
equal -76.

# 9: Arrays

The following guidelines cover the implementation of arrays, including
the PRIORITY\_ARRAY. See Standard 135, Clause 19.2.

## 9.1: Support for array element 0 is required

Support for array element 0, which reports the size of the array, is
required by the BACnet standard for all arrays, including the Device
object’s Object\_List property. See Standard 135, Clause 12.1.5.1. The
BACnet standard defines the datatype of array element 0 to be Unsigned
for all arrays, regardless of the datatype of the other elements.

## 9.2: Arrays shall be readable in their entirety (except…)

Array properties shall be readable in their entirety, such as with the
ReadProperty service, except when the response is too large to fit in a
single APDU and either the client or the server device does not support
segmentation. Devices cannot arbitrarily restrict how an array property
can be read.

## 9.3: Resizable arrays

Standard 135, Clause 12.1.5.1 describes the requirements for resizing
array properties.

## 9.4: Client devices should be prepared for array indexes of at least 16 bits

Client devices need to handle array indexes of at least 16; this
includes the size of array element 0, which reports the size of the
array.

## 9.5: All 16 priority array levels shall be present

When implementing priority arrays, all 16 levels shall be present. The
BACnet standard does not allow an implementation to have fewer levels.

## 9.6: Priority array level 6 is not writable

The BACnet standard reserves Level 6 of the priority array for the
Minimum On/Off Time algorithm and prohibits its use for any other
purpose in any object. See Standard 135, Clause 19.2.3.

## 9.7: If the device cannot afford a priority array in an "Output" object, use a "Value" object

All output objects shall have priority arrays, but value objects are not
required to have them. If the device needs to have an object that
represents a physical output but cannot afford a priority array, use the
corresponding "Value" object type instead (for example, a Binary Value
object instead of a Binary Output).

## 9.8: Re-evaluate priority array as soon as possible upon write

When the Present\_Value of an object that contains a Priority\_Array is
written, the Priority\_Array must be evaluated, and the value of the
Present\_Value must be updated before the next service is processed.
Preferably before the write is acknowledged. This avoids the risk of a
race condition between updating the Present\_Value and a read service
request reading the property.

# 10: Alarms

The following guidelines apply to devices that initiate or receive
alarms, i.e. ConfirmedEventNotification or UnconfirmedEventNotification
messages.

## 10.1: Use the standard event algorithms if at all possible

When implementing event initiation, use the standard event algorithms if
at all possible. See Standard 135, Clauses 13.3 and 13.4.

## 10.2: Parse all event notifications

Client devices that receive and process event notifications must be able
to receive and process proprietary event types, where the construction
of the ‘Event Values’ (the notification parameters) is not known. The
parser should be able to parse through this field, even though the
values will be discarded, in order that the notification can be received
and processed.

## 10.3: Support all forms of BACnetRecipient and the DM-DDB-A BIBB

To receive a BTL Certificate, devices that generate alarms or events
(i.e. support the AE-N-I-B, AE-N-E-B, or T-ATR B BIBBs) must support all
forms of the ‘Recipient’ parameter of BACnetDestination elements of the
Recipient\_List property of the Notification Class object. In addition,
the DM-DDB-A BIBB (initiate the Who-Is service in order to locate alarm
recipients) must be supported.

## 10.4: Support the GetEventInformation service

Devices that generate alarms or events (i.e. support the AE-N-I-B or
AE-N-E-B BIBBs) must support execution of the GetEventInformation
service to obtain BTL Certification.

## 10.5: The Notification Class object’s Recipient\_List property must be persistent

The contents of the Notification Class object’s Recipient\_List property
should not be lost when the device is powered down or reset. There is no
standard mechanism for restoring this information from some other source
when the device starts up.

## 10.6: Recipient\_List writability

A device is allowed restrict some but not all parameters of a writable
Recipient\_List. See Standard 135, Clause 12.21.8.

## 10.7: Support accepting both ConfirmedEventNotification and UnconfirmedEventNotification

To obtain BTL Certification, a device that receives alarms or events,
shall be able to execute both ConfirmedEventNotification and
UnconfirmedEventNotification service requests.

## 10.8: Accept any Acknowledging Process Identifier in the AcknowledgeAlarm service request

A device must accept any Acknowledging Process Identifier parameter in
the AcknowledgeAlarm service.

## 10.9: Use the event priorities found in Annex M

BTL recommends using the event priorities defined in Standard 135, Annex M
for deployments.

## 10.10: To indicate “all day” in BACnetDestination use 00:00:00.00 to 23:59:59.99

For client devices configuring a BACnetDestination, indicate that a
particular destination is viable all day by using 00:00:00.00 for ‘From
Time’ and 23:59:59.99 for ‘To Time’.

## 10.11: If the intrinsic alarming properties are present in an object, it shall support intrinsic alarming

If the properties which support intrinsic alarming are present in an
object, it shall support intrinsic alarming. If the object supports
intrinsic alarming solely for FAULT reporting, the properties which
support intrinsic alarming are nonetheless all present, including
vestigial properties such as Limit\_Enable and Time\_Delay which are not
operative for FAULT reporting.

## 10.12: Rapid transitions in the OUT\_OF\_RANGE and FLOATING\_LIMIT algorithms

The behavior of the OUT\_OF\_RANGE and FLOATING\_LIMIT algorithms when
the Present\_Value transitions faster than Time\_Delay between the
ranges associated with the High\_Limit and Low\_Limit states is
specified in Standard 135, Clauses 13.3.5 and 13.3.6.

# 11: Calendars and Schedules

The following guidelines apply to devices that contain Calendar and
Schedule objects, and to workstations that can modify those objects.

Note: the BTL requires that Schedule objects be implemented per
Protocol\_Revision 4 or higher.

## 11.1: Schedules should be modifiable

Unless the application dictates otherwise the Weekly\_Schedule and
Exception\_Schedule properties of the Schedule object and the Date\_List
property of the Calendar object should be modifiable, preferably using
standard BACnet services. This allows operator workstations from
multiple manufacturers to modify schedules.

## 11.2: Schedules should be capable of numerous weekday entries and exception entries

The Scheduling BIBBs (K.3.2 in the BACnet standard) require only six
entries per day in the Weekly\_Schedule and one entry in the
Exception\_Schedule. For improved usefulness, unless the application
dictates otherwise, devices should support more entries than that,
especially in the Exception\_Schedule.

## 11.3: Schedule objects and basic datatypes

If the List\_Of\_Object\_Property\_References property is writable it
must be able to schedule at least a minimum set of datatypes:

|         |          |            |
| ------- | -------- | ---------- |
| NULL    | Unsigned | REAL       |
| BOOLEAN | INTEGER  | Enumerated |

If a Schedule object supports only SCHED-I-B (that is, it only writes to
properties within the same device as the Schedule object), if the
List\_Of\_Object\_Property\_Reference property references any
commandable objects in the device, then the Schedule object shall be
capable of scheduling NULL values.

## 11.4: Workstations shall be able to configure schedules with NULL datatype

Workstations shall be able to configure Schedule object schedules to
write the NULL datatype. This allows the Schedule object to relinquish a
value previously written to a priority array.

## 11.5: Workstations - Schedule object datatype Support

Operator workstations shall be able to view and modify Schedule objects
that schedule REAL, Enumerated, BOOLEAN, and Unsigned32 datatypes, and
interoperate with Schedules that contain NULL values.

## 11.6: Schedule objects scheduling Unsigned and INTEGER values should support 32 bit values

Schedule objects that are able to schedule Unsigned and INTEGER values
should be able to schedule, at a minimum, 32 bit values for purposes of
interoperability.

## 11.7: Schedule objects scheduling Enumerated values should support 16 bit values

Schedule objects that are able to schedule Enumerated values should be
able to schedule, at a minimum, 16 bit values for purposes of
interoperability.

## 11.8 NULL is a valid value for the Schedule\_Default property

Scheduling prior to revision 4 allowed the use of the NULL value in the
TimeValue pair of the Weekly or Exception Schedule in order to indicate
that the schedule has completed and control should return to the object.
This allows the schedule object to relinquish control of the controlled
objects.

In a Protocol\_Revision 4 or later Schedule, the only way for the
schedule to relinquish control is to place a NULL in the
Schedule\_Default property.

## 11.9: Be prepared to read the Exception\_Schedule array element by element

Some devices , even those that support segmentation, have
Exception\_Schedule property values that are too large to transmit in a
single APDU, even if segmented. If a device needs to read another’s
Exception\_Schedule property value, be prepared to read it array element
by array element.

# 12: Trending

The following guidelines apply to devices that contain Trend Log objects
or that obtain trending data using the ReadRange service.

## 12.1: Implement the most recent Trend Log and ReadRange

The Trend Log object and ReadRange service shall be implemented as
revised in Addendum 135-2001*b* (and as found in BACnet-2004). These are
more robust in guaranteeing properly ordered data.

## 12.2: ReadRange clients shall use a 'Count' parameter in the range of INTEGER16

The BACnet standard restricts the range of values used for the 'Count'
parameter of the ReadRange service, so that it is unnecessary for the
server devices to handle the full BACnet-encodable range of the INTEGER
datatype. Client devices shall restrict the range they use to INTEGER16
to ensure interoperation. This restriction should be observed in all
BACnet implementations, and is mandated in Addendum 135-2010*ad* in
Protocol\_Revision 13 of the BACnet standard.

# 13: Routing

The following guidelines apply to BACnet routers.

## 13.1: Drop messages with a Hop Count of zero

When a message with a Hop Count equal to zero is received, it shall be
discarded. A message with a Hop Count of zero should never be seen on a
LAN; this is a defence against a non-conformant router.

## 13.2: Drop messages if Hop Count is decremented past zero

If the Hop Count of a received message has a value which, after being
decremented, will have a negative or zero value, the message shall be
discarded.

## 13.3: Add known networks to empty Router-Busy-To-Network messages

The BACnet standard permits a router to initiate Router-Busy-To-Network
messages with an empty list of networks. A router that receives such a
message (with an empty list) shall add a list of all networks it knows
to be reachable through the initiating router before forwarding the
Router-Busy-To-Network to its other ports.

## 13.4 Routers should not redirect broadcast messages

The BACnet standard allows a device to locate the router to a remote
network by broadcasting a confirmed request on the local network and
observing the SA conveyed in the response (Clause 6.5.3, paragraph 2).
In order to avoid unnecessary duplicate messages, a router should drop
confirmed-request messages sent with a broadcast MAC address where the
destination network is not reachable through the router.

## 13.5 Routers should forward unknown network message types

The BACnet standard provides for a Reject-Message-To-Network with a
reject reason octet of X’03’, specifying “It is an unknown network layer
message type.” This response should only be returned by the device to
which the message is addressed, not by an intervening router. (Routers
implemented to Protocol\_Revision 4 or higher shall convey unknown
network layer message types not addressed to them.) This allows new
message types to be conveyed across networks using older routers.

## 13.6 Routers should support the maximum frame size for each supported medium

Routers should always support the maximum frame size for each supported
LAN type, as defined in **Table 6-1** of the BACnet standard.

# 14: Backup and Restore

The following guidelines apply to devices that can have their
configurations backed up, or are capable of backing up and restoring
other devices’ configurations, per clause 19.1 of the BACnet standard.

## 14.1: Size the configuration File objects before restoring

If the File\_Size property value does not match the size of the
configuration file content to be restored, clear the contents of the
file object by writing 0 to the File\_Size property. Then start writing
the data in order.

If the client writes a File\_Size to the server when the size does not
need to be changed, the server may respond with an error. The client
shall continue in this case by writing the data to the server in order.

## 14.2: A device should be able to handle an interrupted Restore operation

In the case where a Restore operation fails, Standard 135, Clause
19.1.3.4 states: "Every attempt shall be made to leave device B in a
state that will accept additional restore procedures."

## 14.3: Empty files shall be backed up and restored if listed in the Configuration\_Files property

If a file is listed in the Configuration\_Files property it shall be
backed up and restored even if its File\_Size property indicates a size
of zero.

# 15: Annex J BACnet/IP

The following guidelines apply to devices that implement Annex J of
BACnet-2004, BACnet/IP.

## 15.1: A non-BBMD BACnet/IP device shall be able to register as a Foreign Device

A BACnet/IP device that does not contain a BBMD shall be able to
register as a Foreign Device with a BBMD. This prevents situations where
a BACnet/IP device cannot communicate due to an inability to receive
broadcasts.

## 15.2: A Broadcast Distribution Table should be able to contain more than four entries

A BBMD's Broadcast Distribution Table must be able to hold at least five
entries. The BDT should be much larger and contain at least 32 entries.

## 15.3: Two-hop BBMD entries are required – one-hop support is not required

The BTL is only requiring that a BBMD's Broadcast Distribution Table be
able to support a netmask of 255.255.255.255. This is in accordance with
observed practices and policies for Annex J installations, where one-hop
support (from IP routers) is generally disallowed.

## 15.4: BBMDs shall support Foreign Device registrations

The BTL requires BBMDs to support Foreign Device registrations which
imply a connection across a larger internet. This is to ensure that any
time a BBMD is present in an Annex J B/IP network foreign devices can
connect to the B/IP network.

## 15.5: A Foreign Device Table should be able to contain at least two entries

The BTL requires that a BBMD's Foreign Device Table be able to hold at
least two entries. If at all possible the BTL recommends support for
more entries in an FDT.

## 15.6: A BBMD shall be capable of forwarding Forwarded-NPDU broadcasts to its local subnet

Annex J.4.5, paragraph four, allows a BBMD that receives a
Forwarded-NPDU to omit the broadcast on its local subnet (using the B/IP
broadcast address) if no other BACnet devices are present on its local
subnet. The BTL requires that a BBMD be capable of forwarding the
broadcasts in order to support the presence of other BACnet devices on
the subnet.

## 15.7: BACnet/IP devices should support classless addressing

BACnet/IP devices should support classless IP addressing, because this
is frequently encountered in installations.

## 15.8: Routers supporting BACnet/IP shall support fragmentation

Routers which support Annex J BACnet/IP routers shall support
fragmentation (and re-assembly) of IP frames. This permits full-sized
Ethernet frames (APDU size 1470 octets) to be conveyed across IP
networks.

# 16: Miscellaneous

This section lists items that don't fit anywhere else.

## 16.1 Context tags greater than 14 shall be properly handled by a parser

Although early versions of the BACnet standard contained no standard
context tags greater than 13, it is not unreasonable to expect context
tag numbers across the complete range, including extended tags in the
range 15 and higher. Parsers shall handle all tag numbers, including
those 15 and higher without crashing. The method for encoding tag
numbers of 15 and higher is described in clause 20.2.1.2 of the BACnet
standard.

## 16.2: Implement the Abort reasons of Clause 5.4.5.3 (BACnet-2004)

The BTL suggests that the Abort reasons of Clause 5.4.5.3 (in
BACnet-2004, introduced in Addendum 135-2001*c*-7) be implemented,
regardless of the Protocol\_Revision claimed by the device.

## 16.3: Use of error codes from higher Protocol\_Revisions levels may be supported

The BTL suggests that error codes and usages introduced in a later
Protocol\_Revision may be implemented regardless of the
Protocol\_Revision claimed by the device.

## 16.4: BACnet-EIB/KNX mapping may be done independent of Protocol\_Revision

The BTL allows a device to implement the BACnet to EIB/KNX mapping
defined in Annex H.5 and introduced in Addendum 135-2001*d*-1 without
regard to the Protocol\_Revision claimed by the device.

## 16.5: Non-run-time data should survive a power reset

Property values in objects that are not a reflection of runtime status
should survive a power reset. For example, relinquish-default, object
names, descriptions, etc.

# 17: Time

The following guidelines apply to Time within the BACnet system.

## 17.1: Aligned\_Intervals for time synchronization is aligned to Local\_Time and Local\_Date properties

The alignment of the Align\_Intervals property in the Device object is
relative to Local\_Date and Local\_Time rather than to UTC time even for
the issuance of UTCTimeSynchronization. This means TimeSynchronization
and UTCTimeSynchronization requests will be issued at approximately the
same time.

# 18: Resources

Resources for implementers of BACnet devices are listed here.

## 18.1: BACnet standards

### 18.1.1: ANSI/ASHRAE 135 and ASHRAE 135.1

ANSI/ASHRAE 135 (the BACnet standard) and ASHRAE 135.1 (the testing standard) are available in print and downloadable PDF from:
[http://www.techstreet.com](http://www.techstreet.com/ashraegate.html).
Search for “ASHRAE 135” or for ASHRAE 135.1.” All past versions of the
BACnet standard and the testing standard are available for purchase from
the ASHRAE bookstore. Access to those earlier versions is recommended,
to understand the history of changes in the standard, and to read the
original wording of clauses which have been revised or deprecated.

### 18.1.2: Addenda

**Addenda*** (additions) may be downloaded from ASHRAE's website at:

<https://ashrae.org/technical-resources/standards-and-guidelines/standards-addenda>

### 18.1.3: Errata

Errata (corrections) to the BACnet standards may be downloaded from
ASHRAE's website at:

<https://ashrae.org/technical-resources/standards-and-guidelines/standards-errata>

### 18.1.4: Interpretation Requests

Interpretation Requests (clarification of particular issues in the
standard) may be downloaded from ASHRAE's website at:

<https://ashrae.org/technical-resources/standards-and-guidelines/standards-interpretations>

### 18.1.5: ISO Standard 16484-5

ISO Standard 16484-5 (the BACnet standard) is available from:

<https://www.iso.org/standard/84964.html>

## 18.2: The BACnet Committee website

The BACnet committee (ASHRAE/SSPC 135) maintains a website at
<https://bacnet.org/>. This site presents the latest news from the
BACnet world, links to various BACnet organizations, a bibliography of
BACnet articles, a list of BACnet Vendor IDs plus vendor contact
information, and much more.

## 18.3: The BACnet Testing Labs website

The BACnet Testing Labs (BTL) website is found at: bacnetlabs.org . BTL
supports compliance testing and interoperability testing activities and
consists of the BTL Manager and the BTL Working Group. Resources for BTL
Testing and links to all Recognized BACnet Testing Organizations can be
found on this site. The BTL Test Package, BTL Testing Policies, BTL
Implementation Guidelines and BTL Clarification Requests are found on
<https://www.bacnetlabs.org/page/test_documentation> .

## 18.4: Communications and Announcements:

There are several organizational (open to members) and
regionally-oriented BACnet e-mail lists maintained by various
organizations. There are also a couple of general lists, as follows:

### 18.4.1: BACnet-L

BACnet-L is a list for general discussions of all things BACnet.

<https://bacnet.org/bacnet-l>

### 18.4.2: BTL Announcements

Publications of BTL Test Package and BTL Testing Policy are shared in
the Journal, which is distributed to all BACnet International members
including manufacturers, integrators, and end users. Additionally, this
information is shared on BACnet International's LinkedIn and Twitter
accounts and in the monthly Corporate Update.

Major announcements are distributed on the newswire and picked up by
various editors and publications. These are also posted on the BACnet
International website and the BTL website and shared on LinkedIn and
Twitter.

New versions of BTL testing documents and upcoming meetings and event
for the BTL Working Group are shared on the BTL-WG Forum. Request access
to the BTL Working Group Forum by emailing
btl-wgchair@bacnetinternational.org.
