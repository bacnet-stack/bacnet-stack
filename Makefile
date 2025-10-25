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

.PHONY: mingw32
mingw32:
	i686-w64-mingw32-gcc --version
	ORIGINAL_CC=$(CC) ; \
	ORIGINAL_LD=$(LD) ; \
	export CC=i686-w64-mingw32-gcc ; \
	export LD=i686-w64-mingw32-ld ; \
	$(MAKE) BACNET_PORT=win32 LEGACY=true -s -C apps all ; \
	export CC=$(ORIGINAL_CC) ; \
	export LD=$(ORIGINAL_LD)

.PHONY: mstpwin32-clean
mstpwin32-clean:
	$(MAKE) LEGACY=true BACDL=mstp BACNET_PORT=win32 -s -C apps clean

.PHONY: mstpwin32
mstpwin32:
	$(MAKE) LEGACY=true BACDL=mstp BACNET_PORT=win32 -s -C apps all

.PHONY: mstp
mstp:
	$(MAKE) BACDL=mstp -s -C apps all

.PHONY: bip6-win32
bip6-win32:
	$(MAKE) LEGACY=true BACDL=bip6 BACNET_PORT=win32 -s -C apps all

.PHONY: bip6
bip6:
	$(MAKE) LEGACY=true BACDL=bip6 -s -C apps all

.PHONY: bip
bip:
	$(MAKE) LEGACY=true BACDL=bip -s -C apps all

.PHONY: bip-client
bip-client:
	$(MAKE) LEGACY=true BACDL=bip BBMD=client -s -C apps all

.PHONY: ethernet
ethernet:
	$(MAKE) LEGACY=true BACDL=ethernet -s -C apps all

# note: requires additional libraries to be installed
# see .github/workflows/gcc.yml
.PHONY: bsc
bsc:
	$(MAKE) BACDL=bsc -s -C apps all

.PHONY: apps
apps:
	$(MAKE) -s LEGACY=true -C apps all

.PHONY: lib
lib:
	$(MAKE) -s LEGACY=true -C apps $@

.PHONY: library
library:
	$(MAKE) -s LEGACY=true -C apps lib

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

.PHONY: add-list-element
add-list-element:
	$(MAKE) -s -C apps $@

.PHONY: apdu
apdu:
	$(MAKE) -s -C apps $@

.PHONY: blinkt
blinkt:
	$(MAKE) LEGACY=true -C apps $@

.PHONY: blinkt-pipeline
blinkt-pipeline:
	$(MAKE) LEGACY=true BUILD=pipeline -C apps blinkt

.PHONY: blinkt6
blinkt6:
	$(MAKE) LEGACY=true BACDL=bip6 -C apps blinkt

.PHONY: blinkt6-pipeline
blinkt6-pipeline:
	$(MAKE) LEGACY=true BACDL=bip6 BUILD=pipeline -C apps blinkt

.PHONY: create-object
create-object:
	$(MAKE) -s -C apps $@

.PHONY: dcc
dcc:
	$(MAKE) -s -C apps $@

.PHONY: delete-object
delete-object:
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
	$(MAKE) -s -B -C apps $@

.PHONY: gateway-win32
gateway-win32:
	$(MAKE) BACNET_PORT=win32 -s -B -C apps gateway

.PHONY: piface
piface:
	$(MAKE) CSTANDARD="-std=gnu11" LEGACY=true -s -C apps $@

.PHONY: piface6
piface6:
	$(MAKE) CSTANDARD="-std=gnu11" BACDL=bip6 LEGACY=true -s -C apps piface

.PHONY: readbdt
readbdt:
	$(MAKE) -s -C apps $@

.PHONY: readfdt
readfdt:
	$(MAKE) -s -C apps $@

.PHONY: readprop
readprop:
	$(MAKE) -s -C apps $@

.PHONY: readpropm
readpropm:
	$(MAKE) -s -C apps $@

.PHONY: remove-list-element
remove-list-element:
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

.PHONY: server-basic
server-basic:
	$(MAKE) LEGACY=true NOTIFY=false -s -C apps $@

.PHONY: server-basic-mstp
server-basic-mstp:
	$(MAKE) LEGACY=true NOTIFY=false BACDL=mstp -s -C apps server-basic

.PHONY: server-client
server-client:
	$(MAKE) LEGACY=true -s -C apps $@

.PHONY: server-discover
server-discover:
	$(MAKE) LEGACY=true -s -C apps $@

.PHONY: server-mini
server-mini:
	$(MAKE) LEGACY=true NOTIFY=false -s -C apps $@

.PHONY: sc-hub
sc-hub:
	$(MAKE) BACDL=bsc -s -C apps $@

.PHONY: mstpcap
mstpcap:
	$(MAKE) -s -C apps $@

.PHONY: mstpcrc
mstpcrc:
	$(MAKE) -s -C apps $@

.PHONY: uevent
uevent:
	$(MAKE) -s -C apps $@

.PHONY: who-am-i
who-am-i:
	$(MAKE) -s -C apps $@

.PHONY: whois
whois:
	$(MAKE) -s -C apps $@

.PHONY: writepropm
writepropm:
	$(MAKE) -s -C apps $@

.PHONY: writegroup
writegroup:
	$(MAKE) -s -C apps $@

.PHONY: you-are
you-are:
	$(MAKE) -s -C apps $@

.PHONY: router
router:
	$(MAKE) -s -C apps $@

.PHONY: router-ipv6
router-ipv6:
	$(MAKE) -s -B BACDL=bip-bip6 -C apps $@

.PHONY: router-ipv6-win32
router-ipv6-win32:
	$(MAKE) BACNET_PORT=win32 -s -B BACDL=bip-bip6 -C apps router-ipv6

.PHONY: router-ipv6-clean
router-ipv6-clean:
	$(MAKE) -C apps $@

.PHONY: router-mstp
router-mstp:
	$(MAKE) -s -B BACDL=bip-mstp -C apps $@

.PHONY: router-mstp-win32
router-mstp-win32:
	$(MAKE) BACNET_PORT=win32 -s -B BACDL=bip-mstp -C apps router-mstp

.PHONY: router-mstp-clean
router-mstp-clean:
	$(MAKE) -C apps $@

.PHONY: fuzz-libfuzzer
fuzz-libfuzzer:
	$(MAKE) -s -C apps $@

.PHONY: fuzz-afl
fuzz-afl:
	$(MAKE) -s -C apps $@

# Add "ports" to the build, if desired
.PHONY: ports
ports:	atmega328 bdk-atxx4-mstp at91sam7s stm32f10x stm32f4xx
	@echo "Built the ARM7 and AVR ports"

.PHONY: ports-clean
ports-clean: atmega328-clean bdk-atxx4-mstp-clean at91sam7s-clean \
	stm32f10x-clean stm32f4xx-clean xplained-clean

.PHONY: atmega328
atmega328: ports/atmega328/Makefile
	$(MAKE) -s -C ports/atmega328 clean all

.PHONY: atmega328-clean
atmega328-clean: ports/atmega328/Makefile
	$(MAKE) -s -C ports/atmega328 clean

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

AT91SAM7S_CMAKE_BUILD_DIR=ports/at91sam7s/build
.PHONY: at91sam7s-cmake
at91sam7s-cmake:
	[ -d $(AT91SAM7S_CMAKE_BUILD_DIR) ] || mkdir -p $(AT91SAM7S_CMAKE_BUILD_DIR)
	[ -d $(AT91SAM7S_CMAKE_BUILD_DIR) ] && cd $(AT91SAM7S_CMAKE_BUILD_DIR) && \
	cmake ../ && cmake --build . --clean-first

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

STM32F4XX_CMAKE_BUILD_DIR=ports/stm32f4xx/build
.PHONY: stm32f4xx-cmake
stm32f4xx-cmake:
	[ -d $(STM32F4XX_CMAKE_BUILD_DIR) ] || mkdir -p $(STM32F4XX_CMAKE_BUILD_DIR)
	[ -d $(STM32F4XX_CMAKE_BUILD_DIR) ] && cd $(STM32F4XX_CMAKE_BUILD_DIR) && \
	cmake ../ && cmake --build . --clean-first

.PHONY: xplained
xplained: ports/xplained/Makefile
	$(MAKE) -s -C ports/xplained clean all

.PHONY: xplained-clean
xplained-clean: ports/xplained/Makefile
	$(MAKE) -s -C ports/xplained clean

.PHONY: mstpsnap
mstpsnap: ports/linux/mstpsnap.mak
	$(MAKE) -s -C ports/linux -f mstpsnap.mak clean all

.PHONY: gtk-discover
gtk-discover:
	$(MAKE) LEGACY=true -s -C apps $@

.PHONY: dlmstp-linux
dlmstp-linux: ports/linux/dlmstp.mak
	$(MAKE) -s -C ports/linux -f dlmstp.mak clean all

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
	find ./test/bacnet -type f -iname *.h -o -iname *.c -exec \
	clang-format -i -style=file -fallback-style=none {} \;

CLANG_TIDY_OPTIONS = -fix-errors -checks="readability-braces-around-statements"
CLANG_TIDY_OPTIONS += -- -Isrc -Iports/linux
.PHONY: tidy
tidy:
	find ./src -iname *.h -o -iname *.c -exec clang-tidy {} $(CLANG_TIDY_OPTIONS) \;
	find ./apps -iname *.c -exec clang-tidy {} $(CLANG_TIDY_OPTIONS) \;

.PHONY: scan-build
scan-build:
	scan-build --status-bugs -analyze-headers make -j2 LEGACY=true server

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

CPPCHECK_OPTIONS = --enable=warning,portability,style
CPPCHECK_OPTIONS += --template=gcc
CPPCHECK_OPTIONS += --inline-suppr
CPPCHECK_OPTIONS += --inconclusive
CPPCHECK_OPTIONS += --suppress=selfAssignment
CPPCHECK_OPTIONS += --suppress=integerOverflow
CPPCHECK_OPTIONS += --suppress=variableScope
CPPCHECK_OPTIONS += --suppress=unreadVariable
CPPCHECK_OPTIONS += --suppress=knownConditionTrueFalse
CPPCHECK_OPTIONS += --suppress=constParameter
CPPCHECK_OPTIONS += --suppress=redundantAssignment
CPPCHECK_OPTIONS += --suppress=duplicateCondition
CPPCHECK_OPTIONS += --suppress=funcArgNamesDifferent
CPPCHECK_OPTIONS += --suppress=unusedStructMember
CPPCHECK_OPTIONS += --suppress=uselessAssignmentPtrArg
CPPCHECK_OPTIONS += --suppress=cert-MSC30-c
CPPCHECK_OPTIONS += --suppress=cert-STR05-C
CPPCHECK_OPTIONS += --suppress=cert-API01-C
CPPCHECK_OPTIONS += --suppress=cert-MSC24-C
CPPCHECK_OPTIONS += --suppress=cert-INT31-c
# new in cppcheck 2.13
CPPCHECK_OPTIONS += --suppress=constParameterCallback
CPPCHECK_OPTIONS += --suppress=constParameterPointer
CPPCHECK_OPTIONS += --suppress=constVariablePointer
# suppress the deprecated warning for the BACnet stack
CPPCHECK_OPTIONS += -DBACNET_STACK_DEPRECATED
#CPPCHECK_OPTIONS += -I./src
#CPPCHECK_OPTIONS += --enable=information --check-config
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

# McCabe's Cyclomatic Complexity Scores
# sudo apt install pmccable
COMPLEXITY_SRC = \
	$(wildcard ./src/bacnet/*.c) \
	$(wildcard ./src/bacnet/basic/*.c) \
	$(wildcard ./src/bacnet/basic/binding/*.c) \
	$(wildcard ./src/bacnet/basic/service/*.c) \
	$(wildcard ./src/bacnet/basic/sys/*.c) \
	./src/bacnet/basic/npdu/h_npdu.c \
	./src/bacnet/basic/npdu/s_router.c \
	./src/bacnet/basic/tsm/tsm.c

.PHONY: pmccabe
pmccabe:
	pmccabe $(COMPLEXITY_SRC) | awk '{print $$2,$$6,$$7}' | sort -nr | head -20

# sudo apt install complexity
#  0-9 Easily maintained code.
# 10-19 Maintained with little trouble.
# 20-29 Maintained with some effort.
# 30-39 Difficult to maintain code.
# 40-49 Hard to maintain code.
# 50-99 Unmaintainable code.
# 100-199 Crazy making difficult code.
# 200+ I only wish I were kidding.
.PHONY: complexity
complexity:
	complexity $(COMPLEXITY_SRC)

# sudo apt install sloccount
.PHONY: sloccount
sloccount:
	sloccount .

# sudo apt install doxygen
.PHONY: doxygen
doxygen:
	doxygen BACnet-stack.doxyfile

.PHONY: clean
clean: ports-clean
	$(MAKE) -s -C src clean
	$(MAKE) -s -C apps clean
	$(MAKE) -s -C apps/router clean
	$(MAKE) -s -C apps/router-ipv6 clean
	$(MAKE) -s -C apps/router-mstp clean
	$(MAKE) -s -C apps/gateway clean
	$(MAKE) -s -C apps/sc-hub clean
	$(MAKE) -s -C apps/fuzz-afl clean
	$(MAKE) -s -C apps/fuzz-libfuzzer clean
	$(MAKE) -s -C ports/lwip clean
	$(MAKE) -s -C test clean
	$(MAKE) -s -C ports/linux -f mstpsnap.mak clean
	$(MAKE) -s -C ports/linux -f dlmstp.mak clean
	rm -rf ./build

.PHONY: test
test:
	$(MAKE) -s -C test clean
	$(MAKE) -s -j -C test all

.PHONY: retest
retest:
	$(MAKE) -s -j -C test retest

.PHONY: test-bsc
test-bsc:
	$(MAKE) -s -C test clean
	$(MAKE) -s -j -C test test-bsc

# Zephyr unit testing with twister
# expects zephyr to be installed in ../zephyr in Workspace
# expects ZEPHYR_BASE to be set. E.g. source ../zephyr/zephyr-env.sh
# see https://docs.zephyrproject.org/latest/getting_started/index.html
TWISTER_RESULTS=../twister-out.unit_testing
.PHONY: twister
twister:
ifndef ZEPHYR_BASE
	$(error ZEPHYR_BASE is undefined)
endif
	$(ZEPHYR_BASE)/scripts/twister -O $(TWISTER_RESULTS) -p unit_testing -T zephyr/tests

.PHONY: twister-clean
twister-clean:
	-rm -rf $(TWISTER_RESULTS)
