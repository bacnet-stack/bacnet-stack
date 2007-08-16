###############################################################################
# Makefile for BACnet
###############################################################################

## General Flags
PROJECT = bacnet
MCU = atmega168
TARGET = bacnet
CC = avr-gcc
AR = avr-ar

CSRC = main.c \
	timer.c \
	rs485.c \
	dlmstp.c \
	..\..\mstp.c \
	..\..\crc.c

CORESRC = device.c \
	../../npdu.c \
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
INCLUDES = -I. -I../.. -I../../demo/object

# Source to Object conversion
COBJ = $(CSRC:.c=.o)
COREOBJ = $(CORESRC:.c=.o)

LIBRARY = lib$(TARGET).a

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)
OPTIMIZATION = -O0
#OPTIMIZATION = -Os

## Compile options common for all C compilation units.
BFLAGS = -DBACDL_MSTP -DMAX_APDU=50 -DBIG_ENDIAN=0
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 $(BFLAGS) $(OPTIMIZATION) -fsigned-char
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -Wl,-Map=$(TARGET).map

## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom
HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings

## Objects that must be built in order to link
OBJECTS = $(COBJ)

## Build
TARGET_ELF=$(TARGET).elf

all: $(TARGET_ELF) $(TARGET).hex $(TARGET).eep size $(LIBRARY) Makefile

##Link
$(TARGET_ELF): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBDIRS) $(LIBS) -o $@

%.hex: $(TARGET_ELF)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET_ELF)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET_ELF)
	avr-objdump -h -S $< > $@

lib: $(LIBRARY)

$(LIBRARY): $(COREOBJ) Makefile
	$(AR) rcs $@ $(COREOBJ)

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $*.c -o $@

size: ${TARGET_ELF}
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET_ELF}

## Clean target
.PHONY: clean
clean:
	touch Makefile
	-rm -rf $(OBJECTS) $(TARGET_ELF) dep/* 
	-rm -rf $(LIBRARY) $(COREOBJ)
	-rm -rf $(TARGET).hex $(TARGET).eep $(TARGET).lss $(TARGET).map

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)
