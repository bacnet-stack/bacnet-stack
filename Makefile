# Main Makefile for BACnet-stack example applications using GCC

# tools - only if you need them.
# Most platforms have this already defined
# CC = gcc
# AR = ar
# MAKE = make
# SIZE = size
#
# Assumes rm and cp are available

# configuration
# If BACNET_DEFINES has not already been set, configure to your needs here
MY_BACNET_DEFINES = -DPRINT_ENABLED=1
MY_BACNET_DEFINES += -DBACAPP_ALL
MY_BACNET_DEFINES += -DBACFILE
MY_BACNET_DEFINES += -DINTRINSIC_REPORTING
MY_BACNET_DEFINES += -DBACNET_TIME_MASTER
MY_BACNET_DEFINES += -DBACNET_PROPERTY_LISTS=1
MY_BACNET_DEFINES += -DBACNET_PROTOCOL_REVISION=17
BACNET_DEFINES ?= $(MY_BACNET_DEFINES)

# build in uci integration
ifeq (${UCI},1)
BACNET_DEFINES += -DBAC_UCI
UCI_LIB_DIR ?= /usr/local/lib
endif

# choose a datalink to build the example applications
ifeq (${BACDL},ethernet)
BACDL_DEFINE=-DBACDL_ETHERNET=1
endif
ifeq (${BACDL},arcnet)
BACDL_DEFINE=-DBACDL_ARCNET=1
endif
ifeq (${BACDL},mstp)
BACDL_DEFINE=-DBACDL_MSTP=1
endif
ifeq (${BACDL},bip)
BACDL_DEFINE=-DBACDL_BIP=1
endif
ifeq (${BACDL},bip6)
BACDL_DEFINE=-DBACDL_BIP6=1
endif
ifeq (${BACDL},)
BACDL_DEFINE ?= -DBACDL_BIP=1
BBMD_DEFINE ?= -DBBMD_ENABLED=1
endif

ifeq (${BBMD},server)
BBMD_DEFINE=-DBBMD_ENABLED=1
endif
ifeq (${BBMD},client)
BBMD_DEFINE=-DBBMD_ENABLED=1
BBMD_DEFINE=-DBBMD_CLIENT_ENABLED
endif

# Passing parameters via command line
MAKE_DEFINE ?=

# Define WEAK_FUNC for [...somebody help here; I can't find any uses of it]
BACNET_DEFINES += $(BACDL_DEFINE)
BACNET_DEFINES += $(BBMD_DEFINE)
BACNET_DEFINES += -DWEAK_FUNC=
BACNET_DEFINES += $(MAKE_DEFINE)

# Choose a BACnet Ports Directory for the example applications target OS
# linux, win32, bsd
BACNET_PORT ?= linux

# Default compiler settings
OPTIMIZATION = -Os
DEBUGGING =
WARNINGS = -Wall -Wmissing-prototypes
ifeq (${BUILD},debug)
OPTIMIZATION = -O0
DEBUGGING = -g -DDEBUG_ENABLED=1
ifeq (${BACDL_DEFINE},-DBACDL_BIP=1)
DEFINES += -DBIP_DEBUG
endif
endif

# Export the variables defined here to all subprocesses
# (see http://www.gnu.org/software/automake/manual/make/Special-Targets.html)
.EXPORT_ALL_VARIABLES:

# all: demos router-ipv6 ${DEMO_LINUX}

all: server
.PHONY : all apps router gateway router-ipv6 clean test

apps:
	$(MAKE) -C apps all

gateway:
	$(MAKE) -B -s -C apps gateway

server:
	$(MAKE) -j -s -B -C apps server

mstpcap:
	$(MAKE) -B -C apps mstpcap

mstpcrc:
	$(MAKE) -B -C apps mstpcrc

iam:
	$(MAKE) -B -C apps iam

uevent:
	$(MAKE) -B -C apps uevent

writepropm:
	$(MAKE) -s -B -C apps writepropm

abort:
	$(MAKE) -B -C apps abort

error:
	$(MAKE) -B -C apps error

router:
	$(MAKE) -s -C apps router

router-ipv6:
	$(MAKE) -B -s -C apps router-ipv6

# Add "ports" to the build, if desired
ports:	atmega168 bdk-atxx4-mstp at91sam7s stm32f10x
	@echo "Built the ARM7 and AVR ports"

atmega168: ports/atmega168/Makefile
	$(MAKE) -s -C ports/atmega168 clean all

at91sam7s: ports/at91sam7s/Makefile
	$(MAKE) -s -C ports/at91sam7s clean all

stm32f10x: ports/stm32f10x/Makefile
	$(MAKE) -s -C ports/stm32f10x clean all

mstpsnap: ports/linux/mstpsnap.mak
	$(MAKE) -s -C ports/linux -f mstpsnap.mak clean all

bdk-atxx4-mstp: ports/bdk-atxx4-mstp/Makefile
	$(MAKE) -s -C ports/bdk-atxx4-mstp clean all

pretty:
	find ./src -iname *.h -o -iname *.c -exec \
	clang-format -i -style=file -fallback-style=none {} \;
	find ./apps -iname *.h -o -iname *.c -exec \
	clang-format -i -style=file -fallback-style=none {} \;

clean:
	$(MAKE) -s -C src clean
	$(MAKE) -s -C apps clean
	$(MAKE) -s -C apps/router clean
	$(MAKE) -s -C apps/router-ipv6 clean
	$(MAKE) -s -C apps/gateway clean

test:
	$(MAKE) -s -C test clean
	$(MAKE) -s -C test all
	$(MAKE) -s -C test report

