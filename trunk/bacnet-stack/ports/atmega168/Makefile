###############################################################################
# Makefile for BACnet
###############################################################################

## General Flags
PROJECT = bacnet
MCU = atmega168
TARGET = bacnet
## Tools
CC = avr-gcc
AR = avr-ar
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size

CSRC = main.c \
	timer.c \
	rs485.c \
	dlmstp.c \
	../../npdu.c \
	../../bacint.c \
	../../bacaddr.c \
	../../crc.c

DEMOSRC = h_rp.c \
	h_wp.c \
	device.c \
	ai.c \
	../../demo/handler/txbuf.c \
	../../demo/handler/h_whois.c \
	../../demo/handler/h_rd.c \
	../../demo/handler/h_dcc.c

CORESRC =  \
	../../bacint.c \
	../../apdu.c \
	../../bacdcode.c \
	../../bacstr.c \
	../../abort.c \
	../../bacerror.c \
	../../reject.c \
	../../bacapp.c \
	../../datetime.c \
	../../rp.c \
	../../wp.c \
	../../dcc.c \
	../../rd.c \
	../../whois.c \
	../../iam.c \
	../../version.c

## Include Directories
INCLUDES = -I. -I../.. -I../../demo/object -I../../demo/handler

# Source to Object conversion
COBJ = $(CSRC:.c=.o)
DEMOOBJ = $(DEMOSRC:.c=.o)
COREOBJ = $(CORESRC:.c=.o)

LIBRARY = lib$(TARGET).a

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)
#OPTIMIZATION = -O0
#OPTIMIZATION = -Os
OPTIMIZATION = -Os -mcall-prologues

## Compile options common for all C compilation units.
BFLAGS = -DBACDL_MSTP -DMAX_APDU=50 -DBIG_ENDIAN=0 -DMAX_TSM_TRANSACTIONS=0
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 $(BFLAGS) $(OPTIMIZATION) -fsigned-char
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -Wl,-Map=$(TARGET).map,-L=.,-l$(TARGET)
#LDFLAGS += -Wl,-Map=$(TARGET).map

## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom
HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings

## Objects that must be built in order to link
OBJECTS = $(COBJ) $(DEMOOBJ)
#OBJECTS = $(COBJ)

## Build
TARGET_ELF=$(TARGET).elf

all: $(LIBRARY) $(TARGET_ELF) $(TARGET).hex $(TARGET).eep $(TARGET).lst \
	size Makefile

##Link
$(TARGET_ELF): $(OBJECTS) $(LIBRARY)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

%.hex: $(TARGET_ELF)
	$(OBJCOPY) -O ihex $(HEX_FLASH_FLAGS) $< $@

%.eep: $(TARGET_ELF)
	-$(OBJCOPY) $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lst: $(TARGET_ELF)
	$(OBJDUMP) -h -S $< > $@

lib: $(LIBRARY)

$(LIBRARY): $(COREOBJ) Makefile
	$(AR) rcs $@ $(COREOBJ)
	$(OBJDUMP) --syms $@ > $(LIBRARY:.a=.lst)

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $*.c -o $@

size: ${TARGET_ELF}
	@echo
	@${SIZE} -C --mcu=${MCU} ${TARGET_ELF}

## Clean target
.PHONY: clean
clean:
	touch Makefile
	-rm -rf $(OBJECTS) $(TARGET_ELF) dep/*
	-rm -rf $(LIBRARY) $(COREOBJ) $(LIBRARY:.a=.lst)
	-rm -rf $(TARGET).hex $(TARGET).eep $(TARGET).lst $(TARGET).map

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)
