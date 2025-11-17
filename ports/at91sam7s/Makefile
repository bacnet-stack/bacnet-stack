# Makefile for AT91SAM7S evaluation kit with RS-485 Transceiver on UART0
# Written by Steve Karg <skarg@users.sourceforge.net> 06-Aug-2007

TARGET=bacnet

# Tools
#PREFIX=arm-elf-
PREFIX ?= arm-none-eabi-
#
CC = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
AR = $(PREFIX)ar
SIZE = $(PREFIX)size

LDSCRIPT = at91sam7s256.ld

BACNET_FLAGS = -DBACDL_MSTP
BACNET_FLAGS += -DMAX_TSM_TRANSACTIONS=0
BACNET_FLAGS += -DMAX_CHARACTER_STRING_BYTES=64
BACNET_FLAGS += -DMAX_OCTET_STRING_BYTES=64
BACNET_FLAGS += -DPRINT_ENABLED=0
BACNET_FLAGS += -DMAX_APDU=480
BACNET_FLAGS += -DCRC_USE_TABLE
BACNET_FLAGS += -DBACAPP_MINIMAL
ifeq (${LEGACY},true)
# disable deprecated function warnings for legacy builds
BACNET_FLAGS += -DBACNET_STACK_DEPRECATED_DISABLE
endif

BACNET_SRC = ../../src
BACNET_CORE = $(BACNET_SRC)/bacnet
BACNET_BASIC = $(BACNET_CORE)/basic
INCLUDES = -I. -I$(BACNET_SRC)
#OPTIMIZATION = -O0
OPTIMIZATION = -Os
CFLAGS = -fno-common $(INCLUDES) $(BACNET_FLAGS) -g
CFLAGS += -mno-thumb-interwork
# dead code removal
CFLAGS += -fdata-sections -ffunction-sections
# C standard to use for compiler
CSTANDARD = -std=gnu11
CFLAGS += $(CSTANDARD)
# enable all relevant warnings that find bugs
WARNING_ALL := -Wall -Wextra -Wfloat-equal -Wconversion
WARNING_ALL += -Wredundant-decls -Wswitch-default -pedantic
# don't warn about conversion, sign, compares, long long and attributes
# since they are common in embedded
WARNING_ALL += -Wno-sign-conversion -Wno-conversion -Wno-sign-compare
WARNING_ALL += -Wno-long-long -Wno-attributes
# don't warn about implicit fallthrough since it's common in network protocols
WARNING_ALL += -Wno-implicit-fallthrough
# the older Atmel SDK does not meet coding guidelines
WARNING_ALL += -Wno-comment -Wno-missing-braces
WARNING_ALL += -Wno-unused-variable -Wno-char-subscripts
# fix later
WARNING_ALL += -Wno-unused-parameter
WARNING_ALL += -Wno-type-limits
WARNING_ALL += -Wno-redundant-decls
#WARNING_ALL += -Werror
CFLAGS += $(WARNING_ALL)

LIBRARY = lib$(TARGET).a
#  -Wa,<options> Pass comma-separated <options> on to the assembler
AFLAGS = -Wa,-ahls,-mapcs-32,-adhlns=$(<:.s=.lst)
#  -Wl,<options> Pass comma-separated <options> on to the linker
LIBRARIES=-lc,-lgcc,-lm,-L.,-l$(TARGET)
LDFLAGS = -nostartfiles
LDFLAGS += -Wl,-nostdlib,-Map=$(TARGET).map,$(LIBRARIES),-T$(LDSCRIPT)
# dead code removal
LDFLAGS += -Wl,--gc-sections,-static
CPFLAGS = --output-target=binary
ODFLAGS	= -x --syms


ASRC = crt.s

PORTSRC = main.c \
	timer.c \
	isr.c \
	init.c \
	blinker.c \
	rs485.c \
	dlmstp.c

DEMOSRC = ai.c \
	av.c \
	bi.c \
	bv.c \
	device.c \
	netport.c \
	$(BACNET_BASIC)/tsm/tsm.c \
	$(BACNET_BASIC)/sys/debug.c \
	$(BACNET_BASIC)/sys/ringbuf.c \
	$(BACNET_BASIC)/npdu/h_npdu.c \
	$(BACNET_BASIC)/service/h_noserv.c \
	$(BACNET_BASIC)/service/h_apdu.c \
	$(BACNET_BASIC)/service/h_whohas.c \
	$(BACNET_BASIC)/service/h_whois.c \
	$(BACNET_BASIC)/service/h_rd.c \
	$(BACNET_BASIC)/service/h_rp.c \
	$(BACNET_BASIC)/service/h_rpm.c \
	$(BACNET_BASIC)/service/h_wp.c \
	$(BACNET_BASIC)/service/h_dcc.c \
	$(BACNET_BASIC)/service/s_iam.c \
	$(BACNET_BASIC)/service/s_ihave.c

CORESRC = $(BACNET_CORE)/abort.c \
	$(BACNET_CORE)/bacaction.c \
	$(BACNET_CORE)/bacaddr.c \
	$(BACNET_CORE)/bacapp.c \
	$(BACNET_CORE)/bacdcode.c \
	$(BACNET_CORE)/bacdest.c \
	$(BACNET_CORE)/bacdevobjpropref.c \
	$(BACNET_CORE)/bacerror.c \
	$(BACNET_CORE)/bacint.c \
	$(BACNET_CORE)/bacreal.c \
	$(BACNET_CORE)/bacstr.c \
	$(BACNET_CORE)/datalink/crc.c \
	$(BACNET_CORE)/datetime.c \
	$(BACNET_CORE)/dcc.c \
	$(BACNET_CORE)/iam.c \
	$(BACNET_CORE)/ihave.c \
	$(BACNET_CORE)/hostnport.c \
	$(BACNET_CORE)/lighting.c \
	$(BACNET_CORE)/memcopy.c \
	$(BACNET_CORE)/npdu.c \
	$(BACNET_CORE)/proplist.c \
	$(BACNET_CORE)/rd.c \
	$(BACNET_CORE)/reject.c \
	$(BACNET_CORE)/rp.c \
	$(BACNET_CORE)/rpm.c \
	$(BACNET_CORE)/timer_value.c \
	$(BACNET_CORE)/timestamp.c \
	$(BACNET_CORE)/weeklyschedule.c \
	$(BACNET_CORE)/dailyschedule.c \
	$(BACNET_CORE)/bactimevalue.c \
	$(BACNET_CORE)/calendar_entry.c \
	$(BACNET_CORE)/special_event.c \
	$(BACNET_CORE)/whohas.c \
	$(BACNET_CORE)/whois.c \
	$(BACNET_CORE)/wp.c

CSRC = $(PORTSRC) $(DEMOSRC)
#CSRC = $(PORTSRC)

AOBJ = $(ASRC:.s=.o)
COBJ = $(CSRC:.c=.o)
COREOBJ = $(CORESRC:.c=.o)

all: $(TARGET).bin $(TARGET).elf
	$(OBJDUMP) $(ODFLAGS) $(TARGET).elf > $(TARGET).dmp
	$(SIZE) $(TARGET).elf

$(TARGET).bin:  $(TARGET).elf
	$(OBJCOPY) $(TARGET).elf $(CPFLAGS) $(TARGET).bin

$(TARGET).elf: $(COBJ) $(AOBJ) $(LIBRARY) Makefile
	$(CC) $(CFLAGS) $(AOBJ) $(COBJ) $(LDFLAGS) -o $@

lib: $(LIBRARY)

$(LIBRARY): $(COREOBJ) Makefile
	$(AR) rcs $@ $(COREOBJ)

# allow a single file to be unoptimized for debugging purposes
#dlmstp.o:
#	$(CC) -c $(CFLAGS) $*.c -o $@
#
#main.o:
#	$(CC) -c $(CFLAGS) $*.c -o $@
#
#$(BACNET_CORE)/npdu.o:
#	$(CC) -c $(CFLAGS) $*.c -o $@
#
#$(BACNET_CORE)/apdu.o:
#	$(CC) -c $(CFLAGS) $*.c -o $@

.c.o:
	$(CC) -c $(OPTIMIZATION) $(CFLAGS) $*.c -o $@

.s.o:
	$(CC) -c $(AFLAGS) $*.s -o $@

.PHONY: clean
clean:
	-rm -rf $(COBJ) $(AOBJ) $(COREOBJ)
	-rm -rf $(TARGET).elf $(TARGET).bin $(TARGET).dmp $(TARGET).map
	-rm -rf $(LIBRARY)
	-rm -rf *.lst

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)
