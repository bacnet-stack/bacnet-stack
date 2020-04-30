# Main Makefile for BACnet-stack applications, tests, and sample ports

# Export the variables defined here to all subprocesses
# (see http://www.gnu.org/software/automake/manual/make/Special-Targets.html)
.EXPORT_ALL_VARIABLES:

# all: demos router-ipv6 ${DEMO_LINUX}

.PHONY: all
all: apps

.PHONY: win32
win32:
	$(MAKE) BACNET_PORT=win32 -C apps all

.PHONY: mstpwin32
mstpwin32:
	$(MAKE) BACDL=mstp BACNET_PORT=win32 -C apps all

.PHONY: mstp
mstp:
	$(MAKE) BACDL=mstp -C apps all

.PHONY: bip6-win32
bip6-win32:
	$(MAKE) BACDL=bip6 BACNET_PORT=win32 -C apps all

.PHONY: bip6
bip6:
	$(MAKE) BACDL=bip6 -C apps all

.PHONY: apps
apps:
	$(MAKE) -s -C apps all

.PHONY: abort
abort:
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
	$(MAKE) BACNET_PORT=win32 -C apps gateway

.PHONY: readbdt
readbdt:
	$(MAKE) -s -C apps $@

.PHONY: readfdt
readfdt:
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
	$(MAKE) -C apps $@

.PHONY: writepropm
writepropm:
	$(MAKE) -s -C apps $@

.PHONY: router
router:
	$(MAKE) -s -C apps $@

.PHONY: router-ipv6
router-ipv6:
	$(MAKE) -s -C apps $@

# Add "ports" to the build, if desired
.PHONY: ports
ports:	atmega168 bdk-atxx4-mstp at91sam7s stm32f10x
	@echo "Built the ARM7 and AVR ports"

.PHONY: atmega168
atmega168: ports/atmega168/Makefile
	$(MAKE) -s -C ports/atmega168 clean all

.PHONY: at91sam7s
at91sam7s: ports/at91sam7s/Makefile
	$(MAKE) -C ports/at91sam7s clean all

.PHONY: stm32f10x
stm32f10x: ports/stm32f10x/Makefile
	$(MAKE) -C ports/stm32f10x clean all

.PHONY: mstpsnap
mstpsnap: ports/linux/mstpsnap.mak
	$(MAKE) -s -C ports/linux -f mstpsnap.mak clean all

.PHONY: bdk-atxx4-mstp
bdk-atxx4-mstp: ports/bdk-atxx4-mstp/Makefile
	$(MAKE) -s -C ports/bdk-atxx4-mstp clean all

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

.PHONY: tidy
tidy:
	find ./src -iname *.h -o -iname *.c -exec \
	clang-tidy {} -fix-errors -checks="readability-braces-around-statements" \
	-- -Isrc -Iports/linux \;

.PHONY: lint
lint:
	scan-build --status-bugs -analyze-headers make -j2 clean server

.PHONY: clean
clean:
	$(MAKE) -s -C src clean
	$(MAKE) -s -C apps clean
	$(MAKE) -s -C apps/router clean
	$(MAKE) -s -C apps/router-ipv6 clean
	$(MAKE) -s -C apps/gateway clean
	$(MAKE) -s -C test clean

.PHONY: test
test:
	$(MAKE) -s -C test clean
	$(MAKE) -s -C test all
	$(MAKE) -s -C test report

