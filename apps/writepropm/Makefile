#Makefile to build BACnet Application for the Linux Port

# tools - only if you need them.
# Most platforms have this already defined
# CC = gcc

# Executable file name
TARGET = bacwpm

# BACnet objects that are used with this app
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object
SRC = main.c \
	$(BACNET_OBJECT_DIR)/client/device-client.c \
	$(BACNET_OBJECT_DIR)/netport.c
# common build for command line apps
include ../common/Makefile
