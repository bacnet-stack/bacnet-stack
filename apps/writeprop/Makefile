#Makefile to build BACnet Application for the Linux Port

# Executable file name
TARGET = bacwp
# BACnet objects that are used with this app
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object
SRC = main.c \
	$(BACNET_OBJECT_DIR)/client/device-client.c \
	$(BACNET_OBJECT_DIR)/netport.c
# common build for command line apps
include ../common/Makefile
