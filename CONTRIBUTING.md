# Contributing to BACnet Stack

Thank you for your interest in contributing to the BACnet stack!
Please review the following guidelines for contributing new code
or documentation to the project.

## Maintenance Branch Workflow and Versioning

This is the `bacnet-stack-1.4` maintenance branch. It tracks bug fixes and
security fixes for the 1.4.x release series. New features, API changes, and
refactoring belong on the `master` branch and are not accepted here.

The CMakeLists.txt, CHANGELOG.md, and version.h contain the version number.

This project aspires to adhere to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
Only the patch (maintenance) number changes on this branch. Changes must remain
backward compatible with the 1.4.x API and ABI.

Fixes should normally be made on `master` first and then backported to this
branch (for example, by cherry-picking), so that the fix is not lost in later
releases. Pull requests for this branch should target `bacnet-stack-1.4`.
A fix that only applies to this branch should explain why it is not needed on
`master`.

Patch release candidates are reflected using `-rc1`, `-rc2`, etc., in the
`src/bacnet/version.h` file. Specifically, the `BACNET_VERSION_TEXT` string
in version.h includes the `-rc` suffix during the release candidate phase.
For example:
```c
#define BACNET_VERSION_TEXT "1.4.7-rc1"
```
Once the release is confirmed and finalized, the suffix is removed for the
official release version and the release is tagged on this branch.

### CHANGELOG Updates

The CHANGELOG.md is typically updated by the maintainers outside of bugfix
pull requests. Keeping CHANGELOG.md changes out of those pull requests avoids
merge conflicts and makes it easier to backport fixes from `master`.
Contributors do not need to edit CHANGELOG.md in their pull requests.

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
during code review. Keep changes consistent with `.clang-format`.

## CI Expectations (.github/workflows)

All code contributions are subjected to automated CI testing via GitHub Actions
defined in `.github/workflows/`.
The workflows include:
- Build verification across OS platforms (Linux, macOS, Windows, Raspberry Pi)
- Build variations across embedded microcontrollers (ESP32, STM32/AT91, xmega/AVR, etc.)
- Build for C standards: gnu89 (C89+extensions), gnu99, gnu11, gnu17
- Lint validation using scan-build, cppcheck, and flawfinder
- pre-commit using clang-format and other tools to enforce code style
- CodeQL static analysis

Contributions that break the build or introduce new warnings will not be
merged. Existing unit tests must continue to pass. Add tests under `test/` for
new functionality when practical. Follow the existing test layout and register
new test directories with CMake. See the [unit testing README](test/README.md)
for test conventions, framework details, and instructions for running tests.

## Local Build Guidance

The `apps/Makefile` manages the shared library and the applications built under
`apps/`. The Makefile in the repository root provides most, but not all, build
targets and configures the project include paths for those targets. Use the
appropriate root-level target when one exists, such as `make mstp` for MS/TP
applications including `router-mstp`; otherwise use the corresponding target
from `apps/Makefile`. Invoking an application Makefile directly from a deeper
subdirectory is not self-contained and can fail before compilation with a
missing header such as `bacnet/bacdef.h`.

## Code and Platform Guidelines

Keep code compatible with the supported C standards listed above; do not
introduce language features that break older supported standards unless
required. Declare local variables at the start of a function or block, before
executable statements, to follow the project's C89 style. Initialize variables
meaningful default is available, and ensure every variable is assigned before
it is read; do not rely on implicit initialization. Use explicit parentheses
around each independent comparison in compound `if` conditions, even when
operator precedence makes them optional. Prefer `if`/`else` blocks over ternary
operators when they make the control flow clearer and easier to read. Preserve
existing SPDX license identifiers in source and header files. Header files
under `src/bacnet/` typically use `MIT`; most C source files use
`GPL-2.0-or-later WITH GCC-exception-2.0`, while some use MIT or Apache-2.0.
Consult maintainers before changing a file's license identifier.

The core library must remain portable across supported operating systems and
embedded platforms. Keep operating-system-specific headers and code out of core
library files; platform-specific implementations belong in `ports/`. Include
`bacnet/bacdef.h` first in project headers. It includes
`bacnet/basic/sys/platform.h`, the shared libc and compiler portability layer;
put reusable compatibility abstractions there rather than duplicating them
throughout the codebase. Use `bacnet/config.h` as the central location for
project configuration defines. Product-specific settings can be supplied in a
local `bacnet-config.h` or through the build configuration, as documented in
`config.h`.

The application layer uses standard C structures with function pointers and
callbacks for its object-oriented patterns.

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
that is used for EPICS, printing, logging, and parsing. For example:
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
