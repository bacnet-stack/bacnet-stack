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
cybersecurity vulnerabilities. Here are the published vulnerability records
for v1.5.x:

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

[CVE-2026-45265](https://www.cve.org/CVERecord?id=CVE-2026-45265) -
Atomic-Read-File RecordCount Stack-Based Out-of-Bounds Write
[GHSA-v3gx-mwrp-xvh5](https://github.com/bacnet-stack/bacnet-stack/security/advisories/GHSA-v3gx-mwrp-xvh5)

## Reporting a Vulnerability

Privately discuss, fix, and publish information about security
vulnerabilities in this library using Github Security Advisories:
https://github.com/bacnet-stack/bacnet-stack/security/advisories/new

Alternatively, vulnerabilities can be reported using "issues" at Github.
https://github.com/bacnet-stack/bacnet-stack/issues
