# Security Policy

## Supported Versions

The following versions of the BACnet Stack C library are
currently being supported with security updates in this branch:

| Version | Supported          |
| ------- | ------------------ |
| 1.5.x   | :white_check_mark: |
| 1.4.x   | :x:                |
| 1.3.x   | :x:                |
| 1.2.x   | :x:                |
| 1.1.x   | :x:                |
| 1.0.x   | :x:                |
| 0.9.x   | :x:                |
| 0.8.x   | :x:                |
| 0.7.x   | :x:                |
| < 0.6.x | :x:                |

## Coordinated Vulnerability Disclosure

Vulnerabilites are disclosed to [CVE](https://www.cve.org/)
or [GHSA](https://github.com/bacnet-stack/bacnet-stack/security/advisories?state=published)
and a record is created to identify, define, and catalog publicly disclosed
cybersecurity vulnerabilities.

[CVE-2026-52790](https://www.cve.org/CVERecord?id=CVE-2026-52790) -
bacnet_device.c stack-use-after-return in writable Device string properties
[GHSA-jr7p-rm2x-739x](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-jr7p-rm2x-739x).
Patched versions: 1.5.1
Pull Request: [#1375](https://github.com/bacnet-stack/bacnet-stack/pull/1375).

[CVE-2026-45341](https://www.cve.org/CVERecord?id=CVE-2026-45341) -
WriteProperty to Structured View subordinate-list causes NULL pointer dereference
[GHSA-fv2r-c2m2-7qhh](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-fv2r-c2m2-7qhh)
Patched versions: 1.5.1
Pull Request: [#1321](https://github.com/bacnet-stack/bacnet-stack/pull/1321).

Pre-auth OOB read in xy_color_decode (BACnetXYColor) via WriteGroup/WriteProperty
[GHSA-mmg6-p4pr-cj6h](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-mmg6-p4pr-cj6h).
Patched versions: 1.5.1
Pull Request: [#1386](https://github.com/bacnet-stack/bacnet-stack/pull/1386),
[#1387](https://github.com/bacnet-stack/bacnet-stack/pull/1387).

Remote global-buffer-overflow in apps/router-ipv6/main.c routed APDU forwarding path
[GHSA-4p4w-m434-jrhj](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-4p4w-m434-jrhj).
Patched versions: 1.5.1
Pull Request: [#1392](https://github.com/bacnet-stack/bacnet-stack/pull/1392).

Remote Global-Buffer-Overflow Read in readpropm via Malformed ReadPropertyMultiple-ACK
[GHSA-3xxw-jfwm-rq9c](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-3xxw-jfwm-rq9c).
Patched versions: 1.5.1
Pull Request: [#1395](https://github.com/bacnet-stack/bacnet-stack/pull/1395).

Replacing a RAMFS record with a shorter one via AtomicWriteFile(record-access) can trigger a heap out-of-bounds read
[GHSA-cf8g-hp9m-9fvv](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-cf8g-hp9m-9fvv).
Patched versions: 1.5.1
Pull Request: [#1408](https://github.com/bacnet-stack/bacnet-stack/pull/1408).

Consecutive AtomicWriteFile(record-access) appends can trigger a heap out-of-bounds read in the RAMFS file backend
[GHSA-32jj-x86x-w98w](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-32jj-x86x-w98w).
Patched versions: 1.5.1
Pull Request: [#1408](https://github.com/bacnet-stack/bacnet-stack/pull/1408).

[CVE-2026-52789](https://www.cve.org/CVERecord?id=CVE-2026-52789) -
Denial of Service (Infinite Loop) in handler_read_property_multiple via malformed RPM requests.
[GHSA-4rf9-4vgq-5gcw](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-4rf9-4vgq-5gcw).
Patched versions: 1.5.1
Pull Request: [#1374](https://github.com/bacnet-stack/bacnet-stack/pull/1374).

[CVE-2026-52788](https://www.cve.org/CVERecord?id=CVE-2026-52788) -
Buffer overflows in bsc_node_parse_urls() (BACnet/SC Address Resolution ACK URL parser).
[GHSA-rf83-3rr5-v4mj](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-rf83-3rr5-v4mj).
Patched versions: 1.5.1
Pull Request: [#1365](https://github.com/bacnet-stack/bacnet-stack/pull/1365).

[CVE-2026-52786](https://www.cve.org/CVERecord?id=CVE-2026-52786) -
apps/epics: malicious ReadPropertyMultiple-ACK with excessive properties causes global-buffer-overflow in ProcessRPMData().
[GHSA-c4q6-7827-mfg6](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-c4q6-7827-mfg6).
Patched versions: 1.5.1
Pull Request: [#1366](https://github.com/bacnet-stack/bacnet-stack/pull/1366),
[#1409](https://github.com/bacnet-stack/bacnet-stack/pull/1409).

[CVE-2026-52787](https://www.cve.org/CVERecord?id=CVE-2026-52787) -
Global APDU transmit buffer out-of-bounds write in device.address-binding response encoding via address_list_encode()
[GHSA-4fgg-fghm-jm43](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-4fgg-fghm-jm43).
Patched versions: 1.5.1
Pull Request: [#1363](https://github.com/bacnet-stack/bacnet-stack/pull/1363).

[CVE-2026-49990](https://www.cve.org/CVERecord?id=CVE-2026-49990) -
AtomicReadFile/AtomicWriteFile stream fileStartPosition validation flaw causes out-of-bounds read/write in RAM file backends.
[GHSA-8759-hx7g-94qx](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-8759-hx7g-94qx).
Patched versions: 1.5.1
Pull Request: [#1362](https://github.com/bacnet-stack/bacnet-stack/pull/1362).

[CVE-2026-47710](https://www.cve.org/CVERecord?id=CVE-2026-47710) -
Loop reference to long Structured View description causes stack-buffer-overflow.
[GHSA-2xm5-gjpc-9m6q](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-2xm5-gjpc-9m6q).
Patched versions: 1.5.1
Pull Request: [#1355](https://github.com/bacnet-stack/bacnet-stack/pull/1355).

[CVE-2026-47711](https://www.cve.org/CVERecord?id=CVE-2026-47711) -
Stack buffer overflow in Loop internal ReadProperty path via Life_Safety_Zone accepted-modes property.
[GHSA-vrpm-9gm2-x552](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-vrpm-9gm2-x552).
Patched versions: 1.5.1
Pull Request: [#1354](https://github.com/bacnet-stack/bacnet-stack/pull/1354),
[#1355](https://github.com/bacnet-stack/bacnet-stack/pull/1355).

[CVE-2026-47259](https://www.cve.org/CVERecord?id=CVE-2026-47259) -
Stack-based buffer overflow in Notification Class RemoveListElement recipient-list decoding.
[GHSA-9w9m-w7w5-rrv3](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-9w9m-w7w5-rrv3).
Patched versions: 1.5.1
Pull Request: [#1353](https://github.com/bacnet-stack/bacnet-stack/pull/1353).

[CVE-2026-47258](https://www.cve.org/CVERecord?id=CVE-2026-47258) -
Stack-based buffer overflow in Notification Class AddListElement recipient-list decoding.
[GHSA-rjmv-3mcm-r83j](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-rjmv-3mcm-r83j).
Patched versions: 1.5.1
Pull Request: [#1353](https://github.com/bacnet-stack/bacnet-stack/pull/1353).

[CVE-2026-47257](https://www.cve.org/CVERecord?id=CVE-2026-47257) -
ReinitializeDevice ENDRESTORE can delete existing objects on an empty restore file and still return success.
[GHSA-x6pp-3pf3-f87r](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-x6pp-3pf3-f87r).
Patched versions: 1.5.1
Pull Request: [#1352](https://github.com/bacnet-stack/bacnet-stack/pull/1352).

[CVE-2026-49341](https://www.cve.org/CVERecord?id=CVE-2026-49341) -
Uncontrolled recursion in Timer object writeback path leads to remote server stack overflow.
[GHSA-7r8r-2rj2-5wvr](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-7r8r-2rj2-5wvr).
Patched versions: 1.5.1
Pull Request: [#1347](https://github.com/bacnet-stack/bacnet-stack/pull/1347)

[CVE-2026-47217](https://www.cve.org/CVERecord?id=CVE-2026-47217) -
Channel member self-reference causes uncontrolled recursion and stack overflow in default BACnet/IP server
[GHSA-wjw5-q9g6-2764](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-wjw5-q9g6-2764)
Patched versions: 1.5.1
Pull Request: [#1345](https://github.com/bacnet-stack/bacnet-stack/pull/1345).

[CVE-2026-46677](https://www.cve.org/CVERecord?id=CVE-2026-46677) -
Client-Side Out-of-Bounds Read in AtomicReadFile-ACK Record-Access Handling via RecordCount / fileData[] Mismatch
[GHSA-rv5h-cxwq-q3mh](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-rv5h-cxwq-q3mh)
Patched versions: 1.5.1
Pull Request: [#1344](https://github.com/bacnet-stack/bacnet-stack/pull/1344).

[CVE-2026-46676](https://www.cve.org/CVERecord?id=CVE-2026-46676) -
Uninitialized Value Use in AtomicReadFile-ACK Record-Access Encoder Causes Response Corruption and Conditional Information Disclosure
[GHSA-2fwp-32cj-g3x4](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-2fwp-32cj-g3x4)
Patched versions: 1.5.1
Pull Request: [#1344](https://github.com/bacnet-stack/bacnet-stack/pull/1344).

[CVE-2026-46674](https://www.cve.org/CVERecord?id=CVE-2026-46674) -
Out-of-Bounds Read in AtomicWriteFile Record Decoder via Unbounded returnedRecordCount
[GHSA-8384-pwhh-cxjh](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-8384-pwhh-cxjh)
Patched versions: 1.5.1
Pull Request: [#1344](https://github.com/bacnet-stack/bacnet-stack/pull/1344).

[CVE-2026-45265](https://www.cve.org/CVERecord?id=CVE-2026-45265) -
Atomic-Read-File RecordCount Stack-Based Out-of-Bounds Write
[GHSA-v3gx-mwrp-xvh5](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-v3gx-mwrp-xvh5)
Patched versions: 1.5.1
Pull Request: [#1340](https://github.com/bacnet-stack/bacnet-stack/pull/1340).

[CVE-2026-40279](https://www.cve.org/CVERecord?id=CVE-2026-40279) -
Undefined-behavior signed left shift in `decode_signed32()`
[GHSA-326g-j95f-gmxv](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-326g-j95f-gmxv)
Patched versions: [1.5.0](https://github.com/bacnet-stack/bacnet-stack/tree/bacnet-stack-1.5.0)
Pull Request: [#1300](https://github.com/bacnet-stack/bacnet-stack/pull/1300)

[CVE-2026-41503](https://www.cve.org/CVERecord?id=CVE-2026-41503) -
Out-of-Bounds Read in ReadPropertyMultiple Property Decoder via Deprecated Tag Parser
[GHSA-5w2v-mwqj-pr2c](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-5w2v-mwqj-pr2c)
Patched versions: 1.5.0
Pull Request: [#1244](https://github.com/bacnet-stack/bacnet-stack/pull/1244)

[CVE-2026-41502](https://www.cve.org/CVERecord?id=CVE-2026-41502) -
Off-by-One Out-of-Bounds Read in ReadPropertyMultiple Object ID Decoder
[GHSA-7545-3fpx-4xw3](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-7545-3fpx-4xw3)
Patched versions: 1.5.0
Pull Request: [#1244](https://github.com/bacnet-stack/bacnet-stack/pull/1244)

[CVE-2026-41475](https://www.cve.org/CVERecord?id=CVE-2026-41475) -
Out-of-Bounds Read in WritePropertyMultiple Decoder via Deprecated Tag Parser
[GHSA-cvv4-v3g6-4jmv](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-cvv4-v3g6-4jmv)
Patched versions: 1.5.0
Pull Request: [#1244](https://github.com/bacnet-stack/bacnet-stack/pull/1244)

[CVE-2026-26264](https://www.cve.org/CVERecord?id=CVE-2026-26264) -
WriteProperty decoding length underflow leads to OOB read and crash
[GHSA-phjh-v45p-gmjj](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-phjh-v45p-gmjj)
Patched versions: 1.5.0
Pull Request: [#1231](https://github.com/bacnet-stack/bacnet-stack/pull/1231)

[CVE-2026-21870](https://www.cve.org/CVERecord?id=CVE-2026-21870) -
Off-by-one Stack-based Buffer Overflow in tokenizer_string
[GHSA-pc83-wp6w-93mx](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-pc83-wp6w-93mx)
Patched versions: 1.5.0
Pull Request: [#1196](https://github.com/bacnet-stack/bacnet-stack/pull/1196)

[CVE-2026-21878](https://www.cve.org/CVERecord?id=CVE-2026-21878) -
Improper Limitation of a Pathname to a Restricted Directory
[GHSA-p8rx-c26w-545j](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-p8rx-c26w-545j)
Patched versions: 1.5.0
Pull Request: [#1197](https://github.com/bacnet-stack/bacnet-stack/pull/1197)

[CVE-2025-66624](https://www.cve.org/CVERecord?id=CVE-2025-66624) -
BACnet-stack MS/TP reply matcher OOB read
[GHSA-8wgw-5h6x-qgqg](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-8wgw-5h6x-qgqg)
Patched versions: 1.5.0
Pull Request: [#1178](https://github.com/bacnet-stack/bacnet-stack/pull/1178)

## Reporting a Vulnerability

Privately discuss, fix, and publish information about security
vulnerabilities in this library using Github Security Advisories:
https://github.com/bacnet-stack/bacnet-stack/security/advisories/new

Alternatively, vulnerabilities can be reported using "issues" at Github.
https://github.com/bacnet-stack/bacnet-stack/issues
