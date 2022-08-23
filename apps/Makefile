# tools - only if you need them.
# Most platforms have this already defined
# CC = gcc
# AR = ar
# MAKE = make
# SIZE = size
#
# Assumes rm and cp are available

# Passing parameters via command line or from Makefile export to this one
BACNET_DEFINES ?=
BACNET_LIB ?=

# BACnet Library
BACNET_LIB_DIR = $(realpath lib)
BACNET_LIB_NAME = bacnet
BACNET_LIB_TARGET = $(BACNET_LIB_DIR)/lib$(BACNET_LIB_NAME).a
BACNET_LIB=-L$(BACNET_LIB_DIR),-l$(BACNET_LIB_NAME)

# choose a datalink to build the example applications
# Use BACDL=mstp or BACDL=bip and BBMD=server when invoking make

ifeq (${BACDL_DEFINE},)
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
ifeq (${BACDL},none)
BACDL_DEFINE=-DBACDL_NONE=1
endif
ifeq (${BACDL},)
BACDL_DEFINE ?= -DBACDL_BIP=1
BBMD_DEFINE ?= -DBBMD_ENABLED=1 -DBBMD_CLIENT_ENABLED
endif

ifeq (${BBMD},none)
BBMD_DEFINE = -DBBMD_ENABLED=0
endif
ifeq (${BBMD},server)
BBMD_DEFINE = -DBBMD_ENABLED=1
endif
ifeq (${BBMD},client)
BBMD_DEFINE = -DBBMD_ENABLED=1 -DBBMD_CLIENT_ENABLED
endif

endif

# Define WEAK_FUNC for unsupported or specific compilers
BACNET_DEFINES += $(BACDL_DEFINE)
BACNET_DEFINES += $(BBMD_DEFINE)
BACNET_DEFINES += -DWEAK_FUNC=
BACNET_DEFINES += $(MAKE_DEFINE)

# Choose a BACnet Ports Directory for the example applications target OS
# linux, win32, bsd
BACNET_PORT ?= linux

# build in uci integration - use UCI=1 when invoking make
ifeq (${UCI},1)
BACNET_DEFINES += -DBAC_UCI
UCI_LIB_DIR ?= /usr/local/lib
BACNET_LIB += -L$(UCI_LIB_DIR) -luci
endif
# OS specific builds
ifeq (${BACNET_PORT},linux)
PFLAGS = -pthread
TARGET_EXT =
SYSTEM_LIB=-lc,-lgcc,-lrt,-lm
endif
ifeq (${BACNET_PORT},bsd)
PFLAGS = -pthread
TARGET_EXT =
SYSTEM_LIB=-lc,-lm
endif
ifeq (${BACNET_PORT},win32)
TARGET_EXT = .exe
SYSTEM_LIB=-lws2_32,-lgcc,-lm,-liphlpapi,-lwinmm
BACNET_DEFINES += -D_NO_OLDNAMES
endif

# source file locations
BACNET_PORT_DIR =  $(realpath ../ports/$(BACNET_PORT))
BACNET_SRC_DIR =  $(realpath ../src)

# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard
# gnu99 - c99 plus GCC extensions
CSTANDARD = -std=gnu89

#build for release (default) or debug
OPTIMIZATION ?= -Os
DEBUGGING ?=
WARNINGS ?= -Wall -Wmissing-prototypes -Wno-missing-braces
# dead code removal
ifeq (${BUILD},debug)
OPTIMIZATION = -O0
DEBUGGING = -g -DDEBUG_ENABLED=1
ifeq (${BACDL_DEFINE},-DBACDL_BIP=1)
BACNET_DEFINES += -DBIP_DEBUG
endif
endif

BACNET_DEFINES += -DPRINT_ENABLED=1
BACNET_DEFINES += -DBACAPP_ALL
BACNET_DEFINES += -DBACFILE
BACNET_DEFINES += -DINTRINSIC_REPORTING
BACNET_DEFINES += -DBACNET_TIME_MASTER
BACNET_DEFINES += -DBACNET_PROPERTY_LISTS=1
BACNET_DEFINES += -DBACNET_PROTOCOL_REVISION=24

# put all the flags together
INCLUDES = -I$(BACNET_SRC_DIR) -I$(BACNET_PORT_DIR)
CFLAGS += $(WARNINGS) $(DEBUGGING) $(OPTIMIZATION) $(BACNET_DEFINES) $(INCLUDES)
CFLAGS += $(CSTANDARD)
ifneq (${BACNET_LIB},)
LFLAGS += -Wl,$(BACNET_LIB)
endif
# BACnet library depends on system flags.
# System flags must go after BACnet library flags.
LFLAGS += -Wl,$(SYSTEM_LIB)
# GCC dead code removal
CFLAGS += -ffunction-sections -fdata-sections
ifeq ($(shell uname -s),Darwin)
LFLAGS += -Wl,-dead_strip
else
LFLAGS += -Wl,--gc-sections
endif

.EXPORT_ALL_VARIABLES:

SUBDIRS = lib readprop writeprop readfile writefile reinit server dcc \
	whohas whois iam ucov scov timesync epics readpropm readrange \
	writepropm uptransfer getevent uevent abort error event ack-alarm \
	server-client

ifeq (${BACDL_DEFINE},-DBACDL_BIP=1)
	SUBDIRS += whoisrouter iamrouter initrouter whatisnetnum netnumis
	ifneq (${BBMD},none)
	SUBDIRS += readbdt readfdt writebdt
	endif
endif

ifeq (${BACNET_PORT},linux)
ifneq (${OSTYPE},cygwin)
	SUBDIRS += mstpcap mstpcrc
endif
endif

ifeq (${BACNET_PORT},win32)
	SUBDIRS += mstpcap mstpcrc
endif

.PHONY: all clean
TARGETS = all clean

$(TARGETS): %: $(patsubst %, %.%, $(SUBDIRS))

$(foreach TGT, $(TARGETS), $(patsubst %, %.$(TGT), $(SUBDIRS))):
	$(MAKE) -b -C $(subst ., , $@)

.PHONY: lib
lib: lib/Makefile Makefile $(BACNET_LIB_TARGET)

$(BACNET_LIB_TARGET):
	$(MAKE) -b -C lib

.PHONY: gateway
gateway: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C gateway

.PHONY: abort
abort: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: ack-alarm
ack-alarm: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: dcc
dcc: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: epics
epics: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: error
error: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: event
event: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: getevent
getevent: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: iam
iam: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: iamrouter
iamrouter: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: initrouter
initrouter: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: whatisnetnum
whatisnetnum: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: netnumis
netnumis: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: mstpcap
mstpcap:
	$(MAKE) -b -C $@

.PHONY: mstpcrc
mstpcrc:
	$(MAKE) -b -C $@

.PHONY: ptransfer
ptransfer: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: readprop
readprop: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: readpropm
readpropm: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: readbdt
readbdt: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: readfdt
readfdt: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: readfile
readfile: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: readrange
readrange: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: reinit
reinit: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: scov
scov: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: server
server: $(BACNET_LIB_TARGET)
	$(MAKE) -s -b -C $@

.PHONY: server-client
server-client: $(BACNET_LIB_TARGET)
	$(MAKE) -s -b -C $@

.PHONY: timesync
timesync: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: uevent
uevent: $(BACNET_LIB_TARGET)
	$(MAKE) -C $@

.PHONY: whois
whois: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: writebdt
writebdt: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: writefile
writefile: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: router
router: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: router-ipv6
router-ipv6: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: router-mstp
router-mstp: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

.PHONY: writepropm
writepropm: $(BACNET_LIB_TARGET)
	$(MAKE) -b -C $@

