# Security Policy

## Supported Versions

The following versions of the BACnet Stack C library are
currently being supported with security updates.

| Version | Supported          |
| ------- | ------------------ |
| 1.4.x   | :white_check_mark: |
| 1.3.x   | :x:                |
| 1.2.x   | :x:                |
| 1.1.x   | :x:                |
| 1.0.x   | :x:                |
| 0.9.x   | :x:                |
| 0.8.x   | :x:                |
| 0.7.x   | :x:                |
| < 0.6.x | :x:                |


## Coordinated Vulnerability Disclosure

From time to time a vulnerability is disclosed to [CVE](https://www.cve.org/)
and a record is created to identify, define, and catalog publicly disclosed
cybersecurity vulnerabilities.

Here are the known CVE records for version 1.4.x:

[CVE-2026-52790](https://www.cve.org/CVERecord?id=CVE-2026-CVE-2026-52790) -
bacnet_device.c stack-use-after-return in writable Device string properties
[GHSA-jr7p-rm2x-739x](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-jr7p-rm2x-739x).
Patched versions: 1.4.5

[CVE-2026-52789](https://www.cve.org/CVERecord?id=CVE-2026-52789) -
Denial of Service (Infinite Loop) in handler_read_property_multiple via malformed RPM requests.
[GHSA-4rf9-4vgq-5gcw](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-4rf9-4vgq-5gcw).
Patched versions: 1.4.5

[CVE-2026-52788](https://www.cve.org/CVERecord?id=CVE-2026-52788) -
Buffer overflows in bsc_node_parse_urls() (BACnet/SC Address Resolution ACK URL parser).
[GHSA-rf83-3rr5-v4mj](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-rf83-3rr5-v4mj).
Patched versions: 1.4.5

[CVE-2026-52786](https://www.cve.org/CVERecord?id=CVE-2026-52786) -
apps/epics: malicious ReadPropertyMultiple-ACK with excessive properties causes global-buffer-overflow in ProcessRPMData().
[GHSA-c4q6-7827-mfg6](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-c4q6-7827-mfg6).
Patched versions: 1.4.5

[CVE-2026-52787](https://www.cve.org/CVERecord?id=CVE-2026-52787) -
Global APDU transmit buffer out-of-bounds write in device.address-binding response encoding via address_list_encode()
[GHSA-4fgg-fghm-jm43](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-4fgg-fghm-jm43).
Patched version: 1.4.5

AtomicReadFile/AtomicWriteFile stream fileStartPosition validation flaw causes out-of-bounds read/write in RAM file backends
[GHSA-8759-hx7g-94qx](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-8759-hx7g-94qx)
Patched version: 1.4.5

[CVE-2026-47710](https://www.cve.org/CVERecord?id=CVE-2026-47710) -
Loop reference to long Structured View description causes stack-buffer-overflow
[GHSA-2xm5-gjpc-9m6q](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-2xm5-gjpc-9m6q)

[CVE-2026-47711](https://www.cve.org/CVERecord?id=CVE-2026-47711) -
Stack buffer overflow in Loop internal ReadProperty path via Life_Safety_Zone accepted-modes property
[GHSA-vrpm-9gm2-x552](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-vrpm-9gm2-x552)

[CVE-2026-47259](https://www.cve.org/CVERecord?id=CVE-2026-47259) -
Stack-based buffer overflow in Notification Class RemoveListElement recipient-list decoding
[GHSA-9w9m-w7w5-rrv3](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-9w9m-w7w5-rrv3)

[CVE-2026-47258](https://www.cve.org/CVERecord?id=CVE-2026-47258) -
Stack-based buffer overflow in Notification Class AddListElement recipient-list decoding
[GHSA-rjmv-3mcm-r83j](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-rjmv-3mcm-r83j)

Uncontrolled recursion in Timer object writeback path leads to remote server stack overflow
[GHSA-7r8r-2rj2-5wvr](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-7r8r-2rj2-5wvr)

[CVE-2026-47217](https://www.cve.org/CVERecord?id=CVE-2026-47217) -
Channel member self-reference causes uncontrolled recursion and stack overflow in default BACnet/IP server
[GHSA-wjw5-q9g6-2764](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-wjw5-q9g6-2764)

[CVE-2026-46677](https://www.cve.org/CVERecord?id=CVE-2026-46677) -
Client-Side Out-of-Bounds Read in AtomicReadFile-ACK Record-Access Handling via RecordCount / fileData[] Mismatch
[GHSA-rv5h-cxwq-q3mh](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-rv5h-cxwq-q3mh)

[CVE-2026-46676](https://www.cve.org/CVERecord?id=CVE-2026-46676) -
Uninitialized Value Use in AtomicReadFile-ACK Record-Access Encoder Causes Response Corruption and Conditional Information Disclosure
[GHSA-2fwp-32cj-g3x4](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-2fwp-32cj-g3x4)

[CVE-2026-46674](https://www.cve.org/CVERecord?id=CVE-2026-46674) -
Out-of-Bounds Read in AtomicWriteFile Record Decoder via Unbounded returnedRecordCount
[GHSA-8384-pwhh-cxjh](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-8384-pwhh-cxjh)

[CVE-2026-45341](https://www.cve.org/CVERecord?id=CVE-2026-45341) -
WriteProperty to Structured View subordinate-list causes NULL pointer dereference
[GHSA-fv2r-c2m2-7qhh](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-fv2r-c2m2-7qhh)

[CVE-2026-45265](https://www.cve.org/CVERecord?id=CVE-2026-45265) -
Atomic-Read-File RecordCount Stack-Based Out-of-Bounds Write
[GHSA-v3gx-mwrp-xvh5](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-v3gx-mwrp-xvh5)

[CVE-2026-26264](https://www.cve.org/CVERecord?id=CVE-2026-26264) -
WriteProperty decoding length underflow leads to OOB read and crash
[GHSA-phjh-v45p-gmjj](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-phjh-v45p-gmjj)

[CVE-2026-21870](https://www.cve.org/CVERecord?id=CVE-2026-21870) -
Off-by-one Stack-based Buffer Overflow in tokenizer_string
[GHSA-pc83-wp6w-93mx](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-pc83-wp6w-93mx)

## Reporting a Vulnerability

Privately discuss, fix, and publish information about security
vulnerabilities in this library using Github Security Advisories:
https://github.com/bacnet-stack/bacnet-stack/security/advisories/new

Alternatively, vulnerabilities can be reported using "issues" at Github.
https://github.com/bacnet-stack/bacnet-stack/issues
