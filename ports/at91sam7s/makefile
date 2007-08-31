# Makefile for AT91SAM7S evaluation kit with RS-485 Transceiver on UART0
# Written by Steve Karg <skarg@users.sourceforge.net> 06-Aug-2007

TARGET=bacnet

# Tools
CC=arm-elf-gcc
OBJCOPY=arm-elf-objcopy
OBJDUMP=arm-elf-objdump
AR=arm-elf-ar
SIZE = arm-elf-size

LDSCRIPT=at91sam7s256.ld

BACNET_FLAGS = -DBACDL_MSTP
BACNET_FLAGS += -DMAX_TSM_TRANSACTIONS=0
BACNET_FLAGS += -DPRINT_ENABLED=0
BACNET_FLAGS += -DMAX_APDU=480
#BACNET_FLAGS += -DDLMSTP_TEST

INCLUDES = -I. -I../.. -I../../demo/handler -I../../demo/object
#OPTIMIZATION = -O0
OPTIMIZATION = -Os
CFLAGS = -fno-common $(OPTIMIZATION) $(INCLUDES) $(BACNET_FLAGS) -Wall -g
LIBRARY = lib$(TARGET).a
#  -Wa,<options> Pass comma-separated <options> on to the assembler
AFLAGS = -Wa,-ahls,-mapcs-32,-adhlns=$(<:.s=.lst)
#  -Wl,<options> Pass comma-separated <options> on to the linker
LIBRARIES=-lc,-lgcc,-lm,-L=.,-l$(TARGET)
LDFLAGS = -nostartfiles -Wl,-nostdlib,-Map=$(TARGET).map,$(LIBRARIES),-T$(LDSCRIPT)
CPFLAGS = --output-target=binary
ODFLAGS	= -x --syms

ASRC = crt.s

PORTSRC = main.c \
	timer.c \
	isr.c \
	init.c \
	blinker.c \
	rs485.c \
	dlmstp.c \
	../../crc.c \
	../../mstp.c

DEMOSRC = ai.c \
	av.c \
	bi.c \
	bv.c \
	h_rp.c \
	h_wp.c \
	device.c \
	../../demo/handler/txbuf.c \
	../../demo/handler/h_whois.c \
	../../demo/handler/h_rd.c \
	../../demo/handler/h_dcc.c

CORESRC = ../../npdu.c \
	../../bacint.c \
	../../apdu.c \
	../../bacdcode.c \
	../../bacaddr.c \
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

.c.o:
	$(CC) -c $(CFLAGS) $*.c -o $@

.s.o:
	$(CC) -c $(AFLAGS) $*.s -o $@

clean:
	touch Makefile
	rm $(COBJ) $(AOBJ) $(COREOBJ)
	rm $(TARGET).elf $(TARGET).bin $(TARGET).dmp $(TARGET).map
	rm $(LIBRARY)
	rm *.lst
