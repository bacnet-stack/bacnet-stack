# Copilot Cloud Agent Instructions for `bacnet-stack`

## What this repository is
- BACnet protocol stack in C with:
  - core library in `src/`
  - demo applications in `apps/`
  - unit/integration tests in `test/`
  - platform ports in `ports/`
- Two primary build systems are used in CI:
  - GNU Make (broad demo/port build matrix)
  - CMake (library/apps and tests)

## High-value paths to check first
- `README.md` - project overview and standard local commands
- `CONTRIBUTING.md` - CI expectations and naming conventions
- `Makefile` - top-level targets used by CI (`all`, `test`, `cppcheck`, `scan-build`, etc.)
- `test/Makefile` and `test/CMakeLists.txt` - how tests are configured and run
- `.github/workflows/*.yml` - authoritative CI behavior and dependency setup

## Recommended execution strategy
1. **Keep changes narrow**: this codebase has large build matrices; avoid unrelated edits.
2. **Start with quickest relevant checks** for touched areas, then expand if needed.
3. **Mirror CI commands** whenever possible (prefer existing Make targets).
4. **For docs-only changes**, run formatting/lint checks that apply to Markdown/YAML and avoid full cross-platform build scope unless requested.

## Common commands
### Build
- Main demo/library build (used broadly):
  - `make clean all`
- CMake build (alternative path, also used in workflow):
  - `cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo`
  - `cmake --build build`

### Test
- Standard top-level test target:
  - `make test`
- If already configured under `test/build`:
  - `cd test/build && ctest --quiet --output-on-failure`

### Lint/format/static checks
- Pre-commit hooks (format + quality gates used by CI):
  - `pre-commit run --all-files`
- Additional Make targets used in CI:
  - `make scan-build`
  - `make cppcheck`
  - `make splint`
  - `make spell`

## Dependency notes (from workflows)
- Frequent Linux package requirements:
  - `libconfig-dev` (router/gateway/app builds)
  - `lcov` (coverage step in test flow)
  - `clang-tools`, `cppcheck`, `splint`, `flawfinder`, `codespell` (quality jobs)
- BACnet/SC related builds/tests additionally require `libwebsockets`, `libssl`, `libuv`, `libcap` (see `bsc-tests-*` and `gcc.yml`).

## Runtime/network configuration hints
Demo apps rely on environment variables (see `doc/README.utils`), especially:
- `BACNET_IFACE`
- `BACNET_IP_PORT`
- `BACNET_BBMD_PORT`
- `BACNET_BBMD_TIMETOLIVE`
- `BACNET_BBMD_ADDRESS`

## Known issues encountered during onboarding and workarounds
1. **`pre-commit` not installed by default**
   - Error: `pre-commit: command not found`
   - Workaround used:
     - `python3 -m pip install --user pre-commit`
     - `~/.local/bin/pre-commit run --all-files`

2. **`make test` can fail after successful test build/run when `lcov` is missing**
   - Error at the end of `make test`: `lcov: No such file or directory`
   - Why: `test/Makefile` always runs the `lcov` target after `ctest`
   - Workarounds:
     - Install lcov (`sudo apt-get install -y lcov`) and rerun `make test`, or
     - Run tests directly without coverage generation:
       - `cd test/build && ctest --quiet --output-on-failure`

3. **`make clean` may print missing AVR toolchain messages on environments without AVR GCC**
   - Observed message: `/bin/sh: 1: avr-gcc: not found` from `ports/atmega328` clean path
   - Impact: host Linux app build still completed successfully in this environment
   - Workarounds:
     - Install AVR toolchain when working on AVR ports (`gcc-avr avr-libc binutils-avr`), or
     - Focus on host-only targets when AVR work is out of scope.

## Code style and conventions
- Formatting is enforced by `.clang-format` and pre-commit hooks.
- `.editorconfig` enforces spaces (indent size 4 by default; YAML 2; Makefiles use tabs).
- Naming conventions and BACnet-specific type/function naming guidance are in `CONTRIBUTING.md`.

## When modifying CI or build logic
- Validate against the related workflow files in `.github/workflows/`.
- Treat CI/build/config changes as non-trivial and verify affected Make/CMake paths locally when feasible.
