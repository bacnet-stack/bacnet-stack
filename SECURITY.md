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

Here are the known CVE records:

[CVE-2026-47711](https://www.cve.org/CVERecord?id=CVE-2026-47711) -
Stack buffer overflow in Loop internal ReadProperty path via Life_Safety_Zone accepted-modes property
[GHSA-vrpm-9gm2-x552](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-vrpm-9gm2-x552)

Stack-based buffer overflow in Notification Class RemoveListElement recipient-list decoding
[GHSA-9w9m-w7w5-rrv3](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-9w9m-w7w5-rrv3)

Stack-based buffer overflow in Notification Class AddListElement recipient-list decoding
[GHSA-rjmv-3mcm-r83j](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-rjmv-3mcm-r83j)

Uncontrolled recursion in Timer object writeback path leads to remote server stack overflow
[GHSA-7r8r-2rj2-5wvr](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-7r8r-2rj2-5wvr)

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

[CVE-2023-38341](https://www.cve.org/CVERecord?id=CVE-2023-38341) -
Multiple out-of-bounds accesses in bacerror code paths
[#81](https://sourceforge.net/p/bacnet/bugs/81/)

[CVE-2023-38340](https://www.cve.org/CVERecord?id=CVE-2023-38340) -
Out of bounds accesses in bacnet_npdu_decode
[#80](https://sourceforge.net/auth/?return_to=/p/bacnet/bugs/80/)

[CVE-2023-38339](https://www.cve.org/CVERecord?id=CVE-2023-38339) -
Out of bounds jump in h_apdu.c:apdu_handler
[#79](https://sourceforge.net/p/bacnet/bugs/79/)

[CVE-2019-12480](https://www.cve.org/CVERecord?id=CVE-2019-12480) -
Invalid read in bacserv when decoding alarm tags
[#62](https://sourceforge.net/p/bacnet/bugs/62/)

[CVE-2018-10238](https://www.cve.org/CVERecord?id=CVE-2018-10238) -
Segmentation fault leading to denial of service
[#61](https://sourceforge.net/p/bacnet/bugs/61/)

## Reporting a Vulnerability

Privately discuss, fix, and publish information about security
vulnerabilities in this library using Github Security Advisories:
https://github.com/bacnet-stack/bacnet-stack/security/advisories/new

Alternatively, vulnerabilities can be reported using "issues" at Github.
https://github.com/bacnet-stack/bacnet-stack/issues
