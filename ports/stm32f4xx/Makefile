#+-------------------------------------------------------------------------------------------------+
#| GNU Make script for STM32F4xx microcontroller                                                               |
#+-------------------------------------------------------------------------------------------------+
TARGET=bacnet

BACNET_DIR = $(realpath ../..)
BACNET_SRC := $(BACNET_DIR)/src
BACNET_CORE := $(BACNET_SRC)/bacnet
BACNET_BASIC := $(BACNET_CORE)/basic
BACNET_INCLUDE := $(BACNET_SRC)
PLATFORM_DIR = $(realpath .)
LIBRARY_STM32 = $(realpath ./external/STM32F4xx_StdPeriph_Driver/src)
LIBRARY_STM32_INCLUDES = $(realpath ./external/STM32F4xx_StdPeriph_Driver/inc)
LIBRARY_CMSIS = $(realpath ./external/CMSIS)

CSTACK_TOOL := $(BACNET_DIR)/tools/avstack/avstack.pl
MEMAP_TOOL := $(BACNET_DIR)/tools/memap/memap.py

INCLUDES = -I$(PLATFORM_DIR)
INCLUDES += -I$(LIBRARY_STM32_INCLUDES)
INCLUDES += -I$(LIBRARY_CMSIS)
INCLUDES += -I$(BACNET_INCLUDE)

PLATFORM_SRC = \
	$(PLATFORM_DIR)/main.c \
	$(PLATFORM_DIR)/bacnet.c \
	$(PLATFORM_DIR)/device.c \
	$(PLATFORM_DIR)/led.c \
	$(PLATFORM_DIR)/netport.c \
	$(PLATFORM_DIR)/rs485.c \
	$(PLATFORM_DIR)/mstimer-init.c \
	$(PLATFORM_DIR)/program-ubasic.c \
	$(PLATFORM_DIR)/ubasic-port.c \
	$(PLATFORM_DIR)/stm32f4xx_it.c \
	$(PLATFORM_DIR)/system_stm32f4xx.c

BASIC_SRC = \
	$(BACNET_BASIC)/npdu/h_npdu.c \
	$(BACNET_BASIC)/object/ai.c \
	$(BACNET_BASIC)/object/ao.c \
	$(BACNET_BASIC)/object/av.c \
	$(BACNET_BASIC)/object/bi.c \
	$(BACNET_BASIC)/object/bo.c \
	$(BACNET_BASIC)/object/bv.c \
	$(BACNET_BASIC)/object/ms-input.c \
	$(BACNET_BASIC)/object/mso.c \
	$(BACNET_BASIC)/object/msv.c \
	$(BACNET_BASIC)/object/bacfile.c \
	$(BACNET_BASIC)/object/program.c \
	$(BACNET_BASIC)/program/ubasic/ubasic.c \
	$(BACNET_BASIC)/program/ubasic/tokenizer.c \
	$(BACNET_BASIC)/service/h_apdu.c \
	$(BACNET_BASIC)/service/h_arf.c \
	$(BACNET_BASIC)/service/h_dcc.c \
	$(BACNET_BASIC)/service/h_rd.c \
	$(BACNET_BASIC)/service/h_rp.c \
	$(BACNET_BASIC)/service/h_rpm.c \
	$(BACNET_BASIC)/service/h_ts.c \
	$(BACNET_BASIC)/service/h_whohas.c \
	$(BACNET_BASIC)/service/h_whois.c \
	$(BACNET_BASIC)/service/h_wp.c \
	$(BACNET_BASIC)/service/h_noserv.c \
	$(BACNET_BASIC)/service/s_iam.c \
	$(BACNET_BASIC)/service/s_ihave.c \
	$(BACNET_BASIC)/sys/bsramfs.c \
	$(BACNET_BASIC)/sys/debug.c \
	$(BACNET_BASIC)/sys/datetime_mstimer.c \
	$(BACNET_BASIC)/sys/days.c \
	$(BACNET_BASIC)/sys/dst.c \
	$(BACNET_BASIC)/sys/ringbuf.c \
	$(BACNET_BASIC)/sys/fifo.c \
	$(BACNET_BASIC)/sys/keylist.c \
	$(BACNET_BASIC)/sys/mstimer.c \
	$(BACNET_BASIC)/tsm/tsm.c

BACNET_SRC = \
	$(BACNET_CORE)/abort.c \
	$(BACNET_CORE)/arf.c \
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
	$(BACNET_CORE)/bactimevalue.c \
	$(BACNET_CORE)/calendar_entry.c \
	$(BACNET_CORE)/dailyschedule.c \
	$(BACNET_CORE)/datetime.c \
	$(BACNET_CORE)/dcc.c \
	$(BACNET_CORE)/hostnport.c \
	$(BACNET_CORE)/iam.c \
	$(BACNET_CORE)/ihave.c \
	$(BACNET_CORE)/indtext.c \
	$(BACNET_CORE)/lighting.c \
	$(BACNET_CORE)/memcopy.c \
	$(BACNET_CORE)/npdu.c \
	$(BACNET_CORE)/proplist.c \
	$(BACNET_CORE)/rd.c \
	$(BACNET_CORE)/reject.c \
	$(BACNET_CORE)/rp.c \
	$(BACNET_CORE)/rpm.c \
	$(BACNET_CORE)/special_event.c \
	$(BACNET_CORE)/shed_level.c \
	$(BACNET_CORE)/timer_value.c \
	$(BACNET_CORE)/timestamp.c \
	$(BACNET_CORE)/timesync.c \
	$(BACNET_CORE)/weeklyschedule.c \
	$(BACNET_CORE)/whohas.c \
	$(BACNET_CORE)/whois.c \
	$(BACNET_CORE)/wp.c

DATALINK_SRC = \
	$(BACNET_CORE)/datalink/cobs.c \
	$(BACNET_CORE)/datalink/crc.c \
	$(BACNET_CORE)/datalink/dlmstp.c \
	$(BACNET_CORE)/datalink/mstp.c \
	$(BACNET_CORE)/datalink/mstptext.c

STM32_SRC = \
	$(LIBRARY_STM32)/stm32f4xx_adc.c \
	$(LIBRARY_STM32)/stm32f4xx_can.c \
	$(LIBRARY_STM32)/stm32f4xx_crc.c \
	$(LIBRARY_STM32)/stm32f4xx_dac.c \
	$(LIBRARY_STM32)/stm32f4xx_dbgmcu.c \
	$(LIBRARY_STM32)/stm32f4xx_dcmi.c \
	$(LIBRARY_STM32)/stm32f4xx_dma.c \
	$(LIBRARY_STM32)/stm32f4xx_exti.c \
	$(LIBRARY_STM32)/stm32f4xx_flash.c \
	$(LIBRARY_STM32)/stm32f4xx_fsmc.c \
	$(LIBRARY_STM32)/stm32f4xx_gpio.c \
	$(LIBRARY_STM32)/stm32f4xx_i2c.c \
	$(LIBRARY_STM32)/stm32f4xx_iwdg.c \
	$(LIBRARY_STM32)/stm32f4xx_misc.c \
	$(LIBRARY_STM32)/stm32f4xx_pwr.c \
	$(LIBRARY_STM32)/stm32f4xx_rcc.c \
	$(LIBRARY_STM32)/stm32f4xx_rng.c \
	$(LIBRARY_STM32)/stm32f4xx_rtc.c \
	$(LIBRARY_STM32)/stm32f4xx_sdio.c \
	$(LIBRARY_STM32)/stm32f4xx_spi.c \
	$(LIBRARY_STM32)/stm32f4xx_syscfg.c \
	$(LIBRARY_STM32)/stm32f4xx_tim.c \
	$(LIBRARY_STM32)/stm32f4xx_usart.c \
	$(LIBRARY_STM32)/stm32f4xx_wwdg.c \
	$(LIBRARY_STM32)/syscalls.c

CSRC = $(PLATFORM_SRC)
CSRC += $(BASIC_SRC)
CSRC += $(BACNET_SRC)
CSRC += $(DATALINK_SRC)
CSRC += $(STM32_SRC)

ASRC = $(LIBRARY_CMSIS)/gcc_ride7/startup_stm32f4xx.s

#Set the toolchain command names (only the ones needed are defined)
# sudo apt install gcc-arm-none-eabi
PREFIX ?= arm-none-eabi-

CC = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
AR = $(PREFIX)ar
NM = $(PREFIX)nm
SIZE = $(PREFIX)size

LDSCRIPT = $(PLATFORM_DIR)/stm32f4xx.ld

MCU_FLAGS = -mcpu=cortex-m4
MCU_FLAGS += -mthumb -mabi=aapcs
MCU_FLAGS += -mno-thumb-interwork
MCU_FLAGS += -DUSE_STDPERIPH_DRIVER

OPTIMIZE_FLAGS := -Os -ggdb
OPTIMIZE_FLAGS += -DNDEBUG

BACNET_FLAGS = -DBACDL_MSTP=
# note: MS/TP extended frames can be up to MAX_APDU=1476 bytes
BACNET_FLAGS += -DMAX_APDU=480
BACNET_FLAGS += -DBIG_ENDIAN=0
BACNET_FLAGS += -DMAX_TSM_TRANSACTIONS=0
BACNET_FLAGS += -DMAX_CHARACTER_STRING_BYTES=64
BACNET_FLAGS += -DMAX_OCTET_STRING_BYTES=64
BACNET_FLAGS += -DBACFILE
BACNET_FLAGS += -DBACAPP_MINIMAL
BACNET_FLAGS += -DBACNET_STACK_DEPRECATED_DISABLE
# if called from root Makefile, PRINT was already defined
BACNET_FLAGS += -UPRINT_ENABLED
BACNET_FLAGS += -DPRINT_ENABLED=0
ifeq (${SHIELD},)
BACNET_FLAGS += -DRS485_DFR0259_ENABLED=1
endif
ifeq (${SHIELD},dfr0259)
BACNET_FLAGS += -DRS485_DFR0259_ENABLED=1
endif
ifeq (${SHIELD},linksprite)
BACNET_FLAGS += -DRS485_LINKSPRITE_ENABLED=1
endif
ifeq (${LEGACY},true)
# disable deprecated function warnings for legacy builds
BACNET_FLAGS += -DBACNET_STACK_DEPRECATED_DISABLE
endif

CFLAGS += $(MCU_FLAGS)
CFLAGS += $(OPTIMIZE_FLAGS)
CFLAGS += $(BACNET_FLAGS)
CFLAGS += $(INCLUDES)
# enable garbage collection of unused functions and data to shrink binary
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
# enable stack usage tracking
CFLAGS += -fstack-usage
# function calls will not use any special __builtin_xx to allow debug/linking
CFLAGS += -fno-builtin
# place uninitialized global variables in the data section of the object file.
CFLAGS += -fno-common
# enable all relevant warnings that find bugs
WARNING_ALL := -Wall -Wextra -Wfloat-equal -Wconversion
WARNING_ALL += -Wredundant-decls -Wswitch-default -pedantic
# don't warn about conversion, sign, compares, long long and attributes
# since they are common in embedded
WARNING_ALL += -Wno-sign-conversion -Wno-conversion -Wno-sign-compare
WARNING_ALL += -Wno-long-long -Wno-attributes
# don't warn about implicit fallthrough since it's common in network protocols
WARNING_ALL += -Wno-implicit-fallthrough
# don't warn about missing braces since GCC is over-achiever for this
WARNING_ALL += -Wno-missing-braces
# don't warn about missing prototypes since STM32 library doesn't have some
WARNING_ALL += -Wno-missing-prototypes
# don't warn about array subscript being char
WARNING_ALL += -Wno-char-subscripts
# FIXME later
WARNING_ALL += -Wno-unused-parameter
#WARNING_ALL += -Werror
CFLAGS += $(WARNING_ALL)

#  -Wa,<options> Pass comma-separated <options> on to the assembler
AFLAGS = -Wa,-ahls,-mapcs-32,-adhlns=$(<:.s=.lst)

#Set the linker flags
#  -Wl,<options> Pass comma-separated <options> on to the linker
LIBRARIES=-lc,-lgcc,-lm
LDFLAGS = -nostartfiles
LDFLAGS += -Wl,-nostdlib,-Map=$(TARGET).map,$(LIBRARIES),-T$(LDSCRIPT)
# dead code removal
LDFLAGS += -Wl,--gc-sections,-static
ODFLAGS	= -x --syms

AOBJ = $(ASRC:.s=.o)
COBJ = $(CSRC:.c=.o)
CSTACK = $(CSRC:.c=.su)

all: $(TARGET).bin $(TARGET).hex $(TARGET).elf
	$(OBJDUMP) $(ODFLAGS) $(TARGET).elf > $(TARGET).dmp
	$(SIZE) $(TARGET).elf

$(TARGET).bin:  $(TARGET).elf
	$(OBJCOPY) $(TARGET).elf --output-target=binary $(TARGET).bin

$(TARGET).hex:  $(TARGET).elf
	$(OBJCOPY) $(TARGET).elf --output-target=ihex $(TARGET).hex

$(TARGET).elf: $(COBJ) $(AOBJ) Makefile
	$(CC) $(CFLAGS) $(AOBJ) $(COBJ) $(LDFLAGS) -o $@

.PHONY: ram-usage
ram-usage:
	@$(NM) -t d -S --size-sort $(TARGET).elf 1> $(TARGET).nm
	@echo "=ADDRESS= ==SIZE== = ==VARIABLE NAME=="
	@tail $(TARGET).nm

.PHONY: cstack
cstack:
	@$(CSTACK_TOOL) $(COBJ) 2> /dev/null 1> $(TARGET).su
	@head -n 25 $(TARGET).su

.PHONY: memap
memap:
	# memmap needs Python and PrettyPrint and IntelHex
	# sudo apt install python3-prettytable python3-intelhex
	$(MEMAP_TOOL) -t GCC_ARM $(TARGET).map

# GDB using st-util (GDB server for ST Link)
GDB_PORT = 3333
.PHONY: debug
debug:
	st-util --listen $(GDB_PORT)

# Note: STLink is built into Nucleo board
OPENOCD_FLAGS = -f interface/stlink.cfg -f target/stm32f4x.cfg
# GDB using openocd (GDB server for ST Link)
# sudo apt install openocd
.PHONY: openocd
openocd:
	openocd $(OPENOCD_FLAGS)

flash: $(TARGET).elf
	openocd $(OPENOCD_FLAGS) -c "program $< verify reset" -c "shutdown"

# graphical GDB debugging tool
# note: relies on .gdbinit containing:
# file "./release/target.elf"
# target extended-remote :3333
# load
# break main
# sudo apt install ddd
.PHONY: ddd
ddd:
	ddd --debugger $(GDB) $(realpath $(TARGET).elf)

# web based graphical GDB debugging tool
# pip install gdbgui
.PHONY: gdbgui
gdbgui:
	gdbgui -g $(GDB) $(realpath $(TARGET).elf)

# sudo apt install stlink-tools
# note: might require adding rule file to /etc/udev/rules.d/
# SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", MODE="0666"
# SUBSYSTEM=="usb_device", ATTRS{idVendor}=="0483", MODE="0666"
# and reloading with
# $ sudo udevadm control --reload-rules
# or unplug and plugin the stlink after adding the rules file
install: $(TARGET).bin
	st-flash write $(TARGET).bin 0x08000000

.c.o:
	$(CC) -c $(OPTIMIZATION) $(CFLAGS) $*.c -o $@

.s.o:
	$(CC) -c $(AFLAGS) $*.s -o $@

.PHONY: clean
clean:
	-rm -rf $(COBJ) $(AOBJ) $(COREOBJ)
	-rm -rf $(CSTACK) $(CEXPAND)
	-rm -rf $(TARGET).elf $(TARGET).bin $(TARGET).dmp $(TARGET).map
	-rm -rf $(TARGET).su $(TARGET).nm
	-rm -rf *.lst

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)
