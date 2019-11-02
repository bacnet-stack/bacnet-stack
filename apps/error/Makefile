#Makefile to build BACnet Application for the Linux Port

# tools - only if you need them.
# Most platforms have this already defined
# CC = gcc

TARGET = bacerror
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object
SRC = main.c \
	$(BACNET_OBJECT_DIR)/client/device-client.c \
	$(BACNET_OBJECT_DIR)/netport.c
include ../common/Makefile
