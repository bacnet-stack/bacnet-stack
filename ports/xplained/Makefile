###############################################################################
# Makefile for BACnet
###############################################################################

## General Flags
MCU = atxmega256a3bu
TARGET = bacnet
## Tools
# Choose a BACnet Ports Directory for the example applications target OS
ifeq (${PREFIX},)
	ifeq ($(OS),Windows_NT)
		PREFIX := "C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin\avr-"
	else
		UNAME_S := $(shell uname -s)
		ifeq ($(UNAME_S),Linux)
			PREFIX := avr-
		endif
		ifeq ($(UNAME_S),Darwin)
			PREFIX := avr-
		endif
	endif
endif

#Set the toolchain command names (only the ones needed are defined)
CC = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
AR = $(PREFIX)ar
AS = $(PREFIX)as
LD = $(PREFIX)ld
NM = $(PREFIX)nm
STRIP = $(PREFIX)strip
SIZE = $(PREFIX)size

# Source locations
SDK_DIR = external/ASF
BACNET_DIR = ../../src
BACNET_CORE = $(BACNET_DIR)/bacnet
BACNET_BASIC = $(BACNET_CORE)/basic

# local files for this project
CSRC = main.c \
	adc-hdw.c \
	ai.c \
	bacnet.c \
	bname.c \
	device.c \
	led.c \
	mstimer-init.c \
	netport.c \
	rs485.c \
	stack.c

# common demo files needed
BASICSRC = $(BACNET_BASIC)/tsm/tsm.c \
	$(BACNET_BASIC)/sys/bigend.c \
	$(BACNET_BASIC)/sys/debug.c \
	$(BACNET_BASIC)/sys/fifo.c \
	$(BACNET_BASIC)/sys/ringbuf.c \
	$(BACNET_BASIC)/sys/mstimer.c \
	$(BACNET_BASIC)/npdu/h_npdu.c \
	$(BACNET_BASIC)/service/h_apdu.c \
	$(BACNET_BASIC)/service/h_dcc.c \
	$(BACNET_BASIC)/service/h_rd.c \
	$(BACNET_BASIC)/service/h_rp.c \
	$(BACNET_BASIC)/service/h_rpm.c \
	$(BACNET_BASIC)/service/h_whois.c \
	$(BACNET_BASIC)/service/h_whohas.c \
	$(BACNET_BASIC)/service/h_wp.c \
	$(BACNET_BASIC)/service/s_cov.c \
	$(BACNET_BASIC)/service/s_iam.c \
	$(BACNET_BASIC)/service/s_ihave.c \
	$(BACNET_BASIC)/service/s_rp.c \
	$(BACNET_BASIC)/service/s_whois.c \
	$(BACNET_BASIC)/service/s_wp.c \
	$(BACNET_BASIC)/service/h_noserv.c

# core BACnet stack files
CORESRC =  \
	$(BACNET_CORE)/abort.c \
	$(BACNET_CORE)/bacaction.c \
	$(BACNET_CORE)/bacaddr.c \
	$(BACNET_CORE)/bacapp.c \
	$(BACNET_CORE)/bacdcode.c \
	$(BACNET_CORE)/bacdest.c \
	$(BACNET_CORE)/bacerror.c \
	$(BACNET_CORE)/bacint.c \
	$(BACNET_CORE)/bacreal.c \
	$(BACNET_CORE)/bacstr.c \
	$(BACNET_CORE)/datetime.c \
	$(BACNET_CORE)/datalink/cobs.c \
	$(BACNET_CORE)/datalink/crc.c \
	$(BACNET_CORE)/datalink/dlmstp.c \
	$(BACNET_CORE)/datalink/mstp.c \
	$(BACNET_CORE)/datalink/mstptext.c \
	$(BACNET_CORE)/dcc.c \
	$(BACNET_CORE)/iam.c \
	$(BACNET_CORE)/ihave.c \
	$(BACNET_CORE)/indtext.c \
	$(BACNET_CORE)/memcopy.c \
	$(BACNET_CORE)/hostnport.c \
	$(BACNET_CORE)/npdu.c \
	$(BACNET_CORE)/lighting.c \
	$(BACNET_CORE)/proplist.c \
	$(BACNET_CORE)/rd.c \
	$(BACNET_CORE)/reject.c \
	$(BACNET_CORE)/rp.c \
	$(BACNET_CORE)/rpm.c \
	$(BACNET_CORE)/wp.c \
	$(BACNET_CORE)/whohas.c \
	$(BACNET_CORE)/whois.c

SDK_CSRC = \
	$(SDK_DIR)/xmega/drivers/tc/tc.c \
	$(SDK_DIR)/common/services/clock/xmega/sysclk.c \
	$(SDK_DIR)/common/services/ioport/xmega/ioport_compat.c \
	$(SDK_DIR)/common/services/sleepmgr/xmega/sleepmgr.c \
	$(SDK_DIR)/xmega/boards/xmega_a3bu_xplained/init.c \
	$(SDK_DIR)/xmega/drivers/adc/adc.c \
	$(SDK_DIR)/xmega/drivers/adc/xmega_aau/adc_aau.c \
	$(SDK_DIR)/xmega/drivers/rtc32/rtc32.c \
	$(SDK_DIR)/common/drivers/nvm/xmega/xmega_nvm.c \
	$(SDK_DIR)/common/services/serial/usart_serial.c \
	$(SDK_DIR)/common/utils/stdio/read.c \
	$(SDK_DIR)/common/utils/stdio/write.c \
	$(SDK_DIR)/xmega/drivers/nvm/nvm.c \
	$(SDK_DIR)/xmega/drivers/usart/usart.c \
	$(SDK_DIR)/xmega/drivers/wdt/wdt.c \
	$(SDK_DIR)/xmega/services/pwm/pwm.c \
	$(SDK_DIR)/xmega/services/timeout/timeout.c

SDK_ASRC = \
	$(SDK_DIR)/xmega/drivers/cpu/ccp.S \
	$(SDK_DIR)/xmega/drivers/nvm/nvm_asm.S

## Include Directories
SDK_INCLUDES = -I$(SDK_DIR)/xmega/drivers/rtc32
SDK_INCLUDES += -I$(SDK_DIR)/xmega/drivers/pmic
SDK_INCLUDES += -I$(SDK_DIR)/xmega/boards/xmega_a3bu_xplained
SDK_INCLUDES += -I$(SDK_DIR)/xmega/utils/preprocessor
SDK_INCLUDES += -I$(SDK_DIR)/common/utils
SDK_INCLUDES += -I$(SDK_DIR)/common/services/sleepmgr
SDK_INCLUDES += -I$(SDK_DIR)/xmega/drivers/sleep
SDK_INCLUDES += -I$(SDK_DIR)/common/services/gpio
SDK_INCLUDES += -I$(SDK_DIR)/xmega/drivers/tc
SDK_INCLUDES += -I$(SDK_DIR)/xmega/drivers/adc
SDK_INCLUDES += -I$(SDK_DIR)/xmega/drivers/cpu
SDK_INCLUDES += -I$(SDK_DIR)/common/boards
SDK_INCLUDES += -I$(SDK_DIR)/common/services/ioport
SDK_INCLUDES += -I$(SDK_DIR)/xmega/drivers/nvm
SDK_INCLUDES += -I$(SDK_DIR)/xmega/boards
SDK_INCLUDES += -I$(SDK_DIR)/xmega/utils
SDK_INCLUDES += -I$(SDK_DIR)/xmega/drivers/wdt
SDK_INCLUDES += -I$(SDK_DIR)/common/services/clock
SDK_INCLUDES += -I$(SDK_DIR)/common/services/delay
SDK_INCLUDES += -I$(SDK_DIR)/xmega/drivers/usart
SDK_INCLUDES += -I$(SDK_DIR)/xmega/services/pwm
SDK_INCLUDES += -I$(SDK_DIR)/common/drivers/nvm
SDK_INCLUDES += -I$(SDK_DIR)/common/services/serial/xmega_usart
SDK_INCLUDES += -I$(SDK_DIR)/common/services/serial
SDK_INCLUDES += -I$(SDK_DIR)/common/utils/stdio/stdio_serial
SDK_INCLUDES += -I$(SDK_DIR)/xmega/services/timeout

INCLUDES = -I. -I./external/config -I$(BACNET_DIR) $(SDK_INCLUDES)

# Source to Object conversion
COBJ = $(CSRC:.c=.o)
BASICOBJ = $(BASICSRC:.c=.o)
COREOBJ = $(CORESRC:.c=.o)
SDK_COBJ = $(SDK_CSRC:.c=.o)
SDK_AOBJ = $(SDK_ASRC:.S=.o)

LIBRARY = lib$(TARGET).a

## Options common to compile, link and assembly rules
SDK_FLAGS = -mmcu=$(MCU)

OPTIMIZATION = -Os -g3

## Compile options common for all C compilation units.
BFLAGS = -DBACDL_MSTP
BFLAGS += -DMAX_APDU=128
BFLAGS += -DMAX_TSM_TRANSACTIONS=1
BFLAGS += -DMSTP_PDU_PACKET_COUNT=2
BFLAGS += -DMAX_ADDRESS_CACHE=32
BFLAGS += -DMAX_ANALOG_INPUTS=8
BFLAGS += -DBACNET_PROTOCOL_REVISION=9
BFLAGS += -DBACAPP_MINIMAL
BFLAGS += -DBACNET_USE_DOUBLE=0
ifeq (${LEGACY},true)
# disable deprecated function warnings for legacy builds
BFLAGS += -DBACNET_STACK_DEPRECATED_DISABLE
endif

CFLAGS = $(SDK_FLAGS)
# dead code removal
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -fpack-struct -fshort-enums -mrelax
CFLAGS += -Wall -gdwarf-2 $(BFLAGS) $(OPTIMIZATION)
CFLAGS += -fsigned-char -std=gnu99
CFLAGS += -fno-strict-aliasing
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wmissing-prototypes
CFLAGS += -Wpointer-arith
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d
CFLAGS += -DIOPORT_XMEGA_COMPAT
# silence some warnings
CFLAGS += -Wno-switch
CFLAGS += -std=gnu99

## Assembly specific flags
AFLAGS = $(SDK_FLAGS)
AFLAGS += $(SDK_INCLUDES)
AFLAGS += -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(SDK_FLAGS)
#dead code removal
#LDFLAGS += -Wl,-nostartfiles,-nostdlib
LDFLAGS += -Wl,--gc-sections,-static
LDFLAGS += -Wl,-Map=$(TARGET).map,-L.,-l$(TARGET)
#LDFLAGS += -Wl,-Map=$(TARGET).map

## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom
HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings

## Objects that must be built in order to link
OBJECTS = $(COBJ) $(BASICOBJ) $(SDK_COBJ) $(SDK_AOBJ)

## Build
TARGET_ELF=$(TARGET).elf

all: $(LIBRARY) $(TARGET_ELF) $(TARGET).hex $(TARGET).lst Makefile size

##Link
$(TARGET_ELF): $(OBJECTS) $(LIBRARY)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

%.hex: $(TARGET_ELF)
	$(OBJCOPY) -O ihex $(HEX_FLASH_FLAGS) $< $@

%.lst: $(TARGET_ELF)
	$(OBJDUMP) -h -S $< > $@

lib: $(LIBRARY)

$(LIBRARY): $(COREOBJ) Makefile
	$(AR) rcs $@ $(COREOBJ)
	$(OBJDUMP) --syms $@ > $(LIBRARY:.a=.lst)

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $*.c -o $@

.S.o:
	$(CC) -c $(AFLAGS) $*.S -o $@

size: ${TARGET_ELF}
	@echo
	@${SIZE} ${TARGET_ELF}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) $(TARGET_ELF) dep/*
	-rm -rf $(LIBRARY) $(COREOBJ) $(LIBRARY:.a=.lst)
	-rm -rf $(TARGET).hex $(TARGET).lst $(TARGET).map

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)
