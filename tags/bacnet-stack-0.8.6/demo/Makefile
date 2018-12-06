# tools - only if you need them.
# Most platforms have this already defined
# CC = gcc
# AR = ar
# MAKE = make
# SIZE = size
#
# Assumes rm and cp are available

# Directories
BACNET_PORT ?= linux
BACNET_PORT_DIR = ../../ports/${BACNET_PORT}
BACNET_INCLUDE = ../../include
BACNET_OBJECT = ../../demo/object
BACNET_HANDLER = ../../demo/handler
# BACnet Library
BACNET_LIB_DIR = ../../lib
BACNET_LIB_NAME = bacnet
BACNET_LIB_TARGET = $(BACNET_LIB_DIR)/lib$(BACNET_LIB_NAME).a
# Compiler Setup
INCLUDE1 = -I$(BACNET_PORT_DIR) -I$(BACNET_OBJECT) -I$(BACNET_HANDLER)
INCLUDE2 = -I$(BACNET_INCLUDE)
INCLUDES = $(INCLUDE1) $(INCLUDE2)
BACNET_LIB=-L$(BACNET_LIB_DIR),-l$(BACNET_LIB_NAME)
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
DEFINES += -D_NO_OLDNAMES
endif
#build for release (default) or debug
DEBUGGING =
OPTIMIZATION = -Os
ifeq (${BUILD},debug)
OPTIMIZATION = -O0
DEBUGGING = -g -DDEBUG_ENABLED=1
endif
# put all the flags together
CFLAGS := -Wall $(DEBUGGING) $(OPTIMIZATION) $(INCLUDES) $(DEFINES)
LFLAGS := -Wl,$(BACNET_LIB),$(SYSTEM_LIB)

.EXPORT_ALL_VARIABLES:

SUBDIRS = readprop writeprop readfile writefile reinit server dcc \
	whohas whois ucov scov timesync epics readpropm readrange \
	uptransfer

ifeq (${BACDL_DEFINE},-DBACDL_BIP=1)
	SUBDIRS += whoisrouter iamrouter initrouter readbdt
endif
ifeq (${BACNET_PORT},linux)
SUBDIRS += mstpcap mstpcrc
#SUBDIRS += router
endif

ifeq (${BACNET_PORT},win32)
SUBDIRS += ptransfer mstpcap mstpcrc
endif

.PHONY : all gateway router clean

TARGETS = all clean

$(TARGETS): %: $(patsubst %, %.%, $(SUBDIRS))

$(foreach TGT, $(TARGETS), $(patsubst %, %.$(TGT), $(SUBDIRS))):
	$(MAKE) -s -b -C $(subst ., , $@)

gateway:
	$(MAKE) -s -b -C gateway

mstpcap:
	$(MAKE) -s -b -C mstpcap

router:
	$(MAKE) -s -b -C router

router-ipv6:
	$(MAKE) -b -C router-ipv6

