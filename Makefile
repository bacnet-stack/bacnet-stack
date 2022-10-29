# Main Makefile for BACnet-stack applications, tests, and sample ports

# Export the variables defined here to all subprocesses
# (see http://www.gnu.org/software/automake/manual/make/Special-Targets.html)
.EXPORT_ALL_VARIABLES:

# all: demos router-ipv6 ${DEMO_LINUX}

.PHONY: all
all: apps

.PHONY: bsd
bsd:
	$(MAKE) BACNET_PORT=bsd -s -C apps all

.PHONY: win32
win32:
	$(MAKE) BACNET_PORT=win32 -s -C apps all

.PHONY: mstpwin32
mstpwin32:
	$(MAKE) BACDL=mstp BACNET_PORT=win32 -s -C apps all

.PHONY: mstp
mstp:
	$(MAKE) BACDL=mstp -s -C apps all

.PHONY: bip6-win32
bip6-win32:
	$(MAKE) BACDL=bip6 BACNET_PORT=win32 -s -C apps all

.PHONY: bip6
bip6:
	$(MAKE) BACDL=bip6 -s -C apps all

.PHONY: ethernet
ethernet:
	$(MAKE) BACDL=ethernet -s -C apps all

.PHONY: apps
apps:
	$(MAKE) -s -C apps all

.PHONY: lib
lib:
	$(MAKE) -s -C apps $@

CMAKE_BUILD_DIR=build
.PHONY: cmake
cmake:
	[ -d $(CMAKE_BUILD_DIR) ] || mkdir -p $(CMAKE_BUILD_DIR)
	[ -d $(CMAKE_BUILD_DIR) ] && cd $(CMAKE_BUILD_DIR) && cmake .. -DBUILD_SHARED_LIBS=ON && cmake --build . --clean-first

.PHONY: cmake-win32
cmake-win32:
	mkdir -p $(CMAKE_BUILD_DIR)
	cd $(CMAKE_BUILD_DIR) && cmake ../ -DBACNET_STACK_BUILD_APPS=ON && cmake --build ./ --clean-first
	cp $(CMAKE_BUILD_DIR)/Debug/*.exe ./bin/.

.PHONY: abort
abort:
	$(MAKE) -s -C apps $@

.PHONY: ack-alarm
ack-alarm:
	$(MAKE) -s -C apps $@

.PHONY: dcc
dcc:
	$(MAKE) -s -C apps $@

.PHONY: epics
epics:
	$(MAKE) -s -C apps $@

.PHONY: error
error:
	$(MAKE) -s -C apps $@

.PHONY: event
event:
	$(MAKE) -s -C apps $@

.PHONY: iam
iam:
	$(MAKE) -s -C apps $@

.PHONY: getevent
getevent:
	$(MAKE) -s -C apps $@

.PHONY: gateway
gateway:
	$(MAKE) -s -C apps $@

.PHONY: gateway-win32
gateway-win32:
	$(MAKE) BACNET_PORT=win32 -s -C apps gateway

.PHONY: readbdt
readbdt:
	$(MAKE) -s -C apps $@

.PHONY: readfdt
readfdt:
	$(MAKE) -s -C apps $@

.PHONY: writebdt
writebdt:
	$(MAKE) -s -C apps $@

.PHONY: whatisnetnum
whatisnetnum:
	$(MAKE) -s -C apps $@

.PHONY: netnumis
netnumis:
	$(MAKE) -s -C apps $@

.PHONY: server
server:
	$(MAKE) -s -C apps $@

.PHONY: mstpcap
mstpcap:
	$(MAKE) -s -C apps $@

.PHONY: mstpcrc
mstpcrc:
	$(MAKE) -s -C apps $@

.PHONY: uevent
uevent:
	$(MAKE) -s -C apps $@

.PHONY: whois
whois:
	$(MAKE) -s -C apps $@

.PHONY: writepropm
writepropm:
	$(MAKE) -s -C apps $@

.PHONY: router
router:
	$(MAKE) -s -C apps $@

.PHONY: router-ipv6
router-ipv6:
	$(MAKE) -s -C apps $@

.PHONY: router-mstp
router-mstp:
	$(MAKE) -s -C apps $@

# Add "ports" to the build, if desired
.PHONY: ports
ports:	atmega168 bdk-atxx4-mstp at91sam7s stm32f10x stm32f4xx
	@echo "Built the ARM7 and AVR ports"

.PHONY: ports-clean
ports-clean: atmega168-clean bdk-atxx4-mstp-clean at91sam7s-clean stm32f10x-clean stm32f4xx-clean

.PHONY: atmega168
atmega168: ports/atmega168/Makefile
	$(MAKE) -s -C ports/atmega168 clean all

.PHONY: atmega168-clean
atmega168-clean: ports/atmega168/Makefile
	$(MAKE) -s -C ports/atmega168 clean

.PHONY: bdk-atxx4-mstp
bdk-atxx4-mstp: ports/bdk-atxx4-mstp/Makefile
	$(MAKE) -s -C ports/bdk-atxx4-mstp clean all

.PHONY: bdk-atxx4-mstp-clean
bdk-atxx4-mstp-clean: ports/bdk-atxx4-mstp/Makefile
	$(MAKE) -s -C ports/bdk-atxx4-mstp clean

.PHONY: at91sam7s
at91sam7s: ports/at91sam7s/Makefile
	$(MAKE) -s -C ports/at91sam7s clean all

.PHONY: at91sam7s-clean
at91sam7s-clean: ports/at91sam7s/Makefile
	$(MAKE) -s -C ports/at91sam7s clean

.PHONY: stm32f10x
stm32f10x: ports/stm32f10x/Makefile
	$(MAKE) -s -C ports/stm32f10x clean all

.PHONY: stm32f10x-clean
stm32f10x-clean: ports/stm32f10x/Makefile
	$(MAKE) -s -C ports/stm32f10x clean

.PHONY: stm32f4xx
stm32f4xx: ports/stm32f4xx/Makefile
	$(MAKE) -s -C ports/stm32f4xx clean all

.PHONY: stm32f4xx-clean
stm32f4xx-clean: ports/stm32f4xx/Makefile
	$(MAKE) -s -C ports/stm32f4xx clean

.PHONY: mstpsnap
mstpsnap: ports/linux/mstpsnap.mak
	$(MAKE) -s -C ports/linux -f mstpsnap.mak clean all

.PHONY: lwip
lwip: ports/lwip/Makefile
	$(MAKE) -s -C ports/lwip clean all

.PHONY: pretty
pretty:
	find ./src -iname *.h -o -iname *.c -exec \
	clang-format -i -style=file -fallback-style=none {} \;
	find ./apps -iname *.h -o -iname *.c -exec \
	clang-format -i -style=file -fallback-style=none {} \;

.PHONY: pretty-apps
pretty-apps:
	find ./apps -iname *.h -o -iname *.c -exec \
	clang-format -i -style=file -fallback-style=none {} \;

.PHONY: pretty-ports
pretty-ports:
	find ./ports -maxdepth 2 -type f -iname *.h -o -iname *.c -exec \
	clang-format -i -style=file -fallback-style=none {} \;

.PHONY: pretty-test
pretty-test:
	find ./test/bacnet -maxdepth 2 -type f -iname *.h -o -iname *.c -exec \
	clang-format -i -style=file -fallback-style=none {} \;

CLANG_TIDY_OPTIONS = -fix-errors -checks="readability-braces-around-statements"
CLANG_TIDY_OPTIONS += -- -Isrc -Iports/linux
.PHONY: tidy
tidy:
	find ./src -iname *.h -o -iname *.c -exec clang-tidy {} $(CLANG_TIDY_OPTIONS) \;
	find ./apps -iname *.c -exec clang-tidy {} $(CLANG_TIDY_OPTIONS) \;

.PHONY: scan-build
scan-build:
	scan-build --status-bugs -analyze-headers make -j2 server

SPLINT_OPTIONS := -weak +posixlib +quiet \
	-D__signed__=signed -D__gnuc_va_list=va_list \
	-Isrc -Iports/linux \
	+matchanyintegral +ignoresigns -unrecog -preproc \
	+error-stream-stderr +warning-stream-stderr -warnposix \
	-bufferoverflowhigh

SPLINT_FIND_OPTIONS := ./src -path ./src/bacnet/basic/ucix -prune -o -name "*.c"

.PHONY: splint
splint:
	find $(SPLINT_FIND_OPTIONS) -exec splint $(SPLINT_OPTIONS) {} \;

CPPCHECK_OPTIONS = --enable=warning,portability
CPPCHECK_OPTIONS += --template=gcc
CPPCHECK_OPTIONS += --inline-suppr
CPPCHECK_OPTIONS += --suppress=selfAssignment
CPPCHECK_OPTIONS += --suppress=integerOverflow
CPPCHECK_OPTIONS += --error-exitcode=1
.PHONY: cppcheck
cppcheck:
	cppcheck $(CPPCHECK_OPTIONS) --quiet --force ./src/

.PHONY: flawfinder
flawfinder:
	flawfinder --minlevel 5 --error-level=5 ./src/

IGNORE_WORDS = ba,statics
CODESPELL_OPTIONS = --write-changes --interactive 3 --enable-colors
CODESPELL_OPTIONS += --ignore-words-list $(IGNORE_WORDS)
.PHONY: codespell
codespell:
	codespell $(CODESPELL_OPTIONS) ./src

SPELL_OPTIONS = --enable-colors --ignore-words-list $(IGNORE_WORDS)
.PHONY: spell
spell:
	codespell $(SPELL_OPTIONS) ./src

.PHONY: clean
clean: ports-clean
	$(MAKE) -s -C src clean
	$(MAKE) -s -C apps clean
	$(MAKE) -s -C apps/router clean
	$(MAKE) -s -C apps/router-ipv6 clean
	$(MAKE) -s -C apps/router-mstp clean
	$(MAKE) -s -C apps/gateway clean
	$(MAKE) -s -C ports/lwip clean
	$(MAKE) -s -C test clean
	rm -rf ./build

.PHONY: test
test:
	$(MAKE) -s -C test clean
	$(MAKE) -s -j -C test all
