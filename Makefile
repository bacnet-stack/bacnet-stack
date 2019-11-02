# Main Makefile for BACnet-stack applications, tests, and sample ports

# Export the variables defined here to all subprocesses
# (see http://www.gnu.org/software/automake/manual/make/Special-Targets.html)
.EXPORT_ALL_VARIABLES:

# all: demos router-ipv6 ${DEMO_LINUX}

.PHONY: all
all: server

.PHONY: apps
apps:
	$(MAKE) -s -C apps all

.PHONY: abort
abort:
	$(MAKE) -s -C apps abort

.PHONY: dcc
dcc:
	$(MAKE) -s -C apps dcc

.PHONY: epics
epics:
	$(MAKE) -s -C apps epics

.PHONY: gateway
gateway:
	$(MAKE) -B -s -C apps gateway

.PHONY: server
server:
	$(MAKE) -s -C apps server

.PHONY: mstpcap
mstpcap:
	$(MAKE) -B -C apps mstpcap

.PHONY: mstpcrc
mstpcrc:
	$(MAKE) -B -C apps mstpcrc

.PHONY: iam
iam:
	$(MAKE) -B -C apps iam

.PHONY: uevent
uevent:
	$(MAKE) -C apps uevent

.PHONY: writepropm
writepropm:
	$(MAKE) -s -C apps writepropm

.PHONY: error
error:
	$(MAKE) -C apps error

.PHONY: router
router:
	$(MAKE) -s -C apps router

.PHONY: router-ipv6
router-ipv6:
	$(MAKE) -s -C apps router-ipv6

# Add "ports" to the build, if desired
.PHONY: ports
ports:	atmega168 bdk-atxx4-mstp at91sam7s stm32f10x
	@echo "Built the ARM7 and AVR ports"

.PHONY: atmega168
atmega168: ports/atmega168/Makefile
	$(MAKE) -s -C ports/atmega168 clean all

.PHONY: at91sam7s
at91sam7s: ports/at91sam7s/Makefile
	$(MAKE) -s -C ports/at91sam7s clean all

.PHONY: stm32f10x
stm32f10x: ports/stm32f10x/Makefile
	$(MAKE) -s -C ports/stm32f10x clean all

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

.PHONY: clean
clean:
	$(MAKE) -s -C src clean
	$(MAKE) -s -C apps clean
	$(MAKE) -s -C apps/router clean
	$(MAKE) -s -C apps/router-ipv6 clean
	$(MAKE) -s -C apps/gateway clean

.PHONY: test
test:
	$(MAKE) -s -C test clean
	$(MAKE) -s -C test all
	$(MAKE) -s -C test report

