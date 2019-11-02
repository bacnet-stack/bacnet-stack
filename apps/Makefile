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
BACNET_LIB += -L$(UCI_LIB_DIR),-luci
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

#build for release (default) or debug
OPTIMIZATION ?= -Os
DEBUGGING ?=
WARNINGS ?= -Wall -Wmissing-prototypes
# dead code removal
ifeq (${BUILD},debug)
OPTIMIZATION = -O0
DEBUGGING = -g -DDEBUG_ENABLED=1
ifeq (${BACDL_DEFINE},-DBACDL_BIP=1)
BACNET_DEFINES += -DBIP_DEBUG
endif
endif

# If BACNET_DEFINES has not already been set, configure to your needs here
BACNET_DEFINES ?=
BACNET_DEFINES += -DPRINT_ENABLED=1
BACNET_DEFINES += -DBACAPP_ALL
BACNET_DEFINES += -DBACFILE
BACNET_DEFINES += -DINTRINSIC_REPORTING
BACNET_DEFINES += -DBACNET_TIME_MASTER
BACNET_DEFINES += -DBACNET_PROPERTY_LISTS=1
BACNET_DEFINES += -DBACNET_PROTOCOL_REVISION=17

PORT_ARCNET_SRC = \
	$(BACNET_PORT_DIR)/arcnet.c

PORT_MSTP_SRC = \
	$(BACNET_PORT_DIR)/rs485.c \
	$(BACNET_PORT_DIR)/dlmstp.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/mstp.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/mstptext.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/crc.c

PORT_ETHERNET_SRC = \
	$(BACNET_PORT_DIR)/ethernet.c

PORT_BIP_SRC = \
	$(BACNET_PORT_DIR)/bip-init.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/bvlc.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/bip.c

PORT_BIP6_SRC = \
	$(BACNET_PORT_DIR)/bip6.c \
	$(BACNET_SRC_DIR)/bacnet/basic/bbmd6/h_bbmd6.c \
	$(BACNET_SRC_DIR)/bacnet/basic/bbmd6/vmac.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/bvlc6.c

PORT_ALL_SRC = \
	$(BACNET_SRC_DIR)/bacnet/datalink/datalink.c \
	$(PORT_ARCNET_SRC) \
	$(PORT_MSTP_SRC) \
	$(PORT_ETHERNET_SRC) \
	$(PORT_BIP_SRC) \
	$(PORT_BIP6_SRC)

ifeq (${BACDL_DEFINE},-DBACDL_BIP=1)
BACNET_PORT_SRC = ${PORT_BIP_SRC}
endif
ifeq (${BACDL_DEFINE},-DBACDL_BIP6=1)
BACNET_PORT_SRC = ${PORT_BIP6_SRC}
endif
ifeq (${BACDL_DEFINE},-DBACDL_MSTP=1)
BACNET_PORT_SRC = ${PORT_MSTP_SRC}
endif
ifeq (${BACDL_DEFINE},-DBACDL_ARCNET=1)
BACNET_PORT_SRC = ${PORT_ARCNET_SRC}
endif
ifeq (${BACDL_DEFINE},-DBACDL_ETHERNET=1)
BACNET_PORT_SRC = ${PORT_ETHERNET_SRC}
endif
ifdef BACDL_ALL
BACNET_PORT_SRC = ${PORT_ALL_SRC}
endif

BACNET_PORT_SRC += \
	$(BACNET_SRC_DIR)/bacnet/datalink/dlenv.c \
	$(BACNET_PORT_DIR)/mstimer-init.c \
	$(BACNET_PORT_DIR)/datetime-init.c \

.EXPORT_ALL_VARIABLES:

SUBDIRS = readprop writeprop readfile writefile reinit server dcc \
	whohas whois iam ucov scov timesync epics readpropm readrange \
	writepropm uptransfer getevent uevent abort error

ifeq (${BACDL_DEFINE},-DBACDL_BIP=1)
	SUBDIRS += whoisrouter iamrouter initrouter readbdt
endif

ifeq (${BACNET_PORT},linux)
ifneq (${OSTYPE},cygwin)
	SUBDIRS += mstpcap mstpcrc
endif
endif

ifeq (${BACNET_PORT},win32)
	SUBDIRS += ptransfer mstpcap mstpcrc
endif

.PHONY: all clean
TARGETS = all clean

$(TARGETS): %: $(patsubst %, %.%, $(SUBDIRS))

$(foreach TGT, $(TARGETS), $(patsubst %, %.$(TGT), $(SUBDIRS))):
	$(MAKE) -s -b -C $(subst ., , $@)

.PHONY: gateway
gateway:
	$(MAKE) -s -b -C gateway

.PHONY: server
server:
	$(MAKE) -s -b -C server

.PHONY: mstpcap
mstpcap:
	$(MAKE) -b -C mstpcap

.PHONY: mstpcrc
mstpcrc:
	$(MAKE) -b -C mstpcrc

.PHONY: iam
iam:
	$(MAKE) -b -C iam

.PHONY: uevent
uevent:
	$(MAKE) -b -C uevent

.PHONY: abort
abort:
	$(MAKE) -b -C abort

.PHONY: error
error:
	$(MAKE) -b -C error

.PHONY: router
router:
	$(MAKE) -s -b -C router

.PHONY: router-ipv6
router-ipv6:
	$(MAKE) -b -C router-ipv6

.PHONY: writepropm
writepropm:
	$(MAKE) -b -C writepropm
