
# Directories
BACNET_PORT = linux
BACNET_PORT_DIR = ../../ports/${BACNET_PORT}
BACNET_INCLUDE = ../../include
# BACnet Library
BACNET_LIB_DIR = ../../lib
BACNET_LIB_NAME = bacnet
BACNET_LIB_TARGET = $(BACNET_LIB_DIR)/lib$(BACNET_LIB_NAME).a
# Compiler Setup
INCLUDES = -I$(BACNET_INCLUDE) -I$(BACNET_PORT_DIR)
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
	whohas whois ucov timesync epics readpropm mstpcap

ifneq (,$(findstring -DBAC_ROUTING,$(BACNET_DEFINES)))
	SUBDIRS += whoisrouter iamrouter initrouter gateway
endif


.PHONY : all $(SUBDIRS) clean

all : $(SUBDIRS)
	@echo "utilities are in the bin directory"

$(SUBDIRS) :
	$(MAKE) -C $@

clean :
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

