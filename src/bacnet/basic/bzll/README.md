# README

## BACnet ZigBee Link Layer (BZLL) Integration Guide

### Overview

This document describes the required steps to integrate a BACnet stack with a generic ZigBee stack using BACnet ZigBee Link Layer (BZLL).

The document covers:
- required ZigBee clusters;
- initialization requirements;
- group management;
- BACnet address synchronization;
- address conflict handling;
- validation procedures;
- BACnet NPDU transport over ZigBee.

---

# 1. Required ZigBee Cluster Implementation

All nodes in the ZigBee network shall implement the following clusters on the same endpoint:

## Required clusters

### Generic Tunnel Cluster
- Client
- Server

### BACnet Protocol Tunnel Cluster
- Client
- Server

### Groups Cluster
- Server

## Coordinator requirements

The device responsible for network creation and management — preferably the ZigBee coordinator — shall additionally implement:

### Groups Cluster
- Client

## Endpoint requirements

All clusters shall:
- be implemented on the same ZigBee endpoint;
- use the Commercial Building Automation (CBA) profile.

## Purpose

The Generic Tunnel and BACnet Protocol Tunnel clusters allow:
- encapsulation of BACnet payloads;
- BACnet communication transport over ZigBee;
- bidirectional BACnet communication between ZigBee devices.

The Groups cluster is used for:
- logical device grouping;
- multicast communication;
- BACnet broadcast transport.

---

# 2. BZLL Initialization

During device startup, the integration layer between the BACnet stack and the ZigBee stack shall configure all required BZLL attributes and callbacks.

This initialization shall occur before the device participates in BACnet communications over ZigBee.

---

# 3. Generic Tunnel Attribute Initialization

The Generic Tunnel Cluster attributes shall reflect the parameters assigned by the BACnet stack.

The following attributes shall be updated during initialization:

- Maximum Incoming Transfer Size
- Maximum Outgoing Transfer Size
- Protocol Address

The values shall be obtained from the BACnet stack through the following APIs:

```c
bzll_get_maximum_incoming_transfer_size(...)
```

```c
bzll_get_maximum_outgoing_transfer_size(...)
```

```c
bzll_get_my_protocol_address(...)
```

## Purpose

This synchronization guarantees:
- correct transfer capability advertisement;
- proper BACnet protocol addressing;
- interoperability between BZLL nodes.

---

# 4. Callback Registration

The integration layer shall register all callbacks required for communication between the BACnet stack and the ZigBee transport layer.

The following callbacks shall be registered:

- Send MPDU
- Get My Address
- Advertise Address
- Request Read Property Address

The callbacks shall be registered through:

```c
bzll_send_mpdu_callback_set(...)
```

```c
bzll_get_my_address_callback_set(...)
```

```c
bzll_advertise_address_callback_set(...)
```

```c
bzll_request_read_property_address_callback_set(...)
```

## Callback responsibilities

### Send MPDU
Responsible for:
- transmitting BACnet MPDUs over ZigBee;
- selecting unicast or broadcast transport;
- initiating payload transmission.

### Get My Address
Responsible for:
- returning the EUI-64 address and cluster endpoint.

### Advertise Address
Responsible for:
- advertising the BACnet protocol address.

### Request Read Property Address
Responsible for:
- requesting BACnet address discovery through property reads.

---

# 5. Recommended Initialization Sequence

1. Initialize ZigBee stack;
2. Initialize BACnet stack;
3. Obtain local BACnet parameters;
4. Update Generic Tunnel attributes;
5. Register BZLL callbacks;
6. Join ZigBee network;
7. Complete commissioning;
8. Join BACnet/ZigBee group.

No BACnet payload shall be transmitted before all initialization steps are completed.

---

# 6. BACnet/ZigBee Group Association

After a new node joins and is commissioned into the ZigBee network, the coordinator shall determine whether the device supports BZLL communication.

## Identification criteria

The node shall:
- implement an endpoint using the Commercial Building Automation profile;
- implement:
  - Generic Tunnel Cluster;
  - BACnet Protocol Tunnel Cluster.

## Coordinator responsibilities

The coordinator shall:
1. detect the new device;
2. perform endpoint discovery;
3. validate supported profile and clusters;
4. add the endpoint to the BACnet/ZigBee group;
5. ensure the coordinator itself belongs to the same group.

## Broadcast group registration

The broadcast group identifier shall be configured through:

```c
bzll_set_broadcast_group_id(...)
```

## Purpose

The group shall be used for:
- BACnet broadcast transport;
- multicast BACnet communication;
- communication between all BZLL nodes.

---

# 7. BACnet Address Discovery and Synchronization

After joining the BZLL group, nodes shall synchronize BACnet addresses across the network.

## Protocol address advertisement

Once associated with the BZLL group, the device shall advertise its BACnet Protocol Address.

The address shall be obtained through:

```c
bzll_get_my_protocol_address(...)
```

## Multicast address discovery request

The device shall also perform a multicast attribute read request targeting the configured Group ID.

The purpose is:
- discovering all BZLL participants;
- populating the BACnet address table;
- synchronizing address mappings.

## Node response behavior

Upon receiving the multicast attribute read request, each node shall respond with its own Protocol Address.

The address shall be obtained using:

```c
bzll_get_my_protocol_address(...)
```

## Address table update

Whenever an address advertisement is received, the local BACnet mapping table shall be updated through:

```c
bzll_update_node_protocol_address(...)
```

## Result

After synchronization:
- all nodes know the BACnet addresses of all participants;
- unicast BACnet communication becomes possible.

---

# 8. BACnet Address Conflict Handling

The coordinator may centrally manage BACnet protocol addresses.

## Conflict detection

The coordinator may detect:
- duplicated addresses;
- invalid addresses;
- missing addresses;
- inconsistent advertisements.

## Remote address reassignment

If necessary, the coordinator may perform a Write Attribute operation on the Protocol Address attribute of the target node.

## Node behavior after reassignment

After receiving a new Protocol Address:
1. the node shall update its internal BACnet address;
2. synchronize the new value with the BACnet stack;
3. perform a new address advertisement.

## Network update

All receiving nodes shall update their local mapping tables through:

```c
bzll_update_node_protocol_address(...)
```

## Result

After reassignment:
- address conflicts are resolved;
- all routing tables become synchronized.

---

# 9. BACnet Address Validation

The coordinator may validate whether all nodes correctly registered their BACnet addresses.

This validation is performed through:

- Send Match Protocol Address Command

## Operation

The coordinator sends a Match Protocol Address request containing the expected BACnet address.

Upon receiving the command, the node shall pass the received address to:

```c
bzll_match_protocol_address(...)
```

## Purpose

The BACnet stack shall:
- compare the received address against the local address;
- validate address consistency;
- return the validation result.

## Response

The validation result shall be returned to the command originator.

## Result

This mechanism confirms:
- proper address synchronization;
- consistency between the BACnet stack and Generic Tunnel attributes.

---

# 10. BACnet Payload Transmission and Reception

After all previous steps are completed, the BACnet stack becomes capable of transmitting and receiving BACnet communications over ZigBee.

---

# 11. BACnet Payload Transmission

BACnet transmission occurs through the Send MPDU callback registered during initialization.

When the callback is executed, it receives:
- a 64-bit IEEE address;
- the destination endpoint;
- the BACnet payload.

## Broadcast transmission

If the IEEE address is:

FF:FF:FF:FF:FF:FF:FF:FF

the implementation shall interpret the transmission as a BACnet broadcast.

The payload shall then be transmitted through ZigBee multicast using the configured Group ID.

## Unicast transmission

If the IEEE address differs from:

FF:FF:FF:FF:FF:FF:FF:FF

the implementation shall perform a ZigBee unicast transmission.

## Address resolution

If necessary, the ZigBee implementation may resolve:
- IEEE 64-bit address;
- to ZigBee short address.

## NPDU transport

The BACnet payload shall be encapsulated and transported using the BACnet Protocol Tunnel command:

- Transfer NPDU

---

# 12. BACnet Payload Reception

Reception occurs when the BACnet Protocol Tunnel Cluster receives the:

- Transfer NPDU command

Upon reception:
1. the BACnet payload shall be extracted;
2. the NPDU shall be forwarded to the BACnet stack through:

```c
bzll_receive_pdu(...)
```
