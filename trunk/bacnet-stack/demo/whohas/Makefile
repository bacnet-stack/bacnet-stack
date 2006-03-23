#Makefile to build BACnet Application for the Linux Port
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -O2 -g
# Note: you can strip out symbols using the strip command
# to get an idea of how big the compile really is.
#DEFINES = -DBACFILE=1 -DBACDL_ETHERNET=1
#DEFINES = -DBACFILE=1 -DBACDL_ARCNET=1
#DEFINES = -DBACFILE=1 -DBACDL_MSTP=1 
DEFINES = -DBACFILE=1 -DTSM_ENABLED=0 -DBACDL_BIP=1
BACNET_PORT = ../../ports/linux
BACNET_OBJECT = ../object
BACNET_HANDLER = ../handler
BACNET_ROOT = ../..
INCLUDES = -I$(BACNET_ROOT) -I$(BACNET_PORT) -I$(BACNET_OBJECT) -I$(BACNET_HANDLER) 

CFLAGS  = -Wall -g $(INCLUDES) $(DEFINES)

TARGET = bacwh

SRCS = main.c \
       $(BACNET_ROOT)/rd.c \
       $(BACNET_PORT)/bip-init.c \
       $(BACNET_ROOT)/bip.c \
       $(BACNET_HANDLER)/txbuf.c \
       $(BACNET_HANDLER)/noserv.c \
       $(BACNET_HANDLER)/h_whois.c \
       $(BACNET_HANDLER)/h_ihave.c \
       $(BACNET_HANDLER)/h_rp.c \
       $(BACNET_HANDLER)/s_whohas.c  \
       $(BACNET_HANDLER)/s_ihave.c  \
       $(BACNET_OBJECT)/device.c \
       $(BACNET_OBJECT)/ai.c \
       $(BACNET_OBJECT)/ao.c \
       $(BACNET_OBJECT)/bi.c \
       $(BACNET_OBJECT)/bo.c \
       $(BACNET_OBJECT)/lsp.c \
       $(BACNET_OBJECT)/bacfile.c \
       $(BACNET_ROOT)/filename.c \
       $(BACNET_ROOT)/rp.c \
       $(BACNET_ROOT)/wp.c \
       $(BACNET_ROOT)/bacdcode.c \
       $(BACNET_ROOT)/bacapp.c \
       $(BACNET_ROOT)/bacprop.c \
       $(BACNET_ROOT)/bacstr.c \
       $(BACNET_ROOT)/bactext.c \
       $(BACNET_ROOT)/indtext.c \
       $(BACNET_ROOT)/bigend.c \
       $(BACNET_ROOT)/whois.c \
       $(BACNET_ROOT)/iam.c \
       $(BACNET_ROOT)/whohas.c \
       $(BACNET_ROOT)/ihave.c \
       $(BACNET_ROOT)/datalink.c \
       $(BACNET_ROOT)/address.c \
       $(BACNET_ROOT)/arf.c \
       $(BACNET_ROOT)/dcc.c \
       $(BACNET_ROOT)/abort.c \
       $(BACNET_ROOT)/reject.c \
       $(BACNET_ROOT)/bacerror.c \
       $(BACNET_ROOT)/apdu.c \
       $(BACNET_ROOT)/npdu.c

OBJS = ${SRCS:.c=.o}

all: ${TARGET}
 
${TARGET}: ${OBJS}
	${CC} -o $@ ${OBJS} 

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@
	
depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend
	
clean:
	rm -rf core ${TARGET} $(OBJS) *.bak ports/linux/*.bak *.1 *.ini

include: .depend

