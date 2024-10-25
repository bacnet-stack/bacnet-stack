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

# BACnet Library
BACNET_LIB_DIR = $(realpath ./lib)
BACNET_LIB_NAME = bacnet
BACNET_LIB_TARGET = $(BACNET_LIB_DIR)/lib$(BACNET_LIB_NAME).a
BACNET_LIB ?= -L$(BACNET_LIB_DIR) -l$(BACNET_LIB_NAME)

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
ifeq (${BACDL},bip-mstp)
BACDL_DEFINE=-DBACDL_ROUTER=1
endif
ifeq (${BACDL},bip-bip6)
BACDL_DEFINE=-DBACDL_ROUTER=1
endif
ifeq (${BACDL},all)
BACDL_DEFINE=-DBACDL_ALL=1
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
BBMD_DEFINE = -DBBMD_ENABLED=0 -DBBMD_CLIENT_ENABLED
endif
ifeq (${BBMD},full)
BBMD_DEFINE = -DBBMD_ENABLED=1 -DBBMD_CLIENT_ENABLED
endif

endif

# Define WEAK_FUNC for unsupported or specific compilers
BACNET_DEFINES += $(BACDL_DEFINE)
BACNET_DEFINES += $(BBMD_DEFINE)
BACNET_DEFINES += -DWEAK_FUNC=
BACNET_DEFINES += $(MAKE_DEFINE)

# Choose a BACnet Ports Directory for the example applications target OS
ifeq (${BACNET_PORT},)
	ifeq ($(OS),Windows_NT)
		BACNET_PORT = win32
	else
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S),Linux)
			BACNET_PORT = linux
		endif
		ifeq ($(UNAME_S),Darwin)
			BACNET_PORT = bsd
		endif
		ifeq ($(UNAME_S),FreeBSD)
			BACNET_PORT = bsd
		endif
		ifeq ($(UNAME_S),OpenBSD)
			BACNET_PORT = bsd
		endif
	endif
endif

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
CSTANDARD = -std=c99
endif
ifeq (${BACNET_PORT},win32)
# winget install --id=MSYS2.MSYS2  -e
# pacman -Syu mingw-w64-i686-toolchain
# add alias make=mingw32-make.exe to .bashrc
TARGET_EXT = .exe
SYSTEM_LIB=-lws2_32,-lgcc,-lm,-liphlpapi,-lwinmm
BACNET_DEFINES += -D_NO_OLDNAMES
endif

# source file locations
BACNET_PORT_DIR =  $(realpath ../ports/$(BACNET_PORT))
BACNET_SRC_DIR =  $(realpath ../src)

# Compiler flag to set the C Standard level.
# c89   - "ANSI" C - ISO C90
# gnu89 - c89 plus GNU extensions
# c90   - "ANSI" C - ISO C90
# gnu90 - C90 with GNU extensions
# c99   - ISO C99 standard
# gnu99 - C99 plus GNU extensions
# c11   - C11
# gnu11 - C11 plus GNU extensions
# c17   - C11 with corrections
# note: default is compiled as ANSI C for widest compiler compatibilty
# note: code that requires newer language features must use __STDC_VERSION__
CSTANDARD ?= -std=gnu89

#build for release (default) or debug
OPTIMIZATION ?= -Os
DEBUGGING ?=
# enable all relevant warnings that find bugs
WARNING_ALL := -Wall -Wextra -pedantic
WARNING_ALL += -Wfloat-equal -Wconversion
WARNING_ALL += -Wredundant-decls -Wswitch-default
WARNING_ALL += -Wunused-variable
# don't warn about conversion, sign, compares, long long and attributes
# since they are common in embedded
WARNING_ALL += -Wno-sign-conversion -Wno-conversion -Wno-sign-compare
WARNING_ALL += -Wno-long-long -Wno-attributes
# don't warn about implicit fallthrough since it's common in network protocols
WARNING_ALL += -Wno-implicit-fallthrough
#WARNING_ALL += -Werror
WARNINGS ?= $(WARNING_ALL)
# dead code removal
ifeq (${BUILD},debug)
OPTIMIZATION = -O0
DEBUGGING = -g -DDEBUG_ENABLED=1
ifeq (${BACDL_DEFINE},-DBACDL_BIP=1)
BACNET_DEFINES += -DBIP_DEBUG
endif
endif

ifeq (${LEGACY},true)
# disable deprecated function warnings for legacy builds
BACNET_DEFINES += -DBACNET_STACK_DEPRECATED_DISABLE
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
LFLAGS += $(BACNET_LIB)
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
# Debug linker commands
# LFLAGS += -Wl,-v

.EXPORT_ALL_VARIABLES:

SUBDIRS = lib readprop writeprop readfile writefile reinit server dcc \
	whohas whois iam ucov scov timesync epics readpropm readrange \
	writepropm uptransfer getevent uevent abort error event ack-alarm \
	server-client add-list-element remove-list-element create-object \
	delete-object server-discover apdu writegroup

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

ifeq (${BACNET_PORT},bsd)
	SUBDIRS += mstpcap mstpcrc
endif

#####
# Define target-specific fuzzing flags
#####

# AFL
fuzz-afl: CC=afl-gcc
fuzz-afl: FUZZ_FLAGS=-DFUZZING=1
fuzz-afl: LFLAGS += $(FUZZ_FLAGS)
fuzz-afl: CFLAGS += $(FUZZ_FLAGS)
fuzz-afl: export AFL_USE_ASAN=1

# LIBFUZZER
fuzz-libfuzzer: CC=clang
fuzz-libfuzzer: FUZZ_FLAGS=-DFUZZING=1 -fsanitize=fuzzer,address -g3 -Og -fno-optimize-sibling-calls -fno-omit-frame-pointer
fuzz-libfuzzer: LFLAGS += $(FUZZ_FLAGS)
fuzz-libfuzzer: CFLAGS += $(FUZZ_FLAGS)

#####
# Fuzz setup end
#####

.PHONY: all clean
TARGETS = all clean

$(TARGETS): %: $(patsubst %, %.%, $(SUBDIRS))

$(foreach TGT, $(TARGETS), $(patsubst %, %.$(TGT), $(SUBDIRS))):
	$(MAKE) -C $(subst ., , $@)

.PHONY: lib
lib: lib/Makefile Makefile $(BACNET_LIB_TARGET)

$(BACNET_LIB_TARGET):
	$(MAKE) -B -C lib

clean:
	$(MAKE) -C lib clean

.PHONY: gateway
gateway: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: abort
abort: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: ack-alarm
ack-alarm: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: add-list-element
add-list-element: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@ clean all

.PHONY: apdu
apdu: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: blinkt
blinkt:
	$(MAKE) -C $@

.PHONY: create-object
create-object: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: dcc
dcc: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: delete-object
delete-object: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: epics
epics: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: error
error: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: event
event: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: getevent
getevent: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: iam
iam: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: iamrouter
iamrouter: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: initrouter
initrouter: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: whatisnetnum
whatisnetnum: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: netnumis
netnumis: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: mstpcap
mstpcap:
	$(MAKE) -B -C $@

.PHONY: mstpcrc
mstpcrc:
	$(MAKE) -B -C $@

.PHONY: piface
piface:
	$(MAKE) -B -C $@

.PHONY: ptransfer
ptransfer: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: readprop
readprop: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: readpropm
readpropm: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: readbdt
readbdt: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: readfdt
readfdt: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: readfile
readfile: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: readrange
readrange: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: reinit
reinit: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: remove-list-element
remove-list-element: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@ clean all

.PHONY: scov
scov: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: server
server: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: server-client
server-client: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: server-discover
server-discover: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: timesync
timesync: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: uevent
uevent: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: uptransfer
uptransfer: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: whois
whois: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: writebdt
writebdt: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: writefile
writefile: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: router
router:
	$(MAKE) -B -C $@

.PHONY: router-ipv6
router-ipv6: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: router-ipv6-clean
router-ipv6-clean:
	$(MAKE) -C router-ipv6 clean

.PHONY: router-mstp
router-mstp: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: router-mstp-clean
router-mstp-clean:
	$(MAKE) -C router-mstp clean

.PHONY: fuzz-libfuzzer
fuzz-libfuzzer: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: fuzz-afl
fuzz-afl: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: writepropm
writepropm: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@

.PHONY: writegroup
writegroup: $(BACNET_LIB_TARGET)
	$(MAKE) -B -C $@
