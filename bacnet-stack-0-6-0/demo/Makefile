# tools - only if you need them.
# Most platforms have this already defined
# CC = gcc
# AR = ar
# MAKE = make
# SIZE = size
#
# Assumes rm and cp are available

# Directories
BACNET_PORT = linux
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
ifeq (${BACNET_PORT},linux)
PFLAGS = -pthread
TARGET_EXT =
LIBRARIES=-lc,-lgcc,-lrt,-lm,-L$(BACNET_LIB_DIR),-l$(BACNET_LIB_NAME)
endif
ifeq (${BACNET_PORT},win32)
TARGET_EXT = .exe
LIBRARY1=-L$(BACNET_LIB_DIR),-l$(BACNET_LIB_NAME)
LIBRARY2=-lws2_32,-lgcc,-lm,-liphlpapi,-lwinmm
LIBRARIES=$(LIBRARY1),$(LIBRARY2)
endif
#build for release (default) or debug
DEBUGGING =
OPTIMIZATION = -Os
ifeq (${BUILD},debug)
OPTIMIZATION = -O0
DEBUGGING = -g
endif
# put all the flags together
CFLAGS = -Wall $(DEBUGGING) $(OPTIMIZATION) $(INCLUDES) $(DEFINES)
LFLAGS = -Wl,$(LIBRARIES)

.EXPORT_ALL_VARIABLES:

SUBDIRS = readprop writeprop readfile writefile reinit server dcc \
	whohas whois ucov timesync epics readpropm mstpcap \
	whoisrouter iamrouter initrouter

ifneq (,$(findstring -DBAC_ROUTING,$(BACNET_DEFINES)))
	SUBDIRS += gateway
endif

TARGETS = all clean

$(TARGETS): %: $(patsubst %, %.%, $(SUBDIRS))

$(foreach TGT, $(TARGETS), $(patsubst %, %.$(TGT), $(SUBDIRS))):
	$(MAKE) -b -C $(subst ., , $@)
