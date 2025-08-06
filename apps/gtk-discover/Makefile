# Makefile for BACnet GTK Device Discovery Application

TARGET = bacnet-gtk-discovery

BACNET_SRC_DIR = $(realpath ../../src)
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object
BACNET_CLIENT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/client

SOURCES = main.c \
	$(BACNET_OBJECT_DIR)/client/device-client.c \
	$(BACNET_OBJECT_DIR)/netport.c \
	$(BACNET_CLIENT_DIR)/bac-discover.c \
	$(BACNET_CLIENT_DIR)/bac-rw.c

# BACnet Stack Configuration
BACNET_LIB_DIR = $(realpath ../lib)
BACNET_LIB_NAME = bacnet
BACNET_LIB_TARGET = $(BACNET_LIB_DIR)/lib$(BACNET_LIB_NAME).a
BACNET_LIB = -L$(BACNET_LIB_DIR) -l$(BACNET_LIB_NAME)

# Default datalink (can be overridden)
BACDL ?= bip
ifeq (${BACDL},bip)
BACDL_DEFINE=-DBACDL_BIP=1
endif
ifeq (${BACDL},mstp)
BACDL_DEFINE=-DBACDL_MSTP=1
endif
ifeq (${BACDL},ethernet)
BACDL_DEFINE=-DBACDL_ETHERNET=1
endif

# GTK Configuration
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)

# Check if GTK is available
GTK_CHECK = $(shell pkg-config --exists gtk+-3.0 && echo "yes")

# Compiler and flags
CC ?= gcc
CFLAGS += -Wall -Wextra -std=c99 $(GTK_CFLAGS) $(BACDL_DEFINE)
LDFLAGS += $(GTK_LIBS) $(BACNET_LIB) -lm

# Include paths
INCLUDES = -I../../src

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: check-gtk $(TARGET)

# Check if GTK is available
check-gtk:
ifneq ($(GTK_CHECK),yes)
	@echo "Error: GTK+ 3.0 development libraries not found!"
	@echo "Please install GTK+ 3.0 development packages:"
	@echo "  Ubuntu/Debian: sudo apt-get install libgtk-3-dev"
	@echo "  Fedora/RHEL:   sudo yum install gtk3-devel"
	@echo "  openSUSE:      sudo zypper install gtk3-devel"
	@exit 1
endif

# Build the target
$(TARGET): $(OBJECTS) $(BACNET_LIB_TARGET)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# Build object files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build BACnet library if needed
$(BACNET_LIB_TARGET):
	$(MAKE) -C $(BACNET_LIB_DIR) all

# Clean target
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install target (optional)
install: $(TARGET)
	install -D $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)

# Debug target
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build the GTK BACnet Discovery application"
	@echo "  debug      - Build with debug symbols and DEBUG flag"
	@echo "  clean      - Remove built files"
	@echo "  install    - Install the application to /usr/local/bin"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Prerequisites:"
	@echo "  - GTK+ 3.0 development libraries"
	@echo "  - BACnet stack library"
	@echo ""
	@echo "Example usage:"
	@echo "  make all BACDL=bip    # Build with BACnet/IP datalink"
	@echo "  make debug BACDL=mstp # Build debug version with MS/TP"

.PHONY: all clean install debug help check-gtk
