# Main Makefile for BACnet-stack project with GCC

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
BACNET_DEFINES ?= $(MY_BACNET_DEFINES)

# un-comment the next line to build the routing demo application
#BACNET_DEFINES += -DBAC_ROUTING

#BACDL_DEFINE=-DBACDL_ETHERNET=1
#BACDL_DEFINE=-DBACDL_ARCNET=1
#BACDL_DEFINE=-DBACDL_MSTP=1
BACDL_DEFINE?=-DBACDL_BIP=1

# Define WEAK_FUNC for [...somebody help here; I can't find any uses of it]
DEFINES = $(BACNET_DEFINES) $(BACDL_DEFINE) -DWEAK_FUNC=

# directories
BACNET_PORT ?= linux
BACNET_PORT_DIR = ../ports/${BACNET_PORT}

BACNET_OBJECT = ../demo/object
BACNET_HANDLER = ../demo/handler
BACNET_CORE = ../src
BACNET_INCLUDE = ../include
# compiler configuration
#STANDARDS = -std=c99
INCLUDE1 = -I$(BACNET_PORT_DIR) -I$(BACNET_OBJECT) -I$(BACNET_HANDLER)
INCLUDE2 = -I$(BACNET_INCLUDE)
INCLUDES = $(INCLUDE1) $(INCLUDE2)
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
CFLAGS  = $(WARNINGS) $(DEBUGGING) $(OPTIMIZATION) $(STANDARDS) $(INCLUDES) $(DEFINES)

# Export the variables defined here to all subprocesses
# (see http://www.gnu.org/software/automake/manual/make/Special-Targets.html)
.EXPORT_ALL_VARIABLES:

all: library demos
.PHONY : all library demos clean

library:
	$(MAKE) -s -C lib all

demos:
	$(MAKE) -s -C demo all

# Add "ports" to the build, if desired
ports:	atmega168 bdk-atxx4-mstp at91sam7s
.PHONY : atmega168 bdk-atxx4-mstp at91sam7s

atmega168:
	$(MAKE) -s -C ports/atmega168 clean all

at91sam7s:
	$(MAKE) -s -C ports/at91sam7s clean all

bdk-atxx4-mstp:
	$(MAKE) -s -C ports/bdk-atxx4-mstp clean all

clean:
	$(MAKE) -s -C lib clean
	$(MAKE) -s -C demo clean
