#
# Simple makefile to build an executable for Win32 console
#
# This makefile assumes Borland bcc32 development environment
# on Windows NT/9x/2000/XP, which includes make, bcc32, and ilink
#

!ifndef BORLAND_DIR
BORLAND_DIR_Not_Defined:
	@echo .
	@echo You must define environment variable BORLAND_DIR to compile.
!endif

PRODUCT = bacnet
PRODUCT_EXE = $(PRODUCT).exe

# tools
CC = $(BORLAND_DIR)\bin\bcc32
MAKE=$(BORLAND_DIR)\bin\make.exe
#LINK = $(BORLAND_DIR)\bin\tlink32
LINK = $(BORLAND_DIR)\bin\ilink32

BACNET_LIB_DIR = ..\..\lib
BACNET_LIB = $(BACNET_LIB_DIR)\bacnet.lib

# directories
BACNET_PORT = .
BACNET_INCLUDE = ..\..\include
BACNET_OBJECT = ..\..\demo\object
BACNET_HANDLER = ..\..\demo\handler
BACNET_CORE = ..\..\src
INCLUDES = \
	-I$(BACNET_INCLUDE) \
	-I$(BACNET_OBJECT) \
	-I$(BACNET_HANDLER) \
	-I$(BACNET_PORT) \
	-I$(BORLAND_DIR)\include

#
BACNET_DEFINES = -DPRINT_ENABLED=1
#BACDL_DEFINE=-DBACDL_MSTP=1
BACDL_DEFINE=-DBACDL_BIP=1 -DUSE_INADDR=1
DEFINES = $(BACNET_DEFINES) $(BACDL_DEFINE)

SRCS = main.c

OBJS = $(SRCS:.c=.obj)

#
# Compiler definitions
#
BCC_CFG = bcc32.cfg

#
# Include directories
#
CFLAGS = $(INCLUDES) $(DEFINES)

#
# Libraries
#
C_LIB_DIR = $(BORLAND_DIR)\lib

LIBS = $(BACNET_LIB) \
	$(C_LIB_DIR)\IMPORT32.lib \
	$(C_LIB_DIR)\CW32MT.lib \

#
# Main target
#
# This should be the first one in the makefile

all : $(BACNET_LIB) $(BCC_CFG) $(OBJS) $(PRODUCT_EXE)
	del $(BCC_CFG)

install: $(PRODUCT_EXE)
	copy $(PRODUCT_EXE) ..\..\utils\$(PRODUCT_EXE)

# Linker specific: the link below is for BCC linker/compiler. If you link
# with a different linker - please change accordingly.
#

# need a temp response file (@&&| ... |) because command line is too long
# $** lists each dependency
# $< target name
# $* target name without extension
$(PRODUCT_EXE) : $(OBJS)
	@echo Running Linker for $(PRODUCT_EXE)
	$(LINK)	-L$(C_LIB_DIR) -L$(BACNET_LIB_DIR) -m -c -s -v @&&|
		$(BORLAND_DIR)\lib\c0x32.obj $**
	$<
	$*.map
	$(LIBS)
|

#
# Utilities

clean :
	del $(OBJS)
	del $(PRODUCT_EXE)
	del $(PRODUCT).map
	del $(PRODUCT).ilc
	del $(PRODUCT).ild
	del $(PRODUCT).ilf
	del $(PRODUCT).ils
	del $(PRODUCT).tds
	del $(BCC_CFG)

#
# Generic rules
#
.SUFFIXES: .cpp .c .sbr .obj

#
# cc generic rule
#
.c.obj:
	$(CC) +$(BCC_CFG) -o$@ $<

# Compiler configuration file
$(BCC_CFG) :
	Copy &&|
	$(CFLAGS)
	-c
	-y     #include line numbers in OBJ's
	-v     #include debug info
	-w+    #turn on all warnings
	-Od    #disable all optimizations
	#-a4    #32 bit data alignment
	#-M     # generate link map
	#-ls    # linker options
	#-WM-   #not multithread
	-WM    #multithread
	-w-aus # ignore warning assigned a value that is never used
	-w-sig # ignore warning conversion may lose sig digits
| $@

# EOF: makefile
