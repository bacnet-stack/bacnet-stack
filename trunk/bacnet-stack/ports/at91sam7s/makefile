# Makefile for AT91SAM7S evaluation kit with RS-485 Transceiver on UART0
# Written by Steve Karg <skarg@users.sourceforge.net> 06-Aug-2007

TARGET=bacnet

# Tools
CC=arm-elf-gcc
OBJCOPY=arm-elf-objcopy
OBJDUMP=arm-elf-objdump

LDSCRIPT=at91sam7s256.ld

BACNET_FLAGS = -DBACDL_MSTP=1 -DPRINT_ENABLED=0 -DBIG_ENDIAN=0 -DMAX_APDU=480 -DDLMSTP_TEST
INCLUDES = -I. -I../.. -I../../demo/handler -I../../demo/object 
OPTIMIZATION = -O0
CFLAGS = -fno-common $(OPTIMIZATION) $(INCLUDES) $(BACNET_FLAGS) -Wall -g
#  -Wa,<options> Pass comma-separated <options> on to the assembler
AFLAGS = -Wa,-ahls,-mapcs-32,-adhlns=$(<:.s=.lst)
#  -Wl,<options> Pass comma-separated <options> on to the linker
LDFLAGS = -nostartfiles -Wl,-Map=$(TARGET).map,-lc,-lgcc,-lm,-T$(LDSCRIPT)
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

CORESRC = device.c \
	ai.c \
	av.c \
	bi.c \
	bv.c \
	h_rp.c \
	h_wp.c \
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
	../../demo\handler\txbuf.c \
	../../demo\handler\h_whois.c \
	../../demo\handler\h_rd.c \
	../../demo\handler\h_dcc.c

#CSRC = $(PORTSRC) $(CORESRC)
CSRC = $(PORTSRC)

AOBJ = $(ASRC:.s=.o)
COBJ = $(CSRC:.c=.o)

all: $(TARGET).bin
	$(OBJDUMP) $(ODFLAGS) $(TARGET).elf > $(TARGET).dmp

$(TARGET).bin:  $(TARGET).elf
	$(OBJCOPY) $(TARGET).elf $(CPFLAGS) $(TARGET).bin

$(TARGET).elf: $(COBJ) $(AOBJ) Makefile
	$(CC) $(CFLAGS) $(AOBJ) $(COBJ) $(LDFLAGS) -o $@ 

$(COBJ): %.o : %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(AOBJ): %.o : %.s Makefile
	$(CC) -c $(AFLAGS) $< -o $@

clean:
	touch Makefile
	rm $(COBJ) $(AOBJ) 
	rm $(TARGET).elf $(TARGET).bin $(TARGET).dmp $(TARGET).map
	rm *.lst
