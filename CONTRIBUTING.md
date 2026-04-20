# Contributing to BACnet Stack

Thank you for your interest in contributing to the BACnet stack!
Please review the following guidelines for contributing new code
or documentation to the project.

## GitLab Workflow and Versioning

We utilize a GitLab-style workflow for managing changes and releases.
When developing features or preparing release candidates, please reflect
the release candidate stages using `-rc1`, `-rc2`, etc., in the
`src/bacnet/version.h` file.

Specifically, update the `BACNET_VERSION_TEXT` string to include the `-rc`
suffix during the release candidate phase. For example:
```c
#define BACNET_VERSION_TEXT "1.6.0-rc1"
```
Once the release is confirmed and finalized, the suffix should be removed for
the official release version and a new branch created to track any bug fixes
for that release.

## Pre-commit Hooks

This project uses `pre-commit` to ensure code formatting and quality standards.
You are expected to install and use it locally so that checks and formatting
happen automatically before every commit.

To install:
```bash
pip install pre-commit
pre-commit install
```

This ensures that your code complies with our coding conventions and saves time
during code review.

## CI Expectations (.github/workflows)

All code contributions are subjected to automated CI testing via GitHub Actions
defined in `.github/workflows/`.
The workflows include:
- Build verification across OS platforms (Linux, macOS, Windows, Raspberry Pi)
- Build variations across embedded microcontrollers (ESP32, STM32/AT91, xmega/AVR, etc.)
- Build for C standards: gnu89 (C89+extensions), gnu99, gnu11, gnu17
- Lint validation using clang-build, cppcheck, and flawfinder
- pre-commit using clang-format and other tools to enforce code style
- CodeQL static analysis

Contributions that break the build or introduce new warnings will not be
merged.

## Naming Conventions

New files should be named in all lowercase snake_case (write_group.c),
although there is precedent for no underscores (whois.c, iam.c) or BACnet
standard name acronyms (rpm.c, npdu.c, apdu.c).

New functions should be named in snake_case_with_underscores using
bacnet_ as a prefix for core BACnet modules, followed by the module name,
and using a verb suffix to describe the function's action.
From left to right the name would read general to specific.
For example:
`bacnet_address_copy()` - copy an address
`bacnet_address_from_ascii()` - create an address from an ASCII string
`bacnet_address_to_ascii()` - create an ASCII string from an address

BACnet ASN.1 types are typically defined in the standard using
CamelCase and are likewise defined in the code enums and structs
using the same name as used in the standard, albeit in Snake_Case.
Most (all?) of the BACnet complex data types are typedef'd using
uppercase names in snake_case. For example:
```c
/* BACnetDate */
typedef struct BACnet_Date {
    uint16_t year; /* AD */
    uint8_t month; /* 1=Jan */
    uint8_t day; /* 1..31 */
    uint8_t wday; /* 1=Monday-7=Sunday */
} BACNET_DATE;
```

All BACnet enumerations from the ASN.1 standard are defined in bacenum.h and
mirror the naming convention as the BACnet standard, but in uppercase.
`_MAX` is added to the end of each enumeration to indicate the maximum value
which is used for array sizing, loop bounds, or range validation.
Enumerations always define the value as expressed in the standard.
Most BACnet enumerations have a text equivalent in bactext.c module
that is used for EPICS and logging.
```c
typedef enum BACnetSuccessFilter {
    BACNET_SUCCESS_FILTER_ALL = 0,
    BACNET_SUCCESS_FILTER_SUCCESS_ONLY = 1,
    BACNET_SUCCESS_FILTER_FAILURES_ONLY = 2,
    BACNET_SUCCESS_FILTER_MAX = 3
} BACNET_SUCCESS_FILTER;
```
