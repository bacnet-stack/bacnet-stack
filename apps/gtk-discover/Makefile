# Makefile for BACnet GTK Device Discovery Application

TARGET = bacdiscover-gtk

BACNET_SRC_DIR = $(realpath ../../src)
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object
BACNET_CLIENT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/client

SOURCES = main.c \
	$(BACNET_OBJECT_DIR)/client/device-client.c \
	$(BACNET_OBJECT_DIR)/netport.c \
	$(BACNET_CLIENT_DIR)/bac-discover.c \
	$(BACNET_CLIENT_DIR)/bac-rw.c

# TARGET_EXT is defined in apps/Makefile as .exe or nothing
TARGET_BIN = ${TARGET}$(TARGET_EXT)

# GTK Configuration
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)
LFLAGS += $(GTK_LIBS)
CFLAGS += $(GTK_CFLAGS)
CFLAGS += -std=gnu99

# Check if GTK is available
GTK_CHECK = $(shell pkg-config --exists gtk+-3.0 && echo "yes")

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: check-gtk ${BACNET_LIB_TARGET} Makefile ${TARGET_BIN}

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
$(TARGET_BIN): $(OBJECTS) Makefile $(BACNET_LIB_TARGET)
	$(CC) ${PFLAGS} $(OBJECTS) $(LFLAGS) -o $@
	size $@
	cp $@ ../../bin

# Build BACnet library if needed
$(BACNET_LIB_TARGET):
	$(MAKE) -s -C $(BACNET_LIB_DIR) clean all

# Build object files
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

# Clean target
clean:
	rm -f core ${TARGET_BIN} ${OBJECTS} $(TARGET).map ${BACNET_LIB_TARGET}

# Install target (optional)
install: $(TARGET)
	install -D $(TARGET_BIN) $(DESTDIR)/usr/local/bin/$(TARGET_BIN)

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
	@echo "  - BACnet Stack library"
	@echo ""
	@echo "Example usage:"
	@echo "  make all BACDL=bip    # Build with BACnet/IP datalink"
	@echo "  make debug BACDL=mstp # Build debug version with MS/TP"

.PHONY: all clean install debug help check-gtk depend include
include: .depend
