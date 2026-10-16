# Security Policy

## Supported Versions

The following versions of the BACnet Stack C library are
currently being supported with security updates in this branch:

| Version | Supported          |
| ------- | ------------------ |
| 1.6.x   | :white_check_mark: |

## Coordinated Vulnerability Disclosure

Vulnerabilites are disclosed to [CVE](https://www.cve.org/)
or [GHSA](https://github.com/bacnet-stack/bacnet-stack/security/advisories)
and a record is created to identify, define, and catalog publicly disclosed
cybersecurity vulnerabilities.

BACnet confirmed response source is not bound to the pending TSM transaction
[GHSA-m3mw-mwm8-g846](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-m3mw-mwm8-g846)
Patched versions: 1.6.2
Pull Request: [#1573](https://github.com/bacnet-stack/bacnet-stack/pull/1573).

Out-of-bounds read in bacnet_timer_value_decode via unsigned length underflow (Timer State_Change_Values)
[GHSA-gp2v-g6xp-3vj7](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-gp2v-g6xp-3vj7)
Patched versions: 1.6.2
Pull Request: [#1569](https://github.com/bacnet-stack/bacnet-stack/pull/1569).

Out-of-bounds read in bacnet_ltrim: skip loop never stops at the terminating NUL (reached via WeeklySchedule ASCII parser)
[GHSA-85x2-2p8v-rjhm](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-85x2-2p8v-rjhm)
Patched versions: 1.6.2
Pull Request: [#1568](https://github.com/bacnet-stack/bacnet-stack/pull/1568).

BACnet/SC (Linux port): sc-hub example accepts ReinitializeDevice with default password
[GHSA-r8wc-8mcp-hjfc](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-r8wc-8mcp-hjfc)
Patched versions: 1.6.2
Pull Request: [#1567](https://github.com/bacnet-stack/bacnet-stack/pull/1567).

BACnet State_Text array resize can exhaust CPU and heap remotely
[GHSA-gh4p-3vmc-6mqw](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-gh4p-3vmc-6mqw)
Patched versions: 1.6.2
Pull Request: [#1518](https://github.com/bacnet-stack/bacnet-stack/pull/1518).

BACnet Schedule list WriteProperty can enter a zero-progress CPU loop
[GHSA-9g5v-8wwr-pr5g](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-9g5v-8wwr-pr5g)
Patched versions: 1.6.2
Pull Request: [#1535](https://github.com/bacnet-stack/bacnet-stack/pull/1535).

Off-by-one out-of-bounds read in router-mstp NPDU handler
[GHSA-wgjm-9rv6-ph6j](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-wgjm-9rv6-ph6j)
Patched versions: 1.6.2
Pull Request: [#1534](https://github.com/bacnet-stack/bacnet-stack/pull/1534).

Stack buffer overflow in bacnet_constructed_value_decode() via Timer object WriteProperty (build-flag-gated)
[GHSA-6272-3rw9-xr2j](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-6272-3rw9-xr2j)
Patched versions: 1.6.2
Pull Request: [#1510](https://github.com/bacnet-stack/bacnet-stack/pull/1510).

BACnet/SC hub: unauthenticated peer acceptance and unbound identity claims enable node preemption and fabricated device data
[GHSA-gv5j-fc8m-6gj7](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-gv5j-fc8m-6gj7)
Patched versions: 1.6.2
Pull Request: [#1503](https://github.com/bacnet-stack/bacnet-stack/pull/1503).

Deprecated Decoder Out-of-Bounds Read (BN-05)
[GHSA-f96q-cj8v-939q](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-f96q-cj8v-939q)
Patched versions: 1.6.1

Signed Integer Overflow in bacnet_enclosed_data_length() Leads to Heap Memory Corruption
[GHSA-g68j-34rj-cmhv](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-g68j-34rj-cmhv)
Patched versions: 1.6.1
Pull Request: [#1468](https://github.com/bacnet-stack/bacnet-stack/pull/1468).

BVLC Header Encoder Out-of-Bounds Write (BN-06)
[GHSA-3q5x-2r73-c55v](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-3q5x-2r73-c55v)
Patched versions: 1.6.1
Pull Request: [#1467](https://github.com/bacnet-stack/bacnet-stack/pull/1467).

Buffer overflow from 8-bit integer overflow in bacnet_enclosed_data_length() function
Patched versions: 1.6.1
Pull Request: [#1466](https://github.com/bacnet-stack/bacnet-stack/pull/1466).

Cross-peer disclosure caused by a BVLC length mismatch
[GHSA-rxvq-3mhq-474x](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-rxvq-3mhq-474x).
Patched versions: 1.6.1
Pull Request: [#1451](https://github.com/bacnet-stack/bacnet-stack/pull/1451).

Silent decode failure in bacnet_octet_string_decode() due to discarded return value - oversized octet strings reported as successfully decoded
[GHSA-9qx8-hr5x-r35c](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-9qx8-hr5x-r35c).
Patched versions: 1.6.1
Pull Request: [#1440](https://github.com/bacnet-stack/bacnet-stack/pull/1440).

AtomicWriteFile access-method mismatch leading to RAMFS heap OOB read
[GHSA-gv7j-28x8-cr37](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-gv7j-28x8-cr37).
Patched versions: 1.6.1
Pull Request: [#1439](https://github.com/bacnet-stack/bacnet-stack/pull/1439).

Router route-table Tx_Buffer overflow
[GHSA-gj7v-fwjp-7x8q](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-gj7v-fwjp-7x8q).
Patched versions: 1.6.1
Pull Request: [#1438](https://github.com/bacnet-stack/bacnet-stack/pull/1438).

Structured View subordinate-list[0] WriteProperty allows unbounded synchronous resize and remote unauthenticated DoS
[GHSA-hg85-pmm3-jfcf](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-hg85-pmm3-jfcf).
Patched versions: 1.6.1
Pull Request: [#1437](https://github.com/bacnet-stack/bacnet-stack/pull/1437).

[CVE-2026-102612](https://www.cve.org/CVERecord?id=CVE-2026-102612) -
BACnet/SC hub never requires a client cert, so any host joins the mesh unauthenticated
[GHSA-92q2-p4vr-fvmp](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-92q2-p4vr-fvmp).
Patched versions: 1.6.1
Pull Request: [#1436](https://github.com/bacnet-stack/bacnet-stack/pull/1436).

[CVE-2026-97692](https://www.cve.org/CVERecord?id=CVE-2026-97692) -
Trailing MORE bit on the last BACnet/SC header option walks the option list past the end of the message,
causing a pre-auth out-of-bounds read escalating to an out-of-bounds write into global memory
[GHSA-gr74-333w-7wg8](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-gr74-333w-7wg8).
Patched versions: 1.6.1
Pull Request: [#1435](https://github.com/bacnet-stack/bacnet-stack/pull/1435).

[CVE-2026-102537](https://www.cve.org/CVERecord?id=CVE-2026-102537) -
bacnet-stack BACnet/SC node always accepts a self-signed hub cert, so an attacker can MITM it
[GHSA-jgm4-2wg9-jwfg](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-jgm4-2wg9-jwfg).
Patched versions: 1.6.1
Pull Request: [#1434](https://github.com/bacnet-stack/bacnet-stack/pull/1434).

[CVE-2026-100403](https://www.cve.org/CVERecord?id=CVE-2026-100403) -
BACnet/SC hub grows its WebSocket reassembly buffer without limit, so a peer can exhaust its memory
[GHSA-4vgv-v5ph-rmxr](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-4vgv-v5ph-rmxr).
Patched versions: 1.6.1
Pull Request: [#1433](https://github.com/bacnet-stack/bacnet-stack/pull/1433).

[CVE-2026-82414](https://www.cve.org/CVERecord?id=CVE-2026-82414) -
MS/TP COBS decode overflow
[GHSA-8456-m9x4-j6mc](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-8456-m9x4-j6mc).
Patched versions: 1.6.1
Pull Request: [#1425](https://github.com/bacnet-stack/bacnet-stack/pull/1425).

[CVE-2026-52790](https://www.cve.org/CVERecord?id=CVE-2026-52790) -
bacnet_device.c stack-use-after-return in writable Device string properties
[GHSA-jr7p-rm2x-739x](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-jr7p-rm2x-739x).
Patched versions: 1.6.0
Pull Request: [#1375](https://github.com/bacnet-stack/bacnet-stack/pull/1375).

[CVE-2026-45341](https://www.cve.org/CVERecord?id=CVE-2026-45341) -
WriteProperty to Structured View subordinate-list causes NULL pointer dereference
[GHSA-fv2r-c2m2-7qhh](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-fv2r-c2m2-7qhh)
Patched versions: 1.6.0
Pull Request: [#1321](https://github.com/bacnet-stack/bacnet-stack/pull/1321).

[CVE-2026-64675](https://www.cve.org/CVERecord?id=CVE-2026-64675) -
Pre-auth OOB read in xy_color_decode (BACnetXYColor) via WriteGroup/WriteProperty
[GHSA-mmg6-p4pr-cj6h](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-mmg6-p4pr-cj6h).
Patched versions: 1.6.0
Pull Request: [#1386](https://github.com/bacnet-stack/bacnet-stack/pull/1386),
[#1387](https://github.com/bacnet-stack/bacnet-stack/pull/1387).

[CVE-2026-58498](https://www.cve.org/CVERecord?id=CVE-2026-58498) -
Remote global-buffer-overflow in apps/router-ipv6/main.c routed APDU forwarding path
[GHSA-4p4w-m434-jrhj](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-4p4w-m434-jrhj).
Patched versions: 1.6.0
Pull Request: [#1392](https://github.com/bacnet-stack/bacnet-stack/pull/1392).

[CVE-2026-62973](https://www.cve.org/CVERecord?id=CVE-2026-62973) -
Remote Global-Buffer-Overflow Read in readpropm via Malformed ReadPropertyMultiple-ACK
[GHSA-3xxw-jfwm-rq9c](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-3xxw-jfwm-rq9c).
Patched versions: 1.6.0
Pull Request: [#1395](https://github.com/bacnet-stack/bacnet-stack/pull/1395).

[CVE-2026-58265](https://www.cve.org/CVERecord?id=CVE-2026-58265) -
Out-of-bounds read in BACnet/SC proprietary header-option decode (bvlc_sc_decode_proprietary_option)
[GHSA-j66x-fr38-r7m5](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-j66x-fr38-r7m5).
Patched versions: 1.6.0
Pull Request: [#1396](https://github.com/bacnet-stack/bacnet-stack/pull/1396).

[CVE-2026-62975](https://www.cve.org/CVERecord?id=CVE-2026-62975) -
Replacing a RAMFS record with a shorter one via AtomicWriteFile(record-access) can trigger a heap out-of-bounds read
[GHSA-cf8g-hp9m-9fvv](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-cf8g-hp9m-9fvv).
Patched versions: 1.6.0
Pull Request: [#1408](https://github.com/bacnet-stack/bacnet-stack/pull/1408).

[CVE-2026-62976](https://www.cve.org/CVERecord?id=CVE-2026-62976) -
Consecutive AtomicWriteFile(record-access) appends can trigger a heap out-of-bounds read in the RAMFS file backend
[GHSA-32jj-x86x-w98w](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-32jj-x86x-w98w).
Patched versions: 1.6.0
Pull Request: [#1408](https://github.com/bacnet-stack/bacnet-stack/pull/1408).

[CVE-2026-62974](https://www.cve.org/CVERecord?id=CVE-2026-62974) -
Remote unauthenticated DoS in Life_Safety_Zone PROP_ZONE_MEMBERS WriteProperty parsing
[GHSA-2c8x-f46r-8phh](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-2c8x-f46r-8phh).
Patched versions: 1.6.0
Pull Request: [#1410](https://github.com/bacnet-stack/bacnet-stack/pull/1410).

[CVE-2026-62977](https://www.cve.org/CVERecord?id=CVE-2026-62977) -
WriteProperty(File_Size) can bypass read-only protection and expose uninitialized RAMFS tail bytes through AtomicReadFile
[GHSA-mwj7-2v5r-v934](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-mwj7-2v5r-v934).
Patched versions: 1.6.0
Pull Request: [#1411](https://github.com/bacnet-stack/bacnet-stack/pull/1411).

[CVE-2026-62972](https://www.cve.org/CVERecord?id=CVE-2026-62972) -
Out-of-bounds read in lighting_command_decode (full APDU size passed to nested tag decoders)
[GHSA-9hq3-w3pc-8385](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-9hq3-w3pc-8385).
Patched versions: 1.6.0
Pull Request: [#1412](https://github.com/bacnet-stack/bacnet-stack/pull/1412).

[CVE-2026-52789](https://www.cve.org/CVERecord?id=CVE-2026-52789) -
Denial of Service (Infinite Loop) in handler_read_property_multiple via malformed RPM requests.
[GHSA-4rf9-4vgq-5gcw](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-4rf9-4vgq-5gcw).
Patched versions: 1.6.0
Pull Request: [#1374](https://github.com/bacnet-stack/bacnet-stack/pull/1374).

[CVE-2026-52788](https://www.cve.org/CVERecord?id=CVE-2026-52788) -
Buffer overflows in bsc_node_parse_urls() (BACnet/SC Address Resolution ACK URL parser).
[GHSA-rf83-3rr5-v4mj](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-rf83-3rr5-v4mj).
Patched versions: 1.6.0
Pull Request: [#1365](https://github.com/bacnet-stack/bacnet-stack/pull/1365).

[CVE-2026-52786](https://www.cve.org/CVERecord?id=CVE-2026-52786) -
apps/epics: malicious ReadPropertyMultiple-ACK with excessive properties causes global-buffer-overflow in ProcessRPMData().
[GHSA-c4q6-7827-mfg6](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-c4q6-7827-mfg6).
Patched versions: 1.6.0
Pull Request: [#1366](https://github.com/bacnet-stack/bacnet-stack/pull/1366),
[#1409](https://github.com/bacnet-stack/bacnet-stack/pull/1409).

[CVE-2026-52787](https://www.cve.org/CVERecord?id=CVE-2026-52787) -
Global APDU transmit buffer out-of-bounds write in device.address-binding response encoding via address_list_encode()
[GHSA-4fgg-fghm-jm43](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-4fgg-fghm-jm43).
Patched versions: 1.6.0
Pull Request: [#1363](https://github.com/bacnet-stack/bacnet-stack/pull/1363).

[CVE-2026-49990](https://www.cve.org/CVERecord?id=CVE-2026-49990) -
AtomicReadFile/AtomicWriteFile stream fileStartPosition validation flaw causes out-of-bounds read/write in RAM file backends.
[GHSA-8759-hx7g-94qx](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-8759-hx7g-94qx).
Patched versions: 1.6.0
Pull Request: [#1362](https://github.com/bacnet-stack/bacnet-stack/pull/1362).

[CVE-2026-47710](https://www.cve.org/CVERecord?id=CVE-2026-47710) -
Loop reference to long Structured View description causes stack-buffer-overflow.
[GHSA-2xm5-gjpc-9m6q](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-2xm5-gjpc-9m6q).
Patched versions: 1.6.0
Pull Request: [#1355](https://github.com/bacnet-stack/bacnet-stack/pull/1355).

[CVE-2026-47711](https://www.cve.org/CVERecord?id=CVE-2026-47711) -
Stack buffer overflow in Loop internal ReadProperty path via Life_Safety_Zone accepted-modes property.
[GHSA-vrpm-9gm2-x552](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-vrpm-9gm2-x552).
Patched versions: 1.6.0
Pull Request: [#1354](https://github.com/bacnet-stack/bacnet-stack/pull/1354),
[#1355](https://github.com/bacnet-stack/bacnet-stack/pull/1355).

[CVE-2026-47259](https://www.cve.org/CVERecord?id=CVE-2026-47259) -
Stack-based buffer overflow in Notification Class RemoveListElement recipient-list decoding.
[GHSA-9w9m-w7w5-rrv3](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-9w9m-w7w5-rrv3).
Patched versions: 1.6.0
Pull Request: [#1353](https://github.com/bacnet-stack/bacnet-stack/pull/1353).

[CVE-2026-47258](https://www.cve.org/CVERecord?id=CVE-2026-47258) -
Stack-based buffer overflow in Notification Class AddListElement recipient-list decoding.
[GHSA-rjmv-3mcm-r83j](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-rjmv-3mcm-r83j).
Patched versions: 1.6.0
Pull Request: [#1353](https://github.com/bacnet-stack/bacnet-stack/pull/1353).

[CVE-2026-47257](https://www.cve.org/CVERecord?id=CVE-2026-47257) -
ReinitializeDevice ENDRESTORE can delete existing objects on an empty restore file and still return success.
[GHSA-x6pp-3pf3-f87r](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-x6pp-3pf3-f87r).
Patched versions: 1.6.0
Pull Request: [#1352](https://github.com/bacnet-stack/bacnet-stack/pull/1352).

[CVE-2026-49341](https://www.cve.org/CVERecord?id=CVE-2026-49341) -
Uncontrolled recursion in Timer object writeback path leads to remote server stack overflow.
[GHSA-7r8r-2rj2-5wvr](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-7r8r-2rj2-5wvr).
Patched versions: 1.6.0
Pull Request: [#1347](https://github.com/bacnet-stack/bacnet-stack/pull/1347)

[CVE-2026-47217](https://www.cve.org/CVERecord?id=CVE-2026-47217) -
Channel member self-reference causes uncontrolled recursion and stack overflow in default BACnet/IP server
[GHSA-wjw5-q9g6-2764](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-wjw5-q9g6-2764)
Patched versions: 1.6.0
Pull Request: [#1345](https://github.com/bacnet-stack/bacnet-stack/pull/1345).

[CVE-2026-46677](https://www.cve.org/CVERecord?id=CVE-2026-46677) -
Client-Side Out-of-Bounds Read in AtomicReadFile-ACK Record-Access Handling via RecordCount / fileData[] Mismatch
[GHSA-rv5h-cxwq-q3mh](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-rv5h-cxwq-q3mh)
Patched versions: 1.6.0
Pull Request: [#1344](https://github.com/bacnet-stack/bacnet-stack/pull/1344).

[CVE-2026-46676](https://www.cve.org/CVERecord?id=CVE-2026-46676) -
Uninitialized Value Use in AtomicReadFile-ACK Record-Access Encoder Causes Response Corruption and Conditional Information Disclosure
[GHSA-2fwp-32cj-g3x4](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-2fwp-32cj-g3x4)
Patched versions: 1.6.0
Pull Request: [#1344](https://github.com/bacnet-stack/bacnet-stack/pull/1344).

[CVE-2026-46674](https://www.cve.org/CVERecord?id=CVE-2026-46674) -
Out-of-Bounds Read in AtomicWriteFile Record Decoder via Unbounded returnedRecordCount
[GHSA-8384-pwhh-cxjh](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-8384-pwhh-cxjh)
Patched versions: 1.6.0
Pull Request: [#1344](https://github.com/bacnet-stack/bacnet-stack/pull/1344).

[CVE-2026-45265](https://www.cve.org/CVERecord?id=CVE-2026-45265) -
Atomic-Read-File RecordCount Stack-Based Out-of-Bounds Write
[GHSA-v3gx-mwrp-xvh5](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-v3gx-mwrp-xvh5)
Patched versions: 1.6.0
Pull Request: [#1340](https://github.com/bacnet-stack/bacnet-stack/pull/1340).

## Reporting a Vulnerability

Privately discuss, fix, and publish information about security
vulnerabilities in this library using Github Security Advisories:
https://github.com/bacnet-stack/bacnet-stack/security/advisories/new

Alternatively, vulnerabilities can be reported using "issues" at Github.
https://github.com/bacnet-stack/bacnet-stack/issues
