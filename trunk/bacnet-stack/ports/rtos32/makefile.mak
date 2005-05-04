#
# Simple makefile to build an RTB executable for RTOS-32
#
# This makefile assumes Borland bcc32 development environment
# on Windows NT/9x/2000/XP
#

!ifndef RTOS32_DIR
RTOS32_DIR_Not_Defined:
   @echo .
   @echo You must define environment variable RTOS32_DIR to for this build.
!endif

!ifndef BORLAND_DIR
BORLAND_DIR_Not_Defined:
   @echo .
   @echo You must define environment variable BORLAND_DIR to compile
!endif

PRODUCT = bacnet
PRODUCT_RTB = $(PRODUCT).rtb
PRODUCT_EXE = $(PRODUCT).exe

SRCS = init.c main.c \
       ..\..\handlers.c  \
       ..\..\bip.c  \
       ..\..\bacdcode.c \
       ..\..\bigend.c \
       ..\..\whois.c \
       ..\..\iam.c \
       ..\..\rp.c \
       ..\..\wp.c \
       ..\..\device.c \
       ..\..\ai.c \
       ..\..\abort.c \
       ..\..\reject.c \
       ..\..\bacerror.c \
       ..\..\apdu.c \
       ..\..\npdu.c

OBJS = $(SRCS:.c=.obj)

# Compiler definitions
#
CC = $(BORLAND_DIR)\bin\bcc32 +bcc32.cfg
LINK = $(BORLAND_DIR)\bin\tlink32
TLIB = $(BORLAND_DIR)\bin\tlib
LOCATE = $(RTOS32_DIR)\bin\rtloc

#
# Include directories
#
CC_DIR     = $(BORLAND_DIR)\BIN
CC_INCLDIR = $(BORLAND_DIR)\include
INCL_DIRS = -I$(BORLAND_DIR)\include;$(RTOS32_DIR)\include;../../;
DEFINES = -DDOC;BACDL_BIP=1

CFLAGS = $(INCL_DIRS) $(CS_FLAGS) $(DEFINES)

# Libraries
#
RTOS32_LIB_DIR = $(RTOS32_DIR)\libbc
C_LIB_DIR = $(BORLAND_DIR)\lib

LIBDIR = $(RTOS32_LIB_DIR);$(C_LIB_DIR)

LIBS = $(RTOS32_LIB_DIR)\RTFILES.LIB \
$(RTOS32_LIB_DIR)\RTFSK32.LIB \
$(RTOS32_LIB_DIR)\DRVDOC.LIB \
$(RTOS32_LIB_DIR)\RTIP.LIB \
$(RTOS32_LIB_DIR)\RTK32.LIB \
$(RTOS32_LIB_DIR)\FLTEMUMT.LIB \
$(RTOS32_LIB_DIR)\DRVRT32.LIB \
$(RTOS32_LIB_DIR)\RTEMUMT.LIB \
$(RTOS32_LIB_DIR)\RTT32.LIB \
$(RTOS32_LIB_DIR)\RTTHEAP.LIB \
$(C_LIB_DIR)\DPMI32.lib \
$(C_LIB_DIR)\IMPORT32.lib \
$(C_LIB_DIR)\CW32MT.lib

#
# Main target
#
# This should be the first one in the makefile

all : $(PRODUCT_RTB) monitor.rtb

monitor.rtb: monitor.cfg hardware.cfg
  $(LOCATE) monitor

# debug using COM3 (ISA Card) as the debug port
# boot from floppy
debugcom3: hardware.cfg software.cfg $(PRODUCT_RTB) monitor.rtb
  $(LOCATE) -DDEBUGCOM3 monitor
  $(LOCATE) -d- -DMONITOR -DDEBUGCOM3 $(PRODUCT) software.cfg

$(PRODUCT_RTB): bcc32.cfg hardware.cfg software.cfg $(PRODUCT_EXE)
		@echo Running Locate on $(PRODUCT)
	  $(LOCATE) $(PRODUCT) software.cfg

# Linker specific: the link below is for BCC linker/compiler. If you link
# with a different linker - please change accordingly.
#

# need a temp response file (@&&) because command line is too long
$(PRODUCT_EXE) : $(OBJS)
	@echo Running Linker for $(PRODUCT_EXE)
	$(LINK) -L$(LINKER_LIB) -m -c -s -v @&&| # temp response file, starts with |
	  $(BORLAND_DIR)\lib\c0x32.obj $**  # $** lists each dependency
	$<
	$*.map
	$(LIBS)
| # end of temp response file

#
# Utilities

clean :
	@echo Deleting obj files, $(PRODUCT_EXE), $(PRODUCT_RTB) and map files.
	del *.obj
	del ..\..\*.obj
	del $(PRODUCT_EXE)
	del $(PRODUCT_RTB)
	del *.map
	del bcc32.cfg

install : $(PRODUCT)
	copy $(PRODUCT) ..\bin

#
# Generic rules
#
.SUFFIXES: .cpp .c .sbr .obj

#
# cc generic rule
#
.c.obj:
	@echo Compiling $@ from $<
	$(CC) $(CFLAGS) -c -o$@ $<

# Compiler configuration file
bcc32.cfg :
   Copy &&|
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
