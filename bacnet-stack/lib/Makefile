#Makefile to build BACnet Library for the Linux Port

# tools
CC = gcc
AR = ar

# target
TARGET = bacnet
LIBRARY = lib$(TARGET).a

# configuration
BACNET_DEFINES=-DPRINT_ENABLED=1
#BACDL_DEFINE=-DBACDL_ETHERNET=1
#BACDL_DEFINE=-DBACDL_ARCNET=1
BACDL_DEFINE=-DBACDL_BIP=1
DEFINES = $(BACNET_DEFINES) $(BACDL_DEFINE)
# directories
BACNET_PORT = ../ports/linux
BACNET_OBJECT = ../demo/object
BACNET_HANDLER = ../demo/handler
BACNET_ROOT = ../.
# compiler configuration
INCLUDES = -I$(BACNET_ROOT) -I$(BACNET_PORT) -I$(BACNET_OBJECT) -I$(BACNET_HANDLER) 
CFLAGS  = -Wall -g $(INCLUDES) $(DEFINES)

CORE_SRC = \
	$(BACNET_ROOT)/apdu.c \
	$(BACNET_ROOT)/npdu.c \
	$(BACNET_ROOT)/bacdcode.c \
	$(BACNET_ROOT)/bacint.c \
	$(BACNET_ROOT)/bacapp.c \
	$(BACNET_ROOT)/bacprop.c \
	$(BACNET_ROOT)/bacstr.c \
	$(BACNET_ROOT)/bactext.c \
	$(BACNET_ROOT)/datetime.c \
	$(BACNET_ROOT)/indtext.c \
	$(BACNET_ROOT)/bigend.c \
	$(BACNET_ROOT)/arf.c \
	$(BACNET_ROOT)/awf.c \
	$(BACNET_ROOT)/cov.c \
	$(BACNET_ROOT)/dcc.c \
	$(BACNET_ROOT)/iam.c \
	$(BACNET_ROOT)/ihave.c \
	$(BACNET_ROOT)/rd.c \
	$(BACNET_ROOT)/rp.c \
	$(BACNET_ROOT)/rpm.c \
	$(BACNET_ROOT)/timesync.c \
	$(BACNET_ROOT)/whohas.c \
	$(BACNET_ROOT)/whois.c \
	$(BACNET_ROOT)/wp.c \
	$(BACNET_ROOT)/abort.c \
	$(BACNET_ROOT)/reject.c \
	$(BACNET_ROOT)/bacerror.c \
	$(BACNET_ROOT)/filename.c \
	$(BACNET_ROOT)/tsm.c \
	$(BACNET_ROOT)/bacaddr.c \
	$(BACNET_ROOT)/address.c \
	$(BACNET_ROOT)/version.c

HANDLER_SRC = \
	$(BACNET_HANDLER)/txbuf.c \
	$(BACNET_HANDLER)/noserv.c \
	$(BACNET_HANDLER)/h_whois.c \
	$(BACNET_HANDLER)/h_iam.c  \
	$(BACNET_HANDLER)/h_rp.c \
	$(BACNET_HANDLER)/h_rp_a.c \
	$(BACNET_HANDLER)/h_rpm.c \
	$(BACNET_HANDLER)/h_wp.c  \
	$(BACNET_HANDLER)/h_arf.c  \
	$(BACNET_HANDLER)/h_arf_a.c  \
	$(BACNET_HANDLER)/h_awf.c  \
	$(BACNET_HANDLER)/h_rd.c  \
	$(BACNET_HANDLER)/h_dcc.c  \
	$(BACNET_HANDLER)/h_ts.c  \
	$(BACNET_HANDLER)/h_whohas.c  \
	$(BACNET_HANDLER)/h_ihave.c  \
	$(BACNET_HANDLER)/s_arfs.c \
	$(BACNET_HANDLER)/s_awfs.c \
	$(BACNET_HANDLER)/s_dcc.c \
	$(BACNET_HANDLER)/s_ihave.c \
	$(BACNET_HANDLER)/s_rd.c \
	$(BACNET_HANDLER)/s_rp.c  \
	$(BACNET_HANDLER)/s_ts.c \
	$(BACNET_HANDLER)/s_whohas.c \
	$(BACNET_HANDLER)/s_whois.c  \
	$(BACNET_HANDLER)/s_wp.c

OBJECT_SRC = \
	$(BACNET_OBJECT)/device.c \
	$(BACNET_OBJECT)/ai.c \
	$(BACNET_OBJECT)/ao.c \
	$(BACNET_OBJECT)/av.c \
	$(BACNET_OBJECT)/bi.c \
	$(BACNET_OBJECT)/bo.c \
	$(BACNET_OBJECT)/bv.c \
	$(BACNET_OBJECT)/lc.c \
	$(BACNET_OBJECT)/lsp.c \
	$(BACNET_OBJECT)/mso.c \
	$(BACNET_OBJECT)/bacfile.c

PORT_SRC = \
	$(BACNET_PORT)/bip-init.c \
	$(BACNET_PORT)/ethernet.c \
	$(BACNET_PORT)/arcnet.c \
	$(BACNET_PORT)/dlmstp.c \
	$(BACNET_PORT)/rs485.c \
	$(BACNET_ROOT)/mstp.c \
	$(BACNET_ROOT)/crc.c \
	$(BACNET_ROOT)/bip.c

SRCS = ${CORE_SRC} ${DEMO_SRC} ${PORT_SRC} ${HANDLER_SRC} ${OBJECT_SRC}

OBJS = ${SRCS:.c=.o}

all: $(LIBRARY)
 
lib: $(LIBRARY)

$(LIBRARY): $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

clean:
	rm -rf core $(OBJS) $(LIBRARY)

include: .depend
