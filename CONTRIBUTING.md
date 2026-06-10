# Contributing to BACnet Stack

Thank you for your interest in contributing to the BACnet stack!
Please review the following guidelines for contributing new code
or documentation to the project.

## GitLab Workflow and Versioning

The CMakeLists.txt, CHANGELOG.md, and version.h contain the version number.

This project aspires to adhere to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

We utilize a GitLab-style workflow for managing changes and releases.
When developing features or preparing release candidates, please reflect
the release candidate stages using `-rc1`, `-rc2`, etc., in the
`src/bacnet/version.h` file.

Specifically, update the `BACNET_VERSION_TEXT` string in the version.h file
to include the `-rc` suffix during the release candidate phase. For example:
```c
#define BACNET_VERSION_TEXT "1.6.0-rc1"
```
Once the release is confirmed and finalized, the suffix should be removed for
the official release version and a new branch created to track any bug fixes
for that release.

## Pre-commit Hooks

This project uses `pre-commit` to ensure code formatting and quality standards.
Developers are expected to install and use it locally so that checks and
formatting happen automatically before every commit.

Setting up a Python virtual environment and configuring pre-commit involves
four primary steps: environment creation, tool installation, configuration,
and Git hook activation. For example:
```bash
python -m venv .venv
source .venv/bin/activate
pip install pre-commit
pre-commit install
pre-commit --version
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
`bacnet_` as a prefix for core BACnet modules, followed by the module name,
and using a verb suffix to describe the function's action.
From left to right the name would read general to specific.
For example:
`bacnet_address_copy()` - copy an address
`bacnet_address_from_ascii()` - create an address from an ASCII string
`bacnet_address_to_ascii()` - create an ASCII string from an address

Although there is precedent for names prefixed with the BACnet defined
name, such as `whois_request_encode`, using `bacnet_` prefix integrates
easier into larger project namespaces.

Functions in the `basic/object` and `basic/service` tree are typcially
named in mixed case Snake_Case to indicate that they are example user
functions.

BACnet ASN.1 types are typically defined in the standard using
CamelCase and are likewise defined in the code enums and structs
using the same name as used in the standard, albeit in Snake_Case.
Most (all?) of the BACnet complex data types are typedef'd using
uppercase names in SNAKE_CASE. For example:
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
mirror the naming convention from the BACnet standard. Enumerations always
define and include the integer value as expressed in the standard.
The typedef name is written in in ALL uppercase. The suffix `_MAX` is
appended to indicate the maximum value which is used for array sizing,
loop bounds, or range validation. For example:
```c
typedef enum BACnetSuccessFilter {
    BACNET_SUCCESS_FILTER_ALL = 0,
    BACNET_SUCCESS_FILTER_SUCCESS_ONLY = 1,
    BACNET_SUCCESS_FILTER_FAILURES_ONLY = 2,
    BACNET_SUCCESS_FILTER_MAX = 3
} BACNET_SUCCESS_FILTER;
```

Alternate suffix for enumerations that have vendor defined ranges include
`_RESERVED_MIN`, `_RESERVED_MAX`, `_PROPRIETARY_MIN`, and `_PROPRIETARY_MAX`.
For example:
```c
typedef enum BACnetAuthenticationDisableReason {
    AUTHENTICATION_NONE = 0,
    AUTHENTICATION_DISABLED = 1,
    AUTHENTICATION_DISABLED_LOST = 2,
    AUTHENTICATION_DISABLED_STOLEN = 3,
    AUTHENTICATION_DISABLED_DAMAGED = 4,
    AUTHENTICATION_DISABLED_DESTROYED = 5,
    AUTHENTICATION_DISABLED_MAX = 6,
    AUTHENTICATION_DISABLED_RESERVED_MIN = 6,
    AUTHENTICATION_DISABLED_RESERVED_MAX = 63,
    /* Enumerated values 0-63 are reserved for definition by ASHRAE.
       Enumerated values 64-65535 may be used by others subject to
       the procedures and constraints described in Clause 23. */
    AUTHENTICATION_DISABLED_PROPRIETARY_MIN = 64,
    AUTHENTICATION_DISABLED_PROPRIETARY_MAX = 65535
} BACNET_AUTHENTICATION_DISABLE_REASON;
```

Most BACnet enumerations have a text equivalent in bactext.c module
that is used for EPICS and logging. For example:
```c
static INDTEXT_DATA bactext_audit_level_names[] = {
    /* BACnetAuditLevel enumerations */
    { AUDIT_LEVEL_NONE, "none" },
    { AUDIT_LEVEL_AUDIT_ALL, "audit-all" },
    { AUDIT_LEVEL_AUDIT_CONFIG, "audit-config" },
    { AUDIT_LEVEL_DEFAULT, "default" },
    { 0, NULL }
};
```
