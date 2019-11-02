#Makefile to build BACnet Application using GCC compiler

# Executable file name
TARGET = bacge
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object
SRC = main.c \
	$(BACNET_OBJECT_DIR)/client/device-client.c \
	$(BACNET_OBJECT_DIR)/netport.c
include ../common/Makefile
