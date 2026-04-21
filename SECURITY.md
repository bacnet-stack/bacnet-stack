# Security Policy

## Supported Versions

The following versions of the BACnet Stack C library are
currently being supported with security updates.

| Version | Supported          |
| ------- | ------------------ |
| 1.5.x   | :white_check_mark: |
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

Vulnerabilites are disclosed to [CVE](https://www.cve.org/)
or [GHSA](https://github.com/bacnet-stack/bacnet-stack/security/advisories?state=published)
and a record is created to identify, define, and catalog publicly disclosed
cybersecurity vulnerabilities. Here are the published vulnerability records:

[CVE-2026-26264](https://www.cve.org/CVERecord?id=CVE-2026-26264) -
Undefined-behavior signed left shift in `decode_signed32()`
[GHSA-326g-j95f-gmxv](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-326g-j95f-gmxv)

Out-of-Bounds Read in ReadPropertyMultiple Property Decoder via Deprecated Tag Parser
[GHSA-5w2v-mwqj-pr2c](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-5w2v-mwqj-pr2c)

Off-by-One Out-of-Bounds Read in ReadPropertyMultiple Object ID Decoder
[GHSA-7545-3fpx-4xw3](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-7545-3fpx-4xw3)

[CVE-2026-41475](https://www.cve.org/CVERecord?id=CVE-2026-41475) -
Out-of-Bounds Read in WritePropertyMultiple Decoder via Deprecated Tag Parser
[GHSA-cvv4-v3g6-4jmv](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-cvv4-v3g6-4jmv)

[CVE-2026-26264](https://www.cve.org/CVERecord?id=CVE-2026-26264) -
WriteProperty decoding length underflow leads to OOB read and crash
[GHSA-phjh-v45p-gmjj](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-phjh-v45p-gmjj)

[CVE-2026-21870](https://www.cve.org/CVERecord?id=CVE-2026-21870) -
Off-by-one Stack-based Buffer Overflow in tokenizer_string
[GHSA-pc83-wp6w-93mx](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-pc83-wp6w-93mx)

[CVE-2026-21878](https://www.cve.org/CVERecord?id=CVE-2026-21878) -
Improper Limitation of a Pathname to a Restricted Directory
[GHSA-p8rx-c26w-545j](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-p8rx-c26w-545j)

[CVE-2025-66624](https://www.cve.org/CVERecord?id=CVE-2025-66624) -
BACnet-stack MS/TP reply matcher OOB read
[GHSA-8wgw-5h6x-qgqg](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-8wgw-5h6x-qgqg)

[CVE-2023-38341](https://www.cve.org/CVERecord?id=CVE-2023-38341) -
Multiple out-of-bounds accesses in bacerror code paths
[#81](https://sourceforge.net/p/bacnet/bugs/81/)

[CVE-2023-38340](https://www.cve.org/CVERecord?id=CVE-2023-38340) -
Out of bounds accesses in bacnet_npdu_decode
[#80](https://sourceforge.net/p/bacnet/bugs/80/)

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
